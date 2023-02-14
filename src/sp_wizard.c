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

 WIZARD

 ==============================================================================
 */

#include "g_local.h"

enum
{
	hover1,
	hover2,
	hover3,
	hover4,
	hover5,
	hover6,
	hover7,
	hover8,
	hover9,
	hover10,
	hover11,
	hover12,
	hover13,
	hover14,
	hover15,

	fly1,
	fly2,
	fly3,
	fly4,
	fly5,
	fly6,
	fly7,
	fly8,
	fly9,
	fly10,
	fly11,
	fly12,
	fly13,
	fly14,

	magatt1,
	magatt2,
	magatt3,
	magatt4,
	magatt5,
	magatt6,
	magatt7,
	magatt8,
	magatt9,
	magatt10,
	magatt11,
	magatt12,
	magatt13,

	pain1,
	pain2,
	pain3,
	pain4,

	death1,
	death2,
	death3,
	death4,
	death5,
	death6,
	death7,
	death8,

};

void wiz_stand1();
void wiz_stand2();
void wiz_stand3();
void wiz_stand4();
void wiz_stand5();
void wiz_stand6();
void wiz_stand7();
void wiz_stand8();
void wiz_walk1();
void wiz_walk2();
void wiz_walk3();
void wiz_walk4();
void wiz_walk5();
void wiz_walk6();
void wiz_walk7();
void wiz_walk8();
void wiz_side1();
void wiz_side2();
void wiz_side3();
void wiz_side4();
void wiz_side5();
void wiz_side6();
void wiz_side7();
void wiz_side8();
void wiz_run1();
void wiz_run2();
void wiz_run3();
void wiz_run4();
void wiz_run5();
void wiz_run6();
void wiz_run7();
void wiz_run8();
void wiz_run9();
void wiz_run10();
void wiz_run11();
void wiz_run12();
void wiz_run13();
void wiz_run14();
void wiz_fast1();
void wiz_fast2();
void wiz_fast3();
void wiz_fast4();
void wiz_fast5();
void wiz_fast6();
void wiz_fast7();
void wiz_fast8();
void wiz_fast9();
void wiz_fast10();
void wiz_pain1();
void wiz_pain2();
void wiz_pain3();
void wiz_pain4();
void wiz_death1();
void wiz_death2();
void wiz_death3();
void wiz_death4();
void wiz_death5();
void wiz_death6();
void wiz_death7();
void wiz_death8();

/*
 ==============================================================================

 FAST ATTACKS

 ==============================================================================
 */

void Wiz_IdleSound()
{
	float wr = g_random() * 5;

//	if (self->waitmin < g_globalvars.time)
//	{
//	 	self->waitmin = g_globalvars.time + 2;
	if (wr > 4.5)
	{
		sound(self, CHAN_VOICE, "wizard/widle1.wav", 1, ATTN_IDLE);
	}
	else if (wr < 0.5)
	{
		sound(self, CHAN_VOICE, "wizard/widle2.wav", 1, ATTN_IDLE);
	}
//	}
}

ANIM(wiz_stand1, hover1, wiz_stand2; ai_stand();)
ANIM(wiz_stand2, hover2, wiz_stand3; ai_stand();)
ANIM(wiz_stand3, hover3, wiz_stand4; ai_stand();)
ANIM(wiz_stand4, hover4, wiz_stand5; ai_stand();)
ANIM(wiz_stand5, hover5, wiz_stand6; ai_stand();)
ANIM(wiz_stand6, hover6, wiz_stand7; ai_stand();)
ANIM(wiz_stand7, hover7, wiz_stand8; ai_stand();)
ANIM(wiz_stand8, hover8, wiz_stand1; ai_stand();)

ANIM(wiz_walk1, hover1, wiz_walk2; ai_walk(8); Wiz_IdleSound();)
ANIM(wiz_walk2, hover2, wiz_walk3; ai_walk(8);)
ANIM(wiz_walk3, hover3, wiz_walk4; ai_walk(8);)
ANIM(wiz_walk4, hover4, wiz_walk5; ai_walk(8);)
ANIM(wiz_walk5, hover5, wiz_walk6; ai_walk(8);)
ANIM(wiz_walk6, hover6, wiz_walk7; ai_walk(8);)
ANIM(wiz_walk7, hover7, wiz_walk8; ai_walk(8);)
ANIM(wiz_walk8, hover8, wiz_walk1; ai_walk(8);)

ANIM(wiz_side1, hover1, wiz_side2; ai_run(8); Wiz_IdleSound();)
ANIM(wiz_side2, hover2, wiz_side3; ai_run(8);)
ANIM(wiz_side3, hover3, wiz_side4; ai_run(8);)
ANIM(wiz_side4, hover4, wiz_side5; ai_run(8);)
ANIM(wiz_side5, hover5, wiz_side6; ai_run(8);)
ANIM(wiz_side6, hover6, wiz_side7; ai_run(8);)
ANIM(wiz_side7, hover7, wiz_side8; ai_run(8);)
ANIM(wiz_side8, hover8, wiz_side1; ai_run(8);)

ANIM(wiz_run1, fly1, wiz_run2; ai_run(16); Wiz_IdleSound();)
ANIM(wiz_run2, fly2, wiz_run3; ai_run(16);)
ANIM(wiz_run3, fly3, wiz_run4; ai_run(16);)
ANIM(wiz_run4, fly4, wiz_run5; ai_run(16);)
ANIM(wiz_run5, fly5, wiz_run6; ai_run(16);)
ANIM(wiz_run6, fly6, wiz_run7; ai_run(16);)
ANIM(wiz_run7, fly7, wiz_run8; ai_run(16);)
ANIM(wiz_run8, fly8, wiz_run9; ai_run(16);)
ANIM(wiz_run9, fly9, wiz_run10; ai_run(16);)
ANIM(wiz_run10, fly10, wiz_run11; ai_run(16);)
ANIM(wiz_run11, fly11, wiz_run12; ai_run(16);)
ANIM(wiz_run12, fly12, wiz_run13; ai_run(16);)
ANIM(wiz_run13, fly13, wiz_run14; ai_run(16);)
ANIM(wiz_run14, fly14, wiz_run1; ai_run(16);)

void Wiz_FastFire()
{
	vec3_t vec;
	vec3_t dst;
	gedict_t *oself;

	// owner is wizard.
	// self is missile.
	if (ISLIVE(PROG_TO_EDICT(self->s.v.owner)))
	{
		oself = self; // remember
		self = PROG_TO_EDICT(self->s.v.owner);

		muzzleflash();

		self = oself; // restore

		///////trap_makevectors( PROG_TO_EDICT(self->s.v.enemy)->s.v.angles);
		//dst = self->s.v.enemy->s.v.origin - 13*self->s.v.movedir;
		VectorMA( PROG_TO_EDICT(self->s.v.enemy)->s.v.origin, -13, self->s.v.movedir, dst);

		//vec = normalize(dst - self->s.v.origin);
		VectorSubtract(dst, self->s.v.origin, vec);
		normalize(vec, vec);
		sound(self, CHAN_WEAPON, "wizard/wattack.wav", 1, ATTN_NORM);

		launch_spike(self->s.v.origin, vec);

		VectorScale(vec, 600, newmis->s.v.velocity);
		newmis->s.v.owner = self->s.v.owner;
		newmis->classname = "wizspike";
		setmodel(newmis, "progs/w_spike.mdl");
		setsize(newmis, PASSVEC3(VEC_ORIGIN), PASSVEC3(VEC_ORIGIN));
	}

	ent_remove(self);
}

void Wiz_StartFast()
{
	gedict_t *missile;

	sound(self, CHAN_WEAPON, "wizard/wattack.wav", 1, ATTN_NORM);

	VectorCopy(self->s.v.angles, self->s.v.v_angle)
	trap_makevectors(self->s.v.angles);

	missile = spawn();
	missile->s.v.owner = EDICT_TO_PROG(self);
	setsize(missile, PASSVEC3(VEC_ORIGIN), PASSVEC3(VEC_ORIGIN));
	//self->s.v.origin + '0 0 30' + v_forward * 14 + v_right * 14
	VectorCopy(self->s.v.origin, missile->s.v.origin);
	missile->s.v.origin[2] += 30;
	VectorMA(missile->s.v.origin, 14, g_globalvars.v_forward, missile->s.v.origin);
	VectorMA(missile->s.v.origin, 14, g_globalvars.v_right, missile->s.v.origin);
	setorigin(missile, PASSVEC3(missile->s.v.origin));
	missile->s.v.enemy = self->s.v.enemy;
	missile->s.v.nextthink = g_globalvars.time + 0.8;
	missile->think = (func_t) Wiz_FastFire;
	VectorCopy(g_globalvars.v_right, missile->s.v.movedir);

	missile = spawn();
	missile->s.v.owner = EDICT_TO_PROG(self);
	setsize(missile, PASSVEC3(VEC_ORIGIN), PASSVEC3(VEC_ORIGIN));
	//self->s.v.origin + '0 0 30' + v_forward * 14 + v_right * -14
	VectorCopy(self->s.v.origin, missile->s.v.origin);
	missile->s.v.origin[2] += 30;
	VectorMA(missile->s.v.origin, 14, g_globalvars.v_forward, missile->s.v.origin);
	VectorMA(missile->s.v.origin, -14, g_globalvars.v_right, missile->s.v.origin);
	setorigin(missile, PASSVEC3(missile->s.v.origin));
	missile->s.v.enemy = self->s.v.enemy;
	missile->s.v.nextthink = g_globalvars.time + 0.3;
	missile->think = (func_t) Wiz_FastFire;
	VectorScale(g_globalvars.v_right, -1, missile->s.v.movedir);
}

void WizardAttackFinished()
{
	if ((enemy_range >= RANGE_MID) || !enemy_vis)
	{
		self->attack_state = AS_STRAIGHT;
		self->think = (func_t) wiz_run1;
	}
	else
	{
		self->attack_state = AS_SLIDING;
		self->think = (func_t) wiz_side1;
	}
}

ANIM(wiz_fast1, magatt1, wiz_fast2; ai_face(); Wiz_StartFast();)
ANIM(wiz_fast2, magatt2, wiz_fast3; ai_face();)
ANIM(wiz_fast3, magatt3, wiz_fast4; ai_face();)
ANIM(wiz_fast4, magatt4, wiz_fast5; ai_face();)
ANIM(wiz_fast5, magatt5, wiz_fast6; ai_face();)
ANIM(wiz_fast6, magatt6, wiz_fast7; ai_face();)
ANIM(wiz_fast7, magatt5, wiz_fast8; ai_face();)
ANIM(wiz_fast8, magatt4, wiz_fast9; ai_face();)
ANIM(wiz_fast9, magatt3, wiz_fast10; ai_face();)
ANIM(wiz_fast10, magatt2, wiz_run1; ai_face(); SUB_AttackFinished(2); WizardAttackFinished();)

ANIM(wiz_pain1, pain1, wiz_pain2;)
ANIM(wiz_pain2, pain2, wiz_pain3;)
ANIM(wiz_pain3, pain3, wiz_pain4;)
ANIM(wiz_pain4, pain4, wiz_run1;)

void wiz_pain(gedict_t *attacker, float damage)
{
	if ((g_random() * 70) > damage)
	{
		return;		// didn't flinch
	}

	sound(self, CHAN_VOICE, "wizard/wpain.wav", 1, ATTN_NORM);

	wiz_pain1();
}

void _wiz_death1(void)
{
	self->s.v.velocity[0] = 200 * crandom();
	self->s.v.velocity[1] = 200 * crandom();
	self->s.v.velocity[2] = 100 + 100 * g_random();
	self->s.v.flags = (int)self->s.v.flags & ~FL_ONGROUND;

	sound(self, CHAN_VOICE, "wizard/wdeath.wav", 1, ATTN_NORM);
}

ANIM(wiz_death1, death1, wiz_death2; _wiz_death1();)
ANIM(wiz_death2, death2, wiz_death3;)
ANIM(wiz_death3, death3, wiz_death4; self->s.v.solid = SOLID_NOT;)
ANIM(wiz_death4, death4, wiz_death5;)
ANIM(wiz_death5, death5, wiz_death6;)
ANIM(wiz_death6, death6, wiz_death7;)
ANIM(wiz_death7, death7, wiz_death8;)
ANIM(wiz_death8, death8, wiz_death8;)

void wiz_die()
{
	// check for gib
	if (self->s.v.health < -40)
	{
		sound(self, CHAN_VOICE, "player/udeath.wav", 1, ATTN_NORM);
		ThrowHead("progs/h_wizard.mdl", self->s.v.health);
		ThrowGib("progs/gib2.mdl", self->s.v.health);
		ThrowGib("progs/gib2.mdl", self->s.v.health);
		ThrowGib("progs/gib2.mdl", self->s.v.health);

		self->s.v.nextthink = -1;

		return;
	}

	wiz_death1();
}

//=============================================================================

/*
 =================
 WizardCheckAttack
 =================
 */
float WizardCheckAttack()
{
	vec3_t spot1, spot2;
	gedict_t *targ;
	float chance;

	if (g_globalvars.time < self->attack_finished)
	{
		return false;
	}

	if (!enemy_vis)
	{
		return false;
	}

	if (enemy_range == RANGE_FAR)
	{
		if (self->attack_state != AS_STRAIGHT)
		{
			self->attack_state = AS_STRAIGHT;
			wiz_run1();
		}

		return false;
	}

	targ = PROG_TO_EDICT(self->s.v.enemy);

	// see if any entities are in the way of the shot
	VectorAdd(self->s.v.origin, self->s.v.view_ofs, spot1);
	VectorAdd(targ->s.v.origin, targ->s.v.view_ofs, spot2);

	traceline(PASSVEC3(spot1), PASSVEC3(spot2), false, self);

//	if ( g_globalvars.trace_inopen && g_globalvars.trace_inwater )
//		return false; // sight line crossed contents

	if (PROG_TO_EDICT(g_globalvars.trace_ent) != targ)
	{	// don't have a clear shot, so move to a side
		if (self->attack_state != AS_STRAIGHT)
		{
			self->attack_state = AS_STRAIGHT;
			wiz_run1();
		}

		return false;
	}

	if (enemy_range == RANGE_MELEE)
	{
		chance = 0.9;
	}
	else if (enemy_range == RANGE_NEAR)
	{
		chance = 0.6;
	}
	else if (enemy_range == RANGE_MID)
	{
		chance = 0.2;
	}
	else
	{
		chance = 0;
	}

	if (g_random() < chance)
	{
		self->attack_state = AS_MISSILE;

		return true;
	}

	if (enemy_range == RANGE_MID)
	{
		if (self->attack_state != AS_STRAIGHT)
		{
			self->attack_state = AS_STRAIGHT;
			wiz_run1();
		}
	}
	else
	{
		if (self->attack_state != AS_SLIDING)
		{
			self->attack_state = AS_SLIDING;
			wiz_side1();
		}
	}

	return false;
}

//=============================================================================

/*QUAKED monster_wizard (1 0 0) (-16 -16 -24) (16 16 40) Ambush
 */
void SP_monster_wizard()
{
	if (!AllowMonster(self))
	{
		ent_remove(self);

		return;
	}

	safe_precache_model("progs/wizard.mdl");
	safe_precache_model("progs/h_wizard.mdl");
	safe_precache_model("progs/w_spike.mdl");

	safe_precache_sound("wizard/hit.wav");		// used by c code
	safe_precache_sound("wizard/wattack.wav");
	safe_precache_sound("wizard/wdeath.wav");
	safe_precache_sound("wizard/widle1.wav");
	safe_precache_sound("wizard/widle2.wav");
	safe_precache_sound("wizard/wpain.wav");
	safe_precache_sound("wizard/wsight.wav");

	setsize(self, -16, -16, -24, 16, 16, 40);
	self->s.v.health = 80;

	self->th_stand = wiz_stand1;
	self->th_walk = wiz_walk1;
	self->th_run = wiz_run1;
	self->th_missile = wiz_fast1;
	self->th_pain = wiz_pain;
	self->th_die = wiz_die;

	self->th_respawn = SP_monster_wizard;

	flymonster_start("progs/wizard.mdl");
}
