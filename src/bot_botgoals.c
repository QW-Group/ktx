// Converted from .qc on 05/02/2016

#ifdef BOT_SUPPORT

#include "g_local.h"
#include "fb_globals.h"

//static float best_score;
#define BACKPACK_CLASSNAME "backpack"
#define FROGBOT_CHANCE_HELP_TEAMMATE 0.25

// FIXME: Local Globals
static float best_respawn_time = 0;

qbool POVDMM4DontWalkThroughDoor (gedict_t* goal_entity);
qbool DM6DoorLogic (gedict_t* self, gedict_t* goal_entity);

void SUB_regen (void);
void SUB_regen_powerups ();

qbool WaitingToRespawn (gedict_t* ent)
{
	return ent->s.v.nextthink > g_globalvars.time && (ent->think == (func_t)SUB_regen || ent->think == (func_t)SUB_regen_powerups);
}

// If an item is picked up, all bots heading for that item should re-evaluate their goals
void UpdateGoalEntity(gedict_t* item, gedict_t* taker) {
	gedict_t* plr;
	int item_entity = NUM_FOR_EDICT(item);

	for (plr = world; (plr = find_plr(plr)); ) {
		// if the same team, pretend bot read a 'took' notification
		qbool same_team = SameTeam (plr, taker);
		qbool heard_it = VectorDistance (plr->s.v.origin, item->s.v.origin) < 1000;
		float delay = (same_team || heard_it ? 0 : g_random());

		if (plr->s.v.goalentity == item_entity) {
			plr->fb.goal_refresh_time = min (plr->fb.goal_refresh_time, g_globalvars.time + delay);
			ResetGoalEntity(plr);
		}
	}
}

// If teammate is sitting on item, leave for them
// FIXME: If teammate is considerably weaker and bot needs/wants item, shout COMING instead
static qbool GoalLeaveForTeammate (gedict_t* self, gedict_t* goal_entity) {
	if (g_globalvars.time < goal_entity->fb.touchPlayerTime && goal_entity->fb.touchPlayer && goal_entity->fb.touchPlayer != self) {
		if (WaitingToRespawn(goal_entity) && SameTeam(goal_entity->fb.touchPlayer, self) ) {
			goal_entity->fb.saved_goal_desire = 0;
			return true;
		}
	}

	return false;
}

// Evaluates a goal 
void EvalGoal(gedict_t* self, gedict_t* goal_entity) {
	float goal_desire = goal_entity && goal_entity->fb.desire ? goal_entity->fb.desire (self, goal_entity) : 0;
	float goal_time = 0.0f;

	if (!goal_entity)
		return;

	if (self->fb.fixed_goal != NULL)
		goal_desire = (self->fb.fixed_goal == goal_entity ? 1000 : 0);

	goal_entity->fb.saved_goal_desire = goal_desire;
	if (goal_desire > 0) {
		if (POVDMM4DontWalkThroughDoor (goal_entity))
			return;

		// If one person on a team is sitting waiting for an item to respawn
		if (GoalLeaveForTeammate (self, goal_entity)) {
			return;
		}

		// Calculate travel time to the goal
		from_marker = self->fb.touch_marker;
		to_marker = goal_entity->fb.touch_marker;
		ZoneMarker (from_marker, to_marker, path_normal, self->fb.canRocketJump);
		traveltime = SubZoneArrivalTime(zone_time, middle_marker, to_marker, self->fb.canRocketJump);
		goal_time = traveltime;

		if (self->fb.goal_enemy_repel) {
			// Time for our enemy to get there
			from_marker = g_edicts[self->s.v.enemy].fb.touch_marker;
			ZoneMarker (from_marker, to_marker, path_normal, g_edicts[self->s.v.enemy].fb.canRocketJump);
			traveltime = SubZoneArrivalTime(zone_time, middle_marker, to_marker, g_edicts[self->s.v.enemy].fb.canRocketJump);
			// If enemy will get there much faster than we will...
			if (traveltime <= (goal_time - 1.25)) {
				goal_desire += self->fb.goal_enemy_repel;
				goal_entity->fb.saved_goal_desire = goal_desire;
				if (goal_desire <= 0) {
					return;
				}
			}
		}

		if (DM6DoorLogic (self, goal_entity)) {
			return;
		}

		goal_entity->fb.saved_respawn_time = goal_entity->fb.goal_respawn_time - g_globalvars.time + (goal_time * self->fb.skill.prediction_error * g_random());
		goal_time = max (goal_time, goal_entity->fb.saved_respawn_time);
		goal_entity->fb.saved_goal_time = goal_time;
		if (self->fb.bot_evade) {
			if (self->fb.goal_enemy_repel) {
				qbool rl_routes = g_edicts[self->s.v.enemy].fb.canRocketJump;

				from_marker = g_edicts[self->s.v.enemy].fb.touch_marker;
				ZoneMarker (from_marker, to_marker, path_normal, rl_routes);
				traveltime = SubZoneArrivalTime(zone_time, middle_marker, to_marker, rl_routes);
				goal_entity->fb.saved_enemy_time_squared = traveltime * traveltime;
			}
			if (goal_time * goal_time >= goal_entity->fb.saved_enemy_time_squared) {
				goal_entity->fb.saved_goal_desire = 0;
				return;
			}
		}

		// If the bot can think far enough ahead...
		if (goal_time < self->fb.skill.lookahead_time) {
			float goal_score = goal_desire * (self->fb.skill.lookahead_time - goal_time) / (goal_time + 5);
			if (goal_score > self->fb.best_goal_score) {
				self->fb.best_goal_score = goal_score;
				self->fb.best_goal = goal_entity;
			}
		}
	}
}

// FIXME: parameters
static void EvalGoal2(gedict_t* goal_entity, gedict_t* best_goal_marker, qbool canRocketJump) {
	float goal_desire = 0.0f;
	float traveltime2 = 0.0f;

	if (goal_entity == NULL)
		return;

	goal_desire = goal_entity->fb.saved_goal_desire;
	if (goal_desire > 0) {
		float goal_time2 = goal_entity->fb.saved_goal_time;
		if (goal_time2 <= 5) {
			gedict_t* goal_marker2 = goal_entity->fb.touch_marker;
			from_marker = goal_marker2;
			ZoneMarker (from_marker, best_goal_marker, path_normal, canRocketJump);
			traveltime = SubZoneArrivalTime (zone_time, middle_marker, best_goal_marker, canRocketJump);
			traveltime2 = max(best_respawn_time, goal_time2 + traveltime);

			if (self->fb.bot_evade && self->fb.goal_enemy_repel) {
				if (traveltime2 * traveltime2 >= self->fb.best_goal->fb.saved_enemy_time_squared) {
					traveltime2 = 1000000;
				}
			}

			if (traveltime2 < self->fb.skill.lookahead_time) {
				float goal_score2 = (goal_desire * (self->fb.skill.lookahead_time - goal_time2) / (goal_time2 + 5)) + (self->fb.best_goal->fb.saved_goal_desire * (self->fb.skill.lookahead_time - traveltime2) / (traveltime2 + 5));
				if (goal_score2 > self->fb.best_score2) {
					self->fb.best_score2 = goal_score2;
					self->fb.best_goal2 = goal_entity;
				}
			}
			from_marker = best_goal_marker;
			ZoneMarker (from_marker, goal_marker2, path_normal, canRocketJump);
			traveltime = SubZoneArrivalTime (zone_time, middle_marker, goal_marker2, canRocketJump);
			traveltime2 = max(self->fb.best_goal_time + traveltime, goal_entity->fb.saved_respawn_time);
			if (self->fb.bot_evade && self->fb.goal_enemy_repel) {
				if (traveltime2 * traveltime2 >= goal_entity->fb.saved_enemy_time_squared) {
					return;
				}
			}
			if (traveltime2 < self->fb.skill.lookahead_time) {
				float goal_score2 = self->fb.best_goal_score + (goal_desire * (self->fb.skill.lookahead_time - traveltime2) / (traveltime2 + 5));
				if (goal_score2 > self->fb.best_score2) {
					self->fb.best_score2 = goal_score2;
					self->fb.best_goal2 = self->fb.best_goal;
				}
			}
		}
	}
}

static void EnemyGoalLogic (gedict_t* self)
{
	float best_goal_desire = self->fb.best_goal->fb.saved_goal_desire;
	float best_goal_time = self->fb.best_goal->fb.saved_goal_time;
	gedict_t* best_goal_marker = self->fb.best_goal->fb.touch_marker;
	best_respawn_time = self->fb.best_goal->fb.saved_respawn_time;

	self->fb.best_goal->fb.saved_goal_desire = 0;
	if (self->fb.goal_enemy_desire > 0) {
		float goal_time2 = g_edicts[self->s.v.enemy].fb.saved_goal_time;
		if (goal_time2 <= 5) {
			// Work out time to enemy marker
			gedict_t* goal_marker2 = g_edicts[self->s.v.enemy].fb.touch_marker;
			float traveltime2 = 0.0f;
			from_marker = goal_marker2;
			ZoneMarker (from_marker, best_goal_marker, path_normal, self->fb.canRocketJump);
			traveltime = SubZoneArrivalTime (zone_time, middle_marker, best_goal_marker, self->fb.canRocketJump);
			traveltime2 = max(goal_time2 + traveltime, best_respawn_time);

			if (traveltime2 < self->fb.skill.lookahead_time) {
				float goal_score2 = (self->fb.goal_enemy_desire * (self->fb.skill.lookahead_time - goal_time2) / (goal_time2 + 5)) + (best_goal_desire * (self->fb.skill.lookahead_time - traveltime2) / (traveltime2 + 5));
				if (goal_score2 > self->fb.best_score2) {
					self->fb.best_score2 = goal_score2;
					self->fb.best_goal2 = &g_edicts[self->s.v.enemy];
				}
			}

			// Work out time to best goal marker
			from_marker = best_goal_marker;
			ZoneMarker (from_marker, goal_marker2, path_normal, self->fb.canRocketJump);
			traveltime = SubZoneArrivalTime (zone_time, middle_marker, goal_marker2, self->fb.canRocketJump);
			traveltime2 = max(best_goal_time + traveltime, g_edicts[self->s.v.enemy].fb.saved_respawn_time);

			if (traveltime2 < self->fb.skill.lookahead_time) {
				float goal_score2 = self->fb.best_goal_score + (self->fb.goal_enemy_desire * (self->fb.skill.lookahead_time - traveltime2) / (traveltime2 + 5));
				if (goal_score2 > self->fb.best_score2) {
					self->fb.best_score2 = goal_score2;
					self->fb.best_goal2 = self->fb.best_goal;
				}
			}
		}
	}
}

void UpdateGoal(gedict_t* self) {
	int i = 0;
	gedict_t* enemy_ = &g_edicts[self->s.v.enemy];
	gedict_t* goal_entity = 0;
	char* dropped_powerup_names[] = {
		"item_artifact_invulnerability",
		"item_artifact_invisibility",
		"item_artifact_super_damage"
	};

	self->fb.goal_refresh_time = g_globalvars.time + 2 + g_random();

	if (self->fb.fixed_goal) {
		self->s.v.goalentity = NUM_FOR_EDICT (self->fb.fixed_goal);
		self->fb.goal_refresh_time = g_globalvars.time + 2 + g_random();
		return;
	}

	self->fb.best_goal_score = 0;
	self->fb.best_goal = NULL;
	self->fb.goal_enemy_repel = self->fb.goal_enemy_desire = 0;

	BotEvadeLogic(self);

	if (enemy_->fb.touch_marker) {
		self->fb.virtual_enemy = enemy_;
		self->fb.goal_enemy_desire = enemy_ && enemy_->fb.desire ? enemy_->fb.desire(self, enemy_) : 0;
		if (self->fb.goal_enemy_desire > 0) {
			gedict_t* enemy = &g_edicts[self->s.v.enemy];
			// Time from here to the enemy's last marker
			from_marker = self->fb.touch_marker;
			ZoneMarker (from_marker, enemy->fb.touch_marker, path_normal, self->fb.canRocketJump);
			traveltime = SubZoneArrivalTime(zone_time, middle_marker, enemy->fb.touch_marker, self->fb.canRocketJump);
			enemy_->fb.saved_respawn_time = 0;
			enemy_->fb.saved_goal_time = traveltime;

			if (traveltime < self->fb.skill.lookahead_time) {
				float goal_score = self->fb.goal_enemy_desire * (self->fb.skill.lookahead_time - traveltime) / (traveltime + 5);
				if (goal_score > self->fb.best_goal_score) {
					self->fb.best_goal_score = goal_score;
					self->fb.best_goal = enemy_;
					enemy_->fb.saved_goal_desire = self->fb.goal_enemy_desire;
				}
			}
		}
		else if (enemy_->s.v.enemy == NUM_FOR_EDICT(self)) {
			// our enemy is after us...
			self->fb.goal_enemy_repel = self->fb.goal_enemy_desire;
		}
	}
	else {
		self->fb.virtual_enemy = dropper;
	}

	for (i = 0; i < NUMBER_GOALS; ++i) {
		EvalGoal(self, self->fb.touch_marker->fb.goals[i].next_marker->fb.virtual_goal);
	}

	// Dropped backpacks
	for (goal_entity = world; (goal_entity = ez_find (goal_entity, BACKPACK_CLASSNAME)); ) {
		if (goal_entity->fb.touch_marker) {
			EvalGoal (self, goal_entity);
		}
	}

	// Dropped powerups
	for (i = 0; i < sizeof (dropped_powerup_names) / sizeof (dropped_powerup_names[0]); ++i) {
		for (goal_entity = world; (goal_entity = ez_find (goal_entity, dropped_powerup_names[i])); ) {
			if (goal_entity->cnt > g_globalvars.time && goal_entity->fb.touch_marker) {
				EvalGoal (self, goal_entity);
			}
		}
	}

	if (teamplay && !isRA()) {
		gedict_t* search_entity = HelpTeammate();
		if (search_entity && g_random() < FROGBOT_CHANCE_HELP_TEAMMATE) {
			self->fb.best_goal = search_entity;
		}
	}

	if (self->fb.best_goal) {
		gedict_t* goal_entity;

		self->fb.best_goal2 = self->fb.best_goal;
		self->fb.best_score2 = self->fb.best_goal_score;

		EnemyGoalLogic (self);

		for (i = 0; i < NUMBER_GOALS; ++i) {
			gedict_t* next = self->fb.touch_marker->fb.goals[i].next_marker;
			if (next && next != world && next != dropper) {
				EvalGoal2 (self->fb.touch_marker->fb.goals[i].next_marker->fb.virtual_goal, self->fb.best_goal->fb.touch_marker, self->fb.canRocketJump);
			}
		}

		for (goal_entity = world; (goal_entity = ez_find(goal_entity, BACKPACK_CLASSNAME)); ) {
			if (goal_entity->fb.touch_marker) {
				EvalGoal2 (goal_entity, self->fb.best_goal->fb.touch_marker, self->fb.canRocketJump);
			}
		}

		self->s.v.goalentity = NUM_FOR_EDICT(self->fb.best_goal2);
		self->fb.goal_respawn_time = g_globalvars.time + self->fb.best_goal2->fb.saved_respawn_time;
	}
	else {
		self->s.v.goalentity = NUM_FOR_EDICT(world);
	}
}

#endif // BOT_SUPPORT
