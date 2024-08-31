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

 ZOMBIE

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
	stand14,
	stand15,

	walk1,
	walk2,
	walk3,
	walk4,
	walk5,
	walk6,
	walk7,
	walk8,
	walk9,
	walk10,
	walk11,
	walk12,
	walk13,
	walk14,
	walk15,
	walk16,
	walk17,
	walk18,
	walk19,

	run1,
	run2,
	run3,
	run4,
	run5,
	run6,
	run7,
	run8,
	run9,
	run10,
	run11,
	run12,
	run13,
	run14,
	run15,
	run16,
	run17,
	run18,

	atta1,
	atta2,
	atta3,
	atta4,
	atta5,
	atta6,
	atta7,
	atta8,
	atta9,
	atta10,
	atta11,
	atta12,
	atta13,

	attb1,
	attb2,
	attb3,
	attb4,
	attb5,
	attb6,
	attb7,
	attb8,
	attb9,
	attb10,
	attb11,
	attb12,
	attb13,
	attb14,

	attc1,
	attc2,
	attc3,
	attc4,
	attc5,
	attc6,
	attc7,
	attc8,
	attc9,
	attc10,
	attc11,
	attc12,

	paina1,
	paina2,
	paina3,
	paina4,
	paina5,
	paina6,
	paina7,
	paina8,
	paina9,
	paina10,
	paina11,
	paina12,

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
	painb15,
	painb16,
	painb17,
	painb18,
	painb19,
	painb20,
	painb21,
	painb22,
	painb23,
	painb24,
	painb25,
	painb26,
	painb27,
	painb28,

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
	painc14,
	painc15,
	painc16,
	painc17,
	painc18,

	paind1,
	paind2,
	paind3,
	paind4,
	paind5,
	paind6,
	paind7,
	paind8,
	paind9,
	paind10,
	paind11,
	paind12,
	paind13,

	paine1,
	paine2,
	paine3,
	paine4,
	paine5,
	paine6,
	paine7,
	paine8,
	paine9,
	paine10,
	paine11,
	paine12,
	paine13,
	paine14,
	paine15,
	paine16,
	paine17,
	paine18,
	paine19,
	paine20,
	paine21,
	paine22,
	paine23,
	paine24,
	paine25,
	paine26,
	paine27,
	paine28,
	paine29,
	paine30,

	cruc_1,
	cruc_2,
	cruc_3,
	cruc_4,
	cruc_5,
	cruc_6,

};

qbool frame_is_fast_zombie_pain(int frame)
{
	return (((frame >= paina1) && (frame <= paina12)) || ((frame >= painb1) && (frame <= painb28))
			|| ((frame >= painc1) && (frame <= painc18)) || ((frame >= paind1) && (frame <= paind13)));
}

qbool frame_is_long_zombie_pain(int frame)
{
	return ((frame >= paine1) && (frame <= paine30));
}

void zombie_stand1(void);
void zombie_stand2(void);
void zombie_stand3(void);
void zombie_stand4(void);
void zombie_stand5(void);
void zombie_stand6(void);
void zombie_stand7(void);
void zombie_stand8(void);
void zombie_stand9(void);
void zombie_stand10(void);
void zombie_stand11(void);
void zombie_stand12(void);
void zombie_stand13(void);
void zombie_stand14(void);
void zombie_stand15(void);
void zombie_cruc1(void);
void zombie_cruc2(void);
void zombie_cruc3(void);
void zombie_cruc4(void);
void zombie_cruc5(void);
void zombie_cruc6(void);
void zombie_walk1(void);
void zombie_walk2(void);
void zombie_walk3(void);
void zombie_walk4(void);
void zombie_walk5(void);
void zombie_walk6(void);
void zombie_walk7(void);
void zombie_walk8(void);
void zombie_walk9(void);
void zombie_walk10(void);
void zombie_walk11(void);
void zombie_walk12(void);
void zombie_walk13(void);
void zombie_walk14(void);
void zombie_walk15(void);
void zombie_walk16(void);
void zombie_walk17(void);
void zombie_walk18(void);
void zombie_walk19(void);
void zombie_run1(void);
void zombie_run2(void);
void zombie_run3(void);
void zombie_run4(void);
void zombie_run5(void);
void zombie_run6(void);
void zombie_run7(void);
void zombie_run8(void);
void zombie_run9(void);
void zombie_run10(void);
void zombie_run11(void);
void zombie_run12(void);
void zombie_run13(void);
void zombie_run14(void);
void zombie_run15(void);
void zombie_run16(void);
void zombie_run17(void);
void zombie_run18(void);
void zombie_atta1(void);
void zombie_atta2(void);
void zombie_atta3(void);
void zombie_atta4(void);
void zombie_atta5(void);
void zombie_atta6(void);
void zombie_atta7(void);
void zombie_atta8(void);
void zombie_atta9(void);
void zombie_atta10(void);
void zombie_atta11(void);
void zombie_atta12(void);
void zombie_atta13(void);
void zombie_attb1(void);
void zombie_attb2(void);
void zombie_attb3(void);
void zombie_attb4(void);
void zombie_attb5(void);
void zombie_attb6(void);
void zombie_attb7(void);
void zombie_attb8(void);
void zombie_attb9(void);
void zombie_attb10(void);
void zombie_attb11(void);
void zombie_attb12(void);
void zombie_attb13(void);
void zombie_attb14(void);
void zombie_attc1(void);
void zombie_attc2(void);
void zombie_attc3(void);
void zombie_attc4(void);
void zombie_attc5(void);
void zombie_attc6(void);
void zombie_attc7(void);
void zombie_attc8(void);
void zombie_attc9(void);
void zombie_attc10(void);
void zombie_attc11(void);
void zombie_attc12(void);
void zombie_paina1(void);
void zombie_paina2(void);
void zombie_paina3(void);
void zombie_paina4(void);
void zombie_paina5(void);
void zombie_paina6(void);
void zombie_paina7(void);
void zombie_paina8(void);
void zombie_paina9(void);
void zombie_paina10(void);
void zombie_paina11(void);
void zombie_paina12(void);
void zombie_painb1(void);
void zombie_painb2(void);
void zombie_painb3(void);
void zombie_painb4(void);
void zombie_painb5(void);
void zombie_painb6(void);
void zombie_painb7(void);
void zombie_painb8(void);
void zombie_painb9(void);
void zombie_painb10(void);
void zombie_painb11(void);
void zombie_painb12(void);
void zombie_painb13(void);
void zombie_painb14(void);
void zombie_painb15(void);
void zombie_painb16(void);
void zombie_painb17(void);
void zombie_painb18(void);
void zombie_painb19(void);
void zombie_painb20(void);
void zombie_painb21(void);
void zombie_painb22(void);
void zombie_painb23(void);
void zombie_painb24(void);
void zombie_painb25(void);
void zombie_painb26(void);
void zombie_painb27(void);
void zombie_painb28(void);
void zombie_painc1(void);
void zombie_painc2(void);
void zombie_painc3(void);
void zombie_painc4(void);
void zombie_painc5(void);
void zombie_painc6(void);
void zombie_painc7(void);
void zombie_painc8(void);
void zombie_painc9(void);
void zombie_painc10(void);
void zombie_painc11(void);
void zombie_painc12(void);
void zombie_painc13(void);
void zombie_painc14(void);
void zombie_painc15(void);
void zombie_painc16(void);
void zombie_painc17(void);
void zombie_painc18(void);
void zombie_paind1(void);
void zombie_paind2(void);
void zombie_paind3(void);
void zombie_paind4(void);
void zombie_paind5(void);
void zombie_paind6(void);
void zombie_paind7(void);
void zombie_paind8(void);
void zombie_paind9(void);
void zombie_paind10(void);
void zombie_paind11(void);
void zombie_paind12(void);
void zombie_paind13(void);
void zombie_paine1(void);
void zombie_paine2(void);
void zombie_paine3(void);
void zombie_paine4(void);
void zombie_paine5(void);
void zombie_paine6(void);
void zombie_paine7(void);
void zombie_paine8(void);
void zombie_paine9(void);
void zombie_paine10(void);
void zombie_paine11(void);
void zombie_paine12(void);
void zombie_paine13(void);
void zombie_paine14(void);
void zombie_paine15(void);
void zombie_paine16(void);
void zombie_paine17(void);
void zombie_paine18(void);
void zombie_paine19(void);
void zombie_paine20(void);
void zombie_paine21(void);
void zombie_paine22(void);
void zombie_paine23(void);
void zombie_paine24(void);
void zombie_paine25(void);
void zombie_paine26(void);
void zombie_paine27(void);
void zombie_paine28(void);
void zombie_paine29(void);
void zombie_paine30(void);

const int SPAWN_CRUCIFIED = 1;

//=============================================================================

ANIM(zombie_stand1, stand1, zombie_stand2; ai_stand();)
ANIM(zombie_stand2, stand2, zombie_stand3; ai_stand();)
ANIM(zombie_stand3, stand3, zombie_stand4; ai_stand();)
ANIM(zombie_stand4, stand4, zombie_stand5; ai_stand();)
ANIM(zombie_stand5, stand5, zombie_stand6; ai_stand();)
ANIM(zombie_stand6, stand6, zombie_stand7; ai_stand();)
ANIM(zombie_stand7, stand7, zombie_stand8; ai_stand();)
ANIM(zombie_stand8, stand8, zombie_stand9; ai_stand();)
ANIM(zombie_stand9, stand9, zombie_stand10; ai_stand();)
ANIM(zombie_stand10, stand10, zombie_stand11; ai_stand();)
ANIM(zombie_stand11, stand11, zombie_stand12; ai_stand();)
ANIM(zombie_stand12, stand12, zombie_stand13; ai_stand();)
ANIM(zombie_stand13, stand13, zombie_stand14; ai_stand();)
ANIM(zombie_stand14, stand14, zombie_stand15; ai_stand();)
ANIM(zombie_stand15, stand15, zombie_stand1; ai_stand();)

void _zombie_cruc1(void)
{
	if (g_random() < 0.1)
	{
		sound(self, CHAN_VOICE, "zombie/idle_w2.wav", 1, ATTN_STATIC);
	}
}

ANIM(zombie_cruc1, cruc_1, zombie_cruc2; _zombie_cruc1();)
ANIM(zombie_cruc2, cruc_2,
		zombie_cruc3; self->s.v.nextthink = g_globalvars.time + 0.1 + g_random() * 0.1;)
ANIM(zombie_cruc3, cruc_3,
		zombie_cruc4; self->s.v.nextthink = g_globalvars.time + 0.1 + g_random() * 0.1;)
ANIM(zombie_cruc4, cruc_4,
		zombie_cruc5; self->s.v.nextthink = g_globalvars.time + 0.1 + g_random() * 0.1;)
ANIM(zombie_cruc5, cruc_5,
		zombie_cruc6; self->s.v.nextthink = g_globalvars.time + 0.1 + g_random() * 0.1;)
ANIM(zombie_cruc6, cruc_6,
		zombie_cruc1; self->s.v.nextthink = g_globalvars.time + 0.1 + g_random() * 0.1;)

void _zombie_walk19(void)
{
	ai_walk(0);
	if (g_random() < 0.2)
	{
		sound(self, CHAN_VOICE, "zombie/z_idle.wav", 1, ATTN_IDLE);
	}
}

ANIM(zombie_walk1, walk1, zombie_walk2; ai_walk(0);)
ANIM(zombie_walk2, walk2, zombie_walk3; ai_walk(2);)
ANIM(zombie_walk3, walk3, zombie_walk4; ai_walk(3);)
ANIM(zombie_walk4, walk4, zombie_walk5; ai_walk(2);)
ANIM(zombie_walk5, walk5, zombie_walk6; ai_walk(1);)
ANIM(zombie_walk6, walk6, zombie_walk7; ai_walk(0);)
ANIM(zombie_walk7, walk7, zombie_walk8; ai_walk(0);)
ANIM(zombie_walk8, walk8, zombie_walk9; ai_walk(0);)
ANIM(zombie_walk9, walk9, zombie_walk10; ai_walk(0);)
ANIM(zombie_walk10, walk10, zombie_walk11; ai_walk(0);)
ANIM(zombie_walk11, walk11, zombie_walk12; ai_walk(2);)
ANIM(zombie_walk12, walk12, zombie_walk13; ai_walk(2);)
ANIM(zombie_walk13, walk13, zombie_walk14; ai_walk(1);)
ANIM(zombie_walk14, walk14, zombie_walk15; ai_walk(0);)
ANIM(zombie_walk15, walk15, zombie_walk16; ai_walk(0);)
ANIM(zombie_walk16, walk16, zombie_walk17; ai_walk(0);)
ANIM(zombie_walk17, walk17, zombie_walk18; ai_walk(0);)
ANIM(zombie_walk18, walk18, zombie_walk19; ai_walk(0);)
ANIM(zombie_walk19, walk19, zombie_walk1; _zombie_walk19();)

void _zombie_run18(void)
{
	ai_run(8);

	if (g_random() < 0.2)
	{
		sound(self, CHAN_VOICE, "zombie/z_idle.wav", 1, ATTN_IDLE);
	}

	if (g_random() > 0.8)
	{
		sound(self, CHAN_VOICE, "zombie/z_idle1.wav", 1, ATTN_IDLE);
	}
}

ANIM(zombie_run1, run1, zombie_run2; ai_run(1))
ANIM(zombie_run2, run2, zombie_run3; ai_run(1);)
ANIM(zombie_run3, run3, zombie_run4; ai_run(0);)
ANIM(zombie_run4, run4, zombie_run5; ai_run(1);)
ANIM(zombie_run5, run5, zombie_run6; ai_run(2);)
ANIM(zombie_run6, run6, zombie_run7; ai_run(3);)
ANIM(zombie_run7, run7, zombie_run8; ai_run(4);)
ANIM(zombie_run8, run8, zombie_run9; ai_run(4);)
ANIM(zombie_run9, run9, zombie_run10; ai_run(2);)
ANIM(zombie_run10, run10, zombie_run11; ai_run(0);)
ANIM(zombie_run11, run11, zombie_run12; ai_run(0);)
ANIM(zombie_run12, run12, zombie_run13; ai_run(0);)
ANIM(zombie_run13, run13, zombie_run14; ai_run(2);)
ANIM(zombie_run14, run14, zombie_run15; ai_run(4);)
ANIM(zombie_run15, run15, zombie_run16; ai_run(6);)
ANIM(zombie_run16, run16, zombie_run17; ai_run(7);)
ANIM(zombie_run17, run17, zombie_run18; ai_run(3);)
ANIM(zombie_run18, run18, zombie_run1; _zombie_run18();)

/*
 =============================================================================

 ATTACKS

 =============================================================================
 */

void ZombieGrenadeTouch(void)
{
	if (other == PROG_TO_EDICT(self->s.v.owner))
	{
		return;	// don't explode on owner
	}

	if (other->s.v.takedamage)
	{
		other->deathtype = dtSQUISH; // FIXME
		T_Damage(other, self, PROG_TO_EDICT(self->s.v.owner), 10);
		sound(self, CHAN_WEAPON, "zombie/z_hit.wav", 1, ATTN_NORM);
		ent_remove(self);

		return;
	}

	sound(self, CHAN_WEAPON, "zombie/z_miss.wav", 1, ATTN_NORM);	// bounce sound
	VectorCopy(VEC_ORIGIN, self->s.v.velocity);
	VectorCopy(VEC_ORIGIN, self->s.v.avelocity);
	self->touch = (func_t) SUB_Remove;
}

/*
 ================
 ZombieFireGrenade
 ================
 */
void ZombieFireGrenade(float st_x, float st_y, float st_z)
{
	gedict_t *missile;
	vec3_t org;

	sound(self, CHAN_WEAPON, "zombie/z_shot1.wav", 1, ATTN_NORM);

	missile = spawn();
	missile->s.v.owner = EDICT_TO_PROG(self);
	missile->s.v.movetype = MOVETYPE_BOUNCE;
	missile->isMissile = true;
	missile->s.v.solid = SOLID_BBOX;

	// calc org
	trap_makevectors(self->s.v.angles);

	//org = self->s.v.origin + st[0] * v_forward + st[1] * v_right + st[2] * v_up;
	VectorMA(self->s.v.origin, st_x, g_globalvars.v_forward, org);
	VectorMA(org, st_y, g_globalvars.v_right, org);
	VectorMA(org, st_z, g_globalvars.v_up, org);

	// set missile speed
	VectorSubtract(PROG_TO_EDICT(self->s.v.enemy)->s.v.origin, org, missile->s.v.velocity);
	normalize(missile->s.v.velocity, missile->s.v.velocity);
	VectorScale(missile->s.v.velocity, 600 + 100 * g_random(), missile->s.v.velocity);
	missile->s.v.velocity[2] += 200 + 50 * g_random();

	SetVector(missile->s.v.avelocity, 3000, 1000, 2000);

	missile->touch = (func_t) ZombieGrenadeTouch;

	// set missile duration
	missile->s.v.nextthink = g_globalvars.time + 2.5 + g_random();
	missile->think = (func_t) SUB_Remove;

	setmodel(missile, "progs/zom_gib.mdl");
	setsize(missile, PASSVEC3(VEC_ORIGIN), PASSVEC3(VEC_ORIGIN));
	setorigin(missile, PASSVEC3(org));
}

ANIM(zombie_atta1, atta1, zombie_atta2; ai_face();)
ANIM(zombie_atta2, atta2, zombie_atta3; ai_face();)
ANIM(zombie_atta3, atta3, zombie_atta4; ai_face();)
ANIM(zombie_atta4, atta4, zombie_atta5; ai_face();)
ANIM(zombie_atta5, atta5, zombie_atta6; ai_face();)
ANIM(zombie_atta6, atta6, zombie_atta7; ai_face();)
ANIM(zombie_atta7, atta7, zombie_atta8; ai_face();)
ANIM(zombie_atta8, atta8, zombie_atta9; ai_face();)
ANIM(zombie_atta9, atta9, zombie_atta10; ai_face();)
ANIM(zombie_atta10, atta10, zombie_atta11; ai_face();)
ANIM(zombie_atta11, atta11, zombie_atta12; ai_face();)
ANIM(zombie_atta12, atta12, zombie_atta13; ai_face();)
ANIM(zombie_atta13, atta13, zombie_run1; ai_face(); ZombieFireGrenade( -10, 14, 20 );)

ANIM(zombie_attb1, attb1, zombie_attb2; ai_face();)
ANIM(zombie_attb2, attb2, zombie_attb3; ai_face();)
ANIM(zombie_attb3, attb3, zombie_attb4; ai_face();)
ANIM(zombie_attb4, attb4, zombie_attb5; ai_face();)
ANIM(zombie_attb5, attb5, zombie_attb6; ai_face();)
ANIM(zombie_attb6, attb6, zombie_attb7; ai_face();)
ANIM(zombie_attb7, attb7, zombie_attb8; ai_face();)
ANIM(zombie_attb8, attb8, zombie_attb9; ai_face();)
ANIM(zombie_attb9, attb9, zombie_attb10; ai_face();)
ANIM(zombie_attb10, attb10, zombie_attb11; ai_face();)
ANIM(zombie_attb11, attb11, zombie_attb12; ai_face();)
ANIM(zombie_attb12, attb12, zombie_attb13; ai_face();)
ANIM(zombie_attb13, attb13, zombie_attb14; ai_face();)
ANIM(zombie_attb14, attb13, zombie_run1; ai_face(); ZombieFireGrenade( -10, -10, 20 );)

ANIM(zombie_attc1, attc1, zombie_attc2; ai_face();)
ANIM(zombie_attc2, attc2, zombie_attc3; ai_face();)
ANIM(zombie_attc3, attc3, zombie_attc4; ai_face();)
ANIM(zombie_attc4, attc4, zombie_attc5; ai_face();)
ANIM(zombie_attc5, attc5, zombie_attc6; ai_face();)
ANIM(zombie_attc6, attc6, zombie_attc7; ai_face();)
ANIM(zombie_attc7, attc7, zombie_attc8; ai_face();)
ANIM(zombie_attc8, attc8, zombie_attc9; ai_face();)
ANIM(zombie_attc9, attc9, zombie_attc10; ai_face();)
ANIM(zombie_attc10, attc10, zombie_attc11; ai_face();)
ANIM(zombie_attc11, attc11, zombie_attc12; ai_face();)
ANIM(zombie_attc12, attc12, zombie_run1; ai_face(); ZombieFireGrenade( -10, 14, 20 );)

void zombie_missile(void)
{
	float r;

	r = g_random();

	if (r < 0.3)
	{
		zombie_atta1();
	}
	else if (r < 0.6)
	{
		zombie_attb1();
	}
	else
	{
		zombie_attc1();
	}
}

/*
 =============================================================================

 PAIN

 =============================================================================
 */

ANIM(zombie_paina1, paina1,
		zombie_paina2; sound( self, CHAN_VOICE, "zombie/z_pain.wav", 1, ATTN_NORM );)
ANIM(zombie_paina2, paina2, zombie_paina3; ai_painforward(3);)
ANIM(zombie_paina3, paina3, zombie_paina4; ai_painforward(1);)
ANIM(zombie_paina4, paina4, zombie_paina5; ai_pain(1);)
ANIM(zombie_paina5, paina5, zombie_paina6; ai_pain(3);)
ANIM(zombie_paina6, paina6, zombie_paina7; ai_pain(1);)
ANIM(zombie_paina7, paina7, zombie_paina8;)
ANIM(zombie_paina8, paina8, zombie_paina9;)
ANIM(zombie_paina9, paina9, zombie_paina10;)
ANIM(zombie_paina10, paina10, zombie_paina11;)
ANIM(zombie_paina11, paina11, zombie_paina12;)
ANIM(zombie_paina12, paina12, zombie_run1;)

ANIM(zombie_painb1, painb1,
		zombie_painb2; sound( self, CHAN_VOICE, "zombie/z_pain1.wav", 1, ATTN_NORM );)
ANIM(zombie_painb2, painb2, zombie_painb3; ai_pain(2);)
ANIM(zombie_painb3, painb3, zombie_painb4; ai_pain(8);)
ANIM(zombie_painb4, painb4, zombie_painb5; ai_pain(6);)
ANIM(zombie_painb5, painb5, zombie_painb6; ai_pain(2);)
ANIM(zombie_painb6, painb6, zombie_painb7;)
ANIM(zombie_painb7, painb7, zombie_painb8;)
ANIM(zombie_painb8, painb8, zombie_painb9;)
ANIM(zombie_painb9, painb9,
		zombie_painb10; sound( self, CHAN_BODY, "zombie/z_fall.wav", 1, ATTN_NORM );)
ANIM(zombie_painb10, painb10, zombie_painb11;)
ANIM(zombie_painb11, painb11, zombie_painb12;)
ANIM(zombie_painb12, painb12, zombie_painb13;)
ANIM(zombie_painb13, painb13, zombie_painb14;)
ANIM(zombie_painb14, painb14, zombie_painb15;)
ANIM(zombie_painb15, painb15, zombie_painb16;)
ANIM(zombie_painb16, painb16, zombie_painb17;)
ANIM(zombie_painb17, painb17, zombie_painb18;)
ANIM(zombie_painb18, painb18, zombie_painb19;)
ANIM(zombie_painb19, painb19, zombie_painb20;)
ANIM(zombie_painb20, painb20, zombie_painb21;)
ANIM(zombie_painb21, painb21, zombie_painb22;)
ANIM(zombie_painb22, painb22, zombie_painb23;)
ANIM(zombie_painb23, painb23, zombie_painb24;)
ANIM(zombie_painb24, painb24, zombie_painb25;)
ANIM(zombie_painb25, painb25, zombie_painb26; ai_painforward(1);)
ANIM(zombie_painb26, painb26, zombie_painb27;)
ANIM(zombie_painb27, painb27, zombie_painb28;)
ANIM(zombie_painb28, painb28, zombie_run1;)

ANIM(zombie_painc1, painc1,
		zombie_painc2; sound( self, CHAN_VOICE, "zombie/z_pain1.wav", 1, ATTN_NORM );)
ANIM(zombie_painc2, painc2, zombie_painc3;)
ANIM(zombie_painc3, painc3, zombie_painc4; ai_pain(3);)
ANIM(zombie_painc4, painc4, zombie_painc5; ai_pain(1);)
ANIM(zombie_painc5, painc5, zombie_painc6;)
ANIM(zombie_painc6, painc6, zombie_painc7;)
ANIM(zombie_painc7, painc7, zombie_painc8;)
ANIM(zombie_painc8, painc8, zombie_painc9;)
ANIM(zombie_painc9, painc9, zombie_painc10;)
ANIM(zombie_painc10, painc10, zombie_painc11;)
ANIM(zombie_painc11, painc11, zombie_painc12; ai_painforward(1);)
ANIM(zombie_painc12, painc12, zombie_painc13; ai_painforward(1);)
ANIM(zombie_painc13, painc13, zombie_painc14;)
ANIM(zombie_painc14, painc14, zombie_painc15;)
ANIM(zombie_painc15, painc15, zombie_painc16;)
ANIM(zombie_painc16, painc16, zombie_painc17;)
ANIM(zombie_painc17, painc17, zombie_painc18;)
ANIM(zombie_painc18, painc18, zombie_run1;)

ANIM(zombie_paind1, paind1,
		zombie_paind2; sound( self, CHAN_VOICE, "zombie/z_pain.wav", 1, ATTN_NORM );)
ANIM(zombie_paind2, paind2, zombie_paind3;)
ANIM(zombie_paind3, paind3, zombie_paind4;)
ANIM(zombie_paind4, paind4, zombie_paind5;)
ANIM(zombie_paind5, paind5, zombie_paind6;)
ANIM(zombie_paind6, paind6, zombie_paind7;)
ANIM(zombie_paind7, paind7, zombie_paind8;)
ANIM(zombie_paind8, paind8, zombie_paind9;)
ANIM(zombie_paind9, paind9, zombie_paind10; ai_pain(1);)
ANIM(zombie_paind10, paind10, zombie_paind11;)
ANIM(zombie_paind11, paind11, zombie_paind12;)
ANIM(zombie_paind12, paind12, zombie_paind13;)
ANIM(zombie_paind13, paind13, zombie_run1;)

void _zombie_paine1(void)
{
	sound(self, CHAN_VOICE, "zombie/z_pain.wav", 1, ATTN_NORM);
}

void _zombie_paine10(void)
{
	sound(self, CHAN_BODY, "zombie/z_fall.wav", 1, ATTN_NORM);

	self->s.v.solid = SOLID_NOT;
}

void _zombie_paine11(void)
{
	self->s.v.nextthink = g_globalvars.time + 3 + g_random() + g_random(); // randozime wake up a bit
}

void _zombie_paine12(void)
{
	sound(self, CHAN_VOICE, "zombie/z_idle.wav", 1, ATTN_IDLE);

	self->s.v.solid = SOLID_SLIDEBOX; // this set this before checking walkmove()

	// see if ok to stand up
	if (!walkmove(self, 0, 0))
	{
		// not ok, delay wake up in zombie_paine11
		self->s.v.solid = SOLID_NOT; // return back non solid state
		self->think = (func_t) zombie_paine11;

		return;
	}
}

ANIM(zombie_paine1, paine1, zombie_paine2; _zombie_paine1();)
ANIM(zombie_paine2, paine2, zombie_paine3; ai_pain(8);)
ANIM(zombie_paine3, paine3, zombie_paine4; ai_pain(5);)
ANIM(zombie_paine4, paine4, zombie_paine5; ai_pain(3);)
ANIM(zombie_paine5, paine5, zombie_paine6; ai_pain(1);)
ANIM(zombie_paine6, paine6, zombie_paine7; ai_pain(2);)
ANIM(zombie_paine7, paine7, zombie_paine8; ai_pain(1);)
ANIM(zombie_paine8, paine8, zombie_paine9; ai_pain(1);)
ANIM(zombie_paine9, paine9, zombie_paine10; ai_pain(2);)
ANIM(zombie_paine10, paine10, zombie_paine11; _zombie_paine10();)
ANIM(zombie_paine11, paine11, zombie_paine12; _zombie_paine11();)
ANIM(zombie_paine12, paine12, zombie_paine13; _zombie_paine12();)
ANIM(zombie_paine13, paine13, zombie_paine14;)
ANIM(zombie_paine14, paine14, zombie_paine15;)
ANIM(zombie_paine15, paine15, zombie_paine16;)
ANIM(zombie_paine16, paine16, zombie_paine17;)
ANIM(zombie_paine17, paine17, zombie_paine18;)
ANIM(zombie_paine18, paine18, zombie_paine19;)
ANIM(zombie_paine19, paine19, zombie_paine20;)
ANIM(zombie_paine20, paine20, zombie_paine21;)
ANIM(zombie_paine21, paine21, zombie_paine22;)
ANIM(zombie_paine22, paine22, zombie_paine23;)
ANIM(zombie_paine23, paine23, zombie_paine24;)
ANIM(zombie_paine24, paine24, zombie_paine25;)
ANIM(zombie_paine25, paine25, zombie_paine26; ai_painforward(5);)
ANIM(zombie_paine26, paine26, zombie_paine27; ai_painforward(3);)
ANIM(zombie_paine27, paine27, zombie_paine28; ai_painforward(1);)
ANIM(zombie_paine28, paine28, zombie_paine29; ai_pain(1);)
ANIM(zombie_paine29, paine29, zombie_paine30;)
ANIM(zombie_paine30, paine30, zombie_run1;)

/*
 =================
 zombie_pain

 Zombies can only be killed (gibbed) by doing 60 hit points of damage
 in a single frame (rockets, grenades, quad shotgun, quad nailgun).

 A hit of 25 points or more (super shotgun, quad nailgun) will always put it
 down to the ground.

 A hit of from 10 to 40 points in one frame will cause it to go down if it
 has been twice in two seconds, otherwise it goes into one of the four
 fast pain frames.

 A hit of less than 10 points of damage (winged by a shotgun) will be ignored.

 FIXME: don't use pain_finished because of nightmare hack
 =================
 */
void zombie_pain(gedict_t *attacker, float take)
{
	float r;

	self->s.v.health = 60; // always reset health

	if (take < 9)
	{
		return;	// totally ignore
	}

	if (frame_is_long_zombie_pain(self->s.v.frame))
	{
		return;	// down on ground, so don't reset any counters
	}

	// go down immediately if a big enough hit
	if (take >= 25)
	{
		zombie_paine1();

		return;
	}

	// if alredy in pain, drop down
	if (frame_is_fast_zombie_pain(self->s.v.frame))
	{
		zombie_paine1();

		return;
	}

	// go into one of the fast pain animations

	r = g_random();

	if (r < 0.25)
	{
		zombie_paina1();
	}
	else if (r < 0.5)
	{
		zombie_painb1();
	}
	else if (r < 0.75)
	{
		zombie_painc1();
	}
	else
	{
		zombie_paind1();
	}
}

void zombie_die(void)
{
	sound(self, CHAN_VOICE, "zombie/z_gib.wav", 1, ATTN_NORM);
	ThrowHead("progs/h_zombie.mdl", self->s.v.health);
	ThrowGib("progs/gib1.mdl", self->s.v.health);
	ThrowGib("progs/gib2.mdl", self->s.v.health);
	ThrowGib("progs/gib3.mdl", self->s.v.health);

	self->s.v.nextthink = -1;
}

//============================================================================

/*QUAKED monster_zombie (1 0 0) (-16 -16 -24) (16 16 32) Crucified ambush

 If crucified, stick the bounding box 12 pixels back into a wall to look right.
 */
void SP_monster_zombie(void)
{
	if (!AllowMonster(self))
	{
		ent_remove(self);

		return;
	}

	safe_precache_model("progs/zombie.mdl");
	safe_precache_model("progs/h_zombie.mdl");
	safe_precache_model("progs/zom_gib.mdl");

	safe_precache_sound("zombie/z_idle.wav");
	safe_precache_sound("zombie/z_idle1.wav");
	safe_precache_sound("zombie/z_shot1.wav");
	safe_precache_sound("zombie/z_gib.wav");
	safe_precache_sound("zombie/z_pain.wav");
	safe_precache_sound("zombie/z_pain1.wav");
	safe_precache_sound("zombie/z_fall.wav");
	safe_precache_sound("zombie/z_miss.wav");
	safe_precache_sound("zombie/z_hit.wav");
	safe_precache_sound("zombie/idle_w2.wav");

	setsize(self, -16, -16, -24, 16, 16, 40);
	self->s.v.health = 60;

	self->th_stand = zombie_stand1;
	self->th_walk = zombie_walk1;
	self->th_run = zombie_run1;
	self->th_pain = zombie_pain;
	self->th_die = zombie_die;
	self->th_missile = zombie_missile;

	self->th_respawn = SP_monster_zombie;

	if ((int)self->s.v.spawnflags & SPAWN_CRUCIFIED)
	{
		self->s.v.solid = SOLID_SLIDEBOX;
		self->s.v.movetype = MOVETYPE_NONE;
		setmodel(self, "progs/zombie.mdl");
		zombie_cruc1();
	}
	else
	{
		walkmonster_start("progs/zombie.mdl");
	}
}
