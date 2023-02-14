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

 FISH

 ==============================================================================
 */

#include "g_local.h"

enum
{
	attack1,
	attack2,
	attack3,
	attack4,
	attack5,
	attack6,
	attack7,
	attack8,
	attack9,
	attack10,
	attack11,
	attack12,
	attack13,
	attack14,
	attack15,
	attack16,
	attack17,
	attack18,

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
	death11,
	death12,
	death13,
	death14,
	death15,
	death16,
	death17,
	death18,
	death19,
	death20,
	death21,

	swim1,
	swim2,
	swim3,
	swim4,
	swim5,
	swim6,
	swim7,
	swim8,
	swim9,
	swim10,
	swim11,
	swim12,
	swim13,
	swim14,
	swim15,
	swim16,
	swim17,
	swim18,

	pain1,
	pain2,
	pain3,
	pain4,
	pain5,
	pain6,
	pain7,
	pain8,
	pain9,

};

void f_stand1();
void f_stand2();
void f_stand3();
void f_stand4();
void f_stand5();
void f_stand6();
void f_stand7();
void f_stand8();
void f_stand9();
void f_stand10();
void f_stand11();
void f_stand12();
void f_stand13();
void f_stand14();
void f_stand15();
void f_stand16();
void f_stand17();
void f_stand18();
void f_walk1();
void f_walk2();
void f_walk3();
void f_walk4();
void f_walk5();
void f_walk6();
void f_walk7();
void f_walk8();
void f_walk9();
void f_walk10();
void f_walk11();
void f_walk12();
void f_walk13();
void f_walk14();
void f_walk15();
void f_walk16();
void f_walk17();
void f_walk18();
void f_run1();
void f_run2();
void f_run3();
void f_run4();
void f_run5();
void f_run6();
void f_run7();
void f_run8();
void f_run9();
void f_attack1();
void f_attack2();
void f_attack3();
void f_attack4();
void f_attack5();
void f_attack6();
void f_attack7();
void f_attack8();
void f_attack9();
void f_attack10();
void f_attack11();
void f_attack12();
void f_attack13();
void f_attack14();
void f_attack15();
void f_attack16();
void f_attack17();
void f_attack18();
void f_death1();
void f_death2();
void f_death3();
void f_death4();
void f_death5();
void f_death6();
void f_death7();
void f_death8();
void f_death9();
void f_death10();
void f_death11();
void f_death12();
void f_death13();
void f_death14();
void f_death15();
void f_death16();
void f_death17();
void f_death18();
void f_death19();
void f_death20();
void f_death21();
void f_pain1();
void f_pain2();
void f_pain3();
void f_pain4();
void f_pain5();
void f_pain6();
void f_pain7();
void f_pain8();
void f_pain9();

//==============================================================================

ANIM(f_stand1, swim1, f_stand2; ai_stand();)
ANIM(f_stand2, swim2, f_stand3; ai_stand();)
ANIM(f_stand3, swim3, f_stand4; ai_stand();)
ANIM(f_stand4, swim4, f_stand5; ai_stand();)
ANIM(f_stand5, swim5, f_stand6; ai_stand();)
ANIM(f_stand6, swim6, f_stand7; ai_stand();)
ANIM(f_stand7, swim7, f_stand8; ai_stand();)
ANIM(f_stand8, swim8, f_stand9; ai_stand();)
ANIM(f_stand9, swim9, f_stand10; ai_stand();)
ANIM(f_stand10, swim10, f_stand11; ai_stand();)
ANIM(f_stand11, swim11, f_stand12; ai_stand();)
ANIM(f_stand12, swim12, f_stand13; ai_stand();)
ANIM(f_stand13, swim13, f_stand14; ai_stand();)
ANIM(f_stand14, swim14, f_stand15; ai_stand();)
ANIM(f_stand15, swim15, f_stand16; ai_stand();)
ANIM(f_stand16, swim16, f_stand17; ai_stand();)
ANIM(f_stand17, swim17, f_stand18; ai_stand();)
ANIM(f_stand18, swim18, f_stand1; ai_stand();)

ANIM(f_walk1, swim1, f_walk2; ai_walk(8);)
ANIM(f_walk2, swim2, f_walk3; ai_walk(8);)
ANIM(f_walk3, swim3, f_walk4; ai_walk(8);)
ANIM(f_walk4, swim4, f_walk5; ai_walk(8);)
ANIM(f_walk5, swim5, f_walk6; ai_walk(8);)
ANIM(f_walk6, swim6, f_walk7; ai_walk(8);)
ANIM(f_walk7, swim7, f_walk8; ai_walk(8);)
ANIM(f_walk8, swim8, f_walk9; ai_walk(8);)
ANIM(f_walk9, swim9, f_walk10; ai_walk(8);)
ANIM(f_walk10, swim10, f_walk11; ai_walk(8);)
ANIM(f_walk11, swim11, f_walk12; ai_walk(8);)
ANIM(f_walk12, swim12, f_walk13; ai_walk(8);)
ANIM(f_walk13, swim13, f_walk14; ai_walk(8);)
ANIM(f_walk14, swim14, f_walk15; ai_walk(8);)
ANIM(f_walk15, swim15, f_walk16; ai_walk(8);)
ANIM(f_walk16, swim16, f_walk17; ai_walk(8);)
ANIM(f_walk17, swim17, f_walk18; ai_walk(8);)
ANIM(f_walk18, swim18, f_walk1; ai_walk(8);)

void _f_run1(void)
{
	ai_run(12);

	// force stupid fish attack faster.
	if (self->s.v.enemy && CanDamage(PROG_TO_EDICT(self->s.v.enemy), self))
	{
		f_attack1();

		return;
	}

	if (g_random() < 0.5)
	{
		sound(self, CHAN_VOICE, "fish/idle.wav", 1, ATTN_NORM);
	}
}

#if 0

// NOTE: and why frames are skipped? like 1 3 5 7 ... 13 15 17 1..., perhaps just to make animation more "fast".

ANIM(f_run1, swim1,  f_run2; _f_run1();)
ANIM(f_run2, swim3,  f_run3; ai_run(12);)
ANIM(f_run3, swim5,  f_run4; ai_run(12);)
ANIM(f_run4, swim7,  f_run5; ai_run(12);)
ANIM(f_run5, swim9,  f_run6; ai_run(12);)
ANIM(f_run6, swim11, f_run7; ai_run(12);)
ANIM(f_run7, swim13, f_run8; ai_run(12);)
ANIM(f_run8, swim15, f_run9; ai_run(12);)
ANIM(f_run9, swim17, f_run1; ai_run(12);)

#else

void f_run10();
void f_run11();
void f_run12();
void f_run13();
void f_run14();
void f_run15();
void f_run16();
void f_run17();
void f_run18();

ANIM(f_run1, swim1, f_run2; _f_run1();)
ANIM(f_run2, swim2, f_run3; ai_run(12);)
ANIM(f_run3, swim3, f_run4; ai_run(12);)
ANIM(f_run4, swim4, f_run5; ai_run(12);)
ANIM(f_run5, swim5, f_run6; ai_run(12);)
ANIM(f_run6, swim6, f_run7; ai_run(12);)
ANIM(f_run7, swim7, f_run8; ai_run(12);)
ANIM(f_run8, swim8, f_run9; ai_run(12);)
ANIM(f_run9, swim9, f_run10; ai_run(12);)
ANIM(f_run10, swim10, f_run11; ai_run(12);)
ANIM(f_run11, swim11, f_run12; ai_run(12);)
ANIM(f_run12, swim12, f_run13; ai_run(12);)
ANIM(f_run13, swim13, f_run14; ai_run(12);)
ANIM(f_run14, swim14, f_run15; ai_run(12);)
ANIM(f_run15, swim15, f_run16; ai_run(12);)
ANIM(f_run16, swim16, f_run17; ai_run(12);)
ANIM(f_run17, swim17, f_run18; ai_run(12);)
ANIM(f_run18, swim18, f_run1; ai_run(12);)
#endif

void fish_melee(void)
{
	vec3_t delta;
	float ldmg;

	if (!self->s.v.enemy)
	{
		return;
	}

	if (!CanDamage(PROG_TO_EDICT(self->s.v.enemy), self))
	{
		return;
	}

	VectorSubtract(PROG_TO_EDICT(self->s.v.enemy)->s.v.origin, self->s.v.origin, delta);

	if (vlen(delta) > 110)
	{
		return;
	}

	sound(self, CHAN_VOICE, "fish/bite.wav", 1, ATTN_NORM);

	if (k_bloodfest)
	{
		ldmg = (g_random() + g_random() + g_random() + g_random() + g_random()) * 3;
	}
	else
	{
		ldmg = (g_random() + g_random()) * 3;
	}

	PROG_TO_EDICT(self->s.v.enemy)->deathtype = dtSQUISH; // FIXME
	T_Damage(PROG_TO_EDICT(self->s.v.enemy), self, self, ldmg);
}

ANIM(f_attack1, attack1, f_attack2; ai_charge(10);)
ANIM(f_attack2, attack2, f_attack3; ai_charge(10);)
ANIM(f_attack3, attack3, f_attack4; fish_melee();)
ANIM(f_attack4, attack4, f_attack5; ai_charge(10);)
ANIM(f_attack5, attack5, f_attack6; ai_charge(10);)
ANIM(f_attack6, attack6, f_attack7; ai_charge(10);)
ANIM(f_attack7, attack7, f_attack8; ai_charge(10);)
ANIM(f_attack8, attack8, f_attack9; ai_charge(10);)
ANIM(f_attack9, attack9, f_attack10; fish_melee();)
ANIM(f_attack10, attack10, f_attack11; ai_charge(10);)
ANIM(f_attack11, attack11, f_attack12; ai_charge(10);)
ANIM(f_attack12, attack12, f_attack13; ai_charge(10);)
ANIM(f_attack13, attack13, f_attack14; ai_charge(10);)
ANIM(f_attack14, attack14, f_attack15; ai_charge(10);)
ANIM(f_attack15, attack15, f_attack16; fish_melee();)
ANIM(f_attack16, attack16, f_attack17; ai_charge(10);)
ANIM(f_attack17, attack17, f_attack18; ai_charge(10);)
ANIM(f_attack18, attack18, f_run1; ai_charge(10);)

void _f_death1(void)
{
	sound(self, CHAN_VOICE, "fish/death.wav", 1, ATTN_NORM);
}
void _f_death3(void)
{
	self->s.v.solid = SOLID_NOT;
}

ANIM(f_death1, death1, f_death2; _f_death1();)
ANIM(f_death2, death2, f_death3)
ANIM(f_death3, death3, f_death4; _f_death3();)
ANIM(f_death4, death4, f_death5)
ANIM(f_death5, death5, f_death6)
ANIM(f_death6, death6, f_death7)
ANIM(f_death7, death7, f_death8)
ANIM(f_death8, death8, f_death9)
ANIM(f_death9, death9, f_death10)
ANIM(f_death10, death10, f_death11)
ANIM(f_death11, death11, f_death12)
ANIM(f_death12, death12, f_death13)
ANIM(f_death13, death13, f_death14)
ANIM(f_death14, death14, f_death15)
ANIM(f_death15, death15, f_death16)
ANIM(f_death16, death16, f_death17)
ANIM(f_death17, death17, f_death18)
ANIM(f_death18, death18, f_death19)
ANIM(f_death19, death19, f_death20)
ANIM(f_death20, death20, f_death21)
ANIM(f_death21, death21, f_death21)

ANIM(f_pain1, pain1, f_pain2)
ANIM(f_pain2, pain2, f_pain3; ai_pain(6);)
ANIM(f_pain3, pain3, f_pain4; ai_pain(6);)
ANIM(f_pain4, pain4, f_pain5; ai_pain(6);)
ANIM(f_pain5, pain5, f_pain6; ai_pain(6);)
ANIM(f_pain6, pain6, f_pain7; ai_pain(6);)
ANIM(f_pain7, pain7, f_pain8; ai_pain(6);)
ANIM(f_pain8, pain8, f_pain9; ai_pain(6);)
ANIM(f_pain9, pain9, f_run1; ai_pain(6);)

void fish_pain(struct gedict_s *attacker, float damage)
{
	// fish always do pain frames
	f_pain1();
}

//===========================================================================

/*QUAKED monster_fish (1 0 0) (-16 -16 -24) (16 16 24) Ambush
 */
void SP_monster_fish()
{
	if (!AllowMonster(self))
	{
		ent_remove(self);

		return;
	}

	safe_precache_model("progs/fish.mdl");

	safe_precache_sound("fish/death.wav");
	safe_precache_sound("fish/bite.wav");
	safe_precache_sound("fish/idle.wav");

	if (k_bloodfest)
	{
		setsize(self, -10, -10, -5, 10, 10, 5);
	}
	else
	{
		setsize(self, -16, -16, -24, 16, 16, 24);
	}

	self->s.v.health = 25;

	self->th_stand = f_stand1;
	self->th_walk = f_walk1;
	self->th_run = f_run1;
	self->th_die = f_death1;
	self->th_pain = fish_pain;
	self->th_melee = f_attack1;

	self->th_respawn = SP_monster_fish;

	swimmonster_start("progs/fish.mdl");
}
