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
 *  $Id: world.c,v 1.15 2006/01/05 23:01:57 qqshka Exp $
 */

#include "g_local.h"

float CountALLPlayers ();
void  StartMatchLess ();

void  SUB_regen();

void  FixSpecWizards ();

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
			changelevel( s1 );
	}

	ent_remove( self );
}


void  SP_item_artifact_super_damage();

void SP_worldspawn()
{
	char 		*lastmap;
	char		*s;
	char 		*tmp;
   	gedict_t	*e;


	// exec configs/maps/default.cfg
	// exec configs/maps/mapname.cfg
	if (    cvar( "k_srvcfgmap" )
		 && (    strnull( lastmap = cvar_string("_k_lastmap") ) // server just spawn first time ?
			  || strneq( lastmap, g_globalvars.mapname )
			)
	   ) {
		localcmd("exec configs/maps/default.cfg\n");
		localcmd("exec configs/maps/%s.cfg\n", g_globalvars.mapname);
		trap_executecmd ();
	}
    
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

	trap_precache_model( "progs/wizard.mdl" );

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
	localcmd("localinfo sv_spectalk 1\n");

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

	// spawn quad if map is aerowalk in this case
	if ( iKey(world, "add_q_aerowalk") && streq( "aerowalk", g_globalvars.mapname) ) {
   		gedict_t	*swp = self;

		self = spawn();
		setorigin( self, -912.5f, -898.875f, 248.0f ); // oh, ktpro like
		self->s.v.owner = EDICT_TO_PROG( world );
		SP_item_artifact_super_damage();

		self = swp; // restore self
	}

	if ( !k_matchLess ) // skip practice in matchLess mode
	if ( iKey( world, "srv_practice_mode" ) ) // #practice mode#
		SetPractice( iKey( world, "srv_practice_mode" ), NULL ); // may not reload map
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
	localcmd("set \"%s\" 0\n", var);
	trap_executecmd ();
	return true;
}

// in the first frame - even world is not spawned yet
void FirstFrame	( )
{
	if ( framecount != 1 )
		return;

	trap_executecmd ();

	RegisterCvar("k_mode");
	RegisterCvar("k_matchless");
	RegisterCvar("k_matchless_countdown");
	RegisterCvar("k_disallow_kfjump");
	RegisterCvar("k_disallow_krjump");
	RegisterCvar("k_lock_hdp");
	RegisterCvar("k_disallow_weapons");
	RegisterCvar("_k_lastmap");	// internal usage, name of last map
	RegisterCvar("k_srvcfgmap");
	RegisterCvar("k_pow_min_players");
	RegisterCvar("k_pow_check_time");
	RegisterCvar("_k_players"); // internal usage, count of players on last map
	RegisterCvar("_k_pow_last"); // internal usage, k_pow from last map
	RegisterCvar("allow_spec_wizard");
	RegisterCvar("k_no_wizard_animation"); // disallow wizard animation



	k_matchLess = cvar( "k_matchless" ); // changed only here
}

// items spawned, but probably not solid yet
void SecondFrame ( )
{
	if ( framecount != 2 )
		return;

	if ( k_matchLess )
		StartMatchLess ();
}

void hide_powerups ( char *classname )
{
	gedict_t *p;

	if ( strnull( classname ) )
		G_Error("hide_items");

	for( p = world; p = find(p, FOFCLSN, classname); ) {
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
	gedict_t *p, *swp;

	if ( strnull( classname ) )
		G_Error("show_items");

	swp = self; 

	for( p = world; p = find(p, FOFCLSN, classname); ) {
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

// check if server is misconfigured somehow, made some minimum fixage
void FixRules ( )
{
	gameType_t km = k_mode;
	int k_tt = bound( 0, iKey( world, "k_timetop"), 600 );
	int	tp   = teamplay;
	int tl   = timelimit;
	int fl   = fraglimit;
	int dm   = deathmatch;
	int k_minr = bound(0, iKey(world, "k_minrate"), 20000);
	int k_maxr = bound(0, iKey(world, "k_maxrate"), 20000);	


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
		if ( !isTeam() )
			trap_cvar_set_float("teamplay", (teamplay = 0));
	}
	
	// gametype is team, but teamplay has wrong value, set some default value
	if ( isTeam() ) {
		if ( teamplay != 1 && teamplay != 2 && teamplay != 3 )
			trap_cvar_set_float("teamplay", (teamplay = 2));
	}

	if ( k_tt <= 0 ) { // this change does't broadcasted
		localcmd ("localinfo k_timetop %d\n", (k_tt = 30));// sensible default if no max set
	}

// oldman --> don't allow unlimited timelimit + fraglimit
    if( timelimit == 0 && fraglimit == 0 ) {
        trap_cvar_set_float("timelimit", (float)(timelimit = k_tt));// sensible default if no max set
    }
// <-- oldman

	if ( !k_minr ) {
		localcmd ("localinfo k_minrate %d\n", (k_minr = 500));
	}
	if ( !k_maxr ) {
		localcmd ("localinfo k_maxrate %d\n", (k_maxr = 10000));
	}
	if ( k_minr > k_maxr ) {
		localcmd ("localinfo k_minrate %d\n", (k_minr = k_maxr));
	}

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

float		rj;

extern float intermission_exittime;

void ModPause (int pause);
void CheckTiming();

void StartFrame( int time )
{
	framecount++;

	if ( framecount == 1 )
		FirstFrame();

	if ( framecount == 2 )
		SecondFrame ( );

	rj = max( 0, fKey( world, "rj" ) ); 	// Set Rocket Jump Modifiers

    k_maxspeed = cvar( "sv_maxspeed" );
	timelimit  = cvar( "timelimit" );
	fraglimit  = cvar( "fraglimit" );
	teamplay   = cvar( "teamplay" );
	deathmatch = cvar( "deathmatch" );

	k_mode	   = cvar( "k_mode" );


	FixRules ();

	FixPowerups ();

	FixSpecWizards ();

	framechecks = bound( 0, !iKey( world, "k_noframechecks" ), 1 );

// Tonik: note current "serverinfo maxfps" setting
// (we don't want to do it in every player frame)
	current_maxfps = iKey( world, "maxfps" );
	if ( !current_maxfps )
		current_maxfps = 72;	// 2.30 standard

	current_maxfps = bound(50, current_maxfps, 1981);

	CheckTiming(); // check if client lagged or returned from lag

	if ( !CountALLPlayers() && k_pause ) {
		G_bprint(2, "No players left, unpausing.\n");
		ModPause ( 0 );
	}

	if ( intermission_running && g_globalvars.time >= intermission_exittime - 1 
			&& !strnull( cvar_string( "serverdemo" ) ) )
		localcmd("stop\n"); // demo is recording, stop it and save
}

