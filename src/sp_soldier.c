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

 SOLDIER / PLAYER

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

	death1,
	death2,
	death3,
	death4,
	death5,
	death6,
	death7,
	death8,
	death9,
	death10,

	deathc1,
	deathc2,
	deathc3,
	deathc4,
	deathc5,
	deathc6,
	deathc7,
	deathc8,
	deathc9,
	deathc10,
	deathc11,

	load1,
	load2,
	load3,
	load4,
	load5,
	load6,
	load7,
	load8,
	load9,
	load10,
	load11,

	pain1,
	pain2,
	pain3,
	pain4,
	pain5,
	pain6,

	painb1,
	painb2,
	painb3,
	painb4,
	painb5,
	painb6,
	painb7,
	painb8,
	painb9,
	painb10,
	painb11,
	painb12,
	painb13,
	painb14,

	painc1,
	painc2,
	painc3,
	painc4,
	painc5,
	painc6,
	painc7,
	painc8,
	painc9,
	painc10,
	painc11,
	painc12,
	painc13,

	run1,
	run2,
	run3,
	run4,
	run5,
	run6,
	run7,
	run8,

	shoot1,
	shoot2,
	shoot3,
	shoot4,
	shoot5,
	shoot6,
	shoot7,
	shoot8,
	shoot9,

	prowl_1,
	prowl_2,
	prowl_3,
	prowl_4,
	prowl_5,
	prowl_6,
	prowl_7,
	prowl_8,
	prowl_9,
	prowl_10,
	prowl_11,
	prowl_12,
	prowl_13,
	prowl_14,
	prowl_15,
	prowl_16,
	prowl_17,
	prowl_18,
	prowl_19,
	prowl_20,
	prowl_21,
	prowl_22,
	prowl_23,
	prowl_24,

};

void army_stand1(void);
void army_stand2(void);
void army_stand3(void);
void army_stand4(void);
void army_stand5(void);
void army_stand6(void);
void army_stand7(void);
void army_stand8(void);
void army_walk1(void);
void army_walk2(void);
void army_walk3(void);
void army_walk4(void);
void army_walk5(void);
void army_walk6(void);
void army_walk7(void);
void army_walk8(void);
void army_walk9(void);
void army_walk10(void);
void army_walk11(void);
void army_walk12(void);
void army_walk13(void);
void army_walk14(void);
void army_walk15(void);
void army_walk16(void);
void army_walk17(void);
void army_walk18(void);
void army_walk19(void);
void army_walk20(void);
void army_walk21(void);
void army_walk22(void);
void army_walk23(void);
void army_walk24(void);
void army_run1(void);
void army_run2(void);
void army_run3(void);
void army_run4(void);
void army_run5(void);
void army_run6(void);
void army_run7(void);
void army_run8(void);
void army_atk1(void);
void army_atk2(void);
void army_atk3(void);
void army_atk4(void);
void army_atk5(void);
void army_atk6(void);
void army_atk7(void);
void army_atk8(void);
void army_atk9(void);
void army_pain1(void);
void army_pain2(void);
void army_pain3(void);
void army_pain4(void);
void army_pain5(void);
void army_pain6(void);
void army_painb1(void);
void army_painb2(void);
void army_painb3(void);
void army_painb4(void);
void army_painb5(void);
void army_painb6(void);
void army_painb7(void);
void army_painb8(void);
void army_painb9(void);
void army_painb10(void);
void army_painb11(void);
void army_painb12(void);
void army_painb13(void);
void army_painb14(void);
void army_painc1(void);
void army_painc2(void);
void army_painc3(void);
void army_painc4(void);
void army_painc5(void);
void army_painc6(void);
void army_painc7(void);
void army_painc8(void);
void army_painc9(void);
void army_painc10(void);
void army_painc11(void);
void army_painc12(void);
void army_painc13(void);
void army_die1(void);
void army_die2(void);
void army_die3(void);
void army_die4(void);
void army_die5(void);
void army_die6(void);
void army_die7(void);
void army_die8(void);
void army_die9(void);
void army_die10(void);
void army_cdie1(void);
void army_cdie2(void);
void army_cdie3(void);
void army_cdie4(void);
void army_cdie5(void);
void army_cdie6(void);
void army_cdie7(void);
void army_cdie8(void);
void army_cdie9(void);
void army_cdie10(void);
void army_cdie11(void);

//==============================================================================

ANIM(army_stand1, stand1, army_stand2; ai_stand();)
ANIM(army_stand2, stand2, army_stand3; ai_stand();)
ANIM(army_stand3, stand3, army_stand4; ai_stand();)
ANIM(army_stand4, stand4, army_stand5; ai_stand();)
ANIM(army_stand5, stand5, army_stand6; ai_stand();)
ANIM(army_stand6, stand6, army_stand7; ai_stand();)
ANIM(army_stand7, stand7, army_stand8; ai_stand();)
ANIM(army_stand8, stand8, army_stand1; ai_stand();)

void _army_walk1(void)
{
	if (g_random() < 0.2)
	{
		sound(self, CHAN_VOICE, "soldier/idle.wav", 1, ATTN_IDLE);
	}

	ai_walk(1);
}

ANIM(army_walk1, prowl_1, army_walk2; _army_walk1();)
ANIM(army_walk2, prowl_2, army_walk3; ai_walk(1);)
ANIM(army_walk3, prowl_3, army_walk4; ai_walk(1);)
ANIM(army_walk4, prowl_4, army_walk5; ai_walk(1);)
ANIM(army_walk5, prowl_5, army_walk6; ai_walk(2);)
ANIM(army_walk6, prowl_6, army_walk7; ai_walk(3);)
ANIM(army_walk7, prowl_7, army_walk8; ai_walk(4);)
ANIM(army_walk8, prowl_8, army_walk9; ai_walk(4);)
ANIM(army_walk9, prowl_9, army_walk10; ai_walk(2);)
ANIM(army_walk10, prowl_10, army_walk11; ai_walk(2);)
ANIM(army_walk11, prowl_11, army_walk12; ai_walk(2);)
ANIM(army_walk12, prowl_12, army_walk13; ai_walk(1);)
ANIM(army_walk13, prowl_13, army_walk14; ai_walk(0);)
ANIM(army_walk14, prowl_14, army_walk15; ai_walk(1);)
ANIM(army_walk15, prowl_15, army_walk16; ai_walk(1);)
ANIM(army_walk16, prowl_16, army_walk17; ai_walk(1);)
ANIM(army_walk17, prowl_17, army_walk18; ai_walk(3);)
ANIM(army_walk18, prowl_18, army_walk19; ai_walk(3);)
ANIM(army_walk19, prowl_19, army_walk20; ai_walk(3);)
ANIM(army_walk20, prowl_20, army_walk21; ai_walk(3);)
ANIM(army_walk21, prowl_21, army_walk22; ai_walk(2);)
ANIM(army_walk22, prowl_22, army_walk23; ai_walk(1);)
ANIM(army_walk23, prowl_23, army_walk24; ai_walk(1);)
ANIM(army_walk24, prowl_24, army_walk1; ai_walk(1);)

void _army_run1(void)
{
	if (g_random() < 0.2)
	{
		sound(self, CHAN_VOICE, "soldier/idle.wav", 1, ATTN_IDLE);
	}

	ai_run(11);
}

ANIM(army_run1, run1, army_run2; _army_run1();)
ANIM(army_run2, run2, army_run3; ai_run(15);)
ANIM(army_run3, run3, army_run4; ai_run(10);)
ANIM(army_run4, run4, army_run5; ai_run(10);)
ANIM(army_run5, run5, army_run6; ai_run(8);)
ANIM(army_run6, run6, army_run7; ai_run(15);)
ANIM(army_run7, run7, army_run8; ai_run(10);)
ANIM(army_run8, run8, army_run1; ai_run(8);)

void army_fire(void)
{
	vec3_t dir;
	gedict_t *en;

	ai_face();

	sound(self, CHAN_WEAPON, "soldier/sattck1.wav", 1, ATTN_NORM);

	// fire somewhat behind the player, so a dodging player is harder to hit
	en = PROG_TO_EDICT(self->s.v.enemy);

	//dir = en->s.v.origin - en->s.v.velocity * 0.2;
	VectorMA(en->s.v.origin, -0.2, en->s.v.velocity, dir);
	//dir = normalize (dir - self->s.v.origin);
	VectorSubtract(dir, self->s.v.origin, dir);
	normalize(dir, dir);

	FireBullets(4, dir, 0.1, 0.1, 0, dtSQUISH /* FIXME */);
}

ANIM(army_atk1, shoot1, army_atk2; ai_face();)
ANIM(army_atk2, shoot2, army_atk3; ai_face();)
ANIM(army_atk3, shoot3, army_atk4; ai_face();)
ANIM(army_atk4, shoot4, army_atk5; ai_face();)
ANIM(army_atk5, shoot5, army_atk6; ai_face(); army_fire(); muzzleflash();)
ANIM(army_atk6, shoot6, army_atk7; ai_face();)
ANIM(army_atk7, shoot7, army_atk8; ai_face(); SUB_CheckRefire(( func_t) army_atk1);)
ANIM(army_atk8, shoot8, army_atk9; ai_face();)
ANIM(army_atk9, shoot9, army_run1; ai_face();)

ANIM(army_pain1, pain1, army_pain2;)
ANIM(army_pain2, pain2, army_pain3;)
ANIM(army_pain3, pain3, army_pain4;)
ANIM(army_pain4, pain4, army_pain5;)
ANIM(army_pain5, pain5, army_pain6;)
ANIM(army_pain6, pain6, army_run1; ai_pain(1);)

ANIM(army_painb1, painb1, army_painb2;)
ANIM(army_painb2, painb2, army_painb3; ai_painforward(13);)
ANIM(army_painb3, painb3, army_painb4; ai_painforward(9);)
ANIM(army_painb4, painb4, army_painb5;)
ANIM(army_painb5, painb5, army_painb6;)
ANIM(army_painb6, painb6, army_painb7;)
ANIM(army_painb7, painb7, army_painb8;)
ANIM(army_painb8, painb8, army_painb9;)
ANIM(army_painb9, painb9, army_painb10;)
ANIM(army_painb10, painb10, army_painb11;)
ANIM(army_painb11, painb11, army_painb12;)
ANIM(army_painb12, painb12, army_painb13; ai_pain(2);)
ANIM(army_painb13, painb13, army_painb14;)
ANIM(army_painb14, painb14, army_run1;)

ANIM(army_painc1, painc1, army_painc2;)
ANIM(army_painc2, painc2, army_painc3; ai_pain(1);)
ANIM(army_painc3, painc3, army_painc4;)
ANIM(army_painc4, painc4, army_painc5;)
ANIM(army_painc5, painc5, army_painc6; ai_painforward(1);)
ANIM(army_painc6, painc6, army_painc7; ai_painforward(1);)
ANIM(army_painc7, painc7, army_painc8;)
ANIM(army_painc8, painc8, army_painc9; ai_pain(1);)
ANIM(army_painc9, painc9, army_painc10; ai_painforward(4);)
ANIM(army_painc10, painc10, army_painc11; ai_painforward(3);)
ANIM(army_painc11, painc11, army_painc12; ai_painforward(6);)
ANIM(army_painc12, painc12, army_painc13; ai_painforward(8);)
ANIM(army_painc13, painc13, army_run1;)

void army_pain(gedict_t *attacker, float damage)
{
	float r;

	if (self->pain_finished > g_globalvars.time)
	{
		return;
	}

	r = g_random();

	if (r < 0.2)
	{
		self->pain_finished = g_globalvars.time + 0.6;
		army_pain1();
		sound(self, CHAN_VOICE, "soldier/pain1.wav", 1, ATTN_NORM);
	}
	else if (r < 0.6)
	{
		self->pain_finished = g_globalvars.time + 1.1;
		army_painb1();
		sound(self, CHAN_VOICE, "soldier/pain2.wav", 1, ATTN_NORM);
	}
	else
	{
		self->pain_finished = g_globalvars.time + 1.1;
		army_painc1();
		sound(self, CHAN_VOICE, "soldier/pain2.wav", 1, ATTN_NORM);
	}
}

void army_drop_pack(void)
{
	self->s.v.ammo_shells = 5; // drop packwith 5 shells
	DropBackpack();
}

void army_die_xxx(void)
{
	self->s.v.solid = SOLID_NOT;

	army_drop_pack();
}

ANIM(army_die1, death1, army_die2;)
ANIM(army_die2, death2, army_die3;)
ANIM(army_die3, death3, army_die4; army_die_xxx();)
ANIM(army_die4, death4, army_die5;)
ANIM(army_die5, death5, army_die6;)
ANIM(army_die6, death6, army_die7;)
ANIM(army_die7, death7, army_die8;)
ANIM(army_die8, death8, army_die9;)
ANIM(army_die9, death9, army_die10;)
ANIM(army_die10, death10, army_die10;)

ANIM(army_cdie1, deathc1, army_cdie2;)
ANIM(army_cdie2, deathc2, army_cdie3; ai_back(5);)
ANIM(army_cdie3, deathc3, army_cdie4; ai_back(4); army_die_xxx();)
ANIM(army_cdie4, deathc4, army_cdie5; ai_back(13);)
ANIM(army_cdie5, deathc5, army_cdie6; ai_back(3);)
ANIM(army_cdie6, deathc6, army_cdie7; ai_back(4);)
ANIM(army_cdie7, deathc7, army_cdie8;)
ANIM(army_cdie8, deathc8, army_cdie9;)
ANIM(army_cdie9, deathc9, army_cdie10;)
ANIM(army_cdie10, deathc10, army_cdie11;)
ANIM(army_cdie11, deathc11, army_cdie11;)

void army_die(void)
{
	// check for gib
	if (self->s.v.health < -35)
	{
		army_drop_pack();

		sound(self, CHAN_VOICE, "player/udeath.wav", 1, ATTN_NORM);
		ThrowHead("progs/h_guard.mdl", self->s.v.health);
		ThrowGib("progs/gib1.mdl", self->s.v.health);
		ThrowGib("progs/gib2.mdl", self->s.v.health);
		ThrowGib("progs/gib3.mdl", self->s.v.health);

		self->s.v.nextthink = -1;

		return;
	}

	// regular death
	sound(self, CHAN_VOICE, "soldier/death1.wav", 1, ATTN_NORM);

	if (g_random() < 0.5)
	{
		army_die1();
	}
	else
	{
		army_cdie1();
	}
}

//============================================================================

/*
 ===========
 SoldierCheckAttack

 The player is in view, so decide to move or launch an attack
 Returns false if movement should continue
 ============
 */
float SoldierCheckAttack(void)
{
	vec3_t spot1, spot2;
	gedict_t *targ;
	float chance;

	if (g_globalvars.time < self->attack_finished)
	{
		return false;
	}

	targ = PROG_TO_EDICT(self->s.v.enemy);

	// see if any entities are in the way of the shot
	VectorAdd(self->s.v.origin, self->s.v.view_ofs, spot1);
	VectorAdd(targ->s.v.origin, targ->s.v.view_ofs, spot2);

	traceline(PASSVEC3(spot1), PASSVEC3(spot2), false, self);

	if (g_globalvars.trace_inopen && g_globalvars.trace_inwater)
	{
		return false; // sight line crossed contents
	}

	if ( PROG_TO_EDICT(g_globalvars.trace_ent) != targ)
	{
		return false;	// don't have a clear shot
	}

	// missile attack

	if (enemy_range == RANGE_FAR)
	{
		return false;
	}

	if (enemy_range == RANGE_MELEE)
	{
		chance = 0.9;
	}
	else if (enemy_range == RANGE_NEAR)
	{
		chance = 0.4;
	}
	else if (enemy_range == RANGE_MID)
	{
		chance = 0.05;
	}
	else
	{
		chance = 0;
	}

	if (g_random() < chance)
	{
		if (self->th_missile)
		{
			self->th_missile();
		}

		SUB_AttackFinished(1 + g_random());
		if (g_random() < 0.3)
		{
			self->lefty = !self->lefty;
		}

		return true;
	}

	return false;
}

//============================================================================

/*QUAKED monster_army (1 0 0) (-16 -16 -24) (16 16 40) Ambush
 */
void SP_monster_army(void)
{
	if (!AllowMonster(self))
	{
		ent_remove(self);

		return;
	}

	safe_precache_model("progs/soldier.mdl");
	safe_precache_model("progs/h_guard.mdl");
	safe_precache_model("progs/gib1.mdl");
	safe_precache_model("progs/gib2.mdl");
	safe_precache_model("progs/gib3.mdl");

	safe_precache_sound("soldier/death1.wav");
	safe_precache_sound("soldier/idle.wav");
	safe_precache_sound("soldier/pain1.wav");
	safe_precache_sound("soldier/pain2.wav");
	safe_precache_sound("soldier/sattck1.wav");
	safe_precache_sound("soldier/sight1.wav");

	safe_precache_sound("player/udeath.wav");		// gib death

	setsize(self, -16, -16, -24, 16, 16, 40);
	self->s.v.health = 30;

	self->th_stand = army_stand1;
	self->th_walk = army_walk1;
	self->th_run = army_run1;
	self->th_missile = army_atk1;
	self->th_pain = army_pain;
	self->th_die = army_die;

	self->th_respawn = SP_monster_army;

	walkmonster_start("progs/soldier.mdl");
}
