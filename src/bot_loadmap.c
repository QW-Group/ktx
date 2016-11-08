// Converted from .qc on 05/02/2016

#include "g_local.h"
#include "fb_globals.h"

static qbool map_supported = false;

void check_marker (gedict_t* self, gedict_t* other);
qbool LoadBotRoutingFromFile (void);
qbool pickup_true(void);

// fixme: also in doors.c
#define SECRET_OPEN_ONCE 1	// stays open

typedef struct fb_mapping_s {
	char* name;
	fb_void_func_t func;
} fb_mapping_t;

fb_spawn_t* ItemSpawnFunction (int i);

static fb_mapping_t maps[] = {
	{ "dm6", map_dm6 },
	{ "dm4", map_dm4 },
	{ "e1m2", map_e1m2 },
	{ "aerowalk", map_aerowalk },
	{ "ztndm3", map_ztndm3 },
	{ "dm3", map_dm3 },
	{ "spinev2", map_spinev2 },
	{ "pkeg1", map_pkeg1 },
	{ "ultrav", map_ultrav },
	{ "frobodm2", map_frobodm2 },
	{ "amphi2", map_amphi2 },
	{ "povdmm4", map_povdmm4 },
	{ "ukooldm2", map_ukooldm2 },
	{ "ztndm4", map_ztndm4 },
	{ "ztndm5", map_ztndm5 },
};

void InvalidMap() {
	G_sprint(self, 2, g_globalvars.mapname);
	G_sprint(self, 2, "Map is unsupported for bots.\n");
	G_sprint(self, 2, "Valid maps for bots are:\n");
	G_sprint(self, 2, "dm3, dm4, dm6, e1m2, frobodm2, aerowalk, spinev2, pkeg1, ultrav, ztndm3, amphi2, povdmm4\n");
}

static void fb_spawn_button(gedict_t* ent) {
	AddToQue(ent);

	if (ent->s.v.health) {
		//Add_takedamage(ent);
	}
	else {
		BecomeMarker(ent);
		adjust_view_ofs_z(ent);
	}
}

static void fb_spawn_changelevel(gedict_t* ent) {
	AddToQue(ent);
}

static void fb_spawn_spawnpoint(gedict_t* ent) {
	AddToQue(ent);
	ent->s.v.solid = SOLID_TRIGGER;
	ent->s.v.touch = (func_t) marker_touch;
	ent->s.v.flags = FL_ITEM;
	BecomeMarker(ent);
	setsize(ent, -65, -65, -24, 65, 65, 32);
	SetVector (ent->s.v.view_ofs, 80, 80, 24);
	ent->fb.pickup = pickup_true;
	adjust_view_ofs_z(ent);
}

static void fb_spawn_door(gedict_t* ent) {
	gedict_t* original = ent;
	vec3_t position;

	VectorScale (original->s.v.mins, 0.5, position);
	VectorMA (position, 0.5, original->s.v.maxs, position);
	position[2] = min (original->s.v.mins[2], original->s.v.maxs[2]) + 24;
	ent = CreateMarker(PASSVEC3(position));
	ent->s.v.classname = "door";
	ent->fb.door_entity = original;
	ent->s.v.solid = SOLID_NOT;     // this will be set to SOLID_TRIGGER if MARKER_DOOR_TOUCHABLE flag set

	if (ent->fb.wait < 0) {
		// TODO: ?
		return;
	}
	else if ((int)ent->s.v.spawnflags & SECRET_OPEN_ONCE) {
		// TODO: ?
		return;
	}
	else {
		if (ent->s.v.health) {
			//Add_takedamage(ent);
		}

		adjust_view_ofs_z(ent);
		//BecomeMarker(ent);
	}
}

static void fb_spawn_simple(gedict_t* ent) {
	AddToQue(ent);

	if (ent->fb.wait < 0) {
		// Remove... so ignore?
		return;
	}
	else {
		if (ent->s.v.health) {
			//Add_takedamage(ent);
		}
	}
}

static void fb_spawn_trigger_teleport (gedict_t* ent)
{
	AddToQue (ent);

	//Com_Printf ("fb_trigger_tele([%f %f %f] > [%f %f %f])\n", PASSVEC3 (ent->s.v.mins), PASSVEC3 (ent->s.v.maxs));
	VectorSet (ent->fb.virtual_mins, ent->s.v.mins[0] - 18, ent->s.v.mins[1] - 18, ent->s.v.mins[2] - 34);
	VectorSet (ent->fb.virtual_maxs, ent->s.v.maxs[0] + 18, ent->s.v.maxs[1] + 18, ent->s.v.maxs[2] + 26);
	setsize(ent, ent->s.v.mins[0] - 32, ent->s.v.mins[1] - 32, ent->s.v.mins[2], ent->s.v.maxs[0] + 32, ent->s.v.maxs[1] + 32, ent->s.v.maxs[2]);
	VectorSet (ent->s.v.view_ofs, 0.5 * (ent->s.v.absmax[0] - ent->s.v.absmin[0]), 0.5 * (ent->s.v.absmax[1] - ent->s.v.absmin[1]), 0.5 * (ent->s.v.absmax[2] - ent->s.v.absmin[2]));
	adjust_view_ofs_z(ent);
	BecomeMarker(ent);
}

static void fb_spawn_teleport_destination_touch (void)
{
	if (IsMarkerFrame() && other->ct == ctPlayer) {
		check_marker (self, other);
	}
}

static void fb_spawn_teleport_destination (gedict_t* ent)
{
	AddToQue (ent);

	ent->s.v.solid = SOLID_TRIGGER;
	ent->s.v.touch = (func_t) fb_spawn_teleport_destination_touch;
	ent->s.v.flags = FL_ITEM;
	BecomeMarker (ent);
	setsize (ent, -65, -65, -24 - 27, 65, 65, 32);  // -27 extra to get back to floor
	VectorSet(ent->s.v.view_ofs, 80, 80, 24);
	ent->fb.pickup = pickup_true;
	adjust_view_ofs_z (ent);
}

static fb_spawn_t stdSpawnFunctions[] = {
	{ "func_button", fb_spawn_button },
	{ "trigger_changelevel", fb_spawn_simple },
	{ "info_player_deathmatch", fb_spawn_spawnpoint },
	{ "door", fb_spawn_door },
	{ "trigger_multiple", fb_spawn_simple },
	{ "trigger_once", fb_spawn_simple },
	{ "trigger_secret", fb_spawn_simple },
	{ "trigger_counter", fb_spawn_simple },
	{ "info_teleport_destination", fb_spawn_teleport_destination },
	{ "trigger_teleport", fb_spawn_trigger_teleport },
	{ "trigger_setskill", fb_spawn_simple },
	{ "trigger_onlyregistered", fb_spawn_simple },
	{ "trigger_hurt", fb_spawn_simple },
	{ "trigger_push", fb_spawn_simple },
	{ "train", fb_spawn_simple },
	{ "plat", fb_spawn_simple }
};

static void CreateItemMarkers() {
	// Old frogbot method was to call during item spawns, we just 
	//    catch up afterwards once we know the map is valid
	gedict_t* item;

	for (item = world; item = nextent(item); ) {
		int i = 0;
		qbool found = false;

		if (item->fb.index)
			continue;

		// check for item spawn
		for (i = 0; i < ItemSpawnFunctionCount(); ++i) {
			fb_spawn_t* spawn = ItemSpawnFunction (i);
			if (streq(spawn->name, item->s.v.classname)) {
				spawn->func(item);
				found = true;
				break;
			}
		}

		// check for std spawn (world items like buttons etc)
		if (! found) {
			for (i = 0; i < sizeof(stdSpawnFunctions) / sizeof(stdSpawnFunctions[0]); ++i) {
				if (streq(stdSpawnFunctions[i].name, item->s.v.classname)) {
					stdSpawnFunctions[i].func(item);
					break;
				}
			}
		}
	}
}

// After all markers have been created, re-process items
static void AssignVirtualGoals (void)
{
	gedict_t* item;

	for (item = world; item = nextent(item); ) {
		int i = 0;

		for (i = 0; i < ItemSpawnFunctionCount(); ++i) {
			fb_spawn_t* spawn = ItemSpawnFunction (i);
			if (streq(spawn->name, item->s.v.classname)) {
				AssignVirtualGoal_apply (item);
				break;
			}
		}
	}
}

// Item creation functions 
static void PlaceItemFB (gedict_t* ent)
{
	vec3_t new_size = { 49, 49, 0 };

	if (ent->fb.fixed_size[0])
		new_size[0] = ent->fb.fixed_size[0] / 2 - 15;
	if (ent->fb.fixed_size[1])
		new_size[1] = ent->fb.fixed_size[1] / 2 - 15;
	if (ent->fb.fixed_size[2])
		new_size[2] = ent->fb.fixed_size[2] / 2 - 0;

	ent->s.v.movetype = MOVETYPE_NONE;
	setsize(ent, ent->s.v.mins[0] - new_size[0], ent->s.v.mins[1] - new_size[1], ent->s.v.mins[2], ent->s.v.maxs[0] + new_size[0], ent->s.v.maxs[1] + new_size[1], ent->s.v.maxs[2]);
	adjust_view_ofs_z(ent);
	VectorSet (ent->fb.virtual_mins, ent->s.v.absmin[0] + (new_size[0] - 49 + 32), ent->s.v.absmin[1] + (new_size[1] - 49 + 32), ent->s.v.absmin[2] - (new_size[2] - 0 + 33));
	VectorSet (ent->fb.virtual_maxs, ent->fb.virtual_mins[0] + 96, ent->fb.virtual_mins[1] + 96, ent->fb.virtual_mins[2] + 114);
}

// KTX has in-built modifications to several maps - frogbot routing relies on entity order so we have
//   to customise again here.  Called after all markers created, but before traveltime calculations
static void CustomiseFrogbotMap (void)
{
	gedict_t* ent = NULL;

	// KTX may have added a quad, so to keep routes compatible with PR1-version, we add it as a marker after others
	if (streq(g_globalvars.mapname, "aerowalk"))
	{
		Com_Printf ("---\n");
		gedict_t* quad = find (world, FOFCLSN, "item_artifact_super_damage");
		if (quad) {
			gedict_t* nearest_marker = LocateMarker (quad->s.v.origin);
			int i = 0;

			StartItemFB (quad);

			// Quad is in same zone as nearest marker, and linked by the first path that's valid
			SetZone (nearest_marker->fb.Z_ + 1, quad->fb.index + 1);
			SetGoalForMarker (20, quad);
			for (i = 0; i < NUMBER_PATHS; ++i) {
				if (nearest_marker->fb.paths[i].next_marker == 0) {
					SetMarkerPath (nearest_marker->fb.index + 1, i, quad->fb.index + 1);
					SetMarkerPath (quad->fb.index + 1, 0, nearest_marker->fb.index + 1);
					break;
				}
			}
		}
		Com_Printf ("---\n");
	}

	// We stopped it from removing the telespawn earlier on...
	if (!cvar ("k_end_tele_spawn") && streq ("end", g_globalvars.mapname)) {
		vec3_t      TS_ORIGIN = { -392, 608, 40 }; // tele spawn
		gedict_t*   p = NULL;
		gedict_t*   m = NULL;

		for (p = world; (p = find (p, FOFCLSN, "info_player_deathmatch")); ) {
			if (VectorCompare (p->s.v.origin, TS_ORIGIN)) {
				p->s.v.classname = "info_player_deathmatch_removed";

				// Remove any spawn marker
				for (m = world; m = find (m, FOFCLSN, "spawnpoint"); ) {
					if (m->k_lastspawn == p) {
						ent_remove (m);
						break;
					}
				}
				break;
			}
		}
	}

	// Expand bounding box of all items
	for (ent = world; ent = nextent (ent); ) {
		if (streq (ent->s.v.classname, "marker")) {
			vec3_t mins = { -65, -65, -24 };
			vec3_t maxs = {  65,  65,  32 };
			int i;

			for (i = 0; i < 3; ++i) {
				if (ent->fb.fixed_size[i]) {
					mins[i] = -ent->fb.fixed_size[i] / 2 - (i < 2 ? 15 : 0);
					maxs[i] = ent->fb.fixed_size[i] / 2 - (i < 2 ? 15 : 0);
				}
			}
			setsize(ent, PASSVEC3(mins), PASSVEC3(maxs));
		}
		else if ((int)ent->s.v.flags & FL_ITEM) {
			PlaceItemFB (ent);
		}
	}
}

void LoadMap(void) {
	// Need to do this anyway, otherwise teleporters will be broken
	CreateItemMarkers();

	// If we have a .bot file, use that
	if (LoadBotRoutingFromFile ()) {
		map_supported = true;
		CustomiseFrogbotMap ();
		AssignVirtualGoals ();
		AllMarkersLoaded();
		return;
	}
	else {
		// Fall-back to built-in support
		int i = 0;
		for (i = 0; i < sizeof(maps) / sizeof(maps[0]); ++i) {
			if (streq(g_globalvars.mapname, maps[i].name)) {
				maps[i].func();
				map_supported = true;
				CustomiseFrogbotMap ();
				AssignVirtualGoals ();
				AllMarkersLoaded();
				return;
			}
		}
	}

	// Need this in order to change bounding boxes
	CustomiseFrogbotMap ();
}

qbool FrogbotsCheckMapSupport (void)
{
	if (map_supported)
		return true;

	G_sprint (self, 2, "Map %s not supported for bots\n", g_globalvars.mapname);
	return false;
}

