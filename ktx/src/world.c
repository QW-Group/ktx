/*
 *  QWProgs-DM
 *  Copyright (C) 2004  [sd] angel
 *
 *  This code is based on QuakeWorld DM mod code by Id Software, Inc.
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *  $Id: world.c,v 1.1.1.1 2005/09/24 12:45:07 disconn3ct Exp $
 */

#include "g_local.h"

#ifdef KTEAMS
float CountALLPlayers ();
#endif


#define MAX_BODYQUE 4
gedict_t       *bodyque[MAX_BODYQUE];
int             bodyque_head;
void InitBodyQue()
{
	int             i;

	bodyque[0] = spawn();
	bodyque[0]->s.v.classname = "bodyque";
	for ( i = 1; i < MAX_BODYQUE; i++ )
	{
		bodyque[i] = spawn();
		bodyque[i]->s.v.classname = "bodyque";
		bodyque[i - 1]->s.v.owner = EDICT_TO_PROG( bodyque[i] );
	}
	bodyque[MAX_BODYQUE - 1]->s.v.owner = EDICT_TO_PROG( bodyque[0] );
	bodyque_head = 0;
}

// make a body que entry for the given ent so the ent can be
// respawned elsewhere
void CopyToBodyQue( gedict_t * ent )
{
	VectorCopy( ent->s.v.angles, bodyque[bodyque_head]->s.v.angles );
	VectorCopy( ent->s.v.velocity, bodyque[bodyque_head]->s.v.velocity );

	bodyque[bodyque_head]->s.v.model = ent->s.v.model;
	bodyque[bodyque_head]->s.v.modelindex = ent->s.v.modelindex;
	bodyque[bodyque_head]->s.v.frame = ent->s.v.frame;
	bodyque[bodyque_head]->s.v.colormap = ent->s.v.colormap;
	bodyque[bodyque_head]->s.v.movetype = ent->s.v.movetype;
	bodyque[bodyque_head]->s.v.flags = 0;

	setorigin( bodyque[bodyque_head], PASSVEC3( ent->s.v.origin ) );
	setsize( bodyque[bodyque_head], PASSVEC3( ent->s.v.mins ),
		      PASSVEC3( ent->s.v.maxs ) );

	if ( ++bodyque_head >= MAX_BODYQUE )
		bodyque_head = 0;
}


gedict_t       *lastspawn;


void CheckDefMap ()
{
	float f1;
	char *s1;

	f1 = CountALLPlayers();
	if( !f1 && !atoi( ezinfokey( world, "k_master" ) )
			&& !atoi( ezinfokey( world, "k_lockmap" ) ) )
	{
		s1 = ezinfokey( world, "k_defmap" );

		if( !strnull( s1 ) && strneq( s1, g_globalvars.mapname ) )
			localcmd("map %s\n", s1);
	}

	ent_remove( self );
}


void  SP_item_artifact_super_damage();

void SP_worldspawn()
{
	char		*s;
	char 		*tmp;
   	gedict_t	*e;

    
    // Tonik: check if we're running on an 2.3x server
	tmp = ezinfokey( world, "*version" );
	if ( streq( tmp, "2.30") || streq( tmp, "2.33" ) )
		server_is_2_3x = true;

	G_SpawnString( "classname", "", &s );
	if ( Q_stricmp( s, "worldspawn" ) )
	{
		G_Error( "SP_worldspawn: The first entity isn't 'worldspawn'" );
	}
	world->s.v.classname = "worldspawn";
	lastspawn = world;
	InitBodyQue();

	if ( !Q_stricmp( self->s.v.model, "maps/e1m8.bsp" ) )
		trap_cvar_set( "sv_gravity", "100" );
	else
		trap_cvar_set( "sv_gravity", "800" );
// the area based ambient sounds MUST be the first precache_sounds

// player precaches     
	W_Precache();		// get weapon precaches

// sounds used from C physics code
	trap_precache_sound( "demon/dland2.wav" );	// landing thud
	trap_precache_sound( "misc/h2ohit1.wav" );	// landing splash

// sounds used from C physics code
	trap_precache_sound( "demon/dland2.wav" );	// landing thud
	trap_precache_sound( "misc/h2ohit1.wav" );	// landing splash

// setup precaches allways needed
	trap_precache_sound( "items/itembk2.wav" );	// item respawn sound
	trap_precache_sound( "player/plyrjmp8.wav" );	// player jump
	trap_precache_sound( "player/land.wav" );	// player landing
	trap_precache_sound( "player/land2.wav" );	// player hurt landing
	trap_precache_sound( "player/drown1.wav" );	// drowning pain
	trap_precache_sound( "player/drown2.wav" );	// drowning pain
	trap_precache_sound( "player/gasp1.wav" );	// gasping for air
	trap_precache_sound( "player/gasp2.wav" );	// taking breath
	trap_precache_sound( "player/h2odeath.wav" );	// drowning death

	trap_precache_sound( "misc/talk.wav" );	// talk
	trap_precache_sound( "player/teledth1.wav" );	// telefrag
	trap_precache_sound( "misc/r_tele1.wav" );	// teleport sounds
	trap_precache_sound( "misc/r_tele2.wav" );
	trap_precache_sound( "misc/r_tele3.wav" );
	trap_precache_sound( "misc/r_tele4.wav" );
	trap_precache_sound( "misc/r_tele5.wav" );
	trap_precache_sound( "weapons/lock4.wav" );	// ammo pick up
	trap_precache_sound( "weapons/pkup.wav" );	// weapon up
	trap_precache_sound( "items/armor1.wav" );	// armor up
	trap_precache_sound( "weapons/lhit.wav" );	//lightning
	trap_precache_sound( "weapons/lstart.wav" );	//lightning start
	trap_precache_sound( "items/damage3.wav" );

	trap_precache_sound( "misc/power.wav" );	//lightning for boss

// player gib sounds
	trap_precache_sound( "player/gib.wav" );	// player gib sound
	trap_precache_sound( "player/udeath.wav" );	// player gib sound
	trap_precache_sound( "player/tornoff2.wav" );	// gib sound

// player pain sounds

	trap_precache_sound( "player/pain1.wav" );
	trap_precache_sound( "player/pain2.wav" );
	trap_precache_sound( "player/pain3.wav" );
	trap_precache_sound( "player/pain4.wav" );
	trap_precache_sound( "player/pain5.wav" );
	trap_precache_sound( "player/pain6.wav" );

// player death sounds
	trap_precache_sound( "player/death1.wav" );
	trap_precache_sound( "player/death2.wav" );
	trap_precache_sound( "player/death3.wav" );
	trap_precache_sound( "player/death4.wav" );
	trap_precache_sound( "player/death5.wav" );

	trap_precache_sound( "boss1/sight1.wav" );

// ax sounds    
	trap_precache_sound( "weapons/ax1.wav" );	// ax swoosh
	trap_precache_sound( "player/axhit1.wav" );	// ax hit meat
	trap_precache_sound( "player/axhit2.wav" );	// ax hit world

	trap_precache_sound( "player/h2ojump.wav" );	// player jumping into water
	trap_precache_sound( "player/slimbrn2.wav" );	// player enter slime
	trap_precache_sound( "player/inh2o.wav" );	// player enter water
	trap_precache_sound( "player/inlava.wav" );	// player enter lava
	trap_precache_sound( "misc/outwater.wav" );	// leaving water sound

	trap_precache_sound( "player/lburn1.wav" );	// lava burn
	trap_precache_sound( "player/lburn2.wav" );	// lava burn

	trap_precache_sound( "misc/water1.wav" );	// swimming
	trap_precache_sound( "misc/water2.wav" );	// swimming

// Invulnerability sounds
	trap_precache_sound( "items/protect.wav" );
	trap_precache_sound( "items/protect2.wav" );
	trap_precache_sound( "items/protect3.wav" );


	trap_precache_model( "progs/player.mdl" );
	trap_precache_model( "progs/eyes.mdl" );
	trap_precache_model( "progs/h_player.mdl" );
	trap_precache_model( "progs/gib1.mdl" );
	trap_precache_model( "progs/gib2.mdl" );
	trap_precache_model( "progs/gib3.mdl" );

	trap_precache_model( "progs/s_bubble.spr" );	// drowning bubbles
	trap_precache_model( "progs/s_explod.spr" );	// sprite explosion

	trap_precache_model( "progs/v_axe.mdl" );
	trap_precache_model( "progs/v_shot.mdl" );
	trap_precache_model( "progs/v_nail.mdl" );
	trap_precache_model( "progs/v_rock.mdl" );
	trap_precache_model( "progs/v_shot2.mdl" );
	trap_precache_model( "progs/v_nail2.mdl" );
	trap_precache_model( "progs/v_rock2.mdl" );

	trap_precache_model( "progs/bolt.mdl" );	// for lightning gun
	trap_precache_model( "progs/bolt2.mdl" );	// for lightning gun
	trap_precache_model( "progs/bolt3.mdl" );	// for boss shock
	trap_precache_model( "progs/lavaball.mdl" );	// for testing

	trap_precache_model( "progs/missile.mdl" );
	trap_precache_model( "progs/grenade.mdl" );
	trap_precache_model( "progs/spike.mdl" );
	trap_precache_model( "progs/s_spike.mdl" );

	trap_precache_model( "progs/backpack.mdl" );

	trap_precache_model( "progs/zom_gib.mdl" );

	trap_precache_model( "progs/v_light.mdl" );

//
// Setup light animation tables. 'a' is total darkness, 'z' is maxbright.
//

	// 0 normal
	trap_lightstyle( 0, "m" );

	// 1 FLICKER (first variety)
	trap_lightstyle( 1, "mmnmmommommnonmmonqnmmo" );

	// 2 SLOW STRONG PULSE
	trap_lightstyle( 2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba" );

	// 3 CANDLE (first variety)
	trap_lightstyle( 3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg" );

	// 4 FAST STROBE
	trap_lightstyle( 4, "mamamamamama" );

	// 5 GENTLE PULSE 1
	trap_lightstyle( 5, "jklmnopqrstuvwxyzyxwvutsrqponmlkj" );

	// 6 FLICKER (second variety)
	trap_lightstyle( 6, "nmonqnmomnmomomno" );

	// 7 CANDLE (second variety)
	trap_lightstyle( 7, "mmmaaaabcdefgmmmmaaaammmaamm" );

	// 8 CANDLE (third variety)
	trap_lightstyle( 8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa" );

	// 9 SLOW STROBE (fourth variety)
	trap_lightstyle( 9, "aaaaaaaazzzzzzzz" );

	// 10 FLUORESCENT FLICKER
	trap_lightstyle( 10, "mmamammmmammamamaaamammma" );

	// 11 SLOW PULSE NOT FADE TO BLACK
	trap_lightstyle( 11, "abcdefghijklmnopqrrqponmlkjihgfedcba" );

	// styles 32-62 are assigned by the light program for switchable lights

	// 63 testing
	trap_lightstyle( 63, "a" );

	match_over = 0;
	k_standby = 0;
	lock = 1;
	localcmd("serverinfo status Standby\n");
	localcmd("localinfo sv_spectalk 1\n");
	e = find(world, FOFCLSN, "mapguard");
	while( e ) {
		e->s.v.nextthink = g_globalvars.time + 0.1;
		e->s.v.think = ( func_t ) SUB_Remove;

		e = find(e, FOFCLSN, "mapguard");
	}

	e = spawn();
	e->s.v.classname = "mapguard";
	e->s.v.owner = EDICT_TO_PROG( world );
	e->s.v.think = ( func_t ) CheckDefMap;
	e->s.v.nextthink = g_globalvars.time + 60;

	// spawn quad if map is aerowalk in this case
	if ( atoi ( ezinfokey(world, "add_q_aerowalk") )
			&& streq( "aerowalk", g_globalvars.mapname) ) {
   		gedict_t	*swp = self;

		self = spawn();
		setorigin( self, -912.5f, -898.875f, 248.0f );
		self->s.v.owner = EDICT_TO_PROG( world );
		self->s.v.classname = "item_artifact_super_damage";
		SP_item_artifact_super_damage();

		self = swp; // restore self
	}

	Init_cmds();
}

int             timelimit, fraglimit, teamplay, deathmatch, framecount;

float		rj;

extern float intermission_exittime;

void ModPause (int pause);
void CheckTiming();

void StartFrame( int time )
{
    k_maxspeed = cvar( "sv_maxspeed" );
	timelimit  = cvar( "timelimit" );
	fraglimit  = cvar( "fraglimit" );
	teamplay   = cvar( "teamplay" );
	deathmatch = cvar( "deathmatch" );

// oldman --> don't allow unlimited timelimit + fraglimit
    if( timelimit == 0 && fraglimit == 0 )
    {
		int k_timetop = atoi( ezinfokey( world, "k_timetop") );

		timelimit = k_timetop ? k_timetop : 20;   // sensible default if no max set

        cvar_set("timelimit", va("%d", (int)timelimit));
    }
// <-- oldman

	framecount++;
	framechecks = bound( 0, !atoi( ezinfokey( world, "k_noframechecks" ) ), 1 );

// Tonik: note current "serverinfo maxfps" setting
// (we don't want to do it in every player frame)
	current_maxfps = atoi( ezinfokey( world, "maxfps" ) );
	if ( !current_maxfps )
		current_maxfps = 72;	// 2.30 standard

	CheckTiming(); // check if client lagged or returned from lag

	if ( !CountALLPlayers() && k_pause ) {
		G_bprint(2, "No players left, unpausing.\n");
		ModPause ( 0 );
	}

	if ( intermission_running && g_globalvars.time >= intermission_exittime - 1 
			&& !strnull( cvar_string( "serverdemo" ) ) )
		localcmd("stop\n"); // demo is recording, stop it and save
}

