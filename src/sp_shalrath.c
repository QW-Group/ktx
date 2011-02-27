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

SHAL-RATH

==============================================================================
*/

#include "g_local.h"

enum {

attack1, attack2, attack3, attack4, attack5, attack6, attack7, attack8, attack9, attack10, attack11,

pain1, pain2, pain3, pain4, pain5,

death1, death2, death3, death4, death5, death6, death7,

walk1, walk2, walk3, walk4, walk5, walk6, walk7, walk8, walk9, walk10, walk11, walk12,

};

void shal_stand();
void shal_walk1();
void shal_walk2();
void shal_walk3();
void shal_walk4();
void shal_walk5();
void shal_walk6();
void shal_walk7();
void shal_walk8();
void shal_walk9();
void shal_walk10();
void shal_walk11();
void shal_walk12();
void shal_run1();
void shal_run2();
void shal_run3();
void shal_run4();
void shal_run5();
void shal_run6();
void shal_run7();
void shal_run8();
void shal_run9();
void shal_run10();
void shal_run11();
void shal_run12();
void shal_attack1();
void shal_attack2();
void shal_attack3();
void shal_attack4();
void shal_attack5();
void shal_attack6();
void shal_attack7();
void shal_attack8();
void shal_attack9();
void shal_attack10();
void shal_attack11();
void shal_pain1();
void shal_pain2();
void shal_pain3();
void shal_pain4();
void shal_pain5();
void shal_death1();
void shal_death2();
void shal_death3();
void shal_death4();
void shal_death5();
void shal_death6();
void shal_death7();

//=============================================================================

ANIM(shal_stand, walk1, shal_stand; ai_stand();)

void _shal_walk1( void )
{
	if ( g_random() < 0.2 )
		sound (self, CHAN_VOICE, "shalrath/idle.wav", 1, ATTN_IDLE);
		
	ai_walk( 6 );
}
ANIM(shal_walk1,  walk2,  shal_walk2;  _shal_walk1();)
ANIM(shal_walk2,  walk3,  shal_walk3;  ai_walk(4);)
ANIM(shal_walk3,  walk4,  shal_walk4;  ai_walk(0);)
ANIM(shal_walk4,  walk5,  shal_walk5;  ai_walk(0);)
ANIM(shal_walk5,  walk6,  shal_walk6;  ai_walk(0);)
ANIM(shal_walk6,  walk7,  shal_walk7;  ai_walk(0);)
ANIM(shal_walk7,  walk8,  shal_walk8;  ai_walk(5);)
ANIM(shal_walk8,  walk9,  shal_walk9;  ai_walk(6);)
ANIM(shal_walk9,  walk10, shal_walk10; ai_walk(5);)
ANIM(shal_walk10, walk11, shal_walk11; ai_walk(0);)
ANIM(shal_walk11, walk12, shal_walk12; ai_walk(4);)
ANIM(shal_walk12, walk1,  shal_walk1;  ai_walk(5);)

void _shal_run1( void )
{
	if ( g_random() < 0.2 )
		sound (self, CHAN_VOICE, "shalrath/idle.wav", 1, ATTN_IDLE);
	
	ai_run( 6 );
}
ANIM(shal_run1,  walk2,  shal_run2;  _shal_run1();)
ANIM(shal_run2,  walk3,  shal_run3;  ai_run(4);)
ANIM(shal_run3,  walk4,  shal_run4;  ai_run(0);)
ANIM(shal_run4,  walk5,  shal_run5;  ai_run(0);)
ANIM(shal_run5,  walk6,  shal_run6;  ai_run(0);)
ANIM(shal_run6,  walk7,  shal_run7;  ai_run(0);)
ANIM(shal_run7,  walk8,  shal_run8;  ai_run(5);)
ANIM(shal_run8,  walk9,  shal_run9;  ai_run(6);)
ANIM(shal_run9,  walk10, shal_run10; ai_run(5);)
ANIM(shal_run10, walk11, shal_run11; ai_run(0);)
ANIM(shal_run11, walk12, shal_run12; ai_run(4);)
ANIM(shal_run12, walk1,  shal_run1;  ai_run(5);)

//=============================================================================

void ShalMissileTouch ()
{
	if ( other == PROG_TO_EDICT( self->s.v.owner ) )
		return;		// don't explode on owner

	if ( streq( other->s.v.classname, "monster_zombie" ) )
	{
		other->deathtype = dtSQUISH; // FIXME
		T_Damage( other, self, self, 110 );
	}

	T_RadiusDamage( self, PROG_TO_EDICT( self->s.v.owner ), 40, world, dtSQUISH );
	sound( self, CHAN_WEAPON, "weapons/r_exp3.wav", 1, ATTN_NORM );

	WriteByte( MSG_BROADCAST, SVC_TEMPENTITY );
	WriteByte( MSG_BROADCAST, TE_EXPLOSION );
	WriteCoord( MSG_BROADCAST, self->s.v.origin[0] );
	WriteCoord( MSG_BROADCAST, self->s.v.origin[1] );
	WriteCoord( MSG_BROADCAST, self->s.v.origin[2] );
	trap_multicast( PASSVEC3( self->s.v.origin ), MULTICAST_PHS );

	ent_remove( self );
}

void ShalHome()
{
	vec3_t	dir, vtemp;

	if (   ISDEAD( PROG_TO_EDICT( self->s.v.enemy ) ) // enemy dead
		|| self->spawn_time + 25 < g_globalvars.time  // flying for too long time
	   )
	{
		other = world;
		ShalMissileTouch();

		ent_remove( self ); // ShalMissileTouch not always remove ent, so ensure it
		return;
	}

	VectorCopy( PROG_TO_EDICT( self->s.v.enemy )->s.v.origin, vtemp )
	vtemp[2] += 10;

	VectorSubtract( vtemp, self->s.v.origin, dir )
	normalize( dir, dir );
	VectorScale( dir, skill == 3 ? 350 : 250, self->s.v.velocity);
	self->s.v.nextthink = g_globalvars.time + 0.2;
	self->s.v.think = ( func_t ) ShalHome;
}

/*
================
ShalMissile
================
*/
void ShalMissile()
{
	gedict_t	*missile;
	vec3_t		dir;
	float		dist, flytime;

	VectorCopy( PROG_TO_EDICT( self->s.v.enemy )->s.v.origin, dir );
	dir[2] += 10;
	VectorSubtract( dir, self->s.v.origin, dir );
	dist = vlen( dir );
	normalize( dir, dir );

	flytime = max( 0.1, dist * 0.002 ); // 0.002 is roughtly 1/400 I guess

	muzzleflash();
	sound( self, CHAN_WEAPON, "shalrath/attack2.wav", 1, ATTN_NORM );

	missile = spawn ();
	missile->s.v.classname = "shalrath_missile";
	missile->s.v.owner = EDICT_TO_PROG( self );
	missile->s.v.solid = SOLID_BBOX;
	missile->s.v.movetype = MOVETYPE_FLYMISSILE;
	missile->isMissile = true;
	setmodel( missile, "progs/v_spike.mdl" );
	setsize( missile, PASSVEC3( VEC_ORIGIN ), PASSVEC3( VEC_ORIGIN ) );

	setorigin( missile, self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] + 10 );
	VectorScale( dir, 400, missile->s.v.velocity );
	SetVector( missile->s.v.avelocity, 300, 300, 300 );
	missile->s.v.nextthink = g_globalvars.time + flytime;
	missile->s.v.think = ( func_t ) ShalHome;
	missile->s.v.enemy = self->s.v.enemy;
	missile->s.v.touch = ( func_t) ShalMissileTouch;
}

void _shal_attack1( void )
{
	sound( self, CHAN_VOICE, "shalrath/attack.wav", 1, ATTN_NORM);

	ai_face(); 
}
ANIM(shal_attack1,  attack1,  shal_attack2;  _shal_attack1();)
ANIM(shal_attack2,  attack2,  shal_attack3;  ai_face();)
ANIM(shal_attack3,  attack3,  shal_attack4;  ai_face();)
ANIM(shal_attack4,  attack4,  shal_attack5;  ai_face();)
ANIM(shal_attack5,  attack5,  shal_attack6;  ai_face();)
ANIM(shal_attack6,  attack6,  shal_attack7;  ai_face();)
ANIM(shal_attack7,  attack7,  shal_attack8;  ai_face();)
ANIM(shal_attack8,  attack8,  shal_attack9;  ai_face();)
ANIM(shal_attack9,  attack9,  shal_attack10; ShalMissile();)
ANIM(shal_attack10, attack10, shal_attack11; ai_face();)
ANIM(shal_attack11, attack11, shal_run1; )

ANIM(shal_pain1, pain1, shal_pain2; )
ANIM(shal_pain2, pain2, shal_pain3; )
ANIM(shal_pain3, pain3, shal_pain4; )
ANIM(shal_pain4, pain4, shal_pain5; )
ANIM(shal_pain5, pain5, shal_run1;  )

void shalrath_pain( gedict_t* attacker, float damage )
{
	if ( self->pain_finished > g_globalvars.time )
		return;

	sound( self, CHAN_VOICE, "shalrath/pain.wav", 1, ATTN_NORM );
	shal_pain1();
	self->pain_finished = g_globalvars.time + 3;
}

ANIM(shal_death1, death1, shal_death2; )
ANIM(shal_death2, death2, shal_death3; )
ANIM(shal_death3, death3, shal_death4; )
ANIM(shal_death4, death4, shal_death5; )
ANIM(shal_death5, death5, shal_death6; )
ANIM(shal_death6, death6, shal_death7; )
ANIM(shal_death7, death7, shal_death7; )

void shalrath_die ()
{
	// check for gib
	if ( self->s.v.health < -90 )
	{
		sound( self, CHAN_VOICE, "player/udeath.wav", 1, ATTN_NORM );
		ThrowHead( "progs/h_shal.mdl", self->s.v.health );
		ThrowGib( "progs/gib1.mdl", self->s.v.health );
		ThrowGib( "progs/gib2.mdl", self->s.v.health );
		ThrowGib( "progs/gib3.mdl", self->s.v.health );

		self->s.v.nextthink = -1;

		return;
	}

	sound (self, CHAN_VOICE, "shalrath/death.wav", 1, ATTN_NORM);
	shal_death1();
	self->s.v.solid  = SOLID_NOT;
}

//=================================================================

/*QUAKED monster_shalrath (1 0 0) (-32 -32 -24) (32 32 48) Ambush
*/
void SP_monster_shalrath ()
{
	if ( deathmatch )
	{
		ent_remove( self );
		return;
	}

	safe_precache_model( "progs/shalrath.mdl" );
	safe_precache_model( "progs/h_shal.mdl" );
	safe_precache_model( "progs/v_spike.mdl" );
	                     
	safe_precache_sound( "shalrath/attack.wav" );
	safe_precache_sound( "shalrath/attack2.wav" );
	safe_precache_sound( "shalrath/death.wav" );
	safe_precache_sound( "shalrath/idle.wav" );
	safe_precache_sound( "shalrath/pain.wav" );
	safe_precache_sound( "shalrath/sight.wav" );

	setsize( self, PASSVEC3( VEC_HULL2_MIN ), PASSVEC3( VEC_HULL2_MAX ) );
	self->s.v.health = 400;

	self->th_stand   = shal_stand;
	self->th_walk    = shal_walk1;
	self->th_run     = shal_run1;
	self->th_die     = shalrath_die;
	self->th_pain    = shalrath_pain;
	self->th_missile = shal_attack1;

	self->th_respawn = SP_monster_shalrath;

	walkmonster_start( "progs/shalrath.mdl" );
}
