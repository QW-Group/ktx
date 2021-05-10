/*  Copyright (C) 1996-1997  Id Software, Inc.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

 See file, 'COPYING', for details.
 */

#include "g_local.h"

/*

 .enemy
 Will be world if not currently angry at anyone.

 .movetarget
 The next path spot to walk toward.  If .enemy, ignore .movetarget.
 When an enemy is killed, the monster will try to return to its path.

 .huntt_ime
 Set to time + something when the player is in sight, but movement straight for
 him is blocked.  This causes the monster to use wall following code for
 movement direction instead of sighting on the player.

 .ideal_yaw
 A yaw angle of the intended direction, which will be turned towards at up
 to 45 deg / state.  If the enemy is in view and hunt_time is not active,
 this will be the exact line towards the enemy.

 .pausetime
 A monster will leave its stand state and head towards its .movetarget when
 time > .pausetime.

 walkmove(angle, speed) primitive is all or nothing
 */

//
// globals
//
// when a monster becomes angry at a player, that monster will be used
// as the sight target the next frame so that monsters near that one
// will wake up even if they wouldn't have noticed the player
//
static gedict_t *sight_entity;

static float sight_entity_time;

static float movedist;

float enemy_vis, enemy_infront, enemy_range;
float enemy_yaw;

//============================================================================

/*
 =============
 range

 returns the range catagorization of an entity reletive to self
 0	melee range, will become hostile even if back is turned
 1	visibility and infront, or visibility and show hostile
 2	infront and show hostile
 3	only triggered by damage
 =============
 */
float range(gedict_t *targ)
{
	vec3_t spot1, spot2, org;
	float r;

	VectorAdd(self->s.v.origin, self->s.v.view_ofs, spot1);
	VectorAdd(targ->s.v.origin, targ->s.v.view_ofs, spot2);
	VectorSubtract(spot1, spot2, org);

	r = vlen(org);
	if (r < 120)
	{
		return RANGE_MELEE;
	}

	if (r < 500)
	{
		return RANGE_NEAR;
	}

	if (r < 1000)
	{
		return RANGE_MID;
	}

	// so monsters notice player father/faster in bloodfest mode.
	if (k_bloodfest)
	{
		return RANGE_MID;
	}

	return RANGE_FAR;
}

/*
 =============
 visible

 returns 1 if the entity is visible to self, even if not infront ()
 =============
 */
float visible(gedict_t *targ)
{
	vec3_t spot1, spot2;

	VectorAdd(self->s.v.origin, self->s.v.view_ofs, spot1);
	VectorAdd(targ->s.v.origin, targ->s.v.view_ofs, spot2);

	traceline(PASSVEC3(spot1), PASSVEC3(spot2), true, self); // see through other monsters

	if (g_globalvars.trace_inopen && g_globalvars.trace_inwater)
	{
		return false;			// sight line crossed contents
	}

	if (g_globalvars.trace_fraction == 1)
	{
		return true;
	}

	return false;
}

/*
 =============
 infront

 returns 1 if the entity is in front (in sight) of self
 =============
 */
float infront(gedict_t *targ)
{
	vec3_t vec;
	float dot;

	trap_makevectors(self->s.v.angles);
	VectorSubtract(targ->s.v.origin, self->s.v.origin, vec);
	VectorNormalize(vec);
	dot = DotProduct(vec, g_globalvars.v_forward);

	if (dot > 0.3)
	{
		return true;
	}

	return false;
}

//============================================================================

/*

 in nightmare mode, all attack_finished times become 0
 some monsters refire twice automatically

 */
void SUB_AttackFinished(float normal)
{
	self->cnt = 0;		// refire count for nightmare
	if (skill != 3)
	{
		self->attack_finished = g_globalvars.time + normal;
	}
}

void SUB_CheckRefire(func_t thinkst)
{
	if (skill != 3)
	{
		return;
	}

	if (self->cnt == 1)
	{
		return;
	}

	if (!visible(PROG_TO_EDICT(self->s.v.enemy)))
	{
		return;
	}

	self->cnt = 1;
	self->think = thinkst;
}

//============================================================================

void HuntTarget()
{
	vec3_t tmpv;

	self->s.v.goalentity = self->s.v.enemy;
	self->think = (func_t) self->th_run;
	VectorSubtract(PROG_TO_EDICT( self->s.v.enemy )->s.v.origin, self->s.v.origin, tmpv);
	self->s.v.ideal_yaw = vectoyaw(tmpv);
	self->s.v.nextthink = g_globalvars.time + 0.1;
	SUB_AttackFinished(1);	// wait a while before first attack
}

void SightSound()
{
	if (streq(self->classname, "monster_ogre"))
	{
		sound(self, CHAN_VOICE, "ogre/ogwake.wav", 1, ATTN_NORM);
	}
	else if (streq(self->classname, "monster_knight"))
	{
		sound(self, CHAN_VOICE, "knight/ksight.wav", 1, ATTN_NORM);
	}
	else if (streq(self->classname, "monster_shambler"))
	{
		sound(self, CHAN_VOICE, "shambler/ssight.wav", 1, ATTN_NORM);
	}
	else if (streq(self->classname, "monster_demon1"))
	{
		sound(self, CHAN_VOICE, "demon/sight2.wav", 1, ATTN_NORM);
	}
	else if (streq(self->classname, "monster_wizard"))
	{
		sound(self, CHAN_VOICE, "wizard/wsight.wav", 1, ATTN_NORM);
	}
	else if (streq(self->classname, "monster_zombie"))
	{
		sound(self, CHAN_VOICE, "zombie/z_idle.wav", 1, ATTN_NORM);
	}
	else if (streq(self->classname, "monster_dog"))
	{
		sound(self, CHAN_VOICE, "dog/dsight.wav", 1, ATTN_NORM);
	}
	else if (streq(self->classname, "monster_hell_knight"))
	{
		sound(self, CHAN_VOICE, "hknight/sight1.wav", 1, ATTN_NORM);
	}
	else if (streq(self->classname, "monster_tarbaby"))
	{
		sound(self, CHAN_VOICE, "blob/sight1.wav", 1, ATTN_NORM);
	}
	else if (streq(self->classname, "monster_vomit"))
	{
		sound(self, CHAN_VOICE, "vomitus/v_sight1.wav", 1, ATTN_NORM);
	}
	else if (streq(self->classname, "monster_enforcer"))
	{
		float rsnd = Q_rint(g_random() * 3);

		if (rsnd == 1)
		{
			sound(self, CHAN_VOICE, "enforcer/sight1.wav", 1, ATTN_NORM);
		}
		else if (rsnd == 2)
		{
			sound(self, CHAN_VOICE, "enforcer/sight2.wav", 1, ATTN_NORM);
		}
		else if (rsnd == 0)
		{
			sound(self, CHAN_VOICE, "enforcer/sight3.wav", 1, ATTN_NORM);
		}
		else
		{
			sound(self, CHAN_VOICE, "enforcer/sight4.wav", 1, ATTN_NORM);
		}
	}
	else if (streq(self->classname, "monster_army"))
	{
		sound(self, CHAN_VOICE, "soldier/sight1.wav", 1, ATTN_NORM);
	}
	else if (streq(self->classname, "monster_shalrath"))
	{
		sound(self, CHAN_VOICE, "shalrath/sight.wav", 1, ATTN_NORM);
	}
}

void FoundTarget()
{
	if (PROG_TO_EDICT(self->s.v.enemy)->ct == ctPlayer)
	{	// let other monsters see this monster for a while
		sight_entity = self;
		sight_entity_time = g_globalvars.time;
	}

	self->show_hostile = g_globalvars.time + 1;		// wake up other monsters

	SightSound();
	HuntTarget();
}

/*
 ===========
 FindTarget

 Self is currently not attacking anything, so try to find a target

 Returns true if an enemy was sighted

 When a player fires a missile, the point of impact becomes a fakeplayer so
 that monsters that see the impact will respond as if they had seen the
 player.

 To avoid spending too much time, only a single client (or fakeclient) is
 checked each frame.  This means multi player games will have slightly
 slower noticing monsters.
 ============
 */
float FindTarget()
{
	gedict_t *client = NULL;

// if the first spawnflag bit is set, the monster will only wake up on
// really seeing the player, not another monster getting angry

// spawnflags & 3 is a big hack, because zombie crucified used the first
// spawn flag prior to the ambush flag, and I forgot about it, so the second
// spawn flag works as well
	if (((sight_entity_time + 0.1) >= g_globalvars.time) && !((int)self->s.v.spawnflags & 3))
	{
		client = sight_entity; // NOTE: may be NULL, so be careful

		if (!client || (client == world))
		{
			return false;
		}

		if (client->s.v.enemy == self->s.v.enemy)
		{
			return false; // not sure I understand this, both have same enemy?
		}
	}
	else
	{
		client = checkclient();

		if (!client || (client == world))
		{
			return false;	// current check entity isn't in PVS
		}
	}

	if (client == PROG_TO_EDICT(self->s.v.enemy))
	{
		return false;
	}

	if ((int)client->s.v.flags & FL_NOTARGET)
	{
		return false;
	}

	if ((int)client->s.v.items & IT_INVISIBILITY)
	{
		return false;
	}

	// in bloodfest mode monsters spot players always.
	if (!k_bloodfest)
	{
		float r = range(client);

		if (r == RANGE_FAR)
		{
			return false;
		}

		if (!visible(client))
		{
			return false;
		}

		if (r == RANGE_NEAR)
		{
			if ((client->show_hostile < g_globalvars.time) && !infront(client))
			{
				return false;
			}
		}
		else if (r == RANGE_MID)
		{
			if ( /* client->show_hostile < g_globalvars.time || */!infront(client))
			{
				return false;
			}
		}
	}

//
// got one
//
	self->s.v.enemy = EDICT_TO_PROG(client);

	if ((PROG_TO_EDICT(self->s.v.enemy)->ct != ctPlayer)
			|| ((int)PROG_TO_EDICT(self->s.v.enemy)->s.v.flags & FL_NOTARGET))
	{
		self->s.v.enemy = PROG_TO_EDICT(self->s.v.enemy)->s.v.enemy;

		if ((PROG_TO_EDICT(self->s.v.enemy)->ct != ctPlayer)
				|| ((int)PROG_TO_EDICT( self->s.v.enemy )->s.v.flags & FL_NOTARGET))
		{
			self->s.v.enemy = EDICT_TO_PROG(world);

			return false;
		}
	}

	FoundTarget();

	return true;
}

//=============================================================================

/*
 =============
 GetMadAtAttacker

 T_Damage calls this when a monster is hurt
 =============
 */
void GetMadAtAttacker(gedict_t *attacker)
{
	if (!attacker || (attacker == world))
	{
		return; // ignore world attacks
	}

	if (k_bloodfest && (attacker->ct != ctPlayer))
	{
		return; // in bloodfest mode get mad only on players.
	}

	if (attacker == self)
	{
		return; // do not mad on self.
	}

	if (attacker == PROG_TO_EDICT(self->s.v.enemy))
	{
		return; // alredy mad on this.
	}

	// get mad unless of the same class (except for soldiers)
	if (streq(self->classname, attacker->classname) && strneq(self->classname, "monster_army"))
	{
		return;
	}

	// OK, we are MAD!

	// remember current enemy if it was "player enemy", later we restore it
	if (PROG_TO_EDICT(self->s.v.enemy)->ct == ctPlayer)
	{
		self->oldenemy = PROG_TO_EDICT(self->s.v.enemy);
	}

	// set new enemy
	self->s.v.enemy = EDICT_TO_PROG(attacker);

	FoundTarget();
}

//=============================================================================

/*
 =============
 ai_melee

 =============
 */
void ai_melee()
{
	vec3_t delta;
	float ldmg;

	if (!self->s.v.enemy)
	{
		return;		// removed before stroke
	}

	VectorSubtract(PROG_TO_EDICT( self->s.v.enemy )->s.v.origin, self->s.v.origin, delta);

	if (vlen(delta) > 60)
	{
		return;
	}

	ldmg = (g_random() + g_random() + g_random()) * 3;
	PROG_TO_EDICT( self->s.v.enemy )->deathtype = dtSQUISH; // FIXME
	T_Damage(PROG_TO_EDICT(self->s.v.enemy), self, self, ldmg);
}

void ai_melee_side()
{
	vec3_t delta;
	float ldmg;

	if (!self->s.v.enemy)
	{
		return;		// removed before stroke
	}

	VectorSubtract(PROG_TO_EDICT( self->s.v.enemy )->s.v.origin, self->s.v.origin, delta);

	if (vlen(delta) > 60)
	{
		return;
	}

	if (!CanDamage(PROG_TO_EDICT(self->s.v.enemy), self))
	{
		return;
	}

	ldmg = (g_random() + g_random() + g_random()) * 3;
	PROG_TO_EDICT( self->s.v.enemy )->deathtype = dtSQUISH; // FIXME
	T_Damage(PROG_TO_EDICT(self->s.v.enemy), self, self, ldmg);
}

//=============================================================================

void ai_forward(float dist)
{
	walkmove(self, self->s.v.angles[1], dist);
}

void ai_back(float dist)
{
	walkmove(self, self->s.v.angles[1] + 180, dist);
}

/*
 =============
 ai_pain

 stagger back a bit
 =============
 */
void ai_pain(float dist)
{
	ai_back(dist);
	/*
	 float	away;

	 away = anglemod (vectoyaw (self.origin - self.enemy.origin)
	 + 180*(g_random()- 0.5) );

	 walkmove (away, dist);
	 */
}

/*
 =============
 ai_painforward

 stagger back a bit
 =============
 */
void ai_painforward(float dist)
{
	walkmove(self, self->s.v.ideal_yaw, dist);
}

/*
 =============
 ai_walk

 The monster is walking its beat
 =============
 */
void ai_walk(float dist)
{
	movedist = dist;

	if (streq(self->classname, "monster_dragon"))
	{
		movetogoal(dist);

		return;
	}

	// check for noticing a player
	if (FindTarget())
	{
		return;
	}

	movetogoal(dist);
}

/*
 =============
 ai_stand

 The monster is staying in one place for a while, with slight angle turns
 =============
 */
void ai_stand()
{
	if (FindTarget())
	{
		return;
	}

	if (g_globalvars.time > self->pausetime)
	{
		if (self->th_walk)
		{
			self->th_walk();
		}

		return;
	}

// change angle slightly

}

/*
 =============
 ai_turn

 don't move, but turn towards ideal_yaw
 =============
 */
void ai_turn()
{
	if (FindTarget())
	{
		return;
	}

	changeyaw(self);
}

/*
 ============
 FacingIdeal

 ============
 */
float FacingIdeal()
{
	float delta;

	delta = anglemod(self->s.v.angles[1] - self->s.v.ideal_yaw);

	if ((delta > 45) && (delta < 315))
	{
		return false;
	}

	return true;
}

//=============================================================================

/*
 =============
 ai_face

 Stay facing the enemy
 =============
 */
void ai_face()
{
	vec3_t tmpv;

	VectorSubtract(PROG_TO_EDICT( self->s.v.enemy )->s.v.origin, self->s.v.origin, tmpv);

	self->s.v.ideal_yaw = vectoyaw(tmpv);
	changeyaw(self);
}

/*
 =============
 ai_charge

 The monster is in a melee attack, so get as close as possible to .enemy
 =============
 */

void ai_charge(float d)
{
	if (k_bloodfest)
	{
		if ((int)self->s.v.flags & FL_SWIM)
		{
			d *= 5; // let fish swim faster in bloodfest mode.
		}
		else if (self->bloodfest_boss)
		{
			d *= 2; // let boss move faster
		}
	}

	ai_face();
	movetogoal(d);		// done in C code...
}

void ai_charge_side()
{
	vec3_t tmpv;
	float heading;

	// aim to the left of the enemy for a flyby

	VectorSubtract(PROG_TO_EDICT( self->s.v.enemy )->s.v.origin, self->s.v.origin, tmpv);
	self->s.v.ideal_yaw = vectoyaw(tmpv);
	changeyaw(self);

	trap_makevectors(self->s.v.angles);

	VectorMA( PROG_TO_EDICT( self->s.v.enemy )->s.v.origin, -30, g_globalvars.v_right, tmpv);
	VectorSubtract(tmpv, self->s.v.origin, tmpv);
	heading = vectoyaw(tmpv);

	walkmove(self, heading, 20);
}

//=============================================================================

/*
 ===========
 CheckAttack

 The player is in view, so decide to move or launch an attack
 Returns false if movement should continue
 ============
 */
float CheckAttack()
{
	vec3_t spot1, spot2;
	gedict_t *targ;
	float chance;

	targ = PROG_TO_EDICT(self->s.v.enemy);

	// see if any entities are in the way of the shot
	VectorAdd(self->s.v.origin, self->s.v.view_ofs, spot1);
	VectorAdd(targ->s.v.origin, targ->s.v.view_ofs, spot2);

	traceline(PASSVEC3(spot1), PASSVEC3(spot2), false, self);

	if (PROG_TO_EDICT(g_globalvars.trace_ent) != targ)
	{
		return false; // don't have a clear shot
	}

	if (g_globalvars.trace_inopen && g_globalvars.trace_inwater)
	{
		return false; // sight line crossed contents
	}

	if (enemy_range == RANGE_MELEE)
	{	// melee attack
		if (self->th_melee)
		{
			self->th_melee();

			return true;
		}
	}

	// missile attack
	if (!self->th_missile)
	{
		return false;
	}

	if (g_globalvars.time < self->attack_finished)
	{
		return false;
	}

	if (enemy_range == RANGE_FAR)
	{
		return false;
	}

	if (enemy_range == RANGE_MELEE)
	{
		chance = 0.9;
		self->attack_finished = 0;
	}
	else if (enemy_range == RANGE_NEAR)
	{
		if (self->th_melee)
		{
			chance = 0.2;
		}
		else
		{
			chance = 0.4;
		}
	}
	else if (enemy_range == RANGE_MID)
	{
		if (self->th_melee)
		{
			chance = 0.05;
		}
		else
		{
			chance = 0.1;
		}
	}
	else
		chance = 0;

	if (g_random() < chance)
	{
		if (self->th_missile)
		{
			self->th_missile();
		}

		SUB_AttackFinished(2 * g_random());

		return true;
	}

	return false;
}

float CheckAnyAttack()
{
	if (!enemy_vis)
	{
		return false;
	}

	if (streq(self->classname, "monster_dog"))
	{
		return DogCheckAttack();
	}

	if (streq(self->classname, "monster_army"))
	{
		return SoldierCheckAttack();
	}

	if (streq(self->classname, "monster_ogre"))
	{
		return OgreCheckAttack();
	}

	if (streq(self->classname, "monster_shambler"))
	{
		return ShamCheckAttack();
	}

	if (streq(self->classname, "monster_demon1"))
	{
		return DemonCheckAttack();
	}

	if (streq(self->classname, "monster_wizard"))
	{
		return WizardCheckAttack();
	}

	return CheckAttack();
}

/*
 =============
 ai_run_melee

 Turn and close until within an angle to launch a melee attack
 =============
 */
void ai_run_melee()
{
	self->s.v.ideal_yaw = enemy_yaw;
	changeyaw(self);

	if (FacingIdeal())
	{
		if (self->th_melee)
		{
			self->th_melee();
		}

		self->attack_state = AS_STRAIGHT;
	}
}

/*
 =============
 ai_run_missile

 Turn in place until within an angle to launch a missile attack
 =============
 */
void ai_run_missile()
{
	self->s.v.ideal_yaw = enemy_yaw;
	changeyaw(self);

	if (FacingIdeal())
	{
		if (self->th_missile)
		{
			self->th_missile();
		}

		self->attack_state = AS_STRAIGHT;
	}
}

/*
 =============
 ai_run_slide

 Strafe sideways, but stay at aproximately the same range
 =============
 */
void ai_run_slide()
{
	float ofs;

	self->s.v.ideal_yaw = enemy_yaw;
	changeyaw(self);

	if (self->lefty)
	{
		ofs = 90;
	}
	else
	{
		ofs = -90;
	}

	if (walkmove(self, self->s.v.ideal_yaw + ofs, movedist))
	{
		return;
	}

	self->lefty = 1 - self->lefty;

	walkmove(self, self->s.v.ideal_yaw - ofs, movedist);
}

/*
 =============
 ai_run

 The monster has an enemy it is trying to kill
 =============
 */
void ai_run(float dist)
{
	vec3_t tmpv;

	if (k_bloodfest)
	{
		if ((int)self->s.v.flags & FL_SWIM)
		{
			dist *= 5; // let fish swim faster in bloodfest mode.
		}
		else if (self->bloodfest_boss)
		{
			dist *= 2; // let boss move faster
		}
	}

	movedist = dist;

	// see if the enemy is dead
	if (ISDEAD(PROG_TO_EDICT(self->s.v.enemy))
			|| ((int)PROG_TO_EDICT(self->s.v.enemy)->s.v.flags & FL_NOTARGET))
	{
		self->s.v.enemy = EDICT_TO_PROG(world);

		// FIXME: look all around for other targets
		if (self->oldenemy && ISLIVE(self->oldenemy)
				&& !((int)self->oldenemy->s.v.flags & FL_NOTARGET))
		{
			self->s.v.enemy = EDICT_TO_PROG(self->oldenemy);
			HuntTarget();
		}
		else
		{
			if (!self->movetarget || (self->movetarget == world))
			{
				if (self->th_stand)
				{
					self->th_stand();
				}
			}
			else
			{
				if (self->th_walk)
				{
					self->th_walk();
				}
			}

			return;
		}
	}

	self->show_hostile = g_globalvars.time + 1;		// wake up other monsters

// check knowledge of enemy
	enemy_vis = visible(PROG_TO_EDICT(self->s.v.enemy));
	if (enemy_vis)
	{
		self->search_time = g_globalvars.time + 5; // does not search for enemy next 5 seconds
	}

// look for other coop players
	if (coop && self->search_time < g_globalvars.time)
	{
		if (FindTarget())
		{
			// this is fix for too frequent enemy sighting, required for bloodfest mode.
			if (!visible(PROG_TO_EDICT(self->s.v.enemy)))
			{
				self->search_time = g_globalvars.time + 5; // does not search for enemy next 5 seconds
			}

			return;
		}
	}

	enemy_infront = infront(PROG_TO_EDICT(self->s.v.enemy));
	enemy_range = range(PROG_TO_EDICT(self->s.v.enemy));
	VectorSubtract(PROG_TO_EDICT( self->s.v.enemy )->s.v.origin, self->s.v.origin, tmpv);
	enemy_yaw = vectoyaw(tmpv);

	if (self->attack_state == AS_MISSILE)
	{
		//dprint ("ai_run_missile\n");
		ai_run_missile();

		return;
	}

	if (self->attack_state == AS_MELEE)
	{
		//dprint ("ai_run_melee\n");
		ai_run_melee();

		return;
	}

	if (CheckAnyAttack())
	{
		return;					// beginning an attack
	}

	if (self->attack_state == AS_SLIDING)
	{
		ai_run_slide();

		return;
	}

	// head straight in
	movetogoal(dist);		// done in C code...
}
