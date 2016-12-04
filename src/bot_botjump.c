// Converted from .qc on 05/02/2016

#include "g_local.h"
#include "fb_globals.h"

#define CHANCE_ROCKET_JUMP 0.2       // FIXME: personalise in fb.skill

// Returns true if the bot is travelling in the 'right' direction (with 90 degrees of target)
static qbool right_direction(gedict_t* self)
{
	vec3_t test_direction,
	       direction_to_test_marker;
	float current_direction,
		  desired_direction;
	float min_one,
	      min_two;

	// current direction
	normalize(self->fb.oldvelocity, test_direction);
	current_direction = vectoyaw(test_direction);
	
	// desired direction
	VectorSubtract(self->fb.linked_marker->s.v.origin, self->s.v.origin, direction_to_test_marker);
	normalize(direction_to_test_marker, test_direction);
	desired_direction = vectoyaw(test_direction);

	min_one = fabs(desired_direction - current_direction);

	// Reduce angle size and try again, incase one was 20 and the other was 340.. 
	if (desired_direction >= 180) {
		desired_direction -= 360;
	}
	if (current_direction >= 180) {
		current_direction -= 360;
	}
	min_two = fabs(desired_direction - current_direction);

	return (qbool) (min(min_one, min_two) <= 90);
}

// Returns true if space above bot
static float BotCheckSpaceAbove(gedict_t* self)
{
	traceline(self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2], self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] + 140, true, self);
	return (g_globalvars.trace_fraction == 1);
}

// Returns true if ground directly in front of bot
static float checkground(gedict_t* self)
{
	trap_makevectors(self->s.v.v_angle);
	g_globalvars.v_forward[2] = 0;
	VectorNormalize(g_globalvars.v_forward);
	VectorScale(g_globalvars.v_forward, 10, g_globalvars.v_forward);
	traceline(self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2], self->s.v.origin[0] + g_globalvars.v_forward[0], self->s.v.origin[1] + g_globalvars.v_forward[1], self->s.v.origin[2] + g_globalvars.v_forward[2] -40, true, self);
	return (g_globalvars.trace_fraction != 1);
}

// Performs lava jump (RJ when in lava)
static void lava_jump(gedict_t* self) {

	return;
	/*
	gedict_t *e = NULL,
	         *pt = self;
	vec3_t straight_up = { 0, 0, 1 };
	vec3_t straight_down = { 0, 0, -1 };
	float best_distance = 1001,
	      best_yaw = 0;

	// Find closest marker... don't we have a function for this already?
	// Note: this was bugged in .qc code, referencing entity 't'
	for (e = world; e = trap_findradius(e, self->s.v.origin, 1000); ) {
		if (streq(e->s.v.classname, "marker")) {
			if (VectorDistance(self->s.v.origin, e->s.v.origin) < best_distance) {
				best_distance = VectorDistance(self->s.v.origin, e->s.v.origin);
				pt = e;
			}
		}
	}

	best_yaw = vectoyaw(pt->s.v.origin);
	if (self->s.v.waterlevel == 3) {
		self->fb.arrow = BACK;
		//VelocityForArrow(self);
		NewVelocityForArrow (self, straight_up, "LavaJump1");
	}
	if (self->s.v.waterlevel == 2) {
		if (self->fb.arrow == BACK) {
			self->fb.arrow = BACK;
			//VelocityForArrow(self);
			NewVelocityForArrow (self, straight_up, "LavaJump2");
			self->fb.rocketjumping = true;
			self->fb.desired_weapon_impulse = 7;
			self->fb.firing = true;
			self->fb.up_finished = g_globalvars.time + 0.1;
		}
		else if (g_globalvars.time > self->fb.up_finished) {
			self->fb.swim_arrow = DOWN;
			//VelocityForArrow(self);
			NewVelocityForArrow (self, straight_down, "LavaJump3");
		}
	}*/
}

void BotCanRocketJump (gedict_t* self) {
	qbool has_quad     = self->super_damage_finished > g_globalvars.time;
	qbool tp_damage    = teamplay != 1 && teamplay != 5;
	float health_after = TotalStrengthAfterDamage (self->s.v.health, self->s.v.armorvalue, self->s.v.armortype, tp_damage ? 55 * (has_quad ? 4 : 1) : 0);
	qbool has_rl       = (qbool) (((int)self->s.v.items & IT_ROCKET_LAUNCHER) && (self->s.v.ammo_rockets >= 1));
	qbool has_pent     = (qbool) ((int)self->s.v.items & IT_INVULNERABILITY);
	qbool will_pickup  = self->fb.linked_marker && !WaitingToRespawn (self->fb.linked_marker);
	qbool onground     = (int)self->s.v.flags & FL_ONGROUND;

	if (self->fb.debug_path) {
		self->fb.canRocketJump = self->fb.debug_path_rj;
	}
	else if (!has_rl || self->fb.be_quiet || !onground) {
		self->fb.canRocketJump = false;
	}
	else if (has_pent) {
		// If invincible, can always rocket jump
		self->fb.canRocketJump = true;
	}
	else {
		// work out how much health we'll have after
		if (health_after >= 0 && will_pickup && self->fb.linked_marker->tp_flags & (it_mh | it_health)) {
			health_after += self->fb.linked_marker->healamount;
		}
		// FIXME: this threshold should be flexible depending on fuzzy logic
		// - depending on enemy status (maybe higher or lower depending on strength of enemy)
		// - might be higher or lower depending on running away from enemy
		// - might be lower if we need to be enemy to item (quad for instance)
		self->fb.canRocketJump = health_after > 110;
	}
}

// FIXME: If teammate is strong (RA etc), fire anyway?  Also if boomsticker and we are strong
// FIXME: Use find_radius?  Currently ignores teammates behind them.
static qbool CouldHurtNearbyTeammate(gedict_t* me) {
	gedict_t* p;

	// if the bot can't kill them, then don't worry about it
	if (teamplay != 2) {
		return false;
	}

	p = IdentifyMostVisibleTeammate(me);
	return VectorDistance(p->s.v.origin, me->s.v.origin) < 140;
}

// Checks that (x=>z, y=>z) direction is sufficiently similar
static qbool DirectionCheck (vec3_t x, vec3_t y, vec3_t z, float threshold)
{
	vec3_t xz, yz;

	VectorSubtract (z, x, xz);
	VectorSubtract (z, y, yz);

	VectorNormalize (xz);
	VectorNormalize (yz);

	return DotProduct (xz, yz) >= threshold;
}

// Performs rocket jump
// FIXME: Very basic rocket jumps, needs a lot more work
// Direction of rocket jump (currently just straight up) - set pitch, checkground() looks directly ahead
// 
void BotPerformRocketJump(gedict_t* self) {
	if (!(self->fb.touch_marker && self->fb.linked_marker)) {
		return;
	}

	if (self->fb.rocketJumping) {
		// In middle of rocket jumping
		--self->fb.rocketJumpFrameDelay;
	}
	else if (self->fb.canRocketJump && ((int)self->s.v.flags & FL_ONGROUND)) {
		qbool path_is_rj   = self->fb.path_state & ROCKET_JUMP;
		qbool ok_vel       = VectorLength (self->s.v.velocity) > sv_maxspeed * 0.9;
		qbool ok_to_rj     = match_in_progress == 2 || (self->fb.debug_path && self->fb.debug_path_rj);
		float touch_dist   = VectorDistance (self->s.v.origin, self->fb.touch_marker->s.v.origin);
		qbool ok_distance  = touch_dist >= 10 && touch_dist <= 100; // FIXME: TODO
		qbool ok_direction = DirectionCheck (self->fb.touch_marker->s.v.origin, self->s.v.origin, self->fb.linked_marker->s.v.origin, 0.8f); // FIXME: TODO
		qbool ok_to_fire   = !self->s.v.button0;
		// FIXME: TODO: Also used to check they wouldn't kill teammate, but think that should be
		//        fuzzy logic
		qbool can_start_rj = ok_to_rj && path_is_rj && ok_distance && ok_direction && ok_to_fire && ok_vel;

		if (can_start_rj) {
			// Perform the jump
			SetJumpFlag (self, true, "RocketJump");
			self->fb.rocketJumping = true;
			self->fb.rocketJumpFrameDelay = self->fb.rocketJumpPathFrameDelay;
			self->fb.path_state |= DELIBERATE_AIR_WAIT_GROUND;
		}
		else if (self->fb.debug_path && ok_to_rj) {
			G_bprint (PRINT_HIGH, "path_is_rj: %s, distance %s, dir %s, fire %s\n", path_is_rj ? "yes" : "no", ok_distance ? "yes" : "no", ok_direction ? "yes" : "no", ok_to_fire ? "yes" : "no");
		}
	}

	// Is it time to fire
	if (self->fb.rocketJumping) {
		if (self->fb.rocketJumpFrameDelay <= 0) {
			if (self->fb.path_state & ROCKET_JUMP) {
				if (self->fb.rocketJumpAngles[PITCH] > 0) {
					self->fb.desired_angle[PITCH] = self->fb.rocketJumpAngles[PITCH];
				}
				else {
					self->fb.desired_angle[PITCH] = 78.25;
				}
				if (self->fb.rocketJumpAngles[YAW] > 0) {
					self->fb.desired_angle[YAW] = self->fb.rocketJumpAngles[YAW];
				}
			}
			self->fb.desired_weapon_impulse = 7;
			self->fb.firing = true;

			// Finish rocket jumping
			self->fb.rocketJumping = false;
			self->fb.path_state &= ROCKET_JUMP;
		}
		else {
			// Make sure we don't start firing
			self->fb.firing = false;
		}
	}

	/*
	// FIXME: completely wrong now
	if (match_in_progress == 2 && self->fb.rocketJumping) {

		if (self->s.v.waterlevel > 1) {
			vec3_t point = { self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] - 24 };
			if (trap_pointcontents (point[0], point[1], point[2]) == CONTENT_LAVA) {
				if (BotCheckSpaceAbove (self)) {
					lava_jump (self);
					return;
				}
			}
		}
		if (has_quad && !has_pent) {
			return;
		}
		if (!path_is_rj || self->fb.firing || self->fb.jumping || CouldHurtNearbyTeammate (self)) {
			return;
		}
		if (!onground || self->attack_finished > g_globalvars.time) {
			return;
		}

		// If too far away from the marker, ignore
		if (VectorDistance (self->s.v.origin, self->fb.touch_marker->s.v.origin) > 100) {
			return;
		}
		if (!BotCheckSpaceAbove (self) || !checkground (self) || !right_direction (self)) {
			return;
		}

		self->fb.desired_angle[0] = 78.75;
		self->fb.rocketjumping = true;
		self->fb.desired_weapon_impulse = 7;
		self->fb.firing = true;
		SetJumpFlag (self, true, "RocketJump");
	}*/
}

static qbool PlayerFiringLG (gedict_t* player)
{
	return player && player->s.v.button0 && ((int)player->s.v.weapon & IT_LIGHTNING) && player->s.v.ammo_cells > 0;
}

void CheckCombatJump(gedict_t* self)
{
	qbool inWater = self->s.v.waterlevel && self->fb.allowedMakeNoise;
	qbool onGround = ((int)self->s.v.flags & FL_ONGROUND);
	qbool lgSelected = self->fb.desired_weapon_impulse == 8 && self->fb.firing;
	qbool lookingAtEnemy = self->fb.look_object && NUM_FOR_EDICT(self->fb.look_object) == self->s.v.enemy;
	qbool lookObjectFiringLG = PlayerFiringLG (self->fb.look_object);

	// Never combat jump when debugging route execution
	if (self->fb.debug_path) {
		return;
	}

	// Or during prewar (noisy)
	if (match_in_progress < 2) {
		return;
	}

	// If jumping makes sense...
	if (!onGround || inWater) {
		return;
	}

	// Never jump in midair
	if (cvar ("k_midair")) {
		return;
	}

	// Rocket jumping decisions made elsewhere
	if (self->fb.rocketJumping) {
		return;
	}

	// If path is expecting an edge then no combat jump...
	if (self->fb.path_state & (JUMP_LEDGE | BOTPATH_CURLJUMP_HINT)) {
		return;
	}

	// If surprised or player firing LG, don't jump
	if (!lookingAtEnemy || lookObjectFiringLG) {
		return;
	}

	// Now just down to bot characteristics
	SetJumpFlag (self, (g_random () < self->fb.skill.combat_jump_chance), "CombatJump");
}

