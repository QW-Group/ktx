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
/*
 ==============================================================================

 DEMON

 ==============================================================================
 */

#include "g_local.h"

enum
{
	stand1,
	stand2,
	stand3,
	stand4,
	stand5,
	stand6,
	stand7,
	stand8,
	stand9,
	stand10,
	stand11,
	stand12,
	stand13,

	walk1,
	walk2,
	walk3,
	walk4,
	walk5,
	walk6,
	walk7,
	walk8,

	run1,
	run2,
	run3,
	run4,
	run5,
	run6,

	leap1,
	leap2,
	leap3,
	leap4,
	leap5,
	leap6,
	leap7,
	leap8,
	leap9,
	leap10,
	leap11,
	leap12,

	pain1,
	pain2,
	pain3,
	pain4,
	pain5,
	pain6,

	death1,
	death2,
	death3,
	death4,
	death5,
	death6,
	death7,
	death8,
	death9,

	attacka1,
	attacka2,
	attacka3,
	attacka4,
	attacka5,
	attacka6,
	attacka7,
	attacka8,
	attacka9,
	attacka10,
	attacka11,
	attacka12,
	attacka13,
	attacka14,
	attacka15,

};

void demon1_stand1();
void demon1_stand2();
void demon1_stand3();
void demon1_stand4();
void demon1_stand5();
void demon1_stand6();
void demon1_stand7();
void demon1_stand8();
void demon1_stand9();
void demon1_stand10();
void demon1_stand11();
void demon1_stand12();
void demon1_stand13();
void demon1_walk1();
void demon1_walk2();
void demon1_walk3();
void demon1_walk4();
void demon1_walk5();
void demon1_walk6();
void demon1_walk7();
void demon1_walk8();
void demon1_run1();
void demon1_run2();
void demon1_run3();
void demon1_run4();
void demon1_run5();
void demon1_run6();
void demon1_jump1();
void demon1_jump2();
void demon1_jump3();
void demon1_jump4();
void demon1_jump5();
void demon1_jump6();
void demon1_jump7();
void demon1_jump8();
void demon1_jump9();
void demon1_jump10();
void demon1_jump11();
void demon1_jump12();
void demon1_atta1();
void demon1_atta2();
void demon1_atta3();
void demon1_atta4();
void demon1_atta5();
void demon1_atta6();
void demon1_atta7();
void demon1_atta8();
void demon1_atta9();
void demon1_atta10();
void demon1_atta11();
void demon1_atta12();
void demon1_atta13();
void demon1_atta14();
void demon1_atta15();
void demon1_pain1();
void demon1_pain2();
void demon1_pain3();
void demon1_pain4();
void demon1_pain5();
void demon1_pain6();
void demon1_die1();
void demon1_die2();
void demon1_die3();
void demon1_die4();
void demon1_die5();
void demon1_die6();
void demon1_die7();
void demon1_die8();
void demon1_die9();

//============================================================================

ANIM(demon1_stand1, stand1, demon1_stand2; ai_stand();)
ANIM(demon1_stand2, stand2, demon1_stand3; ai_stand();)
ANIM(demon1_stand3, stand3, demon1_stand4; ai_stand();)
ANIM(demon1_stand4, stand4, demon1_stand5; ai_stand();)
ANIM(demon1_stand5, stand5, demon1_stand6; ai_stand();)
ANIM(demon1_stand6, stand6, demon1_stand7; ai_stand();)
ANIM(demon1_stand7, stand7, demon1_stand8; ai_stand();)
ANIM(demon1_stand8, stand8, demon1_stand9; ai_stand();)
ANIM(demon1_stand9, stand9, demon1_stand10; ai_stand();)
ANIM(demon1_stand10, stand10, demon1_stand11; ai_stand();)
ANIM(demon1_stand11, stand11, demon1_stand12; ai_stand();)
ANIM(demon1_stand12, stand12, demon1_stand13; ai_stand();)
ANIM(demon1_stand13, stand13, demon1_stand1; ai_stand();)

void _demon1_walk1(void)
{
	if (g_random() < 0.2)
	{
		sound(self, CHAN_VOICE, "demon/idle1.wav", 1, ATTN_IDLE);
	}

	ai_walk(8);
}
ANIM(demon1_walk1, walk1, demon1_walk2; _demon1_walk1();)
ANIM(demon1_walk2, walk2, demon1_walk3; ai_walk(6);)
ANIM(demon1_walk3, walk3, demon1_walk4; ai_walk(6);)
ANIM(demon1_walk4, walk4, demon1_walk5; ai_walk(7);)
ANIM(demon1_walk5, walk5, demon1_walk6; ai_walk(4);)
ANIM(demon1_walk6, walk6, demon1_walk7; ai_walk(6);)
ANIM(demon1_walk7, walk7, demon1_walk8; ai_walk(10);)
ANIM(demon1_walk8, walk8, demon1_walk1; ai_walk(10);)

void _demon1_run1(void)
{
	if (g_random() < 0.2)
	{
		sound(self, CHAN_VOICE, "demon/idle1.wav", 1, ATTN_IDLE);
	}

	ai_run(20);
}
ANIM(demon1_run1, run1, demon1_run2; _demon1_run1();)
ANIM(demon1_run2, run2, demon1_run3; ai_run(15);)
ANIM(demon1_run3, run3, demon1_run4; ai_run(36);)
ANIM(demon1_run4, run4, demon1_run5; ai_run(20);)
ANIM(demon1_run5, run5, demon1_run6; ai_run(15);)
ANIM(demon1_run6, run6, demon1_run1; ai_run(36);)

void Demon_JumpTouch()
{
	float ldmg;

	if (ISDEAD(self))
	{
		return;
	}

	if (other->s.v.takedamage)
	{
		if (vlen(self->s.v.velocity) > 400)
		{
			ldmg = 40 + 10 * g_random();
			other->deathtype = dtSQUISH; // FIXME
			T_Damage(other, self, self, ldmg);
		}
	}

	if (!checkbottom(self))
	{
		if ((int)self->s.v.flags & FL_ONGROUND)
		{	// jump randomly to not get hung up
			//dprint ("popjump\n");
			self->touch = (func_t) SUB_Null;
			self->think = (func_t) demon1_jump1;
			self->s.v.nextthink = g_globalvars.time + FRAMETIME;

//			self.velocity_x = (g_random() - 0.5) * 600;
//			self.velocity_y = (g_random() - 0.5) * 600;
//			self.velocity_z = 200;
//			self.flags = self.flags - FL_ONGROUND;
		}

		return;	// not on ground yet
	}

	self->touch = (func_t) SUB_Null;
	self->think = (func_t) demon1_jump11;
	self->s.v.nextthink = g_globalvars.time + FRAMETIME;
}

void _demon1_jump4(void)
{
	ai_face();

	self->touch = (func_t) Demon_JumpTouch;
	trap_makevectors(self->s.v.angles);
	self->s.v.origin[2] += 1; // FIXME: possibile stuck in walls, right?
	VectorScale(g_globalvars.v_forward, 600, self->s.v.velocity);
	self->s.v.velocity[2] += 250;
	self->s.v.flags = (int) self->s.v.flags & ~FL_ONGROUND;
}
void _demon1_jump10(void)
{
	// if three seconds pass, assume demon is stuck and jump again
	// FIXME: qqshka: is it actually working any how?
	self->s.v.nextthink = g_globalvars.time + 3;
}

ANIM(demon1_jump1, leap1,
		demon1_jump2; ai_face(); sound( self, CHAN_VOICE, "demon/djump.wav", 1, ATTN_NORM );)
ANIM(demon1_jump2, leap2, demon1_jump3; ai_face();)
ANIM(demon1_jump3, leap3, demon1_jump4; ai_face();)
ANIM(demon1_jump4, leap4, demon1_jump5; _demon1_jump4();)
ANIM(demon1_jump5, leap5, demon1_jump6;)
ANIM(demon1_jump6, leap6, demon1_jump7;)
ANIM(demon1_jump7, leap7, demon1_jump8;)
ANIM(demon1_jump8, leap8, demon1_jump9;)
ANIM(demon1_jump9, leap9, demon1_jump10;)
ANIM(demon1_jump10, leap10,
		demon1_jump1 /* NOTE it demon1_jump1 not demon1_jump11 */; _demon1_jump10();)
ANIM(demon1_jump11, leap11, demon1_jump12;)
ANIM(demon1_jump12, leap12, demon1_run1;)

void Demon_Melee(float side)
{
	float ldmg;
	vec3_t tmpv, tmpv2;

	ai_face();
	walkmove(self, self->s.v.ideal_yaw, 12);	// allow a little closing

	VectorSubtract(PROG_TO_EDICT(self->s.v.enemy)->s.v.origin, self->s.v.origin, tmpv);

	if (vlen(tmpv) > 100)
	{
		return;
	}

	if (!CanDamage(PROG_TO_EDICT(self->s.v.enemy), self))
	{
		return;
	}

	sound(self, CHAN_WEAPON, "demon/dhit2.wav", 1, ATTN_NORM);
	ldmg = 10 + 5 * g_random();
	PROG_TO_EDICT(self->s.v.enemy)->deathtype = dtSQUISH; // FIXME
	T_Damage(PROG_TO_EDICT(self->s.v.enemy), self, self, ldmg);

	trap_makevectors(self->s.v.angles);
	VectorMA(self->s.v.origin, 16, g_globalvars.v_forward, tmpv);
	VectorScale(g_globalvars.v_right, side, tmpv2);
	SpawnMeatSpray(tmpv, tmpv2);
}

ANIM(demon1_atta1, attacka1, demon1_atta2; ai_charge(4);)
ANIM(demon1_atta2, attacka2, demon1_atta3; ai_charge(0);)
ANIM(demon1_atta3, attacka3, demon1_atta4; ai_charge(0);)
ANIM(demon1_atta4, attacka4, demon1_atta5; ai_charge(1);)
ANIM(demon1_atta5, attacka5, demon1_atta6; ai_charge(2); Demon_Melee(200);)
ANIM(demon1_atta6, attacka6, demon1_atta7; ai_charge(1);)
ANIM(demon1_atta7, attacka7, demon1_atta8; ai_charge(6);)
ANIM(demon1_atta8, attacka8, demon1_atta9; ai_charge(8);)
ANIM(demon1_atta9, attacka9, demon1_atta10; ai_charge(4);)
ANIM(demon1_atta10, attacka10, demon1_atta11; ai_charge(2);)
ANIM(demon1_atta11, attacka11, demon1_atta12; Demon_Melee(-200);)
ANIM(demon1_atta12, attacka12, demon1_atta13; ai_charge(5);)
ANIM(demon1_atta13, attacka13, demon1_atta14; ai_charge(8);)
ANIM(demon1_atta14, attacka14, demon1_atta15; ai_charge(4);)
ANIM(demon1_atta15, attacka15, demon1_run1; ai_charge(4);)

ANIM(demon1_pain1, pain1, demon1_pain2)
ANIM(demon1_pain2, pain2, demon1_pain3)
ANIM(demon1_pain3, pain3, demon1_pain4)
ANIM(demon1_pain4, pain4, demon1_pain5)
ANIM(demon1_pain5, pain5, demon1_pain6)
ANIM(demon1_pain6, pain6, demon1_run1)

void demon1_pain(struct gedict_s *attacker, float damage)
{
	if (self->touch == (func_t)Demon_JumpTouch)
	{
		return;
	}

	if (self->pain_finished > g_globalvars.time)
	{
		return;
	}

	self->pain_finished = g_globalvars.time + 1;

	sound(self, CHAN_VOICE, "demon/dpain1.wav", 1, ATTN_NORM);

	if (g_random() * 200 > damage)
	{
		return;		// didn't flinch
	}

	demon1_pain1();
}

ANIM(demon1_die1, death1, demon1_die2; sound( self, CHAN_VOICE, "demon/ddeath.wav", 1, ATTN_NORM );)
ANIM(demon1_die2, death2, demon1_die3)
ANIM(demon1_die3, death3, demon1_die4)
ANIM(demon1_die4, death4, demon1_die5)
ANIM(demon1_die5, death5, demon1_die6)
ANIM(demon1_die6, death6, demon1_die7; self->s.v.solid = SOLID_NOT;)
ANIM(demon1_die7, death7, demon1_die8)
ANIM(demon1_die8, death8, demon1_die9)
ANIM(demon1_die9, death9, demon1_die9)

void demon_die()
{
	// check for gib
	if (self->s.v.health < -80)
	{
		sound(self, CHAN_VOICE, "player/udeath.wav", 1, ATTN_NORM);
		ThrowHead("progs/h_demon.mdl", self->s.v.health);
		ThrowGib("progs/gib1.mdl", self->s.v.health);
		ThrowGib("progs/gib1.mdl", self->s.v.health);
		ThrowGib("progs/gib1.mdl", self->s.v.health);

		self->s.v.nextthink = -1;

		return;
	}

	// regular death
	demon1_die1();
}

//==============================================================================

/*
 ==============
 CheckDemonMelee

 Returns true if a melee attack would hit right now
 ==============
 */
float CheckDemonMelee()
{
	if (enemy_range == RANGE_MELEE)
	{
		// FIXME: check canreach
		self->attack_state = AS_MELEE;

		return true;
	}

	return false;
}

/*
 ==============
 CheckDemonJump

 ==============
 */
float CheckDemonJump()
{
	vec3_t dist;
	float d;

	if ((self->s.v.origin[2] + self->s.v.mins[2])
			> (PROG_TO_EDICT(self->s.v.enemy)->s.v.origin[2]
					+ PROG_TO_EDICT(self->s.v.enemy)->s.v.mins[2]
					+ (0.75 * PROG_TO_EDICT(self->s.v.enemy)->s.v.size[2])))
	{
		return false;
	}

	if ((self->s.v.origin[2] + self->s.v.maxs[2])
			< (PROG_TO_EDICT(self->s.v.enemy)->s.v.origin[2]
					+ PROG_TO_EDICT(self->s.v.enemy)->s.v.mins[2]
					+ (0.25 * PROG_TO_EDICT(self->s.v.enemy)->s.v.size[2])))
	{
		return false;
	}

	VectorSubtract(PROG_TO_EDICT(self->s.v.enemy)->s.v.origin, self->s.v.origin, dist);
	dist[2] = 0;

	d = vlen(dist);

	if (d < 100)
	{
		return false;
	}

	if (d > 200)
	{
		if (g_random() < 0.9)
		{
			return false;
		}
	}

	return true;
}

float DemonCheckAttack()
{
	// if close enough for slashing, go for it
	if (CheckDemonMelee())
	{
		self->attack_state = AS_MELEE;

		return true;
	}

	if (CheckDemonJump())
	{
		self->attack_state = AS_MISSILE;

		return true;
	}

	return false;
}

//===========================================================================

/*QUAKED monster_demon1 (1 0 0) (-32 -32 -24) (32 32 64) Ambush

 */
void SP_monster_demon1()
{
	if (deathmatch)
	{
		ent_remove(self);

		return;
	}

	safe_precache_model("progs/demon.mdl");
	safe_precache_model("progs/h_demon.mdl");

	safe_precache_sound("demon/ddeath.wav");
	safe_precache_sound("demon/dhit2.wav");
	safe_precache_sound("demon/djump.wav");
	safe_precache_sound("demon/dpain1.wav");
	safe_precache_sound("demon/idle1.wav");
	safe_precache_sound("demon/sight2.wav");

	setsize(self, PASSVEC3(VEC_HULL2_MIN), PASSVEC3(VEC_HULL2_MAX));
	self->s.v.health = 300;

	self->th_stand = demon1_stand1;
	self->th_walk = demon1_walk1;
	self->th_run = demon1_run1;
	self->th_die = demon_die;
	self->th_melee = demon1_atta1;
	self->th_missile = demon1_jump1;			// jump attack
	self->th_pain = demon1_pain;

	self->th_respawn = SP_monster_demon1;

	walkmonster_start("progs/demon.mdl");
}
