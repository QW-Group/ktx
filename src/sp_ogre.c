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

 OGRE

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

	run1,
	run2,
	run3,
	run4,
	run5,
	run6,
	run7,
	run8,

	swing1,
	swing2,
	swing3,
	swing4,
	swing5,
	swing6,
	swing7,
	swing8,
	swing9,
	swing10,
	swing11,
	swing12,
	swing13,
	swing14,

	smash1,
	smash2,
	smash3,
	smash4,
	smash5,
	smash6,
	smash7,
	smash8,
	smash9,
	smash10,
	smash11,
	smash12,
	smash13,
	smash14,

	shoot1,
	shoot2,
	shoot3,
	shoot4,
	shoot5,
	shoot6,

	pain1,
	pain2,
	pain3,
	pain4,
	pain5,

	painb1,
	painb2,
	painb3,

	painc1,
	painc2,
	painc3,
	painc4,
	painc5,
	painc6,

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
	paind14,
	paind15,
	paind16,

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

	bdeath1,
	bdeath2,
	bdeath3,
	bdeath4,
	bdeath5,
	bdeath6,
	bdeath7,
	bdeath8,
	bdeath9,
	bdeath10,

	pull1,
	pull2,
	pull3,
	pull4,
	pull5,
	pull6,
	pull7,
	pull8,
	pull9,
	pull10,
	pull11,

};

void ogre_stand1(void);
void ogre_stand2(void);
void ogre_stand3(void);
void ogre_stand4(void);
void ogre_stand5(void);
void ogre_stand6(void);
void ogre_stand7(void);
void ogre_stand8(void);
void ogre_stand9(void);
void ogre_walk1(void);
void ogre_walk2(void);
void ogre_walk3(void);
void ogre_walk4(void);
void ogre_walk5(void);
void ogre_walk6(void);
void ogre_walk7(void);
void ogre_walk8(void);
void ogre_walk9(void);
void ogre_walk10(void);
void ogre_walk11(void);
void ogre_walk12(void);
void ogre_walk13(void);
void ogre_walk14(void);
void ogre_walk15(void);
void ogre_walk16(void);
void ogre_run1(void);
void ogre_run2(void);
void ogre_run3(void);
void ogre_run4(void);
void ogre_run5(void);
void ogre_run6(void);
void ogre_run7(void);
void ogre_run8(void);
void ogre_swing1(void);
void ogre_swing2(void);
void ogre_swing3(void);
void ogre_swing4(void);
void ogre_swing5(void);
void ogre_swing6(void);
void ogre_swing7(void);
void ogre_swing8(void);
void ogre_swing9(void);
void ogre_swing10(void);
void ogre_swing11(void);
void ogre_swing12(void);
void ogre_swing13(void);
void ogre_swing14(void);
void ogre_smash1(void);
void ogre_smash2(void);
void ogre_smash3(void);
void ogre_smash4(void);
void ogre_smash5(void);
void ogre_smash6(void);
void ogre_smash7(void);
void ogre_smash8(void);
void ogre_smash9(void);
void ogre_smash10(void);
void ogre_smash11(void);
void ogre_smash12(void);
void ogre_smash13(void);
void ogre_smash14(void);
void ogre_nail1(void);
void ogre_nail2(void);
void ogre_nail3(void);
void ogre_nail4(void);
void ogre_nail5(void);
void ogre_nail6(void);
void ogre_nail7(void);
void ogre_pain1(void);
void ogre_pain2(void);
void ogre_pain3(void);
void ogre_pain4(void);
void ogre_pain5(void);
void ogre_painb1(void);
void ogre_painb2(void);
void ogre_painb3(void);
void ogre_painc1(void);
void ogre_painc2(void);
void ogre_painc3(void);
void ogre_painc4(void);
void ogre_painc5(void);
void ogre_painc6(void);
void ogre_paind1(void);
void ogre_paind2(void);
void ogre_paind3(void);
void ogre_paind4(void);
void ogre_paind5(void);
void ogre_paind6(void);
void ogre_paind7(void);
void ogre_paind8(void);
void ogre_paind9(void);
void ogre_paind10(void);
void ogre_paind11(void);
void ogre_paind12(void);
void ogre_paind13(void);
void ogre_paind14(void);
void ogre_paind15(void);
void ogre_paind16(void);
void ogre_paine1(void);
void ogre_paine2(void);
void ogre_paine3(void);
void ogre_paine4(void);
void ogre_paine5(void);
void ogre_paine6(void);
void ogre_paine7(void);
void ogre_paine8(void);
void ogre_paine9(void);
void ogre_paine10(void);
void ogre_paine11(void);
void ogre_paine12(void);
void ogre_paine13(void);
void ogre_paine14(void);
void ogre_paine15(void);
void ogre_die1(void);
void ogre_die2(void);
void ogre_die3(void);
void ogre_die4(void);
void ogre_die5(void);
void ogre_die6(void);
void ogre_die7(void);
void ogre_die8(void);
void ogre_die9(void);
void ogre_die10(void);
void ogre_die11(void);
void ogre_die12(void);
void ogre_die13(void);
void ogre_die14(void);
void ogre_bdie1(void);
void ogre_bdie2(void);
void ogre_bdie3(void);
void ogre_bdie4(void);
void ogre_bdie5(void);
void ogre_bdie6(void);
void ogre_bdie7(void);
void ogre_bdie8(void);
void ogre_bdie9(void);
void ogre_bdie10(void);

//=============================================================================

void _ogre_stand5(void)
{
	if (g_random() < 0.2)
	{
		sound(self, CHAN_VOICE, "ogre/ogidle.wav", 1, ATTN_IDLE);
	}

	ai_stand();
}

ANIM(ogre_stand1, stand1, ogre_stand2; ai_stand();)
ANIM(ogre_stand2, stand2, ogre_stand3; ai_stand();)
ANIM(ogre_stand3, stand3, ogre_stand4; ai_stand();)
ANIM(ogre_stand4, stand4, ogre_stand5; ai_stand();)
ANIM(ogre_stand5, stand5, ogre_stand6; _ogre_stand5();)
ANIM(ogre_stand6, stand6, ogre_stand7; ai_stand();)
ANIM(ogre_stand7, stand7, ogre_stand8; ai_stand();)
ANIM(ogre_stand8, stand8, ogre_stand9; ai_stand();)
ANIM(ogre_stand9, stand9, ogre_stand1; ai_stand();)

void _ogre_walk3(void)
{
	ai_walk(2);

	if (g_random() < 0.2)
	{
		sound(self, CHAN_VOICE, "ogre/ogidle.wav", 1, ATTN_IDLE);
	}
}

void _ogre_walk6(void)
{
	ai_walk(5);

	if (g_random() < 0.1)
	{
		sound(self, CHAN_VOICE, "ogre/ogdrag.wav", 1, ATTN_IDLE);
	}
}

ANIM(ogre_walk1, walk1, ogre_walk2; ai_walk(3);)
ANIM(ogre_walk2, walk2, ogre_walk3; ai_walk(2);)
ANIM(ogre_walk3, walk3, ogre_walk4; _ogre_walk3();)
ANIM(ogre_walk4, walk4, ogre_walk5; ai_walk(2);)
ANIM(ogre_walk5, walk5, ogre_walk6; ai_walk(2);)
ANIM(ogre_walk6, walk6, ogre_walk7; _ogre_walk6();)
ANIM(ogre_walk7, walk7, ogre_walk8; ai_walk(3);)
ANIM(ogre_walk8, walk8, ogre_walk9; ai_walk(2);)
ANIM(ogre_walk9, walk9, ogre_walk10; ai_walk(3);)
ANIM(ogre_walk10, walk10, ogre_walk11; ai_walk(1);)
ANIM(ogre_walk11, walk11, ogre_walk12; ai_walk(2);)
ANIM(ogre_walk12, walk12, ogre_walk13; ai_walk(3);)
ANIM(ogre_walk13, walk13, ogre_walk14; ai_walk(3);)
ANIM(ogre_walk14, walk14, ogre_walk15; ai_walk(3);)
ANIM(ogre_walk15, walk15, ogre_walk16; ai_walk(3);)
ANIM(ogre_walk16, walk16, ogre_walk1; ai_walk(4);)

void _ogre_run1(void)
{
	ai_run(9);

	if (g_random() < 0.2)
	{
		sound(self, CHAN_VOICE, "ogre/ogidle2.wav", 1, ATTN_IDLE);
	}
}

ANIM(ogre_run1, run1, ogre_run2; _ogre_run1();)
ANIM(ogre_run2, run2, ogre_run3; ai_run(12);)
ANIM(ogre_run3, run3, ogre_run4; ai_run(8);)
ANIM(ogre_run4, run4, ogre_run5; ai_run(22);)
ANIM(ogre_run5, run5, ogre_run6; ai_run(16);)
ANIM(ogre_run6, run6, ogre_run7; ai_run(4);)
ANIM(ogre_run7, run7, ogre_run8; ai_run(13);)
ANIM(ogre_run8, run8, ogre_run1; ai_run(24);)

void chainsaw(float side)
{
	vec3_t delta, vel;
	float ldmg;

	if (!self->s.v.enemy)
	{
		return;
	}

	if (!CanDamage(PROG_TO_EDICT(self->s.v.enemy), self))
	{
		return;
	}

	ai_charge(10);

	VectorSubtract(PROG_TO_EDICT(self->s.v.enemy)->s.v.origin, self->s.v.origin, delta);

	if (vlen(delta) > 100)
	{
		return;
	}

	ldmg = (g_random() + g_random() + g_random()) * 4;
	PROG_TO_EDICT(self->s.v.enemy)->deathtype = dtSQUISH; // FIXME
	T_Damage(PROG_TO_EDICT(self->s.v.enemy), self, self, ldmg);

	// in most cases chainsaw called as chainsaw( 0 ) so side is 0
	if (side)
	{
		float scale = (side == 1 ? crandom() * 100 : side);

		trap_makevectors(self->s.v.angles);

		VectorMA(self->s.v.origin, 16, g_globalvars.v_forward, delta);
		VectorScale(g_globalvars.v_right, scale, vel);

		if (side == 1)
		{
			SpawnMeatSpray(delta, vel);
		}
		else
		{
			SpawnMeatSpray(delta, vel);
		}
	}
}

ANIM(ogre_swing1, swing1,
		ogre_swing2; ai_charge(11); sound( self, CHAN_WEAPON, "ogre/ogsawatk.wav", 1, ATTN_NORM );)
ANIM(ogre_swing2, swing2, ogre_swing3; ai_charge(1);)
ANIM(ogre_swing3, swing3, ogre_swing4; ai_charge(4);)
ANIM(ogre_swing4, swing4, ogre_swing5; ai_charge(13);)
ANIM(ogre_swing5, swing5,
		ogre_swing6; ai_charge(9); chainsaw(0); self->s.v.angles[1] += g_random() * 25;)
ANIM(ogre_swing6, swing6, ogre_swing7; chainsaw(200); self->s.v.angles[1] += g_random() * 25;)
ANIM(ogre_swing7, swing7, ogre_swing8; chainsaw(0); self->s.v.angles[1] += g_random() * 25;)
ANIM(ogre_swing8, swing8, ogre_swing9; chainsaw(0); self->s.v.angles[1] += g_random() * 25;)
ANIM(ogre_swing9, swing9, ogre_swing10; chainsaw(0); self->s.v.angles[1] += g_random() * 25;)
ANIM(ogre_swing10, swing10, ogre_swing11; chainsaw(-200); self->s.v.angles[1] += g_random() * 25;)
ANIM(ogre_swing11, swing11, ogre_swing12; chainsaw(0); self->s.v.angles[1] += g_random() * 25;)
ANIM(ogre_swing12, swing12, ogre_swing13; ai_charge(3);)
ANIM(ogre_swing13, swing13, ogre_swing14; ai_charge(8);)
ANIM(ogre_swing14, swing14, ogre_run1; ai_charge(9);)

void _ogre_smash11(void)
{
	ai_charge(2);

	chainsaw(0);

	// slight variation	
	self->s.v.nextthink = g_globalvars.time + g_random() * 0.2;
}

ANIM(ogre_smash1, smash1,
		ogre_smash2; ai_charge(6); sound( self, CHAN_WEAPON, "ogre/ogsawatk.wav", 1, ATTN_NORM );)
ANIM(ogre_smash2, smash2, ogre_smash3; ai_charge(0);)
ANIM(ogre_smash3, smash3, ogre_smash4; ai_charge(0);)
ANIM(ogre_smash4, smash4, ogre_smash5; ai_charge(1);)
ANIM(ogre_smash5, smash5, ogre_smash6; ai_charge(4);)
ANIM(ogre_smash6, smash6, ogre_smash7; ai_charge(4); chainsaw(0);)
ANIM(ogre_smash7, smash7, ogre_smash8; ai_charge(4); chainsaw(0);)
ANIM(ogre_smash8, smash8, ogre_smash9; ai_charge(10); chainsaw(0);)
ANIM(ogre_smash9, smash9, ogre_smash10; ai_charge(13); chainsaw(0);)
ANIM(ogre_smash10, smash10, ogre_smash11; chainsaw(1);)
ANIM(ogre_smash11, smash11, ogre_smash12; _ogre_smash11();)
ANIM(ogre_smash12, smash12, ogre_smash13; ai_charge(0);)
ANIM(ogre_smash13, smash13, ogre_smash14; ai_charge(4);)
ANIM(ogre_smash14, smash14, ogre_run1; ai_charge(12);)

void ogre_melee(void)
{
	if (g_random() > 0.5)
	{
		ogre_smash1();
	}
	else
	{
		ogre_swing1();
	}
}

//=============================================================================

void OgreGrenadeExplode(void)
{
	T_RadiusDamage(self, PROG_TO_EDICT(self->s.v.owner), 40, world, dtSQUISH);
	sound(self, CHAN_VOICE, "weapons/r_exp3.wav", 1, ATTN_NORM);

	WriteByte( MSG_BROADCAST, SVC_TEMPENTITY);
	WriteByte( MSG_BROADCAST, TE_EXPLOSION);
	WriteCoord( MSG_BROADCAST, self->s.v.origin[0]);
	WriteCoord( MSG_BROADCAST, self->s.v.origin[1]);
	WriteCoord( MSG_BROADCAST, self->s.v.origin[2]);
	trap_multicast(PASSVEC3(self->s.v.origin), MULTICAST_PHS);

	ent_remove(self);
}

void OgreGrenadeTouch(void)
{
	if (other == PROG_TO_EDICT(self->s.v.owner))
	{
		return;		// don't explode on owner
	}

	if (other->s.v.takedamage == DAMAGE_AIM)
	{
		OgreGrenadeExplode();

		return;
	}

	sound(self, CHAN_VOICE, "weapons/bounce.wav", 1, ATTN_NORM);	// bounce sound

	if (VectorCompare(self->s.v.velocity, VEC_ORIGIN))
	{
		VectorCopy(VEC_ORIGIN, self->s.v.avelocity);
	}
}

/*
 ================
 OgreFireGrenade
 ================
 */
void OgreFireGrenade(void)
{
	gedict_t *missile;

	muzzleflash();

	sound(self, CHAN_WEAPON, "weapons/grenade.wav", 1, ATTN_NORM);

	missile = spawn();
	missile->s.v.owner = EDICT_TO_PROG(self);
	missile->s.v.movetype = MOVETYPE_BOUNCE;
	missile->isMissile = true;
	missile->s.v.solid = SOLID_BBOX;

	// set missile speed
	trap_makevectors(self->s.v.angles);

	VectorSubtract(PROG_TO_EDICT(self->s.v.enemy)->s.v.origin, self->s.v.origin,
					missile->s.v.velocity);
	normalize(missile->s.v.velocity, missile->s.v.velocity);
	VectorScale(missile->s.v.velocity, 600, missile->s.v.velocity);
	missile->s.v.velocity[2] = 200;

	SetVector(missile->s.v.avelocity, 300, 300, 300);

	vectoangles(missile->s.v.velocity, missile->s.v.angles);

	missile->touch = (func_t) OgreGrenadeTouch;

	// set missile duration
	missile->s.v.nextthink = g_globalvars.time + 2.5;
	missile->think = (func_t) OgreGrenadeExplode;

	setmodel(missile, "progs/grenade.mdl");
	setsize(missile, PASSVEC3(VEC_ORIGIN), PASSVEC3(VEC_ORIGIN));
	setorigin(missile, PASSVEC3(self->s.v.origin));
}

ANIM(ogre_nail1, shoot1, ogre_nail2; ai_face();)
ANIM(ogre_nail2, shoot2, ogre_nail3; ai_face();)
ANIM(ogre_nail3, shoot2, ogre_nail4; ai_face();)
ANIM(ogre_nail4, shoot3, ogre_nail5; ai_face(); OgreFireGrenade();)
ANIM(ogre_nail5, shoot4, ogre_nail6; ai_face();)
ANIM(ogre_nail6, shoot5, ogre_nail7; ai_face();)
ANIM(ogre_nail7, shoot6, ogre_run1; ai_face();)

//=============================================================================

ANIM(ogre_pain1, pain1, ogre_pain2;)
ANIM(ogre_pain2, pain2, ogre_pain3;)
ANIM(ogre_pain3, pain3, ogre_pain4;)
ANIM(ogre_pain4, pain4, ogre_pain5;)
ANIM(ogre_pain5, pain5, ogre_run1;)

ANIM(ogre_painb1, painb1, ogre_painb2;)
ANIM(ogre_painb2, painb2, ogre_painb3;)
ANIM(ogre_painb3, painb3, ogre_run1;)

ANIM(ogre_painc1, painc1, ogre_painc2;)
ANIM(ogre_painc2, painc2, ogre_painc3;)
ANIM(ogre_painc3, painc3, ogre_painc4;)
ANIM(ogre_painc4, painc4, ogre_painc5;)
ANIM(ogre_painc5, painc5, ogre_painc6;)
ANIM(ogre_painc6, painc6, ogre_run1;)

ANIM(ogre_paind1, paind1, ogre_paind2;)
ANIM(ogre_paind2, paind2, ogre_paind3; ai_pain(10);)
ANIM(ogre_paind3, paind3, ogre_paind4; ai_pain(9);)
ANIM(ogre_paind4, paind4, ogre_paind5; ai_pain(4);)
ANIM(ogre_paind5, paind5, ogre_paind6;)
ANIM(ogre_paind6, paind6, ogre_paind7;)
ANIM(ogre_paind7, paind7, ogre_paind8;)
ANIM(ogre_paind8, paind8, ogre_paind9;)
ANIM(ogre_paind9, paind9, ogre_paind10;)
ANIM(ogre_paind10, paind10, ogre_paind11;)
ANIM(ogre_paind11, paind11, ogre_paind12;)
ANIM(ogre_paind12, paind12, ogre_paind13;)
ANIM(ogre_paind13, paind13, ogre_paind14;)
ANIM(ogre_paind14, paind14, ogre_paind15;)
ANIM(ogre_paind15, paind15, ogre_paind16;)
ANIM(ogre_paind16, paind16, ogre_run1;)

ANIM(ogre_paine1, paine1, ogre_paine2;)
ANIM(ogre_paine2, paine2, ogre_paine3; ai_pain(10);)
ANIM(ogre_paine3, paine3, ogre_paine4; ai_pain(9);)
ANIM(ogre_paine4, paine4, ogre_paine5; ai_pain(4);)
ANIM(ogre_paine5, paine5, ogre_paine6;)
ANIM(ogre_paine6, paine6, ogre_paine7;)
ANIM(ogre_paine7, paine7, ogre_paine8;)
ANIM(ogre_paine8, paine8, ogre_paine9;)
ANIM(ogre_paine9, paine9, ogre_paine10;)
ANIM(ogre_paine10, paine10, ogre_paine11;)
ANIM(ogre_paine11, paine11, ogre_paine12;)
ANIM(ogre_paine12, paine12, ogre_paine13;)
ANIM(ogre_paine13, paine13, ogre_paine14;)
ANIM(ogre_paine14, paine14, ogre_paine15;)
ANIM(ogre_paine15, paine15, ogre_run1;)

void ogre_pain(gedict_t *attacker, float damage)
{
	float r;

	// don't make multiple pain sounds right after each other
	if (self->pain_finished > g_globalvars.time)
	{
		return;
	}

	sound(self, CHAN_VOICE, "ogre/ogpain1.wav", 1, ATTN_NORM);

	r = g_random();

	if (r < 0.25)
	{
		ogre_pain1();
		self->pain_finished = g_globalvars.time + 1;
	}
	else if (r < 0.5)
	{
		ogre_painb1();
		self->pain_finished = g_globalvars.time + 1;
	}
	else if (r < 0.75)
	{
		ogre_painc1();
		self->pain_finished = g_globalvars.time + 1;
	}
	else if (r < 0.88)
	{
		ogre_paind1();
		self->pain_finished = g_globalvars.time + 2;
	}
	else
	{
		ogre_paine1();
		self->pain_finished = g_globalvars.time + 2;
	}
}

void ogre_drop_pack(void)
{
	self->s.v.ammo_rockets = 2; // 2 rox in backpack
	DropBackpack();
}

void _ogre_die_xxx(void)
{
	self->s.v.solid = SOLID_NOT;

	ogre_drop_pack();
}

ANIM(ogre_die1, death1, ogre_die2;)
ANIM(ogre_die2, death2, ogre_die3;)
ANIM(ogre_die3, death3, ogre_die4; _ogre_die_xxx();)
ANIM(ogre_die4, death4, ogre_die5;)
ANIM(ogre_die5, death5, ogre_die6;)
ANIM(ogre_die6, death6, ogre_die7;)
ANIM(ogre_die7, death7, ogre_die8;)
ANIM(ogre_die8, death8, ogre_die9;)
ANIM(ogre_die9, death9, ogre_die10;)
ANIM(ogre_die10, death10, ogre_die11;)
ANIM(ogre_die11, death11, ogre_die12;)
ANIM(ogre_die12, death12, ogre_die13;)
ANIM(ogre_die13, death13, ogre_die14;)
ANIM(ogre_die14, death14, ogre_die14;)

ANIM(ogre_bdie1, bdeath1, ogre_bdie2;)
ANIM(ogre_bdie2, bdeath2, ogre_bdie3; ai_forward(5);)
ANIM(ogre_bdie3, bdeath3, ogre_bdie4; _ogre_die_xxx();)
ANIM(ogre_bdie4, bdeath4, ogre_bdie5; ai_forward(1);)
ANIM(ogre_bdie5, bdeath5, ogre_bdie6; ai_forward(3);)
ANIM(ogre_bdie6, bdeath6, ogre_bdie7; ai_forward(7);)
ANIM(ogre_bdie7, bdeath7, ogre_bdie8; ai_forward(25);)
ANIM(ogre_bdie8, bdeath8, ogre_bdie9;)
ANIM(ogre_bdie9, bdeath9, ogre_bdie10;)
ANIM(ogre_bdie10, bdeath10, ogre_bdie10;)

void ogre_die(void)
{
	// check for gib
	if (self->s.v.health < -80)
	{
		ogre_drop_pack();

		sound(self, CHAN_VOICE, "player/udeath.wav", 1, ATTN_NORM);
		ThrowHead("progs/h_ogre.mdl", self->s.v.health);
		ThrowGib("progs/gib3.mdl", self->s.v.health);
		ThrowGib("progs/gib3.mdl", self->s.v.health);
		ThrowGib("progs/gib3.mdl", self->s.v.health);

		self->s.v.nextthink = -1;

		return;
	}

	sound(self, CHAN_VOICE, "ogre/ogdth.wav", 1, ATTN_NORM);

	if (g_random() < 0.5)
	{
		ogre_die1();
	}
	else
	{
		ogre_bdie1();
	}
}

//=============================================================================

/*
 ===========
 OgreCheckAttack

 The player is in view, so decide to move or launch an attack
 Returns false if movement should continue
 ============
 */
float OgreCheckAttack(void)
{
	vec3_t spot1, spot2;
	gedict_t *targ;

	if (enemy_range == RANGE_MELEE)
	{
		if (CanDamage(PROG_TO_EDICT(self->s.v.enemy), self))
		{
			self->attack_state = AS_MELEE;

			return true;
		}
	}

	if (g_globalvars.time < self->attack_finished)
	{
		return false;
	}

	if (!enemy_vis)
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

	if (PROG_TO_EDICT(g_globalvars.trace_ent) != targ)
	{
		return false;	// don't have a clear shot
	}

	// missile attack
	if (enemy_range == RANGE_FAR)
	{
		return false;
	}

	self->attack_state = AS_MISSILE;
	SUB_AttackFinished(1 + 2 * g_random());

	return true;
}

//=============================================================================

/*QUAKED monster_ogre (1 0 0) (-32 -32 -24) (32 32 64) Ambush

 */
void SP_monster_ogre(void)
{
	if (!AllowMonster(self))
	{
		ent_remove(self);

		return;
	}

	safe_precache_model("progs/ogre.mdl");
	safe_precache_model("progs/h_ogre.mdl");
	safe_precache_model("progs/grenade.mdl");

	safe_precache_sound("ogre/ogdrag.wav");
	safe_precache_sound("ogre/ogdth.wav");
	safe_precache_sound("ogre/ogidle.wav");
	safe_precache_sound("ogre/ogidle2.wav");
	safe_precache_sound("ogre/ogpain1.wav");
	safe_precache_sound("ogre/ogsawatk.wav");
	safe_precache_sound("ogre/ogwake.wav");

	setsize(self, PASSVEC3(VEC_HULL2_MIN), PASSVEC3(VEC_HULL2_MAX));
	self->s.v.health = 200;

	self->th_stand = ogre_stand1;
	self->th_walk = ogre_walk1;
	self->th_run = ogre_run1;
	self->th_die = ogre_die;
	self->th_melee = ogre_melee;
	self->th_missile = ogre_nail1;
	self->th_pain = ogre_pain;

	self->th_respawn = SP_monster_ogre;

	walkmonster_start("progs/ogre.mdl");
}
