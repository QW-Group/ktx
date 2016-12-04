// Converted from .qc on 07/02/2016

#include "g_local.h"
#include "fb_globals.h"

#define PERIODIC_MM2_STATUS 4

void PlayerReady ();
void BotBlocked (void);

int weapon_impulse_codes[] = {
	0, IT_AXE, IT_SHOTGUN, IT_SUPER_SHOTGUN, IT_NAILGUN, IT_SUPER_NAILGUN, IT_GRENADE_LAUNCHER, IT_ROCKET_LAUNCHER, IT_LIGHTNING
};

void TeamplayReportVisiblePowerups (gedict_t* player)
{
	unsigned int clientFlag = ClientFlag (player);
	gedict_t* opponent;

	// Assumes that visclients match
	for (opponent = world; opponent = find_plr (opponent); ) {
		qbool diff_team = opponent->k_teamnum != player->k_teamnum;
		qbool powerups = (int)opponent->s.v.items & (IT_QUAD | IT_INVULNERABILITY);
		qbool visible = (opponent->visclients & clientFlag);

		if (diff_team && powerups && visible && opponent->fb.last_mm2_spot < g_globalvars.time) {
			if (VisibleEntity (opponent)) {
				TeamplayMessageByName (player, "enemypwr");
				opponent->fb.last_mm2_spot = g_globalvars.time + 2;
				break;
			}
		}
	}
}

static void ResetEnemy(gedict_t* self) {
	gedict_t* test_enemy = NULL;

	// Find all players who consider the current player their enemy, and clear it
	for (test_enemy = world; test_enemy = find_plr(test_enemy); ) {
		if (test_enemy->s.v.enemy == NUM_FOR_EDICT(self)) {
			test_enemy->s.v.enemy = NUM_FOR_EDICT(world);

			// Clear look object as well
			if (test_enemy->fb.look_object && test_enemy->fb.look_object->ct == ctPlayer) {
				ClearLookObject(test_enemy);
				test_enemy->fb.look_object = NULL;
			}

			// Clear goal entity (if we were hunting them down)
			if (test_enemy->s.v.goalentity == NUM_FOR_EDICT(self)) {
				test_enemy->fb.goal_refresh_time = 0;
			}
		}

		if (test_enemy->fb.prev_look_object == self) {
			test_enemy->fb.prev_look_object = NULL;
		}
	}

	self->s.v.enemy = NUM_FOR_EDICT(world);
}

static void ResetGoalEntity(gedict_t* self)
{
	if (self->s.v.goalentity) {
		gedict_t* ent = &g_edicts[self->s.v.goalentity];
		ent->fb.teamflag &= ~self->fb.teamflag;
		self->s.v.goalentity = NUM_FOR_EDICT(world);
	}
}

void BotPlayerKilledEvent (gedict_t* targ, gedict_t* attacker, gedict_t* inflictor)
{
	if (inflictor && inflictor->ct != ctPlayer && (inflictor->fb.T & UNREACHABLE)) {
		targ->fb.state |= BACKPACK_IS_UNREACHABLE;
	}
}

// Called whenever a player dies
void BotPlayerDeathEvent(gedict_t* self)
{
	ResetGoalEntity(self);
	ResetEnemy(self);

	if (self->isBot && teamplay) {
		qbool dropped_weapon = self->s.v.weapon == IT_ROCKET_LAUNCHER || self->s.v.weapon == IT_LIGHTNING;
		qbool no_teammates = self->tp.teammate_count == 0 || self->tp.enemy_count > self->tp.teammate_count;

		if (dropped_weapon || no_teammates) {
			TeamplayMessageByName (self, "lost");
		}
	}

	self->fb.last_death = g_globalvars.time;
	self->fb.prev_look_object = NULL;
}

// Was: PutClientInServer_apply()
void BotClientEntersEvent(gedict_t* self, gedict_t* spawn_pos)
{
	self->fb.oldwaterlevel = self->fb.oldwatertype = 0;
	self->fb.desired_angle[0] = self->fb.real_pitch = self->s.v.angles[0];
	self->fb.desired_angle[1] = self->fb.real_yaw = self->s.v.angles[1];
	self->fb.state = 0;
	self->fb.min_fire_time = g_globalvars.time + self->fb.skill.awareness_delay;
	self->fb.last_rndaim_time = 0;

	SetMarker(self, spawn_pos);

	self->fb.arrow = 0;
	ClearLookObject(self);
	VectorSet(self->fb.oldvelocity, 0, 0, 0);
	self->fb.jumping = false;
	self->fb.firing = false;
	self->fb.desired_weapon_impulse = 2;
	self->fb.goal_refresh_time = 0;
	self->fb.allowedMakeNoise = true;

	FrogbotSetHealthArmour (self);
	self->fb.weapon_refresh_time = 0;
	self->s.v.blocked = (func_t) BotBlocked;
}

qbool BotUsingCorrectWeapon (gedict_t* self)
{
	return self->fb.desired_weapon_impulse >= 1 &&
	       self->fb.desired_weapon_impulse <= 8 &&
	       self->s.v.weapon == weapon_impulse_codes[self->fb.desired_weapon_impulse];
}

static float goal_client(gedict_t* self) {
	if (g_globalvars.time < self->fb.virtual_enemy->invisible_finished) {
		return 0;     // don't chase enemies with eyes
	}
	else if (g_globalvars.time < self->fb.virtual_enemy->invincible_finished) {
		return 0;     // or with pent
	}
	else if (self->fb.look_object && self->s.v.enemy == NUM_FOR_EDICT(self->fb.look_object)) {
		return ((self->fb.total_damage + 100) * self->fb.firepower - self->fb.virtual_enemy->fb.total_damage * self->fb.virtual_enemy->fb.firepower) * 0.01;
	}
	else if (EnemyDefenceless(self)) {
		return ((self->fb.total_damage + 120) * self->fb.firepower - self->fb.virtual_enemy->fb.total_damage * self->fb.virtual_enemy->fb.firepower) * 0.01;
	}
	else {
		return (self->fb.total_damage * self->fb.firepower - self->fb.virtual_enemy->fb.total_damage * self->fb.virtual_enemy->fb.firepower) * 0.01;
	}
}

static float goal_client6(gedict_t* self) {
	if (! k_matchLess && match_in_progress != 2) {
		return 100;
	}

	if (g_globalvars.time < self->fb.virtual_enemy->invisible_finished || g_globalvars.time < self->fb.virtual_enemy->invincible_finished) {
		return 0;     // or with pent
	}

	return (300 - min(self->fb.virtual_enemy->fb.total_damage, 300));
}

// client.qc
// This is called whenever a client connects (not just bots)
// TODO: any preferences stored against the specific bot to be restored here?
void BotClientConnectedEvent(gedict_t* self) {
	self->fb.desire = (deathmatch <= 3 ? goal_client : goal_client6);
	self->fb.T = UNREACHABLE;
	self->fb.skill.skill_level = g_globalvars.parm3;
	self->fb.skill.lookahead_time = 30;
	self->fb.skill.prediction_error = 0;
	self->fb.ammo_used = FrogbotWeaponFiredEvent;

	if (self->isBot) {
		PlayerReady ();
	}

	// Assign all flags again
	if (match_in_progress == 2) {
		BotsAssignTeamFlags ();
	}
}

// client.qc
void BotOutOfWater(gedict_t* self) {
	if (self->s.v.waterlevel == 2) {
		vec3_t start;
		vec3_t end;

		// Tread water
		self->fb.tread_water_count = self->fb.tread_water_count + 1;
		if (self->fb.tread_water_count > 75) {
			self->fb.old_linked_marker = NULL;
			SetLinkedMarker(self, LocateMarker(self->s.v.origin), "BotOutOfWater");
			self->fb.path_state = 0;
			self->fb.linked_marker_time = g_globalvars.time + 5;
		}

		trap_makevectors(self->s.v.v_angle);
		VectorCopy(self->s.v.origin, start);
		start[2] += 8;
		g_globalvars.v_forward[2] = 0;
		VectorNormalize (g_globalvars.v_forward);
		VectorScale (g_globalvars.v_forward, 24, g_globalvars.v_forward);
		VectorAdd (start, g_globalvars.v_forward, end);
		trap_traceline(PASSVEC3(start), PASSVEC3(end), true, NUM_FOR_EDICT(self));
		if (g_globalvars.trace_fraction < 1) {
			start[2] = self->s.v.origin[2] + self->s.v.maxs[2];
			VectorAdd (start, g_globalvars.v_forward, end);
			VectorScale (g_globalvars.trace_plane_normal, -50, self->s.v.movedir);
			traceline(start[0], start[1], start[2], end[0], end[1], end[2], true, self);
			if (g_globalvars.trace_fraction == 1) {
				// FIXME: Setting vector here has no effect, hangover from PR1
				vec3_t temp_vector;
				VectorCopy (self->fb.dir_move_, temp_vector);
				temp_vector[2] = 225;
				SetDirectionMove (self, temp_vector, "BotOutOfWater");
				SetJumpFlag (self, true, "BotOutOfWater");
				//self->s.v.flags = ((int)self->s.v.flags | FL_WATERJUMP) & ~FL_JUMPRELEASED;
				//self->s.v.teleport_time = g_globalvars.time + 2;
			}
		}
	}
}

static void BotPeriodicMessages (gedict_t* self)
{
	gedict_t* opponent = NULL;

	if (PAST(last_mm2_status)) {
		qbool has_rl = ((int)self->s.v.items & IT_ROCKET_LAUNCHER) && self->s.v.ammo_rockets >= 3;
		qbool has_lg = ((int)self->s.v.items & IT_LIGHTNING) && self->s.v.ammo_cells >= 6;
		qbool is_strong = (has_rl || has_lg) && self->fb.total_damage >= 120;

		if (is_strong && self->tp.enemy_count == 0) {
			TeamplayMessageByName (self, "secure");
		}
		else if (is_strong && self->tp.enemy_count > self->tp.teammate_count) {
			TeamplayMessageByName (self, "help");
		}
		else if (self->tp.enemy_count == 0) {
			TeamplayMessageByName (self, "status");
		}
		else if (self->fb.look_object && NUM_FOR_EDICT (self->fb.look_object) == self->s.v.enemy) {
			TeamplayMessageByName (self, "point");
		}

		self->fb.last_mm2_status = g_globalvars.time + PERIODIC_MM2_STATUS * (g_random() + 0.5);
	}

	// Check for opponents with powerups
	TeamplayReportVisiblePowerups (self);
}

// Called for every player, if bots are enabled
void BotPreThink(gedict_t* self) {
	if (self->isBot) {
		self->fb.firing = self->s.v.button0;
		self->fb.jumping = self->s.v.button2;

		if (self->isBot && match_in_progress == 0 && !self->ready) {
			PlayerReady ();
		}

		if (teamplay && match_in_progress == 2) {
			BotPeriodicMessages (self);
		}
	}

	self->fb.touch_distance = 1000000;
}


