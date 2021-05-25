/*
 bot/botpath.qc

 Copyright (C) 1997-1999 Robert 'Frog' Field
 Copyright (C) 1998-2000 Matt 'asdf' McChesney
 Copyright (C) 2000-2001 DMSouL
 Copyright (C) 2000-2007 ParboiL
 */

// Converted from .qc on 05/02/2016
#ifdef BOT_SUPPORT

#include "g_local.h"
#include "fb_globals.h"

void DM3CampLogic();
void DM4CampLogic();
void DM6CampLogic();
void DM6MarkerTouchLogic(gedict_t *self, gedict_t *goalentity_marker);
qbool DM6LookAtDoor(gedict_t *self);

static qbool HasItem(gedict_t *player, int mask)
{
	return ((int)player->s.v.items & mask);
}

// FIXME: Globals
static void BotWaitLogic(gedict_t *self, int *new_path_state)
{
	gedict_t *touch_marker = self->fb.touch_marker;
	gedict_t *look_object = self->fb.look_object;

	// if we're not looking at a player
	if (look_object->ct != ctPlayer)
	{
		vec3_t linkedPos, lookPos;
		VectorAdd(self->fb.linked_marker->s.v.absmin, self->fb.linked_marker->s.v.view_ofs,
					linkedPos);
		VectorAdd(look_object->s.v.absmin, look_object->s.v.view_ofs, lookPos);
		traceline(linkedPos[0], linkedPos[1], linkedPos[2] + 32, lookPos[0], lookPos[1],
					lookPos[2] + 32, true, self);
		if (g_globalvars.trace_fraction != 1)
		{
			SetLinkedMarker(self, touch_marker, "BotWaitLogic");
			*new_path_state = 0;
		}
	}
	else
	{
		// stop waiting
		self->fb.state &= ~WAIT;
	}
}

static void EvalLook(gedict_t *self, float *best_score, vec3_t dir_look,
						vec3_t linked_marker_origin)
{
	vec3_t temp;
	float look_score;

	VectorAdd(from_marker->s.v.absmin, from_marker->s.v.view_ofs, temp);
	VectorSubtract(temp, linked_marker_origin, temp);
	VectorNormalize(temp);

	look_score = DotProduct(dir_look, temp);
	look_score = look_score + g_random();		// FIXME: Skill
	if (look_score > *best_score)
	{
		*best_score = look_score;
		self->fb.look_object = from_marker;
	}
}

static void EvalCloseRunAway(float runaway_time, gedict_t *enemy_touch_marker,
								float look_traveltime_squared, float *best_away_score,
								gedict_t **best_away_marker, gedict_t *touch_marker)
{
	float test_away_score = 0;
	float traveltime2;

	from_marker = enemy_touch_marker;
	ZoneMarker(from_marker, to_marker, path_normal, false);
	traveltime = SubZoneArrivalTime(zone_time, middle_marker, to_marker, false);
	traveltime2 = traveltime;
	from_marker = touch_marker;
	ZoneMarker(from_marker, to_marker, path_normal, false);
	traveltime = SubZoneArrivalTime(zone_time, middle_marker, to_marker, false);
	if (look_traveltime)
	{
		test_away_score = g_random() * runaway_time
				* ((traveltime2 * traveltime2)
						- (look_traveltime_squared + (traveltime * traveltime)))
				/ (look_traveltime * traveltime);
	}
	else
	{
		test_away_score = g_random() * runaway_time * (traveltime2 - traveltime);
	}

	if (test_away_score > *best_away_score)
	{
		*best_away_marker = to_marker;
		*best_away_score = test_away_score;
	}
}

static qbool CheckForRocketEnemyAim(gedict_t *self)
{
	gedict_t *enemy_ = &g_edicts[self->s.v.enemy];

	if ((enemy_ != world) && HasItem(enemy_, IT_ROCKET_LAUNCHER)
			&& !HasItem(self, IT_INVULNERABILITY))
	{
		// FIXME: random() call to determine behaviour, move threshold to bot's skill
		if ((enemy_->attack_finished <= (g_globalvars.time + 0.2)) && enemy_->s.v.ammo_rockets
				&& (g_random() < 0.5))
		{
			vec3_t src;
			VectorCopy(enemy_->s.v.origin, src);
			src[2] += 16;
			traceline(src[0], src[1], src[2], self->s.v.origin[0], self->s.v.origin[1],
						self->s.v.origin[2], true, self);

			// Fixme: Only avoid rocket aim if there's a direct line, rather than if they are within range of the explosion...
			if (g_globalvars.trace_fraction != 1)
			{
				trap_makevectors(enemy_->s.v.v_angle);
				traceline(src[0], src[1], src[2], src[0] + g_globalvars.v_forward[0] * 500,
							src[1] + g_globalvars.v_forward[1] * 500,
							src[2] + g_globalvars.v_forward[2] * 500, true, self);
				VectorCopy(g_globalvars.trace_endpos, self->fb.rocket_endpos);

				return true;
			}
		}
	}

	return false;
}

static qbool OnLift(gedict_t *self)
{
	if (streq(self->fb.touch_marker->classname, "door") && (self->deathtype == dtSQUISH))
	{
		if ((self->fb.linked_marker->s.v.absmin[2] + self->fb.linked_marker->s.v.view_ofs[2])
				> (self->s.v.origin[2] + 18))
		{
			if (teamplay)
			{
				self->fb.state &= ~HELP_TEAMMATE;
			}

			if (self->s.v.absmin[0] >= self->fb.touch_marker->s.v.absmin[0])
			{
				if (self->s.v.absmax[0] <= self->fb.touch_marker->s.v.absmax[0])
				{
					if (self->s.v.absmin[1] >= self->fb.touch_marker->s.v.absmin[1])
					{
						if (self->s.v.absmax[1] <= self->fb.touch_marker->s.v.absmax[1])
						{
							SetLinkedMarker(self, self->fb.touch_marker, "OnLift");
							self->fb.path_state = 0;
							self->fb.linked_marker_time = g_globalvars.time + 5;
							self->fb.old_linked_marker = NULL;
						}
					}
				}
			}

			return true;
		}
	}

	return false;
}

static qbool LookingAtPlayer(gedict_t *self)
{
	return (self->fb.look_object && (self->fb.look_object->ct == ctPlayer));
}

qbool WaitingToHitGround(gedict_t *self)
{
	return (self->fb.path_state & WAIT_GROUND) && !((int)self->s.v.flags & FL_ONGROUND);
}

static qbool WalkTowardsDroppedItem(gedict_t *self)
{
	gedict_t *goalentity_ = &g_edicts[self->s.v.goalentity];

	if (streq(goalentity_->classname, "backpack") && VisibleEntity(goalentity_))
	{
		SetLinkedMarker(self, goalentity_, "ProcNewLinked(backpack)");
		self->fb.linked_marker_time = g_globalvars.time + 5;
		self->fb.old_linked_marker = self->fb.touch_marker;

		return true;
	}
	else if (goalentity_->cnt)
	{
		SetLinkedMarker(self, goalentity_, "ProcNewLinked(dropped-powerup)");
		self->fb.linked_marker_time = g_globalvars.time + 5;
		self->fb.old_linked_marker = self->fb.touch_marker;

		return true;
	}

	return false;
}

// FIXME: Move to bot_aim
static qbool PredictionShotLogic(gedict_t *self, gedict_t *goalentity_marker)
{
	if ((match_in_progress == 2) && (g_random() < self->fb.skill.look_anywhere))
	{
		gedict_t *from_marker = g_edicts[self->s.v.enemy].fb.touch_marker;

		from_marker = (from_marker ? from_marker : goalentity_marker);
		if (from_marker)
		{
			gedict_t *to_marker = self->fb.linked_marker;

			look_marker = SightFromMarkerFunction(from_marker, to_marker);
			if (look_marker)
			{
				path_normal = true;
				ZoneMarker(from_marker, look_marker, path_normal, self->fb.canRocketJump);
				traveltime = SubZoneArrivalTime(zone_time, middle_marker, look_marker,
												self->fb.canRocketJump);
				look_traveltime = traveltime;
			}
			else
			{
				look_marker = SightMarker(from_marker, to_marker, 0, 0);
			}

			if (look_marker)
			{
				to_marker = from_marker;
				from_marker = self->fb.linked_marker;
				path_normal = true;
				ZoneMarker(from_marker, to_marker, path_normal, self->fb.canRocketJump);
				traveltime = SubZoneArrivalTime(zone_time, middle_marker, to_marker,
												self->fb.canRocketJump);
				if (look_traveltime < traveltime)
				{
					self->fb.look_object = look_marker;
					self->fb.predict_shoot = true;

					return true;
				}
			}
		}
	}

	return false;
}

void ProcessNewLinkedMarker(gedict_t *self)
{
	gedict_t *goalentity_ = &g_edicts[self->s.v.goalentity];
	gedict_t *goalentity_marker =
			(goalentity_ == world || (self->fb.state & RUNAWAY) ?
					NULL : goalentity_->fb.touch_marker);
	qbool trace_bprint = self->fb.debug;
	qbool rocket_jump_routes_allowed = self->fb.canRocketJump;
	qbool rocket_alert = false;
	float best_score = PATH_SCORE_NULL;
	vec3_t player_direction;
	gedict_t *new_linked_marker;
	int new_path_state = 0;
	int new_angle_hint = 0;
	int new_rj_delay = 0;
	float new_rj_angles[2] =
		{ 0, 0 };

	if (WaitingToHitGround(self))
	{
		return;
	}

	if (self->fb.linked_marker == self->fb.touch_marker)
	{
		if (goalentity_ == self->fb.touch_marker)
		{
			if (!WaitingToRespawn(self->fb.touch_marker))
			{
				// Have arrived at goal entity but not quite touched it yet
				return;
			}
		}
		else if (goalentity_marker == self->fb.touch_marker && WalkTowardsDroppedItem(self))
		{
			return;
		}
	}
	else
	{
		new_path_state = 0;
		if (ExistsPath(self->fb.old_linked_marker, self->fb.touch_marker, &new_path_state))
		{
			if (ExistsPath(self->fb.touch_marker, self->fb.linked_marker, &new_path_state))
			{
				self->fb.path_state = new_path_state;

				return;
			}
		}
		self->fb.state &= ~HURT_SELF;
	}

	self->fb.path_normal_ = true;
	if (self->fb.state & RUNAWAY)
	{
		gedict_t *enemy_touch_marker = g_edicts[self->s.v.enemy].fb.touch_marker;
		if (enemy_touch_marker)
		{
			int i = 0;
			float best_away_score = 0;
			float look_traveltime_squared = 0;
			gedict_t *best_away_marker = NULL;

			to_marker = self->fb.touch_marker;
			look_traveltime = SightFromTime(enemy_touch_marker, to_marker);
			look_traveltime_squared = look_traveltime * look_traveltime;
			path_normal = true;

			for (i = 0; i < NUMBER_PATHS; ++i)
			{
				to_marker = self->fb.touch_marker->fb.runaway[i].next_marker;
				if (to_marker)
				{
					EvalCloseRunAway(self->fb.touch_marker->fb.runaway[i].time, enemy_touch_marker,
										look_traveltime_squared, &best_away_score,
										&best_away_marker, self->fb.touch_marker);
				}
			}

			self->fb.goal_respawn_time = 0;
			goalentity_marker = (best_away_marker ? best_away_marker : self->fb.touch_marker);
			self->fb.path_normal_ = true;
		}
	}

	// FIXME: have RL and weaker than enemy... set goal entity to marker on edge of zone that the bot can see?
	//    Almost looks like this is picking a rocket-spam location?
	if (g_random() < 0.5)
	{
		qbool have_rl = HasItem(self, IT_ROCKET_LAUNCHER) && self->s.v.ammo_rockets;
		gedict_t *enemy_ = &g_edicts[self->s.v.enemy];

		if (have_rl)
		{
			if ((self->fb.firepower < enemy_->fb.firepower)
					&& (self->s.v.armorvalue < enemy_->s.v.armorvalue))
			{
				// Not avoiding enemy's rocket...
				if (!self->fb.avoiding && enemy_->fb.touch_marker)
				{
					gedict_t *look_marker = HigherSightFromFunction(self->fb.touch_marker,
																	enemy_->fb.touch_marker);
					if (!look_marker)
					{
						look_marker = SightMarker(self->fb.touch_marker, enemy_->fb.touch_marker,
													1000.0f, 40.0f);
					}

					if (look_marker)
					{
						goalentity_marker = look_marker;
					}
				}
			}
		}
	}

	// FIXME: what is this doing?  Opponent has GL, close & firing... then do nothing?
	//  Picking a grenade-spam location/direction?  But it's checking if opponent has GL...
	if (isDuel())
	{
		gedict_t *enemy_ = &g_edicts[self->s.v.enemy];
		qbool enemy_has_gl = HasItem(enemy_, IT_GRENADE_LAUNCHER) && enemy_->s.v.ammo_rockets > 6;

		if (enemy_has_gl)
		{
			if ((self->s.v.origin[2] + 18) < (enemy_->s.v.absmin[2] + enemy_->s.v.view_ofs[2]))
			{
				vec3_t diff;
				VectorSubtract(self->s.v.origin, enemy_->s.v.origin, diff);
				if (vlen(diff) < 200)
				{
					if (enemy_->s.v.button0 && enemy_->fb.touch_marker)
					{
						gedict_t *look_marker = HigherSightFromFunction(self->fb.touch_marker,
																		enemy_->fb.touch_marker);
						if (!look_marker)
						{
							look_marker = SightMarker(self->fb.touch_marker,
														enemy_->fb.touch_marker, 1000.0f, 40.0f);
						}

						if (look_marker)
						{
							goalentity_marker = look_marker;
						}
					}
				}
			}
		}
	}

	rocket_alert = CheckForRocketEnemyAim(self);
	normalize(self->s.v.velocity, player_direction);
	self->fb.be_quiet = (self->s.v.enemy && &g_edicts[self->s.v.enemy] != self->fb.look_object
			&& !self->fb.allowedMakeNoise);

	new_linked_marker = self->fb.linked_marker;

	PathScoringLogic(self->fb.goal_respawn_time, self->fb.be_quiet, self->fb.skill.lookahead_time,
						self->fb.path_normal_, self->s.v.origin, player_direction,
						self->fb.touch_marker, goalentity_marker, rocket_alert,
						rocket_jump_routes_allowed, trace_bprint, self, &best_score,
						&new_linked_marker, &new_path_state, &new_angle_hint, &new_rj_delay,
						new_rj_angles);
	SetLinkedMarker(self, new_linked_marker, "ProcNewLinked(std)");

	// "check if fully on lift - if not then continue moving to linked_marker_"
	if (OnLift(self))
	{
		return;
	}

	if (self->fb.state & WAIT)
	{
		BotWaitLogic(self, &new_path_state);
	}

	// FIXME: Map specific waiting points
	if (!self->fb.debug_path)
	{
		if (streq(g_globalvars.mapname, "dm3"))
		{
			DM3CampLogic();
		}
		else if (streq(g_globalvars.mapname, "dm4"))
		{
			DM4CampLogic();
		}
		else if (streq(g_globalvars.mapname, "dm6"))
		{
			DM6CampLogic();
		}
	}

	self->fb.path_state = new_path_state;
	self->fb.angle_hint = new_angle_hint;
	self->fb.rocketJumpFrameDelay = new_rj_delay;
	self->fb.rocketJumpAngles[PITCH] = new_rj_angles[PITCH];
	self->fb.rocketJumpAngles[YAW] = new_rj_angles[YAW];
	self->fb.linked_marker_time = g_globalvars.time
			+ (self->fb.touch_marker == self->fb.linked_marker ? 0.3 : 5);
	self->fb.old_linked_marker = self->fb.touch_marker;

	// Logic past this point appears to be deciding what to look at...
	DM6MarkerTouchLogic(self, goalentity_marker);

	self->fb.state &= ~NOTARGET_ENEMY;
	if (((int) self->s.v.flags & FL_ONGROUND) && self->fb.wasinwater)
	{
		self->fb.wasinwater = false;
		self->fb.path_state &= ~WATERJUMP_;
		self->fb.state &= ~NOTARGET_ENEMY;
	}

	if (self->fb.path_state & WATERJUMP_)
	{
		self->fb.wasinwater = true;
		self->fb.state |= NOTARGET_ENEMY;
		self->fb.look_object = self->fb.linked_marker;
		SetJumpFlag(self, true, "WaterJump");
		self->fb.waterjumping = true;
		self->fb.arrow_time = g_globalvars.time + 0.02;

		return;
	}

	// FIXME: Map-specific
	if (DM6LookAtDoor(self) || LookingAtPlayer(self))
	{
		return;
	}

	// When treading water, look at the next waypoint
	if ((self->s.v.waterlevel == 2) || (self->s.v.waterlevel == 1))
	{
		self->fb.look_object = self->fb.linked_marker;
		return;
	}

	if (PredictionShotLogic(self, goalentity_marker))
	{
		return;
	}

	// Either look at the next marker or the marker after that...
	if (self->fb.linked_marker)
	{
		vec3_t dir_look =
			{ 0 };

		if (self->fb.touch_marker != self->fb.linked_marker)
		{
			vec3_t diff, linked_marker_origin;

			VectorAdd(self->fb.linked_marker->s.v.absmin, self->fb.linked_marker->s.v.view_ofs,
						linked_marker_origin);
			VectorSubtract(linked_marker_origin, self->s.v.origin, diff);
			normalize(diff, dir_look);
		}

		// Average of velocity, look angle and direction to next marker
		{
			vec3_t temp;
			trap_makevectors(self->s.v.v_angle);
			VectorScale(self->s.v.velocity, inv_sv_maxspeed, temp);
			VectorAdd(temp, g_globalvars.v_forward, temp);
			VectorAdd(temp, dir_look, temp);
			normalize(temp, dir_look);
		}

		// Look at the next marker...
		{
			int i = 0;
			vec3_t linked_marker_origin =
				{ 0 };
			float best_score = PATH_SCORE_NULL;

			VectorAdd(self->fb.linked_marker->s.v.absmin, self->fb.linked_marker->s.v.view_ofs,
						linked_marker_origin);
			for (i = 0; i < NUMBER_PATHS; ++i)
			{
				from_marker = self->fb.linked_marker->fb.paths[i].next_marker;
				if (from_marker)
				{
					EvalLook(self, &best_score, dir_look, linked_marker_origin);
				}
			}
		}
	}

	self->fb.predict_shoot = false;
}

#endif
