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
 *  $Id: client.c,v 1.49 2006/04/07 21:38:47 qqshka Exp $
 */

//===========================================================================
// Client
// 
//===========================================================================
#include "g_local.h"
vec3_t          VEC_ORIGIN = { 0, 0, 0 };
vec3_t          VEC_HULL_MIN = { -16, -16, -24 };
vec3_t          VEC_HULL_MAX = { 16, 16, 32 };

vec3_t          VEC_HULL2_MIN = { -32, -32, -24 };
vec3_t          VEC_HULL2_MAX = { 32, 32, 64 };
int             modelindex_eyes, modelindex_player;


float CountALLPlayers();
void IdlebotCheck();
void CheckAll();
void PlayerStats();
void ExitCaptain();
void CheckFinishCaptain();
void MakeMOTD();
void ExitKick(gedict_t *kicker);
void play_teleport( gedict_t *sndspot );
void ImpulseCommands();
void StartDie ();
void ZeroFpsStats ();

void del_from_specs_favourites(gedict_t *rm);

void CheckAll ()
{
	int from = 0;
	gedict_t *p;

	for ( p = world; p = find_plrspc(p, &from); )
		CheckRate( p, "" );
}

qboolean CheckRate (gedict_t *p, char *newrate)
{
	qboolean ret = false;
    float player_rate, maxrate=0, minrate=0;

	// This is used to check a players rate.  If above allowed setting then it kicks em off.
	player_rate = strnull(newrate) ? iKey( p, "rate" ) : atoi(newrate);
	
	maxrate = cvar( "k_maxrate" );
	minrate = cvar( "k_minrate" );

	if( maxrate || minrate )
	{
	    if ( player_rate > maxrate )
		{
			G_sprint(p, 2, "\nYour עבפו setting is too high for this server.\n"
						   "Rate set to %d\n", (int)maxrate);
			stuffcmd(p, "rate %d\n", (int)maxrate );
			ret = true;
		}

		if ( player_rate < minrate )
		{
			G_sprint(p, 2, "\nYour עבפו setting is too low for this server.\n"
						   "Rate set to %d\n", (int)minrate);
			stuffcmd(p, "rate %d\n", (int)minrate );
			ret = true;
		}
	}

	return ret;
}


// timing action
#define TA_INFO			(1<<0)
#define TA_GLOW			(1<<1)
#define TA_INVINCIBLE	(1<<2)
#define TA_KPAUSE		(1<<3)
#define TA_ALL			( TA_INFO | TA_GLOW | TA_INVINCIBLE | TA_KPAUSE )

int lasttimed = 0;

// check if client lagged or returned from lag
void CheckTiming()
{
	float timing_players_time = bound(0, cvar( "timing_players_time" ), 30);
	int timing_players_action = TA_ALL & (int)cvar( "timing_players_action" );
	int timed = 0;
	gedict_t *p;

	if ( !cvar( "allow_timing" ) )
		return;

	timing_players_time = timing_players_time ? timing_players_time : 6; // 6 is default

	p = find( world, FOFCLSN, "player" );

	while( p ) {
		if( p->k_lastPostThink + timing_players_time < g_globalvars.time ) {
			int firstTime; // guess is we are already know player is lagged
			firstTime = !p->k_timingWarnTime;
            timed++;

			// warn and repeat warn after some time
			if ( firstTime || p->k_timingWarnTime + 20 < g_globalvars.time ){
			    if ( timing_players_action & TA_INFO )
					G_bprint(2, "\x87%s %s is timing!\n", redtext( "WARNING:" ), p->s.v.netname);

				p->k_timingWarnTime = g_globalvars.time;
			}

			// ok we are detect - player lagged, so do something, effects is exception
			if ( firstTime ) {
				if ( timing_players_action & TA_INVINCIBLE ) {
					p->k_timingTakedmg = p->s.v.takedamage;
					p->k_timingSolid   = p->s.v.solid;
					p->k_timingMovetype= p->s.v.movetype;
					p->s.v.takedamage  = 0;
					p->s.v.solid	   = 0;
					p->s.v.movetype	   = 0;
					SetVector( p->s.v.velocity, 0, 0, 0 ); // speed is zeroed and not restored
				}

				if ( timing_players_action & TA_KPAUSE ) {
					if ( !k_pause ) {
						G_bprint(2, "Server issues a pause\n");
						ModPause ( 2 );
					}
				}
			}

		}
		else
			p->k_timingWarnTime = 0;

		// effects stuff
		if ( p->k_timingWarnTime ) {
			if ( timing_players_action & TA_GLOW )
				p->s.v.effects = ( int ) p->s.v.effects | EF_DIMLIGHT;
		}
		else {
			// don't bother if player have quad or pent
			if ( !p->invincible_finished && !p->super_damage_finished )
				p->s.v.effects = ( int ) p->s.v.effects & ~EF_DIMLIGHT;
		}

		p = find( p, FOFCLSN, "player" );
	}

	if ( lasttimed && !timed && k_pause == 2 )
		G_bprint(2, "You can vote now for %s\n", redtext( "unpause" ) );

	lasttimed = timed;
}


void Check_sready()
{
	int k_sready = cvar( "k_sready" );
	gedict_t *p;

	if ( match_in_progress )
		return;

	for( p = world; p = find( p, FOFCLSN, "player" ); ) {
		// player have quad - so EF_BLUE will be set or removed anyway, but ugly blinking, so work around
		if ( p->super_damage_finished )
			continue;

		if ( k_sready && !p->ready )
			p->s.v.effects = ( int ) p->s.v.effects | EF_BLUE;
		else
			p->s.v.effects = ( int ) p->s.v.effects & ~EF_BLUE;
	}
}

/*
=============================================================================

    LEVEL CHANGING / INTERMISSION

=============================================================================
*/
float           intermission_running;
float           intermission_exittime;
char            nextmap[64] = "";

/*QUAKED info_intermission (1 0.5 0.5) (-16 -16 -16) (16 16 16)
This is the camera point for the intermission.
Use mangle instead of angle, so you can set pitch or roll as well as yaw.  'pitch roll yaw'
*/
void SP_info_intermission()
{
	// so C can get at it
	VectorCopy( self->mangle, self->s.v.angles );	//self.angles = self.mangle;      
}

void InGameParams ()
{
	g_globalvars.parm1 = IT_AXE | IT_SHOTGUN;
	g_globalvars.parm2 = 100;
	g_globalvars.parm3 = 0;
	g_globalvars.parm4 = 25;
	g_globalvars.parm5 = 0;
	g_globalvars.parm6 = 0;
	g_globalvars.parm7 = 0;
	g_globalvars.parm8 = 1;
}

void PrewarParams ()
{
	g_globalvars.parm1 = IT_AXE | IT_SHOTGUN | IT_SUPER_SHOTGUN | IT_NAILGUN | IT_SUPER_NAILGUN
					   | IT_GRENADE_LAUNCHER | IT_ROCKET_LAUNCHER | IT_LIGHTNING;
	g_globalvars.parm2 = 1000;
	g_globalvars.parm3 = 1000;
	g_globalvars.parm4 = 100;
	g_globalvars.parm5 = 200;
	g_globalvars.parm6 = 100;
	g_globalvars.parm7 = 100;
	g_globalvars.parm8 = 32;
}


// generally, this must be called before level will changed

void SetChangeParms()
{
	// ok, server want to change map
	// check, if matchless mode is active, set ingame params,
	// we must use k_matchless cvar here beacuse it can be changed during game somehow (via direct server conlose etc)
	// If matchless mode is not active, set just ordinary prewar stats
	if( /* match_in_progress == 2 ||*/ cvar( "k_matchless" ) )
		InGameParams ();
    else 
		PrewarParams ();

    g_globalvars.parm9  = 0;

	g_globalvars.parm11 = self->k_admin;
	g_globalvars.parm12 = self->k_accepted;
	g_globalvars.parm13 = self->k_stuff;
	g_globalvars.parm14 = self->ps.handicap;

//	G_bprint(2, "SCP: ad:%d\n", (int)self->k_admin);
}

// this called before player connected (or respawned), so he get default params

// WARNING: if from_vmMain == flase, then self is must be valid
void SetNewParms( qboolean from_vmMain )
{
	if( match_in_progress == 2 || k_matchLess ) 
		InGameParams ();
    else 
		PrewarParams ();

	g_globalvars.parm9  = 0;

	g_globalvars.parm11 = from_vmMain ? 0 : self->k_admin;
	g_globalvars.parm12 = from_vmMain ? 0 : self->k_accepted;
	g_globalvars.parm13 = from_vmMain ? 0 : self->k_stuff;
	g_globalvars.parm14 = from_vmMain ? 0 : self->ps.handicap;

//	G_bprint(2, "SNP\n");
}

// called from PutClientInServer

void DecodeLevelParms()
{
	self->s.v.items			= g_globalvars.parm1;
	self->s.v.health		= g_globalvars.parm2;
	self->s.v.armorvalue 	= g_globalvars.parm3;
	self->s.v.ammo_shells 	= g_globalvars.parm4;
	self->s.v.ammo_nails 	= g_globalvars.parm5;
	self->s.v.ammo_rockets 	= g_globalvars.parm6;
	self->s.v.ammo_cells 	= g_globalvars.parm7;
	self->s.v.weapon 		= g_globalvars.parm8;
	self->s.v.armortype 	= g_globalvars.parm9 * 0.01;

//	G_bprint(2, "DLP1 ad:%d ac:%d s:%d\n", (int)self->k_admin, (int)self->k_accepted, (int)self->k_stuff);

	if ( g_globalvars.parm11 )
		self->k_admin = g_globalvars.parm11;

    if( g_globalvars.parm12 )
        self->k_accepted = g_globalvars.parm12;
	
	if ( g_globalvars.parm13 )
    	self->k_stuff = g_globalvars.parm13;

	if ( g_globalvars.parm14 )
    	self->ps.handicap = g_globalvars.parm14;

//	G_bprint(2, "DLP2 ad:%d ac:%d s:%d\n", (int)g_globalvars.parm11, (int)g_globalvars.parm12, (int)g_globalvars.parm13);
}


gedict_t *Do_FindIntermission( char *info_name )
{
	gedict_t       *spot;
	int             i;

	i = find_cnt( FOFS( s.v.classname ), info_name );
	i = ( i ? g_random() * i : -1); // pick a random one

	return (spot = find_idx( i, FOFS( s.v.classname ), info_name ));
}


/*
============
FindIntermission

Returns the entity to view from
============
*/
gedict_t *FindIntermission()
{
	gedict_t       *spot;

	if ( spot = Do_FindIntermission( "info_intermission" ) )
		return spot;

	if ( spot = Do_FindIntermission( "info_player_start" ) )
		return spot;

	if ( spot = Do_FindIntermission( "info_player_deathmatch" ) )
		return spot;

	if ( spot = Do_FindIntermission( "info_player_coop" ) )
		return spot;

	if ( spot = Do_FindIntermission( "info_player_start2" ) )
		return spot;

//	G_Error( "FindIntermission: no spot" );
//	return NULL;
	return world;
}

void GotoNextMap()
{
	char            *newmap = "";
	int i;

	if( get_votes( OV_ELECT ) ) 
        AbortElect();

	if ( trap_cvar( "samelevel" ) /* == 1 */ )	// if samelevel is set, stay on same level
		changelevel( g_globalvars.mapname );
	else
	{
		// configurable map lists, see if the current map exists as a
		// serverinfo/localinfo var
		// qqshka, i modified this, so if someone select map not in map list and match is ended
		// we will select some map from map list, nor just stay on current map.
		// map list have now next syntax:
		// set k_ml_0 dm6
		// set k_ml_1 dm4
		// set k_ml_2 dm2
		// so this mean we have rotation of maps dm6 dm4 dm2 dm6 dm4 dm2 ...

		for ( i = 0; i<999; i++){
			newmap = cvar_string( va("k_ml_%d", i) );

			if ( strnull( newmap ) ) { // end of list
				if ( !i )
					break; // no map list at all
				// current map not in map list, so select first entry in map list as next map
				newmap = cvar_string( "k_ml_0" );
				break;
			}

			if ( streq( g_globalvars.mapname, newmap ) ) { // ok map found in map list, select next map

				newmap = cvar_string( va("k_ml_%d", i + 1) );

				if ( strnull( newmap ) ) { // current entry was last - select first entry
					newmap = cvar_string( "k_ml_0" );
					break;
				}

				break;
			}
		}

		if ( !nextmap[0] ) // so we can reload current map at least
			strcpy( nextmap, g_globalvars.mapname );

		if ( !strnull( newmap ) )
			changelevel( newmap );
		else
			changelevel( nextmap );
	}
}

/*
============
IntermissionThink

When the player presses attack or jump, change to the next level
============
*/
void IntermissionThink()
{
	if ( g_globalvars.time < intermission_exittime )
		return;

	if ( !self->s.v.button0 && !self->s.v.button1 && !self->s.v.button2 )
		return;

	GotoNextMap();
}

/*
============
SendIntermissionToClient

If a client connects during intermission, send svc_intermission
to him personally
============
*/
void SendIntermissionToClient ()
{
	gedict_t *intermission_spot = FindIntermission();

	g_globalvars.msg_entity = EDICT_TO_PROG( self );

// play intermission music
	WriteByte (MSG_ONE, SVC_CDTRACK);
	WriteByte (MSG_ONE, 3);

	WriteByte (MSG_ONE, SVC_INTERMISSION);
	WriteCoord (MSG_ONE, intermission_spot->s.v.origin[0]);
	WriteCoord (MSG_ONE, intermission_spot->s.v.origin[1]);
	WriteCoord (MSG_ONE, intermission_spot->s.v.origin[2]);
	WriteAngle (MSG_ONE, intermission_spot->mangle[0]);
	WriteAngle (MSG_ONE, intermission_spot->mangle[1]);
	WriteAngle (MSG_ONE, intermission_spot->mangle[2]);

	setorigin (self, PASSVEC3( intermission_spot->s.v.origin ) );
}

/*
============
execute_changelevel

The global "nextmap" has been set previously.
Take the players to the intermission spot
============
*/

void execute_changelevel()
{
	gedict_t       *pos;

	intermission_running = 1;

// enforce a wait time before allowing changelevel
	intermission_exittime = g_globalvars.time + 1 +
							max( 1, cvar( "demo_scoreslength" ) );

	pos = FindIntermission();

// play intermission music
	WriteByte( MSG_ALL, SVC_CDTRACK );
	WriteByte( MSG_ALL, 3 );

	WriteByte ( MSG_ALL, SVC_INTERMISSION );
	WriteCoord( MSG_ALL, pos->s.v.origin[0] );
	WriteCoord( MSG_ALL, pos->s.v.origin[1] );
	WriteCoord( MSG_ALL, pos->s.v.origin[2] );
	WriteAngle( MSG_ALL, pos->mangle[0] );
	WriteAngle( MSG_ALL, pos->mangle[1] );
	WriteAngle( MSG_ALL, pos->mangle[2] );

	other = find( world, FOFS( s.v.classname ), "player" );
	while ( other )
	{
		other->s.v.takedamage = DAMAGE_NO;
		other->s.v.solid = SOLID_NOT;
		other->s.v.movetype = MOVETYPE_NONE;
		other->s.v.modelindex = 0;


// KTEAMS: make players invisible
        other->s.v.model = "";
		// take screenshot if requested
        if( iKey( other, "kf" ) & KF_SCREEN )
			stuffcmd(other, "wait; wait; wait; wait; wait; wait; screenshot\n");

		other = find( other, FOFS( s.v.classname ), "player" );
	}
}

void changelevel_touch()
{
	if ( strneq( other->s.v.classname, "player" ) )
		return;

// qqshka: does't change level in any case, just do damage and return

	T_Damage( other, self, self, 50000 );
	return;

/*

// if "noexit" is set, blow up the player trying to leave
//ZOID, 12-13-96, noexit isn't supported in QW.  Overload samelevel
//      if ((cvar("noexit") == 1) || ((cvar("noexit") == 2) && (mapname != "start")))
	if (      ( trap_cvar( "samelevel" ) == 2 )
	     || ( ( trap_cvar( "samelevel" ) == 3 )
		  && ( strneq( g_globalvars.mapname, "start" ) ) ) )
	{
		T_Damage( other, self, self, 50000 );
		return;
	}

	G_bprint( PRINT_HIGH, "%s exited the level\n", other->s.v.netname );

	strcpy( nextmap, self->map );
	
	activator = other;
	SUB_UseTargets();

	self->s.v.touch = ( func_t ) SUB_Null;

// we can't move people right now, because touch functions are called
// in the middle of C movement code, so set a think time to do it
	self->s.v.think = ( func_t ) execute_changelevel;
	self->s.v.nextthink = g_globalvars.time + 0.1;

*/
}

/*QUAKED trigger_changelevel (0.5 0.5 0.5) ? NO_INTERMISSION
When the player touches this, he gets sent to the map listed in the "map" variable.  Unless the NO_INTERMISSION flag is set, the view will go to the info_intermission spot and display stats.
*/
void SP_trigger_changelevel()
{
	if ( !self->map )
		G_Error( "chagnelevel trigger doesn't have map" );

	InitTrigger();
	self->s.v.touch = ( func_t ) changelevel_touch;
}

/*
go to the next level for deathmatch
*/
void NextLevel()
{
	gedict_t       *o;

	if ( nextmap[0] )
		return;		// already done

	strcpy( nextmap, g_globalvars.mapname );

	o = spawn();
	o->map = g_globalvars.mapname;
	o->s.v.classname = "trigger_changelevel";
	o->s.v.think = ( func_t ) execute_changelevel;
	o->s.v.nextthink = g_globalvars.time + 0.1;

/*
	if ( streq( g_globalvars.mapname, "start" ) )
	{
		if ( !trap_cvar( "registered" ) )
		{
			strcpy( g_globalvars.mapname, "e1m1" );

		} else if ( !( ( int ) ( g_globalvars.serverflags ) & 1 ) )
		{
			strcpy( g_globalvars.mapname, "e1m1" );
			g_globalvars.serverflags =
			    ( int ) ( g_globalvars.serverflags ) | 1;

		} else if ( !( ( int ) ( g_globalvars.serverflags ) & 2 ) )
		{
			strcpy( g_globalvars.mapname, "e2m1" );
			g_globalvars.serverflags =
			    ( int ) ( g_globalvars.serverflags ) | 2;

		} else if ( !( ( int ) ( g_globalvars.serverflags ) & 4 ) )
		{
			strcpy( g_globalvars.mapname, "e3m1" );
			g_globalvars.serverflags =
			    ( int ) ( g_globalvars.serverflags ) | 4;

		} else if ( !( ( int ) ( g_globalvars.serverflags ) & 8 ) )
		{
			strcpy( g_globalvars.mapname, "e4m1" );
			g_globalvars.serverflags =
			    ( int ) ( g_globalvars.serverflags ) - 7;
		}

		o = spawn();
		o->map = g_globalvars.mapname;
	} else
	{
		// find a trigger changelevel
		o = find( world, FOFS( s.v.classname ), "trigger_changelevel" );
		if ( !o )
		{		// go back to same map if no trigger_changelevel
			o = spawn();
			o->map = g_globalvars.mapname;
		}
	}

	strcpy( nextmap, o->map );

	if ( o->s.v.nextthink < g_globalvars.time )
	{
		o->s.v.think = ( func_t ) execute_changelevel;
		o->s.v.nextthink = g_globalvars.time + 0.1;
	}
*/
}

/*
=============================================================================

    PLAYER GAME EDGE FUNCTIONS

=============================================================================
*/

void            set_suicide_frame();

// called by ClientKill and DeadThink
void respawn()
{
//	G_bprint(2, "respawn()\n");

	// make a copy of the dead body for appearances sake
	CopyToBodyQue( self );
	// set default spawn parms
//	SetChangeParms();
	SetNewParms( false );
	// respawn              
	PutClientInServer();
}


/*
============
ClientKill

Player entered the suicide command
============
*/
void ClientKill()
{

	if( k_pause || k_standby )
        return;

	if (g_globalvars.time < self->suicide_time) {
		G_sprint (self, PRINT_HIGH, "Only one suicide in 5 seconds\n");
		return;
	}

	self->suicide_time = g_globalvars.time + 5;

    k_nochange = 0;

	if ( match_in_progress == 2 ) // qqshka - message only during match
		G_bprint( PRINT_MEDIUM, "%s suicides\n", self->s.v.netname );

	set_suicide_frame();
	self->s.v.modelindex = modelindex_player;
	logfrag( self, self );

	if( match_in_progress == 2 )	// KTEAMS
		self->s.v.frags -= 2;	// extra penalty

	respawn();
}

float CheckSpawnPoint( vec3_t v )
{
	return false;
}

/*
============
SelectSpawnPoint

Returns the entity to spawn at
============
*/
gedict_t       *SelectSpawnPoint()
{
	gedict_t		*spot;
	gedict_t		*spots;			// chain of "valid" spots
	gedict_t		*thing;
	float			rndspots;
	int             numspots;		// count of "valid" spots
	int				k_nspots;
	int				totalspots;
	int				pcount;
	int				k_spw = cvar( "k_spw" );

	totalspots = numspots = 0;

// testinfo_player_start is only found in regioned levels
	spot = find( world, FOFS( s.v.classname ), "testplayerstart" );
	if ( spot )
		return spot;

// choose a info_player_deathmatch point

// ok, find all spots that don't have players nearby

	spots = world;
	spot = find( world, FOFS( s.v.classname ), "info_player_deathmatch" );
	while ( spot )
	{
		totalspots++;

		thing = findradius( world, spot->s.v.origin, 84 );
		pcount = 0;
		// find count of nearby players for 'spot'
		while ( thing )
		{
			if ( streq( thing->s.v.classname, "player" ) )
			{
                if( !(  k_spw == 2 && match_in_progress == 2 && !thing->k_1spawn )
					   // k_spw == 2 feature, if player is spawned not far away and run
					   // around spot - treat this spot as not valid.
					   // k_1spawn store this "not far away" time.
					   // k_1spawn is _also_ set after player passed teleport
					&& ISLIVE( thing )
				  ) 
					pcount++;
			}

			thing = findradius( thing, spot->s.v.origin, 84 );
		}

		if( k_spw && match_in_progress == 2 && self->k_lastspawn == spot )
			pcount++; // ignore this spot in this case, protection from spawn twice on the same spot

		if ( !pcount ) // valid spawn spot
		{
			spot->s.v.goalentity = EDICT_TO_PROG( spots );
			spots = spot;
			numspots++;
		}
		// Get the next spot in the chain
		spot = find( spot, FOFS( s.v.classname ), "info_player_deathmatch" );
	}


    if( match_in_progress == 2 )
		self->k_1spawn = 200;

    k_nspots = totalspots; // remember totalspots

	if ( !numspots )
	{
		// ack, they are all full, just pick one at random

//  bprint (PRINT_HIGH, "Ackk! All spots are full. Selecting random spawn spot\n");

		rndspots   = g_random() * totalspots;
		totalspots = bound(0, rndspots, totalspots-1);

		spot = find( world, FOFS( s.v.classname ), "info_player_deathmatch" );
		while ( totalspots > 0 )
		{
			totalspots--;
			spot = find( spot, FOFS( s.v.classname ), "info_player_deathmatch" );
		}

		if( k_nspots > 2 && match_in_progress == 2 )
			self->k_lastspawn = spot;

		if( !match_in_progress || k_spw == 1 || ( k_spw && !k_checkx ) ) {
			vec3_t	v1, v2;

			makevectors( spot->s.v.angles );
			thing = findradius(world, spot->s.v.origin, 84);
			while( thing ) {
				if( streq( thing->s.v.classname, "player" ) && ISLIVE( thing ) ) {
					VectorMA (thing->s.v.origin, -15.0, g_globalvars.v_up, v1);
					VectorMA (v1, 160.0, g_globalvars.v_forward, v2);

					traceline( PASSVEC3( v1 ), PASSVEC3( v2 ), false, thing);

					VectorCopy(g_globalvars.trace_endpos, v1);
					VectorMA (v1, 30.0, g_globalvars.v_forward, v2);

					traceline( PASSVEC3( v1 ), PASSVEC3( v2 ), false, thing);

					if( g_globalvars.trace_fraction < 1 )
						VectorMA (g_globalvars.trace_endpos, -35.0, g_globalvars.v_forward, v1);

					VectorMA (v1, 15.0, g_globalvars.v_up, v2);
					setorigin(thing, PASSVEC3( v2 )); // FIXME: wtf is going on?

				} else if( streq( thing->s.v.classname, "teledeath") ) {
					gedict_t *rm = thing;

					thing = findradius(thing, spot->s.v.origin, 84);
					ent_remove( rm );

					continue;
				}

				thing = findradius(thing, spot->s.v.origin, 84);
			}
		}

		return spot;
	}
// We now have the number of spots available on the map in numspots

// Generate a random number between 0 and numspots

	rndspots = g_random() * numspots;
	numspots = bound(0, rndspots, numspots-1);

	spot = spots;
	while ( numspots > 0 )
	{
		spot = PROG_TO_EDICT( spot->s.v.goalentity );
		numspots--;
	}


	if( k_nspots > 2 && match_in_progress == 2 ) 
        self->k_lastspawn = spot;

	return spot;
}

////////////////
// GlobalParams:
// time
// self
// params
///////////////
void ClientConnect()
{
#ifdef KTEAMS
	float f1 = 0;
	gedict_t *p;
	char *tmp;

	Vip_ShowRights( self );
#endif

	k_nochange = 0;


//	if (deathmatch /*|| coop*/ )
//	G_bprint( PRINT_HIGH, "%s entered the game\n", self->s.v.netname );

	// if the guy started connecting during intermission and
	// thus missed the svc_intermission, we'd better let him know
	if ( intermission_running )
		SendIntermissionToClient ();


#ifdef KTEAMS

// ILLEGALFPS[

	// Zibbo's frametime checking code
	self->uptimebugpolicy = 0;
	self->real_time = 0;

	// delay on checking/displaying illegal FPS.
	self->fDisplayIllegalFPS = g_globalvars.time + 10 + g_random() * 5;
	self->fIllegalFPSWarnings = 0;

	ZeroFpsStats ();
// ILLEGALFPS]

	self->fraggie = 0;
	MakeMOTD();
	CheckRate(self, "");

	if( k_captains == 2 ) {
// in case of team picking, pick player if there's a captain with his/her team
		p = find(world, FOFCLSN, "player");
		while( p && !f1 ) {

			while( p && !p->k_captain )
				p = find(p, FOFCLSN, "player");

			if( p && p->k_captain ) {

				tmp = getteam(p);

				if( streq( tmp, getteam(self) ) ) {
					G_sprint(self, 2, "Team picking in progress\n");
					G_bprint(2, "%s is set to team \x90%s\x91\n", self->s.v.netname, tmp);

					// set color, team is already set %)
					stuffcmd(self, "color %d %d\n", iKey(p, "topcolor"), iKey(p, "bottomcolor"));
					f1 = 1;
				}
				else
					p = find(p, FOFCLSN, "player");
			}
		}
// in case of team picking, check if there is a free spot for player number 1-16
		if( !f1 ) {
			p = self;

			while( p && f1 < 16 ) {
				f1++;

				for( p = world; (p = find(p, FOFCLSN, "player")) && p->s.v.frags != f1; )
					; // empty
			}

			// if we found a spot, set the player into it
			if( !p ) {
				stuffcmd(self, "color 0\nteam \"\"\nskin \"\"\n");
				self->fraggie = f1;
			}
		}
	}
#endif
}

////////////////
// GlobalParams:
// time
// self
// called after ClientConnect
///////////////
void PutClientInServer()
{

	gedict_t       *spot;
	vec3_t          v;
	int             items;
//	char            s[20];

//	G_bprint(2, "PutClientInServer()\n");

	self->s.v.classname = "player";
	self->s.v.health = 100;
	self->s.v.takedamage = DAMAGE_AIM;
	self->s.v.solid = SOLID_SLIDEBOX;
	self->s.v.movetype = MOVETYPE_WALK;
	self->show_hostile = 0;
	self->s.v.max_health = 100;
	self->s.v.flags = FL_CLIENT;
	self->air_finished = g_globalvars.time + 12;
	self->dmg = 2;		// initial water damage
	self->super_damage_finished = 0;
	self->radsuit_finished = 0;
	self->invisible_finished = 0;
	self->invincible_finished = 0;
	self->s.v.effects = 0;
	self->spawn_time = g_globalvars.time;


#ifdef KTEAMS
// the spawn falling damage bug workaround
	self->jump_flag = 0;
	self->swim_flag = 0;

// brokenankle
    self->brokenankle = 0;
#endif

	self->invincible_time = 0;

	DecodeLevelParms();

	if ( !( (int)self->s.v.weapon & (int)self->s.v.items ) )
		self->s.v.weapon = W_BestWeapon();
	W_SetCurrentAmmo();

	self->attack_finished = g_globalvars.time;
	self->th_pain = player_pain;
	self->th_die = PlayerDie;

	self->s.v.deadflag = DEAD_NO;
// paustime is set by teleporters to keep the player from moving a while
	self->pausetime = 0;

	spot = SelectSpawnPoint();
	VectorCopy( spot->s.v.origin, self->s.v.origin );
	self->s.v.origin[2] += 1;
	VectorCopy( spot->s.v.angles, self->s.v.angles );

	self->s.v.fixangle = true;


// oh, this is a hack!
	setmodel( self, "progs/eyes.mdl" );
	modelindex_eyes = self->s.v.modelindex;

	setmodel( self, "progs/player.mdl" );
	modelindex_player = self->s.v.modelindex;

	setsize( self, PASSVEC3( VEC_HULL_MIN ), PASSVEC3( VEC_HULL_MAX ) );
	SetVector( self->s.v.view_ofs, 0, 0, 22 );
	SetVector( self->s.v.velocity, 0, 0, 0 );

	player_stand1();

    if (  1 /* deathmatch */ /*|| coop FIXME: remove??? */)
    {

	// FIXME: stupid way

		// do not accept player ez only in case of match.
		// player may be kicked in MOTD stuff while pass some checks.
		// if not kicked, player will be "spawned" in PlayerPostThink(),
		// just look code -> if(self->k_accepted == 1)

		if( !self->k_accepted && match_in_progress == 2 ) {
			self->s.v.classname = "player_na"; // player not accepted yet
			self->s.v.takedamage = 0;
			self->s.v.solid = 0;
			self->s.v.movetype = 0;
			self->s.v.modelindex = 0;
			self->s.v.model = "";
    	} 
    	else {
		// if just prewar or even countdown - accept player immediately
        	self->k_accepted = 2;
        	self->s.v.takedamage = 2;
        	self->s.v.solid = 3;
        	self->s.v.movetype = 3;
        	setmodel (self, "progs/player.mdl");
			modelindex_player = self->s.v.modelindex;
        	player_stand1 ();

			makevectors( self->s.v.angles );
			VectorScale( g_globalvars.v_forward, 20, v );
			VectorAdd( v, self->s.v.origin, v );
			spawn_tfog( v );
			play_teleport( self );
			spawn_tdeath( self->s.v.origin, self );

			//berzerk will not affect players that logs in during berzerk
			//spawn666 will not affect the first spawn of players connecting to a game in progress
			if( match_in_progress == 2 ) {
				if( cvar( "k_bzk" ) && k_berzerk ) {
					self->s.v.items = (int)self->s.v.items | 4194304;
					self->super_time = 1;
					self->super_damage_finished = g_globalvars.time + 3600;
				}

				if( cvar( "k_666" ) ) {
					stuffcmd (self, "bf\n");
					self->invincible_time = 1;
					self->invincible_finished = g_globalvars.time + 2;
					self->k_666 = 1;
					self->s.v.items = (int)self->s.v.items | 1048576;
				}
			}


		}
	}

	if ( deathmatch == 4 && match_in_progress == 2 )
	{
		float dmm4_invinc_time = cvar("dmm4_invinc_time");

 		// default is 2, negative value disable invincible
		dmm4_invinc_time = (dmm4_invinc_time ? bound(0, dmm4_invinc_time, 30) : 2);

		self->s.v.ammo_shells = 0;
		items = self->s.v.items;
		self->s.v.ammo_nails   = 255;
		self->s.v.ammo_shells  = 255;
		self->s.v.ammo_rockets = 255;
		self->s.v.ammo_cells   = 255;
		items |= IT_NAILGUN;
		items |= IT_SUPER_NAILGUN;
		items |= IT_SUPER_SHOTGUN;
		items |= IT_ROCKET_LAUNCHER;
		items |= IT_GRENADE_LAUNCHER;
		items |= IT_LIGHTNING;

		items -= ( items & ( IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3 ) ) - IT_ARMOR3;
		self->s.v.armorvalue = 200;
		self->s.v.armortype = 0.8;
		self->s.v.health = 250;

		if ( dmm4_invinc_time > 0 ) {
			items |= IT_INVULNERABILITY;
			self->invincible_time = 1;
			self->invincible_finished = g_globalvars.time + dmm4_invinc_time;
		}
		self->s.v.items = items;
	}

	if ( deathmatch == 5 && match_in_progress == 2 )
	{
		self->s.v.ammo_nails   = 80;
		self->s.v.ammo_shells  = 30;
		self->s.v.ammo_rockets = 10;
		self->s.v.ammo_cells   = 30;
		items = self->s.v.items;
		items |= IT_NAILGUN;
		items |= IT_SUPER_NAILGUN;
		items |= IT_SUPER_SHOTGUN;
		items |= IT_ROCKET_LAUNCHER;
		items |= IT_GRENADE_LAUNCHER;
		items |= IT_LIGHTNING;
		items -= ( items & ( IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3 ) ) - IT_ARMOR3;
		self->s.v.armorvalue = 200;
		self->s.v.armortype = 0.8;
		self->s.v.health = 200;
		items |= IT_INVULNERABILITY;
		self->s.v.items = items;
		self->invincible_time = 1;
		self->invincible_finished = g_globalvars.time + 3;
	}

	// remove particular weapons in dmm4
	if ( deathmatch == 4 && match_in_progress == 2 )
	{
		int	k_disallow_weapons = (int)cvar("k_disallow_weapons") & DA_WPNS;

		self->s.v.items = (int)self->s.v.items & ~k_disallow_weapons;
	}

	if ( !( (int)self->s.v.weapon & (int)self->s.v.items ) )
		self->s.v.weapon = W_BestWeapon();
	W_SetCurrentAmmo();
}

/*
===============================================================================

RULES

===============================================================================
*/


/*
============
CheckRules

Exit deathmatch games upon conditions
============
*/
void CheckRules()
{
    if ( fraglimit && self->s.v.frags >= fraglimit )
        EndMatch( 0 );
}

//============================================================================
void PlayerDeathThink()
{
// gedict_t*    old_self;
	float           forward;

#ifdef KTEAMS
    if( k_standby )
        return;
#endif

	if ( ( ( int ) ( self->s.v.flags ) ) & FL_ONGROUND )
	{
		forward = vlen( self->s.v.velocity );
		forward = forward - 20;
		if ( forward <= 0 )
			SetVector( self->s.v.velocity, 0, 0, 0 );
		else
		{
			VectorNormalize( self->s.v.velocity );
			VectorScale( self->s.v.velocity, forward, self->s.v.velocity );
		}
	}


// { autospawn
	if( (g_globalvars.time - self->dead_time) > 5 && match_in_progress ) {
		self->s.v.deadflag = DEAD_RESPAWNABLE;
		self->s.v.button0 = 0;
		self->s.v.button1 = 0;
		self->s.v.button2 = 0;
		respawn();
		return;
	}
// }

// wait for all buttons released
	if ( self->s.v.deadflag == DEAD_DEAD )
	{
		if ( self->s.v.button2 || self->s.v.button1 || self->s.v.button0 )
			return;
		self->s.v.deadflag = DEAD_RESPAWNABLE;
		return;
	}
// wait for any button down
	if ( !self->s.v.button2 && !self->s.v.button1 && !self->s.v.button0 )
		return;

	self->s.v.button0 = 0;
	self->s.v.button1 = 0;
	self->s.v.button2 = 0;
	respawn();
}


void PlayerJump()
{
	//vec3_t start, end;

	if ( ( ( int ) ( self->s.v.flags ) ) & FL_WATERJUMP )
		return;

	if ( self->s.v.waterlevel >= 2 )
	{
// play swiming sound
		if ( self->swim_flag < g_globalvars.time )
		{
			self->swim_flag = g_globalvars.time + 1;
			if ( g_random() < 0.5 )
				sound( self, CHAN_BODY, "misc/water1.wav", 1,
					    ATTN_NORM );
			else
				sound( self, CHAN_BODY, "misc/water2.wav", 1,
					    ATTN_NORM );
		}

		return;
	}

	if ( !( ( ( int ) ( self->s.v.flags ) ) & FL_ONGROUND ) )
		return;

	if ( !( ( ( int ) ( self->s.v.flags ) ) & FL_JUMPRELEASED ) )
		return;		// don't pogo stick

	self->s.v.flags -= ( ( ( int ) ( self->s.v.flags ) ) & FL_JUMPRELEASED );
//	self->s.v.button2 = 0;

	self->was_jump = true;

// check the flag and jump if we can
    if ( !self->brokenankle )
	{
        self->s.v.button2 = 0;

		// player jumping sound
		sound( self, CHAN_BODY, "player/plyrjmp8.wav", 1, ATTN_NORM );

       	// JUMPBUG[
        // the engine checks velocity_z and won't perform the jump if it's < zero!

		// qqshka - i know this is shit - but ktpro does it, and probably server engine even ignor this hack
		// qqshka - but seems new zquake pmove code ported to mvdsv - so this fix is commented

//		if ( self->s.v.velocity[2] < 0 )
//			self->s.v.velocity[2] = 0;

		// JUMPBUG]
	}
	else
		 self->s.v.velocity[2] = -270;
}


/*
===========
WaterMove

============
*/

void WaterMove()
{
//dprint (ftos(self->s.v.waterlevel));
	if ( self->s.v.movetype == MOVETYPE_NOCLIP )
		return;

	if ( ISDEAD( self ) )
		return;

	if ( self->s.v.waterlevel != 3 )
	{
		if ( self->air_finished < g_globalvars.time )
			sound( self, CHAN_VOICE, "player/gasp2.wav", 1, ATTN_NORM );
		else if ( self->air_finished < g_globalvars.time + 9 )
			sound( self, CHAN_VOICE, "player/gasp1.wav", 1, ATTN_NORM );

		self->air_finished = g_globalvars.time + 12;
		self->dmg = 2;

	} else if ( self->air_finished < g_globalvars.time )
	{			// drown!
		if ( self->pain_finished < g_globalvars.time )
		{
			self->dmg = self->dmg + 2;
			if ( self->dmg > 15 )
				self->dmg = 10;

			T_Damage( self, world, world, self->dmg );
			self->pain_finished = g_globalvars.time + 1;
		}
	}

	if ( !self->s.v.waterlevel )
	{
		if ( ( ( int ) ( self->s.v.flags ) ) & FL_INWATER )
		{
			// play leave water sound
			sound( self, CHAN_BODY, "misc/outwater.wav", 1, ATTN_NORM );
			self->s.v.flags -= FL_INWATER;
		}
		return;
	}

	if ( self->s.v.watertype == CONTENT_LAVA )
	{			// do damage
		if ( self->dmgtime < g_globalvars.time )
		{
			if ( self->radsuit_finished > g_globalvars.time )
				self->dmgtime = g_globalvars.time + 1;
			else
				self->dmgtime = g_globalvars.time + 0.2;

			T_Damage( self, world, world, 10 * self->s.v.waterlevel );
		}

	} else if ( self->s.v.watertype == CONTENT_SLIME )
	{			// do damage
		if ( self->dmgtime < g_globalvars.time
		     && self->radsuit_finished < g_globalvars.time )
		{
			self->dmgtime = g_globalvars.time + 1;
			T_Damage( self, world, world, 4 * self->s.v.waterlevel );
		}
	}

	if ( !( ( ( int ) ( self->s.v.flags ) ) & FL_INWATER ) )
	{

// player enter water sound

		if ( self->s.v.watertype == CONTENT_LAVA )
			sound( self, CHAN_BODY, "player/inlava.wav", 1, ATTN_NORM );
		if ( self->s.v.watertype == CONTENT_WATER )
			sound( self, CHAN_BODY, "player/inh2o.wav", 1, ATTN_NORM );
		if ( self->s.v.watertype == CONTENT_SLIME )
			sound( self, CHAN_BODY, "player/slimbrn2.wav", 1,
				    ATTN_NORM );

		self->s.v.flags += FL_INWATER;
		self->dmgtime = 0;
	}
}


void CheckWaterJump()
{
	vec3_t          start, end;

// check for a jump-out-of-water
	makevectors( self->s.v.angles );
	
	VectorCopy( self->s.v.origin, start );	//start = self->s.v.origin;
	start[2] = start[2] + 8;
	g_globalvars.v_forward[2] = 0;

	VectorNormalize( g_globalvars.v_forward );
	VectorScale( g_globalvars.v_forward, 24, end );
	VectorAdd( start, end, end );	//end = start + v_forward*24;

	traceline( PASSVEC3( start ), PASSVEC3( end ), true, self );

	if ( g_globalvars.trace_fraction < 1 )
	{			// solid at waist
		start[2] = start[2] + self->s.v.maxs[2] - 8;
		VectorScale( g_globalvars.v_forward, 24, end );
		VectorAdd( start, end, end );	//end = start + v_forward*24;
		VectorScale( g_globalvars.trace_plane_normal, -50, self->s.v.movedir );	//self->s.v.movedir = trace_plane_normal * -50;

		traceline( PASSVEC3( start ), PASSVEC3( end ), true, self );

		if ( g_globalvars.trace_fraction == 1 )
		{		// open at eye level
			self->s.v.flags = ( ( int ) ( self->s.v.flags ) ) | FL_WATERJUMP;
			self->s.v.velocity[2] = 225;
			self->s.v.flags -=
			    ( ( ( int ) ( self->s.v.flags ) ) & FL_JUMPRELEASED );
			self->s.v.teleport_time = g_globalvars.time + 2;	// safety net
			return;
		}
	}
}


void MakeGhost ()
{
	gedict_t *ghost;
	float f1 = 1;
	float f2 = 0;

	while( f1 < k_userid && !f2 ) {
		if( strnull( ezinfokey(world, va("%d", (int)f1) ) ) )
			f2 = 1;
		else
			f1++;
	}

	if( !f2 )
		k_userid++;

	ghost = spawn();
	ghost->s.v.owner  = EDICT_TO_PROG( world );
	ghost->s.v.classname = "ghost";
	ghost->cnt2       = f1;
	ghost->k_teamnum  = self->k_teamnum;
	ghost->s.v.frags  = self->s.v.frags;
	ghost->deaths     = self->deaths;
	ghost->friendly   = self->friendly;
	ghost->ready      = 0;

	ghost->ps         = self->ps; // save player stats
	ghost->ghost_dt   = g_globalvars.time; // save drop time
	ghost->ghost_clr  = bound(0, iKey(self, "topcolor" ), 13) << 8;
	ghost->ghost_clr |= bound(0, iKey(self, "bottomcolor" ), 13) ; // save colors

	localcmd("localinfo %d \"%s\"\n", (int)f1, self->s.v.netname);
	trap_executecmd();
}

////////////////
// GlobalParams:
// self
///////////////
void ClientDisconnect()
{
	float f1;

	k_nochange = 0; // force recalculate frags scores

	del_from_specs_favourites( self );

	if( match_in_progress == 2 && ( self->k_makeghost || streq("player", self->s.v.classname) ) )
	{
		G_bprint( PRINT_HIGH, "%s left the game with %.0f frags\n", self->s.v.netname,
	  					self->s.v.frags );

		sound( self, CHAN_BODY, "player/tornoff2.wav", 1, ATTN_NONE );

		if ( !k_matchLess ) // no ghost in matchless mode
			MakeGhost ();
	}

	// normal disconnect
	if( self->k_accepted == 2 ) {
		
		set_suicide_frame();

	} else {
		self->s.v.takedamage = 0;
		self->s.v.solid = 0;
		self->s.v.movetype = 0;
		self->s.v.modelindex = 0;
		self->s.v.model = "";
	}

// s: added conditional function call here
	if( self->k_admin == 1.5 || self->k_captain > 10 ) {
		G_bprint(2, "Election aborted\n");
		AbortElect();
	}

	self->k_player = 0;
	self->k_accepted = 0;
	self->ready = 0;
	self->s.v.classname = "";

// s: added conditional function call here
	if( self->k_kicking )
		ExitKick( self );

	if( self->k_captain ) {
        G_bprint(2, "A %s has left\n", redtext("captain"));

		ExitCaptain();
	}

	if( k_captains == 2 )
		CheckFinishCaptain();

	if( cvar( "k_idletime" ) > 0 )
		IdlebotCheck();

	f1 = CountALLPlayers();

	if( !f1
		&& (
			 (    !cvar( "k_master" )
			   && !cvar( "k_lockmap" ) 
			 )
			 || ( !cvar( "lock_practice" ) && k_practice ) // reload map even k_master or k_lockmap set
	       )
	  )
	{
		char *s;
		int old_k_practice = k_practice;

		if ( k_pause ) {
			G_bprint(2, "No players left, unpausing.\n");
			ModPause ( 0 ); // no players left, unpause server if paused
		}

		if ( !cvar( "lock_practice" ) && k_practice )  // #practice mode#
			SetPractice( 0, NULL ); // return server to normal mode but not reload map yet

		if( match_in_progress )
			EndMatch( 1 ); // skip demo, make some other stuff

		// Check if issued to execute reset.cfg (sturm)
        if( cvar( "k_autoreset" ) ) {
			char *cfg_name = "configs/reset.cfg";

			if ( can_exec( cfg_name ) )
				localcmd( "exec %s\n", cfg_name );
		}

        s = k_matchLess ? "" : cvar_string( "k_defmap" ); // no defmap in matchLess mode

		// force reload current map in practice mode even k_defmap is set or not
        if ( !cvar( "lock_practice" ) && old_k_practice )
			s = g_globalvars.mapname; // FIXME: k_defmap may not exist on server disk, so we reload current map
									  //        but we may check if k_defmap exist and reload to it, right?

        if( !strnull( s ) )
            changelevel( s );
	}
}

void BackFromLag()
{
	int timing_players_action = TA_ALL & (int)cvar( "timing_players_action" );

	self->k_timingWarnTime = 0;

	if ( timing_players_action & TA_INFO )
		G_bprint(2, "%s %s\n", self->s.v.netname, redtext( "is back from lag") );

	if ( timing_players_action & TA_INVINCIBLE ) {
		self->s.v.takedamage = self->k_timingTakedmg;
		self->s.v.solid 	 = self->k_timingSolid;
		self->s.v.movetype	 = self->k_timingMovetype;
	}
}

#define S_AXE   ( 1<<0 )
#define S_SG    ( 1<<1 )
#define S_SSG   ( 1<<2 )
#define S_NG    ( 1<<3 )
#define S_SNG   ( 1<<4 )
#define S_GL    ( 1<<5 )
#define S_RL    ( 1<<6 )
#define S_LG    ( 1<<7 )

#define S_ALL   ( S_AXE | S_SG | S_SSG | S_NG | S_SNG | S_GL | S_RL | S_LG )

#define S_DEF   ( S_GL | S_RL | S_LG ) /* default */

void Print_Wp_Stats( )
{
	char buf[1024] = {0};

	int  i;
	int _wps = S_ALL & iKey ( self, "wps" );
	int  wps = ( _wps ? _wps : S_DEF ); // if wps is not set - show S_DEF weapons

	gedict_t *g = self->k_spectator ? PROG_TO_EDICT( self->s.v.goalentity ) : NULL;
	gedict_t *e = self->k_player ? self : ( g ? g : world ); // stats of whom we want to show

	float axe = wps & S_AXE ? 100.0 * e->ps.h_axe / max(1, e->ps.a_axe) : 0;
	float sg  = wps & S_SG  ? 100.0 * e->ps.h_sg  / max(1, e->ps.a_sg) : 0;
	float ssg = wps & S_SSG ? 100.0 * e->ps.h_ssg / max(1, e->ps.a_ssg) : 0;
	float ng  = wps & S_NG  ? 100.0 * e->ps.h_ng  / max(1, e->ps.a_ng) : 0;
	float sng = wps & S_SNG ? 100.0 * e->ps.h_sng / max(1, e->ps.a_sng) : 0;
#if 0 /* percentage */
	float gl  = wps & S_GL  ? 100.0 * e->ps.h_gl  / max(1, e->ps.a_gl) : 0;
	float rl  = wps & S_RL  ? 100.0 * e->ps.h_rl  / max(1, e->ps.a_rl) : 0;
#else /* just count of direct hits */
	float gl  = wps & S_GL  ? e->ps.h_gl : 0;
	float rl  = wps & S_RL  ? max(0.001, e->ps.h_rl) : 0;
#endif
	float lg  = wps & S_LG  ? max(0.001, 100.0 * e->ps.h_lg / max(1, e->ps.a_lg)) : 0;

	if ( (i = iKey( self, "lw" )) > 0 ) {
		i = bound(0, i, sizeof(buf)-1 );
		memset( (void*)buf, (int)'\n', i);
		buf[i] = 0;
	}

	if ( e == world || !e->k_player ) { // spec tracking no one
		G_centerprint( self, "%s%s", buf, redtext("Tracking noone (+wp_stats)"));

		self->need_clearCP  = 1;
		self->wp_stats_time = g_globalvars.time + 0.8;

		return;
	}

	if ( !axe && !sg && !ssg && !ng && !sng && !gl && !rl && !lg )
		return; // sanity

	if ( rl || lg || gl ) {
		if ( lg )
			strlcat(buf, (lg  ? va("%s%s:%.1f", (*buf ? " " : ""), redtext("lg") , lg) : ""), sizeof(buf));
		if ( rl )
			strlcat(buf, (rl  ? va("%s%s:%.0f", (*buf ? " " : ""), redtext("rl"),  rl) : ""), sizeof(buf));
		if ( gl )
			strlcat(buf, (gl  ? va("%s%s:%.0f", (*buf ? " " : ""), redtext("gl") , gl) : ""), sizeof(buf));

		strlcat(buf, "\n", sizeof(buf));
	}

	if ( axe || sg || ssg ) {
		if ( axe )
			strlcat(buf, (axe ? va("%s:%.1f", redtext("axe"),  axe) : ""), sizeof(buf));
		if ( sg )
			strlcat(buf, (sg  ? va("%s%s:%.1f", (*buf ? " " : ""), redtext("sg") , sg) : ""), sizeof(buf));
    	if ( ssg )
			strlcat(buf, (ssg ? va("%s%s:%.1f", (*buf ? " " : ""), redtext("ssg") , ssg) : ""), sizeof(buf));

		strlcat(buf, "\n", sizeof(buf));
	}

	if ( ng || sng ) {
		 if ( ng )
			strlcat(buf, (ng  ? va("%s:%.1f", redtext("ng"), ng) : "" ), sizeof(buf));
		 if ( sng )
			strlcat(buf, (sng ? va("%s%s:%.1f", (*buf ? " " : ""), redtext("sng"), sng) : ""), sizeof(buf));

		strlcat(buf, "\n", sizeof(buf));
	}

	if ( (i = iKey( self, "lw" )) < 0 ) {
		int offset = strlen(buf);
		i = bound(0, -i, (int)sizeof(buf) - offset - 1);
		memset( (void*)(buf + offset), (int)'\n', i);
		buf[i+offset] = 0;
	}

	if ( strnull( buf ) )
		return; // sanity

	self->need_clearCP  = 1;
	self->wp_stats_time = g_globalvars.time + 0.8;

	G_centerprint( self, "%s",  buf );
}

void Print_Scores( )
{
	char buf[1024] = {0};

	int  i, minutes = 0, seconds = 0, ts, es;

	qboolean sc_ok = false;
	gedict_t *p, *ed1, *ed2;
	gedict_t *g = self->k_spectator ? PROG_TO_EDICT( self->s.v.goalentity ) : NULL;
	gedict_t *e = self->k_player ? self : ( g ? g : world ); // stats of whom we want to show

	if ( (i = iKey( self, "ls" )) > 0 ) {
		i = bound(0, i, sizeof(buf)-1 );
		memset( (void*)buf, (int)'\n', i);
		buf[i] = 0;
	}

	if ( e == world || !e->k_player ) { // spec tracking noone
		G_centerprint( self, "%s%s", buf, redtext("Tracking noone (+scores)"));

		self->need_clearCP  = 1;
		self->sc_stats_time = g_globalvars.time + 0.8;

		return;
	}

	if( (p = find(world, FOFCLSN, "timer")) && match_in_progress == 2 ) {
		minutes = p->cnt;
		seconds = p->cnt2;
		if( seconds == 60 )
			seconds = 0;
		else
			minutes--;

	}

	strlcat(buf, va("%s:%02d:%02d", redtext("tl"), minutes, seconds), sizeof(buf));

	if( k_showscores ) {
		int s1 = get_scores1();
		int s2 = get_scores2();
		char *t1 = cvar_string( "_k_team1" );
		char *t2 = getteam(e);

		ts = streq(t1, t2) ? s1 : s2;
		es = streq(t1, t2) ? s2 : s1;

		sc_ok = true;
	}
	else if ( (ed1 = get_ed_scores1()) && (ed2 = get_ed_scores2()) ) {
		ts = e->s.v.frags;
		es = ed1 == e ? ed2->s.v.frags : ed1->s.v.frags;

		sc_ok = true;
	}

	if ( sc_ok )
		strlcat(buf, va("  %s:%d  %s:%d  \x90%d\x91", 
						redtext("t"), ts, redtext("e"), es, (ts-es)), sizeof(buf));

	if ( (i = iKey( self, "ls" )) < 0 ) {
		int offset = strlen(buf);
		i = bound(0, -i, (int)sizeof(buf) - offset - 1);
		memset( (void*)(buf + offset), (int)'\n', i);
		buf[i+offset] = 0;
	}

	if ( strnull( buf ) )
		return; // sanity

	self->need_clearCP  = 1;
	self->sc_stats_time = g_globalvars.time + 0.8;

	G_centerprint( self, "%s",  buf );
}

// qqshka - ripped from ktpro

float v_for_jump (int frametime_ms)
{
	if ( cvar("k_no_fps_physics") )
		return 1;

//	G_bprint(2, "MS: %d\n", frametime_ms);

	if (frametime_ms > 44) 
		return  1.05;
	else if (frametime_ms > 38) 
		return  1.041;
	else if (frametime_ms > 33) 
		return  1.035;
	else if (frametime_ms > 28) 
		return  1.032;
	else if (frametime_ms > 24) 
		return  1.029;
	else if (frametime_ms > 22) 
		return  1.025;
	else if (frametime_ms > 19) 
		return  1.02;
	else if (frametime_ms > 18) 
		return  1.015;
	else if (frametime_ms > 16) 
		return  1.01;
	else if (frametime_ms > 15) 
		return  1.005;
	else if (frametime_ms > 14) 
		return  1;
	else if (frametime_ms > 13) 
		return  1;
	else if (frametime_ms > 12) 
		return  1;
	else if (frametime_ms > 11) 
		return  0.9991;
	else if (frametime_ms > 10) 
		return  0.9982;
	else if (frametime_ms > 9) 
		return  0.9961;
	else if (frametime_ms > 8) 
		return  0.9941;
	else if (frametime_ms > 7) 
		return  0.9886;
	else if (frametime_ms > 6) 
		return  0.9882;
	else if (frametime_ms > 5) 
		return  0.9865;
	else if (frametime_ms > 4) 
		return  0.9855;
	else if (frametime_ms > 3) 
		return  0.9801;
	else if (frametime_ms > 2) 
		return  0.9783;
	else 
		return  0.9652;
}

void ZeroFpsStats ()
{
	// zero these so the average/highest FPS is calculated for each delay period.
	self->fAverageFrameTime = 0;
	self->fFrameCount = 0;
	self->fLowestFrameTime = 0.999;
	self->fHighestFrameTime = 0.0001f;
}


////////////////
// GlobalParams:
// time
// frametime
// self
///////////////
/*
================
PlayerPreThink

Called every frame before physics are run
================
*/

void PlayerPreThink()
{
#ifdef KTEAMS
	float   r;
	qboolean zeroFps = false;

	if ( self->k_timingWarnTime )
		BackFromLag();

	if ( self->sc_stats && self->sc_stats_time && self->sc_stats_time <= g_globalvars.time )
		Print_Scores ();

	if ( self->wp_stats && self->wp_stats_time && self->wp_stats_time <= g_globalvars.time )
		Print_Wp_Stats ();

	if ( self->was_jump ) {
		self->s.v.velocity[2] *= v_for_jump (self->fCurrentFrameTime * 1000);
		self->was_jump = false;
	}

// ILLEGALFPS[

	self->fAverageFrameTime += g_globalvars.frametime;
	self->fFrameCount += 1;

	self->fCurrentFrameTime = g_globalvars.frametime;

	if( g_globalvars.frametime < self->fLowestFrameTime )
		self->fLowestFrameTime = g_globalvars.frametime;

	if( g_globalvars.frametime > self->fHighestFrameTime )
		self->fHighestFrameTime = g_globalvars.frametime;
	
	if( self->fDisplayIllegalFPS < g_globalvars.time && framechecks )
	{
		float fps;

// client uptime check
// code by Zibbo
		r = self->fAverageFrameTime * 100 / (g_globalvars.time - self->real_time);

		if( r > 103 && !match_in_progress ) {
			G_sprint(self, PRINT_HIGH, 
				"WARNING: QW clients up to 2.30 have a timer related bug which is caused by too"
				" long uptime. Either reboot your machine or upgrade to QWCL 2.33.\n");

			G_cprint("%s%%speed%%%f\n", self->s.v.netname, r);

			if( r > 105 )
				self->uptimebugpolicy += 1;
		}

		if( self->uptimebugpolicy > 3 ) {
			G_bprint(PRINT_HIGH, "\n%s gets kicked for too long uptime\n", self->s.v.netname);
			G_sprint(self, PRINT_HIGH, "Reboot your machine to get rid of this bug\n");

			GhostFlag(self);

			self->s.v.classname = "";
			stuffcmd(self, "disconnect\n"); // FIXME: stupid way
		}
// ends here


		fps = ( self->fAverageFrameTime / self->fFrameCount );

		fps = fps ? (1.0f / fps) : 1;

//		G_bprint(2, "%s FPS: %3.1f\n", self->s.v.netname, fps);
		
		if( fps > current_maxfps + 1 ) // 1 fps fluctuation is allowed :(
		{
			float peak = self->fLowestFrameTime ? (1.0f / self->fLowestFrameTime) : 1;

			G_bprint( PRINT_HIGH,
				"\n"
				"WARNING: %s is using the timedemo bug and has abnormally high frame rates, "
				"highest FPS = %3.1f, average FPS = %3.1f!\n",
							self->s.v.netname, peak, fps);
							
			self->fIllegalFPSWarnings += 1;
			
            if( self->fIllegalFPSWarnings > 3 )
			{
				// kick the player from server!
				// s: changed the text a bit :)
            	G_bprint(PRINT_HIGH, "%s gets kicked for timedemo cheating\n", self->s.v.netname );
				GhostFlag(self);
            	self->s.v.classname = "";
            	stuffcmd(self, "disconnect\n"); // FIXME: stupid way
            }
		}

		zeroFps = true;
	}

	// zero these so the average/highest FPS is calculated for each delay period.
	if (self->fDisplayIllegalFPS < g_globalvars.time || zeroFps ) {
		self->real_time = g_globalvars.time;

		// delay on checking/displaying illegal FPS.
		// s: changed to 15 for more accurate calculation (lag screws it up)
		self->fDisplayIllegalFPS = g_globalvars.time + 15;

		ZeroFpsStats ();
	}

// ILLEGALFPS]
#endif


	if ( intermission_running )
	{
		IntermissionThink();	// otherwise a button could be missed between
		return;					// the think tics
	}

	if ( self->s.v.view_ofs[0] == 0 && self->s.v.view_ofs[1] == 0
	     && self->s.v.view_ofs[2] == 0 )
		return;		// intermission or finale

#ifdef KTEAMS
	if( !self->k_accepted || k_pause)
		return;
#endif

	makevectors( self->s.v.v_angle );	// is this still used

	self->deathtype = "";

	CheckRules();

// FIXME: really?
//	WaterMove();

/*
 if (self->s.v.waterlevel == 2)
  CheckWaterJump ();
*/

	if ( self->s.v.deadflag >= DEAD_DEAD )
	{
		PlayerDeathThink();
		return;
	}

	if ( self->s.v.deadflag == DEAD_DYING )
	{
        // Sometimes (rarely) the death animation functions in player.qc aren't
        // invoked on death for some reason (couldn't figure out why). This leads to a
        // state when the player stands still after dying and can't respawn or even
        // suicide and has to reconnect. This is checked and fixed here
        if( g_globalvars.time > (self->dead_time + 0.1)
			&& ( self->s.v.frame < 41 || self->s.v.frame > 102 ) // FIXME: hardcoded range of dead frames
		  ) {
			StartDie();
		}

		return;		// dying, so do nothing
	}

// brokenankle included here
	if ( self->s.v.button2 || self->brokenankle )
	{
		PlayerJump();
	} else
		self->s.v.flags = ( ( int ) ( self->s.v.flags ) ) | FL_JUMPRELEASED;

// teleporters can force a non-moving pause g_globalvars.time        
	if ( g_globalvars.time < self->pausetime )
		SetVector( self->s.v.velocity, 0, 0, 0 );

	if ( g_globalvars.time > self->attack_finished && self->s.v.currentammo == 0
	     && self->s.v.weapon != IT_AXE )
	{
		self->s.v.weapon = W_BestWeapon();
		W_SetCurrentAmmo();
	}

}

/////////////////////////////////////////////////////////////////
/*
================
CheckPowerups

Check for turning off powerups
================
*/
void CheckPowerups()
{
	qboolean qlon = false;
	qboolean plon = false;

	if ( ISDEAD( self ) )
		return;

// invisibility
	if ( self->invisible_finished )
	{
// sound and screen flash when items starts to run out
		if ( self->invisible_sound < g_globalvars.time )
		{
			sound( self, CHAN_AUTO, "items/inv3.wav", 0.5, ATTN_IDLE );
			self->invisible_sound =
			    g_globalvars.time + ( ( g_random() * 3 ) + 1 );
		}


		if ( self->invisible_finished < g_globalvars.time + 3 )
		{
			if ( self->invisible_time == 1 )
			{
				G_sprint( self, PRINT_HIGH,
					  "Ring of Shadows magic is fading\n" );
				stuffcmd( self, "bf\n" );
				sound( self, CHAN_AUTO, "items/inv2.wav", 1, ATTN_NORM );
				self->invisible_time = g_globalvars.time + 1;
			}

			if ( self->invisible_time < g_globalvars.time )
			{
				self->invisible_time = g_globalvars.time + 1;
				stuffcmd( self, "bf\n" );
			}
		}

		if ( self->invisible_finished < g_globalvars.time )
		{		// just stopped
			self->s.v.items -= IT_INVISIBILITY;
			self->invisible_finished = 0;
			self->invisible_time = 0;
		}
		// use the eyes
		self->s.v.frame = 0;
		self->s.v.modelindex = modelindex_eyes;
	} else
		self->s.v.modelindex = modelindex_player;	// don't use eyes

// invincibility

	if ( self->invincible_finished )
	{
// sound and screen flash when items starts to run out
		if(self->invincible_finished < g_globalvars.time + 3 && !self->k_666)	//team
		{
			if ( self->invincible_time == 1 )
			{
				G_sprint( self, PRINT_HIGH,
					  "Protection is almost burned out\n" );
				stuffcmd( self, "bf\n" );
				sound( self, CHAN_AUTO, "items/protect2.wav", 1, ATTN_NORM );
				self->invincible_time = g_globalvars.time + 1;
			}

			if ( self->invincible_time < g_globalvars.time )
			{
				self->invincible_time = g_globalvars.time + 1;
				stuffcmd( self, "bf\n" );
			}
		}

		if ( self->invincible_finished < g_globalvars.time )
		{		// just stopped
			self->s.v.items -= IT_INVULNERABILITY;
			self->invincible_time = 0;
			self->invincible_finished = 0;
			self->k_666 = 0;		//team
		}

		if(self->invincible_finished > g_globalvars.time && !self->k_666) // KTeAMS
		{
			plon = true;
			self->s.v.effects = ( int ) self->s.v.effects | EF_DIMLIGHT;
			self->s.v.effects = ( int ) self->s.v.effects | EF_RED;
		}
		else
		{
			if ( !qlon ) // EF_DIMLIGHT shared between quad and pent
				self->s.v.effects -= ( ( int ) self->s.v.effects & EF_DIMLIGHT );
			self->s.v.effects -= ( ( int ) self->s.v.effects & EF_RED );
		}
	}

// super damage
	if ( self->super_damage_finished )
	{
// sound and screen flash when items starts to run out
		if(self->super_damage_finished < g_globalvars.time + 3 && !k_berzerk)	//team
		{
			if ( self->super_time == 1 )
			{
				if ( deathmatch == 4 )
					G_sprint( self, PRINT_HIGH,
						  "OctaPower is wearing off\n" );
				else
					G_sprint( self, PRINT_HIGH,
						  "Quad Damage is wearing off\n" );
				stuffcmd( self, "bf\n" );
				sound( self, CHAN_AUTO, "items/damage2.wav", 1, ATTN_NORM );
				self->super_time = g_globalvars.time + 1;
			}

			if ( self->super_time < g_globalvars.time )
			{
				self->super_time = g_globalvars.time + 1;
				stuffcmd( self, "bf\n" );
			}
		}

        if(self->super_damage_finished < g_globalvars.time && !k_berzerk)	//team
		{		// just stopped
			self->s.v.items -= IT_QUAD;
			if ( deathmatch == 4 )
			{
				self->s.v.ammo_cells = 255;
				self->s.v.armorvalue = 1;
				self->s.v.armortype = 0.8;
				self->s.v.health = 100;
			}
			self->super_damage_finished = 0;
			self->super_time = 0;
		}

		if ( self->super_damage_finished > g_globalvars.time )
		{
			qlon = true;
			self->s.v.effects = ( int ) self->s.v.effects | EF_DIMLIGHT;
			self->s.v.effects = ( int ) self->s.v.effects | EF_BLUE;
		}
		else
		{
			if ( !plon ) // EF_DIMLIGHT shared between quad and pent
				self->s.v.effects -= ( ( int ) self->s.v.effects & EF_DIMLIGHT );
			self->s.v.effects -= ( ( int ) self->s.v.effects & EF_BLUE );
		}
	}

// suit 
	if ( self->radsuit_finished )
	{
		self->air_finished = g_globalvars.time + 12;	// don't drown

// sound and screen flash when items starts to run out
		if ( self->radsuit_finished < g_globalvars.time + 3 )
		{
			if ( self->rad_time == 1 )
			{
				G_sprint( self, PRINT_HIGH,
					  "Air supply in Biosuit expiring\n" );
				stuffcmd( self, "bf\n" );
				sound( self, CHAN_AUTO, "items/suit2.wav", 1, ATTN_NORM );
				self->rad_time = g_globalvars.time + 1;
			}

			if ( self->rad_time < g_globalvars.time )
			{
				self->rad_time = g_globalvars.time + 1;
				stuffcmd( self, "bf\n" );
			}
		}

		if ( self->radsuit_finished < g_globalvars.time )
		{		// just stopped
			self->s.v.items -= IT_SUIT;
			self->rad_time = 0;
			self->radsuit_finished = 0;
		}
	}

}

///////////
// BothPostThink
//
// called for players and specs
//
//////////
void BothPostThink ()
{
	if ( self->shownick_time && self->shownick_time <= g_globalvars.time )
		self->shownick_time = 0;
	if ( self->wp_stats_time && self->wp_stats_time <= g_globalvars.time )
		self->wp_stats_time = 0;
	if ( self->sc_stats_time && self->sc_stats_time <= g_globalvars.time )
		self->sc_stats_time = 0;

	if (     self->need_clearCP 
		 && !self->shownick_time
         && !self->wp_stats_time
         && !self->sc_stats_time
	   )
	{
		self->need_clearCP = 0;
		G_centerprint(self, ""); // clear center print
	}
}


void            W_WeaponFrame();

////////////////
// GlobalParams:
// time
// self
///////////////
void PlayerPostThink()
{
//dprint ("post think\n");

/* FIXME: really comment?

    if (intermission_running)
    {
        setorigin (self, intermission_spot.origin);
        self.velocity = VEC_ORIGIN;	// don't stray off the intermission spot too far
        return;
    }
*/

	// ok accept player now
	if(self->k_accepted == 1) {
		vec3_t v;

		self->s.v.classname = "player";
		self->k_accepted = 2;

		if ( !intermission_running ) {
			self->s.v.takedamage = 2;
			self->s.v.solid = 3;
			self->s.v.movetype = 3;
			setmodel (self, "progs/player.mdl");
			modelindex_player = self->s.v.modelindex;
			player_stand1 ();

			makevectors(self->s.v.angles);

			VectorMA (self->s.v.origin, 20, g_globalvars.v_forward, v);

			spawn_tfog(v);
			play_teleport( self );
			spawn_tdeath (self->s.v.origin, self);
		}
	}

	if( k_pause ) {
		ImpulseCommands();
		return;
	}

	if ( self->s.v.view_ofs[0] == 0 && self->s.v.view_ofs[1] == 0
	     && self->s.v.view_ofs[2] == 0 )
		return;		// intermission or finale

	if ( self->s.v.deadflag )
		return;

#ifdef KTEAMS
	if( !self->k_accepted && match_in_progress == 2 )
			return;

	if( self->k_1spawn )
		self->k_1spawn -= 1;
//team

// WaterMove function call moved here from PlayerPreThink to avoid
// occurrence of the spawn lavaburn bug and to fix the problem on spawning
// and playing the leave water sound if the player died underwater.

    WaterMove ();


// clear the flag if we landed
    if( (int)self->s.v.flags & FL_ONGROUND )
		self->brokenankle = 0;
#endif


// check to see if player landed and play landing sound 
	if ( ( self->jump_flag < -300 )
	     && ( ( ( int ) ( self->s.v.flags ) ) & FL_ONGROUND ) )
	{
#ifdef KTEAMS
// Falling often results in 5-5 points of damage through 2 frames.
// This fixes the bug
        self->s.v.velocity[2] = 0;
#endif
		if ( self->s.v.watertype == CONTENT_WATER )
			sound( self, CHAN_BODY, "player/h2ojump.wav", 1, ATTN_NORM );
		else if ( self->jump_flag < -650 )
		{
			gedict_t *gre = PROG_TO_EDICT ( self->s.v.groundentity );

			// set the flag if needed
           	if( !cvar( "k_fallbunny" ) && self->s.v.waterlevel < 2 )
               	self->brokenankle = 1;  // Yes we have just broken it

			self->deathtype = "falling";
			T_Damage( self, world, world, 5 );
			sound( self, CHAN_VOICE, "player/land2.wav", 1, ATTN_NORM );

			if ( gre && gre->s.v.takedamage == DAMAGE_AIM )
			{
				// we landed on someone's head, hurt him
				gre->deathtype = "stomp";
				T_Damage (gre, self, self, 10);
			}
		} else
			sound( self, CHAN_VOICE, "player/land.wav", 1, ATTN_NORM );
	}

	self->jump_flag = self->s.v.velocity[2];

	CheckPowerups();

	W_WeaponFrame();

	if ( !match_in_progress && !match_over && !k_captains )
	{
		if ( iKey( self, "kf" ) & KF_SPEED ) {
			float velocity = sqrt(self->s.v.velocity[0] * self->s.v.velocity[0] + 
								self->s.v.velocity[1] * self->s.v.velocity[1]);
			self->s.v.armorvalue = (int)(velocity < 1000 ? velocity + 1000 : -velocity);
			self->s.v.frags = (int)(velocity < 1000 ? 0 : velocity / 1000);
		}
		else {
			self->s.v.armorvalue = 1000;
			self->s.v.frags = 0;
		}
 	}
}

/*
===========
ClientObituary

called when a player dies
============
*/

void ClientObituary (gedict_t *targ, gedict_t *attacker)
{
	char *deathstring,  *deathstring2;
	char *attackerteam, *targteam;
	char *attackerteam2;	//team

	// Set it so it should update scores at next attempt.
	k_nochange = 0;

    if ( strneq( targ->s.v.classname, "player" ))
		return;
    
	//ZOID 12-13-96: self.team doesn't work in QW.  Use keys
	attackerteam = getteam(attacker);
	targteam     = getteam(targ);

    if ( deathmatch > 3 )
    {
        if ( streq( targ->deathtype, "selfwater" ) )
        {
            G_bprint (PRINT_MEDIUM, "%s electrocutes %s\n ", targ->s.v.netname, g_himself(targ));

            targ->s.v.frags -=  1;
            return;
        }
    }

    if ( streq( attacker->s.v.classname, "teledeath" ) )
	{

		if( match_in_progress != 2 ) {
// qqshka - no message in prewar
//				G_bprint(PRINT_MEDIUM, "%s was telefragged\n", targ->s.v.netname);
			return;
		}

		attackerteam2 = getteam( PROG_TO_EDICT( attacker->s.v.owner ) );

		if( teamplay && streq( targteam, attackerteam2 ) && !strnull ( attackerteam2 )
				&& targ != PROG_TO_EDICT( attacker->s.v.owner ) )
		{
            G_bprint(PRINT_MEDIUM,"%s was telefragged by %s teammate\n",
													targ->s.v.netname, g_his(targ));

			targ->deaths += 1;
			return;
		}

        G_bprint (PRINT_MEDIUM, "%s was telefragged by %s\n",
			targ->s.v.netname, PROG_TO_EDICT( attacker->s.v.owner )->s.v.netname );

        logfrag (PROG_TO_EDICT( attacker->s.v.owner ), targ);

        PROG_TO_EDICT( attacker->s.v.owner )->s.v.frags += 1;

		targ->deaths += 1;
		PROG_TO_EDICT( attacker->s.v.owner )->victim = targ->s.v.netname;
		targ->killer = PROG_TO_EDICT( attacker->s.v.owner )->s.v.netname;

		return;
	}

	if ( streq( attacker->s.v.classname, "teledeath2" ) )
	{
		G_bprint (PRINT_MEDIUM, "Satan's power deflects %s's telefrag\n", targ->s.v.netname);

        targ->s.v.frags -= 1;
		logfrag (targ, targ);
		return;
	}

	// double 666 telefrag (can happen often in deathmatch 4)
	if ( streq( attacker->s.v.classname, "teledeath3" ) )
	{
		G_bprint (PRINT_MEDIUM, "%s was telefragged by %s's Satan's power\n",
					targ->s.v.netname, PROG_TO_EDICT ( attacker->s.v.owner)->s.v.netname);

		targ->s.v.frags -= 1;
		logfrag (targ, targ);
		return;
	}


	if ( streq( targ->deathtype, "squish" ) )
	{
		if ( teamplay && streq( targteam, attackerteam )
				&& !strnull( attackerteam ) && targ != attacker )
		{
			logfrag (attacker, attacker);
			attacker->s.v.frags -= 1;
			attacker->friendly += 1;

            G_bprint (PRINT_MEDIUM, "%s squished a teammate\n", attacker->s.v.netname);

			return;
		}
		else if ( streq( attacker->s.v.classname, "player" ) && attacker != targ )
		{
			G_bprint (PRINT_MEDIUM, "%s squishes %s\n", attacker->s.v.netname, targ->s.v.netname);
			logfrag (attacker, targ);
			attacker->s.v.frags += 1;
			targ->deaths += 1;
			attacker->victim = targ->s.v.netname;
			targ->killer = attacker->s.v.netname;

			return;
		}
		else
		{
			logfrag (targ, targ);
			targ->s.v.frags -= 1;            // killed self
            G_bprint (PRINT_MEDIUM, "%s was squished\n", targ->s.v.netname);

			return;
		}
	}

 	// ktpro like stomps %)
	if ( streq( targ->deathtype, "stomp" ) )
	{
		if ( isTeam() && streq( targteam, attackerteam )
				&& !strnull( attackerteam ) && targ != attacker )
		{
			logfrag (attacker, attacker);
			attacker->s.v.frags -= 1;
			attacker->friendly += 1;

			switch( (int)(g_random() * 2) ) {
				case 0:  deathstring = " was jumped by "; break;
				default:  deathstring = " was crushed by "; break;
			}

			G_bprint (PRINT_MEDIUM,"%s%s%s teammate\n",
						targ->s.v.netname, deathstring, g_his( targ ));

			return;
		}
		else if ( streq( attacker->s.v.classname, "player" ) && attacker != targ )
		{
			logfrag (attacker, targ);
			attacker->s.v.frags += 1;
			targ->deaths += 1;
			attacker->victim = targ->s.v.netname;
			targ->killer = attacker->s.v.netname;

			deathstring2 = "\n";

			switch( (int)(g_random() * 5) ) {
				case 0:  deathstring = " softens "; deathstring2 = "'s fall\n"; break;
				case 1:  deathstring = " tried to catch "; break;
				case 2:  deathstring = " was jumped by "; break;
				case 3:  deathstring = " was crushed by "; break;
				default:
						 G_bprint (PRINT_MEDIUM, "%s stomps %s\n",
										attacker->s.v.netname, targ->s.v.netname);
						 return; // !!! return !!!
			}

			G_bprint (PRINT_MEDIUM,"%s%s%s%s",
					targ->s.v.netname, deathstring, attacker->s.v.netname, deathstring2);
			return;
		}
	}

    if ( streq( attacker->s.v.classname, "player" ) ) 
    {
		if ( targ == attacker )
		{
			// killed self

			logfrag (attacker, attacker);
			attacker->s.v.frags -= 1;

			if ( streq( targ->deathtype, "grenade" ) ) {
                deathstring = " tries to put the pin back in\n";
			}
			else if ( streq( targ->deathtype, "rocket" ) )
			{
				switch( (int)(g_random() * 2) ) {
					case 0:  deathstring = " discovers blast radius\n"; break;
					default: deathstring = " becomes bored with life\n"; break;
				}
			}
			else if (targ->s.v.weapon == IT_LIGHTNING && targ->s.v.waterlevel > 1)
			{
                if (targ->s.v.watertype == CONTENT_SLIME)
                    deathstring = " discharges into the slime\n";
                else if (targ->s.v.watertype == CONTENT_LAVA)
                    deathstring = " discharges into the lava\n";
				else {
					switch( (int)(g_random() * 2) ) {
						case 0:  deathstring = " heats up the water\n"; break;
						default: deathstring = " discharges into the water\n"; break;
					}
				}
			}
			else
                deathstring = " becomes bored with life\n"; // hm, and how it is possible?

			G_bprint (PRINT_MEDIUM, "%s%s", targ->s.v.netname, deathstring);
            return;
		}
        else if ( ( teamplay == 2 && streq( targteam, attackerteam ) &&
                  !strnull( attackerteam ) ) )
		{
 			// teamkill

			switch( (int)(g_random() * 4) ) {
				case 0:  deathstring = va(" checks %s glasses\n", g_his(attacker)); break;
				case 1:  deathstring = " loses another friend\n"; break;
				case 2:  deathstring = " gets a frag for the other team\n"; break;
				default: deathstring = " mows down a teammate\n"; break;
			}

			G_bprint (PRINT_MEDIUM, "%s%s", attacker->s.v.netname, deathstring);
			attacker->s.v.frags -= 1;
			attacker->friendly += 1;

			//ZOID 12-13-96:  killing a teammate logs as suicide
			logfrag (attacker, attacker);
			return;
		}
		else
		{	// normal kill, Kteams version
			logfrag (attacker, targ);
			attacker->s.v.frags += 1;
			targ->deaths += 1;          //team				

			attacker->victim = targ->s.v.netname;
			targ->killer = attacker->s.v.netname;

			if ( targ->spawn_time + 1 > g_globalvars.time )
				attacker->ps.spawn_frags++;

			deathstring2 = "\n"; // default is "\n"

			if ( streq( targ->deathtype, "nail" ) )
			{
				switch( (int)(g_random() * 2) ) {
					case 0:  deathstring = " was body pierced by "; break;
					default: deathstring = " was nailed by "; break;
				}
			}
			else if ( streq( targ->deathtype, "supernail" ) )
			{
				switch ( (int)(g_random() * 3) ) {
					case 0:	 deathstring = " was punctured by "; break;
					case 1:	 deathstring = " was perforated by "; break;
					default: deathstring = " was ventilated by "; break;
				}

				if ( targ->s.v.health < -40 ) // quad modifier
					deathstring = " was straw-cuttered by ";
			}
			else if ( streq( targ->deathtype, "grenade" ) )
			{
				deathstring = " eats ";
				deathstring2 = "'s pineapple\n";

				if ( targ->s.v.health < -40 )
				{
					deathstring = " was gibbed by ";
					deathstring2 = "'s grenade\n";
				}
			}
			else if ( streq( targ->deathtype, "rocket" ) )
			{
				deathstring = ( targ->s.v.health < -40 ? " was gibbed by " : " rides " );
				deathstring2 = "'s rocket\n";

				if ( attacker->super_damage_finished > 0 && targ->s.v.health < -40 )
				{
					switch ( (int)(g_random() * 3) ) {
						case 0: deathstring = " was brutalized by "; break;
						case 1:	deathstring = " was smeared by "; break;
						default:
								G_bprint (PRINT_MEDIUM, "%s rips %s a new one\n",
											attacker->s.v.netname, targ->s.v.netname);
								return; // !!! return !!!
					}

					deathstring2 = "'s quad rocket\n";
				}
			}
			else if ( attacker->s.v.weapon == IT_AXE )
			{
				deathstring = " was ax-murdered by ";
			}
			else if ( attacker->s.v.weapon == IT_SHOTGUN )
			{
				deathstring = " chewed on ";
				deathstring2 = "'s boomstick\n";

				if ( targ->s.v.health < -40 )
				{
					deathstring = " was lead poisoned by ";
					deathstring2 = "\n";
				}
			}
			else if ( attacker->s.v.weapon == IT_SUPER_SHOTGUN )
			{
				if ( attacker->super_damage_finished > 0 )
					deathstring = " ate 8 loads of ";
				else
					deathstring = " ate 2 loads of ";

				deathstring2 = "'s buckshot\n";
			}
			else if ( attacker->s.v.weapon == IT_LIGHTNING )
			{
				deathstring = " accepts ";
				deathstring2 = "'s shaft\n";

 				if ( targ->s.v.health < -40 ) // quad modifier
				{
					deathstring = " gets a natural disaster from ";
					deathstring2 = "\n";
				}

				if ( attacker->s.v.waterlevel > 1 ) // ok - this was discharge
				{
					switch( (int)(g_random() * 2) ) {
						case 0: 
								 deathstring = " drains ";
								 deathstring2 = "'s batteries\n"; break;
						default:
								 deathstring = " accepts ";
								 deathstring2 = "'s discharge\n";
					}
				}
			}
			else {
				G_cprint ("Unknown death: normal death kt version");
				deathstring = " killed by ";
				deathstring2 = " ?\n";
			}

			G_bprint (PRINT_MEDIUM,"%s%s%s%s",
					targ->s.v.netname, deathstring, attacker->s.v.netname, deathstring2);
		}

		return;
	}
	else // attacker.classname != "player"
	{
		logfrag (targ, targ);
        targ->s.v.frags -= 1;            // killed self

		if ( streq( attacker->s.v.classname, "explo_box" ) )
		{
			deathstring = " blew up\n";
		}
        else if ( streq( targ->deathtype, "falling" ) )
		{
			if( g_random() < 0.5 )
				deathstring = " cratered\n";
			else
				deathstring = va(" fell to %s death\n", g_his(targ));
		}
        else if ( streq( targ->deathtype, "nail" ) || streq( targ->deathtype, "supernail" ) )
		{
               deathstring = " was spiked\n";
		}
		else if ( streq( targ->deathtype, "laser" ) )
		{
			deathstring = " was zapped\n";
		}
		else if ( streq( attacker->s.v.classname, "fireball" ) )
		{
			deathstring = " ate a lavaball\n";
		}
		else if ( streq( attacker->s.v.classname, "trigger_changelevel" ) )
		{
			deathstring = " tried to leave\n";
		}
		else if ( (int)attacker->s.v.flags & FL_MONSTER )
		{
//			deathstring = ObituaryForMonster( attacker->s.v.classname );
			deathstring = " killed by monster? :)\n";
		}
		else if ( targ->s.v.watertype == CONTENT_WATER )
		{
			switch( (int)(g_random() * 2) ) {
				case 0:  deathstring = " sleeps with the fishes\n"; break;
				default: deathstring = " sucks it down\n"; break;
			}
		}
		else if ( targ->s.v.watertype == CONTENT_SLIME )
		{
			switch( (int)(g_random() * 2) ) {
				case 0:  deathstring = " gulped a load of slime\n"; break;
				default: deathstring = " can't exist on slime alone\n"; break;
			}
		}
		else if ( targ->s.v.watertype == CONTENT_LAVA )
		{
			switch( (int)(g_random() * 2) ) {
				case 0:  deathstring = " turned into hot slag\n"; break;
				default: deathstring = " visits the Volcano God\n"; break;
			}

			if ( targ->s.v.health < -15 )
				deathstring = " burst into flames\n";
		}
		else
		{
			deathstring = " died\n";
		}

		G_bprint (PRINT_MEDIUM, "%s%s", targ->s.v.netname, deathstring );
	}
}

