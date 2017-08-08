/*
bot/bothazd.qc

Copyright (C) 1997-1999 Robert 'Frog' Field
Copyright (C) 2003 3d[Power]
Copyright (C) 2000-2007 ParboiL
*/

// Converted from .qc on 05/02/2016

#ifdef BOT_SUPPORT

#include "g_local.h"
#include "fb_globals.h"

// A lot of this is the bot 'cheating'?..
#define ARROW_TIME_AFTER_TELEPORT 0.20  // was 0.5

void GrenadeExplode (void);
void BotPathCheck (gedict_t* self, gedict_t* touch_marker);

// FIXME: Local globals
static float first_trace_fraction = 0;
static vec3_t first_trace_plane_normal = { 0 };

static const char* FallType (int fallType)
{
	const char* fallNames[] = { "FALSE", "BLOCKED", "LAND", "DEATH", "???" };

	return fallNames[(int)bound (0, fallType, sizeof (fallNames) / sizeof (fallNames[0]))];
}

static float StandardFallHeight (gedict_t* self)
{
	// Calculate fall height
	float fallheight = self->s.v.origin[2] - 38; // FIXME: Why 38
	if (self->fb.linked_marker) {
		float min_second = self->fb.linked_marker->s.v.absmin[2] + self->fb.linked_marker->s.v.view_ofs[2] - 36; // FIXME: Why 36?

		fallheight = min (min_second, fallheight);
	}
	return fallheight;
}

// Input globals: first_trace_fraction, output: first_trace_fraction & first_trace_plane_normal
static void TestTopBlock(gedict_t* self, vec3_t last_clear_point, vec3_t testplace) {
	float xDelta[4] = { -16,  16, -16,  16 };
	float yDelta[4] = { -16, -16,  16,  16 };
	int i = 0;
	for (i = 0; i < 4; ++i)
	{
		traceline(last_clear_point[0] + xDelta[i], last_clear_point[1] + yDelta[i], last_clear_point[2] + 32, 
			testplace[0] + xDelta[i], testplace[1] + yDelta[i], testplace[2] + 32, true, self);
		if (g_globalvars.trace_fraction != 1) {
			if (g_globalvars.trace_plane_normal[2] <= 0) {
				if (first_trace_fraction > g_globalvars.trace_fraction) {
					first_trace_fraction = g_globalvars.trace_fraction;
					VectorCopy(g_globalvars.trace_plane_normal, first_trace_plane_normal);
				}
			}
		}
	}
}

// Input globals first_trace_fraction, output first_trace_fraction & first_trace_plane_normal
static void TestBottomBlock(gedict_t* self, vec3_t last_clear_point, vec3_t testplace) {
	float xDelta[4] = { -16,  16, -16,  16 };
	float yDelta[4] = { -16, -16,  16,  16 };
	int i = 0;
	for (i = 0; i < 4; ++i)
	{
		traceline(last_clear_point[0] + xDelta[i], last_clear_point[1] + yDelta[i], last_clear_point[2] - 24, 
			testplace[0] + xDelta[i], testplace[1] + yDelta[i], testplace[2] - 24, true, self);
		if (g_globalvars.trace_fraction != 1) {
			if (g_globalvars.trace_plane_normal[2] >= 0) {
				if (first_trace_fraction > g_globalvars.trace_fraction) {
					first_trace_fraction = g_globalvars.trace_fraction;
					VectorCopy(g_globalvars.trace_plane_normal, first_trace_plane_normal);
				}
			}
		}
	}
}

// 
static void AvoidEdge(gedict_t* self) {
	vec3_t dir_forward;

	VectorCopy(self->fb.oldvelocity, dir_forward);
	dir_forward[2] = 0;
	if (dir_forward[0] || dir_forward[1] || dir_forward[2]) {
		vec3_t dir_move;
		self->fb.oldvelocity[0] = self->fb.oldvelocity[1] = 0;  // lavacheat always enabled
		VectorScale(dir_forward, -1, dir_move);
		NewVelocityForArrow(self, dir_move, "AvoidEdge");
		self->fb.arrow_time2 = self->fb.arrow_time;
	}
}

static void HazardTeleport(gedict_t* teleport, gedict_t* teleported_player) {
	gedict_t* plr;

	teleport->fb.arrow_time = max (teleport->fb.arrow_time, g_globalvars.time + ARROW_TIME_AFTER_TELEPORT);
	teleport->fb.arrow_time_setby = teleported_player;

	// If teleported bot already has flag set, don't set for others
	//   (otherwise two bots can keep triggering the same logic and blocking each other)
	if (teleported_player->fb.path_state & DELIBERATE_BACKUP) {
		return;
	}

	// Make all other bots wait a brief period before going through tele
	for (plr = world; (plr = find_plr (plr)); ) {
		// Tell other bots directly next to the teleport to back up a while
		if (plr->isBot && plr != teleported_player && plr->fb.linked_marker == teleport) {
			plr->fb.path_state |= DELIBERATE_BACKUP;
			plr->fb.linked_marker_time = g_globalvars.time + 0.1;
		}
	}
}

// This sets arrow_time on closest markers, to indicate danger
static void ExplodeAlert(vec3_t org, float next_time) {
	gedict_t* marker;
	gedict_t* tele;

	// Find all map markers close to this point
	for (marker = world; (marker = trap_findradius(marker, org, 256)); ) {
		if (marker->fb.fl_marker) {
			// Would the grenade hurt the marker?
			traceline(org[0], org[1], org[2], marker->s.v.absmin[0] + marker->s.v.view_ofs[0], marker->s.v.absmin[1] + marker->s.v.view_ofs[1], marker->s.v.absmin[2] + marker->s.v.view_ofs[2], true, marker);
			if (g_globalvars.trace_fraction == 1) {
				// Mark the current time
				marker->fb.arrow_time = next_time;

				for (tele = world; (tele = ez_find (tele, "trigger_teleport")); ) {
					// If this teleport takes us to the marker close to the grenade, set arrow_time
					if (!strnull (tele->target)) {
						gedict_t* target = find (world, FOFS (targetname), tele->target);
						if (target == marker) {
							tele->fb.arrow_time = max (tele->fb.arrow_time, next_time);
						}
					}
				}
			}
		}
	}
}

// This is essentially just to call ExplodeAlert(origin) every 0.05s, until the grenade explodes
static void GrenadeAlert(void) {
	self->s.v.nextthink = g_globalvars.time + 0.05;
	self->think = (func_t) GrenadeAlert;
	if (self->fb.frogbot_nextthink <= g_globalvars.time) {
		self->think = (func_t) GrenadeExplode;
		self->s.v.nextthink = g_globalvars.time;
	}
	ExplodeAlert(self->s.v.origin, self->s.v.nextthink);
}

static void Missile_Remove() {
	self->s.v.owner = NUM_FOR_EDICT(world);
	ent_remove (self);
}

static void RocketAlert(void) {
	vec3_t end_point;

	self->s.v.nextthink = g_globalvars.time + 0.5;
	if (self->fb.frogbot_nextthink <= g_globalvars.time) {
		self->think = (func_t) Missile_Remove;
	}
	VectorMA (self->s.v.origin, 600, self->s.v.velocity, end_point);
	traceline(PASSVEC3(self->s.v.origin), PASSVEC3(end_point), true, PROG_TO_EDICT(self->s.v.owner));
	ExplodeAlert(g_globalvars.trace_endpos, 0.5);
}

// 
static qbool JumpInWater(vec3_t testpoint) {
	return (trap_pointcontents(testpoint[0], testpoint[1], testpoint[2] - 64) == CONTENT_WATER);
}

static int FallSpotGround(vec3_t testplace, float fallheight) {
	VectorCopy(testplace, dropper->s.v.origin);
	dropper->s.v.flags = FL_ONGROUND_PARTIALGROUND;

	if (walkmove(dropper, 0, 0)) {
		int content;

		if (dropper->s.v.origin[2] <= MapDeathHeight ())
			return FALL_DEATH;

		// If we didn't fall into water...
		if (!JumpInWater(dropper->s.v.origin) && !droptofloor(dropper)) {
			VectorCopy(testplace, dropper->s.v.origin);
			dropper->s.v.origin[2] -= 256;
			if (!droptofloor(dropper)) {
				return FALL_DEATH;
			}
		}

		// Check we didn't fall into lava...
		content = trap_pointcontents(dropper->s.v.origin[0], dropper->s.v.origin[1], dropper->s.v.origin[2] - 24);
		if (content == CONTENT_LAVA)
			return FALL_DEATH;
		
		if (dropper->s.v.origin[2] < fallheight)
			return FALL_LAND;

		return FALL_FALSE;
	}

	if (dropper->s.v.origin[2] <= MapDeathHeight ())
		return FALL_DEATH;

	return FALL_BLOCKED;
}

static int FallSpotAir (gedict_t* self, vec3_t testplace, float fallheight) {
	vec3_t final_origin;
	int content;

	VectorCopy(testplace, dropper->s.v.origin);
	dropper->s.v.flags = FL_ONGROUND_PARTIALGROUND;

	if (walkmove(dropper, 0, 0)) {
		if (dropper->s.v.origin[2] <= MapDeathHeight ())
			return FALL_DEATH;

		if (!JumpInWater (dropper->s.v.origin) && dropper->s.v.origin[2] > testplace[2]) {
			return FALL_BLOCKED;
		}
	}
	else {
		return FALL_BLOCKED;
	}

	if (!JumpInWater(dropper->s.v.origin) && !(droptofloor(dropper))) {
		VectorCopy(testplace, dropper->s.v.origin);
		dropper->s.v.origin[2] -= 256;

		if (! droptofloor(dropper))
			return FALL_DEATH;
	}

	VectorCopy (dropper->s.v.origin, final_origin);
	content = trap_pointcontents(final_origin[0], final_origin[1], final_origin[2] - 24);
	if (content == CONTENT_LAVA) {
		return FALL_DEATH;
	}

	if (final_origin[2] <= MapDeathHeight ()) {
		return FALL_DEATH;
	}

	if (dropper->s.v.origin[2] < fallheight) {
		return FALL_LAND;
	}

	return FALL_FALSE;
}

// last_clear_point is output variable
static qbool CanJumpOver(gedict_t* self, vec3_t jump_origin, vec3_t jump_velocity, float fallheight, int current_fallspot) {
	int tries = 0;
	float last_clear_hor_speed = 0;
	vec3_t last_clear_hor_velocity;
	vec3_t last_clear_point;
	vec3_t last_clear_velocity;
	vec3_t accel_direction;

	VectorCopy(jump_origin, last_clear_point);
	VectorCopy(jump_velocity, last_clear_velocity);
	VectorCopy(last_clear_velocity, last_clear_hor_velocity);
	last_clear_hor_velocity[2] = 0;

	last_clear_hor_speed = vlen(last_clear_hor_velocity);
	normalize (last_clear_hor_velocity, accel_direction);
	last_clear_velocity[2] = jump_velocity[2] - (12800 / last_clear_hor_speed);   // 12800 = sv_gravity * 16?

	//G_bprint (2, "CanJumpOver([%d %d %d], [%d %d %d], %f)\n", PASSINTVEC3 (jump_origin), PASSINTVEC3 (jump_velocity), fallheight);
	for (tries = 0; tries < 20; ++tries) {
		int fall = 0;
		vec3_t testplace;

		//G_bprint (2, "%02d: last_clear_point = %f %f %f\n", tries, PASSVEC3(last_clear_point));
		if (last_clear_point[2] < fallheight) {
			//G_bprint (2, "    last_clear_point_z < fallheight (%f), quitting\n", fallheight);
			return false;
		}

		VectorMA (last_clear_point, (32 / last_clear_hor_speed), last_clear_velocity, testplace);

		fall = FallSpotAir(self, testplace, fallheight);
		//G_bprint (2, "    Testplace: %d %d %d = %s\n", PASSINTVEC3 (testplace), FallType(fall));
		if (fall == FALL_BLOCKED) {
			first_trace_fraction = 1;
			TestTopBlock(self, last_clear_point, testplace);
			TestBottomBlock(self, last_clear_point, testplace);

			if (first_trace_fraction != 1) {
				float dotProduct;

				// If we didn't hit anything above/below then it was a side-on collision, so keep going
				VectorMA (last_clear_point, (first_trace_fraction * 32 / last_clear_hor_speed), last_clear_velocity, testplace);

				dotProduct = DotProduct(first_trace_plane_normal, last_clear_velocity);
				VectorMA (last_clear_velocity, -dotProduct, first_trace_plane_normal, last_clear_velocity);

				VectorCopy(last_clear_velocity, last_clear_hor_velocity);
				last_clear_hor_velocity[2] = 0;
				last_clear_hor_speed = vlen(last_clear_hor_velocity);
				normalize (last_clear_hor_velocity, accel_direction);

				VectorMA (testplace, (32 / last_clear_hor_speed) * (1 - first_trace_fraction), last_clear_velocity, testplace);
			}
			fall = FallSpotAir (self, testplace, fallheight);
			//G_bprint (2, "    Blocked testplace: %f %f %f = %d\n", PASSVEC3 (testplace), fall);
		}

		if (fall == FALL_BLOCKED) {
			return false;
		}
		
		if (fall > current_fallspot) {
			last_clear_velocity[2] = last_clear_velocity[2] - (25600 / last_clear_hor_speed);
			VectorCopy(testplace, last_clear_point);
		}
		else {
			qbool do_jump = true;
			if ((int)self->s.v.flags & FL_ONGROUND) {
				gedict_t* test_enemy;

				for (test_enemy = world; (test_enemy = find_plr (test_enemy)); ) {
					if (test_enemy->fb.T & UNREACHABLE) {
						if (VectorDistance(test_enemy->s.v.origin, testplace) <= 84) {
							do_jump = false;
							break;
						}
					}
				}
			}
			return do_jump;
		}

		// Air-control (FIXME: looks like this function runs simulation at 10fps
		VectorMA (last_clear_velocity, 7 * 5.0f, accel_direction, last_clear_velocity);
		VectorMA (last_clear_hor_velocity, 7 * 5.0f, accel_direction, last_clear_hor_velocity);
		last_clear_hor_speed = vlen (last_clear_hor_velocity);
	}

	return false;
}

/*
// This code removed - was called at end of main loop of CanJumpOver
static void ApplyTurningAccel (void)
{
	if (self->fb.turning_speed) {
		vec3_t last_clear_angle;
		vectoangles(last_clear_velocity, last_clear_angle);
		last_clear_angle[0] = 0 - last_clear_angle[0];
		last_clear_angle[1] = last_clear_angle[1] + (self->fb.turning_speed * 32 / last_clear_hor_speed);
		trap_makevectors(last_clear_angle);
		VectorScale(g_globalvars.v_forward, vlen(last_clear_velocity), last_clear_velocity);
	}
}*/

// This code removed.... because ledge_backup fires based on velocity, this fires too often when bot is in midair,
//   reversing the direction and causing the bot to go back to platform.
static void LedgeBackupLogic (gedict_t* self, vec3_t rel_pos, vec3_t new_velocity)
{
	qbool ledge_backup = (rel_pos[2] > 0);
	if (new_velocity[2] >= 0) {
		ledge_backup = ((new_velocity[2] * new_velocity[2] * 0.000625) < rel_pos[2]);	// 0.000625 = 0.5 / sv_gravity
	}

	if (self->fb.debug_path) {
		G_bprint (2, "jl2: new_vel %f %f %f\n", PASSVEC3 (new_velocity));
		G_bprint (2, "   : rel_pos %f %f %f\n", PASSVEC3 (rel_pos));
	}

	if (ledge_backup) {
		if (self->fb.ledge_backup_time) {
			if (g_globalvars.time >= self->fb.ledge_backup_time) {
				vec3_t dir_move;

				VectorScale(rel_pos, -1, dir_move);
				dir_move[2] = 0;
				NewVelocityForArrow(self, dir_move, "JumpLedge2");
				self->fb.ledge_backup_time = 0;
			}
		}
		else  {
			self->fb.ledge_backup_time = g_globalvars.time + 0.15;
		}
	}
}

static qbool LedgeJumpLogic (gedict_t* self, vec3_t rel_pos, vec3_t new_velocity)
{
	vec3_t rel_hor_dir;
	qbool try_jump_ledge = true;
	qbool being_blocked = false;
	if (vlen(self->fb.oldvelocity) <= 100) {
		VectorCopy(rel_pos, rel_hor_dir);
		rel_hor_dir[2] = 0;
		try_jump_ledge = (vlen(rel_hor_dir) <= 80);
		VectorNormalize(rel_hor_dir);
		being_blocked = (DotProduct(self->fb.obstruction_normal, rel_hor_dir) > 0.5);
	}

	if (try_jump_ledge && rel_pos[2] > 18) {
		float jumpspeed  = new_velocity[2] + JUMPSPEED;
		if ((jumpspeed * jumpspeed * 0.000625) >= rel_pos[2]) {  // 0.000625 = 0.5 / sv_gravity
			SetJumpFlag (self, true, "LedgeJumpLogic");
			self->fb.path_state |= WAIT_GROUND;
			self->fb.ledge_backup_time = 0;
			return true;
		}
	}

	if (being_blocked && g_globalvars.time > self->fb.arrow_time) {
		if (self->fb.ledge_backup_time) {
			if (g_globalvars.time >= self->fb.ledge_backup_time) {
				vec3_t dir_move;

				VectorMA(rel_hor_dir, -DotProduct(self->fb.obstruction_normal, rel_hor_dir), self->fb.obstruction_normal, dir_move);
				if (dir_move[0] == 0 && dir_move[1] == 0 && dir_move[2] == 0) {
					VectorScale(self->fb.obstruction_normal, -1, dir_move);
					NewVelocityForArrow(self, dir_move, "JumpLedge(dir_move 0)");
				}
				else if (g_random() < 0.5) {
					VectorScale(dir_move, -1, dir_move);
					NewVelocityForArrow(self, dir_move, "JumpLedge(random)");
				}
				else {
					NewVelocityForArrow(self, dir_move, "JumpLedge");
				}
				self->fb.ledge_backup_time = 0;
			}
		}
		else {
			self->fb.ledge_backup_time = g_globalvars.time + 0.15;
		}
	}

	return false;
}

// Only called if self->fb.path_state & JUMP_LEDGE
static qbool JumpLedgeLogic (gedict_t* self, vec3_t new_velocity)
{
	if (g_globalvars.time > self->fb.arrow_time2) {
		vec3_t rel_pos;

		VectorAdd(self->fb.linked_marker->s.v.absmin, self->fb.linked_marker->s.v.view_ofs, rel_pos);
		VectorSubtract(rel_pos, self->s.v.origin, rel_pos);
		VectorCopy(rel_pos, self->fb.obstruction_direction);

		if ((int)self->s.v.flags & FL_ONGROUND) {
			if (LedgeJumpLogic (self, rel_pos, new_velocity)) {
				return true;
			}
		}
		else if (g_globalvars.time > self->fb.arrow_time) {
			LedgeBackupLogic (self, rel_pos, new_velocity);
		}
	}

	if (g_globalvars.time >= (self->fb.ledge_backup_time + 0.15)) {
		self->fb.ledge_backup_time = 0;
	}

	return false;
}

// Called only if current path isn't flagged as JUMP_LEDGE, but obstruction_normal still set
static void ObstructionLogic (gedict_t* self, vec3_t new_velocity)
{
	// not being stationary
	if (self->fb.linked_marker != self->fb.touch_marker) {
		if (vlen(self->fb.oldvelocity) <= 32) {
			vec3_t dir_move;

			VectorMA(new_velocity, -DotProduct (self->fb.obstruction_normal, new_velocity), self->fb.obstruction_normal, dir_move);
			//G_bprint (PRINT_HIGH, "NV %3d %3d %3d, ON %3.2f %3.2f %3.2f, DM %3d %3d %3d\n", PASSINTVEC3 (new_velocity), PASSVEC3 (self->fb.obstruction_normal), PASSINTVEC3 (dir_move));

			if ((dir_move[0] == 0) && (dir_move[1] == 0)) {
				VectorScale(self->fb.obstruction_normal, -1, dir_move);
				self->fb.path_state |= STUCK_PATH;
			}
			else if ((self->fb.oldvelocity[0] == 0) && (self->fb.oldvelocity[1] == 0)) {
				// maybe stuck in a corner
				if (g_random() < 0.5) {
					VectorScale(dir_move, -1, dir_move);
				}
				self->fb.path_state |= STUCK_PATH;
			}
			else if ((int)self->fb.path_state & STUCK_PATH) {
				//
				vec3_t norm_new_velocity;
				VectorNormalize(dir_move);
				normalize(new_velocity, norm_new_velocity);
				VectorAdd(dir_move, norm_new_velocity, dir_move);
			}
			else {
				VectorMA(self->fb.dir_move_, -0.5 * DotProduct(self->fb.obstruction_normal, self->fb.dir_move_), self->fb.obstruction_normal, dir_move);
			}
			dir_move[2] = 0;
			NewVelocityForArrow(self, dir_move, "Obstruct");
		}
	}

	VectorMA(new_velocity, -DotProduct(self->fb.obstruction_normal, new_velocity), self->fb.obstruction_normal, new_velocity);
}

static void DumpDebugLines (char lines[10][128], int count, const char* explanation)
{
	int i;

	return;

	for (i = 0; i < count; ++i)
		G_bprint (PRINT_HIGH, "%s", lines[i]);
	G_bprint (PRINT_HIGH, "    %s\n", explanation);
}

static void AvoidHazardsOnGround (gedict_t* self, float hor_speed, vec3_t new_origin, vec3_t new_velocity, vec3_t dir_forward)
{
	int fall = 0;
	int new_fall = 0;
	vec3_t jump_origin = { 0, 0, 0 };
	vec3_t last_clear_point = { 0, 0, 0 };
	vec3_t jump_velocity;
	vec3_t testplace;
	float fallheight = StandardFallHeight (self);
	char debug[10][128] = { { 0 } };
	int line = 0;

	if (new_velocity[2] < 0) {
		new_velocity[2] = 0;
	}
	VectorCopy(self->s.v.origin, last_clear_point);
	VectorMA(last_clear_point, (16 / hor_speed), new_velocity, testplace);
	fall = FallSpotGround(testplace, fallheight);
	snprintf (debug[line++], sizeof(debug[0]), "AvoidHazardsOnGround(origin [%d %d %d], vel [%d %d %d])\n", PASSINTVEC3 (new_origin), PASSINTVEC3 (new_velocity));
	snprintf (debug[line++], sizeof(debug[0]), "> FallSpotGround([%d %d %d], %f) = %s\n", PASSINTVEC3 (testplace), fallheight, FallType (fall));

	if (fall == FALL_BLOCKED) {
		first_trace_fraction = 1;
		TestTopBlock(self, last_clear_point, testplace);
		if (first_trace_fraction != 1) {
			vec3_t hor_velocity;

			VectorMA(last_clear_point, (16 / hor_speed) * first_trace_fraction, new_velocity, testplace);
			VectorMA(new_velocity, -DotProduct(first_trace_plane_normal, new_velocity), first_trace_plane_normal, new_velocity);
			VectorCopy(new_velocity, hor_velocity);
			hor_velocity[2] = 0;
			hor_speed = vlen(hor_velocity);
			VectorMA(testplace, (16 / hor_speed) * (1 - first_trace_fraction), new_velocity, testplace);
		}
		fall = FallSpotGround(testplace, fallheight);
		snprintf (debug[line++], sizeof(debug[0]), "> FallSpotGround([%d %d %d], %f) = %s\n", PASSINTVEC3 (testplace), fallheight, FallType (fall));
	}

	if (fall >= FALL_LAND && (self->fb.path_state & BOTPATH_CURLJUMP_HINT)) {
		snprintf (debug[line++], sizeof(debug[0]), "> CurlJumpHint(%d)... jumping\n", self->fb.angle_hint);
		DumpDebugLines (debug, line, "CurlJumpHint\n");
		SetJumpFlag (self, true, "AvoidHazards(CurlJump)");
		self->fb.path_state |= DELIBERATE_AIR_WAIT_GROUND;
		return;
	}

	if (fall >= FALL_LAND) {
		VectorCopy(testplace, jump_origin);
		new_fall = fall;
		VectorCopy(new_origin, testplace);
		fall = FallSpotGround(testplace, fallheight);
		snprintf (debug[line++], sizeof(debug[0]), "> 2: FallSpotGround([%d %d %d], %f) = %s\n", PASSINTVEC3 (testplace), fallheight, FallType (fall));
		if ((int)self->fb.path_state & DELIBERATE_AIR) {
			if (fall < FALL_LAND) {
				DumpDebugLines (debug, line, "Deliberate_Air detected\n");
				return;
			}
			self->fb.path_state &= ~DELIBERATE_AIR;
		}
		if (new_fall > fall) {
			//G_bprint (2, "%s: %d (%s) -> %d (%s)\n", self->netname, self->fb.touch_marker->fb.index + 1, self->fb.touch_marker->classname, self->fb.linked_marker->fb.index, self->fb.linked_marker->classname);
			if (g_globalvars.time > self->fb.arrow_time2) {
				// if (CanFallAndGetAcrossHazard(self, new_velocity))
				VectorCopy(new_velocity, jump_velocity);
				jump_velocity[2] = jump_velocity[2] - (6400 / hor_speed);   // 6400 = sv_gravity * 8
				jump_origin[2] = jump_origin[2] + (jump_velocity[2] * (16 / hor_speed));
				jump_velocity[2] = jump_velocity[2] + (6400 / hor_speed);   // 6400 = sv_gravity * 8    (was -)

				if (CanJumpOver(self, jump_origin, jump_velocity, fallheight, fall)) {
					DumpDebugLines (debug, line, "CanFallAcross()\n");
					self->fb.path_state |= DELIBERATE_AIR_WAIT_GROUND;
					return;
				}
				
				// if (CanJumpAcrossHazard(self, new_velocity))
				VectorCopy(new_origin, jump_origin);
				VectorCopy(new_velocity, jump_velocity);
				jump_velocity[2] += JUMPSPEED;
				snprintf (debug[line++], sizeof(debug[0]), "> CanJumpOver([%d %d %d], [%d %d %d], %f, %s)\n", PASSINTVEC3 (jump_origin), PASSINTVEC3 (jump_velocity), fallheight, FallType (fall));
				if (CanJumpOver(self, jump_origin, jump_velocity, fallheight, fall)) {
					//G_bprint (2, "    CanJumpOver(jumpo[%f %f %f] v[%f %f %f] %f %d %f)\n", PASSVEC3 (new_origin), PASSVEC3(jump_velocity), fallheight, fall, hor_speed);
					DumpDebugLines (debug, line, "CanJumpOver() => Jumping\n");
					SetJumpFlag (self, true, "AvoidHazards(CanJumpAcrossHzd)");
					self->fb.path_state |= DELIBERATE_AIR_WAIT_GROUND;
					return;
				}

				//G_bprint (2, "    Failed CanJumpOver tests: [%f %f %f, %f %f %f, %f, %d, %f]\n", PASSVEC3(new_origin), PASSVEC3(jump_velocity), fallheight, fall, hor_speed);
			}
			else {
				//G_bprint (2, "    Not trying, arrow_time2 %f (%f, now %f)\n", self->fb.arrow_time2, self->fb.arrow_time, g_globalvars.time);
			}
			AvoidEdge(self);
		}
		return;
	}

	VectorMA(testplace, 8 / hor_speed, new_velocity, new_origin);
	VectorMA(testplace, 16 / hor_speed, new_velocity, testplace);
	fall = FallSpotGround (testplace, fallheight);
	snprintf (debug[line++], sizeof(debug[0]), "> 3: FallSpotGround([%d %d %d], %f) = %s\n", PASSINTVEC3 (testplace), fallheight, FallType (fall));

	if (fall >= FALL_LAND && (self->fb.path_state & BOTPATH_CURLJUMP_HINT)) {
		snprintf (debug[line++], sizeof(debug[0]), "> CurlJumpHint(%d)... jumping\n", self->fb.angle_hint);
		DumpDebugLines (debug, line, "CurlJumpHint\n");
		SetJumpFlag (self, true, "AvoidHazards(CurlJumpHint2)");
		self->fb.path_state |= DELIBERATE_AIR_WAIT_GROUND;
		return;
	}

	if (fall >= FALL_LAND) {
		new_fall = fall;
		VectorCopy(testplace, jump_origin);
		VectorCopy(self->s.v.origin, testplace);
		fall = FallSpotGround (testplace, fallheight);
		snprintf (debug[line++], sizeof(debug[0]), "> 4: FallSpotGround([%d %d %d], %f) = %s\n", PASSINTVEC3 (testplace), fallheight, FallType (fall));
		if ((int)self->fb.path_state & DELIBERATE_AIR) {
			if (fall < FALL_LAND) {
				DumpDebugLines (debug, line, "Deliberate_Air detected\n");
				return;
			}
			self->fb.path_state &= ~DELIBERATE_AIR;
		}
		if (new_fall > fall) {
			float normal_comp = 0;
			vec3_t edge_normal;

			if (g_globalvars.time > self->fb.arrow_time2) {
				VectorCopy(new_velocity, jump_velocity);
				jump_velocity[2] = jump_velocity[2] - (6400 / hor_speed);
				jump_origin[2] = jump_origin[2] + (jump_velocity[2] * (16 / hor_speed));
				jump_velocity[2] = jump_velocity[2] - (6400 / hor_speed);
				snprintf (debug[line++], sizeof(debug[0]), "> CanJumpOver([%d %d %d], [%d %d %d], %f, %s)\n", PASSINTVEC3 (jump_origin), PASSINTVEC3 (jump_velocity), fallheight, FallType (fall));
				if (CanJumpOver(self, jump_origin, jump_velocity, fallheight, fall)) {
					DumpDebugLines (debug, line, "CanFallOver2\n");
					self->fb.path_state |= NO_DODGE;
					return;
				}
				VectorCopy(new_origin, jump_origin);
				VectorCopy(new_velocity, jump_velocity);
				jump_velocity[2] += JUMPSPEED;
				snprintf (debug[line++], sizeof(debug[0]), "> CanJumpOver([%d %d %d], [%d %d %d], %f, %s)\n", PASSINTVEC3 (jump_origin), PASSINTVEC3 (jump_velocity), fallheight, FallType (fall));
				if (CanJumpOver(self, jump_origin, jump_velocity, fallheight, fall)) {
					DumpDebugLines (debug, line, "CanJumpOver2\n");
					self->fb.path_state |= NO_DODGE;
					SetJumpFlag (self, true, "AvoidHazards(CanJumpOver2)");
					return;
				}
			}
			traceline(
				self->s.v.origin[0] + (dir_forward[0] * 32), 
				self->s.v.origin[1] + (dir_forward[1] * 32), 
				self->s.v.origin[2] + (dir_forward[2] * 32) - 24.1, 
				self->s.v.origin[0] - (dir_forward[0] * 16), 
				self->s.v.origin[1] - (dir_forward[1] * 16),
				self->s.v.origin[2] - (dir_forward[2] * 16) -24.1, true, world);
			g_globalvars.trace_plane_normal[2] = 0;
			if (g_globalvars.trace_plane_normal[0] == 0 && g_globalvars.trace_plane_normal[1] == 0 && g_globalvars.trace_plane_normal[2] == 0) {
				AvoidEdge(self);
				DumpDebugLines (debug, line, "AvoidingEdge1\n");
				return;
			}

			normalize(g_globalvars.trace_plane_normal, edge_normal);
			normal_comp = DotProduct(edge_normal, dir_forward);
			if (normal_comp <= 0) {
				AvoidEdge(self);
				DumpDebugLines (debug, line, "AvoidingEdge2\n");
				return;
			}

			{
				vec3_t dir_move;
				VectorMA(dir_forward, -2 * normal_comp, edge_normal, dir_move);
				dir_move[2] = 0;
				NewVelocityForArrow(self, dir_move, "AvoidHzds");
				DumpDebugLines (debug, line, "BackingUp...\n");
				if (normal_comp > 0.5) {
					self->fb.arrow_time2 = self->fb.arrow_time;
				}
			}
		}
	}
}

static void AvoidHazardsInAir (gedict_t* self, float hor_speed, vec3_t new_origin, vec3_t new_velocity, vec3_t last_clear_point)
{
	float fallheight = StandardFallHeight (self);
	vec3_t testplace;
	int fall;

	// Don't change trajectory on rocket jump while ascending
	if (self->fb.path_state & BOTPATH_RJ_IN_PROGRESS) {
		return;
	}

	VectorMA(new_origin, 32 / hor_speed, new_velocity, testplace);  // FIXME: Why 32?  (10 fps?)
	fall = FallSpotAir (self, testplace, fallheight);
	if (fall >= FALL_LAND) {
		int new_fall = fall;
		VectorCopy(new_origin, testplace);
		fall = FallSpotAir (self, testplace, fallheight);
		if (self->fb.path_state & DELIBERATE_AIR) {
			if (fall < FALL_LAND) {
				if (FrogbotOptionEnabled (FB_OPTION_SHOW_MOVEMENT_LOGIC)) {
					G_sprint (self, PRINT_HIGH, "AvoidHazardsInAir: DELIBERATE_AIR [%s/%s]\n", FallType(new_fall), FallType(fall));
				}
				return;
			}
			self->fb.path_state &= ~DELIBERATE_AIR;
		}

		if (new_fall > fall) {
			VectorMA(new_origin, (16 / hor_speed), new_velocity, testplace);
			fall = FallSpotAir (self, testplace, fallheight);
			if (new_fall > fall) {
				vec3_t jump_origin, jump_velocity;

				VectorCopy(new_origin, jump_origin);
				VectorCopy(new_velocity, jump_velocity);
				if (CanJumpOver(self, jump_origin, jump_velocity, fallheight, fall)) {
					if (FrogbotOptionEnabled (FB_OPTION_SHOW_MOVEMENT_LOGIC)) {
						G_sprint (self, PRINT_HIGH, "AvoidHazardsInAir: CanJumpOver [%s/%s]\n", FallType(new_fall), FallType(fall));
					}
					return;
				}

				if (FrogbotOptionEnabled (FB_OPTION_SHOW_MOVEMENT_LOGIC)) {
					G_sprint (self, PRINT_HIGH, "AvoidHazardsInAir: AvoidingEdge [%s/%s]\n", FallType(new_fall), FallType(fall));
				}
				AvoidEdge(self);
			}
		}
		else if (FrogbotOptionEnabled (FB_OPTION_SHOW_MOVEMENT_LOGIC)) {
			G_sprint (self, PRINT_HIGH, "AvoidHazardsInAir: Ignored [%s/%s]\n", FallType(new_fall), FallType(fall));
		}
	}
	else if (FrogbotOptionEnabled (FB_OPTION_SHOW_MOVEMENT_LOGIC)) {
		G_sprint (self, PRINT_HIGH, "AvoidHazardsInAir: Ignored [%s]\n", FallType(fall));
	}
}

// FIXME: Globals.
// FIXME: Magic numbers all over the place.
void AvoidHazards(gedict_t* self)
{
	float hor_speed = 0;
	vec3_t new_origin = { 0 };
	vec3_t new_velocity = { 0 };
	vec3_t hor_velocity = { 0 };
	vec3_t dir_forward;
	vec3_t testplace;

	// FIXME: should this be current velocity or proposed velocity (self->fb.dir_move_, scaled by speed?)
	VectorCopy(self->s.v.velocity, new_velocity);
	//VectorScale (self->fb.dir_move_, , new_velocity);
	if (self->fb.path_state & JUMP_LEDGE) {
		if (JumpLedgeLogic (self, new_velocity))
			return;
	}
	else if (self->fb.obstruction_normal[0] || self->fb.obstruction_normal[1] || self->fb.obstruction_normal[2]) {
		if (g_globalvars.time > self->fb.arrow_time && ((int)self->s.v.flags & FL_WATERJUMP))
			return;
		else if (g_globalvars.time > self->fb.arrow_time)
			ObstructionLogic (self, new_velocity);
		//G_bprint (2, "Unhandled obstruction...");
	}

	if (self->s.v.waterlevel)
		return;

	// Reduce to horizontal velocity only
	VectorCopy(new_velocity, hor_velocity);
	hor_velocity[2] = 0;
	hor_speed = vlen(hor_velocity);
	if (!hor_speed)
		return; // falling straight down

	VectorCopy(self->s.v.origin, new_origin);

	if ((int)self->s.v.flags & FL_ONGROUND) {
		normalize(hor_velocity, dir_forward);
		AvoidHazardsOnGround (self, hor_speed, new_origin, new_velocity, dir_forward);
	}
	else {
		AvoidHazardsInAir (self, hor_speed, new_origin, new_velocity, testplace);
	}
}

// Called after a grenade has been spawned
void BotsGrenadeSpawned (gedict_t* newmis)
{
	if (!bots_enabled ())
		return;

	// Call GrenadeAlert() repeatedly so we can avoid the grenade
	newmis->fb.frogbot_nextthink = newmis->s.v.nextthink;
	newmis->s.v.nextthink = g_globalvars.time + 0.05; // New
	newmis->think = (func_t) GrenadeAlert;
}

// Called after a rocket has been spawned
void BotsRocketSpawned (gedict_t* newmis)
{
	if (!bots_enabled ())
		return;

	// Call RocketAlert() repeatedly so we can avoid the rocket
	newmis->fb.frogbot_nextthink = newmis->s.v.nextthink;
	newmis->think = (func_t) RocketAlert;
	newmis->s.v.nextthink = 0.001;

	// Store spawn time so we don't avoid too early
	newmis->fb.missile_spawntime = g_globalvars.time;

	// Store rocket direction
	VectorCopy(g_globalvars.v_forward, newmis->fb.missile_forward);
	VectorCopy(g_globalvars.v_right, newmis->fb.missile_right);
}

qbool BotsPreTeleport (gedict_t* self, gedict_t* other)
{
	if (NoItemTouch (self, other)) {
		if (IsMarkerFrame ()) {
			HazardTeleport (self, other);
		}
		return true;
	}

	if (HasSavedMarker()) {
		return true;
	}

	return false;
}

void BotsPostTeleport (gedict_t* teleport_trigger, gedict_t* player, gedict_t* teleport_destination)
{
	if (player->isBot) {
		//if (teamplay != 0)
		//	say_team_report_teleport(other, t);
		player->fb.wiggle_run_dir = 0;
		BotPathCheck (player, teleport_trigger);
	}

	player->fb.frogbot_nextthink = g_globalvars.time;

	if (player->fb.linked_marker == teleport_trigger) {
		SetLinkedMarker (player, teleport_destination, "BotsPostTeleport");
	}

	HazardTeleport (teleport_trigger, player);
	SetMarker (player, teleport_destination);
}

#endif // BOT_SUPPORT
