// Converted from .qc on 05/02/2016

#include "g_local.h"
#include "fb_globals.h"

// A lot of this is the bot 'cheating'?..
#define ARROW_TIME_AFTER_TELEPORT 0.20  // was 0.5

void GrenadeExplode (void);

// FIXME: Local globals
static float first_trace_fraction = 0;
static vec3_t first_trace_plane_normal = { 0 };

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

static void HazardTeleport(gedict_t* self, gedict_t* other) {
	gedict_t* plr;

	self->fb.arrow_time = max (self->fb.arrow_time, g_globalvars.time + ARROW_TIME_AFTER_TELEPORT);

	for (plr = world; plr = find_plr(plr); ) {
		if (plr != other && plr->fb.linked_marker == self) {
			plr->fb.old_linked_marker = NULL;
			SetLinkedMarker(plr, LocateMarker(plr->s.v.origin), "HazardTeleport");
			plr->fb.path_state = 0;
			plr->fb.linked_marker_time = g_globalvars.time + 5;
		}
	}
}

// This sets arrow_time on closest markers, to indicate danger
static void ExplodeAlert(vec3_t org, float next_time) {
	gedict_t* marker;
	gedict_t* tele;

	// Find all map markers close to this point
	for (marker = world; marker = trap_findradius(marker, org, 256); ) {
		if (marker->fb.fl_marker) {
			// Would the grenade hurt the marker?
			traceline(org[0], org[1], org[2], marker->s.v.absmin[0] + marker->s.v.view_ofs[0], marker->s.v.absmin[1] + marker->s.v.view_ofs[1], marker->s.v.absmin[2] + marker->s.v.view_ofs[2], true, marker);
			if (g_globalvars.trace_fraction == 1) {
				// Mark the current time
				marker->fb.arrow_time = next_time;

				for (tele = world; tele = ez_find (tele, "trigger_teleport"); ) {
					// If this teleport takes us to the marker close to the grenade, set arrow_time
					if (!strnull (tele->s.v.target)) {
						gedict_t* target = find (world, FOFS (s.v.targetname), tele->s.v.target);
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
	self->s.v.think = (func_t) GrenadeAlert;
	if (self->fb.frogbot_nextthink <= g_globalvars.time) {
		self->s.v.think = (func_t) GrenadeExplode;
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
		self->s.v.think = (func_t) Missile_Remove;
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
		int fall = 0;
		int content;

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

	return FALL_BLOCKED;
}

static int FallSpotAir(vec3_t testplace, float fallheight) {
	int fall = 0;
	int content;

	VectorCopy(testplace, dropper->s.v.origin);
	dropper->s.v.flags = FL_ONGROUND_PARTIALGROUND;

	if (walkmove(dropper, 0, 0)) {
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

		if (!(droptofloor(dropper)))
			return FALL_DEATH;
	}

	content = trap_pointcontents(dropper->s.v.origin[0], dropper->s.v.origin[1], dropper->s.v.origin[2] - 24);
	if (content == CONTENT_LAVA)
		return FALL_DEATH;
	
	if (dropper->s.v.origin[2] < fallheight) {
		return FALL_LAND;
	}

	return FALL_FALSE;
}

// last_clear_point is output variable
static qbool CanJumpOver(gedict_t* self, vec3_t jump_origin, vec3_t jump_velocity, float fallheight, int current_fallspot) {
	int i = 0;
	int tries = 0;
	float last_clear_hor_speed = 0;
	vec3_t last_clear_hor_velocity;
	vec3_t last_clear_point;
	vec3_t last_clear_velocity;

	VectorCopy(jump_origin, last_clear_point);
	VectorCopy(jump_velocity, last_clear_velocity);
	VectorCopy(last_clear_velocity, last_clear_hor_velocity);
	last_clear_hor_velocity[2] = 0;

	last_clear_hor_speed = vlen(last_clear_hor_velocity);
	last_clear_velocity[2] = jump_velocity[2] - (12800 / last_clear_hor_speed);   // 12800 = sv_gravity * 16?
	for (tries = 0; tries < 20; ++tries) {
		int fall = 0;
		vec3_t testplace;

		//G_bprint (2, "%02d: last_clear_point = %f %f %f\n", tries, PASSVEC3(last_clear_point));
		if (last_clear_point[2] < fallheight) {
			//G_bprint (2, "    last_clear_point_z < fallheight (%f), quitting\n", fallheight);
			return false;
		}

		VectorMA (last_clear_point, (32 / last_clear_hor_speed), last_clear_velocity, testplace);

		fall = FallSpotAir(testplace, fallheight);
		//G_bprint (2, "    Testplace: %f %f %f = %d\n", PASSVEC3 (testplace), fall);
		if (fall == FALL_BLOCKED) {
			first_trace_fraction = 1;
			TestTopBlock(self, last_clear_point, testplace);
			TestBottomBlock(self, last_clear_point, testplace);
			if (first_trace_fraction != 1) {
				float dotProduct;

				VectorMA (last_clear_point, (first_trace_fraction * 32 / last_clear_hor_speed), last_clear_velocity, testplace);

				dotProduct = DotProduct(first_trace_plane_normal, last_clear_velocity);
				VectorMA (last_clear_velocity, -dotProduct, first_trace_plane_normal, last_clear_velocity);

				VectorCopy(last_clear_velocity, last_clear_hor_velocity);
				last_clear_hor_velocity[2] = 0;
				last_clear_hor_speed = vlen(last_clear_hor_velocity);

				VectorMA (testplace, (32 / last_clear_hor_speed) * (1 - first_trace_fraction), last_clear_velocity, testplace);
			}
			fall = FallSpotAir (testplace, fallheight);
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

				for (test_enemy = world; test_enemy = find_plr (test_enemy); ) {
					test_enemy->s.v.solid = test_enemy->fb.oldsolid;
				}

				for (test_enemy = world; test_enemy = trap_findradius(test_enemy, testplace, 84); ) {
					if (test_enemy->fb.T & BOTFLAG_UNREACHABLE) {
						test_enemy = NULL;
						do_jump = false;
						break;
					}
				}

				for (test_enemy = world; test_enemy = find_plr (test_enemy); ) {
					test_enemy->s.v.solid = SOLID_NOT;
				}
			}
			return do_jump;
		}

		if (self->fb.turning_speed) {
			vec3_t last_clear_angle;
			vectoangles(last_clear_velocity, last_clear_angle);
			last_clear_angle[0] = 0 - last_clear_angle[0];
			last_clear_angle[1] = last_clear_angle[1] + (self->fb.turning_speed * 32 / last_clear_hor_speed);
			trap_makevectors(last_clear_angle);
			VectorScale(g_globalvars.v_forward, vlen(last_clear_velocity), last_clear_velocity);
		}
	}

	return false;
}

// This code removed.... because ledge_backup fires based on velocity, this fires too often when bot is in midair,
//   reversing the direction and causing the bot to go back to platform.
static void LedgeBackupLogic (gedict_t* self, vec3_t rel_pos, vec3_t new_velocity)
{
	qbool ledge_backup = (rel_pos[2] > 0);
	if (new_velocity[2] >= 0) {
		ledge_backup = ((new_velocity[2] * new_velocity[2] * 0.000625) < rel_pos[2]);	// 0.000625 = 0.5 / sv_gravity
	}

	return;

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

// Only called if self->fb.path_state & JUMP_LEDGE
static qbool JumpLedgeLogic (gedict_t* self, vec3_t new_velocity)
{
	vec3_t rel_hor_dir;

	if (g_globalvars.time > self->fb.arrow_time2) {
		vec3_t rel_pos;

		VectorAdd(self->fb.linked_marker->s.v.absmin, self->fb.linked_marker->s.v.view_ofs, rel_pos);
		VectorSubtract(rel_pos, self->s.v.origin, rel_pos);
		VectorCopy(rel_pos, self->fb.obstruction_direction);

		if ((int)self->s.v.flags & FL_ONGROUND) {
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
					if (self->fb.debug_path)
						G_bprint (2, "JumpLedgeLogic() => jumping\n");
					self->fb.jumping = true;
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
static qbool ObstructionLogic (gedict_t* self, vec3_t new_velocity)
{
	if (g_globalvars.time > self->fb.arrow_time) {
		if ((int)self->s.v.flags & FL_WATERJUMP)
			return true;

		// not being stationary
		if (self->fb.linked_marker != self->fb.touch_marker) {
			if (vlen(self->fb.oldvelocity) <= 32) {
				vec3_t dir_move;

				//VectorMA(new_velocity, -DotProduct (self->fb.obstruction_normal, new_velocity), self->fb.obstruction_normal, dir_move);
				{
					vec3_t scale;
					float dotProduct = DotProduct (self->fb.obstruction_normal, new_velocity);

					VectorScale (self->fb.obstruction_normal, dotProduct, scale);

					VectorSubtract (new_velocity, scale, dir_move);
				}

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
	}

	VectorMA(new_velocity, -DotProduct(self->fb.obstruction_normal, new_velocity), self->fb.obstruction_normal, new_velocity);
	return false;
}

static void AvoidHazardsOnGround (gedict_t* self, float hor_speed, vec3_t new_origin, vec3_t new_velocity, float fallheight, vec3_t dir_forward)
{
	int fall = 0;
	int new_fall = 0;
	vec3_t jump_origin = { 0, 0, 0 };
	vec3_t last_clear_point = { 0, 0, 0 };
	vec3_t jump_velocity;
	vec3_t testplace;

	if (new_velocity[2] < 0) {
		new_velocity[2] = 0;
	}
	VectorCopy(self->s.v.origin, last_clear_point);
	VectorMA(last_clear_point, (16 / hor_speed), new_velocity, testplace);
	fall = FallSpotGround(testplace, fallheight);

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
	}

	if (fall >= FALL_LAND) {
		VectorCopy(testplace, jump_origin);
		new_fall = fall;
		VectorCopy(new_origin, testplace);
		fall = FallSpotGround(testplace, fallheight);
		if ((int)self->fb.path_state & DELIBERATE_AIR) {
			if (fall < FALL_LAND) {
				return;
			}
			self->fb.path_state &= ~DELIBERATE_AIR;
		}
		if (new_fall > fall) {
			//G_bprint (2, "%s: %d (%s) -> %d (%s)\n", self->s.v.netname, self->fb.touch_marker->fb.index, self->fb.touch_marker->s.v.classname, self->fb.linked_marker->fb.index, self->fb.linked_marker->s.v.classname);
			if (g_globalvars.time > self->fb.arrow_time2) {
				// Can fall and still get across (?)
				VectorCopy(new_velocity, jump_velocity);
				jump_velocity[2] = jump_velocity[2] - (6400 / hor_speed);   // 6400 = sv_gravity * 8
				jump_origin[2] = jump_origin[2] + (jump_velocity[2] * (16 / hor_speed));
				jump_velocity[2] = jump_velocity[2] + (6400 / hor_speed);   // 6400 = sv_gravity * 8    (was -)

				if (CanJumpOver(self, jump_origin, jump_velocity, fallheight, fall)) {
					self->fb.path_state |= DELIBERATE_AIR_WAIT_GROUND | (self->fb.turning_speed ? AIR_ACCELERATION : 0);
					return;
				}
				
				// Can jump across instead
				VectorCopy(new_origin, jump_origin);
				VectorCopy(new_velocity, jump_velocity);
				jump_velocity[2] += JUMPSPEED;
				if (CanJumpOver(self, jump_origin, jump_velocity, fallheight, fall)) {
					//G_bprint (2, "    CanJumpOver(jumpo[%f %f %f] v[%f %f %f] %f %d %f)\n", PASSVEC3 (new_origin), PASSVEC3(jump_velocity), fallheight, fall, hor_speed);
					if (self->fb.debug_path)
						G_bprint (2, "CanJumpOver() => jumping\n");
					self->fb.jumping = true;
					self->fb.path_state |= DELIBERATE_AIR_WAIT_GROUND | (self->fb.turning_speed ? AIR_ACCELERATION : 0);
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
	if (fall >= FALL_LAND) {
		new_fall = fall;
		VectorCopy(testplace, jump_origin);
		VectorCopy(self->s.v.origin, testplace);
		fall = FallSpotGround (testplace, fallheight);
		if ((int)self->fb.path_state & DELIBERATE_AIR) {
			if (fall < FALL_LAND) {
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
				if (CanJumpOver(self, jump_origin, jump_velocity, fallheight, fall)) {
					self->fb.path_state |= NO_DODGE;
					return;
				}
				VectorCopy(new_origin, jump_origin);
				VectorCopy(new_velocity, jump_velocity);
				jump_velocity[2] += JUMPSPEED;
				if (CanJumpOver(self, jump_origin, jump_velocity, fallheight, fall)) {
					self->fb.path_state |= NO_DODGE;
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
				return;
			}

			normalize(g_globalvars.trace_plane_normal, edge_normal);
			normal_comp = DotProduct(edge_normal, dir_forward);
			if (normal_comp <= 0) {
				AvoidEdge(self);
				return;
			}

			{
				vec3_t dir_move;
				VectorMA(dir_forward, -2 * normal_comp, edge_normal, dir_move);
				dir_move[2] = 0;
				NewVelocityForArrow(self, dir_move, "AvoidHzds");
				if (normal_comp > 0.5) {
					self->fb.arrow_time2 = self->fb.arrow_time;
				}
			}
		}
	}
}

static void AvoidHazardsInAir (gedict_t* self, float hor_speed, vec3_t new_origin, vec3_t new_velocity, vec3_t last_clear_point, vec3_t testplace, float fallheight)
{
	int fall = FallSpotAir(testplace, fallheight);
	if (fall >= FALL_LAND) {
		int new_fall = fall;
		VectorCopy(new_origin, testplace);
		fall = FallSpotAir(testplace, fallheight);
		if (self->fb.path_state & DELIBERATE_AIR) {
			if (fall < FALL_LAND) {
				return;
			}
			self->fb.path_state &= ~DELIBERATE_AIR;
		}

		if (new_fall > fall) {
			VectorMA(new_origin, (16 / hor_speed), new_velocity, testplace);
			fall = FallSpotAir(testplace, fallheight);
			if (new_fall > fall) {
				vec3_t jump_origin, jump_velocity;

				VectorCopy(new_origin, jump_origin);
				VectorCopy(new_velocity, jump_velocity);
				if (CanJumpOver(self, jump_origin, jump_velocity, fallheight, fall)) {
					return;
				}

				AvoidEdge(self);
			}
		}
	}
}

// FIXME: Globals.
// FIXME: Magic numbers all over the place.
void AvoidHazards(gedict_t* self) {
	float hor_speed = 0;
	vec3_t new_origin = { 0 };
	vec3_t new_velocity = { 0 };
	vec3_t hor_velocity = { 0 };
	vec3_t dir_forward;
	vec3_t testplace;
	float fallheight = 0;

	//if (self->fb.touch_marker && (self->fb.touch_marker->fb.index == 89 || self->fb.touch_marker->fb.index == 51)) {
		//G_bprint (2, "touching #%d: goal entity %s, linked %d\n", self->fb.touch_marker->fb.index, g_edicts[self->s.v.goalentity].s.v.classname, (self->fb.linked_marker ? self->fb.linked_marker->fb.index : -1));
	//}

	// FIXME: should this be current velocity or proposed velocity (self->fb.dir_move_, scaled by speed?)
	VectorCopy(self->s.v.velocity, new_velocity);
	//VectorScale (self->fb.dir_move_, , new_velocity);
	if (self->fb.path_state & JUMP_LEDGE) {
		if (JumpLedgeLogic (self, new_velocity))
			return;
	}
	else if (self->fb.obstruction_normal[0] || self->fb.obstruction_normal[1] || self->fb.obstruction_normal[2]) {
		if (ObstructionLogic (self, new_velocity))
			return;
		//G_bprint (2, "Unhandled obstruction...");
	}

	// Reduce to horizontal velocity only
	VectorCopy(new_velocity, hor_velocity);
	hor_velocity[2] = 0;
	hor_speed = vlen(hor_velocity);
	if (!hor_speed)
		return; // falling straight down

	VectorCopy(self->s.v.origin, new_origin);
	VectorMA(new_origin, 32 / hor_speed, new_velocity, testplace);  // FIXME: Why 32?

	if (self->s.v.waterlevel)
		return;

	normalize(hor_velocity, dir_forward);

	// Calculate fall height
	fallheight = self->s.v.origin[2] - 38; // FIXME: Why 38
	if (self->fb.linked_marker) {
		float min_second = self->fb.linked_marker->s.v.absmin[2] + self->fb.linked_marker->s.v.view_ofs[2] - 36; // FIXME: Why 36?

		fallheight = min (min_second, fallheight);
	}

	if ((int)self->s.v.flags & FL_ONGROUND) {
		AvoidHazardsOnGround (self, hor_speed, new_origin, new_velocity, fallheight, dir_forward);
	}
	else {
		vec3_t last_clear_point = { 0 };

		AvoidHazardsInAir (self, hor_speed, new_origin, new_velocity, last_clear_point, testplace, fallheight);
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
	newmis->s.v.think = (func_t) GrenadeAlert;
}

// Called after a rocket has been spawned
void BotsRocketSpawned (gedict_t* newmis)
{
	if (!bots_enabled ())
		return;

	// Call RocketAlert() repeatedly so we can avoid the rocket
	newmis->fb.frogbot_nextthink = newmis->s.v.nextthink;
	newmis->s.v.think = (func_t) RocketAlert;
	newmis->s.v.nextthink = 0.001;

	// Store rocket direction
	VectorCopy(g_globalvars.v_forward, newmis->fb.missile_forward);
	VectorCopy(g_globalvars.v_right, newmis->fb.missile_right);
}

qbool BotsPreTeleport (gedict_t* self, gedict_t* other)
{
	if (NoItemTouch (self, other)) {
		other->fb.near_teleport = self;
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

void BotsPostTeleport (gedict_t* self, gedict_t* other, gedict_t* teleport_destination)
{
	if (other->isBot) {
		//if (teamplay != 0)
		//	say_team_report_teleport(other, t);

		// other.angles holds crosshair position
		other->fb.real_pitch = other->s.v.angles[0];
		other->fb.real_yaw = other->s.v.angles[1];
	}

	other->fb.frogbot_nextthink = g_globalvars.time;

	if (other->fb.linked_marker == self) {
		SetLinkedMarker (other, teleport_destination, "BotsPostTeleport");
	}

	HazardTeleport (self, other);
	SetMarker (other, teleport_destination);
}
