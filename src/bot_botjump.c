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
	self->fb.real_yaw = (360 - best_yaw);
	if (self->s.v.waterlevel == 3) {
		self->fb.real_pitch = 78.75;
		self->fb.arrow = BACK;
		//VelocityForArrow(self);
		NewVelocityForArrow (self, straight_up, "LavaJump1");
	}
	if (self->s.v.waterlevel == 2) {
		if (self->fb.arrow == BACK) {
			self->fb.real_pitch = 78.75;
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
	}
}

qbool able_rj(gedict_t* self) {
	float health_after = min(self->s.v.armorvalue, ceil(self->s.v.armortype * 50));
	qbool has_rj       = (qbool) (((int)self->s.v.items & IT_ROCKET_LAUNCHER) && (self->s.v.ammo_rockets >= 1));
	qbool has_pent     = (qbool) ((int)self->s.v.items & IT_INVULNERABILITY);

	// If invincible, can always rocket jump
	if (has_pent) {
		return true;
	}

	// work out how much health we'll have after - factor in if we're heading towards a respawned healthbox
	health_after = ((teamplay == 1) || (teamplay == 5) ? 100 : self->s.v.health - ceil(55 - health_after));
	if (self->fb.linked_marker && self->fb.linked_marker->healamount > 0 && ! WaitingToRespawn(self->fb.linked_marker)) {
		health_after += self->fb.linked_marker->healamount;
	}

	return (qbool) (health_after > 50 && has_rj && (!self->fb.be_quiet) && g_random() < CHANCE_ROCKET_JUMP);
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

// Performs rocket jump
// FIXME: Very basic rocket jumps, needs a lot more work
// Direction of rocket jump (currently just straight up) - set pitch, checkground() looks directly ahead
// 
void BotPerformRocketJump(gedict_t* self) {
	qbool has_rl = (qbool) (self->s.v.ammo_rockets && ((int)self->s.v.items & IT_ROCKET_LAUNCHER));

	self->fb.rocketjumping = false;
	if (match_in_progress == 2 && has_rl && self->fb.willRocketJumpThisTic) {
		qbool has_quad = (int)self->s.v.items & IT_QUAD;
		qbool has_pent = (int)self->s.v.items & IT_INVULNERABILITY;
		qbool path_is_rj = (int)self->fb.path_state & ROCKET_JUMP;
		qbool onground = (int)self->s.v.flags & FL_ONGROUND;

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

		self->fb.real_pitch = 78.75;
		self->fb.desired_angle[0] = 78.75;
		self->fb.rocketjumping = true;
		self->fb.desired_weapon_impulse = 7;
		self->fb.firing = true;
		SetJumpFlag (self, true, "RocketJump");
	}
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

	// If jumping makes sense...
	if (!onGround || inWater) {
		return;
	}

	// Never jump in midair
	if (cvar ("k_midair")) {
		return;
	}

	// Rocket jumping decisions made elsewhere
	if (self->fb.rocketjumping) {
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

