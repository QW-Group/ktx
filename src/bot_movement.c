
#include "g_local.h"
#include "fb_globals.h"

#define ARROW_TIME_INCREASE       0.15  // Seconds to advance after NewVelocityForArrow
#define MIN_DEAD_TIME 0.2f
#define MAX_DEAD_TIME 1.0f

void SetLinkedMarker (gedict_t* player, gedict_t* marker, char* explanation)
{
	gedict_t* touch = player->fb.touch_marker;

	//if (player->isBot && marker != player->fb.linked_marker)
	//	G_sprint (player, 2, "linked to %3d/%s, touching %3d/%s g %s (%s)\n", marker ? marker->fb.index : -1, marker ? marker->s.v.classname : "(null)", touch ? touch->fb.index : -1, touch ? touch->s.v.classname : "(null)", g_edicts[player->s.v.goalentity].s.v.classname, explanation ? explanation : "");
	if (player->isBot && player->fb.debug_path)
		G_bprint (2, "%3.2f: linked to %3d/%s, touching %3d/%s g %s (%s)\n", g_globalvars.time, marker ? marker->fb.index : -1, marker ? marker->s.v.classname : "(null)", touch ? touch->fb.index : -1, touch ? touch->s.v.classname : "(null)", g_edicts[player->s.v.goalentity].s.v.classname, explanation ? explanation : "");

	player->fb.linked_marker = marker;
}

void SetDirectionMove (gedict_t* self, vec3_t dir_move, const char* explanation)
{
	normalize (dir_move, self->fb.dir_move_);

	//if (self->isBot)
	//	G_bprint (2, "%3.2f: SetDirection(%2.2f %2.2f %2.2f): %s\n", g_globalvars.time, PASSVEC3(self->fb.dir_move_), explanation);
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

static void ApplyPhysics (gedict_t* self)
{
	float drop = 0;
	vec3_t expected_velocity;
	float vel_length = 0;
	float hor_speed_squared;
	float movement_skill = bound (0, self->fb.skill.movement, 1.0);
	qbool onGround = (int)self->s.v.flags & FL_ONGROUND;

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
		float used_numerator = min_numerator + movement_skill * (max_numerator - min_numerator);
		float max_incr = used_numerator * used_numerator;

		// Gravity kicks in
		if (!onGround)
			expected_velocity[2] -= self->gravity * cvar ("sv_gravity") * g_globalvars.frametime;
		else
			expected_velocity[2] = 0;

		// Ground & air acceleration is the same
		hor_speed_squared = (expected_velocity[0] * expected_velocity[0] + expected_velocity[1] * expected_velocity[1]);

		if (onGround && hor_speed_squared < 300) {
			return;
		}

		if (hor_speed_squared >= max_incr) {
			vec3_t current_direction;
			vec3_t original_direction;
			vec3_t perpendicular;

			vec3_t up_vector = { 0, 0, 1 };
			float rotation   = acos (max_incr / hor_speed_squared) * 180 / M_PI;

			// Find out if rotation should be positive or negative
			normalize (self->fb.dir_move_, original_direction);
			normalize (expected_velocity, current_direction);
			CrossProduct (current_direction, original_direction, perpendicular);

			// Negative rotation => rotate 'right'
			if (perpendicular[2] < 0)
				rotation = -rotation;
			RotatePointAroundVector (self->fb.dir_move_, up_vector, current_direction, rotation);
		}
	}
}

float AverageTraceAngle (gedict_t* self, qbool debug)
{
	vec3_t back_left, projection, incr;
	int angles[] = { 45, 30, 15, 0, -15, -30, -45 };
	int i;
	float best_angle = 0;
	float best_angle_frac = 0;
	float total_angle = 0;
	float avg_angle;

	trap_makevectors (self->s.v.angles);
	VectorMA (self->s.v.origin, -VEC_HULL_MIN[0], g_globalvars.v_forward, back_left);
	VectorMA (back_left, VEC_HULL_MIN[1], g_globalvars.v_right, back_left);

	VectorScale (g_globalvars.v_right, (VEC_HULL_MAX[0] - VEC_HULL_MIN[0]) / (sizeof(angles) / sizeof(angles[0]) - 1), incr);

	if (debug) {
		G_sprint (self, 2, "Current origin: %d %d %d\n", PASSINTVEC3 (self->s.v.origin));
		G_sprint (self, 2, "Current angles: %d %d\n", PASSINTVEC3 (self->s.v.angles));
	}

	for (i = 0; i < sizeof (angles) / sizeof (angles[0]); ++i) {
		int angle = angles[i];

		RotatePointAroundVector (projection, g_globalvars.v_up, g_globalvars.v_forward, angle);
		VectorMA (back_left, 320, projection, projection);
		traceline (PASSVEC3 (back_left), PASSVEC3 (projection), false, self);

		total_angle += angle * g_globalvars.trace_fraction;

		if (debug) {
			G_sprint (self, 2, "Angle: %d => [%d %d %d] [%d %d %d] = %f\n", angle, PASSINTVEC3 (back_left), PASSINTVEC3 (projection), g_globalvars.trace_fraction);
		}

		if (i == 0 || g_globalvars.trace_fraction > best_angle_frac) {
			best_angle = angle;
			best_angle_frac = g_globalvars.trace_fraction;
		}

		VectorAdd (back_left, incr, back_left);
	}

	avg_angle = total_angle / (sizeof (angles) / sizeof (angles[0]));

	if (debug) {
		G_sprint (self, 2, "Best angle: %d\n", best_angle);
		G_sprint (self, 2, "Total angle: %f\n", avg_angle);
	}
	return avg_angle;
}

static void BestJumpingDirection (gedict_t* self)
{
	float avg_angle = AverageTraceAngle (self, false);

	if (avg_angle < 0)
		avg_angle = min (avg_angle, -15);
	else
		avg_angle = max (avg_angle, 15);

	RotatePointAroundVector (self->fb.dir_move_, g_globalvars.v_up, g_globalvars.v_forward, avg_angle);

	/*
	vec3_t forward_left, forward_right;
	vec3_t back_left = 
	int angle;

	for (angle = 45; angle >= -45; angle -= 15) {
	}
	RotatePointAroundVector (forward_left, g_globalvars.v_up, g_globalvars.v_forward, 45);
	RotatePointAroundVector (forward_right, g_globalvars.v_up, g_globalvars.v_forward, -45);
	VectorMA (self->s.v.origin, 320, forward_left, forward_left);
	VectorMA (self->s.v.origin, 320, forward_right, forward_right);
	traceline (PASSVEC3 (self->s.v.origin), PASSVEC3 (forward_left), false, self);
	traceline (PASSVEC3 (self->s.v.origin), PASSVEC3 (forward_right), false, self);
	*/
}

void BotSetCommand (gedict_t* self)
{
	float msec = g_globalvars.frametime * 1000; //min ((g_globalvars.time - self->fb.last_cmd_sent) * 1000, 255);
	int weapon_script_impulse = 0;
	int impulse = 0;
	qbool jumping;
	qbool firing;
	vec3_t direction;

	BotPerformRocketJump (self);

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

		direction[0] = DotProduct (g_globalvars.v_forward, self->fb.dir_move_) * 800;
		direction[1] = DotProduct (g_globalvars.v_right, self->fb.dir_move_) * 800;
		direction[2] = DotProduct (g_globalvars.v_up, self->fb.dir_move_) * 800;
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
		msec,
		PASSVEC3(self->fb.desired_angle),
		PASSVEC3(direction),
		(firing ? 1 : 0) | (jumping ? 2 : 0),
		impulse
	);

	self->fb.next_impulse = 0;
	self->fb.botchose = false;
	self->fb.last_cmd_sent = g_globalvars.time;

	VectorClear (self->fb.obstruction_normal);
	if (self->s.v.button0 && !firing) {
		// Stopped firing, randomise next time
		self->fb.last_rndaim_time = 0;
	}
	self->fb.prev_look_object = self->fb.look_object;
	VectorCopy (self->s.v.velocity, self->fb.prev_velocity);
}
