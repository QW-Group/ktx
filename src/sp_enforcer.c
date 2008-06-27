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

enum {

stand1, stand2, stand3, stand4, stand5, stand6, stand7,

walk1, walk2, walk3, walk4, walk5, walk6, walk7, walk8, walk9, walk10, walk11, walk12, walk13, walk14, walk15, walk16,

run1, run2, run3, run4, run5, run6, run7, run8,

attack1, attack2, attack3, attack4, attack5, attack6, attack7, attack8, attack9, attack10,

death1, death2, death3, death4, death5, death6, death7, death8, death9, death10, death11, death12, death13, death14,

fdeath1, fdeath2, fdeath3, fdeath4, fdeath5, fdeath6, fdeath7, fdeath8, fdeath9, fdeath10, fdeath11,

paina1, paina2, paina3, paina4,

painb1, painb2, painb3, painb4, painb5,

painc1, painc2, painc3, painc4, painc5, painc6, painc7, painc8,

paind1, paind2, paind3, paind4, paind5, paind6, paind7, paind8,
paind9, paind10, paind11, paind12, paind13, paind14, paind15, paind16, paind17, paind18, paind19,

};

void enf_stand1();
void enf_stand2();
void enf_stand3();
void enf_stand4();
void enf_stand5();
void enf_stand6();
void enf_stand7();
void enf_walk1();
void enf_walk2();
void enf_walk3();
void enf_walk4();
void enf_walk5();
void enf_walk6();
void enf_walk7();
void enf_walk8();
void enf_walk9();
void enf_walk10();
void enf_walk11();
void enf_walk12();
void enf_walk13();
void enf_walk14();
void enf_walk15();
void enf_walk16();
void enf_run1();
void enf_run2();
void enf_run3();
void enf_run4();
void enf_run5();
void enf_run6();
void enf_run7();
void enf_run8();
void enf_atk1();
void enf_atk2();
void enf_atk3();
void enf_atk4();
void enf_atk5();
void enf_atk6();
void enf_atk7();
void enf_atk8();
void enf_atk9();
void enf_atk10();
void enf_atk11();
void enf_atk12();
void enf_atk13();
void enf_atk14();
void enf_paina1();
void enf_paina2();
void enf_paina3();
void enf_paina4();
void enf_painb1();
void enf_painb2();
void enf_painb3();
void enf_painb4();
void enf_painb5();
void enf_painc1();
void enf_painc2();
void enf_painc3();
void enf_painc4();
void enf_painc5();
void enf_painc6();
void enf_painc7();
void enf_painc8();
void enf_paind1();
void enf_paind2();
void enf_paind3();
void enf_paind4();
void enf_paind5();
void enf_paind6();
void enf_paind7();
void enf_paind8();
void enf_paind9();
void enf_paind10();
void enf_paind11();
void enf_paind12();
void enf_paind13();
void enf_paind14();
void enf_paind15();
void enf_paind16();
void enf_paind17();
void enf_paind18();
void enf_paind19();
void enf_die1();
void enf_die2();
void enf_die3();
void enf_die4();
void enf_die5();
void enf_die6();
void enf_die7();
void enf_die8();
void enf_die9();
void enf_die10();
void enf_die11();
void enf_die12();
void enf_die13();
void enf_die14();
void enf_fdie1();
void enf_fdie2();
void enf_fdie3();
void enf_fdie4();
void enf_fdie5();
void enf_fdie6();
void enf_fdie7();
void enf_fdie8();
void enf_fdie9();
void enf_fdie10();
void enf_fdie11();

//============================================================================

ANIM(enf_stand1, stand1, enf_stand2; ai_stand();)
ANIM(enf_stand2, stand2, enf_stand3; ai_stand();)
ANIM(enf_stand3, stand3, enf_stand4; ai_stand();)
ANIM(enf_stand4, stand4, enf_stand5; ai_stand();)
ANIM(enf_stand5, stand5, enf_stand6; ai_stand();)
ANIM(enf_stand6, stand6, enf_stand7; ai_stand();)
ANIM(enf_stand7, stand7, enf_stand1; ai_stand();)

void _enf_walk1(void)
{
	if ( g_random() < 0.2 )
		sound (self, CHAN_VOICE, "enforcer/idle1.wav", 1, ATTN_IDLE);

	ai_walk( 2 );
}
ANIM(enf_walk1,  walk1,  enf_walk2;  _enf_walk1();)
ANIM(enf_walk2,  walk2,  enf_walk3;  ai_walk(4);)
ANIM(enf_walk3,  walk3,  enf_walk4;  ai_walk(4);)
ANIM(enf_walk4,  walk4,  enf_walk5;  ai_walk(3);)
ANIM(enf_walk5,  walk5,  enf_walk6;  ai_walk(1);)
ANIM(enf_walk6,  walk6,  enf_walk7;  ai_walk(2);)
ANIM(enf_walk7,  walk7,  enf_walk8;  ai_walk(2);)
ANIM(enf_walk8,  walk8,  enf_walk9;  ai_walk(1);)
ANIM(enf_walk9,  walk9,  enf_walk10; ai_walk(2);)
ANIM(enf_walk10, walk10, enf_walk11; ai_walk(4);)
ANIM(enf_walk11, walk11, enf_walk12; ai_walk(4);)
ANIM(enf_walk12, walk12, enf_walk13; ai_walk(1);)
ANIM(enf_walk13, walk13, enf_walk14; ai_walk(2);)
ANIM(enf_walk14, walk14, enf_walk15; ai_walk(3);)
ANIM(enf_walk15, walk15, enf_walk16; ai_walk(4);)
ANIM(enf_walk16, walk16, enf_walk1;  ai_walk(2);)

void _enf_run1(void)
{
	if ( g_random() < 0.2 )
		sound( self, CHAN_VOICE, "enforcer/idle1.wav", 1, ATTN_IDLE );

	ai_run( 18 );
}
ANIM(enf_run1, run1, enf_run2; _enf_run1();)
ANIM(enf_run2, run2, enf_run3; ai_run(14);)
ANIM(enf_run3, run3, enf_run4; ai_run(7);)
ANIM(enf_run4, run4, enf_run5; ai_run(12);)
ANIM(enf_run5, run5, enf_run6; ai_run(14);)
ANIM(enf_run6, run6, enf_run7; ai_run(14);)
ANIM(enf_run7, run7, enf_run8; ai_run(7);)
ANIM(enf_run8, run8, enf_run1; ai_run(11);)

void enforcer_fire ()
{
	vec3_t	org, delta;

	muzzleflash();
	trap_makevectors( self->s.v.angles );

	VectorMA( self->s.v.origin, 30, g_globalvars.v_forward, org );
	VectorMA( org, 8.5, g_globalvars.v_right, org );
	org[2] += 16;

	VectorSubtract( PROG_TO_EDICT( self->s.v.enemy )->s.v.origin, self->s.v.origin, delta );

	LaunchLaser( org, delta );
}
ANIM(enf_atk1,  attack1,  enf_atk2;  ai_face();)
ANIM(enf_atk2,  attack2,  enf_atk3;  ai_face();)
ANIM(enf_atk3,  attack3,  enf_atk4;  ai_face();)
ANIM(enf_atk4,  attack4,  enf_atk5;  ai_face();)
ANIM(enf_atk5,  attack5,  enf_atk6;  ai_face();)
ANIM(enf_atk6,  attack6,  enf_atk7;  enforcer_fire();)
ANIM(enf_atk7,  attack7,  enf_atk8;  ai_face();)
ANIM(enf_atk8,  attack8,  enf_atk9;  ai_face();)
ANIM(enf_atk9,  attack5,  enf_atk10; ai_face();)
ANIM(enf_atk10, attack6,  enf_atk11; enforcer_fire();)
ANIM(enf_atk11, attack7,  enf_atk12; ai_face();)
ANIM(enf_atk12, attack8,  enf_atk13; ai_face();)
ANIM(enf_atk13, attack9,  enf_atk14; ai_face();)
ANIM(enf_atk14, attack10, enf_run1;  ai_face(); SUB_CheckRefire( ( func_t ) enf_atk1);)

ANIM(enf_paina1, paina1,  enf_paina2)
ANIM(enf_paina2, paina2,  enf_paina3)
ANIM(enf_paina3, paina3,  enf_paina4)
ANIM(enf_paina4, paina4,  enf_run1)

ANIM(enf_painb1, painb1,  enf_painb2)
ANIM(enf_painb2, painb2,  enf_painb3)
ANIM(enf_painb3, painb3,  enf_painb4)
ANIM(enf_painb4, painb4,  enf_painb5)
ANIM(enf_painb5, painb5,  enf_run1)

ANIM(enf_painc1, painc1,  enf_painc2)
ANIM(enf_painc2, painc2,  enf_painc3)
ANIM(enf_painc3, painc3,  enf_painc4)
ANIM(enf_painc4, painc4,  enf_painc5)
ANIM(enf_painc5, painc5,  enf_painc6)
ANIM(enf_painc6, painc6,  enf_painc7)
ANIM(enf_painc7, painc7,  enf_painc8)
ANIM(enf_painc8, painc8,  enf_run1)

ANIM(enf_paind1,  paind1,  enf_paind2)
ANIM(enf_paind2,  paind2,  enf_paind3)
ANIM(enf_paind3,  paind3,  enf_paind4)
ANIM(enf_paind4,  paind4,  enf_paind5; ai_painforward(2);)
ANIM(enf_paind5,  paind5,  enf_paind6; ai_painforward(1);)
ANIM(enf_paind6,  paind6,  enf_paind7)
ANIM(enf_paind7,  paind7,  enf_paind8)
ANIM(enf_paind8,  paind8,  enf_paind9)
ANIM(enf_paind9,  paind9,  enf_paind10)
ANIM(enf_paind10, paind10, enf_paind11)
ANIM(enf_paind11, paind11, enf_paind12; ai_painforward(1);)
ANIM(enf_paind12, paind12, enf_paind13; ai_painforward(1);)
ANIM(enf_paind13, paind13, enf_paind14; ai_painforward(1);)
ANIM(enf_paind14, paind14, enf_paind15)
ANIM(enf_paind15, paind15, enf_paind16)
ANIM(enf_paind16, paind16, enf_paind17; ai_pain(1);)
ANIM(enf_paind17, paind17, enf_paind18; ai_pain(1);)
ANIM(enf_paind18, paind18, enf_paind19)
ANIM(enf_paind19, paind19, enf_run1)

void enf_pain( struct gedict_s *attacker, float damage )
{
	float	r;

	if ( self->pain_finished > g_globalvars.time )
		return;

	r = g_random();

	if ( r < 0.5 )
		sound (self, CHAN_VOICE, "enforcer/pain1.wav", 1, ATTN_NORM);
	else
		sound (self, CHAN_VOICE, "enforcer/pain2.wav", 1, ATTN_NORM);

	if ( r < 0.2 )
	{
		self->pain_finished = g_globalvars.time + 1;
		enf_paina1();
	}
	else if ( r < 0.4 )
	{
		self->pain_finished = g_globalvars.time + 1;
		enf_painb1();
	}
	else if ( r < 0.7 )
	{
		self->pain_finished = g_globalvars.time + 1;
		enf_painc1();
	}
	else
	{
		self->pain_finished = g_globalvars.time + 2;
		enf_paind1();
	}
}

void _enf_die_xxx(void)
{
	self->s.v.solid = SOLID_NOT;
	self->s.v.ammo_cells = 5; // drop 5 cells in backpack
	DropBackpack();
}

ANIM(enf_die1,  death1,  enf_die2)
ANIM(enf_die2,  death2,  enf_die3)
ANIM(enf_die3,  death3,  enf_die4; _enf_die_xxx();)
ANIM(enf_die4,  death4,  enf_die5; ai_forward(14);)
ANIM(enf_die5,  death5,  enf_die6; ai_forward(2);)
ANIM(enf_die6,  death6,  enf_die7)
ANIM(enf_die7,  death7,  enf_die8)
ANIM(enf_die8,  death8,  enf_die9)
ANIM(enf_die9,  death9,  enf_die10; ai_forward(3);)
ANIM(enf_die10, death10, enf_die11; ai_forward(5);)
ANIM(enf_die11, death11, enf_die12; ai_forward(5);)
ANIM(enf_die12, death12, enf_die13; ai_forward(5);)
ANIM(enf_die13, death13, enf_die14)
ANIM(enf_die14, death14, enf_die14)

ANIM(enf_fdie1,  fdeath1,  enf_fdie2)
ANIM(enf_fdie2,  fdeath2,  enf_fdie3)
ANIM(enf_fdie3,  fdeath3,  enf_fdie4; _enf_die_xxx();)
ANIM(enf_fdie4,  fdeath4,  enf_fdie5)
ANIM(enf_fdie5,  fdeath5,  enf_fdie6)
ANIM(enf_fdie6,  fdeath6,  enf_fdie7)
ANIM(enf_fdie7,  fdeath7,  enf_fdie8)
ANIM(enf_fdie8,  fdeath8,  enf_fdie9)
ANIM(enf_fdie9,  fdeath9,  enf_fdie10)
ANIM(enf_fdie10, fdeath10, enf_fdie11)
ANIM(enf_fdie11, fdeath11, enf_fdie11)

void enf_die ()
{
	// check for gib
	if ( self->s.v.health < -35 )
	{
		sound( self, CHAN_VOICE, "player/udeath.wav", 1, ATTN_NORM );
		ThrowHead( "progs/h_mega.mdl", self->s.v.health );
		ThrowGib( "progs/gib1.mdl", self->s.v.health );
		ThrowGib( "progs/gib2.mdl", self->s.v.health );
		ThrowGib( "progs/gib3.mdl", self->s.v.health );

		self->s.v.nextthink = -1;

		return;
	}

	// regular death
	sound( self, CHAN_VOICE, "enforcer/death1.wav", 1, ATTN_NORM );
	if ( g_random() > 0.5 )
		enf_die1();
	else
		enf_fdie1();
}

//============================================================================

/*QUAKED monster_enforcer (1 0 0) (-16 -16 -24) (16 16 40) Ambush

*/
void SP_monster_enforcer()
{
	if ( deathmatch )
	{
		ent_remove( self );
		return;
	}

	trap_precache_model( "progs/enforcer.mdl" );
	trap_precache_model( "progs/h_mega.mdl" );
	trap_precache_model( "progs/laser.mdl" );
	                
	trap_precache_sound( "enforcer/death1.wav" );
	trap_precache_sound( "enforcer/enfire.wav" );
	trap_precache_sound( "enforcer/enfstop.wav" );
	trap_precache_sound( "enforcer/idle1.wav" );
	trap_precache_sound( "enforcer/pain1.wav" );
	trap_precache_sound( "enforcer/pain2.wav" );
	trap_precache_sound( "enforcer/sight1.wav" );
	trap_precache_sound( "enforcer/sight2.wav" );
	trap_precache_sound( "enforcer/sight3.wav" );
	trap_precache_sound( "enforcer/sight4.wav" );

	self->s.v.solid = SOLID_SLIDEBOX;
	self->s.v.movetype = MOVETYPE_STEP;

	setmodel( self, "progs/enforcer.mdl" );

	setsize( self, -16, -16, -24, 16, 16, 40);
	self->s.v.health = 80;

	self->th_stand   = enf_stand1;
	self->th_walk    = enf_walk1;
	self->th_run     = enf_run1;
	self->th_pain    = enf_pain;
	self->th_die     = enf_die;
	self->th_missile = enf_atk1;

	walkmonster_start();
}

