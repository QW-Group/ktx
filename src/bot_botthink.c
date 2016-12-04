// Converted from .qc on 05/02/2016

#include "g_local.h"
#include "fb_globals.h"

void AMPHI2BotInLava(void);

// FIXME: Move to bot.skill
#define CHANCE_EVADE_DUEL 0.08
#define CHANCE_EVADE_NONDUEL 0.1

void Bot_Print_Thinking (void);
static void PeriodicAllClientLogic (void);
static void BotStopFiring (gedict_t* bot);
void BotsFireLogic (void);
void FrogbotEditorMarkerTouched (gedict_t* marker);

static void SetNextThinkTime(gedict_t* ent) {
	if (!((int)ent->s.v.flags & FL_ONGROUND)) {
		ent->fb.frogbot_nextthink += 0.15 + (0.015 * g_random ());
		if (PAST (frogbot_nextthink)) {
			ent->fb.frogbot_nextthink = g_globalvars.time + 0.16;
		}
	}
}

static void AvoidLookObjectsMissile(gedict_t* self) {
	gedict_t* rocket;

	self->fb.dodge_missile = NULL;
	if (self->fb.look_object && self->fb.look_object->ct == ctPlayer) {
		for (rocket = world; rocket = ez_find (rocket, "rocket"); ) {
			if (rocket->s.v.owner == EDICT_TO_PROG(self->fb.look_object)) {
				self->fb.dodge_missile = rocket;
				break;
			}
		}
	}
}

static void LookingAtEnemyLogic(gedict_t* self) {
	if (Visible_360(self, self->fb.look_object)) {
		if (self->fb.look_object == &g_edicts[self->s.v.enemy]) {
			self->fb.enemy_dist = VectorDistance (self->fb.look_object->s.v.origin, self->s.v.origin);
		}
		else if (PAST(enemy_time)) {
			ClearLookObject(self);
		}
	}
	else {
		ClearLookObject(self);
	}
}

static void NewlyPickedEnemyLogic() {
	gedict_t* goalentity_ = &g_edicts[self->s.v.goalentity];
	gedict_t* enemy_ = &g_edicts[self->s.v.enemy];

	if (self->s.v.goalentity == self->s.v.enemy) {
		if (Visible_360(self, goalentity_)) {
			LookEnemy(self, goalentity_);
		}
		else if (PAST(enemy_time)) {
			if (BotsPickBestEnemy(self)) {
				self->fb.goal_refresh_time = 0;
			}
		}
	}
	else {
		if (Visible_infront(self, enemy_)) {
			LookEnemy(self, enemy_);
		}
		else if (PAST(enemy_time)) {
			BotsPickBestEnemy(self);
		}
	}
}

static void TargetEnemyLogic(gedict_t* self) {
	self->fb.dodge_missile = NULL;

	if (!(self->fb.state & NOTARGET_ENEMY)) {
		if (self->fb.look_object && self->fb.look_object->ct == ctPlayer) {
			// Interesting - they only avoid missiles from players they are looking at?
			AvoidLookObjectsMissile(self);

			LookingAtEnemyLogic(self);
		}
		else if (self->s.v.enemy) {
			NewlyPickedEnemyLogic();
		}
		else {
			BotsPickBestEnemy(self);
		}
	}
}

static void BotDodgeMovement(gedict_t* self, vec3_t dir_move, float dodge_factor) {
	if (dodge_factor) {
		if (dodge_factor < 0) {
			++dodge_factor;
		}
		else {
			--dodge_factor;
		}
		trap_makevectors(self->s.v.v_angle);
		VectorMA(dir_move, g_random() * self->fb.skill.dodge_amount * dodge_factor, g_globalvars.v_right, dir_move);
	}
}

static void BotOnGroundMovement(gedict_t* self, vec3_t dir_move) {
	float dodge_factor = 0;

	if ((int)self->s.v.flags & FL_ONGROUND) {
		if (!(self->fb.path_state & NO_DODGE)) {
			// Dodge a rocket our enemy is firing at us
			if (self->fb.dodge_missile) {
				if (PROG_TO_EDICT(self->fb.dodge_missile->s.v.owner)->ct == ctPlayer) {
					vec3_t rel_pos;

					VectorSubtract(self->s.v.origin, self->fb.dodge_missile->s.v.origin, rel_pos);
					if (DotProduct(rel_pos, self->fb.dodge_missile->fb.missile_forward) > 0.7071067) {
						vec3_t temp;
						normalize(rel_pos, temp);
						dodge_factor = DotProduct(temp, self->fb.dodge_missile->fb.missile_right);
					}
				}
				else {
					self->fb.dodge_missile = NULL;
				}
			}

			// Not dodging a missile, dodge away from the player instead
			if (self->fb.look_object && self->fb.look_object->ct == ctPlayer) {
				if (!dodge_factor) {
					vec3_t rel_pos;

					VectorSubtract (self->s.v.origin, self->fb.look_object->s.v.origin, rel_pos);
					trap_makevectors(self->fb.look_object->s.v.v_angle);
					if (DotProduct(rel_pos, g_globalvars.v_forward) > 0) {
						vec3_t temp;
						normalize(rel_pos, temp);
						dodge_factor = DotProduct(temp, g_globalvars.v_right);
					}
				}
			}

			BotDodgeMovement(self, dir_move, dodge_factor);
		}
	}

	// If we're not in water, cannot have vertical direction (think of markers heading up stairs)
	if (self->s.v.waterlevel <= 1) {
		dir_move[2] = 0;
	}
}

static void BotMoveTowardsLinkedMarker(gedict_t* self, vec3_t dir_move) {
	vec3_t temp;
	gedict_t* goalentity_ = &g_edicts[self->s.v.goalentity];
	gedict_t* linked = self->fb.linked_marker;
	qbool onGround = ((int)self->s.v.flags & FL_ONGROUND);
	qbool curlJump = ((int)self->fb.path_state & BOTPATH_CURLJUMP_HINT);

	VectorAdd(linked->s.v.absmin, linked->s.v.view_ofs, temp);
	VectorSubtract(temp, self->s.v.origin, temp);
	normalize(temp, dir_move);

	if (curlJump && (onGround || self->s.v.velocity[2] > 0)) {
		vec3_t up = { 0, 0, 1 };

		if (self->isBot && self->fb.debug_path) {
			G_bprint (PRINT_HIGH, "%3.2f: Moving %3d > %3d, dir %3.1f %3.1f %3.1f\n", g_globalvars.time, self->fb.touch_marker->fb.index + 1, self->fb.linked_marker->fb.index + 1, PASSVEC3 (dir_move));
		}

		RotatePointAroundVector (dir_move, up, dir_move, self->fb.angle_hint);

		if (self->isBot && self->fb.debug_path) {
			G_bprint (PRINT_HIGH, "%3.2f: Rotating %d, %3.1f %3.1f %3.1f\n", g_globalvars.time, self->fb.angle_hint, PASSVEC3 (dir_move));
		}
	}

	if (self->isBot && self->fb.debug_path) {
		//G_bprint (PRINT_HIGH, "%3.2f: Moving %3d > %3d, dir %3.1f %3.1f %3.1f\n", g_globalvars.time, self->fb.touch_marker->fb.index + 1, self->fb.linked_marker->fb.index + 1, PASSVEC3 (dir_move));
	}

	if (self->fb.path_state & DELIBERATE_BACKUP) {
		if (linked->fb.arrow_time > g_globalvars.time) {
			VectorInverse (dir_move);
		}
		else {
			self->fb.path_state &= ~DELIBERATE_BACKUP;
		}
	}
	else if (linked == self->fb.touch_marker) {
		if (goalentity_ == self->fb.touch_marker) {
			if (WaitingToRespawn(self->fb.touch_marker)) {
				VectorClear(dir_move);
			}
		}
		else {
			VectorClear(dir_move);
		}
	}
}

// Called when the bot has a touch marker set
static void BotTouchMarkerLogic() {
	TargetEnemyLogic(self);

	if (PAST(goal_refresh_time)) {
		UpdateGoal(self);
	}

	if (PAST(linked_marker_time)) {
		self->fb.old_linked_marker = NULL;
	}

	if (self->fb.old_linked_marker != self->fb.touch_marker) {
		ProcessNewLinkedMarker(self);
	}

	if (FUTURE(arrow_time)) {
		if (self->isBot && self->fb.debug_path)
			G_bprint (PRINT_HIGH, "%3.2f: arrow_time is %3.2f\n", g_globalvars.time, self->fb.arrow_time);
		if (FUTURE(arrow_time2)) {
			if (g_random() < 0.5) {
				SetLinkedMarker (self, self->fb.touch_marker, "BotTouchMarkerLogic");
				self->fb.old_linked_marker = self->fb.linked_marker;
				self->fb.path_state = 0;
				self->fb.linked_marker_time = g_globalvars.time + 0.3;
			}
		}
	}
	else {
		vec3_t dir_move;

		BotMoveTowardsLinkedMarker(self, dir_move);
		BotOnGroundMovement(self, dir_move);

		SetDirectionMove (self, dir_move, "OnGround");
	}

	SelectWeapon();
}

// Called when a human player touches a marker
static void HumanTouchMarkerLogic(void) {
	if (PAST(enemy_time)) {
		BotsPickBestEnemy(self);
	}

	if (FrogbotOptionEnabled (FB_OPTION_EDITOR_MODE)) {
		FrogbotEditorMarkerTouched (self->fb.touch_marker);
	}
}

void BotPathCheck (gedict_t* self, gedict_t* touch_marker)
{
	// FIXME: This should be when they touch the marker, not when linking to it
	if (self->fb.debug_path && self->fb.fixed_goal == touch_marker) {
		G_bprint (2, "at goal, path complete.  %4.3f seconds\n", g_globalvars.time - self->fb.debug_path_start);
		self->fb.fixed_goal = NULL;
		self->fb.debug_path = false;
		self->fb.debug_path_start = 0;
		cvar_fset ("k_fb_debug", 0);
	}
}

static void PeriodicAllClientLogic(void)
{
	SetNextThinkTime(self);

	if (PAST(weapon_refresh_time)) {
		FrogbotSetFirepower(self);
	}

	// If we haven't touched a marker in a while, find closest marker
	if (PAST(touch_marker_time)) {
		SetMarker(self, LocateMarker(self->s.v.origin));
	}

	if (self->fb.touch_marker) {
		BotPathCheck (self, self->fb.touch_marker);

		if (self->fb.state & AWARE_SURROUNDINGS) {
			if (self->isBot) {
				BotTouchMarkerLogic();
			}
			else {
				HumanTouchMarkerLogic();
			}
		}
		else {
			self->fb.goal_refresh_time = 0;
			self->fb.state |= AWARE_SURROUNDINGS;
			self->fb.old_linked_marker = (self->isBot ? NULL : self->fb.old_linked_marker);
		}
	}
}

void BotEvadeLogic(gedict_t* self) {
	gedict_t* enemy_ = &g_edicts[self->s.v.enemy];

	self->fb.bot_evade = false;
	if (deathmatch <= 3 && !isRA()) {
		if (isDuel() && g_random() < CHANCE_EVADE_DUEL) {
			if ((self->s.v.origin[2] + 18) > (enemy_->s.v.absmin[2] + enemy_->s.v.view_ofs[2])) {
				if ((int)self->s.v.items & IT_ROCKET_LAUNCHER && self->s.v.ammo_rockets > 4) {
					if (!self->s.v.waterlevel) {
						self->fb.bot_evade = (qbool) (self->s.v.health > 70) && (self->s.v.armorvalue > 100) && !self->fb.enemy_visible;
					}
				}
			}
		}
		else if (! isDuel() && g_random() < CHANCE_EVADE_NONDUEL) {
			if ((self->s.v.origin[2] + 18) > (enemy_->s.v.absmin[2] + enemy_->s.v.view_ofs[2])) {
				if (((int)self->s.v.items & IT_ROCKET_LAUNCHER) || ((int)self->s.v.items & IT_LIGHTNING)) {
					if ((self->s.v.ammo_cells >= 20) || (self->s.v.ammo_rockets > 3)) {
						if (!self->s.v.waterlevel) {
							if ((self->s.v.health > 70) && (self->s.v.armorvalue > 90)) {
								self->fb.bot_evade = (qbool) (!((int)self->s.v.items & (IT_INVULNERABILITY | IT_INVISIBILITY | IT_QUAD)));
							}
						}
					}
				}
			}
		}
	}
}

// Logic that gets called for every player
void BotsThinkTime(gedict_t* self) {
	self->fb.jumping = false;

	if (PAST(frogbot_nextthink)) {
		PeriodicAllClientLogic();

		if (self->isBot) {
			CheckCombatJump();
			AMPHI2BotInLava();
		}
	}
}

// Sets a client's last marker
void SetMarker(gedict_t* client, gedict_t* marker) {
	client->fb.touch_distance = 0;
	client->fb.touch_marker = marker;
	client->fb.Z_ = marker ? marker->fb.Z_ : 0;
	client->fb.touch_marker_time = g_globalvars.time + 5;
}

