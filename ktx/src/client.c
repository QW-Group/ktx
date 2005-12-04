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
 *  $Id: client.c,v 1.11 2005/12/04 17:14:26 qqshka Exp $
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


void EndMatch(float skip_log);
float CountALLPlayers();
void IdlebotCheck();
void CheckAll();
void CheckConnectRate();
void PlayerStats();
void PlayerDead();
void ExitCaptain();
void CheckFinishCaptain();
void AbortElect();
void MakeMOTD();
void ExitKick(gedict_t *kicker);
void play_teleport();
void ImpulseCommands();


void CheckAll ()
{
	float player_rate, maxrate=0, minrate=0;
	gedict_t *p;

	maxrate = iKey(world, "k_maxrate");
	minrate = iKey(world, "k_minrate");

	if( maxrate || minrate )
	{
		p = find(world, FOFCLSN, "player");
		while( p ) 
		{
			if( !strnull( p->s.v.netname ) ) 
			{
				// This is used to check a players rate.
				// If above allowed setting then it sets it to max allowed.
				player_rate = iKey( p, "rate" );
	      		if ( player_rate > maxrate )
				{
					G_sprint(p, 2, "\nYour עבפו setting is too high for this server.\n"
								   "Rate set to %d\n", (int)maxrate);
			
					stuffcmd(p, "rate %d\n", (int)maxrate );
				}

	      		if ( player_rate < minrate )
				{
					G_sprint(p, 2, "\nYour עבפו setting is too low for this server.\n"
								   "Rate set to %d\n", (int)minrate);
					stuffcmd(p, "rate %d\n", (int)minrate );
				}
			}

			p = find(p, FOFCLSN, "player");
		}
	}
}

void CheckConnectRate ()
{
    float player_rate, maxrate=0, minrate=0;

	// This is used to check a players rate.  If above allowed setting then it kicks em off.
	player_rate = iKey( self, "rate" );
	
	maxrate = iKey(world, "k_maxrate" );
	minrate = iKey(world, "k_minrate" );

	if( maxrate || minrate )
	{
	    if ( player_rate > maxrate )
		{
					G_sprint(self, 2, "\nYour עבפו setting is too high for this server.\n"
								   "Rate set to %d\n", (int)maxrate);
					stuffcmd(self, "rate %d\n", (int)maxrate );
		}

		if (player_rate < minrate)
		{
					G_sprint(self, 2, "\nYour עבפו setting is too low for this server.\n"
								   "Rate set to %d\n", (int)minrate);
					stuffcmd(self, "rate %d\n", (int)minrate );
		}
	}
}


void ModPause (int pause);

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
	float timing_players_time = bound(0, atoi( ezinfokey( world, "timing_players_time" ) ), 30);
	int timing_players_action = TA_ALL & atoi( ezinfokey( world, "timing_players_action" ) );
	int timed = 0;
	gedict_t *p;

	if ( !atoi( ezinfokey( world, "allow_timing" ) ) )
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

			if ( firstTime ) { // ok we are detect - player lagged, so do something
				if ( timing_players_action & TA_GLOW ) {
					p->k_timingEffects = ( int ) p->s.v.effects;
					p->s.v.effects = ( int ) p->s.v.effects | EF_DIMLIGHT;
				}

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

		p = find( p, FOFCLSN, "player" );
	}

	if ( lasttimed && !timed && k_pause == 2 )
		G_bprint(2, "You can vote now for %s\n", redtext( "unpause" ) );

	lasttimed = timed;
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
	g_globalvars.parm1 = 4096 | 1;
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
	g_globalvars.parm1 = 4096 | 1 | 2 | 4 | 8 | 32 | 16 | 64;
	g_globalvars.parm2 = 100;
	g_globalvars.parm3 = 0;
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
	// check, if matchless mode is active, set ingame params, we cant
	// use k_matchLess here beacuse it can be changed during game somehow (via direct server conlose etc)
	// If matchless mode is not active, set just ordinary prewar stats
	if( /* match_in_progress == 2 ||*/ cvar( "k_matchless" ) )
		InGameParams ();
    else 
		PrewarParams ();

    g_globalvars.parm9  = 0;

	g_globalvars.parm11 = self->k_admin;
	g_globalvars.parm12 = self->k_accepted;
	g_globalvars.parm13 = self->k_stuff;

//	G_bprint(2, "SCP: ad:%d\n", (int)self->k_admin);
}

// this called before player connected, so he get default params

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

//	G_bprint(2, "DLP2 ad:%d ac:%d s:%d\n", (int)g_globalvars.parm11, (int)g_globalvars.parm12, (int)g_globalvars.parm13);
}

/*
============
FindIntermission

Returns the entity to view from
============
*/
gedict_t       *FindIntermission()
{
	gedict_t       *spot;
	int             cyc;

// look for info_intermission first
	spot = find( world, FOFS( s.v.classname ), "info_intermission" );
	if ( spot )
	{			// pick a random one
		cyc = g_random() * 4;
		while ( cyc > 1 )
		{
			spot = find( spot, FOFS( s.v.classname ), "info_intermission" );
			if ( !spot )
				spot =
				    find( world, FOFS( s.v.classname ),
					  "info_intermission" );
			cyc = cyc - 1;
		}
		return spot;
	}

// then look for the start position
	spot = find( world, FOFS( s.v.classname ), "info_player_start" );
	if ( spot )
		return spot;

	G_Error( "FindIntermission: no spot" );
	return NULL;
}

void GotoNextMap()
{
	char            newmap[64];

#ifdef KTEAMS
	if( k_velect ) 
        AbortElect();
#endif

//ZOID: 12-13-96, samelevel is overloaded, only 1 works for same level

	if ( trap_cvar( "samelevel" ) == 1 )	// if samelevel is set, stay on same level
		trap_changelevel( g_globalvars.mapname );
	else
	{
		// configurable map lists, see if the current map exists as a
		// serverinfo/localinfo var
		infokey( world, g_globalvars.mapname, newmap, sizeof( newmap ) );

		if ( newmap[0] )
			trap_changelevel( newmap );
		else
			trap_changelevel( nextmap );
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
							max( 1, atoi( ezinfokey( world, "demo_scoreslength" ) ) );

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

#ifdef KTEAMS
// KTEAMS: make players invisible
        other->s.v.model = "";
		// take screenshot if requested
        if( atoi( ezinfokey( other, "k_flags" ) ) & 2 )
			stuffcmd(other, "wait; wait; wait; wait; wait; wait; screenshot\n");
#endif

		other = find( other, FOFS( s.v.classname ), "player" );
	}
}

void changelevel_touch()
{
	//gedict_t*    pos;

	if ( strneq( other->s.v.classname, "player" ) )
		return;

// if "noexit" is set, blow up the player trying to leave
//ZOID, 12-13-96, noexit isn't supported in QW.  Overload samelevel
//      if ((cvar("noexit") == 1) || ((cvar("noexit") == 2) && (mapname != "start")))
	if ( ( trap_cvar( "samelevel" ) == 2 )
	     || ( ( trap_cvar( "samelevel" ) == 3 )
		  && ( strneq( g_globalvars.mapname, "start" ) ) ) )
	{
		T_Damage( other, self, self, 50000 );
		return;
	}

	G_bprint( PRINT_HIGH, "%s exited the level\n", other->s.v.netname );

	strcpy( nextmap, self->map );
// nextmap = self.map;
	activator = other;
	SUB_UseTargets();

	self->s.v.touch = ( func_t ) SUB_Null;

// we can't move people right now, because touch functions are called
// in the middle of C movement code, so set a think g_globalvars.time to do it
	self->s.v.think = ( func_t ) execute_changelevel;
	self->s.v.nextthink = g_globalvars.time + 0.1;
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
	int             numspots;		// count of "valid" spots
	int				k_nspots;
	int				totalspots;
	int				pcount;
	int				k_spw = atoi( ezinfokey( world, "k_spw" ) );

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
					   // k_spw == 2 feature, this player will counted
					   // only in certain time after his last spawn (i.e. while k_1spawn != 0).
					   // k_1spawn is _also_ set after player passed teleport
					&& thing->s.v.health > 0
				  ) 
					pcount++;
			}

			thing = findradius( thing, spot->s.v.origin, 84 );
		}

		if( k_spw && match_in_progress == 2 && self->k_lastspawn == spot)
			pcount++; // ignore this spot in this case

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
	totalspots--;

	if ( !numspots )
	{
		// ack, they are all full, just pick one at random

//  bprint (PRINT_HIGH, "Ackk! All spots are full. Selecting random spawn spot\n");

		totalspots = g_random() * totalspots ;
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
				if( streq( thing->s.v.classname, "player" ) && thing->s.v.health > 0 ) {
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

// Generate a random number between 1 and numspots

	numspots--;

	numspots = g_random() * numspots;

	spot = spots;
	while ( numspots > 0 )
	{
		spot = PROG_TO_EDICT( spot->s.v.goalentity );
		numspots = numspots - 1;
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


//	if (deathmatch /*|| coop*/ )
//	G_bprint( PRINT_HIGH, "%s entered the game\n", self->s.v.netname );

// a client connecting during an intermission can cause problems
// 	if ( intermission_running )
//		GotoNextMap ();

	// if the guy started connecting during intermission and
	// thus missed the svc_intermission, we'd better let him know
	if ( intermission_running )
		SendIntermissionToClient ();


#ifdef KTEAMS

// ILLEGALFPS[
	// delay on checking/displaying illegal FPS.
	self->fAverageFrameTime = 0;
	self->fFrameCount = 0;
	self->fDisplayIllegalFPS = g_globalvars.time + 10 + g_random() * 5;
	self->fLowestFrameTime = 0.013; // 1/72
	self->fIllegalFPSWarnings = 0;
// ILLEGALFPS]

	self->fraggie = 0;
	MakeMOTD();
	CheckConnectRate();

	if( k_captains == 2 ) {
// in case of team picking, pick player if there's a captain with his/her team
		p = find(world, FOFCLSN, "player");
		while( p && !f1 ) {

			while( p && !p->k_captain )
				p = find(p, FOFCLSN, "player");

			if( p && p->k_captain ) {

				tmp = ezinfokey(p, "team");

				if( streq( tmp, ezinfokey(self, "team") ) ) {
					G_sprint(self, 2, "Team picking in progress\n");
					G_bprint(2, "%s is set to team ע%sף\n", self->s.v.netname, tmp);
					// set color, team is already set %)
					stuffcmd(self, "color %d %d\n",
						atoi(ezinfokey(p, "topcolor")), atoi(ezinfokey(p, "bottomcolor")));
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
				p = find(world, FOFCLSN, "player");

				while( p && p->s.v.frags != f1 )
					p = find(p, FOFCLSN, "player");
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
	char            s[20];

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

#ifdef KTEAMS
    if (  1 /* deathmatch */ /*|| coop FIXME: remove??? */)
    {

	// FIXME: stupid way

		// do not accept player ez only in case of match.
		// player may be kicked in MOTD stuff while pass some checks.
		// if not kicked, player will be "spawned" in PlayerPostThink(),
		// just look code -> if(self->k_accepted == 1)

		if( !self->k_accepted && match_in_progress == 2 ) {
			self->s.v.classname = "player_na"; // player not accepted
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
			// FIXME:
			//        play_teleport();
			spawn_tdeath( self->s.v.origin, self );

			//berzerk will not affect players that logs in during berzerk
			//spawn666 will not affect the first spawn of players connecting to a game in progress
			if( match_in_progress == 2 ) {
				if( atoi( ezinfokey( world, "k_bzk" ) ) && k_berzerk ) {
					self->s.v.items = (int)self->s.v.items | 4194304;
					self->super_time = 1;
					self->super_damage_finished = g_globalvars.time + 3600;
				}

				if( atoi( ezinfokey( world, "k_666" ) ) ) {
					stuffcmd (self, "bf\n");
					self->invincible_time = 1;
					self->invincible_finished = g_globalvars.time + 2;
					self->k_666 = 1;
					self->s.v.items = (int)self->s.v.items | 1048576;
				}
			}


		}
	}
#endif

//	makevectors( self->s.v.angles );
//	VectorScale( g_globalvars.v_forward, 20, v );
//	VectorAdd( v, self->s.v.origin, v );
//	spawn_tfog( v );

//	spawn_tdeath( self->s.v.origin, self );

	// Set Rocket Jump Modifiers
	infokey( world, "rj", s, sizeof( s ) );
	if ( atof( s ) != 0 ) // can be float, so atof
	{
		rj = atof( s );
	}


	if ( deathmatch == 4 && match_in_progress == 2 )
	{
		self->s.v.ammo_shells = 0;
		infokey( world, "axe", s, sizeof( s ) );
		if ( !atoi( s ) )
		{
			self->s.v.ammo_nails   = 255;
			self->s.v.ammo_shells  = 255;
			self->s.v.ammo_rockets = 255;
			self->s.v.ammo_cells   = 255;
			self->s.v.items = ( int ) self->s.v.items | IT_NAILGUN;
			self->s.v.items = ( int ) self->s.v.items | IT_SUPER_NAILGUN;
			self->s.v.items = ( int ) self->s.v.items | IT_SUPER_SHOTGUN;
			self->s.v.items = ( int ) self->s.v.items | IT_ROCKET_LAUNCHER;
//  self.items = self.items | IT_GRENADE_LAUNCHER;
			self->s.v.items = ( int ) self->s.v.items | IT_LIGHTNING;
		}
		self->s.v.items -=
		    ( ( int ) self->s.v.items & ( IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3 ) ) -
		    IT_ARMOR3;
		self->s.v.armorvalue = 200;
		self->s.v.armortype = 0.8;
		self->s.v.health = 250;
		self->s.v.items = ( int ) self->s.v.items | IT_INVULNERABILITY;
		self->invincible_time = 1;
		self->invincible_finished = g_globalvars.time + 3;
	}


	if (deathmatch == 5 && match_in_progress == 2)
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
		self->s.v.items = items | IT_INVULNERABILITY;
		self->invincible_time = 1;
		self->invincible_finished = g_globalvars.time + 3;
	}
}

/*
===============================================================================

RULES

===============================================================================
*/

/*
go to the next level for deathmatch
*/
void NextLevel()
{
	gedict_t       *o;

	if (  nextmap[0] )
		return;		// already done

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
		if ( !o || streq( g_globalvars.mapname, "start" ) )
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
}

/*
============
CheckRules

Exit deathmatch games upon conditions
============
*/
void CheckRules()
{
    if ( fraglimit && self->s.v.frags >= fraglimit )
#ifdef KTEAMS
        EndMatch( 0 );
#else
        NextLevel ();
#endif
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

// check the flag and jump if we can
    if ( !self->brokenankle )
	{
        self->s.v.button2 = 0;

		// player jumping sound
		sound( self, CHAN_BODY, "player/plyrjmp8.wav", 1, ATTN_NORM );

#ifdef KTEAMS
		if ( server_is_2_3x ) {
	        // if and only if running on a 2.30 or 2.33 server,
	        // fix the jump bug via QC
	        // newer servers should have an engine-side fix
	        if (self->s.v.velocity[2] < 0 )
	            self->s.v.velocity[2] = 0;
		}
#endif
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

	if ( self->s.v.health < 0 )
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


////////////////
// GlobalParams:
// self
///////////////
void ClientDisconnect()
{
	gedict_t *ghost;
	float f1, f2;

	if( match_in_progress == 2 && self->k_makeghost ) 
	{
		G_bprint( PRINT_HIGH, "%s left the game with %.0f frags\n", self->s.v.netname,
	  					self->s.v.frags );

		sound( self, CHAN_BODY, "player/tornoff2.wav", 1, ATTN_NONE );

		f1 = 1;
		f2 = 0;

		while( f1 < k_userid && !f2 ) {
			if( strnull( ezinfokey(world, va("%d", (int)f1) ) ) )
				f2 = 1;
			else
				f1++;
		}

		if( !f2 )
			k_userid++;

		ghost = spawn();
		ghost->s.v.owner = EDICT_TO_PROG( world );
		ghost->s.v.classname = "ghost";
		ghost->cnt2      = f1;
		ghost->k_teamnum = self->k_teamnum;
		ghost->s.v.frags = self->s.v.frags;
		ghost->deaths    = self->deaths;
		ghost->friendly  = self->friendly;
		ghost->ready     = 0;

		ghost->ps        = self->ps; // save player stats

		localcmd("localinfo %d \"%s\"\n", (int)f1, self->s.v.netname);
		trap_executecmd();
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

	self->k_accepted = 0;
	self->ready = 0;
	self->s.v.classname = "";


// s: added conditional function call here
	if( self->k_kicking )
		ExitKick( self );

	if( self->k_captain ) {
        G_bprint(2, "A דבנפבימ has left\n");

		ExitCaptain();
	}

	if( k_captains == 2 )
		CheckFinishCaptain();

	if( atoi( ezinfokey( world, "k_idletime" ) ) && !k_force && !match_in_progress )
		IdlebotCheck();

	f1 = CountALLPlayers();

	if( !f1
		&& (
			 (    !atoi( ezinfokey( world, "k_master" ) )
			   && !atoi( ezinfokey( world, "k_lockmap" ) ) 
			 )
			 || ( !atoi( ezinfokey( world, "lock_practice" ) ) && k_practice )
	       )
	  )
	{
		char *s;
		int old_k_practice = k_practice;

		if ( k_pause ) {
			G_bprint(2, "No players left, unpausing.\n");
			ModPause ( 0 ); // no players left, unpause server if paused
		}

		if ( !atoi( ezinfokey( world, "lock_practice" ) ) && k_practice )  // #practice mode#
			SetPractice( 0, NULL ); // return server to normal mode but not reload map yet

		if( match_in_progress )
			EndMatch( 1 ); // skip demo, make some other stuff

		// Check if issued to execute reset.cfg (sturm)
        if( atoi( ezinfokey( world, "k_autoreset" ) ) )
			localcmd("exec configs/reset.cfg\n");

        s = k_matchLess ? "" : ezinfokey( world, "k_defmap" ); // no defmap in matchLess mode

		// force reload current map in practice mode even k_defmap is set or not
        if ( !atoi( ezinfokey( world, "lock_practice" ) ) && old_k_practice )
			s = g_globalvars.mapname; // FIXME: k_defmap may not exist on server disk, so we reload current map
									  //        but we may check if k_defmap exist and reload to it, right?

        if( !strnull( s ) )
            localcmd("map %s\n", s);
	}
}

void BackFromLag()
{
	int timing_players_action = TA_ALL & atoi( ezinfokey( world, "timing_players_action" ) );

	self->k_timingWarnTime = 0;

	if ( timing_players_action & TA_INFO )
		G_bprint(2, "%s %s\n", self->s.v.netname, redtext( "is back from lag") );

	if ( timing_players_action & TA_GLOW )
		self->s.v.effects = self->k_timingEffects;

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
	int _wps = S_ALL & atoi( ezinfokey ( self, "wps" ) );
	int  wps = ( _wps ? _wps : S_DEF ); // if wps is not set - show S_DEF weapons

	gedict_t *e = self; // stats of whom we want to show

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
	float rl  = wps & S_RL  ? e->ps.h_rl : 0;
#endif
	float lg  = wps & S_LG  ? 100.0 * e->ps.h_lg  / max(1, e->ps.a_lg) : 0;


	if ( !axe && !sg && !ssg && !ng && !sng && !gl && !rl && !lg )
		return; // sanity

	i = bound(0, atoi ( ezinfokey( self, "lw" ) ), sizeof(buf)-1 );
	memset( (void*)buf, (int)'\n', i);
	buf[i] = 0;

	if ( rl || lg || gl ) {
		if ( rl )
			strlcat(buf, (rl  ? va("%s%s:%.0f", (*buf ? " " : ""), redtext("rl"),  rl) : ""), sizeof(buf));
		if ( lg )
			strlcat(buf, (lg  ? va("%s%s:%.1f", (*buf ? " " : ""), redtext("lg") , lg) : ""), sizeof(buf));
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

	if ( strnull( buf ) )
		return; // sanity

	self->need_clearCP  = 1;
	self->wp_stats_time = g_globalvars.time + 0.8;

	G_centerprint( self, "%s",  buf );
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
//  float   mspeed, aspeed;
//  float   r;
#ifdef KTEAMS
	float   r;

	if ( self->k_timingWarnTime )
		BackFromLag();

	if ( self->wp_stats && self->wp_stats_time && self->wp_stats_time <= g_globalvars.time )
		Print_Wp_Stats ();

// ILLEGALFPS[

	self->fAverageFrameTime += g_globalvars.frametime;
	self->fFrameCount += 1;

	if( g_globalvars.frametime < self->fLowestFrameTime )
		self->fLowestFrameTime = g_globalvars.frametime;
	
	if( self->fDisplayIllegalFPS < g_globalvars.time && framechecks )
	{
		float fps;

// client uptime check
// code by Zibbo
		r = self->fAverageFrameTime * 100 / (g_globalvars.time - self->real_time);
		self->real_time = g_globalvars.time;

		if(r > 103 && !match_in_progress) {
			G_sprint(self, PRINT_HIGH, 
				"WARNING: QW clients up to 2.30 have a timer related bug which is caused by too"
				" long uptime. Either reboot your machine or upgrade to QWCL 2.33.\n");
			G_dprint("%s%%speed%%%f\n", self->s.v.netname, r);
			if(r > 105)
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

// delay on checking/displaying illegal FPS.
// s: changed to 15 for more accurate calculation (lag screws it up)
		self->fDisplayIllegalFPS = g_globalvars.time + 15;
		
		fps = floor( 72 * 13 / ( self->fAverageFrameTime / self->fFrameCount * 1000 ) );
		
		if( fps > current_maxfps )
		{
			float peak;

			G_bprint( PRINT_HIGH,
				"\nWARNING: %s is using the timedemo bug and has abnormally high frame rates, ",
							self->s.v.netname);
							
			peak = floor( 72 * 13 / ( self->fLowestFrameTime * 1000 ) );
			G_bprint( PRINT_HIGH, "highest FPS = %3.1f (frametime was ", peak);
			G_bprint( PRINT_HIGH, "%3.1f), ", self->fLowestFrameTime * 1000);
			G_bprint( PRINT_HIGH, "average FPS = %3.1f!\n", fps);
			
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
		
		// zero these so the average/highest FPS is calculated for each delay period.
		self->fAverageFrameTime = 0;
		self->fFrameCount = 0;
		self->fLowestFrameTime = 0.013; // 1/72 ?
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
        if( g_globalvars.time > (self->dead_time + 2) )
            PlayerDead (); // so he can respawn

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
	if ( self->s.v.health <= 0 )
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
#ifdef KTEAMS
		if(self->invincible_finished < g_globalvars.time + 3 && !self->k_666)	//team
#else
		if(self->invincible_finished < g_globalvars.time + 3)
#endif
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
#ifdef KTEAMS
			self->k_666 = 0;		//team
#endif
		}

#ifdef KTEAMS
		if(self->invincible_finished > g_globalvars.time && !self->k_666) // KTeAMS
#else
		if ( self->invincible_finished > g_globalvars.time )
#endif
		{
			self->s.v.effects = ( int ) self->s.v.effects | EF_DIMLIGHT;
			self->s.v.effects = ( int ) self->s.v.effects | EF_RED;
		}
		else
		{
			self->s.v.effects -= ( ( int ) self->s.v.effects & EF_DIMLIGHT );
			self->s.v.effects -= ( ( int ) self->s.v.effects & EF_RED );
		}
	}
// super damage
	if ( self->super_damage_finished )
	{
// sound and screen flash when items starts to run out
#ifdef KTEAMS
		if(self->super_damage_finished < g_globalvars.time + 3 && !k_berzerk)	//team
#else
		if ( self->super_damage_finished < g_globalvars.time + 3 )
#endif
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

#ifdef KTEAMS
        if(self->super_damage_finished < g_globalvars.time && !k_berzerk)	//team
#else
		if ( self->super_damage_finished < g_globalvars.time )
#endif
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
			self->s.v.effects = ( int ) self->s.v.effects | EF_DIMLIGHT;
			self->s.v.effects = ( int ) self->s.v.effects | EF_BLUE;
		}
		else
		{
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
	if (self->shownick_time && self->shownick_time <= g_globalvars.time )
		self->shownick_time = 0;
	if (self->wp_stats_time && self->wp_stats_time <= g_globalvars.time )
		self->wp_stats_time = 0;

	if ( self->need_clearCP && !self->shownick_time && !self->wp_stats_time )
	{
		self->need_clearCP = 0;
		G_centerprint(self, ""); // clear center print
	}

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
			// FIXME:
//			play_teleport();
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
           	if( !atoi( ezinfokey(world, "k_fallbunny" ) ) && self->s.v.waterlevel < 2 )
               	self->brokenankle = 1;  // Yes we have just broken it

			self->deathtype = "falling";
			T_Damage( self, world, world, 5 );
			sound( self, CHAN_VOICE, "player/land2.wav", 1, ATTN_NORM );

			if ( gre && gre->s.v.takedamage == DAMAGE_AIM )
			{
				// we landed on someone's head, hurt him
				PROG_TO_EDICT ( self->s.v.groundentity )->deathtype = "stomp";
				T_Damage (gre, self, self, 10);
			}
		} else
			sound( self, CHAN_VOICE, "player/land.wav", 1, ATTN_NORM );
	}

	self->jump_flag = self->s.v.velocity[2];

	CheckPowerups();

	W_WeaponFrame();
}

/*
===========
ClientObituary

called when a player dies
============
*/

void ClientObituary (gedict_t *targ, gedict_t *attacker)
{
	float rnum;
	char *deathstring,  *deathstring2;
	char *attackerteam, *targteam;

#ifdef KTEAMS
	char *attackerteam2;	//team

	// Set it so it should update scores at next attempt.
	k_nochange = 0;
#endif

    if ( strneq( targ->s.v.classname, "player" ))
		return;
    
	rnum = g_random();
	//ZOID 12-13-96: self.team doesn't work in QW.  Use keys
	attackerteam = ezinfokey(attacker, "team");
	targteam     = ezinfokey(targ,     "team");

    if ( deathmatch > 3 )
    {
        if ( streq( targ->deathtype, "selfwater" ) )
        {
#ifdef KTEAMS
            if( streq( ezinfokey( targ, "gender" ), "f" )) 
                G_bprint (PRINT_MEDIUM, "%s electrocutes herself.\n ", targ->s.v.netname);
            else 
#endif
                G_bprint (PRINT_MEDIUM, "%s electrocutes himself.\n ", targ->s.v.netname);

            targ->s.v.frags -=  1;
            return;
        }
    }

    if ( streq( attacker->s.v.classname, "teledeath" ) )
	{
#ifdef KTEAMS
			if( match_in_progress != 2 ) {
				G_bprint(PRINT_MEDIUM, "%s was telefragged\n", targ->s.v.netname);
				return;
			}

			attackerteam2 = ezinfokey( PROG_TO_EDICT( attacker->s.v.owner ), "team");

			if( teamplay && streq( targteam, attackerteam2 ) && !strnull ( attackerteam2 )
					&& targ != PROG_TO_EDICT( attacker->s.v.owner ) )
			{
				G_bprint(PRINT_MEDIUM, targ->s.v.netname);
				if( streq( ezinfokey( targ, "gender" ), "f" ) ) 
                    G_bprint (PRINT_MEDIUM," was telefragged by her teammate\n");
				else 
                    G_bprint(PRINT_MEDIUM," was telefragged by his teammate\n");
				targ->deaths += 1;
				return;
			}
#endif
            G_bprint (PRINT_MEDIUM, "%s was telefragged by %s\n",
				targ->s.v.netname, PROG_TO_EDICT( attacker->s.v.owner )->s.v.netname );

            logfrag (PROG_TO_EDICT( attacker->s.v.owner ), targ);

            PROG_TO_EDICT( attacker->s.v.owner )->s.v.frags += 1;
#ifdef KTEAMS
			targ->deaths += 1;
			PROG_TO_EDICT( attacker->s.v.owner )->victim = targ->s.v.netname;
			targ->killer = PROG_TO_EDICT( attacker->s.v.owner )->s.v.netname;
#endif
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
#ifdef KTEAMS
			attacker->friendly += 1;		//team
#endif
            G_bprint (PRINT_MEDIUM, "%s squished a teammate\n", attacker->s.v.netname);
			return;
		}
		else if ( streq( attacker->s.v.classname, "player" ) && attacker != targ )
		{
			G_bprint (PRINT_MEDIUM, "%s squishes %s\n", attacker->s.v.netname, targ->s.v.netname);
			logfrag (attacker, targ);
			attacker->s.v.frags += 1;
#ifdef KTEAMS
			targ->deaths += 1;
			attacker->victim = targ->s.v.netname;
			targ->killer = attacker->s.v.netname;
#endif
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

    if ( streq( attacker->s.v.classname, "player" ) ) 
    {
#ifdef KTEAMS
            // included big part starts to handle optional new death messages :p
			if( atoi( ezinfokey( world, "k_deathmsg" ) ) ) {
#endif
				if ( targ == attacker )
				{
					// killed self, KTeams version
					logfrag (attacker, attacker);
					attacker->s.v.frags -= 1;

					if (targ->deathtype == "grenade")
                        deathstring = " tries to put the pin back in\n";
					else if (targ->deathtype == "rocket")
					{
#ifdef KTEAMS
						if (rnum < 0.5)
                            deathstring = " discovers blast radius\n";
						else
#endif
                            deathstring = " becomes bored with life\n";
					}
					else if (targ->s.v.weapon == 64 && targ->s.v.waterlevel > 1)
					{
                        if (targ->s.v.watertype == CONTENT_SLIME)
                            deathstring = " discharges into the slime\n";
                        else if (targ->s.v.watertype == CONTENT_LAVA)
                            deathstring = " discharges into the lava\n";
#ifdef KTEAMS
						else if (g_random() < 0.5)
                            deathstring = " heats up the water.\n";
#endif
						else
                            deathstring = " discharges into the water.\n";
					}
					else
                        deathstring = " becomes bored with life.\n"; // hm, and how it is possible?

					G_bprint (PRINT_MEDIUM, "%s%s", targ->s.v.netname, deathstring);
                    return;
				}
                else if ( ( teamplay == 2 && streq( targteam, attackerteam ) &&
                            !strnull( attackerteam) ) /* || coop */ )
				{
					// teamkill, KTeams version
//                    if (coop)
//                        rnum = rnum * 0.5;	// only use the first two messages
					if( rnum < 0.25 ) 
					{
#ifdef KTEAMS
                        if( streq( ezinfokey( PROG_TO_EDICT( attacker->s.v.owner ), "gender") , "f" ) ) 
                            deathstring = " checks her glasses\n";
						else 
#endif
                            deathstring = " checks his glasses\n";
					}
					else if( rnum < 0.5 ) 
						deathstring = " loses another friend\n";
					else if ( rnum < 0.75 )
						deathstring = " gets a frag for the other team\n";
					else
                        deathstring = " mows down a teammate\n";
					G_bprint (PRINT_MEDIUM, "%s%s", attacker->s.v.netname, deathstring);
					attacker->s.v.frags -= 1;
#ifdef KTEAMS
					attacker->friendly += 1;
#endif
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

					rnum = attacker->s.v.weapon;

					if ( streq( targ->deathtype, "nail" ) )
					{
						if ( g_random() < 0.5 )
							deathstring = " was body pierced by ";
						else
							deathstring = " was nailed by ";
						deathstring2 = "\n";
					}
					else if ( streq( targ->deathtype, "supernail" ) )
					{
						rnum = g_random();
						if ( targ->s.v.health < -40 )
							deathstring = " was straw-cuttered by ";
						else if ( rnum < 0.33 )
							deathstring = " was punctured by ";
						else if ( rnum < 0.66 )
							deathstring = " was perforated by ";
						else
							deathstring = " was ventilated by ";
						deathstring2 = "\n";
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
						if (attacker->super_damage_finished > 0 && targ->s.v.health < -40)
						{
							rnum = g_random();
							if ( rnum < 0.33 )
								deathstring = " was brutalized by ";
							else if ( rnum < 0.66 )
								deathstring = " was smeared by ";
							else
							{
								G_bprint (PRINT_MEDIUM, "%s rips %s a new one\n",
											attacker->s.v.netname, targ->s.v.netname);
								return;
							}
							deathstring2 = "'s quad rocket\n";
						}
						else
						{
							if (targ->s.v.health < -40)
								deathstring = " was gibbed by ";
							else
								deathstring = " rides ";
							deathstring2 = "'s rocket\n";
						}
					}
					else if ( streq( targ->deathtype, "stomp" ) )
					{
						deathstring = " was stomped by ";
						deathstring2 = "\n";
					}
					else if ( rnum == 4096 )
					{
						deathstring = " was ax-murdered by ";
						deathstring2 = "\n";
					}
					else if ( rnum == 1 )
					{
						if (targ->s.v.health < -40)
						{
							deathstring = " was lead poisoned by ";
							deathstring2 = "\n";
						}
						else
						{
							deathstring = " chewed on ";
							deathstring2 = "'s boomstick\n";
						}
					}
					else if ( rnum == 2 )
					{
						if ( attacker->super_damage_finished > 0 )
							deathstring = " ate 8 loads of ";
						else
							deathstring = " ate 2 loads of ";
						deathstring2 = "'s buckshot\n";
					}
					else if ( rnum == 64 )
					{
						deathstring = " accepts ";
						deathstring2 = "'s shaft\n";

						if ( attacker->s.v.waterlevel > 1 )
						{
							if ( g_random() < 0.5 )
							{
								deathstring = " drains ";
								deathstring2 = "'s batteries\n";
							}
							else
								deathstring2 = "'s discharge\n";
						}
						else if ( targ->s.v.health < -40 )
						{
							deathstring = " gets a natural disaster from ";
							deathstring2 = "\n";
						}

					}
					else {
						G_dprint ("Unknown death: normal death kt version");
						deathstring = " killed by ";
						deathstring2 = " ?\n";
					}

					G_bprint (PRINT_MEDIUM,"%s%s%s%s",
							targ->s.v.netname, deathstring, attacker->s.v.netname, deathstring2);
				}

				return;
// included big part ends

			} else { // !atoi( ezinfokey( world, "k_deathmsg" ) )
				if ( targ == attacker )
				{
					// killed self, native qw version
					logfrag (attacker, attacker);
					attacker->s.v.frags -= 1;
					G_bprint (PRINT_MEDIUM, targ->s.v.netname);

					if ( streq( targ->deathtype, "grenade" ) )
						G_bprint (PRINT_MEDIUM," tries to put the pin back in\n");
					else if ( streq( targ->deathtype, "rocket" ) )
						G_bprint (PRINT_MEDIUM," becomes bored with life\n");
					else if ( targ->s.v.weapon == 64 && targ->s.v.waterlevel > 1 )

					{
						if ( targ->s.v.watertype == -4 )
							G_bprint (PRINT_MEDIUM," discharges into the slime\n");
						else if ( targ->s.v.watertype == -5 )
							G_bprint (PRINT_MEDIUM," discharges into the lava\n");
						else
        					G_bprint (PRINT_MEDIUM," discharges into the water.\n");
					}
                    else
						G_bprint (PRINT_MEDIUM," becomes bored with life\n");

					return;
                }
				else if ( teamplay == 2 && streq( targteam, attackerteam )
							 && !strnull( attackerteam ) )
				{
					// teamkill, native qw version

///team
//0.25 changed to 0.33 and 0.50 changed to 0.66 for kombat teams
					if( rnum < 0.33 )
						deathstring = " mows down a teammate\n";
//				else if(rnum < 0.66) if(ezinfokey(attacker.owner,"gender") == "f") deathstring = " checks her glasses\n";
					else if( rnum < 0.66 )
					{
					 	if( streq( ezinfokey(attacker, "gender"), "f" ) )
							deathstring = " checks her glasses\n";
						else
							deathstring = " checks his glasses\n";
// shite message - removed by kombat teams
//                                else if (rnum < 0.75)
//					deathstring = " gets a frag for the other team\n";
// removed by kombat teams
///team
					}
					else
						deathstring = " loses another friend\n";

					G_bprint (PRINT_MEDIUM, "%s%s", attacker->s.v.netname, deathstring);

					attacker->s.v.frags -= 1;
					attacker->friendly += 1;		//team
					//ZOID 12-13-96:  killing a teammate logs as suicide
					logfrag (attacker, attacker);
					return;
				}
				else
				{
					// normal kill, native qw version
					logfrag (attacker, targ);
					attacker->s.v.frags += 1;
					targ->deaths += 1;		//team

					attacker->victim = targ->s.v.netname;
					targ->killer = attacker->s.v.netname;

					if ( targ->spawn_time + 1 > g_globalvars.time )
						attacker->ps.spawn_frags++;

	   				rnum = attacker->s.v.weapon;

					if ( streq( targ->deathtype, "nail" ) )
					{
						deathstring = " was nailed by ";
						deathstring2 = "\n";
					}
					else if ( streq( targ->deathtype, "supernail" ) )
					{
						deathstring = " was punctured by ";
						deathstring2 = "\n";
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
					else if ( streq( targ->deathtype, "rocket") )
					{
						if ( attacker->super_damage_finished > 0 && targ->s.v.health < -40 )
						{
							rnum = g_random();
							if ( rnum < 0.3 )
								deathstring = " was brutalized by ";
							else if ( rnum < 0.6 )
								deathstring = " was smeared by ";
							else
							{
								G_bprint (PRINT_MEDIUM, "%s rips %s a new one\n",
											attacker->s.v.netname, targ->s.v.netname);
								return;
							}

       						deathstring2 = "'s quad rocket\n";
						}
						else
                        {
							deathstring = " rides ";
							deathstring2 = "'s rocket\n";
							if ( targ->s.v.health < -40 )
	                                                {
								deathstring = " was gibbed by ";
								deathstring2 = "'s rocket\n" ;
							}
						}
					}
					else if ( streq( targ->deathtype, "stomp" ) )
					{
						deathstring = " was stomped by ";
						deathstring2 = "\n";
					}
					else if ( rnum == IT_AXE )
					{
						deathstring = " was ax-murdered by ";
						deathstring2 = "\n";
					}
					else if ( rnum == IT_SHOTGUN )
					{
						deathstring = " chewed on ";
						deathstring2 = "'s boomstick\n";
					}
					else if ( rnum == IT_SUPER_SHOTGUN )
					{
						deathstring = " ate 2 loads of ";
						deathstring2 = "'s buckshot\n";
					}
					else if ( rnum == IT_LIGHTNING )
					{
						deathstring = " accepts ";
						if (attacker->s.v.waterlevel > 1)
							deathstring2 = "'s discharge\n";
						else
							deathstring2 = "'s shaft\n";
					}
					else {
						G_dprint ("Unknown death: normal death qw native version");
						deathstring = " killed by ";
						deathstring2 = " ?\n";
					}

					G_bprint (PRINT_MEDIUM,"%s%s%s%s",
							targ->s.v.netname, deathstring, attacker->s.v.netname, deathstring2);
				}
				return;
			}
	}
	else // attacker.classname != "player"
	{
		logfrag (targ, targ);
        targ->s.v.frags -= 1;            // killed self

		rnum = targ->s.v.watertype;

		if ( rnum == -3 )
		{
			if ( g_random() < 0.5 )
				deathstring = " sleeps with the fishes\n";
			else
				deathstring = " sucks it down\n";
		}
		else if ( rnum == -4 )
		{
			if ( g_random() < 0.5 )
				deathstring = " gulped a load of slime\n";
			else
				deathstring = " can't exist on slime alone\n";
		}
		else if ( rnum == -5 )
		{
			if ( targ->s.v.health < -15 )
				deathstring = " burst into flames\n";
			else if ( g_random() < 0.5 )
				deathstring = " turned into hot slag\n";
			else
				deathstring = " visits the Volcano God\n";
		}
		else if ( streq( attacker->s.v.classname, "explo_box" ) )
		{
			deathstring = " blew up\n";
		}
        else if ( streq( targ->deathtype, "falling" ) )
		{
#ifdef KTEAMS
				rnum = atoi( ezinfokey( world, "k_deathmsg" ) );
				if( g_random() < 0.5 && rnum )
					deathstring = " cratered\n";
				else {
					if( streq( ezinfokey( targ, "gender" ), "f" ) ) 
                        deathstring = " fell to her death\n";
					else 
#endif
                       deathstring = " fell to his death\n";
				}
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
		else
		{
			deathstring = " died\n";
		}

		G_bprint (PRINT_MEDIUM, "%s%s", targ->s.v.netname, deathstring );
	}
}

