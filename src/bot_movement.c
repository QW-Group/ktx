
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

	// FIXME: This should be when they touch the marker, not when linking to it
	if (player->fb.fixed_goal == marker) {
		G_bprint (2, "at goal, path complete.\n");
		player->fb.fixed_goal = NULL;
		player->fb.debug_path = false;
	}

	player->fb.linked_marker = marker;
}

void SetDirectionMove (gedict_t* self, vec3_t dir_move, const char* explanation)
{
	normalize (dir_move, self->fb.dir_move_);

	if (self->isBot && self->fb.debug_path)
		G_bprint (2, "%3.2f: SetDirection(%2.2f %2.2f %2.2f): %s\n", g_globalvars.time, PASSVEC3(self->fb.dir_move_), explanation);
}

void NewVelocityForArrow(gedict_t* self, vec3_t dir_move, const char* explanation) {
	SetDirectionMove (self, dir_move, explanation);
	self->fb.arrow_time = g_globalvars.time + ARROW_TIME_INCREASE;
}

static qbool BotRequestRespawn(gedict_t* self) {
	float time_dead = min (g_globalvars.time - self->fb.last_death, MAX_DEAD_TIME);

	return self->s.v.deadflag == DEAD_RESPAWNABLE && time_dead > MIN_DEAD_TIME && (g_random () < (time_dead / MAX_DEAD_TIME));
}

void BotSetCommand(gedict_t* self) {
	float msec = g_globalvars.frametime * 1000; //min ((g_globalvars.time - self->fb.last_cmd_sent) * 1000, 255);
	int weapon_script_impulse = 0;
	int impulse = 0;
	qbool jumping;
	qbool firing;
	vec3_t direction;

	BotPerformRocketJump(self);

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

	if (self->fb.firing && BotUsingCorrectWeapon(self)) {
		impulse = 0; // we already have the requested weapon
	}

	jumping = self->fb.jumping || self->fb.waterjumping;
	firing = self->fb.firing;

	self->fb.waterjumping = false;

	if (self->s.v.waterlevel >= 2) {
		// Swimming...
	}
	else if (!((int)self->s.v.flags & FL_ONGROUND)) {
		// Perform air-control instead - rotate current velocity towards desired direction
		float speed_squared = (self->s.v.velocity[0] * self->s.v.velocity[0] + self->s.v.velocity[1] * self->s.v.velocity[1]);

		if (speed_squared > 900) {
			vec3_t cross_product;
			float rotation = acos( 900 / speed_squared ) * 180 / M_PI;
			vec3_t current_direction;

			normalize (self->s.v.velocity, current_direction);

			CrossProduct (current_direction, self->fb.dir_move_, cross_product);

			if (self->isBot && self->fb.debug_path) {
				G_bprint (PRINT_HIGH, "%3.2f: AC: XProduct %4.1f %4.1f %4.1f, Rot %3.1f\n", g_globalvars.time, PASSVEC3(cross_product), rotation);
			}

			RotatePointAroundVector (self->fb.dir_move_, cross_product, current_direction, rotation);
		}
	}
	else {
		// On ground acceleration
	}

	if (self->fb.dbg_countdown > 0) {
		jumping = firing = false;
		VectorClear (direction);
		--self->fb.dbg_countdown;
	}
	else {
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
	if (!firing) {
		self->fb.last_pitch_sign = self->fb.last_yaw_sign = 0;
	}
	self->fb.prev_look_object = self->fb.look_object;
	if (self->isBot && self->fb.debug_path) {
		vec3_t accel, raw_dir;
		VectorSubtract (self->s.v.velocity, self->fb.prev_velocity, accel);
		VectorScale (self->fb.dir_move_, 320, raw_dir);
		G_bprint (PRINT_HIGH, "%3.2f: V %4d %4d %4d, D %4d %4d %4d, A %4d %4d %4d\n", g_globalvars.time, PASSINTVEC3(self->s.v.velocity), PASSINTVEC3(raw_dir), PASSINTVEC3(accel));
	}
	VectorCopy (self->s.v.velocity, self->fb.prev_velocity);
}
