#ifdef BOT_SUPPORT

#include "g_local.h"

#define ARROW_TIME_INCREASE       0.15  // Seconds to advance after NewVelocityForArrow
#define MIN_DEAD_TIME 0.2f
#define MAX_DEAD_TIME 1.0f
#define CURLJUMP_BRAKE_DROP 32    // Curl jump target this far below the takeoff marker to pull back in the air

//#define DEBUG_MOVEMENT

void SetLinkedMarker(gedict_t *player, gedict_t *marker, char *explanation)
{
	gedict_t *touch = player->fb.touch_marker;

	if ((marker != player->fb.linked_marker) && FrogbotOptionEnabled(FB_OPTION_SHOW_MOVEMENT_LOGIC))
	{
		G_sprint(player, 2, "linked to %3d/%s, touching %3d/%s g %s (%s)\n",
					marker ? marker->fb.index + 1 : -1, marker ? marker->classname : "(null)",
					touch ? touch->fb.index + 1 : -1, touch ? touch->classname : "(null)",
					g_edicts[player->s.v.goalentity].classname, explanation ? explanation : "");
	}

	if (player->fb.debug_path)
	{
		G_bprint(2, "%3.2f: linked to %3d/%s, touching %3d/%s g %s (%s)\n", g_globalvars.time,
					marker ? marker->fb.index + 1 : -1, marker ? marker->classname : "(null)",
					touch ? touch->fb.index + 1 : -1, touch ? touch->classname : "(null)",
					g_edicts[player->s.v.goalentity].classname, explanation ? explanation : "");
	}

	player->fb.linked_marker = marker;
}

void SetJumpFlag(gedict_t *player, qbool jumping, const char *explanation)
{
	if (jumping != player->fb.jumping)
	{
		if (self->fb.debug_path)
		{
			G_bprint(PRINT_HIGH, "%3.2f: jumping (%s)\n", g_globalvars.time, explanation);
		}

		if (FrogbotOptionEnabled(FB_OPTION_SHOW_MOVEMENT_LOGIC))
		{
			G_sprint(player, PRINT_HIGH, "%3.2f: jumping (%s)\n", g_globalvars.time, explanation);
		}
	}

	player->fb.jumping = jumping;
}

void SetDirectionMove(gedict_t *self, vec3_t dir_move, const char *explanation)
{
	VectorCopy(dir_move, self->fb.dir_move_);
	self->fb.dir_speed = VectorNormalize(self->fb.dir_move_);

	/*	if (self->fb.debug_path) {
	 G_bprint (PRINT_HIGH, "%3.2f: SetDirection(%4d %4d %4d): %s\n", g_globalvars.time, PASSSCALEDINTVEC3 (self->fb.dir_move_, 320), explanation);
	 }
	 if (FrogbotOptionEnabled (FB_OPTION_SHOW_MOVEMENT_LOGIC)) {
	 G_sprint (self, PRINT_HIGH, "%3.2f: SetDirection(%4d %4d %4d): %s\n", g_globalvars.time, PASSSCALEDINTVEC3 (self->fb.dir_move_, 320), explanation);
	 }*/
}

void NewVelocityForArrow(gedict_t *self, vec3_t dir_move, const char *explanation)
{
	SetDirectionMove(self, dir_move, explanation);
	self->fb.arrow_time = g_globalvars.time + ARROW_TIME_INCREASE;
}

static qbool BotRequestRespawn(gedict_t *self)
{
	float time_dead = min(g_globalvars.time - self->fb.last_death, MAX_DEAD_TIME);

	return ((self->s.v.deadflag == DEAD_RESPAWNABLE) && (time_dead > MIN_DEAD_TIME)
			&& (g_random() < (time_dead / MAX_DEAD_TIME)));
}

static void PM_Accelerate(vec3_t vel_after_friction, qbool onGround, vec3_t orig_wishdir,
							vec3_t vel_result, qbool trace)
{
	float addspeed, accelspeed, currentspeed;
	float wishspeed = 320; // FIXME: assuming attemping sv_maxspeed
	float accel = 10; // FIXME: assumption
	vec3_t velocity;
	vec3_t wishdir;

	VectorCopy(vel_after_friction, velocity);
	if (onGround)
	{
		velocity[2] = 0;
	}

	wishdir[0] = orig_wishdir[0];
	wishdir[1] = orig_wishdir[1];
	wishdir[2] = 0;
	wishspeed = VectorNormalize(wishdir);
	wishspeed = sv_maxspeed; // fixme: we scale back up to maximum just as passing command

	currentspeed = DotProduct(velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
	{
		VectorCopy(vel_after_friction, vel_result);
#ifdef DEBUG_MOVEMENT
		if (trace) {
			G_bprint (PRINT_HIGH, "%3.2f,KTX,%4.1f,%4.1f,%4.1f,%4.1f,%4.1f,%4.1f,%3.2f,%3.2f,%3.2f,0,%4.1f,%4.1f,%4.1f\n",
				g_globalvars.time, PASSVEC3 (vel_after_friction), PASSVEC3 (wishdir), wishspeed, currentspeed, addspeed, PASSVEC3 (vel_result)
			);
		}
#endif
		return;
	}

	accelspeed = accel * g_globalvars.frametime * wishspeed;
	VectorMA(vel_after_friction, min(accelspeed, addspeed), wishdir, vel_result);
#ifdef DEBUG_MOVEMENT
	if (trace) {
		G_bprint (PRINT_HIGH, "%3.2f,KTX,%4.1f,%4.1f,%4.1f,%4.1f,%4.1f,%4.1f,%3.2f,%3.2f,%3.2f,%3.2f,%4.1f,%4.1f,%4.1f\n",
			g_globalvars.time, PASSVEC3 (vel_after_friction), PASSVEC3 (wishdir), wishspeed, currentspeed, addspeed, accelspeed, PASSVEC3 (vel_result)
		);
	}
#endif
}

// Pulls back in the air if the bot would fly past the curl jump target before dropping to its height,
//   as a player does to land on it.  Returns false if the bot needs more speed, not less
static qbool CurlJumpBrake(gedict_t *self, vec3_t velocity, vec3_t dir_command)
{
	gedict_t *linked = self->fb.linked_marker;
	gedict_t *from = self->fb.touch_marker;
	float gravity = cvar("sv_gravity");
	vec3_t target;
	vec3_t wish;
	float drop_time;
	float above;

	VectorAdd(linked->s.v.absmin, linked->s.v.view_ofs, target);

	// only when dropping to a lower floor (otherwise it can fall short into the gap of a level jump)
	if (!from
			|| (from->s.v.absmin[2] + from->s.v.view_ofs[2] - target[2] < CURLJUMP_BRAKE_DROP))
	{
		return false;
	}

	above = self->s.v.origin[2] - target[2];
	if ((gravity <= 0) || (velocity[2] * velocity[2] + 2 * gravity * above <= 0))
	{
		return false;
	}

	drop_time = (velocity[2] + sqrt(velocity[2] * velocity[2] + 2 * gravity * above)) / gravity;
	if (drop_time < g_globalvars.frametime)
	{
		return false;
	}

	// the horizontal velocity that is over the target when the bot drops to its height
	VectorSubtract(target, self->s.v.origin, wish);
	VectorScale(wish, 1 / drop_time, wish);
	wish[2] = 0;
	if ((wish[0] * wish[0] + wish[1] * wish[1])
			>= (velocity[0] * velocity[0] + velocity[1] * velocity[1]))
	{
		return false;
	}

	VectorSubtract(wish, velocity, wish);
	wish[2] = 0;
	normalize(wish, dir_command);

	return true;
}

// Points dir_command, which starts out as dir_move_, where the bot speeds up most toward dir_move_
static void ApplyPhysics(gedict_t *self, vec3_t dir_command)
{
	float drop = 0;
	vec3_t expected_velocity;
	float vel_length = 0;
	float hor_speed_squared;
	float movement_skill = bound(0, self->fb.skill.movement, 1.0);
	qbool onGround = (int)self->s.v.flags & FL_ONGROUND;

	// Just perform the move if we're backing away
	if (FUTURE(arrow_time2))
	{
		return;
	}

	if ((deathmatch >= 4) && isDuel() && !self->fb.skill.wiggle_run_dmm4)
	{
		return;
	}

	// Step 1: Apply friction
	VectorCopy(self->s.v.velocity, expected_velocity);
	vel_length = VectorLength(expected_velocity);
	if (vel_length < 1)
	{
		return;
	}

	if (self->s.v.waterlevel >= 2)
	{
		// Swimming...
		float waterfriction = cvar("sv_waterfriction");

		drop = vel_length * waterfriction * self->s.v.waterlevel * g_globalvars.frametime;
	}
	else if (onGround)
	{
		// FIXME: friction is doubled if player is about to drop off a ledge
		float stopspeed = cvar("sv_stopspeed");
		float friction = cvar("sv_friction");
		float control = vel_length < stopspeed ? stopspeed : vel_length;

		drop = control * friction * g_globalvars.frametime;
	}

	if (drop)
	{
		float new_vel = max(vel_length - drop, 0);

		VectorScale(expected_velocity, new_vel / vel_length, expected_velocity);

		vel_length = new_vel;
	}
	else
	{
		vel_length = VectorLength(expected_velocity);
	}

	// Step 2: change direction to maximise acceleration in desired direction
	if (self->s.v.waterlevel >= 2)
	{
		// Water movement
	}
	else
	{
		float min_numerator = onGround ? 319 : 29;
		float max_numerator = onGround ? 281.6 : -8.4;
		float used_numerator;
		float max_incr;
		vec3_t current_direction;
		vec3_t original_direction;

		// Gravity kicks in
		/*
		 if (!onGround)
		 expected_velocity[2] -= self->gravity * cvar ("sv_gravity") * g_globalvars.frametime;
		 else
		 expected_velocity[2] = 0;
		 */

		// Ground & air acceleration is the same
		hor_speed_squared = (expected_velocity[0] * expected_velocity[0]
				+ expected_velocity[1] * expected_velocity[1]);
		if (onGround && hor_speed_squared < sv_maxspeed * sv_maxspeed * 0.8 * 0.8)
		{
			return;
		}

		if ((self->fb.path_state & BOTPATH_CURLJUMP_HINT) && !onGround && self->fb.linked_marker
				&& CurlJumpBrake(self, expected_velocity, dir_command))
		{
			return;
		}

		VectorCopy(self->fb.dir_move_, original_direction);
		original_direction[2] = 0;
		normalize(original_direction, original_direction);
		normalize(expected_velocity, current_direction);
		used_numerator = min_numerator + movement_skill * (max_numerator - min_numerator);
		max_incr = used_numerator * used_numerator;
		if (hor_speed_squared >= max_incr)
		{
			vec3_t perpendicular;
			vec3_t up_vector =
				{ 0, 0, 1 };
			float rotation = acos(max_incr / hor_speed_squared) * 180 / M_PI;

			// Find out if rotation should be positive or negative
			CrossProduct(current_direction, original_direction, perpendicular);

			if ((self->fb.path_state & BOTPATH_CURLJUMP_HINT) && !onGround)
			{
				// curl back toward the target every frame, as a player strafe-turning in the air
				if (perpendicular[2] < 0)
				{
					rotation = -rotation;
				}
			}
			else if (deathmatch == 4)
			{
				if (self->fb.wiggle_run_dir == 0)
				{
					self->fb.wiggle_increasing = perpendicular[2] > 0;
					self->fb.wiggle_run_dir = self->fb.wiggle_increasing ? 1 : -1;
				}
				else if ((self->fb.wiggle_run_dir > self->fb.skill.wiggle_run_limit)
						&& (perpendicular[2] < 0))
				{
					self->fb.wiggle_increasing = false;
				}
				else if ((self->fb.wiggle_run_dir < -self->fb.skill.wiggle_run_limit)
						&& (perpendicular[2] > 0))
				{
					self->fb.wiggle_increasing = 1;
				}
				else if (self->fb.wiggle_increasing)
				{
					++self->fb.wiggle_run_dir;
				}
				else
				{
					--self->fb.wiggle_run_dir;
				}

				if (self->fb.wiggle_increasing)
				{
					rotation = -rotation;
				}
			}
			else if (perpendicular[2] < 0)
			{
				rotation = -rotation;
			}

			if (rotation)
			{
				vec3_t proposed_dir;
				vec3_t vel_after_rot;
				vec3_t vel_std;
				float dp_std, dp_rot;

				RotatePointAroundVector(proposed_dir, up_vector, current_direction, rotation);

				// Calculate what mvdsv will do (roughly)
				PM_Accelerate(expected_velocity, (int)self->s.v.flags & FL_ONGROUND, proposed_dir,
								vel_after_rot, false);
				PM_Accelerate(expected_velocity, (int)self->s.v.flags & FL_ONGROUND,
								current_direction, vel_std, false);

				// Only rotate if 'better' than moving normally
				dp_rot = DotProduct(vel_after_rot, original_direction);
				dp_std = DotProduct(vel_std, original_direction);

				if ((dp_rot > dp_std) || (dp_rot >= 0.9))
				{
					VectorCopy(proposed_dir, dir_command);
					if (self->fb.debug_path)
					{
						PM_Accelerate(expected_velocity, (int)self->s.v.flags & FL_ONGROUND,
										proposed_dir, vel_after_rot, true);
					}
				}
				else if (self->fb.debug_path)
				{
					PM_Accelerate(expected_velocity, (int)self->s.v.flags & FL_ONGROUND,
									current_direction, vel_std, true);
				}
			}
			else
			{
#ifdef DEBUG_MOVEMENT
				if (self->fb.debug_path && ! onGround) {
					G_bprint (PRINT_HIGH, "> AirControl rotation: <ignoring>\n");
				}
#endif
			}
		}
	}
}

float AverageTraceAngle(gedict_t *self, qbool debug, qbool report)
{
	vec3_t back_left, projection, incr;
	int angles[] =
		{ 45, 30, 15, 0, -15, -30, -45 };
	int i;
	float best_angle = 0;
	float best_angle_frac = 0;
	float total_angle = 0;
	float avg_angle;

	float distance = 320;

	if (self->fb.path_state & JUMP_LEDGE)
	{
		return 0;
	}

	if (debug)
	{
		trap_makevectors(self->s.v.angles);
	}
	else
	{
		trap_makevectors(self->fb.dir_move_);
	}

	VectorMA(self->s.v.origin, -VEC_HULL_MIN[0], g_globalvars.v_forward, back_left);
	VectorMA(back_left, VEC_HULL_MIN[1], g_globalvars.v_right, back_left);

	VectorScale(g_globalvars.v_right,
				(VEC_HULL_MAX[0] - VEC_HULL_MIN[0]) / (sizeof(angles) / sizeof(angles[0]) - 1),
				incr);

	if (debug)
	{
		G_bprint(2, "Current origin: %d %d %d\n", PASSINTVEC3(self->s.v.origin));
		G_bprint(2, "Current angles: %d %d\n", PASSINTVEC2(self->s.v.angles));
	}

	for (i = 0; i < sizeof(angles) / sizeof(angles[0]); ++i)
	{
		int angle = angles[i];

		RotatePointAroundVector(projection, g_globalvars.v_up, g_globalvars.v_forward, angle);
		VectorMA(back_left, distance, projection, projection);
		traceline(PASSVEC3(back_left), PASSVEC3(projection), false, self);

		if (g_globalvars.trace_fraction == 1)
		{
			total_angle += angle * 1.5; // bonus for success
		}
		else if (g_globalvars.trace_fraction > 0.4)
		{
			total_angle += angle * g_globalvars.trace_fraction;
		}

		if (debug)
		{
			G_bprint(2, "Angle: %d => [%d %d %d] [%d %d %d] = %f\n", angle, PASSINTVEC3(back_left),
						PASSINTVEC3(projection), g_globalvars.trace_fraction);
		}

		if ((i == 0) || g_globalvars.trace_fraction > best_angle_frac)
		{
			best_angle = angle;
			best_angle_frac = g_globalvars.trace_fraction;
		}

		VectorAdd(back_left, incr, back_left);
	}

	avg_angle = total_angle / (sizeof(angles) / sizeof(angles[0]));

	if (debug)
	{
		G_bprint(2, "Best angle: %f\n", best_angle);
		G_bprint(2, "Total angle: %f\n", avg_angle);
	}

	return avg_angle;
}

static void BestJumpingDirection(gedict_t *self, vec3_t dir_command)
{
	float raw_avg_angle = AverageTraceAngle(self, false, self->fb.debug_path);
	float avg_angle;

	if (raw_avg_angle < 0)
	{
		avg_angle = min(raw_avg_angle, -15);
	}
	else
	{
		avg_angle = max(raw_avg_angle, 15);
	}

	RotatePointAroundVector(dir_command, g_globalvars.v_up, g_globalvars.v_forward, avg_angle);
}

// players keep jumping on landing to hold speed on open runs
#define BUNNY_HOP_SPEED 300
#define BUNNY_HOP_TOP 600
#define BUNNY_HOP_EAGER 450			// from here a first hop may bend like a later one
#define BUNNY_HOP_AIRTIME 0.675		// standing jump at sv_gravity 800
#define BUNNY_HOP_AIM 0.5			// cos 60 degrees, how far off the heading the linked marker may be
#define BUNNY_HOP_ROUTE 4			// markers to look along the route
#define BUNNY_HOP_STEPS 15			// flight simulation steps for a hop that lands level
#define BUNNY_HOP_MAX_STEPS 25		// and for one that drops BUNNY_HOP_DROP
#define BUNNY_HOP_DROP 160			// players jump down ledges rather than walk off; this lands under fall damage
#define BUNNY_HOP_STEP (BUNNY_HOP_AIRTIME / BUNNY_HOP_STEPS)
#define BUNNY_HOP_AIR_TURN 2000		// sideways acceleration air control gives, a bit under 30 per 13 ms frame
#define BUNNY_HOP_TOUCH 64			// steer for the next spot this close to one
#define BUNNY_HOP_ITEM_HEIGHT 16		// higher than this over an item, the bot misses it
#define BUNNY_HOP_RUN_ON 0.2			// seconds of running after landing that need floor too
#define BUNNY_HOP_FIRST_TURN 0.35	// radians, about 20 degrees of steering in a first hop
#define BUNNY_HOP_SIDE 32			// floor needed either side of the flight
#define BUNNY_HOP_MARGIN 6			// room the flight needs beside the player box
#define BUNNY_HOP_PATHS (JUMP_LEDGE | BOTPATH_CURLJUMP_HINT | DELIBERATE_AIR | WAIT_GROUND | ROCKET_JUMP \
		| VERTICAL_PLATFORM | NO_DODGE | BOTPATH_DOOR | BOTPATH_RJ_IN_PROGRESS | WATER_PATH | STUCK_PATH \
		| DELIBERATE_BACKUP)

static int PathFlags(gedict_t *from_marker, gedict_t *to_marker)
{
	int i;

	for (i = 0; i < NUMBER_PATHS; ++i)
	{
		if (from_marker->fb.paths[i].next_marker == to_marker)
		{
			return from_marker->fb.paths[i].flags;
		}
	}

	return 0;
}

static gedict_t* StraightestPath(gedict_t *from_marker, vec3_t heading)
{
	gedict_t *best = NULL;
	float best_dot = -2;
	int i;

	for (i = 0; i < NUMBER_PATHS; ++i)
	{
		gedict_t *next = from_marker->fb.paths[i].next_marker;
		vec3_t dir;
		float dot;

		if (!next)
		{
			continue;
		}

		VectorSubtract(next->s.v.origin, from_marker->s.v.origin, dir);
		dir[2] = 0;
		VectorNormalize(dir);
		dot = DotProduct(dir, heading);
		if (dot > best_dot)
		{
			best = next;
			best_dot = dot;
		}
	}

	return best;
}

// Up to BUNNY_HOP_ROUTE spots along the route on plain paths, the linked marker first,
//   and in item the first that is an item the bot would pick up, or -1
static int RouteAhead(gedict_t *self, vec3_t heading, vec3_t *spots, int *item)
{
	gedict_t *goal = &g_edicts[self->s.v.goalentity];
	gedict_t *goal_marker = (goal == world || (self->fb.state & RUNAWAY)) ? NULL : goal->fb.touch_marker;
	gedict_t *m = self->fb.linked_marker;
	int n = 0;

	*item = -1;
	while (m && (n < BUNNY_HOP_ROUTE))
	{
		gedict_t *next;

		VectorAdd(m->s.v.absmin, m->s.v.view_ofs, spots[n]);
		if ((spots[n][2] - self->s.v.origin[2] > 40) || (self->s.v.origin[2] - spots[n][2] > BUNNY_HOP_DROP))
		{
			break;
		}

		// a marker can have no classname, so check before comparing it; the bot can fly over
		//   an item that is respawning or that it has no use for
		if ((*item < 0) && m->classname
				&& (!strncmp(m->classname, "item_", 5) || !strncmp(m->classname, "weapon_", 7))
				&& !WaitingToRespawn(m) && m->fb.desire && (m->fb.desire(self, m) > 0))
		{
			*item = n;
		}

		++n;
		if (m == goal_marker)
		{
			break;
		}

		// with no goal the route is unknown, so carry on along the path closest to straight ahead
		next = goal_marker ? RouteNextMarker(m, goal_marker, self->fb.canRocketJump) : StraightestPath(m, heading);
		if (!next || (PathFlags(m, next) & BUNNY_HOP_PATHS))
		{
			break;
		}

		m = next;
	}

	return n;
}

// True if there is floor within depth below point that is neither lava nor slime, with its height in floor_z
static qbool FloorBelow(gedict_t *self, vec3_t point, float depth, float *floor_z)
{
	int contents;

	traceline(PASSVEC3(point), point[0], point[1], point[2] - depth, true, self);
	if (g_globalvars.trace_fraction == 1)
	{
		return false;
	}

	*floor_z = g_globalvars.trace_endpos[2];
	contents = trap_pointcontents(g_globalvars.trace_endpos[0], g_globalvars.trace_endpos[1],
									g_globalvars.trace_endpos[2] + 4);

	return (contents != CONTENT_LAVA) && (contents != CONTENT_SLIME);
}

// True if a hop now, steering round the route in the air as fast as air control turns,
//   stays clear of walls and over safe floor until it lands, at most BUNNY_HOP_DROP lower,
//   turning no more than max_total radians
static qbool BunnyHopFlightClear(gedict_t *self, vec3_t velocity, vec3_t *spots, int n, int item, float max_total)
{
	vec3_t pos, vel;
	float speed = VectorLength(velocity);
	float total = 0, t, floor_z = 0;
	float max_turn = BUNNY_HOP_AIR_TURN / speed * BUNNY_HOP_STEP;
	float lowest = self->s.v.origin[2] + VEC_HULL_MIN[2] - BUNNY_HOP_DROP;
	qbool landed = false;
	int i, k = 0, side;

	VectorCopy(self->s.v.origin, pos);
	VectorCopy(velocity, vel);
	for (i = 1; (i <= BUNNY_HOP_MAX_STEPS) && !landed; ++i)
	{
		vec3_t to_spot, next;
		float turn;

		VectorSubtract(spots[k], pos, to_spot);
		to_spot[2] = 0;
		if ((VectorLength(to_spot) < BUNNY_HOP_TOUCH) && (k < n - 1))
		{
			++k;
			VectorSubtract(spots[k], pos, to_spot);
			to_spot[2] = 0;
		}

		// signed angle from the velocity to the spot, limited to what air control turns in a step
		turn = atan2(vel[0] * to_spot[1] - vel[1] * to_spot[0], DotProduct(vel, to_spot));
		turn = bound(-max_turn, turn, max_turn);
		total += fabs(turn);
		if (total > max_total)
		{
			return false;
		}

		next[0] = vel[0] * cos(turn) - vel[1] * sin(turn);
		next[1] = vel[0] * sin(turn) + vel[1] * cos(turn);
		vel[0] = next[0];
		vel[1] = next[1];

		VectorMA(pos, BUNNY_HOP_STEP, vel, next);
		t = i * BUNNY_HOP_STEP;
		next[2] = self->s.v.origin[2] + (JUMPSPEED - 400 * t) * t;

		// the floor under this step, from where the arc is down to the lowest landing allowed
		{
			vec3_t probe;

			VectorCopy(next, probe);
			probe[2] = max(next[2], pos[2]);
			if (!FloorBelow(self, probe, probe[2] - lowest, &floor_z))
			{
				return false;
			}
		}

		// past the top of the arc, the feet reaching the floor is the landing
		if ((t > JUMPSPEED / 800.0) && (next[2] + VEC_HULL_MIN[2] <= floor_z + 1))
		{
			next[2] = floor_z - VEC_HULL_MIN[2] + 1;
			landed = true;
		}

		// the whole player box along the jump's arc, so walls beside the line and steps under it count,
		//   also a few units to each side as the flown path drifts from this one; other players count
		//   in the middle trace
		for (side = -1; side <= 1; ++side)
		{
			vec3_t from, to;
			float dx = -side * vel[1] / speed * BUNNY_HOP_MARGIN, dy = side * vel[0] / speed * BUNNY_HOP_MARGIN;

			VectorCopy(pos, from);
			VectorCopy(next, to);
			from[0] += dx;
			from[1] += dy;
			to[0] += dx;
			to[1] += dy;
			TraceCapsule(PASSVEC3(from), PASSVEC3(to), side != 0, self, PASSVEC3(VEC_HULL_MIN), PASSVEC3(VEC_HULL_MAX));
			if ((g_globalvars.trace_fraction != 1) || g_globalvars.trace_startsolid)
			{
				return false;
			}
		}

		VectorCopy(next, pos);

		// sailing over an item on the route misses it, and the bot turns back for it in the air
		if ((k == item) && (pos[2] + VEC_HULL_MIN[2] - floor_z > BUNNY_HOP_ITEM_HEIGHT))
		{
			VectorSubtract(spots[k], pos, to_spot);
			to_spot[2] = 0;
			if (VectorLength(to_spot) < BUNNY_HOP_TOUCH)
			{
				return false;
			}
		}

		if (i % (BUNNY_HOP_STEPS / 3))
		{
			continue;
		}

		// air control drifts the bot sideways, so a walkway over lava is no place to hop
		for (side = -1; side <= 1; ++side)
		{
			vec3_t point;
			float side_z;

			VectorCopy(pos, point);
			point[0] -= side * vel[1] / speed * BUNNY_HOP_SIDE;
			point[1] += side * vel[0] / speed * BUNNY_HOP_SIDE;
			if (!FloorBelow(self, point, point[2] - lowest, &side_z))
			{
				return false;
			}
		}
	}

	if (!landed)
	{
		return false;
	}

	// fast landings run on, and an edge there is a fall the bot cannot stop for
	for (i = 1; i <= 4; ++i)
	{
		vec3_t point;
		float run_z;

		VectorMA(pos, BUNNY_HOP_RUN_ON * i / 4, vel, point);
		if (!FloorBelow(self, point, 64, &run_z))
		{
			return false;
		}
	}

	return true;
}

// True if the bot can jump now and land on clear floor, steering round its route in the air.
//   Without ground friction between hops, air control keeps adding speed
static qbool BotBunnyHop(gedict_t *self)
{
	vec3_t velocity, heading, to_spot;
	vec3_t spots[BUNNY_HOP_ROUTE];
	float speed;
	int n, item;

	if ((match_in_progress != 2) || !((int)self->s.v.flags & FL_ONGROUND) || self->s.v.waterlevel
			|| self->fb.rocketJumping || self->fb.debug_path || !self->fb.linked_marker
			|| (self->fb.path_state & BUNNY_HOP_PATHS) || self->fb.enemy_visible || FUTURE(arrow_time2))
	{
		return false;
	}

	VectorCopy(self->s.v.velocity, velocity);
	velocity[2] = 0;
	speed = VectorLength(velocity);
	// players rarely carry more than about 600 u/s, however long the run
	if ((speed < BUNNY_HOP_SPEED) || (speed > BUNNY_HOP_TOP))
	{
		return false;
	}

	VectorScale(velocity, 1 / speed, heading);
	n = RouteAhead(self, heading, spots, &item);
	if (!n)
	{
		return false;
	}

	VectorSubtract(spots[0], self->s.v.origin, to_spot);
	to_spot[2] = 0;
	VectorNormalize(to_spot);
	if (DotProduct(heading, to_spot) < BUNNY_HOP_AIM)
	{
		return false;
	}

	// head room
	traceline(PASSVEC3(self->s.v.origin), self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] + 48,
				true, self);
	if (g_globalvars.trace_fraction != 1)
	{
		return false;
	}

	// a first hop wants a straight run unless the bot is already fast, as players start chains readily
	//   at speed; once hopping, the bot keeps it up round bends
	if (!BunnyHopFlightClear(self, velocity, spots, n, item,
			(g_globalvars.time - self->fb.bunny_hop_time < 1) || (speed >= BUNNY_HOP_EAGER) ? M_PI : BUNNY_HOP_FIRST_TURN))
	{
		return false;
	}

	self->fb.bunny_hop_time = g_globalvars.time;

	return true;
}

void BotSetCommand(gedict_t *self)
{
	extern float last_frame_time;
	float msec_since_last = (last_frame_time - self->fb.last_cmd_sent) * 1000;
	int cmd_msec = (int)msec_since_last;
	int weapon_script_impulse = 0;
	int impulse = 0, buttons = 0;
	qbool jumping;
	qbool bunny_hop;
	qbool firing;
	vec3_t direction;
	vec3_t dir_command;

	BotPerformRocketJump(self);

	if (cmd_msec)
	{
		self->fb.cmd_msec_lost += (msec_since_last - cmd_msec);
		if (self->fb.cmd_msec_lost >= 1)
		{
			self->fb.cmd_msec_lost -= 1;
			cmd_msec += 1;
		}
	}
	else if (self->fb.cmd_msec_last)
	{
		// Probably re-sending after blocked(), re-use old number
		cmd_msec = self->fb.cmd_msec_last;
	}
	else
	{
		cmd_msec = 12;
	}

	//G_sprint(self, PRINT_HIGH, "Movement length @ %f: %d\n", last_frame_time, cmd_msec);

	// dir_move_ is the direction we want to move in, but need to take inertia into effect
	// ... as rough guide (and save doubling physics calculations), scale command > 
	VectorNormalize(self->fb.dir_move_);
	VectorScale(self->fb.dir_move_, sv_maxspeed, self->fb.last_cmd_direction);

	trap_makevectors(self->fb.desired_angle);

	// During intermission, always do nothing and leave humans to change level
	if (intermission_running)
	{
		self->fb.firing = self->fb.jumping = false;
	}
	else if (teamplay && (deathmatch == 1) && !self->fb.firing)
	{
		// Weaponscripts
		if ((self->s.v.weapon != IT_SHOTGUN) && (self->s.v.weapon != IT_AXE))
		{
			weapon_script_impulse = (self->s.v.ammo_shells ? 2 : 1);
		}
	}

	impulse = self->fb.botchose ? self->fb.next_impulse :
				self->fb.firing ? self->fb.desired_weapon_impulse : weapon_script_impulse;

	if (self->fb.firing && BotUsingCorrectWeapon(self))
	{
		impulse = 0; // we already have the requested weapon
	}

	jumping = self->fb.jumping || self->fb.waterjumping;
	bunny_hop = !jumping && BotBunnyHop(self);
	jumping |= bunny_hop;
	firing = self->fb.firing;

	self->fb.waterjumping = false;

	if (self->fb.dbg_countdown > 0)
	{
		jumping = firing = false;
		VectorClear(direction);
		--self->fb.dbg_countdown;
	}
	else
	{
		// leave dir_move_ alone: bots in the air only set it every 0.15s, and each frame would turn
		//   further from it
		VectorCopy(self->fb.dir_move_, dir_command);
		if (jumping && !bunny_hop && ((int)self->s.v.flags & FL_ONGROUND))
		{
			BestJumpingDirection(self, dir_command);
		}
		else
		{
			ApplyPhysics(self, dir_command);
		}

		if (self->s.v.waterlevel <= 1)
		{
			vec3_t hor;

			VectorCopy(dir_command, hor);
			hor[2] = 0;
			VectorNormalize(hor);
			VectorScale(hor, 800, hor);

			direction[0] = DotProduct(g_globalvars.v_forward, hor);
			direction[1] = DotProduct(g_globalvars.v_right, hor);
			direction[2] = 0;
		}
		else
		{
			direction[0] = DotProduct (g_globalvars.v_forward, dir_command) * 800;
			direction[1] = DotProduct (g_globalvars.v_right, dir_command) * 800;
			direction[2] = DotProduct (g_globalvars.v_up, dir_command) * 800;
		}

#ifdef DEBUG_MOVEMENT
		if (self->fb.debug_path) {
			G_bprint (PRINT_HIGH, "     : final direction sent [%4.1f %4.1f %4.1f]\n", PASSVEC3 (dir_command));
		}
#endif
	}

	self->fb.desired_angle[2] = 0;

	if (ISDEAD(self))
	{
		firing = false;
		jumping = BotRequestRespawn(self);
		VectorClear(direction);
		impulse = 0;
	}
	else if (self->fb.min_move_time > g_globalvars.time)
	{
		VectorClear(direction);
	}

	// Keep bots on spawns before match start
	if (match_in_progress != 2 && cvar(FB_CVAR_FREEZE_PREWAR))
	{
		jumping = firing = false;
		VectorClear(direction);
		impulse = 0;
	}

	buttons |= (firing ? 1 : 0);
	buttons |= (jumping ? 2 : 0);
	trap_SetBotCMD(NUM_FOR_EDICT(self), cmd_msec, PASSVEC3(self->fb.desired_angle),
					PASSVEC3(direction), buttons, impulse);

	self->fb.next_impulse = 0;
	self->fb.botchose = false;
	self->fb.last_cmd_sent = last_frame_time;
	self->fb.cmd_msec_last = cmd_msec;

	VectorClear(self->fb.obstruction_normal);
	if (self->s.v.button0 && !firing)
	{
		// Stopped firing, randomise next time
		self->fb.last_rndaim_time = 0;
	}

	self->fb.prev_look_object = self->fb.look_object;
	VectorCopy(self->s.v.velocity, self->fb.prev_velocity);
}

#endif
