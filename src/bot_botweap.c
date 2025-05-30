/*
 bot/botweap.qc

 Copyright (C) 1997-1999 Robert 'Frog' Field
 Copyright (C) 1998-2000 Matt 'asdf' McChesney
 Copyright (C) 2000-2001 DMSouL
 Copyright (C) 2003 3d[Power]
 Copyright (C) 2000-2007 ParboiL
 */

// Converted from .qc on 05/02/2016
#ifdef BOT_SUPPORT

#include "g_local.h"

// FIXME: globals, this is just setting
void DM6SelectWeaponToOpenDoor(gedict_t *self);

// FIXME: This is just stopping quad damage rocket shot, always replacing with shotgun
// Can do far better than this
static qbool TP_CouldDamageTeammate(gedict_t *self)
{
	if ((int)self->s.v.items & IT_QUAD)
	{
		if ((teamplay != 1) && (teamplay != 5))
		{
			gedict_t *search_entity = IdentifyMostVisibleTeammate(self);

			if (!search_entity->invincible_time)
			{
				if (VisibleEntity(search_entity))
				{
					if (self->fb.enemy_visible)
					{
						if (VectorDistance(search_entity->s.v.origin,
											g_edicts[self->s.v.enemy].s.v.origin) < 150)
						{
							return (self->s.v.ammo_shells > 0);
						}
					}
				}
			}
		}
	}

	return false;
}

static qbool WaterCombat(gedict_t *self)
{
	gedict_t *enemy_ = &g_edicts[self->s.v.enemy];

	if (self->s.v.waterlevel < 2)
	{
		return true;
	}

	return ((trap_pointcontents(enemy_->s.v.origin[0], enemy_->s.v.origin[1], enemy_->s.v.origin[2])
			== CONTENT_WATER) && (enemy_->s.v.origin[2] < self->s.v.origin[2] - 32));
}

static qbool RocketSafe(void)
{
	float splash_damage = 80 - (0.25 * self->fb.enemy_dist);

	if ((teamplay == 1) || (teamplay == 5) || cvar("k_midair") || (splash_damage <= 0))
	{
		return true;
	}

	if (self->super_damage_finished > g_globalvars.time)
	{
		splash_damage = splash_damage * (deathmatch != 4 ? 4 : 8);
		if (self->ctf_flag & ITEM_RUNE_MASK)
		{
			if (self->ctf_flag & CTF_RUNE_STR)
			{
				splash_damage = splash_damage * (cvar("k_ctf_rune_power_str") / 2) + 1;
			}
			else if (self->ctf_flag & CTF_RUNE_RES)
			{
				splash_damage = splash_damage / (cvar("k_ctf_rune_power_res") / 2) + 1;
			}
		}
	}

	return (self->fb.total_damage > splash_damage);
}

qbool CheckNewWeapon(int desired_weapon)
{
	int weapons[] =
		{
		IT_AXE, IT_SHOTGUN, IT_SUPER_SHOTGUN, IT_NAILGUN, IT_SUPER_NAILGUN, IT_GRENADE_LAUNCHER,
				IT_ROCKET_LAUNCHER, IT_LIGHTNING };

	if ((self->s.v.weapon != desired_weapon) || !BotUsingCorrectWeapon(self))
	{
		int i = 0;
		for (i = 0; i < sizeof(weapons) / sizeof(weapons[0]); ++i)
		{
			if (weapons[i] == desired_weapon)
			{
				self->fb.desired_weapon_impulse = i + 1;

				return true;
			}
		}

		return false;
	}

	return true;
}

static qbool ShotForLuck(vec3_t object)
{
	trap_makevectors(self->s.v.v_angle);
	traceline(self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2], object[0], object[1],
				object[2], true, self);

	return (g_globalvars.trace_fraction == 1);
}

static void SetFireButtonBasedOnAngles(gedict_t *self, float rel_dist)
{
	float risk_factor = 0.5;
	float risk = g_random();
	float min_angle_error;
	vec3_t angle_error;
	int i;

	risk *= risk;
	VectorSubtract(self->fb.desired_angle, self->s.v.v_angle, angle_error);

	for (i = 0; i < 2; ++i)
	{
		if (angle_error[i] >= 180)
		{
			angle_error[i] -= 360;
		}
		else if (angle_error[i] < -180)
		{
			angle_error[i] += 360;
		}

		angle_error[i] = fabs(angle_error[i]);
	}

	min_angle_error = (1 + risk) * risk_factor * (self->fb.skill.accuracy + (1440 / rel_dist));

	// Frogbots take into account the distance they've had to snap to look at the player, and won't fire if distance is too high, compared to skill.accuracy
	self->fb.firing |= (angle_error[0] <= min_angle_error && angle_error[1] <= min_angle_error);
}

// FIXME: should just be avoiding bore anyway?
// FIXME: take strength of player & enemy into account, player might survive quad splashdamage, to enemy weapon
static void AvoidQuadBore(gedict_t *self)
{
	qbool has_quad = (int)self->s.v.items & IT_QUAD;
	qbool has_pent = (int)self->s.v.items & IT_INVULNERABILITY;
	qbool could_explode = self->fb.desired_weapon_impulse == 7
			|| self->fb.desired_weapon_impulse == 6;
	qbool could_hurt_self = could_explode && !has_pent && teamplay != 1 && teamplay != 5;

	if (cvar("k_midair") || !has_quad || !could_hurt_self)
	{
		return;
	}

	if ((self->fb.look_object == &g_edicts[self->s.v.enemy]) && (self->fb.enemy_dist <= 250))
	{
		// Enemy is too close for explosion, fire something else instead.
		int items_ = (int)self->s.v.items;
		int desired_weapon = IT_AXE;

		if ((items_ & IT_LIGHTNING) && (self->s.v.ammo_cells))
		{
			desired_weapon = IT_LIGHTNING;
		}
		else if ((items_ & IT_SUPER_NAILGUN) && (self->s.v.ammo_nails))
		{
			desired_weapon = IT_SUPER_NAILGUN;
		}
		else if ((items_ & IT_NAILGUN) && (self->s.v.ammo_nails))
		{
			desired_weapon = IT_NAILGUN;
		}
		else if ((items_ & IT_SUPER_SHOTGUN) && (self->s.v.ammo_shells))
		{
			desired_weapon = IT_SUPER_SHOTGUN;
		}
		else if (self->s.v.ammo_shells)
		{
			desired_weapon = IT_SHOTGUN;
		}

		self->fb.firing |= CheckNewWeapon(desired_weapon);
	}
}

// FIXME: This doesn't do what it says, as it has teamplay-value checks
static qbool CouldHurtTeammate(gedict_t *me)
{
	float ang, curang;
	gedict_t *p;

	if (teamplay == 0 || teamplay == 1 || teamplay == 5)
	{
		return false;
	}

	for (p = world; (p = find_plr(p));)
	{
		if (p != me)
		{
			if (SameTeam(me, p))
			{
				if (VisibleEntity(p))
				{
					vec3_t diff;
					VectorSubtract(p->s.v.origin, me->s.v.origin, diff);
					curang = vectoyaw(diff);
					ang = anglemod(me->s.v.angles[1] - curang);
					// FIXME: fb.skill
					if (ang < 20 || ang > 340)
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

// FIXME: Interesting... if a marker is the look object then it wouldn't explode on that?
static void SpamRocketShot(gedict_t *self)
{
	qbool has_rl = ((int)self->s.v.items & IT_ROCKET_LAUNCHER) && self->s.v.ammo_rockets > 3;
	qbool safe_to_fire = self->fb.allowedMakeNoise && !CouldHurtTeammate(self);

	if (self->fb.rocketJumping)
	{
		return;
	}

	if (!has_rl || !safe_to_fire)
	{
		return;
	}

	if (self->fb.look_object)
	{
		// dist_sfl = threshold distance before attempting shot for luck
		float dist_sfl = cvar("k_midair") ? 0 : ((int)self->s.v.items & IT_QUAD) ? 300.0f : 250.0f;
		vec3_t testplace;
		vec3_t rel_pos;
		float rel_dist;

		// rel_dist = distance between player and the item they're about to fire at
		VectorAdd(self->fb.look_object->s.v.absmin, self->fb.look_object->s.v.view_ofs, testplace);
		VectorSubtract(testplace, self->s.v.origin, rel_pos);
		rel_dist = vlen(rel_pos);

		if (rel_dist > dist_sfl && ShotForLuck(testplace))
		{
			// FIXME: This uses distance to enemy, not to testplace (?)
			if (RocketSafe())
			{
				// FIXME: Aim lower?  This looks like copy & paste from BotsFireLogic()
				//        Why self->origin + rel_pos when rel_pos = testplace - origin, why not just testplace? (or did RocketSafe() just overwrite?)
				traceline(self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] + 16,
							self->s.v.origin[0] + rel_pos[0], self->s.v.origin[1] + rel_pos[1],
							self->s.v.origin[2] + rel_pos[2] - 22, true, self);
				if (g_globalvars.trace_fraction == 1)
				{
					rel_pos[2] -= 38;
				}
				self->fb.state |= SHOT_FOR_LUCK;
				self->fb.desired_weapon_impulse = 7;
				self->fb.firing = true;
			}
			else
			{
				self->fb.state &= ~SHOT_FOR_LUCK;
			}
		}
	}
}

void FindRocketExplosionPoint(vec3_t player_origin, vec3_t player_angles, vec3_t output,
								float *distance_frac)
{
	vec3_t rocket_origin;
	vec3_t final_point;

	VectorCopy(player_origin, rocket_origin);
	rocket_origin[2] += 16;
	trap_makevectors(player_angles);
	VectorMA(rocket_origin, 600, g_globalvars.v_forward, final_point);
	traceline(PASSVEC3(rocket_origin), PASSVEC3(final_point), false, self);
	VectorCopy(g_globalvars.trace_endpos, output);
	*distance_frac = g_globalvars.trace_fraction;
}

// FIXME: Predicts aim*600, then predicts if the enemy's predicted position will be close to the explosion point...
static void RocketLauncherShot(gedict_t *self)
{
	float hit_radius = 160;
	float risk_strength;
	gedict_t *test_enemy;
	float risk_factor = 0.5;
	float risk = g_random();
	risk *= risk;

	FindRocketExplosionPoint(self->s.v.origin, self->fb.desired_angle, self->fb.rocket_endpos,
								&risk_strength);

	for (test_enemy = world; (test_enemy = find_plr(test_enemy));)
	{
		float predict_dist = 1000000;
		vec3_t testplace;

		// Ignore corpses
		if (!test_enemy->s.v.takedamage)
		{
			continue;
		}

		if (test_enemy == &g_edicts[self->s.v.enemy])
		{
			if (self->fb.look_object && (self->fb.look_object->ct == ctPlayer))
			{
				if (self->fb.look_object == &g_edicts[self->s.v.enemy])
				{
					VectorCopy(self->fb.predict_origin, testplace);
					predict_dist = VectorDistance(testplace, self->fb.rocket_endpos);
				}
			}
			else if (self->fb.look_object && (self->fb.look_object != world))
			{
				if (self->fb.allowedMakeNoise && self->fb.predict_shoot)
				{
					VectorAdd(self->fb.look_object->s.v.absmin, self->fb.look_object->s.v.view_ofs,
								testplace);
					from_marker = g_edicts[self->s.v.enemy].fb.touch_marker;
					path_normal = true;
					ZoneMarker(from_marker, self->fb.look_object, path_normal,
								g_edicts[self->s.v.enemy].fb.canRocketJump);
					traveltime = SubZoneArrivalTime(zone_time, middle_marker, self->fb.look_object,
													g_edicts[self->s.v.enemy].fb.canRocketJump);
					predict_dist = (traveltime * sv_maxspeed)
							+ VectorDistance(testplace, self->fb.rocket_endpos);
				}
			}
		}
		else
		{
			VectorCopy(test_enemy->s.v.origin, testplace);
			predict_dist = VectorDistance(testplace, self->fb.rocket_endpos);
		}

		if (predict_dist <= (hit_radius / (1 - risk)))
		{
			// See if the explosion would hurt that player
			traceline(PASSVEC3(self->fb.rocket_endpos), PASSVEC3(testplace), true, self);
			if (g_globalvars.trace_fraction == 1)
			{
				// Nothing blocking the explosion...
				if (!SameTeam(test_enemy, self))
				{
					// Enemy
					risk_factor /= risk_strength;
					if (self->fb.look_object == &g_edicts[self->s.v.enemy])
					{
						self->fb.firing = true;
					}
					else if (predict_dist <= (80 / (1.2 - risk)))
					{
						self->fb.firing = true;
					}
					else
					{
						SpamRocketShot(self);

						if ((int)self->s.v.items & IT_GRENADE_LAUNCHER)
						{
							if (self->fb.arrow == BACK)
							{
								if (self->s.v.enemy && !self->fb.rocketJumping)
								{
									if (self->fb.allowedMakeNoise && (self->s.v.ammo_rockets > 3)
											&& !CouldHurtTeammate(self))
									{
										self->fb.desired_weapon_impulse = 6;
										self->fb.firing = true;
									}
								}
							}
						}
					}
				}
				else
				{
					if (test_enemy != self)
					{
						return;
					}
					else
					{
						risk_factor = risk_factor * risk_strength;
					}
				}
			}
		}
	}
}

// If bot can jump and can attack player from higher position, will jump
static void JumpToAttack(vec3_t rel_pos)
{
	if (self->fb.look_object == &g_edicts[self->s.v.enemy])
	{
		if ((self->s.v.waterlevel == 0) && self->fb.allowedMakeNoise
				&& ((int)self->s.v.flags & FL_ONGROUND))
		{
			traceline(self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] + 32,
						self->s.v.origin[0] + rel_pos[0], self->s.v.origin[1] + rel_pos[1],
						self->s.v.origin[2] + rel_pos[2] + 32, false, self);
			SetJumpFlag(self, (g_globalvars.trace_fraction == 1), "JumpToAttack");
		}
	}
}

static qbool PreWarBlockFiring(gedict_t *self)
{
	// Only fire in pre-war if enemy attacked us
	if (match_in_progress == 0)
	{
		gedict_t *enemy = &g_edicts[self->s.v.enemy];
		qbool enemy_is_world = self->s.v.enemy && enemy->ct == ctNone;
		qbool looking_at_enemy = enemy == self->fb.look_object;
		qbool enemy_attacked = self->s.v.enemy && enemy->client_time < enemy->attack_finished + 0.5;
		qbool debugging_door = (self->fb.debug_path && enemy_is_world);

		// Don't fire at other bots
		if ((self->s.v.enemy == 0) || enemy->isBot || !self->fb.look_object)
		{
			self->fb.firing = false;

			return true;
		}

		// If looking at enemy and they haven't attacked us recently, don't fire
		// Exception for debug_path, when it can fire at doors (but not players)
		if (looking_at_enemy && !(enemy_attacked || debugging_door))
		{
			self->fb.firing = false;

			return true;
		}
	}

	// Countdown, never fire
	if (match_in_progress == 1)
	{
		self->fb.firing = false;

		return true;
	}

	return false;
}

qbool AttackFinished(gedict_t *self)
{
	if (self->client_time < self->attack_finished)
	{
		if ((int)self->s.v.weapon & (IT_LIGHTNING | IT_EITHER_NAILGUN))
		{
			return (g_globalvars.time < self->s.v.nextthink);
		}

		return true;
	}

	return false;
}

static qbool KeepFiringAtEnemy(gedict_t *self)
{
	// Keep fire button down if tracking enemy
	return ((self->fb.look_object == &g_edicts[self->s.v.enemy]) && (g_random() < 0.666667f)
			&& BotUsingCorrectWeapon(self));
}

static qbool MidairAimLogic(gedict_t *self, float rel_dist)
{
	// In midair mode, delay firing until the rocket will hit opponent near peak of their jump
	if (cvar(
			"k_midair") && self->fb.look_object && (self->fb.look_object->s.v.velocity[2] > JUMPSPEED))
	{
		float time_to_hit = rel_dist
				/ (self->super_damage_finished > g_globalvars.time ? 2000 : 1000);
		float time_to_stationary = self->fb.look_object->s.v.velocity[2] / 800;

		if (time_to_stationary - time_to_hit > 0.15f)
		{
			self->fb.firing = false;

			return true;
		}
	}

	return false;
}

static qbool HurtSelfLogic(gedict_t *self)
{
	// If we want to grab an armour to stop player getting it...
	if (self->fb.state & HURT_SELF)
	{
		if (HasWeapon(self, IT_ROCKET_LAUNCHER) && (self->fb.desired_angle[PITCH] > 75))
		{
			self->fb.desired_weapon_impulse = 7;
			self->fb.firing = true;
			self->fb.state &= ~HURT_SELF;
		}

		return true;
	}

	return false;
}

void SetFireButton(gedict_t *self, vec3_t rel_pos, float rel_dist)
{
	if (PreWarBlockFiring(self))
	{
		return;
	}

	if ((self->fb.enemy_dist > 600) && lgc_enabled())
	{
		self->fb.firing = false;

		return;
	}

	if (self->fb.firing)
	{
		if (KeepFiringAtEnemy(self))
		{
			return;
		}

		if (!AttackFinished(self))
		{
			return;
		}

		self->fb.firing &= self->fb.rocketJumping && self->fb.rocketJumpFrameDelay == 0;
	}
	else if (self->fb.next_impulse)
	{
		return;
	}

	if (FUTURE(min_fire_time))
	{
		self->fb.firing = false;

		return;
	}

	if (MidairAimLogic(self, rel_dist))
	{
		return;
	}

	if (SameTeam(self->fb.look_object, self))
	{
		self->fb.firing = false;

		return;
	}

	DM6SelectWeaponToOpenDoor(self);

	if (HurtSelfLogic(self))
	{
		return;
	}

	if (self->s.v.enemy && g_edicts[self->s.v.enemy].fb.touch_marker)
	{
		traceline(self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] + 16,
					self->s.v.origin[0] + rel_pos[0], self->s.v.origin[1] + rel_pos[1],
					self->s.v.origin[2] + rel_pos[2] + 16, false, self);

		if (g_globalvars.trace_fraction == 1)
		{
			// FIXME: This keeps accuracy too high, stops the first shot from missing? !!!! FIXME FIXME FIXME
			// i.e. could be using rocket launcher for 'shot-for-luck', or using other weapon to hit world
			if (!((self->fb.desired_weapon_impulse == 7)
					|| (NUM_FOR_EDICT(self->fb.look_object) == self->s.v.enemy)))
			{
				// don't needlessly fire into space
				return;
			}
		}
		else if (PROG_TO_EDICT(g_globalvars.trace_ent) != self->fb.look_object)
		{
			gedict_t *traced = PROG_TO_EDICT(g_globalvars.trace_ent);
			if (traced->ct == ctPlayer)
			{
				if (!SameTeam(traced, self))
				{
					if (!((int)self->s.v.flags & FL_WATERJUMP))
					{
						self->s.v.enemy = NUM_FOR_EDICT(traced);
						LookEnemy(self, traced);
					}
				}

				return;
			}
			else
			{
				JumpToAttack(rel_pos);

				return;
			}
		}

		// At this point, bot is looking at enemy

		// FIXME: This is broken, we should use other weapon in water and override with discharge if necessary
		if ((self->fb.desired_weapon_impulse == 8) && (self->s.v.waterlevel > 1))
		{
			return;
		}

		{
			AvoidQuadBore(self);

			if (self->fb.desired_weapon_impulse == 7)
			{
				RocketLauncherShot(self);
			}
			else
			{
				SetFireButtonBasedOnAngles(self, rel_dist);
			}
		}
	}
}

// FIXME: should still discharge if < 25 cells and would kill enemy...
static qbool BotShouldDischarge(void)
{
	gedict_t *enemy = &g_edicts[self->s.v.enemy];

	if (self->s.v.waterlevel != 3)
	{
		return false;
	}

	if (!((int)self->s.v.items & IT_LIGHTNING))
	{
		return false;
	}

	if (self->s.v.ammo_cells < 25)
	{
		return false;
	}

	if (self->fb.enemy_dist > 600)
	{
		return false;
	}

	if (self->fb.look_object != enemy)
	{
		return false;
	}

	if (self->invincible_time > g_globalvars.time)
	{
		if (trap_pointcontents(PASSVEC3(enemy->s.v.origin)) == CONTENT_WATER)
		{
			return true;
		}
	}

	if (((int)self->s.v.items & (IT_ROCKET_LAUNCHER | IT_LIGHTNING))
			&& (self->s.v.ammo_rockets > 25) && (self->s.v.ammo_cells > 25))
	{
		return false;
	}

	if (((int)self->s.v.items & IT_NAILGUN_ROCKET) && (self->s.v.ammo_rockets > 25)
			&& (self->s.v.ammo_nails > 25))
	{
		return false;
	}

	if ((self->tp.enemy_count - self->tp.teammate_count) >= 2)
	{
		return (g_random() < 0.003);
	}

	return false;
}

static int DesiredWeapon(void)
{
	int items_ = self->s.v.items;
	qbool has_rl = self->s.v.ammo_rockets && (items_ & IT_ROCKET_LAUNCHER);
	qbool has_lg = self->s.v.ammo_cells && (items_ & IT_LIGHTNING);
	qbool shaft_available = false;
	qbool avoid_rockets = false;
	qbool firing_lg = self->fb.firing && self->s.v.weapon == IT_LIGHTNING && self->s.v.ammo_cells
			&& self->client_time < self->attack_finished;

	if (TP_CouldDamageTeammate(self))
	{
		return IT_SHOTGUN;
	}

	// When to always use RL
	if ((self->fb.skill.rl_preference >= g_random()) || fb_lg_disabled())
	{
		if (has_rl)
		{
			if (RocketSafe())
			{
				return IT_ROCKET_LAUNCHER;
			}

			avoid_rockets = true;
		}
	}

	// If firing LG then keep going, else look at LG_pref to switch to it
	if ((firing_lg || (self->fb.skill.lg_preference >= g_random())) && !fb_lg_disabled())
	{
		if ((self->s.v.waterlevel <= 1) || ((int)self->s.v.items & IT_INVULNERABILITY))
		{
			if (has_lg)
			{
				if (self->fb.enemy_dist <= 600)
				{
					return IT_LIGHTNING;
				}

				shaft_available = true;
			}
		}
	}

	if (BotShouldDischarge())
	{
		return IT_LIGHTNING;
	}

	if (!fb_lg_disabled())
	{
		if ((self->s.v.waterlevel <= 1) || ((int)self->s.v.items & IT_INVULNERABILITY))
		{
			if (items_ & IT_LIGHTNING)
			{
				if (self->s.v.ammo_cells)
				{
					if (self->fb.enemy_dist <= 600)
					{
						if (self->fb.look_object == &g_edicts[self->s.v.enemy])
						{
							vec3_t diff, enemy_angles;

							VectorSubtract(self->fb.look_object->s.v.origin, self->s.v.origin, diff);
							vectoangles(diff, enemy_angles);
							if (enemy_angles[0] < 15)
							{
								if (enemy_angles[0] > -15)
								{
									return IT_LIGHTNING;
								}
							}
						}
						else
						{
							return IT_LIGHTNING;
						}

						shaft_available = true;
					}
				}
			}
		}
	}

	if (!avoid_rockets)
	{
		if (items_ & IT_ROCKET_LAUNCHER)
		{
			if (self->s.v.ammo_rockets)
			{
				if (RocketSafe())
				{
					return IT_ROCKET_LAUNCHER;
				}

				if (!((int)self->s.v.items & IT_INVULNERABILITY))
				{
					avoid_rockets = true;
				}
			}
		}
	}

	if (self->fb.state & WAIT)
	{
		if (items_ & IT_ROCKET_LAUNCHER)
		{
			if (self->s.v.ammo_rockets)
			{
				if (RocketSafe())
				{
					return IT_ROCKET_LAUNCHER;
				}
			}
		}
		else if (items_ & IT_LIGHTNING)
		{
			if (self->s.v.ammo_cells)
			{
				if (shaft_available)
				{
					return IT_LIGHTNING;
				}
			}
		}
	}

	if (shaft_available)
	{
		return IT_LIGHTNING;
	}

	if (self->fb.enemy_dist <= 320)
	{
		if (!avoid_rockets)
		{
			if (items_ & IT_GRENADE_LAUNCHER)
			{
				if (self->s.v.ammo_rockets)
				{
					if (RocketSafe())
					{
						if (WaterCombat(self))
						{
							return IT_GRENADE_LAUNCHER;
						}
					}
				}
			}
		}
	}

	if (self->fb.enemy_dist <= 600)
	{
		if (items_ & IT_SUPER_NAILGUN)
		{
			if (self->s.v.ammo_nails)
			{
				return IT_SUPER_NAILGUN;
			}
		}

		if (items_ & IT_SUPER_SHOTGUN)
		{
			if (self->s.v.ammo_shells)
			{
				return IT_SUPER_SHOTGUN;
			}
		}

		if (items_ & IT_NAILGUN)
		{
			if (self->s.v.ammo_nails)
			{
				return IT_NAILGUN;
			}
		}
	}

	if (self->s.v.ammo_shells)
	{
		return IT_SHOTGUN;
	}

	return IT_AXE;
}

static int WeaponToImpulse(int weapon)
{
	static int weapons[] = {
		IT_AXE, IT_SHOTGUN, IT_SUPER_SHOTGUN,
		IT_NAILGUN, IT_SUPER_NAILGUN, IT_GRENADE_LAUNCHER,
		IT_ROCKET_LAUNCHER, IT_LIGHTNING };
	int i;

	for (i = 0; i < sizeof(weapons) / sizeof(weapons[0]); ++i)
	{
		if (weapons[i] == weapon)
		{
			return i + 1;
		}
	}

	return -1;
}

void SelectWeapon(void)
{
	int desired_weapon;
	int fb_weapon;
	int impulse;

	if (self->fb.path_state & DM6_DOOR)
	{
		return;
	}

	if (self->fb.state & HURT_SELF)
	{
		qbool has_rl = self->s.v.ammo_rockets && ((int)self->s.v.items & IT_ROCKET_LAUNCHER);

		if (has_rl && (self->s.v.health >= 60) && (self->super_damage_finished <= g_globalvars.time))
		{
			self->fb.desired_weapon_impulse = 7;

			return;
		}

		self->fb.state &= ~HURT_SELF;
	}

	desired_weapon = DesiredWeapon();
	CheckNewWeapon(desired_weapon);

	if (tot_mode_enabled())
	{
		if ((fb_weapon = FrogbotWeapon()))
		{
			self->fb.desired_weapon_impulse = fb_weapon;
		}
		else
		{
			impulse = WeaponToImpulse(desired_weapon);
			self->fb.desired_weapon_impulse =
				self->fb.random_desired_weapon_impulse == impulse ? impulse : 2;
		}
	}
}

#endif // BOT_SUPPORT
