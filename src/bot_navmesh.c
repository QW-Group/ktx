
// Navigation mesh

#ifdef BOT_SUPPORT

#include "g_local.h"
#include "fb_globals.h"

#define NAVMESH_LINK_INVALID -1
#define NAVMESH_LINK_FORWARD  0
#define NAVMESH_LINK_BACK     1
#define NAVMESH_LINK_LEFT     2
#define NAVMESH_LINK_RIGHT    3
#define NAVMESH_LINK_ABOVE    4
#define NAVMESH_LINK_BELOW    5

static const char* NAVMESH_DIRECTION_NAMES[] = {
	"forward", "back", "left", "right", "above", "below"
};
static int NAVMESH_ANGLES[] = { 0, 180, 90, -90, 0, 0 };
static vec3_t NAVMESH_DIRECTIONS[] = {
	{  1,  0, 0 },
	{ -1,  0, 0 },
	{  0,  1, 0 },
	{  0, -1, 0 }
};
static int grid_size = 16;
#define NAVMESH_MOVE_DISTANCE_RATIO 1
#define NAVMESH_MIN_DISTANCE 8
static qbool auto_start = false;

#define NAVMESH_MAX_BLOCKS_PER_FRAME 16

typedef struct navmesh_block_s {
	vec3_t min;
	vec3_t max;
	int links[6];
	qbool complete;
} navmesh_block_t;

static float last_announce = 0;
static navmesh_block_t blocks[65535];
static int next_block = 0;
static int todo = 0;
static gedict_t* testent = 0;

static vec3_t investigate_area_min = { 350, -1675, 215 };
static vec3_t investigate_area_max = { 545, -1540, 260 };

static void ClearNavMesh(void)
{
	next_block = 0;
	todo = 0;
}

static void DumpNavMesh(void)
{
	int i;
	fileHandle_t file;
	char* entityFile = cvar_string("k_entityfile");
	char fileName[128];
	char date[64];

	if (!QVMstrftime(date, sizeof(date), "%Y%m%d-%H%M%S", 0))
		snprintf(date, sizeof(date), "%d", i_rnd(0, 9999));

	snprintf(fileName, sizeof(fileName), "bots/maps/%s[%s].nmr", strnull(entityFile) ? g_globalvars.mapname : entityFile, date);
	file = std_fwopen(fileName);
	if (file == -1) {
		G_bprint(PRINT_HIGH, "Failed to open navmesh file.  Check bots/maps/ directory is writable\n");
		return;
	}

	for (i = 0; i < next_block; ++i) {
		std_fprintf(file, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", i, PASSINTVEC3(blocks[i].min), PASSINTVEC3(blocks[i].max), blocks[i].links[0], blocks[i].links[1], blocks[i].links[2], blocks[i].links[3], blocks[i].links[4], blocks[i].links[5]);
	}
	std_fclose(file);
	G_bprint(PRINT_HIGH, "Created file %s\n", fileName);
}

static qbool PointInBlock(vec3_t min, vec3_t max, float x, float y, float z)
{
	if (x < min[0] || x > max[0])
		return false;
	if (y < min[1] || y > max[1])
		return false;
	if (z < min[2] || z > max[2])
		return false;
	return true;
}

static navmesh_block_t* FindBlockByPoint(vec3_t p, float distance)
{
	int i = 0, j = 0;
	for (i = 0; i < next_block; ++i) {
		vec3_t min, max, size;
		VectorSubtract(blocks[i].max, blocks[i].min, size);
		VectorMA(blocks[i].max, -distance, size, min);
		VectorMA(blocks[i].min, distance, size, max);
		if (PointInBlock(min, max, PASSVEC3(p))) {
			return &blocks[i];
		}
	}
	return NULL;
}

static int AddBlockToNavMesh(navmesh_block_t* current, gedict_t* edict, int direction, qbool fallen)
{
	navmesh_block_t* block;
	vec3_t point;

	VectorAdd(edict->s.v.absmin, edict->s.v.absmax, point);
	VectorScale(point, 0.5f, point);

	if (block = FindBlockByPoint(point, 0.9f)) {
		if (current && current->links[direction] == -1) {
			current->links[direction] = fallen ? -2 : (block - blocks);
		}
		return (block - blocks);
	}

	if (next_block >= sizeof(blocks) / sizeof(blocks[0])) {
		return -1;
	}

	// Add the block to the todo list
	block = &blocks[next_block];
	memset(block->links, -1, sizeof(int) * 6);
	VectorCopy(edict->s.v.absmin, block->min);
	VectorCopy(edict->s.v.absmax, block->max);
	if (current && current->links[direction] == -1) {
		current->links[direction] = fallen ? -2 : next_block;
	}
	++next_block;
	return  (block - blocks);
}

static void MarkBlockAsComplete(navmesh_block_t* block)
{
	block->complete = true;
	++todo;
}

static void ProcessNavMeshNode(gedict_t* testent)
{
	navmesh_block_t* block = &blocks[todo];
	int direction = 0;
	float max_distance = (block->max[1] - block->min[1]) * NAVMESH_MOVE_DISTANCE_RATIO;
	vec3_t block_center;
	qbool debug = false;

	VectorAdd(block->min, block->max, block_center);
	VectorScale(block_center, 0.5f, block_center);

	debug = todo == 0; //PointInBlock(investigate_area_min, investigate_area_max, PASSVEC3(block_center));

	if (debug) {
		G_bprint(PRINT_HIGH, "Block %d: %d %d %d\n", block - blocks, PASSINTVEC3(block_center));
	}
	for (direction = NAVMESH_LINK_FORWARD; direction <= NAVMESH_LINK_RIGHT; ++direction) {
		if (block->links[direction] == -1) {
			vec3_t new_center;
			qbool fallen;
			float distance = max_distance;

			if (debug) {
				G_bprint(PRINT_HIGH, "> Trying %s\n", NAVMESH_DIRECTION_NAMES[direction]);
			}

			// Try and walk up some stairs?
			qbool anyWalkmoveSuccess = false;
			while (distance > 0) {
				qbool success = false;

				setorigin(testent, PASSVEC3(block_center));
				testent->s.v.flags = FL_ONGROUND;
				testent->s.v.solid = SOLID_SLIDEBOX;
				success = walkmove(testent, NAVMESH_ANGLES[direction], distance);
				if (debug) {
					G_bprint(PRINT_HIGH, "> Attempting %d degrees @ %f (%s) [flags now %d]\n", NAVMESH_ANGLES[direction], distance, NAVMESH_DIRECTION_NAMES[direction], (int) testent->s.v.flags);
				}

				if (success) {
					int n;
					if (debug) {
						G_bprint(PRINT_HIGH, ">> Walkmove successful: new origin %d %d %d\n", PASSINTVEC3(testent->s.v.origin));
					}
					droptofloor(testent);
					if (debug) {
						G_bprint(PRINT_HIGH, ">> dropped to floor: new origin %d %d %d\n", PASSINTVEC3(testent->s.v.origin));
					}
					fallen = (block_center[2] - testent->s.v.origin[2] > 18);

					n = AddBlockToNavMesh(block, testent, direction, fallen);
					if (debug) {
						G_bprint(PRINT_HIGH, ">> added, result = %d\n", n);
					}
					distance -= 4;
					anyWalkmoveSuccess = true;
				}
				else {
					--distance;
				}
			}

			if (false) { //! anyWalkmoveSuccess) {
				// Simply move, drop to floor and see if that's valid
				qbool success = false;
				distance = max_distance;
				while (distance >= NAVMESH_MIN_DISTANCE) {
					success = false;
					for (int zoffset = 18; zoffset >= 0 && !success; zoffset -= 4) {
						new_center[0] = block_center[0] + NAVMESH_DIRECTIONS[direction][0] * distance;
						new_center[1] = block_center[1] + NAVMESH_DIRECTIONS[direction][1] * distance;
						new_center[2] = block_center[2] + zoffset;
						if (debug) {
							G_bprint(PRINT_HIGH, ">> attempting simple move: %d %d %d\n", PASSINTVEC3(new_center));
						}
						setorigin(testent, PASSVEC3(new_center));
						testent->s.v.flags = FL_ONGROUND;
						testent->s.v.solid = SOLID_SLIDEBOX;
						droptofloor(testent);
						if (debug) {
							G_bprint(PRINT_HIGH, ">> after dropping to floor: %d %d %d\n", PASSINTVEC3(new_center));
						}

						success = walkmove(testent, 0, 0);
					}

					if (success) {
						if (debug) {
							G_bprint(PRINT_HIGH, ">> walkmove success, origin %d %d %d flags %d\n", PASSINTVEC3(testent->s.v.origin), (int)testent->s.v.flags);
						}

						fallen = (new_center[2] - testent->s.v.origin[2] > 18);
						AddBlockToNavMesh(block, testent, direction, fallen);

						distance -= 4;
					}
					else {
						--distance;
					}
				}
			}
		}
	}

	MarkBlockAsComplete(block);
}

static void BotNavMeshBegin(void)
{
	gedict_t* dmspawn;
	char* initial_objects[] = {
		"info_player_deathmatch",
		"item_armor1", "item_armor2", "item_armor3",
		"item_artifact_invulnerability", "item_artifact_envirosuit", "item_artifact_invisibility", "item_artifact_super_damage",
		"weapon_supershotgun", "weapon_nailgun", "weapon_supernailgun", "weapon_grenadelauncher", "weapon_rocketlauncher", "weapon_lightning"
	};
	int i;
	char* classname;
	vec3_t temp;
	vec3_t entmin, entmax;

	ClearNavMesh();
	if (!testent) {
		testent = spawn();
		VectorSet(entmin, VEC_HULL_MIN[0], VEC_HULL_MIN[1], VEC_HULL_MIN[2]);
		VectorSet(entmax, VEC_HULL_MAX[0], VEC_HULL_MAX[1], VEC_HULL_MAX[2]);
		setsize(testent, PASSVEC3(entmin), PASSVEC3(entmax));
		testent->s.v.classname = "navmesh_testent";
	}

	for (i = 0; i < sizeof(initial_objects) / sizeof(initial_objects[0]); ++i) {
		classname = initial_objects[i];

		for (dmspawn = world; dmspawn = ez_find(dmspawn, classname); ) {
			VectorCopy(dmspawn->s.v.origin, temp);
			temp[2] = -4095;
			setorigin(testent, PASSVEC3(temp));
			traceline(dmspawn->s.v.origin[0], dmspawn->s.v.origin[1], dmspawn->s.v.origin[2] + 18, PASSVEC3(testent->s.v.origin), true, dmspawn);
			if (g_globalvars.trace_fraction == 1) {
				G_bprint(PRINT_HIGH, "> %s @ [%d %d %d]: failed to find start point\n", dmspawn->s.v.classname, PASSINTVEC3(dmspawn->s.v.origin));
				continue;
			}
			temp[2] = (int)(g_globalvars.trace_endpos[2] + 50);
			setorigin(testent, PASSVEC3(temp));
			droptofloor(testent);

			testent->s.v.flags = FL_ONGROUND_PARTIALGROUND;
			if (walkmove(testent, 0, 0)) {
				if (g_globalvars.trace_fraction == 1) {
					G_bprint(PRINT_HIGH, "> %s @ [%d %d %d]: walkmove failed\n", dmspawn->s.v.classname, PASSINTVEC3(dmspawn->s.v.origin));
					continue;
				}
				G_bprint(PRINT_HIGH, "> %s @ [%d %d %d]: walkmove success\n", dmspawn->s.v.classname, PASSINTVEC3(testent->s.v.origin));
				AddBlockToNavMesh(NULL, testent, NAVMESH_LINK_INVALID, false);
				last_announce = g_globalvars.time;
			}
		}
	}
}

void BotNavMeshBuild(void)
{
	if (self->ct != ctSpec) {
		G_sprint(self, PRINT_HIGH, "You must be a spectator to use this command\n");
		return;
	}

	BotNavMeshBegin();

	if (next_block == 0) {
		G_sprint(self, PRINT_HIGH, "Error: unable to start, no blocks to process\n");
	}
	else {
		G_sprint(self, PRINT_HIGH, "Started generation, blocks\n");
	}
}

void BotNavMeshFrameThink(void)
{
	if (auto_start && next_block == 0) {
		BotNavMeshBegin();
	}

	if (testent && todo < next_block) {
		int block_countdown = NAVMESH_MAX_BLOCKS_PER_FRAME;
		int block_count = next_block;
		while (todo < next_block && block_countdown >= 0) {
			ProcessNavMeshNode(testent);
			--block_countdown;
		}

		if (todo >= next_block) {
			// Finished...
			for (todo = 0; todo < next_block; ++todo) {
				if (!blocks[todo].complete) {
					G_bprint(PRINT_HIGH, "Sanity check failed: block %d\n", todo);
				}
			}
			DumpNavMesh();
			ent_remove(testent);
			testent = NULL;
			last_announce = 0;
		}
		else if (g_globalvars.time - last_announce > 1) {
			G_bprint(PRINT_HIGH, "Navmesh progress: %d/%d blocks\n", todo, next_block);
			last_announce = g_globalvars.time;
		}
	}
}

#endif 
