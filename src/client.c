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
int             modelindex_eyes, modelindex_player, vwep_index;

qboolean can_prewar( qboolean fire );
void IdlebotCheck();
void CheckAll();
void PlayerStats();
void ExitCaptain();
void CheckFinishCaptain();
void MakeMOTD();
void play_teleport( gedict_t *sndspot );
void ImpulseCommands();
void StartDie ();
void ZeroFpsStats ();

void del_from_specs_favourites(gedict_t *rm);

void CheckAll ()
{
	static float next_check = -1;
	gedict_t *p;

	if ( next_check > g_globalvars.time )
		return;

	next_check = g_globalvars.time + 20;

	for ( p = world; (p = find_client( p )); )
		CheckRate( p, "" );
}

qboolean CheckRate (gedict_t *p, char *newrate)
{
	qboolean ret = false;
    float player_rate, maxrate=0, minrate=0;

	// This is used to check a players rate.  If above allowed setting then it kicks em off.
	player_rate = atof( strnull(newrate) ? (newrate = ezinfokey(p, "rate" )) : newrate );

	if ( strnull(newrate) )
		return false; // empty rate is special, with new mvdsv mean maximum allowed, so allow it

	maxrate = cvar( "sv_maxrate" );
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

	for( p = world; (p = find_plr( p )); ) {
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

#ifndef NO_K_PAUSE
				if ( timing_players_action & TA_KPAUSE ) {
					if ( !k_pause ) {
						G_bprint(2, "Server issues a pause\n");
						ModPause ( 2 );
					}
				}
#endif
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
			if ( !p->invincible_finished && !p->super_damage_finished && !(p->ctf_flag & CTF_FLAG) )
				p->s.v.effects = ( int ) p->s.v.effects & ~EF_DIMLIGHT;
		}
	}

	if ( lasttimed && !timed && k_pause == 2 )
		G_bprint(2, "You can vote now for %s\n", redtext( "unpause" ) );

	lasttimed = timed;
}


void Check_sready()
{
	int k_sready = cvar( "k_sready" );
	gedict_t *p;

	if ( match_in_progress == 2 || match_over || k_matchLess )
		return;

	for( p = world; (p = find_plr( p )); ) {
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

	if ( isCTF() && cvar("k_ctf_hook") )
		g_globalvars.parm1 = (int)g_globalvars.parm1 | IT_HOOK;

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

	if ( isCTF() && cvar("k_ctf_hook") )
		g_globalvars.parm1 = (int)g_globalvars.parm1 | IT_HOOK;

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
	// we must use k_matchless cvar here because it can be changed during game somehow (via direct server conlose etc)
	// If matchless mode is not active, set just ordinary prewar stats
	if( /* match_in_progress == 2 ||*/ cvar( "k_matchless" ) )
		InGameParams ();
    else 
		PrewarParams ();

    g_globalvars.parm9  = 0;

	g_globalvars.parm11 = self->k_admin;
//	g_globalvars.parm12 = self->k_accepted;
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
//	g_globalvars.parm12 = from_vmMain ? 0 : self->k_accepted;
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
	// remove any ctf items from previous level
	self->ctf_flag = 0;
       
//	G_bprint(2, "DLP1 ad:%d ac:%d s:%d\n", (int)self->k_admin, (int)self->k_accepted, (int)self->k_stuff);

	if ( g_globalvars.parm11 )
		self->k_admin = g_globalvars.parm11;

//    if( g_globalvars.parm12 )
//        self->k_accepted = g_globalvars.parm12;
	
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

	if ( (spot = Do_FindIntermission( "info_intermission" )) )
		return spot;

	if ( (spot = Do_FindIntermission( "info_player_start" )) )
		return spot;

	if ( (spot = Do_FindIntermission( "info_player_deathmatch" )) )
		return spot;

	if ( (spot = Do_FindIntermission( "info_player_coop" )) )
		return spot;

	if ( (spot = Do_FindIntermission( "info_player_start2" )) )
		return spot;

//	G_Error( "FindIntermission: no spot" );
//	return NULL;
	return world;
}

void GotoNextMap()
{
	extern  char *SelectMapInCycle(char *buf, int buf_size);

	char	newmap[64] = {0};

	if ( trap_cvar( "samelevel" ) /* == 1 */ )	// if samelevel is set, stay on same level
		changelevel( g_globalvars.mapname );
	else
	{
		SelectMapInCycle(newmap, sizeof(newmap));

		if ( !nextmap[0] ) // so we can reload current map at least
			strlcpy( nextmap, g_globalvars.mapname, sizeof(nextmap) );

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
	intermission_exittime = g_globalvars.time + 1 +	max( 1, cvar( "demo_scoreslength" ) );

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

	for( other = world; (other = find_plr( other )); )
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
	}
}

void changelevel_touch()
{
	if ( other->ct != ctPlayer )
		return;

// qqshka: does't change level in any case, just do damage and return

	if ( !isCTF() ) { // ctf has always allowed players to hide in exits, etc 
		other->deathtype = dtCHANGELEVEL;
		T_Damage( other, self, self, 50000 );
	}
	return;

/*

// if "noexit" is set, blow up the player trying to leave
//ZOID, 12-13-96, noexit isn't supported in QW.  Overload samelevel
//      if ((cvar("noexit") == 1) || ((cvar("noexit") == 2) && (mapname != "start")))
	if (      ( trap_cvar( "samelevel" ) == 2 )
	     || ( ( trap_cvar( "samelevel" ) == 3 )
		  && ( strneq( g_globalvars.mapname, "start" ) ) ) )
	{
		other->deathtype = dtCHANGELEVEL;
		T_Damage( other, self, self, 50000 );
		return;
	}

	G_bprint( PRINT_HIGH, "%s exited the level\n", other->s.v.netname );

	strlcpy( nextmap, self->map, sizeof(nextmap) );
	
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

	// qqshka: yeah, treat k_remove_end_hurt as hint to remove some shit from this level,
	//         not only hurt trigger
	if (    streq( "end", g_globalvars.mapname )
		 && cvar( "k_remove_end_hurt" )
		 && cvar( "k_remove_end_hurt" ) != 2 
	   ) {
		ent_remove ( self );
		return;
	}

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

	strlcpy( nextmap, g_globalvars.mapname, sizeof(nextmap) );

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
			strlcpy( g_globalvars.mapname, "e1m1", sizeof(g_globalvars.mapname) );

		} else if ( !( ( int ) ( g_globalvars.serverflags ) & 1 ) )
		{
			strlcpy( g_globalvars.mapname, "e1m1", sizeof(g_globalvars.mapname) );
			g_globalvars.serverflags =
			    ( int ) ( g_globalvars.serverflags ) | 1;

		} else if ( !( ( int ) ( g_globalvars.serverflags ) & 2 ) )
		{
			strlcpy( g_globalvars.mapname, "e2m1", sizeof(g_globalvars.mapname) );
			g_globalvars.serverflags =
			    ( int ) ( g_globalvars.serverflags ) | 2;

		} else if ( !( ( int ) ( g_globalvars.serverflags ) & 4 ) )
		{
			strlcpy( g_globalvars.mapname, "e3m1", sizeof(g_globalvars.mapname) );
			g_globalvars.serverflags =
			    ( int ) ( g_globalvars.serverflags ) | 4;

		} else if ( !( ( int ) ( g_globalvars.serverflags ) & 8 ) )
		{
			strlcpy( g_globalvars.mapname, "e4m1", sizeof(g_globalvars.mapname) );
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

	strlcpy( nextmap, o->map, sizeof(nextmap) );

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

void SP_info_player_deathmatch()
{
	gedict_t *spot;
	vec3_t saved_org;
	int i = 0;

	for ( spot = world; (spot = find( spot, FOFS( s.v.classname ), self->s.v.classname )); i++ )
	{
		if ( spot == self )
			self->cnt = i;
	}

// some maps like aerowalk or ztndm* have spawn points hanging some distance
// above the ground, fix them
	setsize( self, PASSVEC3( VEC_HULL_MIN ), PASSVEC3( VEC_HULL_MAX ) );
	VectorCopy (self->s.v.origin, saved_org);
	droptofloor (self);
	if (self->s.v.origin[2] < saved_org[2] - 64)
		setorigin (self, PASSVEC3(saved_org));
	setsize( self, 0, 0, 0, 0, 0, 0 );
}

// called by ClientKill and DeadThink
void respawn()
{
//	G_bprint(2, "respawn()\n");

	// make a copy of the dead body for appearances sake
	CopyToBodyQue( self );
	// set default spawn parms
	SetNewParms( false );
	// respawn              
	PutClientInServer();
}

// i put next code in function, since it appear frequently
void k_respawn( gedict_t *p )
{
	gedict_t *swap = self;

	self = p; // warning

	self->s.v.deadflag = DEAD_RESPAWNABLE;
	self->wreg_attack = 0;
	self->s.v.button0 = 0;
	self->s.v.button1 = 0;
	self->s.v.button2 = 0;
	respawn();

	self = swap;
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

	if ( ISDEAD( self ) || !self->s.v.takedamage )
		return; // alredy dead

	if ( self->ct != ctPlayer )
		return; // not a player

	if ( isRA() ) {
		G_sprint (self, PRINT_HIGH, "Can't suicide in RA mode\n");
		return;
	}

	if (g_globalvars.time < self->suicide_time) {
		G_sprint (self, PRINT_HIGH, "Only one suicide in 1 second\n");
		return;
	}

	self->suicide_time = g_globalvars.time + 1;

	self->deathtype = dtSUICIDE;
	T_Damage( self, self, self, 999999 );
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
gedict_t *SelectSpawnPoint( char *spawnname )
{
	gedict_t		*spot;
	gedict_t		*spots;			// chain of "valid" spots
	gedict_t		*thing;
	int             numspots;		// count of "valid" spots
	int				totalspots;
	int				pcount;
	int				k_spw = cvar( "k_spw" );
	int				weight_sum = 0;	// used by "fair spawns"
    
// testinfo_player_start is only found in regioned levels
	spot = find( world, FOFS( s.v.classname ), "testplayerstart" );
	if ( spot )
		return spot;

// ok, find all spots that don't have players nearby

	spots = world;
	totalspots = numspots = 0;

	for ( spot = world; (spot = find( spot, FOFS( s.v.classname ), spawnname )); )
	{
		totalspots++;
		pcount = 0;

		// find count of nearby players for 'spot'
		for ( thing = world; (thing = trap_findradius( thing, spot->s.v.origin, 84 )); )
		{
			if ( thing->ct != ctPlayer || ISDEAD( thing ) || thing == self )
				continue; // ignore non player, or dead played, or self

		   // k_spw 2 and 3 feature, if player is spawned not far away and run
		   // around spot - treat this spot as not valid.
		   // k_1spawn store this "not far away" time.
		   // k_1spawn is _also_ set after player passed teleport
            if( !( ( k_spw == 2 || k_spw == 3 ) && match_in_progress == 2 && thing->k_1spawn < g_globalvars.time ) )  {
//				G_bprint(2, "ignore player: %s\n", thing->s.v.netname);
				pcount++; // ignore spot
			}
		}

		if( !k_jawnmode && k_spw && match_in_progress == 2 && self->k_lastspawn == spot ) {
//			G_bprint(2, "ignore spot\n");
			pcount++; // ignore this spot in this case, protection from spawn twice on the same spot
		}

		if ( !pcount ) // valid spawn spot
		{
			spot->s.v.goalentity = EDICT_TO_PROG( spots );
			spots = spot;
			numspots++;

			// Calculate weight sum (only goes up to MAX_SPAWN_WEIGHTS spots)
			if ( totalspots && totalspots <= MAX_SPAWN_WEIGHTS )
				weight_sum += self->spawn_weights[ totalspots - 1 ];
		}
	}

    if( match_in_progress == 2 ) // protect(in some case) player from be spawnfragged for some time
		self->k_1spawn = g_globalvars.time + 2.6;

	if ( !numspots )
	{
		// ack, they are all full, just pick one at random

//		G_bprint (PRINT_HIGH, "%s\n", "Ackk! All spots are full. Selecting random spawn spot");

		if ( !(spot = find_idx( i_rnd(0, totalspots - 1), FOFS( s.v.classname ), spawnname )) ) {
			totalspots = 1; // proper count is not so important, something going wrong anyway...

			if ( !(spot = Do_FindIntermission( "info_player_deathmatch" )) )
		    	spot = world;
		}

		if( !match_in_progress || k_spw == 1 || ( k_spw == 2 && !k_checkx ) ) {
			vec3_t	v1, v2;

			trap_makevectors( isRA() ? spot->mangle : spot->s.v.angles ); // stupid ra uses mangles instead of angles

			for( thing = world; (thing = trap_findradius(thing, spot->s.v.origin, 84)); ) {
 				if( streq( thing->s.v.classname, "teledeath") ) {
					ent_remove( thing );
					continue;
				}

				if ( thing->ct != ctPlayer || ISDEAD( thing ) || thing == self )
					continue; // ignore non player, or dead played, or self

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
			}
		}

		if( totalspots > 2 && match_in_progress == 2 )
			self->k_lastspawn = spot;

		return spot;
	}

// We now have the number of spots available on the map in numspots
// Generate a random number between 0 and numspots

	// Jawnmode: use new "fair spawns" model
	// Because of array limits, fall back to old spawn model on maps with more than MAX_SPAWN_WEIGHTS spots
	// - Molgrum
	if ( k_jawnmode && match_in_progress == 2 && totalspots <= MAX_SPAWN_WEIGHTS )
	{
		float    f;
		gedict_t *spawnp;
		int      i = 0;

		// Setup randomization
		f = i_rnd( 0, weight_sum );

		// Get a random spawn point
		for ( spot = spots; i < numspots; i++ )
		{
			f = f - self->spawn_weights[ (int)spot->cnt ];

			// Finished randomizing
			if ( f < 0 )
				break;

			// Prevent spawn spot from becoming world
			if ( PROG_TO_EDICT( spot->s.v.goalentity ) != world )
				spot = PROG_TO_EDICT( spot->s.v.goalentity );
		}

		spawnp = spot;
		spot = spots;

		// Fix weights
		f = self->spawn_weights[ (int)spawnp->cnt ] / 2;
		for ( i = 0; i < numspots; i++)
		{
			if ( spot == spawnp )
				self->spawn_weights[ (int)spot->cnt ] -= f;
			else
				self->spawn_weights[ (int)spot->cnt ] += ( f / (float)( numspots - 1 ) );

			spot = PROG_TO_EDICT( spot->s.v.goalentity );
		}

		return spawnp;
	}
	else
	{
		// Original spawnmodes
		numspots = i_rnd(0, numspots - 1);

		for ( spot = spots; numspots > 0; numspots-- )
			spot = PROG_TO_EDICT( spot->s.v.goalentity );

		if( totalspots > 2 && match_in_progress == 2 ) 
			self->k_lastspawn = spot;

		return spot;
	}
}

qboolean CanConnect()
{
	gedict_t *p;
	char *t;
	int from, usrid, tmid;

	if ( k_sv_locktime && !VIP( self ) ) { // kick non vip in this case
		int seconds = k_sv_locktime - g_globalvars.time;

		G_sprint(self, 2, "%s: %d second%s\n", 
					redtext("server is temporary locked"), seconds, count_s(seconds));

		return false; // _can't_ connect
	}

	if( !match_in_progress || k_matchLess ) { // no ghost, team, etc... checks in matchLess mode
		G_bprint(2, "%s entered the game\n", self->s.v.netname);
		return true; // can connect
	}

	// If the game is running already then . . .
	// guess is player can enter/re-enter the game if server locked or not

	if( cvar("k_lockmode") == 2 ) { // kick anyway
		G_sprint(self, 2, "Match in progress, server locked\n"
						  "Please reconnect as spectator\n");

		return false; // _can't_ connect
	}
	else if( cvar("k_lockmode") == 1 ) // different behavior for team/duel/ffa
	{
		if ( isDuel() || isFFA() ) {
			 // kick if no ghost with same name as for self
			for( from = 1, p = world; (p = find_plrghst( p, &from )); )
				if ( streq( getname( p ), self->s.v.netname ) )
					break;  // don't kick, find "ghost" with equal name
		
			if ( !p ) {
				G_sprint(self, 2, "%s in progress, server locked\n"
								  "Please reconnect as spectator\n", isDuel() ? "Duel" : "Match");
				return false; // _can't_ connect
			}
		}
		else if ( ( isTeam() || isCTF() ) ) {
			// kick if no ghost or player with team as for self
			t = getteam( self );

			for( from = 0, p = world; (p = find_plrghst( p, &from )); )
				if ( p != self && streq( getteam( p ), t ) )
					break;  // don't kick, find "player" or "ghost" with equal team
		
			if ( !p ) {
				G_sprint(self, 2, "Match in progress, server locked\n"
								  "Set your team before connecting\n"
						  		  "or reconnect as spectator\n");
				return false; // _can't_ connect
			}
		}
		else {
			// unknown mode, kick anyway
			G_sprint(self, 2, "Match in progress, server locked\n"
					  		  "Please reconnect as spectator\n");
			return false; // _can't_ connect
		}
	}

	// kick if exclusive 
	if( CountPlayers() >= k_attendees && cvar("k_exclusive") ) 
	{
		G_sprint(self, 2, "Sorry, server is full\n"
						  "Please reconnect as spectator\n");
		return false; // _can't_ connect
	}

	if( match_in_progress != 2 ) // actually that match_in_progress == 1
	{
		G_bprint(2, "%s entered the game\n", self->s.v.netname);
		return true; // can connect
	} 

	for( usrid = 1; usrid < k_userid; usrid++ ) // search for ghost for this player (localinfo)
		if( streq( ezinfokey(world, va("%d", (int) usrid)), self->s.v.netname ))
			break;

	if( usrid < k_userid ) // ghost probably found (localinfo)
	{
		for( p = world; (p = find(p, FOFCLSN, "ghost")); ) // search ghost entity
			if( p->cnt2 == usrid )
				break;

		if( p ) // found ghost entity
		{
			// check teams only for team mode
			if( ( isTeam() || isCTF() ) && strneq( getteam( self ), getteam( p ) ) ) 
			{
				G_sprint(self, 2, "Please join your old team and reconnect\n");
				return false; // _can't_ connect
			}

			ghostClearScores( p );

			self->ps        = p->ps; // restore player stats
			self->s.v.frags = p->s.v.frags;
			self->deaths    = p->deaths;
			self->friendly  = p->friendly;

			if ( isTeam() || isCTF() ) {
				self->k_teamnum = p->k_teamnum; // we alredy have team in localinfo
				G_bprint(2, "%s \220%s\221 %s %d %s%s\n", self->s.v.netname, getteam( self ),
					redtext("rejoins the game with"), (int)self->s.v.frags, redtext("frag"), redtext(count_s(self->s.v.frags)));
			}
			else {
				self->k_teamnum = 0; // force check is we have team in localinfo or not below
				G_bprint(2, "%s %s %d %s%s\n", self->s.v.netname, 
					redtext("rejoins the game with"), (int)self->s.v.frags, redtext("frag"), redtext(count_s(self->s.v.frags)));
			}

			localcmd("localinfo %d \"\"\n", usrid); // remove ghost in localinfo
			ent_remove( p ); // remove ghost entity
		}
		else // ghost entity not found
		{
			localcmd("localinfo %d \"\"\n", usrid); // remove ghost in localinfo
			if ( isTeam() || isCTF() )
				G_bprint(2, "%s \220%s\221 %s\n", self->s.v.netname, getteam( self ), redtext("reenters the game without stats"));
			else
				G_bprint(2, "%s %s\n", self->s.v.netname, redtext("reenters the game without stats"));
		}
	}
	else { // ghost not found (localinfo)
		if ( isTeam() || isCTF() )
			G_bprint(2, "%s \220%s\221 %s\n", self->s.v.netname, getteam( self ), redtext("arrives late"));
		else
			G_bprint(2, "%s %s\n", self->s.v.netname, redtext("arrives late"));
	}

	// check is we have team in localinfo or not	
	if( !strnull ( t = getteam( self ) ) ) 
	{
		for( tmid = 665; tmid < k_teamid && !self->k_teamnum; ) 
		{
			tmid++;
			if( streq( t, ezinfokey(world, va("%d", tmid)) ) )
				self->k_teamnum = tmid;
		}

		if( !self->k_teamnum )  // team not found in localinfo, so put it in
		{
			tmid++;

			localcmd("localinfo %d \"%s\"\n", tmid, getteam( self ));
			k_teamid     = tmid;
			self->k_teamnum = tmid;
		}
	}
	else
		self->k_teamnum = 666;

	return true;
}

////////////////
// GlobalParams:
// time
// self
// params
///////////////
void ClientConnect()
{
	gedict_t *p;
	int i, totalspots;

	VIP_ShowRights( self );

	k_nochange = 0;

	if ( !CanConnect() ) {
		stuffcmd(self, "disconnect\n"); // FIXME: stupid way
		return;
	}

	newcomer = self;

	self->ct = ctPlayer;
	self->s.v.classname = "player";
	self->k_accepted = 1; // ok, we allowed to connect
	self->ready = ((match_in_progress || k_matchLess) ? 1 : 0);

	// if the guy started connecting during intermission and
	// thus missed the svc_intermission, we'd better let him know
	if ( intermission_running )
		SendIntermissionToClient ();

// ILLEGALFPS[

	// Zibbo's frametime checking code
	self->uptimebugpolicy = 0;
	self->real_time = 0;

	// delay on checking/displaying illegal FPS.
	self->fDisplayIllegalFPS = g_globalvars.time + 10 + g_random() * 5;
	self->fIllegalFPSWarnings = 0;

	ZeroFpsStats ();
// ILLEGALFPS]

	CheckRate(self, "");

	if( k_captains == 2 ) { // in case of team picking, check if there is a free spot for player number 1-16
		int i;

		for( i = 1, p = world; p && i <= MAX_CLIENTS; i++ )
			for( p = world; (p = find_plr( p )) && (p == self || p->s.v.frags != i); )
				; // empty

		// if we found a spot, set the player into it
		if( !p ) {
			stuffcmd(self, "color 0\nteam \"\"\nskin \"\"\n");
			self->s.v.frags = i - 1;
		}
	}

	if ( isRA() )
		ra_in_que( self ); // put cleint in ra queue, so later we can move it to loser or winner

	// Jawnmode: reset spawn weights at server join (can handle max MAX_SPAWN_WEIGHTS spawn points atm)
	// Just count the spots
	totalspots = find_cnt(FOFS( s.v.classname ), "info_player_deathmatch" );

	// Don't use this spawn model for maps with more than MAX_SPAWN_WEIGHTS spawns (shouldn't even happen)
	if ( totalspots <= MAX_SPAWN_WEIGHTS )
	{
		// Set the spawn weights to number of spots
		for ( i = 0; i < totalspots; i++ )
			self->spawn_weights[i] = totalspots;
	}

	MakeMOTD();
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

//	G_bprint(2, "PutClientInServer()\n");

	self->deathtype = dtNONE;
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
	self->k_666 = 0;
	self->axhitme = 0;
	self->lastwepfired = 0;

	self->q_pickup_time = self->p_pickup_time = self->r_pickup_time = 0;
        
// the spawn falling damage bug workaround
	self->jump_flag = 0;
	self->swim_flag = 0;

// brokenankle
	self->brokenankle = 0;

// ctf
	self->on_hook = false;
	self->hook_out = false;
	self->maxspeed = cvar("sv_maxspeed"); // qqshka - ctf stuff, discard haste rune modifier after u die
	self->regen_time = -1;
	self->carrier_hurt_time = -1;

	self->invincible_time = 0;

	DecodeLevelParms();

	if ( !( (int)self->s.v.weapon & (int)self->s.v.items ) )
		self->s.v.weapon = W_BestWeapon();
	W_SetCurrentAmmo();

	// if flag is not at base update our items so clients can display icon
	if ( isCTF() && match_in_progress == 2 )
	{
		gedict_t *rflag = find( world, FOFCLSN, "item_flag_team1" );
		gedict_t *bflag = find( world, FOFCLSN, "item_flag_team2" );
            
		if ( rflag && rflag->cnt)
			self->s.v.items = (int) self->s.v.items | IT_KEY2;
		if ( bflag && bflag->cnt)
			self->s.v.items = (int) self->s.v.items | IT_KEY1;
	}

	self->attack_finished = g_globalvars.time;
	self->th_pain = player_pain;
	self->th_die = PlayerDie;

	self->s.v.deadflag = DEAD_NO;
// paustime is set by teleporters to keep the player from moving a while
	self->pausetime = 0;
	
	if ( isCTF() && match_start_time == g_globalvars.time ) // first spawn in CTF on corresponding base
		spot = SelectSpawnPoint(streq(getteam(self), "red") ? "info_player_team1" : "info_player_team2" );
	else if ( isRA() && ( isWinner( self ) || isLoser( self ) ) )
		spot = SelectSpawnPoint("info_teleport_destination" );
	else
		spot = SelectSpawnPoint("info_player_deathmatch");

	VectorCopy( spot->s.v.origin, self->s.v.origin );
	self->s.v.origin[2] += 1;

	if ( isRA() )
	{
		VectorCopy( spot->mangle, self->s.v.angles );
	}
	else
	{
		VectorCopy( spot->s.v.angles, self->s.v.angles );
	}

	self->s.v.fixangle = true;

// oh, this is a hack!
	setmodel( self, "progs/eyes.mdl" );
	modelindex_eyes = self->s.v.modelindex;

	setmodel( self, "progs/player.mdl" );
	modelindex_player = self->s.v.modelindex;

	setsize( self, PASSVEC3( VEC_HULL_MIN ), PASSVEC3( VEC_HULL_MAX ) );
	SetVector( self->s.v.view_ofs, 0, 0, 22 );
	SetVector( self->s.v.velocity, 0, 0, 0 );

	self->walkframe = 0;	
	player_stand1();

	trap_makevectors( self->s.v.angles );
	VectorScale( g_globalvars.v_forward, 20, v );
	VectorAdd( v, self->s.v.origin, v );
	spawn_tfog( v );
	play_teleport( self );
	spawn_tdeath( self->s.v.origin, self );

	if ( isRA() )
	{
		ra_PutClientInServer();

		// drop down to best weapon actually hold
		if ( !( (int)self->s.v.weapon & (int)self->s.v.items ) )
			self->s.v.weapon = W_BestWeapon();

		W_SetCurrentAmmo(); // important shit, not only ammo
		return;
	}

	if( match_in_progress == 2 )
	{
		if( k_berzerk )
		{
			self->s.v.items = (int)self->s.v.items | IT_QUAD;
			self->super_time = 1;
			self->super_damage_finished = g_globalvars.time + 3600*10;
		}

		if( cvar( "k_666" ) )
		{
			stuffcmd (self, "bf\n");
			self->invincible_time = 1;
			self->invincible_finished = g_globalvars.time + 2;
			self->k_666 = 1;
			self->s.v.items = (int)self->s.v.items | IT_INVULNERABILITY;
		}
	}

	if ( deathmatch == 4 && match_in_progress == 2 )
	{
		float dmm4_invinc_time = cvar("dmm4_invinc_time");

		if ( cvar("k_midair") )
		{
			dmm4_invinc_time       = -1; // means off

			self->s.v.ammo_shells  = 0;
			self->s.v.ammo_nails   = 0;
			self->s.v.ammo_cells   = 0;
			self->s.v.ammo_rockets = 255;

			self->s.v.armorvalue   = 200;
			self->s.v.armortype    = 0.8;
			self->s.v.health       = 250;

			items = IT_AXE | IT_ROCKET_LAUNCHER | IT_ARMOR3;
		}
		else if ( cvar("k_instagib") )
		{
			dmm4_invinc_time       = 1; // means 1s

			self->s.v.ammo_shells  = 999;
			self->s.v.ammo_nails   = 0;
			self->s.v.ammo_cells   = 0;
			self->s.v.ammo_rockets = 0;

			self->s.v.armorvalue   = 0;
			self->s.v.armortype    = 0;
			self->s.v.health       = 250;

			items = IT_AXE;
			items |= ( cvar("k_instagib") == 1 || cvar("k_instagib") == 3 ) ? IT_SHOTGUN : IT_SUPER_SHOTGUN;
		}
		else
		{
			self->s.v.ammo_nails   = 255;
			self->s.v.ammo_shells  = 255;
			self->s.v.ammo_rockets = 255;
			self->s.v.ammo_cells   = 255;

			self->s.v.armorvalue   = 200;
			self->s.v.armortype    = 0.8;
			self->s.v.health       = 250;

			items = self->s.v.items;
			items |= IT_NAILGUN;
			items |= IT_SUPER_NAILGUN;
			items |= IT_SUPER_SHOTGUN;
			items |= IT_ROCKET_LAUNCHER;
			items |= IT_GRENADE_LAUNCHER;
			items |= IT_LIGHTNING;

			items &= ~( IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3 ); // remove all armors
			items |= IT_ARMOR3; // add red armor
		}

		// 0 evalutes to 2, negative value disable invincible
		dmm4_invinc_time = (dmm4_invinc_time ? bound(0, dmm4_invinc_time, 30) : 2);

		if ( dmm4_invinc_time > 0 )
		{
			self->k_666 = (self->k_666 ? self->k_666 : 2); // may be already set by cvar( "k_666" ) )

			items |= IT_INVULNERABILITY;

			self->invincible_time = 1;
			self->invincible_finished = g_globalvars.time + dmm4_invinc_time;
		}

		self->s.v.items = items;

		// default to spawning with rl, except if instagib is on
		if ( cvar("k_instagib") )
			self->s.v.weapon = cvar("k_instagib") == 1 ? IT_SHOTGUN : IT_SUPER_SHOTGUN;
		else
			self->s.v.weapon = IT_ROCKET_LAUNCHER;
	}

	if ( deathmatch == 5 && match_in_progress == 2 )
	{
		self->s.v.ammo_nails   = 80;
		self->s.v.ammo_shells  = 30;
		self->s.v.ammo_rockets = 10;
		self->s.v.ammo_cells   = 30;

		self->s.v.armorvalue   = 200;
		self->s.v.armortype    = 0.8;
		self->s.v.health       = 200;

		items = self->s.v.items;
		items |= IT_NAILGUN;
		items |= IT_SUPER_NAILGUN;
		items |= IT_SUPER_SHOTGUN;
		items |= IT_ROCKET_LAUNCHER;
		items |= IT_GRENADE_LAUNCHER;
		items |= IT_LIGHTNING;

		items &= ~( IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3 ); // remove all armors
		items |= IT_ARMOR3; // add red armor

		items |= IT_INVULNERABILITY;
		
		self->s.v.items = items;

		self->invincible_time = 1;
		self->invincible_finished = g_globalvars.time + 3;

		// default to spawning with rl
		self->s.v.weapon = IT_ROCKET_LAUNCHER;
	}

	// remove particular weapons in dmm4
	if ( deathmatch == 4 && match_in_progress == 2 )
	{
		int	k_disallow_weapons = (int)cvar("k_disallow_weapons") & DA_WPNS;

		self->s.v.items = (int)self->s.v.items & ~k_disallow_weapons;
	}

	// drop down to best weapon actually hold
	if ( !( (int)self->s.v.weapon & (int)self->s.v.items ) )
		self->s.v.weapon = W_BestWeapon();

	W_SetCurrentAmmo();
}

/*
===============================================================================

RULES

===============================================================================
*/

// frag difference to win on tiebreak overtime
int	tiecount()
{
	return ( deathmatch == 4 ? 2 : 3 );
}

// check sudden death end
// call this on player death
void Check_SD( gedict_t *p )
{
	if ( !match_in_progress )
		return;

	if ( !k_sudden_death || p->ct != ctPlayer )
		return;

	switch ( (int)k_sudden_death ) {
		case SD_NORMAL:
			EndMatch( 0 );
			return;

		case SD_TIEBREAK: {
			gedict_t *ed1 = get_ed_scores1(), *ed2 = get_ed_scores2();
			int sc = get_scores1() - get_scores2();

			if ( (isDuel() || isFFA()) && ed1 && ed2 )
				sc = ed1->s.v.frags - ed2->s.v.frags;

			if(     ( (isDuel() || isFFA()) && ed1 && ed2 ) // duel or ffa
				 || ( isTeam() || isCTF() ) // some team
			  ) {
				if ( abs( sc ) >= tiecount() )
					EndMatch( 0 );
			} // unknown so end match
			else
				EndMatch( 0 );

			return;
		}
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
	if ( !match_in_progress )
		return;

    if ( fraglimit && self->s.v.frags >= fraglimit )
        EndMatch( 0 );
}

//============================================================================
void PlayerDeathThink()
{
// gedict_t*    old_self;
	float           forward;
	float			respawn_time;

    if( k_standby )
        return;

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
	respawn_time = cvar("k_midair") || cvar("k_instagib") ? 2 : 5;

	if ( dtSUICIDE == self->deathtype || isRA() )
		respawn_time = -999999; // force respawn ASAP if suicides or in RA mode

	if( (g_globalvars.time - self->dead_time) > respawn_time ) {
		k_respawn( self );
		return;
	}
// }

// wait for all buttons released
	if ( self->s.v.deadflag == DEAD_DEAD )
	{
		if ( self->s.v.button2 || self->s.v.button1 || self->s.v.button0 || self->wreg_attack )
			return;
		self->s.v.deadflag = DEAD_RESPAWNABLE;
		return;
	}

// wait for any button down
	if ( !self->s.v.button2 && !self->s.v.button1 && !self->s.v.button0 && !self->wreg_attack )
		return;

	k_respawn( self );
}


void PlayerJump()
{
	//vec3_t start, end;

	if ( self->spawn_time + 0.05 > g_globalvars.time ) {
		self->s.v.velocity[2] = -270;  // discard +jump till 50 ms after respawn, like ktpro 
		self->s.v.flags = (int)self->s.v.flags & ~FL_JUMPRELEASED;
		return;
	}

	if ( ( ( int ) ( self->s.v.flags ) ) & FL_WATERJUMP )
		return;

	if ( self->s.v.waterlevel >= 2 )
	{
// play swiming sound
		if ( self->swim_flag < g_globalvars.time )
		{
			self->swim_flag = g_globalvars.time + 1;

			// Jawnmode: don't play swimming sound to cut older clients without smartjump some slack
			// - Molgrum
			if ( !k_jawnmode )
			{
				if ( g_random() < 0.5 )
					sound( self, CHAN_BODY, "misc/water1.wav", 1, ATTN_NORM );
				else
					sound( self, CHAN_BODY, "misc/water2.wav", 1, ATTN_NORM );
			}
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

	if ( !can_prewar( false ) )
		self->s.v.velocity[2] = -270;

// check the flag and jump if we can
    if ( !self->brokenankle )
	{
        self->s.v.button2 = 0;

		// player jumping sound
		//crt - get rid of jump sound for spec
		if ( !isRA() || ( isWinner( self ) || isLoser( self ) ) )
			sound( self, CHAN_BODY, "player/plyrjmp8.wav", 1, ATTN_NORM );

       	// JUMPBUG[
        // the engine checks velocity_z and won't perform the jump if it's < zero!

		// qqshka - i know this is shit - but ktpro does it, and probably server engine even ignor this hack

		if ( self->s.v.velocity[2] < 0 )
			self->s.v.velocity[2] = 0;

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

			switch ( (int)self->s.v.watertype ) {
				case CONTENT_LAVA:  self->deathtype = dtLAVA_DMG;  break; // funny, we can drown in lava ?
				case CONTENT_SLIME: self->deathtype = dtSLIME_DMG; break; // and in slime ?
				default:			self->deathtype = dtWATER_DMG; break; // and sure we can drown in water
			}
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

			self->deathtype = dtLAVA_DMG;
			T_Damage( self, world, world, 10 * self->s.v.waterlevel );
		}

	} else if ( self->s.v.watertype == CONTENT_SLIME )
	{			// do damage
		if ( self->dmgtime < g_globalvars.time && self->radsuit_finished < g_globalvars.time )
		{
			self->dmgtime = g_globalvars.time + 1;
			self->deathtype = dtSLIME_DMG;
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
	trap_makevectors( self->s.v.angles );
	
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

	adjust_pickup_time( &self->q_pickup_time, &self->ps.itm[itQUAD].time );
	adjust_pickup_time( &self->p_pickup_time, &self->ps.itm[itPENT].time );
	adjust_pickup_time( &self->r_pickup_time, &self->ps.itm[itRING].time );

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
	ghost->ghost_clr  = (int)bound(0, iKey(self, "topcolor" ), 13) << 8;
	ghost->ghost_clr |= (int)bound(0, iKey(self, "bottomcolor" ), 13) ; // save colors

//	G_bprint( PRINT_HIGH, "name num: %d team num %d\n", (int)ghost->cnt2, (int)ghost->k_teamnum);
  
	localcmd("localinfo %d \"%s\"\n", (int)f1, self->s.v.netname);
	trap_executecmd();
}

void set_important_fields(gedict_t *p)
{
	p->s.v.takedamage	= DAMAGE_NO;
	p->s.v.solid		= SOLID_NOT;
	p->s.v.movetype		= MOVETYPE_NONE;
	p->s.v.deadflag		= DEAD_DEAD;
	p->s.v.health		= 0;
	p->s.v.frame		= 0;
	p->s.v.modelindex	= 0;
	p->s.v.model		= "";
	p->s.v.nextthink	= -1;

	p->ct				= ctNone;
	p->k_accepted		= 0;
	p->s.v.classname	= ""; // clear client classname on disconnect
}

////////////////
// GlobalParams:
// self
///////////////
void ClientDisconnect()
{
	k_nochange = 0; // force recalculate frags scores

	if ( !self->k_accepted ) {
		set_important_fields( self ); // set classname == "" and etc
		return;
	}

	del_from_specs_favourites( self );

	ra_ClientDisconnect();

	if( match_in_progress == 2 && self->ct == ctPlayer )
	{
		G_bprint( PRINT_HIGH, "%s left the game with %.0f frags\n", self->s.v.netname, self->s.v.frags );

		sound( self, CHAN_BODY, "player/tornoff2.wav", 1, ATTN_NONE );

		if ( !k_matchLess ) // no ghost in matchless mode
			MakeGhost ();
	}

	DropRune();
	PlayerDropFlag( self );

// s: added conditional function call here
	if( self->v.elect_type != etNone ) {
		G_bprint(2, "Election aborted\n");
		AbortElect();
	}

	set_important_fields( self ); // set classname == "" and etc

// s: added conditional function call here
	if( self->k_kicking )
		ExitKick( self );

	if( capt_num( self ) ) {
        G_bprint(2, "A %s has left\n", redtext("captain"));

		ExitCaptain();
	}

	if( k_captains == 2 )
		CheckFinishCaptain();

	if( cvar( "k_idletime" ) > 0 )
		IdlebotCheck();

	if ( !CountPlayers() ) {
		void Spawn_DefMapChecker( float timeout );
		int um_idx;

		cvar_fset("_k_last_xonx", 0); // forget last XonX command

		if( match_in_progress )
			EndMatch( 1 ); // skip demo, make some other stuff

		// Check if issued to execute reset.cfg (sturm)
        if( cvar( "k_autoreset" ) ) {
			char *cfg_name = "configs/reset.cfg";
			char buf[1024*4];

			if ( can_exec( cfg_name ) ) {
				trap_readcmd( va("exec %s\n", cfg_name), buf, sizeof(buf) );
				G_cprint("%s", buf);
			}
		}

		if ( ( um_idx = um_idx_byname( k_matchLess ? "ffa" : cvar_string("k_defmode") ) ) >= 0 )
			UserMode( -(um_idx + 1) ); // force exec configs for default user mode
		
		if ( !cvar( "lock_practice" ) && k_practice )
			changelevel( g_globalvars.mapname ); // force reload current map in practice mode anyway, ASAP
		else
			Spawn_DefMapChecker( 10 ); // delayed map reload, may be skipped due to different settings
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

void wp_wrap_cat(char *s, char *buf, int size)
{
	const int max_line = 40; // 320 / 8 = 40
	char *n_pos = strrchr(buf, '\n');
	int s_len = strlen( s ), b_len = strlen( n_pos ? n_pos : buf );

	strlcat(buf, ( s_len + b_len > max_line ) ? "\n" : " ", size);
	strlcat(buf, s, size);
}

void Print_Wp_Stats( )
{
	char buf[1024] = {0};

	qboolean ktpl = (iKey( self, "ktpl" ) ? true : false);
	int  i, lw = iKey( self, "lw" ) + (ktpl ? 12 : 0), lw_x = iKey ( self, "lw_x" );
	int _wps = S_ALL & iKey ( self, "wps" );
	int  wps = ( _wps ? _wps : S_DEF ); // if wps is not set - show S_DEF weapons

	gedict_t *g = self->ct == ctSpec ? PROG_TO_EDICT( self->s.v.goalentity ) : NULL;
	gedict_t *e = self->ct == ctPlayer ? self : ( g ? g : world ); // stats of whom we want to show

#if 0 /* percentage */
	float axe = wps & S_AXE ? 100.0 * e->ps.wpn[wpAXE].hits / max(1, e->ps.wpn[wpAXE].attacks) : 0;
#else /* just count of direct hits */
	float axe = wps & S_AXE ? e->ps.wpn[wpAXE].hits : 0;
#endif
	float sg  = wps & S_SG  ? 100.0 * e->ps.wpn[wpSG].hits  / max(1, e->ps.wpn[wpSG].attacks) : 0;
	float ssg = wps & S_SSG ? 100.0 * e->ps.wpn[wpSSG].hits / max(1, e->ps.wpn[wpSSG].attacks) : 0;
	float ng  = wps & S_NG  ? 100.0 * e->ps.wpn[wpNG].hits  / max(1, e->ps.wpn[wpNG].attacks) : 0;
	float sng = wps & S_SNG ? 100.0 * e->ps.wpn[wpSNG].hits / max(1, e->ps.wpn[wpSNG].attacks) : 0;
#if 0 /* percentage */
	float gl  = wps & S_GL  ? 100.0 * e->ps.wpn[wpGL].hits  / max(1, e->ps.wpn[wpGL].attacks) : 0;
	float rl  = wps & S_RL  ? 100.0 * e->ps.wpn[wpRL].hits  / max(1, e->ps.wpn[wpRL].attacks) : 0;
#else /* just count of direct hits */
	float gl  = wps & S_GL  ? e->ps.wpn[wpGL].hits : 0;
	float rl  = wps & S_RL  ? max(0.001, e->ps.wpn[wpRL].hits) : 0;
#endif
	float lg  = wps & S_LG  ? max(0.001, 100.0 * e->ps.wpn[wpLG].hits / max(1, e->ps.wpn[wpLG].attacks)) : 0;

	if ( (i = lw) > 0 ) {
		i = bound(0, i, sizeof(buf)-1 );
		memset( (void*)buf, (int)'\n', i);
		buf[i] = 0;
	}

	if ( e == world || e->ct != ctPlayer ) { // spec tracking no one
		G_centerprint( self, "%s%s", buf, redtext("Tracking no one (+wp_stats)"));

		self->need_clearCP  = 1;
		self->wp_stats_time = g_globalvars.time + WP_STATS_UPDATE;

		return;
	}

	if ( !axe && !sg && !ssg && !ng && !sng && !gl && !rl && !lg )
		return; // sanity

	if ( (i = lw_x) > 0 ) {
		int offset = strlen(buf);
		i = bound(0, i, (int)sizeof(buf) - offset - 1);
		memset( (void*)(buf + offset), (int)' ', i);
		buf[i+offset] = 0;
	}

	if ( ktpl ) {
		if ( lg )
			strlcat(buf, lg  ? va("%s:%.1f ", redtext("lg"),  lg) : "", sizeof(buf));
		if ( rl )
			strlcat(buf, rl  ? va("%s:%.0f ", redtext("rl"),  rl) : "", sizeof(buf));
		if ( gl )
			strlcat(buf, gl  ? va("%s:%.0f ", redtext("gl"),  gl) : "", sizeof(buf));
		if ( axe )
			strlcat(buf, axe ? va("%s:%.0f ", redtext("axe"),axe) : "", sizeof(buf));
		if ( sg )
			strlcat(buf, sg  ? va("%s:%.1f ", redtext("sg"),  sg) : "", sizeof(buf));
    	if ( ssg )
			strlcat(buf, ssg ? va("%s:%.1f ", redtext("ssg"),ssg) : "", sizeof(buf));
		if ( ng )
			strlcat(buf, ng  ? va("%s:%.1f ", redtext("ng"),  ng) : "", sizeof(buf));
		if ( sng )
			strlcat(buf, sng ? va("%s:%.1f ", redtext("sng"),sng) : "", sizeof(buf));
	}
	else {
		if ( lg )
			wp_wrap_cat(lg  ? va("%s:%.1f", redtext("lg"),  lg) : "", buf, sizeof(buf));
		if ( rl )
			wp_wrap_cat(rl  ? va("%s:%.0f", redtext("rl"),  rl) : "", buf, sizeof(buf));
		if ( gl )
			wp_wrap_cat(gl  ? va("%s:%.0f", redtext("gl"),  gl) : "", buf, sizeof(buf));
		if ( axe )
			wp_wrap_cat(axe ? va("%s:%.0f", redtext("axe"),axe) : "", buf, sizeof(buf));
		if ( sg )
			wp_wrap_cat(sg  ? va("%s:%.1f", redtext("sg"),  sg) : "", buf, sizeof(buf));
    	if ( ssg )
			wp_wrap_cat(ssg ? va("%s:%.1f", redtext("ssg"),ssg) : "", buf, sizeof(buf));
		if ( ng )
			wp_wrap_cat(ng  ? va("%s:%.1f", redtext("ng"),  ng) : "", buf, sizeof(buf));
		if ( sng )
			wp_wrap_cat(sng ? va("%s:%.1f", redtext("sng"),sng) : "", buf, sizeof(buf));
	}

	if ( (i = lw_x) < 0 ) {
		int offset = strlen(buf);
		i = bound(0, -i, (int)sizeof(buf) - offset - 1);
		memset( (void*)(buf + offset), (int)' ', i);
		buf[i+offset] = 0;
	}

	if ( (i = lw) < 0 ) {
		int offset = strlen(buf);
		i = bound(0, -i, (int)sizeof(buf) - offset - 1);
		memset( (void*)(buf + offset), (int)'\n', i);
		buf[i+offset] = 0;
	}

	if ( strnull( buf ) )
		return; // sanity

	self->need_clearCP  = 1;
	self->wp_stats_time = g_globalvars.time + WP_STATS_UPDATE;

	G_centerprint( self, "%s",  buf );
}

void Print_Scores( )
{
	char buf[1024] = {0}, *last_va;

	int  i, minutes = 0, seconds = 0, ts = 0, es = 0, ls = iKey( self, "ls" ) + (iKey( self, "ktpl" ) ? 12 : 0);

	qboolean sc_ok = false;
	gedict_t *p, *ed1, *ed2;
	gedict_t *g = self->ct == ctSpec ? PROG_TO_EDICT( self->s.v.goalentity ) : NULL;
	gedict_t *e = self->ct == ctPlayer ? self : ( g ? g : world ); // stats of whom we want to show

	if ( (i = ls) > 0 ) {
		i = bound(0, i, sizeof(buf)-1 );
		memset( (void*)buf, (int)'\n', i);
		buf[i] = 0;
	}

	if ( e == world || e->ct != ctPlayer ) { // spec tracking no one
		G_centerprint( self, "%s%s", buf, redtext("Tracking no one (+scores)"));

		self->need_clearCP  = 1;
		self->sc_stats_time = g_globalvars.time + SC_STATS_UPDATE;

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

	if ( isCTF() ) {
		gedict_t *flag1 = find( world, FOFCLSN, "item_flag_team1" );
		gedict_t *flag2 = find( world, FOFCLSN, "item_flag_team2" );
		char *r_f = "", *b_f = "", *rune = "";

		if ( flag1 && flag2 ) {
			switch ( (int) flag1->cnt )
			{
				case FLAG_AT_BASE: r_f = " "; break;
 	 			case FLAG_CARRIED: r_f = "R"; break;
				case FLAG_DROPPED: r_f = "\322"; break;
				default: r_f = " ";
			}

			switch ( (int) flag2->cnt )
			{
				case FLAG_AT_BASE: b_f = " "; break;
 	 			case FLAG_CARRIED: b_f = "B"; break;
				case FLAG_DROPPED: b_f = "\302"; break;
				default: b_f = " ";
			}

			if ( e->ctf_flag & CTF_RUNE_RES )
				rune = "res";
			else if ( e->ctf_flag & CTF_RUNE_STR )
				rune = "str";
			else if ( e->ctf_flag & CTF_RUNE_HST )
				rune = "hst";
			else if ( e->ctf_flag & CTF_RUNE_RGN )
				rune = "rgn";
			else
				rune = "   ";

			strlcat(buf, va("%s \205%s\205%s\205 ", rune, r_f, b_f), sizeof(buf));
		}
	}

	if ( k_sudden_death )
		strlcat(buf, va("%s:%-5.5s", redtext("tl"), redtext(k_sudden_death == SD_NORMAL ? "sd" : "tb")), sizeof(buf));
	else
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

	last_va = "";

	if ( sc_ok ) {
		if ( isCTF() )
			strlcat(buf, last_va = va("  \x90%d\x91", ts-es), sizeof(buf));
		else
			strlcat(buf, last_va = va("  \364:%d  \345:%d  \x90%d\x91", ts, es, ts-es), sizeof(buf));
	}

	// add spaces, so line in most cases is don't move from side to side during frags changes
	if ( (i = (isCTF() ? sizeof("  [-zzzzz]")-1 : sizeof("  t:xxxxx  e:yyyyy  [-zzzzz]")-1 ) - strlen(last_va) ) > 0 ) {
		int offset = strlen(buf);
		i = bound(0, i, (int)sizeof(buf) - offset - 1);
		memset( (void*)(buf + offset), (int)' ', i);
		buf[i+offset] = 0;
	}

	if ( (i = ls) < 0 ) {
		int offset = strlen(buf);
		i = bound(0, -i, (int)sizeof(buf) - offset - 1);
		memset( (void*)(buf + offset), (int)'\n', i);
		buf[i+offset] = 0;
	}

	if ( strnull( buf ) )
		return; // sanity

	self->need_clearCP  = 1;
	self->sc_stats_time = g_globalvars.time + SC_STATS_UPDATE;

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

void mv_playback ();

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
	float   r;
	qboolean zeroFps = false;

	if ( self->k_timingWarnTime )
		BackFromLag();

	if ( self->sc_stats && self->sc_stats_time && self->sc_stats_time <= g_globalvars.time && match_in_progress != 1 )
		Print_Scores ();

	if ( self->wp_stats && self->wp_stats_time && self->wp_stats_time <= g_globalvars.time && match_in_progress != 1 )
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

			if( r > 105 )
				self->uptimebugpolicy += 1;
		}

		if( self->uptimebugpolicy > 3 ) {
			G_bprint(PRINT_HIGH, "\n%s gets kicked for too long uptime\n", self->s.v.netname);
			G_sprint(self, PRINT_HIGH, "Reboot your machine to get rid of this bug\n");

			stuffcmd(self, "disconnect\n"); // FIXME: stupid way
		}
// ends here


		fps = ( self->fAverageFrameTime / self->fFrameCount );

		fps = fps ? (1.0f / fps) : 1;

//		G_bprint(2, "%s FPS: %3.1f\n", self->s.v.netname, fps);
		
		if( fps > current_maxfps + 2 ) // 2 fps fluctuation is allowed :(
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

	mv_playback ();

	if ( intermission_running )
	{
		IntermissionThink();	// otherwise a button could be missed between
		return;					// the think tics
	}

	if ( self->s.v.view_ofs[0] == 0 && self->s.v.view_ofs[1] == 0
	     && self->s.v.view_ofs[2] == 0 )
		return;		// intermission or finale

	if ( isRA() )
		RocketArenaPre();

	if( k_pause )
		return;

	trap_makevectors( self->s.v.v_angle );	// is this still used

	CheckRules();

// FIXME: really?
//	WaterMove();

/*
 if (self->s.v.waterlevel == 2)
  CheckWaterJump ();
*/

	if ( self->s.v.deadflag >= DEAD_DEAD )
	{
		self->super_damage_finished = 0; // moved from PlayerDie()
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
		PlayerJump();
	else
		self->s.v.flags = ( ( int ) ( self->s.v.flags ) ) | FL_JUMPRELEASED;

// teleporters can force a non-moving pause time
	if ( g_globalvars.time < self->pausetime )
		SetVector( self->s.v.velocity, 0, 0, 0 );

	if ( g_globalvars.time > self->attack_finished && self->s.v.currentammo == 0
	     && self->s.v.weapon != IT_AXE && self->s.v.weapon != IT_HOOK )
	{
		self->s.v.weapon = W_BestWeapon();
		W_SetCurrentAmmo();
	}

	// CTF
	if ( self->on_hook )
		GrappleService();

	if ( self->ctf_flag & CTF_RUNE_RGN )
	{
		if ( self->regen_time < g_globalvars.time )
		{
			self->regen_time = g_globalvars.time;

			if ( self->s.v.health < 150 )
			{
				self->s.v.health += 5;
				if ( self->s.v.health > 150 )
					self->s.v.health = 150;
				self->regen_time += 0.5;
				RegenerationSound( self );
	    	}

			if ( self->s.v.armorvalue < 150 && self->s.v.armorvalue > 0 )
			{
				self->s.v.armorvalue += 5;
				if ( self->s.v.armorvalue > 150 )
					self->s.v.armorvalue = 150;
				self->regen_time += 0.5;
				RegenerationSound( self );
	    	}
		}
	}

	VectorCopy( self->s.v.velocity, self->old_vel );
}

/////////////////////////////////////////////////////////////////

/*
================
CheckPowerups

Check for turning off powerups
================
*/
extern void ktpro_autotrack_on_powerup_out (gedict_t *dude);

void CheckPowerups()
{
	qboolean dim = false;

	if ( ISDEAD( self ) )
		return;

	if ( self->ctf_flag & CTF_FLAG )
		dim = true;

// invisibility
	if ( self->invisible_finished )
	{
// sound and screen flash when items starts to run out
		if ( self->invisible_sound < g_globalvars.time )
		{
			sound( self, CHAN_AUTO, "items/inv3.wav", 0.5, ATTN_IDLE );
			self->invisible_sound = g_globalvars.time + ( ( g_random() * 3 ) + 1 );
		}

		if ( self->invisible_finished < g_globalvars.time + 3 )
		{
			if ( self->invisible_time == 1 )
			{
				G_sprint( self, PRINT_HIGH, "Ring of Shadows magic is fading\n" );
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
			if ( cvar("k_instagib") ) {
				G_bprint( PRINT_HIGH, "%s lost his powers\n", self->s.v.netname );
				self->s.v.health = min(200, self->s.v.health);
			}
			self->invisible_finished = 0;
			self->invisible_time = 0;
			if (vw_enabled)
				W_SetCurrentAmmo();		// set the correct .vw_index
			self->s.v.modelindex = modelindex_player;	// don't use eyes

			adjust_pickup_time( &self->r_pickup_time, &self->ps.itm[itRING].time );

			ktpro_autotrack_on_powerup_out( self );
		}
		else
		{
			// use the eyes
			self->s.v.frame = 0;
			self->vw_index = 0;
			self->s.v.modelindex = modelindex_eyes;
		}
	}

// invincibility

	if ( self->invincible_finished )
	{
// sound and screen flash when items starts to run out
		if(self->invincible_finished < g_globalvars.time + 3 && (!self->k_666 || self->k_666 == 2))	//team
		{
			if ( self->invincible_time == 1 )
			{
				G_sprint( self, PRINT_HIGH, "Protection is almost burned out\n" );
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

			adjust_pickup_time( &self->p_pickup_time, &self->ps.itm[itPENT].time );

			ktpro_autotrack_on_powerup_out( self );
		}

		if(self->invincible_finished > g_globalvars.time && !self->k_666) // KTeAMS
		{
			dim = true;
			self->s.v.effects = ( int ) self->s.v.effects | EF_DIMLIGHT;
			self->s.v.effects = ( int ) self->s.v.effects | EF_RED;
		}
		else
		{
			if ( !dim ) // EF_DIMLIGHT shared between quad, pent and CTF-flag
				self->s.v.effects -= ( ( int ) self->s.v.effects & EF_DIMLIGHT );
			self->s.v.effects -= ( ( int ) self->s.v.effects & EF_RED );
		}
	}

// super damage
	if ( self->super_damage_finished )
	{
// sound and screen flash when items starts to run out
		if(self->super_damage_finished < g_globalvars.time + 3)
		{
			if ( self->super_time == 1 )
			{
				if ( deathmatch == 4 )
					G_sprint( self, PRINT_HIGH, "OctaPower is wearing off\n" );
				else
					G_sprint( self, PRINT_HIGH, "Quad Damage is wearing off\n" );
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

        if(self->super_damage_finished < g_globalvars.time)
		{		// just stopped
			self->s.v.items -= IT_QUAD;
			if ( !k_practice ) // #practice mode#
			if ( deathmatch == 4 )
			{
				self->s.v.ammo_cells = 255;
				self->s.v.armorvalue = 1;
				self->s.v.armortype = 0.8;
				self->s.v.health = min(100, self->s.v.health);
			}
			self->super_damage_finished = 0;
			self->super_time = 0;

			adjust_pickup_time( &self->q_pickup_time, &self->ps.itm[itQUAD].time );

			ktpro_autotrack_on_powerup_out( self );
		}

		if ( self->super_damage_finished > g_globalvars.time )
		{
			dim = true;
			self->s.v.effects = ( int ) self->s.v.effects | EF_DIMLIGHT;
			self->s.v.effects = ( int ) self->s.v.effects | EF_BLUE;
		}
		else
		{
			if ( !dim ) // EF_DIMLIGHT shared between quad, pent and CTF-flag
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
				G_sprint( self, PRINT_HIGH, "Air supply in Biosuit expiring\n" );
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

			ktpro_autotrack_on_powerup_out( self );
		}
	}
}

void check_callalias ();

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
	if ( !self->wp_stats && self->wp_stats_time && self->wp_stats_time <= g_globalvars.time )
		self->wp_stats_time = 0;
	if ( !self->sc_stats && self->sc_stats_time && self->sc_stats_time <= g_globalvars.time )
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

	KickThink ();

	check_callalias ();
}


void	W_WeaponFrame();
void	mv_record ();
void	CheckStuffRune ();

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

	if( k_pause ) {
		ImpulseCommands();
		return;
	}

	if ( self->s.v.view_ofs[0] == 0 && self->s.v.view_ofs[1] == 0 && self->s.v.view_ofs[2] == 0 )
		return;		// intermission or finale

	if ( self->s.v.deadflag )
		return;

//team

// WaterMove function call moved here from PlayerPreThink to avoid
// occurrence of the spawn lavaburn bug and to fix the problem on spawning
// and playing the leave water sound if the player died underwater.

    WaterMove ();


// clear the flag if we landed
    if( (int)self->s.v.flags & FL_ONGROUND )
		self->brokenankle = 0;

// check to see if player landed and play landing sound 
	if ( ( self->jump_flag < -300 ) && ( ( ( int ) ( self->s.v.flags ) ) & FL_ONGROUND ) )
	{
// Falling often results in 5-5 points of damage through 2 frames.
// This fixes the bug
        self->s.v.velocity[2] = 0;

		if ( self->s.v.watertype == CONTENT_WATER )
			sound( self, CHAN_BODY, "player/h2ojump.wav", 1, ATTN_NORM );
		else if ( self->jump_flag < jumpf_flag )
		{
			gedict_t *gre = PROG_TO_EDICT ( self->s.v.groundentity );

			// set the flag if needed
			if ( self->s.v.waterlevel < 2 )
			{
				// Jawnmode: fallbunny is always enabled, but with a modification: speed is capped
				// - Molgrum
				if ( !get_fallbunny() )
					self->brokenankle = 1;  // Yes we have just broken it
			}

			self->deathtype = dtFALL;
			T_Damage( self, world, world, 5 );
			sound( self, CHAN_VOICE, "player/land2.wav", 1, ATTN_NORM );

			if ( gre && gre->s.v.takedamage == DAMAGE_AIM && gre != self )
			{
				// we landed on someone's head, hurt him
				gre->deathtype = dtSTOMP;
				T_Damage (gre, self, self, 10);
			}
		} else
			sound( self, CHAN_VOICE, "player/land.wav", 1, ATTN_NORM );
	}

	self->jump_flag = self->s.v.velocity[2];

	CheckPowerups();
	CheckStuffRune();

	mv_record();

	W_WeaponFrame();

	{
		float velocity = sqrt(self->s.v.velocity[0] * self->s.v.velocity[0] + 
							  self->s.v.velocity[1] * self->s.v.velocity[1]);
		if ( !match_in_progress && !match_over && !k_captains )
		{
			if ( iKey( self, "kf" ) & KF_SPEED ) {
				float velocity_vert_abs	= fabs(self->s.v.velocity[2]);
				self->s.v.armorvalue	= (int)(velocity < 1000 ? velocity + 1000 : -velocity);
				self->s.v.frags			= (int)(velocity) / 1000;
				self->s.v.ammo_shells   = 100 + (int)(velocity_vert_abs) / 100000000;
				self->s.v.ammo_nails    = 100 + (int)(velocity_vert_abs) %   1000000 / 10000;
				self->s.v.ammo_rockets  = 100 + (int)(velocity_vert_abs) %     10000 / 100;
				self->s.v.ammo_cells    = 100 + (int)(velocity_vert_abs) %       100;
			}
			else {
				self->s.v.armorvalue = 1000;
				self->s.v.frags = 0;
			}
 		}
		if( match_in_progress == 2 )
		{
			self->ps.vel_frames++;
			self->ps.velocity_sum += velocity;
			if (self->ps.velocity_max < velocity)
				self->ps.velocity_max = velocity;
		}
	}
}

#define MAX_MEMBERS (10) // max members per team

// clnum origin(3 ints) health armor items nick
void SendTeamInfo(gedict_t *t)
{
	int cl, cnt, h, a;
	gedict_t *p, *s;
	char *tm, *nick;

	s = (t->ct == ctSpec ? PROG_TO_EDICT( t->s.v.goalentity ) : t);
	if (s->ct != ctPlayer)
		return;

	tm = getteam( s );
	
	for ( cnt = 0, p = world; (p = find_plr( p )); ) {
		if (cnt >= MAX_MEMBERS)
			break;

		if ( p == s )
			continue; // ignore self

		if ( strneq(tm, getteam( p )) )
			continue; // on different team

		if ( strnull( nick = ezinfokey(p, "k_nick") ) ) // get nick, if any, do not send name, client can guess it too
			nick = ezinfokey(p, "k");

		if (nick[0] && nick[1] && nick[2] && nick[3])
			nick[4] = 0; // truncate nick to 4 symbols

		cnt++;

		cl = NUM_FOR_EDICT( p ) - 1;
		h = bound(0, (int)p->s.v.health, 999);
		a = bound(0, (int)p->s.v.armorvalue, 999);

		stuffcmd( t, "//tinfo %d %d %d %d %d %d %d \"%s\"\n", cl,
		 (int)p->s.v.origin[0], (int)p->s.v.origin[1], (int)p->s.v.origin[2], h, a, (int)p->s.v.items, nick);
	}
}

void CheckTeamStatus( )
{
	gedict_t *p;
	int k_teamoverlay;

	if ( !isTeam() && !isCTF() )
		return; // non team game

	if ( g_globalvars.time - lastTeamLocationTime < TEAM_LOCATION_UPDATE_TIME )
		return;
	
	lastTeamLocationTime = g_globalvars.time;

	k_teamoverlay = cvar("k_teamoverlay");

	for ( p = world; (p = find_client( p )); ) {
		char *clinfo;
		int ti;

		if (!k_teamoverlay) // teamoverlay turned off
			if (p->ct != ctSpec) // sent overlay to spec only then
				continue;

		if ( (ti = iKey(p, "ti")) < 0 )
			continue; // user specifie no team info

		// check for ezquake or user specifie use team info even non ezquake client
		if ( ti > 0 || (!strnull( clinfo = ezinfokey( p, "*client" ) ) && strstr(clinfo, "ezQuake")) )
			SendTeamInfo( p );
	}
}

void TookWeaponHandler( gedict_t *p, int new_wp )
{
	weaponName_t wp;

	switch ( new_wp ) { // guess which weapon he took
		case IT_AXE:				wp = wpAXE; break;
		case IT_SHOTGUN:			wp = wpSG;  break;
		case IT_SUPER_SHOTGUN:		wp = wpSSG; break;
		case IT_NAILGUN:			wp = wpNG;  break;
		case IT_SUPER_NAILGUN:		wp = wpSNG; break;
		case IT_GRENADE_LAUNCHER:	wp = wpGL;  break;
		case IT_ROCKET_LAUNCHER:	wp = wpRL;  break;
		case IT_LIGHTNING:			wp = wpLG;  break;
		default:					wp = wpNONE;
	}

	p->ps.wpn[wp].ttooks++; // total weapon tooks
	if ( !((int)p->s.v.items & new_wp) ) // player does't have this weapon before took
		p->ps.wpn[wp].tooks++;
}

void StatsHandler(gedict_t *targ, gedict_t *attacker)
{
	int items = targ->s.v.items;
	weaponName_t wp;
	char *attackerteam, *targteam;

	attackerteam = getteam(attacker);
	targteam     = getteam(targ);

	adjust_pickup_time( &targ->q_pickup_time, &targ->ps.itm[itQUAD].time );
	adjust_pickup_time( &targ->p_pickup_time, &targ->ps.itm[itPENT].time );
	adjust_pickup_time( &targ->r_pickup_time, &targ->ps.itm[itRING].time );

	// update spree stats
	if ( strneq( attackerteam, targteam ) )
	{
		attacker->ps.spree_current++;
		if ( attacker->super_damage_finished > 0 )
			attacker->ps.spree_current_q++;
	}

	targ->ps.spree_max = max(targ->ps.spree_current, targ->ps.spree_max);
	targ->ps.spree_max_q = max(targ->ps.spree_current_q, targ->ps.spree_max_q);
	targ->ps.spree_current = targ->ps.spree_current_q = 0;

	if ( attacker->ct == ctPlayer ) {
		switch ( targ->deathtype ) {
			case dtAXE: wp = wpAXE; break;
			case dtSG:  wp = wpSG;  break;
			case dtSSG: wp = wpSSG; break;
			case dtNG:  wp = wpNG;  break;
			case dtSNG: wp = wpSNG; break;
			case dtGL:  wp = wpGL;  break;
			case dtRL:  wp = wpRL;  break;
			case dtLG_BEAM:
			case dtLG_DIS:
			case dtLG_DIS_SELF:
  						wp = wpLG;  break;
			default:	wp = wpNONE;
		}
		
		if ( targ == attacker ) {
			; // killed self, nothing interest
		}
        else if ( (isTeam() || isCTF()) && streq( targteam, attackerteam ) && !strnull( attackerteam ) ) {
			// team kill
			attacker->ps.wpn[wp].tkills++;
		}
		else {
			// normal kill
			attacker->ps.wpn[wp].kills++;
			targ->ps.wpn[wp].deaths++;

			// hmm, may be add some priority? so if targ have rl and gl bump only wpn[wpRL].ekills ?
			if ( (items & IT_AXE) )
				attacker->ps.wpn[wpAXE].ekills++;
			if ( (items & IT_SHOTGUN) )
				attacker->ps.wpn[wpSG].ekills++;
			if ( (items & IT_SUPER_SHOTGUN) )
				attacker->ps.wpn[wpSSG].ekills++;
			if ( (items & IT_NAILGUN) )
				attacker->ps.wpn[wpNG].ekills++;
			if ( (items & IT_SUPER_NAILGUN) )
				attacker->ps.wpn[wpSNG].ekills++;
			if ( (items & IT_GRENADE_LAUNCHER) )
				attacker->ps.wpn[wpGL].ekills++;
			if ( (items & IT_ROCKET_LAUNCHER) )
				attacker->ps.wpn[wpRL].ekills++;
			if ( (items & IT_LIGHTNING) )
				attacker->ps.wpn[wpLG].ekills++;
		}
	}
}


/*
===========
ClientObituary

called when a player dies
============
*/
extern void ktpro_autotrack_on_death (gedict_t *dude);

void ClientObituary (gedict_t *targ, gedict_t *attacker)
{
	char *deathstring,  *deathstring2;
	char *attackerteam, *targteam;

	// Set it so it should update scores at next attempt.
	k_nochange = 0;

	if( match_in_progress != 2 )
		return; // nothing TODO in non match

    if ( targ->ct != ctPlayer )
		return;

	refresh_plus_scores ();
    
	//ZOID 12-13-96: self.team doesn't work in QW.  Use keys
	attackerteam = getteam(attacker);
	targteam     = getteam(targ);

	StatsHandler(targ, attacker);

	ktpro_autotrack_on_death(targ);

	if ( isRA() ) {
		ra_ClientObituary (targ, attacker);
		return;
	}

	targ->deaths += 1; // somehow dead, bump counter
	if ( (isTeam() || isCTF()) && streq( targteam, attackerteam ) && !strnull( attackerteam ) && targ != attacker )
		attacker->friendly += 1; // bump teamkills counter

// { !!! THIS TELEFRAGS TYPES DOES'T HANDLE TEAM KILLS I DUNNO WHY !!!
	// mortal trying telefrag someone who has 666
	if ( dtTELE2 == targ->deathtype )
	{
		G_bprint (PRINT_MEDIUM, "Satan's power deflects %s's telefrag\n", targ->s.v.netname);

        targ->s.v.frags -= 1;
		logfrag (targ, targ);
		return;
	}

	// double 666 telefrag (can happen often in deathmatch 4)
	if ( dtTELE3 == targ->deathtype )
	{
		G_bprint (PRINT_MEDIUM, "%s was telefragged by %s's Satan's power\n", targ->s.v.netname, attacker->s.v.netname);

		targ->s.v.frags -= 1;
		logfrag (targ, targ);
		return;
	}
// }

    if ( attacker->ct == ctPlayer )  // so, inside this "if" targ and attacker is players
    {
		if ( targ == attacker )
		{
			// killed self

			targ->s.v.frags -= (dtSUICIDE == targ->deathtype ? 2 : 1);
			logfrag (targ, targ);

			if ( dtGL == targ->deathtype ) {
                deathstring = " tries to put the pin back in\n";
			}
			else if ( dtRL == targ->deathtype )
			{
				switch( (int)(g_random() * 2) ) {
					case 0:  deathstring = " discovers blast radius\n"; break;
					default: deathstring = " becomes bored with life\n"; break;
				}
			}
			else if ( dtLG_DIS_SELF == targ->deathtype )
			{
				deathstring = va(" electrocutes %s\n", g_himself(targ));
			}
			else if ( dtSQUISH == targ->deathtype )
			{ //similar code present in case where !(attacker->ct == ctPlayer)
				deathstring = " was squished\n";
			}
			else if ( dtLG_DIS == targ->deathtype )
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
			else if ( dtSUICIDE == targ->deathtype ) {
				deathstring = " suicides\n";
			}
			else
                deathstring = " somehow becomes bored with life\n"; // hm, and how it is possible?

			G_bprint (PRINT_MEDIUM, "%s%s", targ->s.v.netname, deathstring);

            return;
		}
        else if ( (isTeam() || isCTF()) && streq( targteam, attackerteam ) && !strnull( attackerteam ) )
		{
 			// teamkill

			if ( dtTELE1 != targ->deathtype || cvar("k_tp_tele_death") ) {
				// -1 frag always if non "teledeath", and -1 on "teledeath" if allowed
				// also relax this rules on first seconds of match
				if ( g_globalvars.time - match_start_time > 1 ) {
					attacker->s.v.frags -= 1;
					logfrag (attacker, attacker); //ZOID 12-13-96:  killing a teammate logs as suicide
				}
			}

			// some deathtypes have specific death messages

			if ( dtTELE1 == targ->deathtype ) {
				G_bprint (PRINT_MEDIUM, "%s was telefragged by %s teammate\n", targ->s.v.netname, g_his(targ));
				return;
			}
			else if ( dtSQUISH == targ->deathtype ) {
				G_bprint (PRINT_MEDIUM, "%s squished a teammate\n", attacker->s.v.netname);
				return;
			}
			else if ( dtSTOMP == targ->deathtype ) {
				switch( (int)(g_random() * 2) ) {
					case 0:  deathstring = " was jumped by "; break;
					default: deathstring = " was crushed by "; break;
				}

				G_bprint (PRINT_MEDIUM,"%s%s%s teammate\n",	targ->s.v.netname, deathstring, g_his( targ ));
				return;
			}

			// basic death messages

			switch( (int)(g_random() * 4) ) {
				case 0:  deathstring = va(" checks %s glasses\n", g_his(attacker)); break;
				case 1:  deathstring = " loses another friend\n"; break;
				case 2:  deathstring = " gets a frag for the other team\n"; break;
				default: deathstring = " mows down a teammate\n"; break;
			}

			G_bprint (PRINT_MEDIUM, "%s%s", attacker->s.v.netname, deathstring);
			return;
		}
		else
		{	// normal kill, Kteams version
			attacker->s.v.frags += 1;
			logfrag (attacker, targ);

			attacker->victim = targ->s.v.netname;
			targ->killer = attacker->s.v.netname;

			if ( targ->spawn_time + 2 > g_globalvars.time )
				attacker->ps.spawn_frags++;

			if ( isCTF() ) // handle various ctf bonuses
				CTF_Obituary(targ, attacker);

			deathstring2 = "\n"; // default is "\n"

			if ( dtTELE1 == targ->deathtype ) {
				deathstring = " was telefragged by ";
			}
			else if ( dtSQUISH == targ->deathtype )	{
				G_bprint (PRINT_MEDIUM, "%s squishes %s\n", attacker->s.v.netname, targ->s.v.netname);
				return;	// !!! return !!!
			}
			else if ( dtSTOMP == targ->deathtype )	{
				if ( cvar("k_instagib") ) {
					deathstring = " was literally stomped into particles by ";
					deathstring2 = "!\n";
				} else {
					switch( (int)(g_random() * 5) ) {
						case 0:  deathstring = " softens "; deathstring2 = "'s fall\n"; break;
						case 1:  deathstring = " tried to catch "; break;
						case 2:  deathstring = " was jumped by "; break;
						case 3:  deathstring = " was crushed by "; break;
						default:
							 	G_bprint (PRINT_MEDIUM, "%s stomps %s\n", attacker->s.v.netname, targ->s.v.netname);
							 	return; // !!! return !!!
					}
				}
			}
			else if ( dtNG == targ->deathtype )
			{
				switch( (int)(g_random() * 2) ) {
					case 0:  deathstring = " was body pierced by "; break;
					default: deathstring = " was nailed by "; break;
				}
			}
			else if ( dtSNG == targ->deathtype )
			{
				switch ( (int)(g_random() * 3) ) {
					case 0:	 deathstring = " was punctured by "; break;
					case 1:	 deathstring = " was perforated by "; break;
					default: deathstring = " was ventilated by "; break;
				}

				if ( targ->s.v.health < -40 ) // quad modifier
					deathstring = " was straw-cuttered by ";
			}
			else if ( dtGL == targ->deathtype )
			{
				deathstring = " eats ";
				deathstring2 = "'s pineapple\n";

				if ( targ->s.v.health < -40 )
				{
					deathstring = " was gibbed by ";
					deathstring2 = "'s grenade\n";
				}
			}
			else if ( dtRL == targ->deathtype )
			{
				deathstring = ( targ->s.v.health < -40 ? " was gibbed by " : " rides " );
				deathstring2 = "'s rocket\n";

				if ( attacker->super_damage_finished > 0 && targ->s.v.health < -40 )
				{
					switch ( (int)(g_random() * 3) ) {
						case 0: deathstring = " was brutalized by "; break;
						case 1:	deathstring = " was smeared by "; break;
						default:
								G_bprint (PRINT_MEDIUM, "%s rips %s a new one\n", attacker->s.v.netname, targ->s.v.netname);
								return; // !!! return !!!
					}

					deathstring2 = "'s quad rocket\n";
				}
			}
			else if ( dtAXE == targ->deathtype )
			{
				if ( !cvar("k_instagib") ) {
					deathstring = " was ax-murdered by ";
				} else {
					deathstring = " was axed to pieces by ";
					deathstring2 = "!\n";
				}	
			}
            else if ( dtHOOK == targ->deathtype )
			{
				deathstring = " was hooked by ";
			}
			else if ( dtSG == targ->deathtype )
			{
				deathstring = " chewed on ";
				deathstring2 = "'s boomstick\n";

				if ( targ->s.v.health < -40 )
				{
					if ( !cvar("k_instagib") )
						deathstring = " was lead poisoned by ";
					else
						deathstring = " was instagibbed by ";
					deathstring2 = "\n";
				}
			}
			else if ( dtSSG == targ->deathtype )
			{
				if ( attacker->super_damage_finished > 0 )
					deathstring = " ate 8 loads of ";
				else
					deathstring = " ate 2 loads of ";

				if ( cvar("k_instagib") ) {
					deathstring = " was instagibbed by ";
					deathstring2 = "\n";
				} else {
					deathstring2 = "'s buckshot\n";
				}
			}
			else if ( dtLG_BEAM == targ->deathtype )
			{
 				if ( targ->s.v.health < -40 ) { // quad shaft
					deathstring = " gets a natural disaster from ";
					deathstring2 = "\n";
				}
				else { // normal shaft in most cases
					deathstring = " accepts ";
					deathstring2 = "'s shaft\n";
				}
			}
			else if ( dtLG_DIS == targ->deathtype )
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
			else {
				G_cprint ("Unknown death: normal death kt version\n");
				deathstring = " killed by ";
				deathstring2 = " ?\n";
			}

			G_bprint (PRINT_MEDIUM,"%s%s%s%s", targ->s.v.netname, deathstring, attacker->s.v.netname, deathstring2);
		}

		return;
	}
	else // attacker->ct != ctPlayer
	{
        targ->s.v.frags -= 1;            // killed self
		logfrag (targ, targ);

		if ( dtEXPLO_BOX == targ->deathtype )
		{
			deathstring = " blew up\n";
		}
        else if ( dtFALL == targ->deathtype )
		{
			switch( (int)(g_random() * 2) ) {
				case 0:  deathstring = " cratered\n"; break;
				default: deathstring = va(" fell to %s death\n", g_his(targ)); break;
			}
		}
        else if ( dtNG == targ->deathtype || dtSNG == targ->deathtype )
		{
			deathstring = " was spiked\n";
		}
		else if ( dtLASER == targ->deathtype )
		{
			deathstring = " was zapped\n";
		}
		else if ( dtFIREBALL == targ->deathtype )
		{
			deathstring = " ate a lavaball\n";
		}
		else if ( dtCHANGELEVEL == targ->deathtype )
		{
			deathstring = " tried to leave\n";
		}
		else if ( dtSQUISH == targ->deathtype )
		{
			deathstring = " was squished\n";
		}
		else if ( dtWATER_DMG == targ->deathtype )
		{
			switch( (int)(g_random() * 2) ) {
				case 0:  deathstring = " sleeps with the fishes\n"; break;
				default: deathstring = " sucks it down\n"; break;
			}
		}
		else if ( dtSLIME_DMG == targ->deathtype )
		{
			switch( (int)(g_random() * 2) ) {
				case 0:  deathstring = " gulped a load of slime\n"; break;
				default: deathstring = " can't exist on slime alone\n"; break;
			}
		}
		else if ( dtLAVA_DMG == targ->deathtype )
		{
			switch( (int)(g_random() * 2) ) {
				case 0:  deathstring = " turned into hot slag\n"; break;
				default: deathstring = " visits the Volcano God\n"; break;
			}

			if ( targ->s.v.health < -15 )
				deathstring = " burst into flames\n";
		}
		else if ( dtTRIGGER_HURT == targ->deathtype )
		{
			deathstring = " died\n";
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

