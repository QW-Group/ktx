
// bot_aim.c
#include "g_local.h"
#include "fb_globals.h"

// FIXME: fb.skill
#define ATTACK_RESPAWN_TIME 3

#define MAX_YAW_ERROR 8
#define MAX_PITCH_ERROR 5

qbool DM6FireAtDoor (gedict_t* self, vec3_t rel_pos);

static void BotSetDesiredAngles (gedict_t* self, vec3_t rel_pos)
{
	vectoangles(rel_pos, self->fb.desired_angle);
	if (self->fb.desired_angle[0] > 180) {
		self->fb.desired_angle[0] = 360 - self->fb.desired_angle[0];
	}
	else {
		self->fb.desired_angle[0] = 0 - self->fb.desired_angle[0];
	}

	if (self->fb.state & HURT_SELF) {
		self->fb.desired_angle[0] = 180;
	}
}

static void BotStopFiring(gedict_t* bot) {
	qbool continuous =
		bot->fb.desired_weapon_impulse == 4 ||
		bot->fb.desired_weapon_impulse == 5 ||
		bot->fb.desired_weapon_impulse == 8;
	qbool correct_weapon = BotUsingCorrectWeapon (bot);
	qbool enemy_alive = bot->s.v.enemy && ISLIVE (&g_edicts[bot->s.v.enemy]);

	bot->fb.firing &= (continuous && correct_weapon && enemy_alive) || bot->fb.rocketjumping;
}

// FIXME: Magic numbers (400 = 0.5 * sv_gravity)
static qbool PredictSpot(gedict_t* self, gedict_t* enemy_, vec3_t testplace, float rel_time, float fallheight) {
	VectorCopy(testplace, dropper->s.v.origin);
	dropper->s.v.flags = FL_ONGROUND_PARTIALGROUND;

	if (walkmove(dropper, 0, 0)) {
		if (! droptofloor(dropper)) {
			testplace[2] = testplace[2] - 400 * (rel_time * rel_time) - 38;
			return false;
		}

		if (dropper->s.v.origin[2] < fallheight) {
			testplace[2] = testplace[2] - 400 * (rel_time * rel_time) - 38;
			return false;
		}
		return true;
	}

	VectorCopy(enemy_->s.v.origin, testplace);
	return false;
}

static void PredictEnemyLocationInFuture(gedict_t* enemy, float rel_time) {
	vec3_t testplace;
	float fallheight = enemy->s.v.origin[2] - 56 + enemy->s.v.velocity[2] * rel_time;

	enemy->fb.oldsolid = enemy->s.v.solid;
	enemy->s.v.solid = SOLID_NOT;
	VectorMA(enemy->s.v.origin, rel_time, enemy->s.v.velocity, testplace);
	testplace[2] += 36;

	if (PredictSpot(self, enemy, testplace, rel_time, fallheight)) {
		VectorCopy(dropper->s.v.origin, self->fb.predict_origin);
	}
	else {
		vec3_t dir_forward;

		VectorSubtract(self->fb.predict_origin, enemy->s.v.origin, dir_forward);
		dir_forward[2] = 0;
		if ((vlen(dir_forward) > half_sv_maxspeed) || (DotProduct(dir_forward, enemy->s.v.velocity) <= 0)) {
			VectorCopy(testplace, self->fb.predict_origin);
		}
	}
	enemy->s.v.solid = enemy->fb.oldsolid;
}

// This is when firing at buttons/doors etc
static void BotsFireAtWorldLogic(gedict_t* self, vec3_t rel_pos, float* rel_dist) {
	VectorAdd(self->fb.look_object->s.v.absmin, self->fb.look_object->s.v.view_ofs, rel_pos);
	VectorSubtract(rel_pos, self->s.v.origin, rel_pos);
	*rel_dist = vlen(rel_pos);

	if (DM6FireAtDoor (self, rel_pos))
		return;

	if (*rel_dist < 160) {
		vec3_t rel_pos2 = { rel_pos[0], rel_pos[1], 0 };
		VectorNormalize(rel_pos2);
		VectorScale(rel_pos2, 160, rel_pos2);
		rel_pos[0] = rel_pos2[0];
		rel_pos[1] = rel_pos2[1];
		*rel_dist = 160;
	}
}

static qbool IsHitscanWeapon (int impulse)
{
	return impulse == 1 || impulse == 2 || impulse == 3 || impulse == 8;
}

static qbool IsVelocityWeapon (int impulse)
{
	return impulse == 4 || impulse == 5 || impulse == 6 || impulse == 7;
}

static qbool IsNailgun (int impulse)
{
	return impulse == 4 || impulse == 5;
}

// When firing at another player
static void BotsAimAtPlayerLogic(gedict_t* self, vec3_t rel_pos, float* rel_dist) {
	float rel_time = 0;

	VectorSubtract(self->fb.look_object->s.v.origin, self->s.v.origin, rel_pos);
	*rel_dist = vlen(rel_pos);

	if (IsVelocityWeapon(self->fb.desired_weapon_impulse)) {
		rel_time = *rel_dist / 1000;
		if (IsNailgun(self->fb.desired_weapon_impulse) && (self->ctf_flag & CTF_RUNE_HST)) {
			rel_time *= 0.5;
		}
		else if (self->fb.desired_weapon_impulse == 6) {
			rel_time = *rel_dist / 600;
		}
		rel_time = min(rel_time, 0.5);

		if (self->s.v.enemy) {
			PredictEnemyLocationInFuture(&g_edicts[self->s.v.enemy], rel_time);

			if (self->fb.look_object == &g_edicts[self->s.v.enemy]) {
				VectorSubtract(self->fb.predict_origin, self->s.v.origin, rel_pos);
			}
		}
	}
}

typedef void (*bot_aim_model_t)(gedict_t* self, vec3_t rel_pos, float rel_distance);

#define YAW_SNAP_THRESHOLD     4
#define PITCH_SNAP_THRESHOLD   6

static qbool HorizontalVelocityCheck (vec3_t velocity, float threshold)
{
	float value = velocity[0] * velocity[0] + velocity[1] * velocity[1];

	return value > threshold * threshold;
}

static void CalculateVolatility (gedict_t* self)
{
	float volatility   = self->fb.skill.current_volatility;
	gedict_t* opponent = self->fb.look_object;
	int vol_flags = 0;

	if (opponent != self->fb.prev_look_object) {
		// Treat as if they hadn't seen player before
		volatility = self->fb.skill.initial_volatility;
		self->fb.min_fire_time = g_globalvars.time + self->fb.skill.awareness_delay;
		self->fb.last_pitch_sign = self->fb.last_yaw_sign = 0;

		vol_flags = 1;
	}
	else {
		vec3_t bot_direction = { PASSVEC3 (self->s.v.velocity) };
		vec3_t enemy_direction = { PASSVEC3 (opponent->s.v.velocity) };
		float same_direction = 0.0f;

		volatility *= self->fb.skill.reduce_volatility;

		// Ownspeed penalty
		if (HorizontalVelocityCheck(self->s.v.velocity, self->fb.skill.ownspeed_volatility_threshold)) {
			volatility += self->fb.skill.ownspeed_volatility;
			vol_flags = 2;
		}

		// Speed penalty
		if (HorizontalVelocityCheck(opponent->s.v.velocity, self->fb.skill.enemyspeed_volatility_threshold)) {
			volatility += self->fb.skill.enemyspeed_volatility;
			vol_flags |= 4;
		}

		VectorNormalize (bot_direction);
		VectorNormalize (enemy_direction);
		same_direction = DotProduct (bot_direction, enemy_direction);

		// Direction penalty ... if we're going in same direction, no penalty
		volatility += (1 - same_direction) * (self->fb.skill.enemydirection_volatility / 2);
		vol_flags |= 8;

		volatility = bound(self->fb.skill.min_volatility, volatility * self->fb.skill.reduce_volatility, self->fb.skill.max_volatility);
	}

	self->fb.skill.current_volatility = volatility;
}

// Called after desired angles have been set to aim at the player
static void BotsModifyAimAtPlayerLogic (gedict_t* self, vec3_t rel_pos, float rel_dist)
{
	fb_botaim_t* pitch = &self->fb.skill.aim_params[PITCH];
	fb_botaim_t* yaw   = &self->fb.skill.aim_params[YAW];

	float raw_yaw_diff = self->fb.desired_angle[PITCH] - self->s.v.angles[PITCH];
	float raw_pitch_diff = self->fb.desired_angle[YAW] - self->s.v.angles[YAW];

	float pitch_diff, yaw_diff;
	float pitch_rnd, yaw_rnd;

	CalculateVolatility (self);

	pitch_diff = bound (pitch->minimum, fabs (raw_pitch_diff) * pitch->scale, pitch->maximum);
	yaw_diff = bound(yaw->minimum, fabs(raw_yaw_diff) * yaw->scale, yaw->maximum);

	pitch_rnd = dist_random (-pitch_diff, pitch_diff, pitch->multiplier * self->fb.skill.current_volatility);
	yaw_rnd = dist_random (-yaw_diff, yaw_diff, yaw->multiplier * self->fb.skill.current_volatility);

	// If making a small adjustment when tracking, keep the aim to the same side
	if (self->fb.last_yaw_sign && g_random() < 0.9) {
		yaw_rnd = self->fb.last_yaw_sign * abs (yaw_rnd);
	}
	else {
		self->fb.last_yaw_sign = yaw_rnd < 0 ? -1 : 1;
	}
	if (self->fb.last_pitch_sign && g_random() < 0.9) {
		pitch_rnd = self->fb.last_pitch_sign * abs (pitch_rnd);
	}
	else {
		self->fb.last_pitch_sign = pitch_rnd < 0 ? -1 : 1;
	}

	//G_bprint (PRINT_HIGH, "Volatility %3.1f, %s%s%s%s yaw_diff %f\n", volatility, vol_flags & 1 ? "max" : "", vol_flags & 2 ? "o" : "", vol_flags & 4 ? "s" : "", vol_flags & 8 ? "d" : "", yaw_diff);

	// Randomise the aim a bit based on raw skill level
	self->fb.desired_angle[PITCH] = bound (-89.9, self->fb.desired_angle[PITCH] + pitch_rnd, 89.9);
	self->fb.desired_angle[YAW] += yaw_rnd;
}

// Aim lower over longer distances?  (FIXME: we already allow for gravity in predicting where to fire - should this test for the enemy being on the ground?)
static void BotsAimAtFloor (gedict_t* self, vec3_t rel_pos, float rel_dist)
{
	if (self->fb.desired_weapon_impulse == 7 && rel_dist > 96) {
		traceline(self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] + 16, self->s.v.origin[0] + rel_pos[0], self->s.v.origin[1] + rel_pos[1], self->s.v.origin[2] + rel_pos[2] - 22, true, self);
		if (g_globalvars.trace_fraction == 1) {
			rel_pos[2] = rel_pos[2] - 38;
		}
	}
}

static void FireAtSpawnPoint (gedict_t* self)
{
	gedict_t* resp;
	for (resp = world; resp = trap_findradius(resp, self->s.v.origin, 1000); ) {
		if (streq(resp->s.v.classname, "info_player_deathmatch")) {
			vec3_t test;
			VectorCopy(self->s.v.origin, test);
			test[2] += 16;
			if (VectorDistance(resp->s.v.origin, test) > 160) {
				if (VisibleEntity(resp)) {
					float ang1, ang2;
					vec3_t diff;

					self->fb.desired_weapon_impulse = 7;
					self->fb.look_object = resp;
					VectorCopy(resp->s.v.origin, self->fb.predict_origin);
					self->fb.predict_origin[2] += 16;
					self->fb.old_linked_marker = NULL;

					VectorSubtract(resp->s.v.origin, self->s.v.origin, diff);
					ang2 = vectoyaw(diff);
					ang1 = anglemod(self->s.v.angles[1] - ang2);
					self->fb.firing |= (ang1 < 20 || ang1 > 340);
					return;
				}
			}
		}
	}
}

// When duelling, try and spawn frag.
static void AttackRespawns(gedict_t* self) {
	gedict_t* enemy_ = &g_edicts[self->s.v.enemy];

	if (isRA() || isHoonyMode() || !isDuel()) {
		return;
	}

	if (enemy_ != world && enemy_->s.v.health < 1) {
		if (enemy_->fb.last_death + ATTACK_RESPAWN_TIME >= g_globalvars.time) {
			if (self->fb.skill.attack_respawns) {
				if ((int)self->s.v.items & IT_ROCKET_LAUNCHER) {
					if (self->s.v.ammo_rockets > 3) {
						if (!self->fb.rocketjumping) {
							if (g_random() > 0.15) {
								FireAtSpawnPoint (self);
							}
						}
					}
				}
			}
		}
	}
}

void BotsFireLogic(void) {
	vec3_t rel_pos;

	BotStopFiring(self);

	AttackRespawns(self);

	// a_attackfix()
	if (!self->fb.rocketjumping && self->s.v.enemy == 0 && !(self->fb.state & SHOT_FOR_LUCK)) {
		self->fb.firing = false;
	}

	if (self->fb.look_object) {
		float rel_dist = 0;

		if (self->fb.look_object->ct == ctPlayer) {
			BotsAimAtPlayerLogic (self, rel_pos, &rel_dist);
		}
		else {
			BotsFireAtWorldLogic (self, rel_pos, &rel_dist);
		}

		BotsAimAtFloor (self, rel_pos, rel_dist);
		BotSetDesiredAngles (self, rel_pos);
		if (self->fb.look_object->ct == ctPlayer) {
			BotsModifyAimAtPlayerLogic (self, rel_pos, rel_dist);
		}

		if (!self->fb.rocketjumping) {
			SetFireButton(self, rel_pos, rel_dist);
		}
	}
}
