/*
 bot/botgoals.qc

 Copyright (C) 1997-1999 Robert 'Frog' Field
 Copyright (C) 2000-2001 DMSouL
 Copyright (C) 2000-2007 ParboiL
 */

// Converted from .qc on 05/02/2016

#ifdef BOT_SUPPORT

#include "g_local.h"

//static float best_score;
#define BACKPACK_CLASSNAME "backpack"
#define FROGBOT_CHANCE_HELP_TEAMMATE 0.25

// FIXME: Local Globals
static float best_respawn_time = 0;

qbool POVDMM4DontWalkThroughDoor(gedict_t *goal_entity);
qbool DM6DoorLogic(gedict_t *self, gedict_t *goal_entity);

void SUB_regen(void);
void SUB_regen_powerups(void);

qbool WaitingToRespawn(gedict_t *ent)
{
	return (((ent->s.v.nextthink >= g_globalvars.time) && (ent->think == (func_t)SUB_regen_powerups))
			|| ((ent->s.v.nextthink >= g_globalvars.time) && (ent->think == (func_t)SUB_regen)))
			&& strnull(ent->model);
}

// If an item is picked up, all bots heading for that item should re-evaluate their goals
void UpdateGoalEntity(gedict_t *item, gedict_t *taker)
{
	gedict_t *plr;
	int item_entity = NUM_FOR_EDICT(item);

	for (plr = world; (plr = find_plr(plr));)
	{
		// if the same team, pretend bot read a 'took' notification
		qbool same_team = SameTeam(plr, taker);
		qbool heard_it = VectorDistance(plr->s.v.origin, item->s.v.origin) < 1000;
		float delay = (same_team || heard_it ? 0 : g_random());

		if (plr->s.v.goalentity == item_entity)
		{
			plr->fb.goal_refresh_time = min(plr->fb.goal_refresh_time, g_globalvars.time + delay);
			ResetGoalEntity(plr);
		}
	}
}

// If teammate is sitting on item, leave for them
// FIXME: If teammate is considerably weaker and bot needs/wants item, shout COMING instead
static qbool GoalLeaveForTeammate(gedict_t *self, gedict_t *goal_entity)
{
	gedict_t* plr;

	if ((g_globalvars.time < goal_entity->fb.touchPlayerTime) && goal_entity->fb.touchPlayer
			&& (goal_entity->fb.touchPlayer != self))
	{
		if (WaitingToRespawn(goal_entity) && SameTeam(goal_entity->fb.touchPlayer, self))
		{
			goal_entity->fb.saved_goal_desire = 0;

			return true;
		}
	}

	// Check bot teammates so the bots don't all gang up for items
	for (plr = world; (plr = find_plr(plr));)
	{
		if (plr->isBot && SameTeam(self, plr))
		{
			// Don't ever leave these items!
			char* ignore[] =
			{ "item_artifact_invulnerability", "item_artifact_invisibility",
					"item_artifact_super_damage", "item_flag_team1", "item_flag_team2" };

			int i;
			for (i = 0; i < sizeof(ignore) / sizeof(ignore[0]); ++i)
			{
				if (streq(goal_entity->classname, ignore[i])) return false;
			}

			// Let the bot with the highest desire take the item, if the bots can see each other.
			if (goal_entity == plr->fb.best_goal
				&& Visible_360(self, plr)
				&& goal_entity->fb.saved_goal_desire <= plr->fb.best_goal->fb.saved_goal_desire)
			{
				goal_entity->fb.saved_goal_desire = 0;
				return true;
			}
		}
	}

	return false;
}

static qbool ShouldGoalIgnoreDistance(gedict_t *goal_entity) 
{
	if (isCTF())
	{
		if ((streq(goal_entity->classname, "item_flag_team1") || streq(goal_entity->classname, "item_flag_team2"))
			&& (self->fb.skill.ctf_role == FB_CTF_ROLE_ATTACK || goal_entity->fb.saved_goal_desire > 5000))
		{
			return true;
		}
		else if (streq(goal_entity->classname, "item_artifact_super_damage")
			&& self->fb.skill.ctf_role == FB_CTF_ROLE_MIDFIELD)
		{
			return true;
		}
		else if (streq(goal_entity->classname, "marker") && (goal_entity->fb.T & MARKER_FLAG1_DEFEND || goal_entity->fb.T & MARKER_FLAG2_DEFEND)
			&& self->fb.skill.ctf_role == FB_CTF_ROLE_DEFEND)
		{
			return true;
		}
	}
	return false;
}

// Evaluates a goal 
void EvalGoal(gedict_t *self, gedict_t *goal_entity)
{
	float goal_desire =
			goal_entity && goal_entity->fb.desire ? goal_entity->fb.desire(self, goal_entity) : 0;
	float goal_time = 0.0f;
	qbool ignoreDistance;

	if (!goal_entity)
	{
		return;
	}

	if (self->fb.fixed_goal != NULL)
	{
		goal_desire = (self->fb.fixed_goal == goal_entity ? 1000 : 0);
	}

	goal_entity->fb.saved_goal_desire = goal_desire;
	if (goal_desire > 0)
	{
		if (POVDMM4DontWalkThroughDoor(goal_entity))
		{
			return;
		}

		// If one person on a team is sitting waiting for an item to respawn
		if (GoalLeaveForTeammate(self, goal_entity))
		{
			return;
		}

		// If item isn't going to respawn before match end
		if (match_end_time && goal_entity->fb.goal_respawn_time > match_end_time)
		{
			goal_entity->fb.saved_goal_desire = 0;

			return;
		}

		// Calculate travel time to the goal
		from_marker = self->fb.touch_marker;
		to_marker = goal_entity->fb.touch_marker;
		ZoneMarker(from_marker, to_marker, path_normal, self->fb.canRocketJump, self->fb.canHook);
		traveltime = SubZoneArrivalTime(zone_time, middle_marker, to_marker,
										self->fb.canRocketJump, self->fb.canHook);
		goal_time = traveltime;

		if (self->fb.goal_enemy_repel)
		{
			// Time for our enemy to get there
			from_marker = g_edicts[self->s.v.enemy].fb.touch_marker;
			ZoneMarker(from_marker, to_marker, path_normal,
						g_edicts[self->s.v.enemy].fb.canRocketJump,
						g_edicts[self->s.v.enemy].fb.canHook);
			traveltime = SubZoneArrivalTime(zone_time, middle_marker, to_marker,
											g_edicts[self->s.v.enemy].fb.canRocketJump, self->fb.canHook);

			// If enemy will get there much faster than we will...
			if (traveltime <= (goal_time - 1.25))
			{
				goal_desire += self->fb.goal_enemy_repel;
				goal_entity->fb.saved_goal_desire = goal_desire;
				if (goal_desire <= 0)
				{
					return;
				}
			}
		}

		if (DM6DoorLogic(self, goal_entity))
		{
			return;
		}

		goal_entity->fb.saved_respawn_time = (goal_entity->fb.goal_respawn_time - g_globalvars.time)
				+ (goal_time * self->fb.skill.prediction_error * g_random());
		if (goal_entity->fb.G_ == 18)
		{
			G_sprint(
					self,
					PRINT_HIGH,
					"Goal18: [%3.1f vs %3.1f] Eval(%s) = max(gt=%3.1f, respawn=%3.1f, srespawn=%3.1f)\n",
					g_globalvars.time, match_end_time, goal_entity->classname, goal_time,
					goal_entity->fb.goal_respawn_time, goal_entity->fb.saved_respawn_time);
		}
		goal_time = max(goal_time, goal_entity->fb.saved_respawn_time);
		goal_entity->fb.saved_goal_time = goal_time;
		if (self->fb.bot_evade)
		{
			if (self->fb.goal_enemy_repel)
			{
				qbool rl_routes = g_edicts[self->s.v.enemy].fb.canRocketJump;
				qbool hook_routes = g_edicts[self->s.v.enemy].fb.canHook;

				from_marker = g_edicts[self->s.v.enemy].fb.touch_marker;
				ZoneMarker(from_marker, to_marker, path_normal, rl_routes, hook_routes);
				traveltime = SubZoneArrivalTime(zone_time, middle_marker, to_marker, rl_routes, hook_routes);
				goal_entity->fb.saved_enemy_time_squared = traveltime * traveltime;
			}

			if (goal_time * goal_time >= goal_entity->fb.saved_enemy_time_squared)
			{
				goal_entity->fb.saved_goal_desire = 0;
				return;
			}
		}

		ignoreDistance = ShouldGoalIgnoreDistance(goal_entity);

		// If the bot can think far enough ahead...
		if (goal_time < self->fb.skill.lookahead_time || ignoreDistance)
		{
			float goal_score = ignoreDistance ? goal_desire : goal_desire * (self->fb.skill.lookahead_time - goal_time) / (goal_time + 5);
			
			if (goal_score > self->fb.best_goal_score)
			{
				self->fb.best_goal_score = goal_score;
				self->fb.best_goal = goal_entity;
			}
		}
	}
}

// FIXME: parameters
static void EvalGoal2(gedict_t *goal_entity, gedict_t *best_goal_marker, qbool canRocketJump, qbool canHook)
{
	float goal_desire = 0.0f;
	float traveltime2 = 0.0f;
	qbool ignoreDistance;

	if (goal_entity == NULL)
	{
		return;
	}

	goal_desire = goal_entity->fb.saved_goal_desire;
	if (goal_desire > 0)
	{
		float goal_time2 = goal_entity->fb.saved_goal_time;
		if (goal_time2 <= 5)
		{
			gedict_t *goal_marker2 = goal_entity->fb.touch_marker;
			from_marker = goal_marker2;
			ZoneMarker(from_marker, best_goal_marker, path_normal, canRocketJump, canHook);
			traveltime = SubZoneArrivalTime(zone_time, middle_marker, best_goal_marker,
											canRocketJump, canHook);
			traveltime2 = max(best_respawn_time, goal_time2 + traveltime);

			if (self->fb.bot_evade && self->fb.goal_enemy_repel)
			{
				if (traveltime2 * traveltime2 >= self->fb.best_goal->fb.saved_enemy_time_squared)
				{
					traveltime2 = 1000000;
				}
			}

			if (traveltime2 < self->fb.skill.lookahead_time)
			{
				float goal_score2 =
						(goal_desire * (self->fb.skill.lookahead_time - goal_time2)
								/ (goal_time2 + 5))
								+ (self->fb.best_goal->fb.saved_goal_desire
										* (self->fb.skill.lookahead_time - traveltime2)
										/ (traveltime2 + 5));
				if (goal_score2 > self->fb.best_score2)
				{
					self->fb.best_score2 = goal_score2;
					self->fb.best_goal2 = goal_entity;
				}
			}

			from_marker = best_goal_marker;
			ZoneMarker(from_marker, goal_marker2, path_normal, canRocketJump, canHook);
			traveltime = SubZoneArrivalTime(zone_time, middle_marker, goal_marker2, canRocketJump, canHook);
			traveltime2 = max(self->fb.best_goal_time + traveltime,
								goal_entity->fb.saved_respawn_time);
			if (self->fb.bot_evade && self->fb.goal_enemy_repel)
			{
				if (traveltime2 * traveltime2 >= goal_entity->fb.saved_enemy_time_squared)
				{
					return;
				}
			}

			ignoreDistance = ShouldGoalIgnoreDistance(goal_entity);

			if (traveltime2 < self->fb.skill.lookahead_time || ignoreDistance)
			{
				float goal_score2 = ignoreDistance ? goal_desire : self->fb.best_goal_score
						+ (goal_desire * (self->fb.skill.lookahead_time - traveltime2)
								/ (traveltime2 + 5));

				if (goal_score2 > self->fb.best_score2)
				{
					self->fb.best_score2 = goal_score2;
					self->fb.best_goal2 = self->fb.best_goal;
				}
			}
		}
	}
}

static void EnemyGoalLogic(gedict_t *self)
{
	float best_goal_desire = self->fb.best_goal->fb.saved_goal_desire;
	float best_goal_time = self->fb.best_goal->fb.saved_goal_time;
	gedict_t *best_goal_marker = self->fb.best_goal->fb.touch_marker;

	best_respawn_time = self->fb.best_goal->fb.saved_respawn_time;

	self->fb.best_goal->fb.saved_goal_desire = 0;
	if (self->fb.goal_enemy_desire > 0)
	{
		float goal_time2 = g_edicts[self->s.v.enemy].fb.saved_goal_time;
		if (goal_time2 <= 5)
		{
			// Work out time to enemy marker
			gedict_t *goal_marker2 = g_edicts[self->s.v.enemy].fb.touch_marker;
			float traveltime2 = 0.0f;

			from_marker = goal_marker2;
			ZoneMarker(from_marker, best_goal_marker, path_normal, self->fb.canRocketJump, self->fb.canHook);
			traveltime = SubZoneArrivalTime(zone_time, middle_marker, best_goal_marker,
											self->fb.canRocketJump, self->fb.canHook);
			traveltime2 = max(goal_time2 + traveltime, best_respawn_time);

			if (traveltime2 < self->fb.skill.lookahead_time)
			{
				float goal_score2 = (self->fb.goal_enemy_desire
						* (self->fb.skill.lookahead_time - goal_time2) / (goal_time2 + 5))
						+ (best_goal_desire * (self->fb.skill.lookahead_time - traveltime2)
								/ (traveltime2 + 5));

				if (goal_score2 > self->fb.best_score2)
				{
					self->fb.best_score2 = goal_score2;
					self->fb.best_goal2 = &g_edicts[self->s.v.enemy];
				}
			}

			// Work out time to best goal marker
			from_marker = best_goal_marker;
			ZoneMarker(from_marker, goal_marker2, path_normal, self->fb.canRocketJump, self->fb.canHook);
			traveltime = SubZoneArrivalTime(zone_time, middle_marker, goal_marker2,
											self->fb.canRocketJump, self->fb.canHook);
			traveltime2 = max(best_goal_time + traveltime,
								g_edicts[self->s.v.enemy].fb.saved_respawn_time);

			if (traveltime2 < self->fb.skill.lookahead_time)
			{
				float goal_score2 =
						self->fb.best_goal_score
								+ (self->fb.goal_enemy_desire
										* (self->fb.skill.lookahead_time - traveltime2)
										/ (traveltime2 + 5));

				if (goal_score2 > self->fb.best_score2)
				{
					self->fb.best_score2 = goal_score2;
					self->fb.best_goal2 = self->fb.best_goal;
				}
			}
		}
	}
}

void UpdateGoal(gedict_t *self)
{
	int i = 0;
	gedict_t *enemy_ = &g_edicts[self->s.v.enemy];
	gedict_t *goal_entity = 0;
	char *dropped_powerup_names[] =
		{ "item_artifact_invulnerability", "item_artifact_invisibility",
				"item_artifact_super_damage" };

	self->fb.goal_refresh_time = g_globalvars.time + 2 + g_random();

	if (self->fb.fixed_goal)
	{
		self->s.v.goalentity = NUM_FOR_EDICT(self->fb.fixed_goal);
		self->fb.goal_refresh_time = g_globalvars.time + 2 + g_random();

		return;
	}

	self->fb.best_goal_score = 0;
	self->fb.best_goal = NULL;
	self->fb.goal_enemy_repel = self->fb.goal_enemy_desire = 0;

	BotEvadeLogic(self);

	if (enemy_->fb.touch_marker && !(self->ctf_flag & CTF_FLAG)) // Don't chase if carrying flag
	{
		self->fb.virtual_enemy = enemy_;
		self->fb.goal_enemy_desire =
				enemy_ && enemy_->fb.desire ? enemy_->fb.desire(self, enemy_) : 0;
		if (self->fb.goal_enemy_desire > 0)
		{
			gedict_t *enemy = &g_edicts[self->s.v.enemy];
			// Time from here to the enemy's last marker
			from_marker = self->fb.touch_marker;
			ZoneMarker(from_marker, enemy->fb.touch_marker, path_normal, self->fb.canRocketJump, self->fb.canHook);
			traveltime = SubZoneArrivalTime(zone_time, middle_marker, enemy->fb.touch_marker,
											self->fb.canRocketJump, self->fb.canHook);
			enemy_->fb.saved_respawn_time = 0;
			enemy_->fb.saved_goal_time = traveltime;

			if (traveltime < self->fb.skill.lookahead_time)
			{
				float goal_score = self->fb.goal_enemy_desire
						* (self->fb.skill.lookahead_time - traveltime) / (traveltime + 5);

				if (goal_score > self->fb.best_goal_score)
				{
					self->fb.best_goal_score = goal_score;
					self->fb.best_goal = enemy_;
					enemy_->fb.saved_goal_desire = self->fb.goal_enemy_desire;
				}
			}
		}
		else if (enemy_->s.v.enemy == NUM_FOR_EDICT(self))
		{
			// our enemy is after us...
			self->fb.goal_enemy_repel = self->fb.goal_enemy_desire;
		}
	}
	else
	{
		self->fb.virtual_enemy = dropper;
	}

	for (i = 0; i < NUMBER_GOALS; ++i)
	{
		EvalGoal(self, self->fb.touch_marker->fb.goals[i].next_marker->fb.virtual_goal);
	}

	// Dropped backpacks
	for (goal_entity = world; (goal_entity = ez_find(goal_entity, BACKPACK_CLASSNAME));)
	{
		if (goal_entity->fb.touch_marker)
		{
			EvalGoal(self, goal_entity);
		}
	}

	// Dropped powerups
	for (i = 0; i < sizeof(dropped_powerup_names) / sizeof(dropped_powerup_names[0]); ++i)
	{
		for (goal_entity = world; (goal_entity = ez_find(goal_entity, dropped_powerup_names[i]));)
		{
			if ((goal_entity->cnt > g_globalvars.time) && goal_entity->fb.touch_marker)
			{
				EvalGoal(self, goal_entity);
			}
		}
	}

	// Dropped flags
	for (goal_entity = world; (goal_entity = ez_find(goal_entity, "item_flag_team1"));)
	{
		EvalGoal(self, goal_entity);
	}
	for (goal_entity = world; (goal_entity = ez_find(goal_entity, "item_flag_team2"));)
	{
		EvalGoal(self, goal_entity);
	}

	// Dropped runes
	for (goal_entity = world; (goal_entity = ez_find(goal_entity, "rune"));)
	{
		EvalGoal(self, goal_entity);
	}

	// Defense Markers and Discharge
	// TODO hiipe - might be more efficient to make these goals instead and then they would be checked above?
	// but this would be mixing generic behaviour the ctf and map specific goals.
	for (goal_entity = world; (goal_entity = ez_find(goal_entity, "marker"));)
	{
		if (goal_entity->fb.T & MARKER_FLAG1_DEFEND || goal_entity->fb.T & MARKER_FLAG2_DEFEND)
		{
			EvalGoal(self, goal_entity);
		}
		if (goal_entity->fb.T & MARKER_E2M2_DISCHARGE)
		{
			EvalGoal(self, goal_entity);
		}
	}

	if (teamplay && !isRA())
	{
		gedict_t *search_entity = HelpTeammate();

		if (search_entity && (g_random() < FROGBOT_CHANCE_HELP_TEAMMATE))
		{
			self->fb.best_goal = search_entity;
		}
	}

	if (self->fb.best_goal)
	{
		gedict_t *goal_entity;

		self->fb.best_goal2 = self->fb.best_goal;
		self->fb.best_score2 = self->fb.best_goal_score;

		EnemyGoalLogic(self);

		for (i = 0; i < NUMBER_GOALS; ++i)
		{
			gedict_t *next = self->fb.touch_marker->fb.goals[i].next_marker;

			if (next && (next != world) && (next != dropper))
			{
				EvalGoal2(self->fb.touch_marker->fb.goals[i].next_marker->fb.virtual_goal,
							self->fb.best_goal->fb.touch_marker, self->fb.canRocketJump, self->fb.canHook);
			}
		}

		for (goal_entity = world; (goal_entity = ez_find(goal_entity, BACKPACK_CLASSNAME));)
		{
			if (goal_entity->fb.touch_marker)
			{
				EvalGoal2(goal_entity, self->fb.best_goal->fb.touch_marker, self->fb.canRocketJump, self->fb.canHook);
			}
		}

		self->s.v.goalentity = NUM_FOR_EDICT(self->fb.best_goal2);
		self->fb.goal_respawn_time = g_globalvars.time + self->fb.best_goal2->fb.saved_respawn_time;
	}
	else
	{
		self->s.v.goalentity = NUM_FOR_EDICT(world);
	}
}

#endif // BOT_SUPPORT
