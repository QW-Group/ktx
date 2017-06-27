
#ifdef BOT_SUPPORT

#include "g_local.h"
#include "fb_globals.h"

#define ARROW_TIME_INCREASE       0.15  // Seconds to advance after NewVelocityForArrow
#define MIN_DEAD_TIME 0.2f
#define MAX_DEAD_TIME 1.0f

//#define DEBUG_MOVEMENT

void SetLinkedMarker (gedict_t* player, gedict_t* marker, char* explanation)
{
	gedict_t* touch = player->fb.touch_marker;

	if (marker != player->fb.linked_marker && FrogbotOptionEnabled (FB_OPTION_SHOW_MOVEMENT_LOGIC))
		G_sprint (player, 2, "linked to %3d/%s, touching %3d/%s g %s (%s)\n", marker ? marker->fb.index + 1 : -1, marker ? marker->classname : "(null)", touch ? touch->fb.index + 1 : -1, touch ? touch->classname : "(null)", g_edicts[player->s.v.goalentity].classname, explanation ? explanation : "");
	if (player->fb.debug_path)
		G_bprint (2, "%3.2f: linked to %3d/%s, touching %3d/%s g %s (%s)\n", g_globalvars.time, marker ? marker->fb.index + 1 : -1, marker ? marker->classname : "(null)", touch ? touch->fb.index + 1 : -1, touch ? touch->classname : "(null)", g_edicts[player->s.v.goalentity].classname, explanation ? explanation : "");

	player->fb.linked_marker = marker;
}

void SetJumpFlag (gedict_t* player, qbool jumping, const char* explanation)
{
	if (jumping != player->fb.jumping) {
		if (self->fb.debug_path) {
			G_bprint (PRINT_HIGH, "%3.2f: jumping (%s)\n", g_globalvars.time, explanation);
		}
		if (FrogbotOptionEnabled (FB_OPTION_SHOW_MOVEMENT_LOGIC)) {
			G_sprint (player, PRINT_HIGH, "%3.2f: jumping (%s)\n", g_globalvars.time, explanation);
		}
	}
	player->fb.jumping = jumping;
}

void SetDirectionMove (gedict_t* self, vec3_t dir_move, const char* explanation)
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

void NewVelocityForArrow(gedict_t* self, vec3_t dir_move, const char* explanation)
{
	SetDirectionMove (self, dir_move, explanation);
	self->fb.arrow_time = g_globalvars.time + ARROW_TIME_INCREASE;
}

static qbool BotRequestRespawn(gedict_t* self)
{
	float time_dead = min (g_globalvars.time - self->fb.last_death, MAX_DEAD_TIME);

	return self->s.v.deadflag == DEAD_RESPAWNABLE && time_dead > MIN_DEAD_TIME && (g_random () < (time_dead / MAX_DEAD_TIME));
}

static void PM_Accelerate (vec3_t vel_after_friction, qbool onGround, vec3_t orig_wishdir, vec3_t vel_result, qbool trace)
{
	float addspeed, accelspeed, currentspeed;
	float wishspeed = 320; // FIXME: assuming attemping sv_maxspeed
	float accel = 10; // FIXME: assumption
	vec3_t velocity;
	vec3_t wishdir;

	VectorCopy (vel_after_friction, velocity);
	if (onGround) {
		velocity[2] = 0;
	}

	wishdir[0] = orig_wishdir[0];
	wishdir[1] = orig_wishdir[1];
	wishdir[2] = 0;
	wishspeed = VectorNormalize (wishdir);
	wishspeed = sv_maxspeed; // fixme: we scale back up to maximum just as passing command

	currentspeed = DotProduct (velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0) {
		VectorCopy (vel_after_friction, vel_result);
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

static void ApplyPhysics (gedict_t* self)
{
	float drop = 0;
	vec3_t expected_velocity;
	float vel_length = 0;
	float hor_speed_squared;
	float movement_skill = bound (0, self->fb.skill.movement, 1.0);
	qbool onGround = (int)self->s.v.flags & FL_ONGROUND;

	// Just perform the move if we're backing away
	if (FUTURE (arrow_time2)) {
		return;
	}

	if (deathmatch >= 4 && isDuel() && !self->fb.skill.wiggle_run_dmm4) {
		return;
	}

	// Step 1: Apply friction
	VectorCopy (self->s.v.velocity, expected_velocity);
	vel_length = VectorLength (expected_velocity);
	if (vel_length < 1)
		return;

	if (self->s.v.waterlevel >= 2) {
		// Swimming...
		float waterfriction = cvar ("sv_waterfriction");

		drop = vel_length * waterfriction * self->s.v.waterlevel * g_globalvars.frametime;
	}
	else if (onGround) {
		// FIXME: friction is doubled if player is about to drop off a ledge
		float stopspeed = cvar ("sv_stopspeed");
		float friction = cvar ("sv_friction");
		float control = vel_length < stopspeed ? stopspeed : vel_length;

		drop = control * friction * g_globalvars.frametime;
	}

	if (drop) {
		float new_vel = max (vel_length - drop, 0);

		VectorScale (expected_velocity, new_vel / vel_length, expected_velocity);

		vel_length = new_vel;
	}
	else {
		vel_length = VectorLength (expected_velocity);
	}

	// Step 2: change direction to maximise acceleration in desired direction
	if (self->s.v.waterlevel >= 2) {
		// Water movement
	}
	else {
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
		hor_speed_squared = (expected_velocity[0] * expected_velocity[0] + expected_velocity[1] * expected_velocity[1]);
		if (onGround && hor_speed_squared < sv_maxspeed * sv_maxspeed * 0.8 * 0.8) {
			return;
		}

		self->fb.dir_move_[2] = 0;
		normalize(self->fb.dir_move_, original_direction);
		normalize(expected_velocity, current_direction);
		used_numerator = min_numerator + movement_skill * (max_numerator - min_numerator);
		max_incr = used_numerator * used_numerator;
		if (hor_speed_squared >= max_incr) {
			vec3_t perpendicular;
			vec3_t up_vector = { 0, 0, 1 };
			float dot_prod = 0.0f;
			float rotation = acos(max_incr / hor_speed_squared) * 180 / M_PI;

			// Find out if rotation should be positive or negative
			CrossProduct (current_direction, original_direction, perpendicular);

			if ((self->fb.path_state & BOTPATH_CURLJUMP_HINT) && !onGround) {
				// Once in the air, we rotate in opposite direction
				// FIXME: THIS IS UGLY HACK
				if (framecount % 3) {
					rotation = 0;
				}
				else if (self->fb.angle_hint > 0) {
					rotation = -rotation;
				}
			}
			else if (deathmatch == 4) {
				if (self->fb.wiggle_run_dir == 0) {
					self->fb.wiggle_increasing = perpendicular[2] > 0;
					self->fb.wiggle_run_dir = self->fb.wiggle_increasing ? 1 : -1;
				}
				else if (self->fb.wiggle_run_dir > self->fb.skill.wiggle_run_limit && perpendicular[2] < 0) {
					self->fb.wiggle_increasing = false;
				}
				else if (self->fb.wiggle_run_dir < -self->fb.skill.wiggle_run_limit && perpendicular[2] > 0) {
					self->fb.wiggle_increasing = 1;
				}
				else if (self->fb.wiggle_increasing) {
					++self->fb.wiggle_run_dir;
				}
				else {
					--self->fb.wiggle_run_dir;
				}

				if (self->fb.wiggle_increasing) {
					rotation = -rotation;
				}
			}
			else if (perpendicular[2] < 0) {
				rotation = -rotation;
			}

			if (rotation) {
				vec3_t proposed_dir;
				vec3_t vel_after_rot;
				vec3_t vel_std;
				float dp_std, dp_rot;

				RotatePointAroundVector (proposed_dir, up_vector, current_direction, rotation);

				// Calculate what mvdsv will do (roughly)
				PM_Accelerate (expected_velocity, (int)self->s.v.flags & FL_ONGROUND, proposed_dir, vel_after_rot, false);
				PM_Accelerate (expected_velocity, (int)self->s.v.flags & FL_ONGROUND, current_direction, vel_std, false);

				// Only rotate if 'better' than moving normally
				dp_rot = DotProduct (vel_after_rot, original_direction);
				dp_std = DotProduct (vel_std, original_direction);

				if (dp_rot > dp_std || dp_rot >= 0.9) {
					VectorCopy (proposed_dir, self->fb.dir_move_);
					if (self->fb.debug_path) {
						PM_Accelerate(expected_velocity, (int)self->s.v.flags & FL_ONGROUND, proposed_dir, vel_after_rot, true);
					}
				}
				else if (self->fb.debug_path) {
					PM_Accelerate (expected_velocity, (int)self->s.v.flags & FL_ONGROUND, current_direction, vel_std, true);
				}
			}
			else {
#ifdef DEBUG_MOVEMENT
				if (self->fb.debug_path && ! onGround) {
					G_bprint (PRINT_HIGH, "> AirControl rotation: <ignoring>\n");
				}
#endif
			}
		}
	}
}

float AverageTraceAngle (gedict_t* self, qbool debug, qbool report)
{
	vec3_t back_left, projection, incr;
	int angles[] = { 45, 30, 15, 0, -15, -30, -45 };
	int i;
	float best_angle = 0;
	float best_angle_frac = 0;
	float total_angle = 0;
	float avg_angle;

	float distance = 320;

	if (self->fb.path_state & JUMP_LEDGE)
		return 0;

	if (debug) {
		trap_makevectors (self->s.v.angles);
	}
	else {
		trap_makevectors (self->fb.dir_move_);
	}
	VectorMA (self->s.v.origin, -VEC_HULL_MIN[0], g_globalvars.v_forward, back_left);
	VectorMA (back_left, VEC_HULL_MIN[1], g_globalvars.v_right, back_left);

	VectorScale (g_globalvars.v_right, (VEC_HULL_MAX[0] - VEC_HULL_MIN[0]) / (sizeof(angles) / sizeof(angles[0]) - 1), incr);

	if (debug) {
		G_bprint (2, "Current origin: %d %d %d\n", PASSINTVEC3 (self->s.v.origin));
		G_bprint (2, "Current angles: %d %d\n", PASSINTVEC3 (self->s.v.angles));
	}

	for (i = 0; i < sizeof (angles) / sizeof (angles[0]); ++i) {
		int angle = angles[i];

		RotatePointAroundVector (projection, g_globalvars.v_up, g_globalvars.v_forward, angle);
		VectorMA (back_left, distance, projection, projection);
		traceline (PASSVEC3 (back_left), PASSVEC3 (projection), false, self);

		if (g_globalvars.trace_fraction == 1) {
			total_angle += angle * 1.5; // bonus for success
		}
		else if (g_globalvars.trace_fraction > 0.4) {
			total_angle += angle * g_globalvars.trace_fraction;
		}

		if (debug) {
			G_bprint (2, "Angle: %d => [%d %d %d] [%d %d %d] = %f\n", angle, PASSINTVEC3 (back_left), PASSINTVEC3 (projection), g_globalvars.trace_fraction);
		}

		if (i == 0 || g_globalvars.trace_fraction > best_angle_frac) {
			best_angle = angle;
			best_angle_frac = g_globalvars.trace_fraction;
		}

		VectorAdd (back_left, incr, back_left);
	}

	avg_angle = total_angle / (sizeof (angles) / sizeof (angles[0]));

	if (debug) {
		G_bprint (2, "Best angle: %d\n", best_angle);
		G_bprint (2, "Total angle: %f\n", avg_angle);
	}
	return avg_angle;
}

static void BestJumpingDirection (gedict_t* self)
{
	float raw_avg_angle = AverageTraceAngle (self, false, self->fb.debug_path);
	float avg_angle;

	if (raw_avg_angle < 0)
		avg_angle = min (raw_avg_angle, -15);
	else
		avg_angle = max (raw_avg_angle, 15);

	RotatePointAroundVector (self->fb.dir_move_, g_globalvars.v_up, g_globalvars.v_forward, avg_angle);
}

void BotSetCommand (gedict_t* self)
{
	extern float last_frame_time;
	float msec_since_last = (last_frame_time - self->fb.last_cmd_sent) * 1000;
	int cmd_msec = (int)msec_since_last;
	int weapon_script_impulse = 0;
	int impulse = 0;
	qbool jumping;
	qbool firing;
	vec3_t direction;

	BotPerformRocketJump (self);

	if (cmd_msec) {
		self->fb.cmd_msec_lost += (msec_since_last - cmd_msec);
		if (self->fb.cmd_msec_lost >= 1) {
			self->fb.cmd_msec_lost -= 1;
			cmd_msec += 1;
		}
	}
	else if (self->fb.cmd_msec_last) {
		// Probably re-sending after blocked(), re-use old number
		cmd_msec = self->fb.cmd_msec_last;
	}
	else {
		cmd_msec = 12;
	}

	G_sprint(self, PRINT_HIGH, "Movement length @ %f: %d\n", last_frame_time, cmd_msec);

	// dir_move_ is the direction we want to move in, but need to take inertia into effect
	// ... as rough guide (and save doubling physics calculations), scale command > 
	VectorNormalize (self->fb.dir_move_);
	VectorScale (self->fb.dir_move_, sv_maxspeed, self->fb.last_cmd_direction);

	trap_makevectors (self->fb.desired_angle);

	// During intermission, always do nothing and leave humans to change level
	if (intermission_running) {
		self->fb.firing = self->fb.jumping = false;
	}
	else if (teamplay && deathmatch == 1 && !self->fb.firing) {
		// Weaponscripts
		if (self->s.v.weapon != IT_SHOTGUN && self->s.v.weapon != IT_AXE) {
			weapon_script_impulse = (self->s.v.ammo_shells ? 2 : 1);
		}
	}

	impulse =
		self->fb.botchose ? self->fb.next_impulse :
		self->fb.firing ? self->fb.desired_weapon_impulse :
		weapon_script_impulse;

	if (self->fb.firing && BotUsingCorrectWeapon (self)) {
		impulse = 0; // we already have the requested weapon
	}

	jumping = self->fb.jumping || self->fb.waterjumping;
	firing = self->fb.firing;

	self->fb.waterjumping = false;

	if (self->fb.dbg_countdown > 0) {
		jumping = firing = false;
		VectorClear (direction);
		--self->fb.dbg_countdown;
	}
	else {
		if (jumping && ((int)self->s.v.flags & FL_ONGROUND)) {
			BestJumpingDirection (self);
		}
		else {
			ApplyPhysics (self);
		}

		if (self->s.v.waterlevel <= 1) {
			vec3_t hor;

			VectorCopy (self->fb.dir_move_, hor);
			hor[2] = 0;
			VectorNormalize (hor);
			VectorScale (hor, 800, hor);

			direction[0] = DotProduct (g_globalvars.v_forward, hor);
			direction[1] = DotProduct (g_globalvars.v_right, hor);
			direction[2] = 0;
		}
		else {
			direction[0] = DotProduct (g_globalvars.v_forward, self->fb.dir_move_) * 800;
			direction[1] = DotProduct (g_globalvars.v_right, self->fb.dir_move_) * 800;
			direction[2] = DotProduct (g_globalvars.v_up, self->fb.dir_move_) * 800;
		}
#ifdef DEBUG_MOVEMENT
		if (self->fb.debug_path) {
			G_bprint (PRINT_HIGH, "     : final direction sent [%4.1f %4.1f %4.1f]\n", PASSVEC3 (self->fb.dir_move_));
		}
#endif
	}
	self->fb.desired_angle[2] = 0;

	if (ISDEAD (self)) {
		firing = false;
		jumping = BotRequestRespawn (self);
		VectorClear (direction);
		impulse = 0;
	}

	trap_SetBotCMD (
		NUM_FOR_EDICT (self),
		cmd_msec,
		PASSVEC3(self->fb.desired_angle),
		PASSVEC3(direction),
		(firing ? 1 : 0) | (jumping ? 2 : 0),
		impulse
	);

	self->fb.next_impulse = 0;
	self->fb.botchose = false;
	self->fb.last_cmd_sent = last_frame_time;
	self->fb.cmd_msec_last = cmd_msec;

	VectorClear (self->fb.obstruction_normal);
	if (self->s.v.button0 && !firing) {
		// Stopped firing, randomise next time
		self->fb.last_rndaim_time = 0;
	}
	self->fb.prev_look_object = self->fb.look_object;
	VectorCopy (self->s.v.velocity, self->fb.prev_velocity);
}

#endif
