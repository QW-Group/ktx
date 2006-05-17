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
 *  $Id: world.c,v 1.47 2006/05/17 20:57:12 oldmanuk Exp $
 */

#include "g_local.h"

float CountALLPlayers ();

void  SUB_regen();

void  FixSpecWizards ();

void  FixSayFloodProtect();

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
	if( !f1 && !cvar( "k_master" )
			&& !cvar( "k_lockmap" ) )
	{
		s1 = cvar_string( "k_defmap" );

		if( !strnull( s1 ) && strneq( s1, g_globalvars.mapname ) )
			changelevel( s1 );
	}

	ent_remove( self );
}


void	SP_item_artifact_super_damage();

void SP_worldspawn()
{
	char 		*lastmap = cvar_string("_k_lastmap");
	char		*s;
   	gedict_t	*e;

/* removed k_srvcfgmap
	// exec configs/maps/default.cfg
	// exec configs/maps/mapname.cfg
	if (    cvar( "k_srvcfgmap" )
		 && (    strnull( lastmap ) // server just spawn first time ?
			  || strneq( lastmap, g_globalvars.mapname )
			)
	   ) {
		char *cfg_name;

		cfg_name = "configs/maps/default.cfg";
		if ( can_exec( cfg_name ) )
			localcmd("exec %s\n", cfg_name);

		cfg_name = va("configs/maps/%s.cfg", g_globalvars.mapname);
		if ( can_exec( cfg_name ) )
			localcmd("exec %s\n", cfg_name);

		trap_executecmd ();
	}
*/
	// since we remove k_srvcfgmap, we need configure different maps in matchless mode.
	// doing this by execiting configs like we do for "ffa" command in _non_ matchless mode
	if ( k_matchLess ) {
		int um_idx = um_idx_byname("ffa");

		if ( um_idx >= 0 )
			cvar_fset("_k_last_xonx", um_idx + 1); // force server call "ffa" user mode
		else {
			G_bprint(2, "SP_worldspawn: um_idx_byname fail\n"); // shout
			cvar_fset("_k_last_xonx", 0);
		}
	}

	if ( cvar("_k_last_xonx") > 0 && strneq( lastmap, g_globalvars.mapname ) )
		UserMode( -cvar("_k_last_xonx") ); // auto call XonX command if map switched to another
    
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

// quad sounds - need this due to aerowalk customize
	trap_precache_sound( "items/damage.wav" );
	trap_precache_sound( "items/damage2.wav" );
	trap_precache_sound( "items/damage3.wav" );

// ctf
	if ( k_allowed_free_modes & UM_CTF ) {
		trap_precache_sound( "weapons/chain1.wav" );
		trap_precache_sound( "weapons/chain2.wav" );
		trap_precache_sound( "weapons/chain3.wav" );
		trap_precache_sound( "weapons/bounce2.wav" );
		trap_precache_sound( "misc/flagtk.wav" );
		trap_precache_sound( "misc/flagcap.wav" );
		trap_precache_sound( "doors/runetry.wav" );
		trap_precache_sound( "blob/land1.wav" );
		trap_precache_sound( "rune/rune1.wav" );
		trap_precache_sound( "rune/rune2.wav" );
		trap_precache_sound( "rune/rune22.wav" );
		trap_precache_sound( "rune/rune3.wav" );
		trap_precache_sound( "rune/rune4.wav" );
	}

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

	trap_precache_model( "progs/wizard.mdl" );

	if ( k_ctf_custom_models ) {
		trap_precache_model( "progs/v_star.mdl" );
		trap_precache_model( "progs/bit.mdl" );
		trap_precache_model( "progs/star.mdl" );
		trap_precache_model( "progs/flag.mdl" );
	}
	else
	{
		trap_precache_model( "progs/v_spike.mdl" );
		trap_precache_model( "progs/w_g_key.mdl" );
		trap_precache_model( "progs/w_s_key.mdl" );
	}

// ctf runes
	if ( k_allowed_free_modes & UM_CTF ) {
		trap_precache_model( "progs/end1.mdl" );
		trap_precache_model( "progs/end2.mdl" );
		trap_precache_model( "progs/end3.mdl" );
		trap_precache_model( "progs/end4.mdl" );
	}

// quad mdl - need this due to aerowalk customize
	trap_precache_model( "progs/quaddama.mdl" );

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
	lock = k_matchLess ? 0 : 1; // no server lockining in matchLess mode
	localcmd("serverinfo status Standby\n");

	e = find(world, FOFCLSN, "mapguard");
	while( e ) {
		e->s.v.nextthink = g_globalvars.time + 0.1;
		e->s.v.think = ( func_t ) SUB_Remove;

		e = find(e, FOFCLSN, "mapguard");
	}

	if ( !k_matchLess ) { // no defmap in matchLess mode
		e = spawn();
		e->s.v.classname = "mapguard";
		e->s.v.owner = EDICT_TO_PROG( world );
		e->s.v.think = ( func_t ) CheckDefMap;
		e->s.v.nextthink = g_globalvars.time + 60;
	}

	if ( !k_matchLess ) // skip practice in matchLess mode
	if ( cvar( "srv_practice_mode" ) ) // #practice mode#
		SetPractice( cvar( "srv_practice_mode" ), NULL ); // may not reload map
}

void SpawnCTFItem( char* classname, float x, float y, float z, float angle );
void Customize_Maps()
{
	gedict_t *p = world;

	// spawn quad if map is aerowalk in this case
	if ( cvar("add_q_aerowalk") && streq( "aerowalk", g_globalvars.mapname) ) {
   		gedict_t	*swp = self;

		self = spawn();
		setorigin( self, -912.6f, -898.9f, 248.0f ); // oh, ktpro like
		self->s.v.owner = EDICT_TO_PROG( world );
		SP_item_artifact_super_damage();

		self = swp; // restore self
	}

	if ( !cvar("k_end_tele_spawn") && streq( "end", g_globalvars.mapname) ) {
		vec3_t      TS_ORIGIN = { -392, 608, 40 }; // tele spawn

		while( (p = find( p, FOFCLSN, "info_player_deathmatch" )) )
			if ( VectorCompare(p->s.v.origin, TS_ORIGIN) ) {
				ent_remove( p );
				break;
			}
	}

	// Add ctf items to id maps
	if ( k_allowed_free_modes & UM_CTF )
	{
		if ( streq( "e1m7", g_globalvars.mapname ) )
		{
			SpawnCTFItem( "item_flag_team1"  ,  839,    62,  148,    0 );
			SpawnCTFItem( "item_flag_team2"  , -532,    57,   32,    0 );
			SpawnCTFItem( "info_player_team1",  930,   400,  216,  180 );
			SpawnCTFItem( "info_player_team1",  930,   225,  216,  180 );
			SpawnCTFItem( "info_player_team1",  930,   100,  216,  180 );
			SpawnCTFItem( "info_player_team1",  930,  -250,  216,  180 );
			SpawnCTFItem( "info_player_team2", -532,    57,   56,    0 );
			SpawnCTFItem( "info_player_team2", -472,   117,   56,    0 );
			SpawnCTFItem( "info_player_team2", -472,    57,   56,    0 );
			SpawnCTFItem( "info_player_team2", -472,    -3,   56,    0 );
		}
		else if ( streq( "e2m1", g_globalvars.mapname ) )
		{
			SpawnCTFItem( "item_flag_team1"  , 1800,   416,  244,  90 );
			SpawnCTFItem( "item_flag_team2"  , -300,   -25,  -30, 180 );
			SpawnCTFItem( "info_player_team1", 1836,   570,  248,  90 );
			SpawnCTFItem( "info_player_team1", 1736,   570,  248,  90 );
			SpawnCTFItem( "info_player_team1", 1836,   490,  248,  90 );
			SpawnCTFItem( "info_player_team1", 1736,   490,  248,  90 );
			SpawnCTFItem( "info_player_team1", 1836,   410,  248,  90 );
			SpawnCTFItem( "info_player_team1", 1736,   410,  248,  90 );
			SpawnCTFItem( "info_player_team2",   42,   162,  -40,   0 );
			SpawnCTFItem( "info_player_team2",  -62,   162,  -40,   0 );
			SpawnCTFItem( "info_player_team2", -142,   162,  -40,   0 );
			SpawnCTFItem( "info_player_team2",   42,    82,  -40,   0 );
			SpawnCTFItem( "info_player_team2",  -62,    82,  -40,   0 );
			SpawnCTFItem( "info_player_team2", -142,    82,  -40,   0 );
		}
		else if ( streq( "e2m2", g_globalvars.mapname ) ) 
		{
			SpawnCTFItem( "item_flag_team1"  , -256, -1952,  300,  90 );
			SpawnCTFItem( "item_flag_team2"  , 2062,  -176,  240,   0 );
			SpawnCTFItem( "info_player_team1", -186, -1724,  248,  90 );
			SpawnCTFItem( "info_player_team1", -266, -1724,  248,  90 );
			SpawnCTFItem( "info_player_team1", -346, -1724,  248,  90 );
			SpawnCTFItem( "info_player_team1", -176, -1872,  264,  90 );
			SpawnCTFItem( "info_player_team1", -256, -1872,  264,  90 );
			SpawnCTFItem( "info_player_team1", -336, -1872,  264,  90 );
			SpawnCTFItem( "info_player_team2", 1902,   -76,  200, 180 );
			SpawnCTFItem( "info_player_team2", 1902,  -176,  200, 180 );
			SpawnCTFItem( "info_player_team2", 1902,  -276,  200, 180 );
			SpawnCTFItem( "info_player_team2", 1982,   -76,  200, 180 );
			SpawnCTFItem( "info_player_team2", 1982,  -176,  200, 180 );
			SpawnCTFItem( "info_player_team2", 1982,  -276,  200, 180 );
		}
		else if ( streq( "e2m5", g_globalvars.mapname ) )
		{
			SpawnCTFItem( "item_flag_team1"  , -856, -1296, -200,  90 );
			SpawnCTFItem( "item_flag_team2"  ,-1246,  2944,  -76,   0 );
			SpawnCTFItem( "info_player_team1", -956, -1146, -216,  90 );
			SpawnCTFItem( "info_player_team1", -856, -1146, -216,  90 );
			SpawnCTFItem( "info_player_team1", -756, -1146, -216,  90 );
			SpawnCTFItem( "info_player_team1", -956, -1246, -232,  90 );
			SpawnCTFItem( "info_player_team1", -856, -1246, -232,  90 );
			SpawnCTFItem( "info_player_team1", -756, -1246, -232,  90 );
			SpawnCTFItem( "info_player_team2",-1056,  3044,  -48,   0 );
			SpawnCTFItem( "info_player_team2",-1056,  2944,  -48,   0 );
			SpawnCTFItem( "info_player_team2",-1056,  2844,  -48,   0 );
			SpawnCTFItem( "info_player_team2",-1200,  3044,  -96,   0 );
			SpawnCTFItem( "info_player_team2",-1200,  2944,  -96,   0 );
			SpawnCTFItem( "info_player_team2",-1200,  2844,  -96,   0 );		
		}
		else if ( streq( "e4m3", g_globalvars.mapname ) )
		{
			SpawnCTFItem( "item_flag_team1"  , 1182, -1647,  172,   0 );
			SpawnCTFItem( "item_flag_team2"  , 2200,   544,  -60,   0 );
			SpawnCTFItem( "info_player_team1", 1232, -1347,  152,  90 );
			SpawnCTFItem( "info_player_team1", 1232, -1447,  152,  90 );
			SpawnCTFItem( "info_player_team1", 1232, -1547,  152,  90 );
			SpawnCTFItem( "info_player_team1", 1132, -1347,  152,  90 );
			SpawnCTFItem( "info_player_team1", 1132, -1447,  152,  90 );
			SpawnCTFItem( "info_player_team1", 1132, -1547,  152,  90 );
			SpawnCTFItem( "info_player_team2", 2240,   -64, -104, 180 );
			SpawnCTFItem( "info_player_team2", 2244,   110, -104, 180 );
			SpawnCTFItem( "info_player_team2", 2370,   273, -104, 180 );
			SpawnCTFItem( "info_player_team2", 2599,   269, -104, 180 );
			SpawnCTFItem( "info_player_team2", 2630,   430, -104, 180 );
			SpawnCTFItem( "info_player_team2", 2447,   439, -104, 180 );
		}
		else if ( streq( "ctf8", g_globalvars.mapname ) )
		{
			// remove some bad spawns from ctf8
			vec3_t spawn1 = {  1704, -540, 208 }; // blue spawn in red base
			vec3_t spawn2 = { -1132,  -72, 208 }; // red spawn in blue base
			// vec3_t spawn3 = {   660,  256, 400 }; // red spawn at quad

			while( (p = find( p, FOFCLSN, "info_player_team2" )) )
				if ( VectorCompare( p->s.v.origin, spawn1 ) ) {
					ent_remove( p );
					break;
				}

			while( (p = find( p, FOFCLSN, "info_player_team1" )) )
				if ( VectorCompare( p->s.v.origin, spawn2 ) ) { 
					ent_remove( p );
					break;
				}
		}
	}
}

// create cvar via 'set' command
// FIXME: unfortunately with current API I can't check if cvar already exist
qboolean RegisterCvar ( const char *var )
{

	if ( !strnull( cvar_string( var ) ) ) {
//		G_cprint("RegisterCvar: \"%s\" already exist, value is \"%s\"\n", var, cvar_string( var ));
		return false;
	}
	else {
		// FIXME: some hack to check if cvar already exist, this check may give wrong results
		// thats all i can do with current api
		char *save = cvar_string( var );

		cvar_set(var, "~SomEHacK~~SomEHacK~");
		if ( !strnull( cvar_string( var ) ) ) { // ok, cvar exist but was empty
			cvar_set(var, save); // restore empty string %)
//			G_cprint("RegisterCvar: \"%s\" already exist\n", var);
			return false;
		}
		// but cvar_set may fail, if cvar is ROM for example
		// so, if cvar is empty and ROM we can't guess is this cvar exist
	}

//	G_cprint("RegisterCvar: \"%s\" registered\n", var);
	localcmd("set \"%s\" \"\"\n", var);
	trap_executecmd ();
	return true;
}

// in the first frame - even world is not spawned yet
void FirstFrame	( )
{
	int i;

	if ( framecount != 1 )
		return;

	trap_executecmd ();

	RegisterCvar("_k_last_xonx"); // internal usage, save last XonX command
	RegisterCvar("_k_lastmap");	  // internal usage, name of last map
	RegisterCvar("_k_players");   // internal usage, count of players on last map
	RegisterCvar("_k_pow_last");  // internal usage, k_pow from last map

	RegisterCvar("k_mode");
	RegisterCvar("k_matchless");
	RegisterCvar("k_matchless_countdown");
	RegisterCvar("k_disallow_kfjump");
	RegisterCvar("k_disallow_krjump");
	RegisterCvar("k_lock_hdp");
	RegisterCvar("k_disallow_weapons");

// removed k_srvcfgmap
//	RegisterCvar("k_srvcfgmap");
	RegisterCvar("k_pow_min_players");
	RegisterCvar("k_pow_check_time");
	RegisterCvar("allow_spec_wizard");
	RegisterCvar("k_no_wizard_animation"); // disallow wizard animation

	RegisterCvar("k_vp_break");   // votes percentage for stopping the match voting
	RegisterCvar("k_vp_admin");   // votes percentage for admin election
	RegisterCvar("k_vp_captain"); // votes percentage for captain election
	RegisterCvar("k_vp_map");     // votes percentage for map change voting
	RegisterCvar("k_vp_pickup");  // votes percentage for pickup voting
	RegisterCvar("k_vp_rpickup"); // votes percentage for rpickup voting

	RegisterCvar("k_end_tele_spawn"); // don't remove end tele spawn

	RegisterCvar("k_motd_time"); 	  // motd time in seconds

// >>> convert localinfo to set
	RegisterCvar("k_admincode");
	RegisterCvar("k_prewar");
	RegisterCvar("k_lockmap");
	RegisterCvar("k_master");
	RegisterCvar("k_fallbunny");
	RegisterCvar("timing_players_time");
	RegisterCvar("timing_players_action");
	RegisterCvar("allow_timing");
	RegisterCvar("demo_scoreslength");
	RegisterCvar("k_bzk");
	RegisterCvar("lock_practice");
	RegisterCvar("k_autoreset");
	RegisterCvar("k_defmap");
	RegisterCvar("k_admins");
	RegisterCvar("k_overtime");
	RegisterCvar("k_exttime");
	RegisterCvar("k_spw");
	RegisterCvar("k_lockmin");
	RegisterCvar("k_lockmax");
	RegisterCvar("k_spectalk");
	RegisterCvar("k_sayteam_to_spec");
	RegisterCvar("k_666");
	RegisterCvar("k_dis");
	RegisterCvar("dq");
	RegisterCvar("dr");
	RegisterCvar("dp");
	RegisterCvar("k_frp");
	RegisterCvar("k_highspeed");
	RegisterCvar("k_btime");
	RegisterCvar("k_freeze");
	RegisterCvar("k_free_mode");
	RegisterCvar("k_allowed_free_modes");
	RegisterCvar("allow_toggle_practice");
	RegisterCvar("k_pow");
	RegisterCvar("k_remove_end_hurt");
	RegisterCvar("k_allowvoteadmin");
	RegisterCvar("k_maxrate");
	RegisterCvar("k_minrate");
	RegisterCvar("k_sready");
	RegisterCvar("k_idletime");
	RegisterCvar("k_timetop");
	RegisterCvar("k_dm2mod");
	RegisterCvar("k_membercount");
	RegisterCvar("demo_tmp_record");
	RegisterCvar("demo_skip_ktffa_record");
	RegisterCvar("k_count");
	RegisterCvar("k_exclusive");
	RegisterCvar("k_short_gib");
	RegisterCvar("k_ann");
	RegisterCvar("srv_practice_mode");
	RegisterCvar("add_q_aerowalk");
	RegisterCvar("k_noframechecks");
	RegisterCvar("dmm4_invinc_time");
// removed
//	RegisterCvar("rj");
	RegisterCvar("k_no_fps_physics");
//{ ctf
	RegisterCvar("k_ctf_custom_models");
//}
	RegisterCvar("k_spec_info");
	RegisterCvar("k_no_vote_break");
	RegisterCvar("k_no_vote_map");
	RegisterCvar("k_midair");

// { cmd flood protection
	RegisterCvar("k_cmd_fp_count");
	RegisterCvar("k_cmd_fp_per");
	RegisterCvar("k_cmd_fp_for");
	RegisterCvar("k_cmd_fp_kick");
	RegisterCvar("k_cmd_fp_dontkick");
	RegisterCvar("k_cmd_fp_disabled");
// }

	RegisterCvar("k_demo_mintime");
	RegisterCvar("k_dmm4_gren_mode");
	RegisterCvar("k_fp"); // say team floodprot

	RegisterCvar("_k_captteam1"); // internal mod usage
	RegisterCvar("_k_captcolor1"); // internal mod usage
	RegisterCvar("_k_captteam2"); // internal mod usage
	RegisterCvar("_k_captcolor2"); // internal mod usage
	RegisterCvar("_k_team1"); // internal mod usage
	RegisterCvar("_k_team2"); // internal mod usage
	RegisterCvar("_k_host"); // internal mod usage

// { lastscores support

	RegisterCvar("__k_ls");  // current lastscore, really internal mod usage

	for ( i = 0; i < MAX_LASTSCORES; i++ ) {
		RegisterCvar(va("__k_ls_m_%d", i));  // mode, really internal mod usage
		RegisterCvar(va("__k_ls_e1_%d", i)); // entry team/nick, really internal mod usage
		RegisterCvar(va("__k_ls_e2_%d", i)); // entry team/nick, really internal mod usage
		RegisterCvar(va("__k_ls_t1_%d", i)); // nicks, really internal mod usage
		RegisterCvar(va("__k_ls_t2_%d", i)); // nicks, really internal mod usage
		RegisterCvar(va("__k_ls_s_%d", i));  // scores, really internal mod usage
	}

// }

// <<<

	// below globals changed only here

	k_matchLess = cvar( "k_matchless" );
	k_allowed_free_modes = cvar( "k_allowed_free_modes" );
	if ( k_matchLess )
		k_allowed_free_modes |= UM_FFA;
	// do not precache models if CTF is not really allowed
	k_ctf_custom_models = cvar( "k_ctf_custom_models" ) && (k_allowed_free_modes & UM_CTF);
}


// items spawned, but probably not solid yet
void SecondFrame ( )
{
	if ( framecount != 2 )
		return;

	Customize_Maps();

	if ( isCTF() )
		SpawnRunes();
}

void hide_powerups ( char *classname )
{
	gedict_t *p = world;

	if ( strnull( classname ) )
		G_Error("hide_items");

	while( (p = find(p, FOFCLSN, classname)) ) {
		p->s.v.solid = SOLID_NOT;
 		p->s.v.model = "";
		if ( p->s.v.think == ( func_t ) SUB_regen ) {
			p->nthink = p->s.v.nextthink > 0 ? p->s.v.nextthink : 0; // save respawn time
			p->s.v.nextthink = 0;  // disable item auto respawn
		}
	}
}

void show_powerups ( char *classname )
{
	gedict_t *p = world, *swp;

	if ( strnull( classname ) )
		G_Error("show_items");

	swp = self; 

	while( (p = find(p, FOFCLSN, classname)) ) {
		self = p; // WARNING

		// spawn item if not yet so
		if ( strnull( self->s.v.model ) || self->s.v.solid != SOLID_TRIGGER ) {
			if ( self->s.v.think == ( func_t ) SUB_regen && self->nthink > 0 )
				self->s.v.nextthink	= self->nthink; // spawn at this time
			else
				SUB_regen(); // spawn suddenly
		}
	}

	self  = swp;
}

// called when switching to/from ctf mode
void FixCTFItems()
{
	gedict_t *e = world;

	if ( isCTF() )
	{
		RegenFlags();
		SpawnRunes();
		while ( (e = find( e, FOFCLSN, "player" )) )
			e->s.v.items = (int) e->s.v.items | IT_HOOK;
	}
	else
	{
		while ( (e = find( e, FOFCLSN, "player" )) )
			e->s.v.items -= (int) e->s.v.items & IT_HOOK;

		e = find( world, FOFCLSN, "item_flag_team1" );
		if ( e )
		{
			e->s.v.touch = (func_t) SUB_Null;
			setmodel( e, "" );
		}

		e = find( world, FOFCLSN, "item_flag_team2" );
		if ( e )
		{
			e->s.v.touch = (func_t) SUB_Null;
			setmodel( e, "" );
		}

		while ( (e = find( e, FOFCLSN, "rune" )) )
			ent_remove( e );
	}
}

// serve k_pow and k_pow_min_players
void FixPowerups ()
{
	static int k_pow = -1; // static

	qboolean changed   = false;
	int 	 k_pow_new = Get_Powerups();

	if( k_pow != k_pow_new || framecount == 1 ) { // force on first frame
		changed = true;
		k_pow = k_pow_new;
	}

	if ( changed ) {
		if ( k_pow ) { // show powerups for players
			show_powerups( "item_artifact_invulnerability" );
			show_powerups( "item_artifact_super_damage" );
			show_powerups( "item_artifact_envirosuit" );
			show_powerups( "item_artifact_invisibility" );
		}
		else{ // hide powerups from players
			hide_powerups( "item_artifact_invulnerability" );
			hide_powerups( "item_artifact_super_damage" );
			hide_powerups( "item_artifact_envirosuit" );
			hide_powerups( "item_artifact_invisibility" );
		}
	}
}

void FixCmdFloodProtect ()
{
	k_cmd_fp_count = bound(0, cvar("k_cmd_fp_count"), MAX_FP_CMDS);
	k_cmd_fp_count = (k_cmd_fp_count ? k_cmd_fp_count : min(10, MAX_FP_CMDS));
	k_cmd_fp_per = bound(0, cvar("k_cmd_fp_per"), 30);
	k_cmd_fp_per = (k_cmd_fp_per ? k_cmd_fp_per : 4);
	k_cmd_fp_for = bound(0, cvar("k_cmd_fp_for"), 30);
	k_cmd_fp_for = (k_cmd_fp_for ? k_cmd_fp_for : 5);
	k_cmd_fp_kick = bound(0, cvar("k_cmd_fp_kick"), 10);
	k_cmd_fp_kick = (k_cmd_fp_kick ? k_cmd_fp_kick : 4);
	k_cmd_fp_dontkick = bound(0, cvar("k_cmd_fp_dontkick"), 1);
	k_cmd_fp_disabled = bound(0, cvar("k_cmd_fp_disabled"), 1);
}

void FixSayTeamToSpecs()
{
	int k_sayteam_to_spec = bound(0, cvar("k_sayteam_to_spec"), 3);

	switch ( k_sayteam_to_spec ) {
		case  0: if ( cvar("sv_sayteam_to_spec") )
					cvar_fset("sv_sayteam_to_spec", 0);
				 break;
		case  1: if ( match_in_progress )
					cvar_fset("sv_sayteam_to_spec", 1);
				 else
					cvar_fset("sv_sayteam_to_spec", 0);
				 break;
		case  2: if ( match_in_progress )
					cvar_fset("sv_sayteam_to_spec", 0);
				 else
					cvar_fset("sv_sayteam_to_spec", 1);
				 break;
		case  3:
		default: if ( !cvar("sv_sayteam_to_spec") )
					cvar_fset("sv_sayteam_to_spec", 1);
				 break;
	}
}

// check if server is misconfigured somehow, made some minimum fixage
void FixRules ( )
{
	gameType_t km = k_mode;
	int k_tt = bound( 0, cvar( "k_timetop" ), 600 );
	int	tp   = teamplay;
	int tl   = timelimit;
	int fl   = fraglimit;
	int dm   = deathmatch;
	int k_minr = bound(0, cvar( "k_minrate" ), 20000);
	int k_maxr = bound(0, cvar( "k_maxrate" ), 20000);	


	FixCmdFloodProtect(); // cmd flood protect

	FixSayFloodProtect(); // say flood protect

	FixSayTeamToSpecs(); // k_sayteam_to_spec

	// turn CTF off if CTF usermode is not allowed, due to precache_sound or precache_model
	if ( isCTF() && !( k_allowed_free_modes & UM_CTF ) )
		cvar_fset("k_mode", (float)( k_mode = gtTeam ));

	// we are does't support coop
	if ( cvar( "coop" ) )
		trap_cvar_set_float("coop", 0);

	// if unknown teamplay - disable it at all
	if ( teamplay != 0 && teamplay != 1 && teamplay != 2 && teamplay != 3 )
		trap_cvar_set_float("teamplay", (teamplay = 0));

	// if unknown deathmatch - set some default value
	if ( deathmatch != 1 && deathmatch != 2 && deathmatch != 3 && deathmatch != 4
		 && deathmatch != 5
	   )
		trap_cvar_set_float("deathmatch", (deathmatch = 3));

	if ( k_matchLess ) {
		if ( !isFFA() )
			trap_cvar_set_float("k_mode", (float)( k_mode = gtFFA ));
		if ( teamplay ) // sanity
			trap_cvar_set_float("teamplay", (teamplay = 0));
	}

	// if unknown k_mode - set some appropriate value
	if ( isUnknown() )
		trap_cvar_set_float("k_mode", (float)( k_mode = teamplay ? gtTeam : gtDuel ));

	// teamplay set, but gametype is not team, disable teamplay in this case
	if ( teamplay ) {
		if ( !isTeam() && !isCTF())
			trap_cvar_set_float("teamplay", (teamplay = 0));
	}
	
	// gametype is team, but teamplay has wrong value, set some default value
	// qqshka - CTF need some teamplay too?
	if ( isTeam() || isCTF() ) {
		if ( teamplay != 1 && teamplay != 2 && teamplay != 3 )
			trap_cvar_set_float("teamplay", (teamplay = 2));
	}

	if ( k_tt <= 0 ) { // this change does't broadcasted
		cvar_fset( "k_timetop", k_tt = 30 ); // sensible default if no max set
	}

// oldman --> don't allow unlimited timelimit + fraglimit
    if( timelimit == 0 && fraglimit == 0 ) {
        cvar_fset( "timelimit", timelimit = k_tt ); // sensible default if no max set
    }
// <-- oldman

	if ( !k_minr ) {
		cvar_fset( "k_minrate", k_minr = 500 );
	}
	if ( !k_maxr ) {
		cvar_fset( "k_maxrate", k_maxr = 10000 );
	}
	if ( k_minr > k_maxr ) {
		cvar_fset( "k_minrate", k_minr = k_maxr );
	}

	if ( cvar("k_midair") && deathmatch != 4 )
		cvar_fset( "k_midair", 0 ); // midair only in dmm4

	// ok, broadcast changes if any, a bit tech info, but this is misconfigured server
	// and must not happen on well configured servers, k?
	if (km != k_mode)
		G_bprint(2, "%s: k_mode changed to: %d\n", redtext("WARNING"), (int) k_mode);
	if (tp != teamplay)
		G_bprint(2, "%s: teamplay changed to: %d\n", redtext("WARNING"), teamplay);
	if (tl != timelimit)
		G_bprint(2, "%s: timelimit changed to: %d\n", redtext("WARNING"), timelimit);
	if (fl != fraglimit)
		G_bprint(2, "%s: fraglimit changed to: %d\n", redtext("WARNING"), fraglimit);
	if (dm != deathmatch)
		G_bprint(2, "%s: deathmatch changed to: %d\n", redtext("WARNING"), deathmatch);

	if ( framecount == 1 )
		trap_executecmd ();
}


int         timelimit, fraglimit, teamplay, deathmatch, framecount;

//float		rj;

extern float intermission_exittime;

void CheckTiming();
void Check_sready();

void StartFrame( int time )
{
	gameType_t old_k_mode = k_mode;

	framecount++;

	if ( framecount == 1 )
		FirstFrame();

	if ( framecount == 2 )
		SecondFrame ( );

//	rj = max( 0, cvar( "rj" ) ); 	// Set Rocket Jump Modifiers

    k_maxspeed = cvar( "sv_maxspeed" );
	timelimit  = cvar( "timelimit" );
	fraglimit  = cvar( "fraglimit" );
	teamplay   = cvar( "teamplay" );
	deathmatch = cvar( "deathmatch" );

    k_mode = cvar( "k_mode" );         

	// if modes have changed we may need to add/remove flags etc
	if ( k_mode != old_k_mode && framecount > 1 )
		FixCTFItems(); 

	FixRules ();

	FixPowerups ();

	FixSpecWizards ();

	framechecks = bound( 0, !cvar( "k_noframechecks" ), 1 );

	CalculateBestPlayers(); // autotrack stuff

// Tonik: note current "serverinfo maxfps" setting
// (we don't want to do it in every player frame)
	current_maxfps = iKey( world, "maxfps" );
	if ( !current_maxfps )
		current_maxfps = 72;	// 2.30 standard

	current_maxfps = bound(50, current_maxfps, 1981);

	CheckTiming(); // check if client lagged or returned from lag
	Check_sready(); // k_sready stuff

	if ( !CountALLPlayers() && k_pause ) {
		G_bprint(2, "No players left, unpausing.\n");
		ModPause ( 0 );
	}

	if ( intermission_running && g_globalvars.time >= intermission_exittime - 1 
			&& !strnull( cvar_string( "serverdemo" ) ) )
		localcmd("stop\n"); // demo is recording, stop it and save

	if ( k_matchLess && !match_in_progress )
		StartTimer(); // trying start countdown in matchless mode

	if ( framecount > 10 )
		vote_check_all ();
}

