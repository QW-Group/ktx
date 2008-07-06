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

SHAMBLER

==============================================================================
*/

#include "g_local.h"

enum {

stand1, stand2, stand3, stand4, stand5, stand6, stand7, stand8, stand9,
stand10, stand11, stand12, stand13, stand14, stand15, stand16, stand17,

walk1, walk2, walk3, walk4, walk5, walk6, walk7,
walk8, walk9, walk10, walk11, walk12,

run1, run2, run3, run4, run5, run6,

smash1, smash2, smash3, smash4, smash5, smash6, smash7,
smash8, smash9, smash10, smash11, smash12,

swingr1, swingr2, swingr3, swingr4, swingr5,
swingr6, swingr7, swingr8, swingr9,

swingl1, swingl2, swingl3, swingl4, swingl5,
swingl6, swingl7, swingl8, swingl9,

magic1, magic2, magic3, magic4, magic5,
magic6, magic7, magic8, magic9, magic10, magic11, magic12,

pain1, pain2, pain3, pain4, pain5, pain6,

death1, death2, death3, death4, death5, death6,
death7, death8, death9, death10, death11,

};

void sham_stand1();
void sham_stand2();
void sham_stand3();
void sham_stand4();
void sham_stand5();
void sham_stand6();
void sham_stand7();
void sham_stand8();
void sham_stand9();
void sham_stand10();
void sham_stand11();
void sham_stand12();
void sham_stand13();
void sham_stand14();
void sham_stand15();
void sham_stand16();
void sham_stand17();
void sham_walk1();
void sham_walk2();
void sham_walk3();
void sham_walk4();
void sham_walk5();
void sham_walk6();
void sham_walk7();
void sham_walk8();
void sham_walk9();
void sham_walk10();
void sham_walk11();
void sham_walk12();
void sham_run1();
void sham_run2();
void sham_run3();
void sham_run4();
void sham_run5();
void sham_run6();
void sham_smash1();
void sham_smash2();
void sham_smash3();
void sham_smash4();
void sham_smash5();
void sham_smash6();
void sham_smash7();
void sham_smash8();
void sham_smash9();
void sham_smash10();
void sham_smash11();
void sham_smash12();
void sham_swingl1();
void sham_swingl2();
void sham_swingl3();
void sham_swingl4();
void sham_swingl5();
void sham_swingl6();
void sham_swingl7();
void sham_swingl8();
void sham_swingl9();
void sham_swingr1();
void sham_swingr2();
void sham_swingr3();
void sham_swingr4();
void sham_swingr5();
void sham_swingr6();
void sham_swingr7();
void sham_swingr8();
void sham_swingr9();
void sham_magic1();
void sham_magic2();
void sham_magic3();
void sham_magic4();
void sham_magic5();
void sham_magic6();
void sham_magic9();
void sham_magic10();
void sham_magic11();
void sham_magic12();
void sham_pain1();
void sham_pain2();
void sham_pain3();
void sham_pain4();
void sham_pain5();
void sham_pain6();
void sham_death1();
void sham_death2();
void sham_death3();
void sham_death4();
void sham_death5();
void sham_death6();
void sham_death7();
void sham_death8();
void sham_death9();
void sham_death10();
void sham_death11();

//=============================================================================

ANIM(sham_stand1,  stand1,  sham_stand2;  ai_stand();)
ANIM(sham_stand2,  stand2,  sham_stand3;  ai_stand();)
ANIM(sham_stand3,  stand3,  sham_stand4;  ai_stand();)
ANIM(sham_stand4,  stand4,  sham_stand5;  ai_stand();)
ANIM(sham_stand5,  stand5,  sham_stand6;  ai_stand();)
ANIM(sham_stand6,  stand6,  sham_stand7;  ai_stand();)
ANIM(sham_stand7,  stand7,  sham_stand8;  ai_stand();)
ANIM(sham_stand8,  stand8,  sham_stand9;  ai_stand();)
ANIM(sham_stand9,  stand9,  sham_stand10; ai_stand();)
ANIM(sham_stand10, stand10, sham_stand11; ai_stand();)
ANIM(sham_stand11, stand11, sham_stand12; ai_stand();)
ANIM(sham_stand12, stand12, sham_stand13; ai_stand();)
ANIM(sham_stand13, stand13, sham_stand14; ai_stand();)
ANIM(sham_stand14, stand14, sham_stand15; ai_stand();)
ANIM(sham_stand15, stand15, sham_stand16; ai_stand();)
ANIM(sham_stand16, stand16, sham_stand17; ai_stand();)
ANIM(sham_stand17, stand17, sham_stand1;  ai_stand();)

void _sham_walk12( void )
{
	ai_walk( 7 );
	
	if ( g_random() > 0.8 )
		sound( self, CHAN_VOICE, "shambler/sidle.wav", 1, ATTN_IDLE );
}
ANIM(sham_walk1,  walk1,  sham_walk2;  ai_walk(10);)
ANIM(sham_walk2,  walk2,  sham_walk3;  ai_walk(9);)
ANIM(sham_walk3,  walk3,  sham_walk4;  ai_walk(9);)
ANIM(sham_walk4,  walk4,  sham_walk5;  ai_walk(5);)
ANIM(sham_walk5,  walk5,  sham_walk6;  ai_walk(6);)
ANIM(sham_walk6,  walk6,  sham_walk7;  ai_walk(12);)
ANIM(sham_walk7,  walk7,  sham_walk8;  ai_walk(8);)
ANIM(sham_walk8,  walk8,  sham_walk9;  ai_walk(3);)
ANIM(sham_walk9,  walk9,  sham_walk10; ai_walk(13);)
ANIM(sham_walk10, walk10, sham_walk11; ai_walk(9);)
ANIM(sham_walk11, walk11, sham_walk12; ai_walk(7);)
ANIM(sham_walk12, walk12, sham_walk1;  _sham_walk12();)

void _sham_run6( void )
{
	ai_run( 20 );
	
	if ( g_random() > 0.8 )
		sound( self, CHAN_VOICE, "shambler/sidle.wav", 1, ATTN_IDLE );
}
ANIM(sham_run1, run1, sham_run2; ai_run(20);)
ANIM(sham_run2, run2, sham_run3; ai_run(24);)
ANIM(sham_run3, run3, sham_run4; ai_run(20);)
ANIM(sham_run4, run4, sham_run5; ai_run(20);)
ANIM(sham_run5, run5, sham_run6; ai_run(24);)
ANIM(sham_run6, run6, sham_run1; _sham_run6();)

void _sham_smash1( void )
{
	sound( self, CHAN_VOICE, "shambler/melee1.wav", 1, ATTN_NORM );
	
	ai_charge( 2 );
}
void _sham_smash10( void )
{
	vec3_t	delta, org;
	float	ldmg;

	if ( !self->s.v.enemy )
		return;

	ai_charge( 0 );

	VectorSubtract( PROG_TO_EDICT( self->s.v.enemy )->s.v.origin, self->s.v.origin, delta );

	if ( vlen( delta ) > 100 )
		return;

	if ( !CanDamage( PROG_TO_EDICT( self->s.v.enemy ), self ) )
		return;

	ldmg = ( g_random() + g_random() + g_random() ) * 40;
	PROG_TO_EDICT( self->s.v.enemy )->deathtype = dtSQUISH; // FIXME
	T_Damage( PROG_TO_EDICT( self->s.v.enemy ), self, self, ldmg );
	sound( self, CHAN_VOICE, "shambler/smack.wav", 1, ATTN_NORM );

	trap_makevectors( self->s.v.angles );
	VectorMA( self->s.v.origin, 16, g_globalvars.v_forward, org );
	VectorScale( g_globalvars.v_right, crandom() * 100, delta );
	SpawnMeatSpray( org, delta );
	VectorScale( g_globalvars.v_right, crandom() * 100, delta );
	SpawnMeatSpray( org, delta );
}
ANIM(sham_smash1,  smash1,  sham_smash2;  _sham_smash1();)
ANIM(sham_smash2,  smash2,  sham_smash3;  ai_charge(6);)
ANIM(sham_smash3,  smash3,  sham_smash4;  ai_charge(6);)
ANIM(sham_smash4,  smash4,  sham_smash5;  ai_charge(5);)
ANIM(sham_smash5,  smash5,  sham_smash6;  ai_charge(4);)
ANIM(sham_smash6,  smash6,  sham_smash7;  ai_charge(1);)
ANIM(sham_smash7,  smash7,  sham_smash8;  ai_charge(0);)
ANIM(sham_smash8,  smash8,  sham_smash9;  ai_charge(0);)
ANIM(sham_smash9,  smash9,  sham_smash10; ai_charge(0);)
ANIM(sham_smash10, smash10, sham_smash11; _sham_smash10();)
ANIM(sham_smash11, smash11, sham_smash12; ai_charge(5);)
ANIM(sham_smash12, smash12, sham_run1;    ai_charge(4);)

void ShamClaw( float side )
{
	vec3_t	delta, org;
	float	ldmg;

	if ( !self->s.v.enemy )
		return;

	ai_charge( 10 );

	VectorSubtract( PROG_TO_EDICT( self->s.v.enemy )->s.v.origin, self->s.v.origin, delta );

	if ( vlen( delta ) > 100 )
		return;

	if ( !CanDamage( PROG_TO_EDICT( self->s.v.enemy ), self ) )
		return;

	ldmg = ( g_random() + g_random() + g_random() ) * 20;
	PROG_TO_EDICT( self->s.v.enemy )->deathtype = dtSQUISH; // FIXME
	T_Damage( PROG_TO_EDICT( self->s.v.enemy ), self, self, ldmg );
	sound( self, CHAN_VOICE, "shambler/smack.wav", 1, ATTN_NORM );

	if ( side )
	{
		trap_makevectors( self->s.v.angles );
		VectorMA( self->s.v.origin, 16, g_globalvars.v_forward, org );
		VectorScale( g_globalvars.v_right, side, delta );
		SpawnMeatSpray( org, delta );
	}
}

void _sham_swingl1( void )
{
	sound( self, CHAN_VOICE, "shambler/melee2.wav", 1, ATTN_NORM );
	
	ai_charge( 5 );
}
void _sham_swingl9( void )
{
	ai_charge(8);
	
	if ( g_random() < 0.5 )  // why so complicated, why not just random it at sham_melee(), who knows...
		self->s.v.think = ( func_t ) sham_swingr1;
}
ANIM(sham_swingl1, swingl1, sham_swingl2; _sham_swingl1();)
ANIM(sham_swingl2, swingl2, sham_swingl3; ai_charge(3);)
ANIM(sham_swingl3, swingl3, sham_swingl4; ai_charge(7);)
ANIM(sham_swingl4, swingl4, sham_swingl5; ai_charge(3);)
ANIM(sham_swingl5, swingl5, sham_swingl6; ai_charge(7);)
ANIM(sham_swingl6, swingl6, sham_swingl7; ai_charge(9);)
ANIM(sham_swingl7, swingl7, sham_swingl8; ai_charge(5); ShamClaw(250);)
ANIM(sham_swingl8, swingl8, sham_swingl9; ai_charge(4);)
ANIM(sham_swingl9, swingl9, sham_run1;    _sham_swingl9();)

void _sham_swingr1( void )
{
	sound( self, CHAN_VOICE, "shambler/melee1.wav", 1, ATTN_NORM );

	ai_charge(1);
}
void _sham_swingr9( void )
{
	ai_charge(11);
	
	if ( g_random() < 0.5 )  // why so complicated, why not just random it at sham_melee(), who knows...
		self->s.v.think = ( func_t ) sham_swingl1;
}
ANIM(sham_swingr1, swingr1, sham_swingr2; _sham_swingr1();)
ANIM(sham_swingr2, swingr2, sham_swingr3; ai_charge(8);)
ANIM(sham_swingr3, swingr3, sham_swingr4; ai_charge(14);)
ANIM(sham_swingr4, swingr4, sham_swingr5; ai_charge(7);)
ANIM(sham_swingr5, swingr5, sham_swingr6; ai_charge(3);)
ANIM(sham_swingr6, swingr6, sham_swingr7; ai_charge(6);)
ANIM(sham_swingr7, swingr7, sham_swingr8; ai_charge(6); ShamClaw(-250);)
ANIM(sham_swingr8, swingr8, sham_swingr9; ai_charge(3);)
ANIM(sham_swingr9, swingr9, sham_run1;    _sham_swingr9();)

void sham_melee()
{
	float	chance;

	chance = g_random();

	if ( chance > 0.6 || self->s.v.health == 600 )
		sham_smash1();
	else if ( chance > 0.3 )
		sham_swingr1();
	else
		sham_swingl1();
}


//============================================================================

void CastLightning()
{
	vec3_t	org, dir;

	muzzleflash();

	ai_face ();

	// from 
	VectorCopy( self->s.v.origin, org );
	org[2] += 40;

	// to
	VectorCopy( PROG_TO_EDICT( self->s.v.enemy )->s.v.origin, dir );
	dir[2] += 16;
	VectorSubtract( dir, org, dir );
	normalize( dir, dir );
	VectorMA( self->s.v.origin, 600, dir, dir );

	// trace from -> to
	traceline( PASSVEC3( org ), PASSVEC3( dir ), true, self );

	WriteByte( MSG_BROADCAST, SVC_TEMPENTITY );
	WriteByte( MSG_BROADCAST, TE_LIGHTNING1 );
	WriteEntity( MSG_BROADCAST, self);
	WriteCoord( MSG_BROADCAST, org[0] );
	WriteCoord( MSG_BROADCAST, org[1] );
	WriteCoord( MSG_BROADCAST, org[2] );
	WriteCoord( MSG_BROADCAST, g_globalvars.trace_endpos[0] );
	WriteCoord( MSG_BROADCAST, g_globalvars.trace_endpos[1] );
	WriteCoord( MSG_BROADCAST, g_globalvars.trace_endpos[2] );

	LightningDamage( org, g_globalvars.trace_endpos, self, 10 );
}

// shambler's bolt have 3 frames, I guess
void sham_bolt()
{
	if ( self->s.v.frame == 2 || self->spawn_time + 0.7 < g_globalvars.time )
	{
		ent_remove( self );
		return;
	}

	self->s.v.frame = bound( 0, self->s.v.frame + 1, 2 );
	self->s.v.nextthink = g_globalvars.time + FRAMETIME;
}

void _sham_magic3( void )
{
	ai_face();
	
	self->s.v.nextthink = g_globalvars.time + 0.2; // hrm

	muzzleflash();

	ai_face();

	// spawn shambler's bolt
	{
		gedict_t	*o;

		o = spawn();
		o->s.v.owner = EDICT_TO_PROG( self );
		setmodel( o, "progs/s_light.mdl" );
		setorigin( o, PASSVEC3( self->s.v.origin ) );
		VectorCopy( self->s.v.angles, o->s.v.angles );
		o->s.v.nextthink = g_globalvars.time + FRAMETIME;
		o->s.v.think = ( func_t ) sham_bolt;
	}
}

ANIM(sham_magic1,  magic1,  sham_magic2;  ai_face(); sound( self, CHAN_WEAPON, "shambler/sattck1.wav", 1, ATTN_NORM );)
ANIM(sham_magic2,  magic2,  sham_magic3;  ai_face();)
ANIM(sham_magic3,  magic3,  sham_magic4;  _sham_magic3();)
ANIM(sham_magic4,  magic4,  sham_magic5;  muzzleflash();)
ANIM(sham_magic5,  magic5,  sham_magic6;  muzzleflash();)
ANIM(sham_magic6,  magic6,  sham_magic9;  CastLightning(); sound( self, CHAN_WEAPON, "shambler/sboom.wav", 1, ATTN_NORM );)
ANIM(sham_magic9,  magic9,  sham_magic10; CastLightning(); )
ANIM(sham_magic10, magic10, sham_magic11; CastLightning(); )
ANIM(sham_magic11, magic11, sham_magic12; if ( skill == 3 )	CastLightning(); )
ANIM(sham_magic12, magic12, sham_run1; )


ANIM(sham_pain1, pain1, sham_pain2; )
ANIM(sham_pain2, pain2, sham_pain3; )
ANIM(sham_pain3, pain3, sham_pain4; )
ANIM(sham_pain4, pain4, sham_pain5; )
ANIM(sham_pain5, pain5, sham_pain6; )
ANIM(sham_pain6, pain6, sham_run1;  )

void sham_pain( gedict_t* attacker, float damage )
{
	if ( ISDEAD( self ) )
		return;		// already dying, don't go into pain frame

	if ( g_random() * 400 > damage )
		return;		// didn't flinch

	if ( self->pain_finished > g_globalvars.time )
		return;

	sound( self, CHAN_VOICE, "shambler/shurt2.wav", 1, ATTN_NORM );

	self->pain_finished = g_globalvars.time + 2;

	sham_pain1 ();
}

//============================================================================

ANIM(sham_death1,  death1,  sham_death2; )
ANIM(sham_death2,  death2,  sham_death3; )
ANIM(sham_death3,  death3,  sham_death4; self->s.v.solid  = SOLID_NOT;)
ANIM(sham_death4,  death4,  sham_death5; )
ANIM(sham_death5,  death5,  sham_death6; )
ANIM(sham_death6,  death6,  sham_death7; )
ANIM(sham_death7,  death7,  sham_death8; )
ANIM(sham_death8,  death8,  sham_death9; )
ANIM(sham_death9,  death9,  sham_death10; )
ANIM(sham_death10, death10, sham_death11; )
ANIM(sham_death11, death11, sham_death11; )

void sham_die ()
{
	// check for gib
	if ( self->s.v.health < -60 )
	{
		sound( self, CHAN_VOICE, "player/udeath.wav", 1, ATTN_NORM );
		ThrowHead( "progs/h_shams.mdl", self->s.v.health );
		ThrowGib( "progs/gib1.mdl", self->s.v.health );
		ThrowGib( "progs/gib2.mdl", self->s.v.health );
		ThrowGib( "progs/gib3.mdl", self->s.v.health );

		self->s.v.nextthink = -1;
		
		return;
	}

	// regular death
	sound( self, CHAN_VOICE, "shambler/sdeath.wav", 1, ATTN_NORM );

	sham_death1 ();
}

//=============================================================================

/*
===========
ShamCheckAttack

The player is in view, so decide to move or launch an attack
Returns false if movement should continue
============
*/
float ShamCheckAttack ()
{
	vec3_t		spot1, spot2, delta;
	gedict_t	*targ;

	if ( enemy_range == RANGE_MELEE )
	{
		if ( CanDamage( PROG_TO_EDICT( self->s.v.enemy ), self ) )
		{
			self->attack_state = AS_MELEE;
			return true;
		}
	}

	if ( g_globalvars.time < self->attack_finished )
		return false;

	if ( !enemy_vis )
		return false;

	targ = PROG_TO_EDICT( self->s.v.enemy );

	// see if any entities are in the way of the shot
	VectorAdd( self->s.v.origin, self->s.v.view_ofs, spot1 );
	VectorAdd( targ->s.v.origin, targ->s.v.view_ofs, spot2 );

	VectorSubtract( spot1, spot2, delta );

	if ( vlen( delta ) > 600 )
		return false;

	traceline( PASSVEC3( spot1 ), PASSVEC3( spot2 ), false, self );

	if ( g_globalvars.trace_inopen && g_globalvars.trace_inwater )
		return false; // sight line crossed contents

	if ( PROG_TO_EDICT( g_globalvars.trace_ent ) != targ )
		return false;	// don't have a clear shot

	// missile attack
	if ( enemy_range == RANGE_FAR )
		return false;

	self->attack_state = AS_MISSILE;
	SUB_AttackFinished( 2 + 2 * g_random() );

	return true;
}

//============================================================================

/*QUAKED monster_shambler (1 0 0) (-32 -32 -24) (32 32 64) Ambush
*/
void SP_monster_shambler()
{
	if ( deathmatch )
	{
		ent_remove( self );
		return;
	}

	safe_precache_model( "progs/shambler.mdl" );
	safe_precache_model( "progs/s_light.mdl" );
	safe_precache_model( "progs/h_shams.mdl" );
	safe_precache_model( "progs/bolt.mdl" );
	                     
	safe_precache_sound( "shambler/sattck1.wav" );
	safe_precache_sound( "shambler/sboom.wav" );
	safe_precache_sound( "shambler/sdeath.wav" );
	safe_precache_sound( "shambler/shurt2.wav" );
	safe_precache_sound( "shambler/sidle.wav" );
	safe_precache_sound( "shambler/ssight.wav" );
	safe_precache_sound( "shambler/melee1.wav" );
	safe_precache_sound( "shambler/melee2.wav" );
	safe_precache_sound( "shambler/smack.wav" );

	setsize( self, PASSVEC3( VEC_HULL2_MIN ),  PASSVEC3( VEC_HULL2_MAX ) );
	self->s.v.health = 600;

	self->th_stand   = sham_stand1;
	self->th_walk    = sham_walk1;
	self->th_run     = sham_run1;
	self->th_die     = sham_die;
	self->th_melee   = sham_melee;
	self->th_missile = sham_magic1;
	self->th_pain    = sham_pain;

	self->th_respawn = SP_monster_shambler;

	walkmonster_start( "progs/shambler.mdl" );
}
