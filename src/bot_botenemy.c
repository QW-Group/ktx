/*
 bot/botenemy.qc

 Copyright (C) 1997-1999 Robert 'Frog' Field
 */
// Converted from .qc on 05/02/2016

#ifdef BOT_SUPPORT

#include "g_local.h"

// Removes the look object for the given player
void ClearLookObject(gedict_t *player)
{
	player->fb.look_object = NULL;
}

// Sets the look object for the player
void LookEnemy(gedict_t *player, gedict_t *enemy)
{
	player->fb.look_object = enemy;
	VectorCopy(enemy->s.v.origin, player->fb.predict_origin);
	self->fb.old_linked_marker = NULL;
}

// Called when a player inflicts damage on another
void BotDamageInflictedEvent(gedict_t *attacker, gedict_t *targ)
{
	targ->fb.last_hurt = g_globalvars.time;

	if (targ->isBot)
	{
		// in dmm4, there's a chance taking damage will reset move direction
		if ((deathmatch >= 4) && (g_random() < targ->fb.skill.wiggle_toggle)
				&& (abs(targ->fb.wiggle_run_dir) > (self->fb.skill.wiggle_run_limit / 2)))
		{
			targ->fb.wiggle_run_dir = targ->fb.wiggle_run_dir < 0 ? 1 : -1;
		}

		// if object we're looking at has less firepower than us...
		if (targ->fb.look_object && (targ->fb.look_object->fb.firepower < attacker->fb.firepower))
		{
			if (attacker != targ)
			{
				// look at the attacker
				targ->fb.look_object = attacker;
				VectorCopy(attacker->s.v.origin, targ->fb.predict_origin);

				if (!SameTeam(attacker, targ))
				{
					if (targ->s.v.goalentity == targ->s.v.enemy)
					{
						targ->fb.goal_refresh_time = 0;
					}

					targ->fb.enemy_time = g_globalvars.time + 1;
					targ->s.v.enemy = NUM_FOR_EDICT(attacker);
				}
				else
				{
					targ->fb.enemy_time = g_globalvars.time + 2.5;
				}
			}
		}
	}
}

// Triggered whenever an entity makes a noise
void BotsSoundMadeEvent(gedict_t *entity)
{
	if (entity && entity->ct == ctPlayer)
	{
		gedict_t *plr;

		// Find all bots which has this entity as enemy
		for (plr = world; (plr = find_plr(plr));)
		{
			if (plr->isBot && !(plr->fb.state & NOTARGET_ENEMY))
			{
				if ((NUM_FOR_EDICT(entity) == plr->s.v.enemy) && (entity != plr->fb.look_object))
				{
					vec3_t temp;
					VectorSubtract(entity->s.v.origin, plr->s.v.origin, temp);

					// Did the bot hear it?
					if (VectorLength(temp) < 1000)
					{
						if (Visible_360(plr, entity))
						{
							// Look directly at the player
							plr->fb.look_object = entity;
						}
						else if (isDuel())
						{
							if (entity->fb.touch_marker
									&& Visible_360(plr, entity->fb.touch_marker))
							{
								plr->fb.look_object = entity->fb.touch_marker;
							}
							else
							{
								// TODO: Look at the closest possible point
								//   (think of picking up mega on DM4, humans would look at mega exit)
								//   (find route from noise -> player, look at last visible marker?)
							}
						}
					}
				}
			}
		}
	}
}

// FIXME: Globals (from_marker, to_marker, etc)
// Evaluates a potential enemy (globals: path_normal)
static void BestEnemy_apply(gedict_t *test_enemy, float *best_score, gedict_t **enemy_,
							float *predict_dist)
{
	float enemy_score;

	path_normal = true;

	look_marker = SightFromMarkerFunction(from_marker, to_marker);
	if (look_marker != NULL)
	{
		ZoneMarker(from_marker, look_marker, path_normal, test_enemy->fb.canRocketJump, test_enemy->fb.canHook);
		traveltime = SubZoneArrivalTime(zone_time, middle_marker, look_marker,
										test_enemy->fb.canRocketJump, test_enemy->fb.canHook);
		enemy_score = traveltime + g_random();
	}
	else
	{
		look_marker = SightMarker(from_marker, to_marker, 0, 0);
		enemy_score = look_traveltime + g_random();
	}

	// Prioritize enemy with flag
	if (test_enemy->ctf_flag & CTF_FLAG) enemy_score /= 2;

	if (enemy_score < *best_score && look_marker != NULL) // TODO hiipe - crashes here sometimes! Surely it should be possible for look_marker to be NULL?
	{
		vec3_t marker_view;
		vec3_t to_marker_view;

		VectorAdd(look_marker->s.v.absmin, look_marker->s.v.view_ofs, marker_view);
		VectorAdd(to_marker->s.v.absmin, to_marker->s.v.view_ofs, to_marker_view);

		*best_score = enemy_score;
		*enemy_ = test_enemy;
		*predict_dist = VectorDistance(marker_view, to_marker_view);
	}
}

// Selects best enemy from all players.
//   Evaluates based on time from respective last-touched markers 
//   FIXME: evaluates in either direction (ease vs risk?  fix)
qbool BotsPickBestEnemy(gedict_t *self)
{
	float best_score = 1000000;
	gedict_t *enemy_ = NULL;
	float predict_dist = 600;
	gedict_t *test_enemy;
	int old_enemy = self->s.v.enemy;
	qbool team_game = isTeam();

	for (test_enemy = world; (test_enemy = find_plr(test_enemy));)
	{
		if ((!team_game || ISLIVE(test_enemy)) && !SameTeam(self, test_enemy))
		{
			from_marker = test_enemy->fb.touch_marker;
			if (from_marker)
			{
				to_marker = self->fb.touch_marker;
				BestEnemy_apply(test_enemy, &best_score, &enemy_, &predict_dist);
				to_marker = from_marker;
				from_marker = self->fb.touch_marker;
				BestEnemy_apply(test_enemy, &best_score, &enemy_, &predict_dist);
			}
		}
	}

	self->fb.enemy_time = g_globalvars.time + 1;
	self->s.v.enemy = (enemy_ == NULL ? 0 : NUM_FOR_EDICT(enemy_));
	self->fb.enemy_dist = predict_dist;

	return (self->s.v.enemy != old_enemy);
}

#endif
