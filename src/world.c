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
 *  $Id$
 */

#include "g_local.h"

void  SUB_regen();
void  CheckAll();
void  FixSpecWizards ();
void  FixSayFloodProtect();
void  FixRules ();
void  ShowSpawnPoints();
void  r_route();

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
	if ( ISLIVE( ent ) )
		return; // no corpse, since here may be frame where player live, so u got standing player model, will looks like bug

	VectorCopy( ent->s.v.angles, bodyque[bodyque_head]->s.v.angles );
	VectorCopy( ent->s.v.velocity, bodyque[bodyque_head]->s.v.velocity );

	bodyque[bodyque_head]->s.v.model = ent->s.v.model;
	bodyque[bodyque_head]->s.v.modelindex = ent->s.v.modelindex;
	bodyque[bodyque_head]->s.v.frame = ent->s.v.frame;
	bodyque[bodyque_head]->s.v.colormap = ent->s.v.colormap;
	// once i got here MOVETYPE_WALK, so server crashed, probaly here must be always toss movement
	bodyque[bodyque_head]->s.v.movetype = /* ent->s.v.movetype */ MOVETYPE_TOSS;
	bodyque[bodyque_head]->s.v.flags = 0;

	setorigin( bodyque[bodyque_head], PASSVEC3( ent->s.v.origin ) );
	setsize( bodyque[bodyque_head], PASSVEC3( ent->s.v.mins ), PASSVEC3( ent->s.v.maxs ) );

	if ( ++bodyque_head >= MAX_BODYQUE )
		bodyque_head = 0;
}

void ClearBodyQue()
{
	int             i;

	for ( i = 0; i < MAX_BODYQUE; i++ )
	{
		bodyque[i]->s.v.model = "";
		bodyque[i]->s.v.modelindex = 0;
		bodyque[i]->s.v.frame = 0;
		bodyque[i]->s.v.movetype = MOVETYPE_NONE;
	}
	bodyque_head = 0;
}

void CheckDefMap( )
{
	if( !CountPlayers() && !cvar( "k_lockmap" ) )
	{
		char *s1 = cvar_string( "k_defmap" );

		// reload map to default one if we are not on it alredy, in case of intermission reload anyway

		if( !strnull( s1 ) && strneq( s1, g_globalvars.mapname ) )
			changelevel( s1 );
		else if ( intermission_running )
			changelevel( g_globalvars.mapname );
	}

	ent_remove( self );
}

void Spawn_DefMapChecker( float timeout )
{
   	gedict_t	*e;

	for( e = world; (e = find(e, FOFCLSN, "mapguard")); )
		ent_remove( e );

	if ( k_matchLess ) // no defmap in matchLess mode
		return;

	e = spawn();

	e->s.v.classname = "mapguard";
	e->s.v.owner = EDICT_TO_PROG( world );
	e->s.v.think = ( func_t ) CheckDefMap;
	e->s.v.nextthink = g_globalvars.time + max(0.0001, timeout);
}

float max_map_uptime = 3600 * 12; // 12 hours

void Check_LongMapUptime()
{
	if ( match_in_progress )
		return; // no no no, not even bother with this during match

	if ( max_map_uptime > g_globalvars.time )
		return; // seems all ok

	max_map_uptime += (60 * 5); // so if map reloading fail, we repeat it after some time

	if ( CountPlayers() ) { // oh, here players, warn but not reload
		G_bprint(2, "\x87%s Long map uptime detected, reload map please!\n", redtext( "WARNING:" ));
		return;
	}

	G_bprint(2, "Long map uptime, reloading\n");
	changelevel( g_globalvars.mapname );
}

void	SP_item_artifact_super_damage();

void SP_worldspawn()
{
	char		*s;

	race_add_standard_routes();

	G_SpawnString( "classname", "", &s );
	if ( Q_stricmp( s, "worldspawn" ) )
	{
		G_Error( "SP_worldspawn: The first entity isn't 'worldspawn'" );
	}
	world->s.v.classname = "worldspawn";
	InitBodyQue();

	if ( !Q_stricmp( self->s.v.model, "maps/e1m8.bsp" ) )
		trap_cvar_set( "sv_gravity", "100" );
	else if ( !Q_stricmp( self->s.v.model, "maps/bunmoo3.bsp" ) )
		trap_cvar_set( "sv_gravity", "150" );
	else if ( !Q_stricmp( self->s.v.model, "maps/lowgrav.bsp" ) )
		trap_cvar_set( "sv_gravity", "150" );
	else
		trap_cvar_set( "sv_gravity", "800" );
// the area based ambient sounds MUST be the first precache_sounds

// player precaches     
	W_Precache();		// get weapon precaches

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

// Invisibility sounds
	trap_precache_sound( "items/inv1.wav" );
	trap_precache_sound( "items/inv2.wav" );
	trap_precache_sound( "items/inv3.wav" );

// quad sounds - need this due to aerowalk customize
	trap_precache_sound( "items/damage.wav" );
	trap_precache_sound( "items/damage2.wav" );
	trap_precache_sound( "items/damage3.wav" );

// ctf
#ifdef CTF_RELOADMAP
	if ( isCTF() ) // precache only if CTF is really on
#else
	if ( k_allowed_free_modes & UM_CTF ) // precache if CTF even only possible, does't matter is it on or off currently
#endif
	{
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

	if ( cvar("k_instagib_custom_models") ) // precache if custom models actived in config, even if instagib not yet activated 
	{
		trap_precache_model( "progs/v_coil.mdl" );	
		trap_precache_sound( "weapons/coilgun.wav" );
	}
	
	if ( cvar("k_spm_custom_model") )
		trap_precache_model( "progs/spawn.mdl" );
	
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

	// FIXME: checkextension in mvdsv?
    // vw_available = checkextension("ZQ_VWEP");
    vw_available = (FTE_sv ? 0 : 1);

    if(cvar("k_allow_vwep") && vw_available)
    {
        // precache our vwep models
        trap_precache_vwep_model ("progs/vwplayer.mdl");  // vwep-enabled player model to use
        trap_precache_vwep_model ("progs/w_axe.mdl");	// index 2
        trap_precache_vwep_model ("progs/w_shot.mdl");	// index 3
        trap_precache_vwep_model ("progs/w_shot2.mdl");
        trap_precache_vwep_model ("progs/w_nail.mdl");
        trap_precache_vwep_model ("progs/w_nail2.mdl");
        trap_precache_vwep_model ("progs/w_rock.mdl");
        trap_precache_vwep_model ("progs/w_rock2.mdl");
        trap_precache_vwep_model ("progs/w_light.mdl");
		if ( cvar("k_instagib_custom_models") )
			trap_precache_vwep_model ("progs/w_coil.mdl");	//index 10
        trap_precache_vwep_model ("-");			// null vwep model
    }
	vw_enabled = vw_available && cvar("k_allow_vwep") && cvar("k_vwep");

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

// ctf
	if ( k_ctf_custom_models ) {
		trap_precache_model( "progs/v_star.mdl" );
		trap_precache_model( "progs/bit.mdl" );
		trap_precache_model( "progs/star.mdl" );
		trap_precache_model( "progs/flag.mdl" );
	}
	else
	{
		trap_precache_model( "progs/v_spike.mdl" );
	}

// this used in alot of places, so precache it anyway
	trap_precache_model( "progs/w_g_key.mdl" );
	trap_precache_model( "progs/w_s_key.mdl" );

// ctf runes, actually may be precached anyway, since come with full quake distro
	if ( k_allowed_free_modes & UM_CTF ) {
		trap_precache_model( "progs/end1.mdl" );
		trap_precache_model( "progs/end2.mdl" );
		trap_precache_model( "progs/end3.mdl" );
		trap_precache_model( "progs/end4.mdl" );
	}

// quad mdl - need this due to aerowalk customize
	trap_precache_model( "progs/quaddama.mdl" );

// pent mdl - need this for race and coop
	trap_precache_model( "progs/invulner.mdl" );
	if ( cvar("k_race_custom_models") ) 
	{
		// precache if custom models actived in config
		trap_precache_model( "progs/start.mdl" );
		trap_precache_model( "progs/check.mdl" );
		trap_precache_model( "progs/finish.mdl" );
	}

// pent sounds - need for coop
	trap_precache_sound( "items/protect.wav" );
	trap_precache_sound( "items/protect2.wav" );
	trap_precache_sound( "items/protect3.wav" );

// suit wav - need this for race
	trap_precache_sound( "items/suit.wav" );
	trap_precache_model( "progs/suit.mdl" );
	trap_precache_sound( "items/suit2.wav" );

// for race
	trap_precache_sound( "knight/sword2.wav" );
	trap_precache_sound( "boss2/idle.wav" );
	trap_precache_sound( "boss2/sight.wav" );
	trap_precache_sound( "ambience/thunder1.wav" );
	trap_precache_sound( "enforcer/enfire.wav" );
	trap_precache_sound( "zombie/z_miss.wav" );

// g_models required for yawnmode weapondrops
	trap_precache_model( "progs/g_shot.mdl" );
	trap_precache_model( "progs/g_nail.mdl" );
	trap_precache_model( "progs/g_nail2.mdl" );
	trap_precache_model( "progs/g_rock.mdl" );
	trap_precache_model( "progs/g_rock2.mdl" );
	trap_precache_model( "progs/g_light.mdl" );

// for instagib bonus
	trap_precache_model( "progs/invisibl.mdl" );

// various items.

// health 15
	trap_precache_model( "maps/b_bh10.bsp" );
	trap_precache_sound( "items/r_item1.wav" );
// health 25
	trap_precache_model( "maps/b_bh25.bsp" );
	trap_precache_sound( "items/health1.wav" );
// megahealth
	trap_precache_model( "maps/b_bh100.bsp" );
	trap_precache_sound( "items/r_item2.wav" );
// armor
	trap_precache_model( "progs/armor.mdl" );
// shells 20
	trap_precache_model( "maps/b_shell0.bsp" );
// shells 40
	trap_precache_model( "maps/b_shell1.bsp" );
// nails 20/25
	trap_precache_model( "maps/b_nail0.bsp" );
// nails 40/50
	trap_precache_model( "maps/b_nail1.bsp" );
// rockets 5
	trap_precache_model( "maps/b_rock0.bsp" );
// rockets 10
	trap_precache_model( "maps/b_rock1.bsp" );
// cells 6
	trap_precache_model( "maps/b_batt0.bsp" );
// cells 12
	trap_precache_model( "maps/b_batt1.bsp" );

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
	localcmd("serverinfo status Standby\n");

	Spawn_DefMapChecker ( cvar("_k_worldspawns") == 1 ? 0.5 : 60 + g_random() * 30 );

	if ( !k_matchLess ) // skip practice in matchLess mode
	if ( cvar( "srv_practice_mode" ) ) // #practice mode#
		SetPractice( cvar( "srv_practice_mode" ), NULL ); // may not reload map
}

void ShowSpawnPoints();
void Customize_Maps()
{
	gedict_t *p;

	jumpf_flag = -650;

	if ( streq( "q1dm17", g_globalvars.mapname ) )
		jumpf_flag = -1000;

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

		for( p = world; (p = find( p, FOFCLSN, "info_player_deathmatch" )); )
			if ( VectorCompare(p->s.v.origin, TS_ORIGIN) ) {
				ent_remove( p );
				break;
			}
	}

	// correcting some teleport destintions on death32c (c) ktpro
	if ( streq( "death32c", g_globalvars.mapname ) )
		for( p = world; (p = find( p, FOFCLSN, "trigger_teleport" )); )
			if ( streq( "dm220", p->s.v.target ) )
				p->s.v.target = "dm6t1";

	// Modify some ctf maps
	if ( k_allowed_free_modes & UM_CTF )
	{

		if ( !cvar("k_ctf_based_spawn") && ( find_cnt(FOFCLSN, "info_player_deathmatch") <= 1 ) ) {
			G_sprint ( self, 2, "Spawn on base enforced due to map limitation\n" );
			cvar_fset( "k_ctf_based_spawn", 1);
		}

		if ( streq( "ctf8", g_globalvars.mapname ) )
		{
			// fix/remove some bad spawns from ctf8
			vec3_t spawn1 = {  1704, -540, 208 }; // blue spawn in red base
			vec3_t spawn2 = { -1132,  -72, 208 }; // red spawn in blue base
			vec3_t spawn3 = {   660,  256, 40 };  // red spawn at quad
			
			for( p = world; (p = find( p, FOFCLSN, "info_player_team2" )); )
				if ( VectorCompare( p->s.v.origin, spawn1 ) ) {
					p->s.v.classname = "info_player_team1";
					break;
				}

			for( p = world; (p = find( p, FOFCLSN, "info_player_team1" )); )
				if ( VectorCompare( p->s.v.origin, spawn2 ) ) { 
					p->s.v.classname = "info_player_team2";
					break;
				}

			for( p = world; (p = find( p, FOFCLSN, "info_player_team1" )); )
				if ( VectorCompare( p->s.v.origin, spawn3 ) ) { 
					ent_remove( p );
					break;
				}
		}
	}

	if ( cvar( "k_spm_show" ) )
		ShowSpawnPoints();

	if ( isRACE() )
	{
		r_route();
	}
}

// create cvar via 'set' command
// FIXME: unfortunately with current API I can't check if cvar already exist
qbool RegisterCvarEx ( const char *var, const char *defaultstr )
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
	localcmd("set \"%s\" \"%s\"\n", var, defaultstr);
	trap_executecmd ();
	return true;
}

// like RegisterCvarEx, but uses "" for default value
qbool RegisterCvar ( const char *var )
{
	return RegisterCvarEx(var, "");
}
// in the first frame - even world is not spawned yet
void FirstFrame	( )
{
	int 		i, um_idx;
	qbool	matchless_was_forced = false;

	if ( framecount != 1 )
		return;

// clear buffer
	trap_executecmd ();

// register mod cvars

	RegisterCvarEx("maxfps", "77"); // well, got tired from serverinfo, let it be cvar (mvdsv have it now too so it should just set it)

#ifdef HITBOXCHECK
	RegisterCvarEx("k_hitboxcheck_bullets", "32"); // DEBUG: help me test hitbox of the player with modified shot gun
#endif

	RegisterCvar("_k_last_xonx"); // internal usage, save last XonX command
	RegisterCvar("_k_lastmap");	  // internal usage, name of last map
	RegisterCvar("_k_last_cycle_map");  // internal usage, name of last map in map cycle,
										// so we can back to map cycle if someone voted for map not in map cycle
	RegisterCvar("_k_worldspawns"); // internal usage, count of maps server spawned
	RegisterCvar("_k_pow_last");  // internal usage, k_pow from last map

	RegisterCvar("_k_nospecs");  // internal usage, will reject spectators connection
	
	RegisterCvar("_k_recordeddemoname");  // internal usage, name of last easyrecorded demo

	RegisterCvar("k_noitems");

	RegisterCvar("k_random_maplist"); // select random map from k_ml_XXX variables.

	RegisterCvar("k_mode");
	RegisterCvar("k_defmode");
	RegisterCvar("k_auto_xonx"); // switch XonX mode dependant on players + specs count
	RegisterCvar("k_matchless");
	RegisterCvar("k_matchless_countdown");
	RegisterCvar("k_use_matchless_dir"); // use configs/usermodes/matchless instead of configs/usermodes/ffa in matchless mode
	RegisterCvar("k_disallow_kfjump");
	RegisterCvar("k_disallow_krjump");
	RegisterCvar("k_lock_hdp");
	RegisterCvar("k_disallow_weapons");

	RegisterCvar("k_pow");
	RegisterCvarEx("k_pow_q", "1"); // quad
	RegisterCvarEx("k_pow_p", "1"); // pent
	RegisterCvarEx("k_pow_r", "1"); // ring
	RegisterCvarEx("k_pow_s", "1"); // suit
	RegisterCvar("k_pow_min_players");
	RegisterCvar("k_pow_check_time");
	RegisterCvarEx("k_pow_pickup", "0");	// allow multiple pickup of same powerup - off by default

	RegisterCvar("allow_spec_wizard");
	RegisterCvar("k_no_wizard_animation"); // disallow wizard animation

	RegisterCvar("k_vp_break");   // votes percentage for stopping the match voting
	RegisterCvar("k_vp_admin");   // votes percentage for admin election
	RegisterCvar("k_vp_captain"); // votes percentage for captain election
	RegisterCvar("k_vp_map");     // votes percentage for map change voting
	RegisterCvar("k_vp_pickup");  // votes percentage for pickup voting
	RegisterCvar("k_vp_rpickup"); // votes percentage for rpickup voting
	RegisterCvar("k_vp_nospecs"); // votes percentage for nospecs voting
	RegisterCvar("k_vp_coop");    // votes percentage for coop voting
	RegisterCvar("k_vp_antilag"); // votes percentage for antilag voting
	RegisterCvar("k_no_vote_map"); // dis allow map voting in matcless mode, also disallow /next_map

	RegisterCvar("k_end_tele_spawn"); // don't remove end tele spawn

	RegisterCvar("k_motd_time"); 	  // motd time in seconds

	RegisterCvar("k_admincode");
	RegisterCvarEx("k_prewar", "1");
	RegisterCvar("k_lockmap");
	RegisterCvar("k_fallbunny");
	RegisterCvar("timing_players_time");
	RegisterCvar("timing_players_action");
	RegisterCvar("allow_timing");
	RegisterCvarEx("demo_scoreslength", "10");
	RegisterCvar("lock_practice");
	RegisterCvar("k_defmap");
	RegisterCvar("k_admins");
	RegisterCvar("k_overtime");
	RegisterCvar("k_exttime");
	RegisterCvar("k_spw");
	RegisterCvar("k_lockmin");
	RegisterCvar("k_lockmax");
	RegisterCvar("k_spectalk");
	RegisterCvarEx("k_keepspectalkindemos", "0");
	RegisterCvar("k_sayteam_to_spec");
	RegisterCvar("k_dis");
	RegisterCvar("dq");
	RegisterCvar("dr");
	RegisterCvar("dp");
	RegisterCvar("k_frp");
	RegisterCvar("k_highspeed");
	RegisterCvar("k_freeze");
	RegisterCvar("k_free_mode");
	RegisterCvar("k_allowed_free_modes");
	RegisterCvarEx("k_allow_vwep", "0");
	RegisterCvarEx("k_vwep", "1");
	RegisterCvar("allow_toggle_practice");
	RegisterCvar("k_remove_end_hurt");
	RegisterCvar("k_allowvoteadmin");
//	RegisterCvar("k_maxrate"); -> now using sv_maxrate instead
	RegisterCvar("k_minrate");
	RegisterCvar("k_sready");
	RegisterCvarEx("k_spm_show", "1");
	RegisterCvarEx("k_spm_glow", "0");
	RegisterCvarEx("k_spm_custom_model", "0");
// { race
	RegisterCvarEx("k_race", "0");
	RegisterCvarEx("k_race_custom_models", "0");
	RegisterCvarEx("k_race_autorecord", "0");
	RegisterCvarEx("k_race_times_per_port", "0");
	//RegisterCvarEx("k_race_topscores", "10");
// }
	RegisterCvar("k_idletime");
	RegisterCvar("k_timetop");
	RegisterCvar("k_membercount");
	RegisterCvarEx("demo_tmp_record", "0");
	RegisterCvar("demo_skip_ktffa_record");
	RegisterCvar("k_demoname_date"); // add date to demo name, value is argument for strftime() function
	RegisterCvarEx("k_count", "10");
	RegisterCvar("k_exclusive"); // stores whether players can join when a game is already in progress
	RegisterCvar("k_lockmode");
	RegisterCvar("k_short_gib");
	RegisterCvar("k_ann");
	RegisterCvar("srv_practice_mode");
	RegisterCvar("add_q_aerowalk");
	RegisterCvar("k_noframechecks");
	RegisterCvar("dmm4_invinc_time");
	RegisterCvarEx("k_classic_shotgun", "1");

	RegisterCvar("k_no_fps_physics");
//{ ctf
	RegisterCvar("k_ctf_custom_models");
	RegisterCvar("k_ctf_hook");
	RegisterCvar("k_ctf_runes");
	RegisterCvar("k_ctf_ga");
	RegisterCvar("k_ctf_based_spawn"); // spawn players on the base (red/blue)
//}
	RegisterCvar("k_spec_info");
	RegisterCvar("k_midair");
	
	RegisterCvarEx("k_killquad", "0");

	RegisterCvarEx("k_bloodfest", "0");

	RegisterCvarEx("k_nightmare_pu", "0");
	RegisterCvarEx("k_nightmare_pu_droprate", "0.15");
	RegisterCvarEx("k_instagib", "0");
	RegisterCvarEx("k_instagib_custom_models", "0");
	RegisterCvarEx("k_cg_kb", "1");
	
	RegisterCvar("k_rocketarena"); // rocket arena
	RegisterCvar("k_dmgfrags");
	RegisterCvar("k_tp_tele_death");
// { Clan Arena
	RegisterCvarEx("k_clan_arena", "0");
	RegisterCvarEx("k_clan_arena_rounds", "9");
// }
// { upplayers/upspecs
	RegisterCvar("k_allowcountchange");
	RegisterCvar("k_maxclients");
	RegisterCvar("k_maxspectators");
// }
	RegisterCvar("k_ip_list");

// { cmd flood protection
	RegisterCvar("k_cmd_fp_count");
	RegisterCvar("k_cmd_fp_per");
	RegisterCvar("k_cmd_fp_for");
	RegisterCvar("k_cmd_fp_kick");
	RegisterCvar("k_cmd_fp_dontkick");
	RegisterCvar("k_cmd_fp_disabled");
// }

	RegisterCvarEx("k_extralog_xsd_uri", "http://qw-dev.net/attachments/download/177/ktxlog_0.1.xsd");
	RegisterCvar("k_extralog");
	RegisterCvar("k_demo_mintime");
	RegisterCvar("k_dmm4_gren_mode");
	RegisterCvarEx("k_fp", "1"); // say floodprot for players
	RegisterCvarEx("k_fp_spec", "3"); // say floodprot for spectators

// { yawnmode implementation by Molgrum
	RegisterCvar("k_yawnmode");
	RegisterCvar("k_teleport_cap");
// }

	RegisterCvar("k_teamoverlay"); // q3 like team overlay

// { SP
	RegisterCvarEx("k_monster_spawn_time", "20");
// }

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

// below globals changed only here

	k_matchLess = cvar( "k_matchless" );
	if ( !cvar( "deathmatch" ) || cvar( "coop" ) )
	{
		k_matchLess = 1; // treat coop or singleplayer as matchLess
		matchless_was_forced = true;
	}

	k_allowed_free_modes = cvar( "k_allowed_free_modes" ); // must be setup before UserMode(...) call
	if ( k_matchLess )
		k_allowed_free_modes |= UM_FFA;
	// do not precache models if CTF is not really allowed
	k_ctf_custom_models = cvar( "k_ctf_custom_models" ) && (k_allowed_free_modes & UM_CTF);

// use k_defmode or reuse last mode from _k_last_xonx
	cvar_fset("_k_worldspawns", (int)cvar("_k_worldspawns") + 1);

	if ( cvar("_k_worldspawns") == 1 ) { // server spawn first map
		sv_minping = cvar("sv_minping"); // remember, so we can broadcast changes

		if ( ( um_idx = um_idx_byname( cvar_string("k_defmode") ) ) >= 0 )
			cvar_fset("_k_last_xonx", um_idx + 1); // force exec configs for default user mode
	}

	// since we remove k_srvcfgmap, we need configure different maps in matchless mode.
	// doing this by execiting configs like we do for "ffa" command in _non_ matchless mode
	if ( k_matchLess ) {
		if ( ( um_idx = um_idx_byname("ffa") ) >= 0 ) {
			cvar_fset("_k_last_xonx", um_idx + 1); // force server call "ffa" user mode
		}
		else {
			G_bprint(2, "FirstFrame: um_idx_byname fail\n"); // shout
			cvar_fset("_k_last_xonx", 0);
		}
	}

	if ( cvar("_k_last_xonx") > 0 && strneq( cvar_string("_k_lastmap"), g_globalvars.mapname ) )
		UserMode( -cvar("_k_last_xonx") ); // auto call XonX command if map switched to another

// fix game rules, if cfgs some how misconfigured
#ifdef CTF_RELOADMAP
	k_ctf = (cvar( "k_mode" ) == gtCTF); // emulate CTF is active so FixRules is silent
#endif

	if ( matchless_was_forced )
		trap_cvar_set_float("deathmatch", (deathmatch = 0));

	FixRules();

#ifdef CTF_RELOADMAP
	k_ctf = (k_mode == gtCTF); // finaly decide is ctf active or not
	k_ctf_custom_models = k_ctf_custom_models && isCTF(); // precache only if CTF is really on
#endif
}

// items spawned, but probably not solid yet
void SecondFrame ( )
{
	if ( framecount != 2 )
		return;

	Customize_Maps();
}

void CheckSvUnlock ()
{
	if ( k_sv_locktime && k_sv_locktime < g_globalvars.time ) {
		G_bprint(2, "%s\n", redtext("server unlocked"));
		k_sv_locktime = 0;
	}
}

// switch XonX mode dependant on players + specs count
void CheckAutoXonX(qbool use_time)
{
	static int old_count = -666; // static
	static float last_check_time = 0;

	gedict_t *p;
	int count, um_idx = -1;

	if ( !cvar("k_auto_xonx") || match_in_progress || k_matchLess 
		 || (use_time && g_globalvars.time - last_check_time < 7) /* allow users reconnect */ 
	   )
		return;

	last_check_time = g_globalvars.time;

	for( count = 0, p = world; (p = find_client( p )); )
		if ( p->ct == ctPlayer || (p->ct == ctSpec && p->ready) )
			count++;

	if ( count == old_count )
		return;

	switch( count ) {
		case 0: case 1: 
		case 2: case 3: um_idx = um_idx_byname( "1on1" );   break;
		case 4: case 5:	um_idx = um_idx_byname( "2on2" );   break;
		case 6: case 7:	um_idx = um_idx_byname( "3on3" );   break;
		case 8: case 9:	um_idx = um_idx_byname( "4on4" );   break;
		default:		um_idx = um_idx_byname( "10on10" ); break;
	}

	if ( um_idx >= 0 && cvar("_k_last_xonx") - 1 != um_idx ) {
		G_bprint(2, "Server decides to switch user mode\n");
		UserMode( -(um_idx + 1) );
	}

	old_count = count;
}

// called when switching to/from ctf mode.
void FixCTFItems()
{
	static gameType_t old_k_mode = 0;	// static
	static int k_ctf_runes = 0;			// static
	static int k_ctf_hook = 0;			// static

	if ( framecount == 1 ) { // just init vars at first frame, after this we can determine if such vars changed
		old_k_mode = k_mode;
		k_ctf_runes = cvar("k_ctf_runes");
		k_ctf_hook = cvar("k_ctf_hook");

		return;
	}

#ifdef CTF_RELOADMAP
	if (old_k_mode != k_mode && (old_k_mode == gtCTF || k_mode == gtCTF))
		changelevel( g_globalvars.mapname );
#endif

	if ( match_in_progress )
		return; // some optimization, ok ?

	if ( old_k_mode != k_mode )
		RegenFlags( isCTF() );

	if ( old_k_mode != k_mode || k_ctf_runes != cvar("k_ctf_runes") || framecount == 2 )
		SpawnRunes( isCTF() && cvar("k_ctf_runes") );

	if ( old_k_mode != k_mode || k_ctf_hook != cvar("k_ctf_hook") )
		AddHook( isCTF() && cvar("k_ctf_hook") );

	old_k_mode = k_mode;
	k_ctf_runes = cvar("k_ctf_runes");
	k_ctf_hook = cvar("k_ctf_hook");
}

void FixRA()
{
	static qbool old_k_rocketarena = false;	// static

	if ( framecount == 1 )
		return; // can't guess here something yet

	if ( framecount == 2 )
	{
		old_k_rocketarena = isRA(); // ok, save RA status after world spawn, and start check status changes on 3-t frame
		return;
	}

	// do that even match in progress...
	if ( old_k_rocketarena != isRA() )
	{
		old_k_rocketarena = isRA();
		G_bprint(2, "%s: RA settings changed, map will be reloaded\n", redtext("WARNING"));
		changelevel( g_globalvars.mapname );
	}
}

void FixRace()
{
	static qbool old_k_race = false;	// static

	if ( framecount == 1 )
		return; // can't guess here something yet

	if ( framecount == 2 )
	{
		old_k_race = isRACE(); 
		return;
	}

	// do that even match in progress...
	if ( old_k_race != isRACE() )
	{
		old_k_race = isRACE();
		G_bprint(2, "%s: Race settings changed, map will be reloaded\n", redtext("WARNING"));
		changelevel( g_globalvars.mapname );
	}
}

// serve k_pow and k_pow_min_players
void FixPowerups ()
{
	static int	k_pow = -1; // static
	static int	k_pow_q = -1; // static
	static int	k_pow_p = -1; // static
	static int	k_pow_r = -1; // static
	static int	k_pow_s = -1; // static

	qbool		changed   = false;
	int 		k_pow_new = Get_Powerups();
	int			k_pow_q_new = cvar("k_pow_q");
	int			k_pow_p_new = cvar("k_pow_p");
	int			k_pow_r_new = cvar("k_pow_r");
	int			k_pow_s_new = cvar("k_pow_s");

	if (   k_pow != k_pow_new
		|| k_pow_q != k_pow_q_new
		|| k_pow_r != k_pow_r_new
		|| k_pow_p != k_pow_p_new
		|| k_pow_s != k_pow_s_new
		|| framecount == 1 // force on first frame
	)
	{
		changed = true;
		k_pow = k_pow_new;
		k_pow_q = k_pow_q_new;
		k_pow_r = k_pow_r_new;
		k_pow_p = k_pow_p_new;
		k_pow_s = k_pow_s_new;
	}

	if ( changed )
	{
		extern void hide_powerups ( char *classname );
		extern void show_powerups ( char *classname );

		if ( k_pow && k_pow_p )
			show_powerups( "item_artifact_invulnerability" );
		else
			hide_powerups( "item_artifact_invulnerability" );

		if ( k_pow && k_pow_q )
			show_powerups( "item_artifact_super_damage" );
		else
			hide_powerups( "item_artifact_super_damage" );

		if ( k_pow && k_pow_s )
			show_powerups( "item_artifact_envirosuit" );
		else
			hide_powerups( "item_artifact_envirosuit" );

		if ( k_pow && k_pow_r )
			show_powerups( "item_artifact_invisibility" );
		else
			hide_powerups( "item_artifact_invisibility" );
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

int skip_fixrules = 0;

// check if server is misconfigured somehow, made some minimum fixage
void FixRules ( )
{
	extern void FixYawnMode();

	gameType_t km = k_mode = cvar( "k_mode" );
	int k_tt = bound( 0, cvar( "k_timetop" ), 600 );
	int	tp   = teamplay  = cvar( "teamplay" );
	int tl   = timelimit = cvar( "timelimit" );
	int fl   = fraglimit = cvar( "fraglimit" );
	int dm   = deathmatch = cvar( "deathmatch" );
	int k_minr = bound(0, cvar( "k_minrate" ),  100000);
	int k_maxr = bound(0, cvar( "sv_maxrate" ), 100000);

	k_bloodfest = cvar( "k_bloodfest" );

	k_killquad = cvar( "k_killquad" );

	skill = cvar( "skill" );

	coop = cvar( "coop" );

	FixYawnMode(); // yawn mode

    k_maxspeed = cvar( "sv_maxspeed" );

	FixCmdFloodProtect(); // cmd flood protect

	FixSayFloodProtect(); // say flood protect

	FixSayTeamToSpecs(); // k_sayteam_to_spec

	current_maxfps = cvar( "maxfps" );
	if ( current_maxfps != bound(50, current_maxfps, 1981) )
	{
//		current_maxfps = 72;	// 2.30 standard
		current_maxfps = 77;	// year 2007 standard
		cvar_fset( "maxfps", current_maxfps );
	}

	if ( skip_fixrules > 0 ) {
		skip_fixrules--;
		return;
	}

	// turn CTF off if CTF usermode is not allowed, due to precache_sound or precache_model
	if ( isCTF() && !( k_allowed_free_modes & UM_CTF ) )
		cvar_fset("k_mode", (float)( k_mode = gtTeam ));

	if ( coop )
	{
		// if we are in coop, then deathmatch should be 0
		if ( deathmatch )
			trap_cvar_set_float("deathmatch", (deathmatch = 0));
		// set some teamplay in coop mode.
		if ( !teamplay )
			trap_cvar_set_float("teamplay", (teamplay = 2));
	}
	else
	{
// qqshka: interesting, why I commented it out, since I do not recall case then we can have zero deathmatch
//		  in non coop game.
//		if ( !deathmatch )
//			trap_cvar_set_float("deathmatch", (deathmatch = 3));
	}

	// if unknown teamplay - disable it at all
	if ( teamplay != 0 && teamplay != 1 && teamplay != 2 && teamplay != 3 && teamplay != 4 )
		trap_cvar_set_float("teamplay", (teamplay = 0));

	// if unknown deathmatch - set some default value
	if ( deathmatch != 0 && deathmatch != 1 && deathmatch != 2 && deathmatch != 3 && deathmatch != 4 && deathmatch != 5 )
		trap_cvar_set_float("deathmatch", (deathmatch = 3));

	if ( k_matchLess ) {
		// matchless mode MUST be FFA
		if ( !isFFA() )
			trap_cvar_set_float("k_mode", (float)( k_mode = gtFFA ));
		// matchless mode should have teamplay set to 0 unless coop.
		if ( teamplay && !coop )
			trap_cvar_set_float("teamplay", (teamplay = 0));
	}

	// if unknown k_mode - set some appropriate value
	if ( isUnknown() )
		trap_cvar_set_float("k_mode", (float)( k_mode = (teamplay ? gtTeam : gtDuel) ));

	// teamplay set, but gametype is not team, disable teamplay in this case
	if ( teamplay ) {
		if ( !isTeam() && !isCTF() && !coop )
			trap_cvar_set_float("teamplay", (teamplay = 0));
	}
	
	// gametype is team, but teamplay has wrong value, set some default value
	// qqshka - CTF need some teamplay too?
	if ( isTeam() || isCTF() ) {
		if ( teamplay != 1 && teamplay != 2 && teamplay != 3 && teamplay != 4 )
			trap_cvar_set_float("teamplay", (teamplay = 2));
	}

	if ( k_tt <= 0 ) { // this change does't broadcasted
		cvar_fset( "k_timetop", k_tt = 30 ); // sensible default if no max set
	}

// oldman --> don't allow unlimited timelimit + fraglimit
// also do not allow some weird timelimit
	if ( deathmatch )
	{
    	if( (timelimit == 0 && fraglimit == 0) || timelimit > k_tt || timelimit < 0 )
    	{
        	cvar_fset( "timelimit", timelimit = k_tt ); // sensible default if no max set
    	}
	}
	else
	{
		if ( timelimit )
        	cvar_fset( "timelimit", timelimit = 0 );

		if ( fraglimit )
        	cvar_fset( "fraglimit", fraglimit = 0 );
	}
// <-- oldman

// {  rate bounds
	if ( !k_minr )
		k_minr = 500; // was wrong/zero setting
	if ( !k_maxr )
		k_maxr = 30000; // was wrong/zero setting
	if ( k_minr > k_maxr )
		k_minr = k_maxr; // hehe
	if ( k_minr != cvar( "k_minrate" ) )
		cvar_fset( "k_minrate", k_minr );
	if ( k_maxr != cvar( "sv_maxrate" ) )
		cvar_fset( "sv_maxrate", k_maxr );
// }

	if ( deathmatch )
		g_globalvars.serverflags = (int)g_globalvars.serverflags & ~15; // remove runes

	if ( cvar("k_midair") && deathmatch != 4 )
		cvar_fset( "k_midair", 0 ); // midair only in dmm4

	if ( cvar("k_instagib") && deathmatch != 4 )
		cvar_fset( "k_instagib", 0 ); // instagib only in dmm4

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

	if (sv_minping != (int)cvar("sv_minping")) {
		sv_minping = cvar("sv_minping"); // remember, so we can broadcast changes
		G_bprint(2, "%s changed to %d\n", redtext("sv_minping"), sv_minping);
	}

	if ( framecount == 1 )
		trap_executecmd ();
}


int         timelimit, fraglimit, teamplay, deathmatch, framecount, coop, skill;

extern float intermission_exittime;

void CheckTiming();
void check_fcheck();
void CheckTeamStatus();
void DoMVDAutoTrack( void );

void FixNoSpecs( void );

void StartFrame( int time )
{
	framecount++;

	if ( framecount == 1 )
		FirstFrame();

	FixRules();

	if ( framecount == 2 ) {
		SecondFrame();
		FixRules();
	}

	FixNoSpecs(); // if no players left turn off "no spectators" mode

	FixCTFItems(); // if modes have changed we may need to add/remove flags etc

	FixRA(); // we may need reload map
	
	FixRace(); // we may need reload map

	FixPowerups();

	FixSpecWizards();

	framechecks = bound( 0, !cvar( "k_noframechecks" ), 1 );

	CheckSvUnlock();

	DoMVDAutoTrack(); // mvd autotrack stuff

	CheckTiming(); // check if client lagged or returned from lag

	if ( intermission_running && g_globalvars.time >= intermission_exittime - 1 
			&& !strnull( cvar_string( "serverdemo" ) ) )
		localcmd("stop\n"); // demo is recording, stop it and save

	if ( k_matchLess && !match_in_progress && !k_bloodfest )
		StartTimer(); // trying start countdown in matchless mode

	if ( isRA() )
		ra_Frame();

	if ( isCA() )
		CA_Frame();

	if ( framecount > 10 )
		vote_check_all();

	CheckAll(); // just check some clients params

	if ( isRACE() )
		race_think();

	check_monsters_respawn();

	CheckTeamStatus();

	CheckAutoXonX(true); // switch XonX mode dependant on players + specs count

	Check_LongMapUptime(); // reload map after some long up time, so our float time variables are happy

	check_fcheck();
}

