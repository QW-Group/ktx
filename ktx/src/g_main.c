/*
 *  QWProgs-DM
 *  Copyright (C) 2004  [sd] angel
 *
 *  This code is based on Q3 VM code by Id Software, Inc.
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
 *  $Id: g_main.c,v 1.23 2006/05/28 04:26:18 qqshka Exp $
 */

#include "g_local.h"
/* global 4 fix
entity          self;
entity          other;
entity          world;
entity          newmis;                         // if this is set, the entity that just
        // run created a new missile that should
        // be simulated immediately
entity          trace_ent;
entity          msg_entity;                             // destination of single entity writes

*/
gedict_t        g_edicts[MAX_EDICTS];	//768
gedict_t       *world = g_edicts;
gedict_t       *self, *other;

//short shortvar=0xfedc;
globalvars_t    g_globalvars;
field_t         expfields[] = {
#ifdef VWEP_TEST
	{"vw_index",    FOFS( vw_index ),    F_FLOAT},
	{"vw_frame",    FOFS( vw_frame ),    F_FLOAT},
#endif
	{"maxspeed",    FOFS( maxspeed ),    F_FLOAT},
	{"gravity",     FOFS( gravity ),     F_FLOAT},
	{"isBot",       FOFS( isBot ),       F_INT},
	{"brokenankle", FOFS( brokenankle ), F_FLOAT},
	{NULL}
};
static char     mapname[64];
static char     worldmodel[64] = "worldmodel";
static char     netnames[MAX_CLIENTS][32];

static wreg_t   wregs[MAX_CLIENTS][MAX_WREGS];

gameData_t      gamedata =
    { ( edict_t * ) g_edicts, sizeof( gedict_t ), &g_globalvars, expfields , GAME_API_VERSION};

float           starttime;
void            G_InitGame( int levelTime, int randomSeed );
void			G_ShutDown();
void            StartFrame( int time );
qboolean        ClientCommand();
qboolean 		ClientUserInfoChanged();
void            G_EdictTouch();
void            G_EdictThink();
void            G_EdictBlocked();
void            ClearGlobals();

qboolean		ClientSay( qboolean isTeamSay );

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
int vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5,
	    int arg6, int arg7, int arg8, int arg9, int arg10, int arg11 )
{
    int api_ver;

	ClearGlobals();
	
	switch ( command )
	{
	case GAME_INIT:
        api_ver = trap_GetApiVersion();

		if ( api_ver == 8 && GAME_API_VERSION == 9 ) {
			// ok, this allow start mod with API_V9 on server with API_V8
			// since versions differ only in one thing GAME_CLIENT_SAY
			gamedata.APIversion = 8;
		}
		else if ( api_ver < GAME_API_VERSION )
		{
			G_cprint("Mod requried API_VERSION %d or higher, server have %d\n", GAME_API_VERSION,api_ver);
			return 0;
		}

		G_InitGame( arg0, arg1 );
		return ( int ) ( &gamedata );

	case GAME_LOADENTS:
		G_SpawnEntitiesFromString();
		return 1;

	case GAME_START_FRAME:
		StartFrame( arg0 );
		return 1;

	case GAME_CLIENT_CONNECT:
		self = PROG_TO_EDICT( g_globalvars.self );

		self->connect_time = g_globalvars.time;

		self->vip = G_FLOAT( OFS_PARM0 ); // mvdsv store vip here, valid only at this moment

		self->k_spectator = arg0;
		self->k_player    = !arg0;

		self->wreg = wregs[(int)(self - world)-1];
		memset( self->wreg, 0, sizeof( wreg_t ) * MAX_WREGS ); // clear

		show_sv_version();

		cmdinfo_infoset ( self );

		if ( arg0 )
			SpectatorConnect();
		else
			ClientConnect();

		update_ghosts();
		
		return 1;

	case GAME_PUT_CLIENT_IN_SERVER:
		self = PROG_TO_EDICT( g_globalvars.self );

		if ( !arg0 )
			PutClientInServer();
		else
			PutSpectatorInServer();

		return 1;

	case GAME_CLIENT_DISCONNECT:
		self = PROG_TO_EDICT( g_globalvars.self );

		s_lr_clear( self );

		cmdinfo_clear ( self ); // remove all keys for this client on disconnect

		if ( arg0 )
			SpectatorDisconnect();
		else
			ClientDisconnect();

		update_ghosts();

		return 1;

	case GAME_SETNEWPARMS:
		SetNewParms( true );
		return 1;

	case GAME_CLIENT_PRETHINK:
		self = PROG_TO_EDICT( g_globalvars.self );

		if ( self->wreg_attack ) // client simulate +attack via "cmd wreg" feature
			self->s.v.button0 = true;

		if ( !arg0 )
			PlayerPreThink();
		return 1;

	case GAME_CLIENT_POSTTHINK:
		self = PROG_TO_EDICT( g_globalvars.self );
		self->k_lastPostThink = g_globalvars.time;

		if ( self->wreg_attack ) // client simulate +attack via "cmd wreg" feature
			self->s.v.button0 = true;

		if ( !arg0 )
			PlayerPostThink();
		else
			SpectatorThink();

		BothPostThink ();

		return 1;

	case GAME_EDICT_TOUCH:
		G_EdictTouch();
		return 1;

	case GAME_EDICT_THINK:
		G_EdictThink();
		return 1;

	case GAME_EDICT_BLOCKED:
		G_EdictBlocked();
		return 1;

	case GAME_SETCHANGEPARMS:
		self = PROG_TO_EDICT( g_globalvars.self );
		SetChangeParms();
		return 1;

	case GAME_CLIENT_COMMAND:

		return ClientCommand();

	case GAME_CLIENT_USERINFO_CHANGED:
		// called on user /cmd setinfo	if value changed
		// return not zero dont allow change
		// params like GAME_CLIENT_COMMAND, but argv(0) always "setinfo" and argc always 3

		self = PROG_TO_EDICT( g_globalvars.self );
		return ClientUserInfoChanged();

	case GAME_SHUTDOWN:
		// called before level change/spawn
		G_ShutDown();
		return 0;

	case GAME_CONSOLE_COMMAND:
		// called on server console command "mod"
		// params like GAME_CLIENT_COMMAND, but argv(0) always "mod"
		return 0;//ConsoleCommand();

	case GAME_CLIENT_SAY:
		// called on user /say or /say_team
		// arg0 non zero if say_team
		// return non zero if say/say_team handled by mod
		// params like GAME_CLIENT_COMMAND

		return ClientSay( arg0 );
	}

	return 0;
}

//===================================================================
void G_Printf( const char *fmt, ... )
{
	va_list         argptr;
	char            text[1024];

	va_start( argptr, fmt );
	vsprintf( text, fmt, argptr );
	va_end( argptr );

	trap_DPrintf( text );
}

void G_Error( const char *fmt, ... )
{
	va_list         argptr;
	char            text[1024];

	va_start( argptr, fmt );
	vsprintf( text, fmt, argptr );
	va_end( argptr );

	trap_Error( text );
}

void Com_Error( int level, const char *error, ... )
{
	va_list         argptr;
	char            text[1024];

	va_start( argptr, error );
	vsprintf( text, error, argptr );
	va_end( argptr );

	G_Error( "%s", text );
}

void Com_Printf( const char *msg, ... )
{
	va_list         argptr;
	char            text[1024];

	va_start( argptr, msg );
	vsprintf( text, msg, argptr );
	va_end( argptr );

	G_Printf( "%s", text );
}

//===================================================================
void G_InitGame( int levelTime, int randomSeed )
{
	int             i;

	srand( randomSeed );
	framecount = 0;
	starttime = levelTime * 0.001;
	G_Printf( "Init Game\n" );
	G_InitMemory();
	memset( g_edicts, 0, sizeof( gedict_t ) * MAX_EDICTS );


	world->s.v.model = worldmodel;
	g_globalvars.mapname = mapname;
	for ( i = 0; i < MAX_CLIENTS; i++ )
	{
		g_edicts[i + 1].s.v.netname = netnames[i];
	}

	Init_cmds();

	// put mod version in serverinfo
	localcmd( "serverinfo \"%s\" \"%s\"\n", MOD_SERVERINFO_MOD_KEY, MOD_VERSION );
	localcmd( "serverinfo \"%s\" \"%05d\"\n", MOD_SERVERINFO_BUILD_KEY, build_number() );
}

void G_ShutDown()
{
	char *map = g_globalvars.mapname;

	AbortElect();

	if ( k_pause )
		ModPause ( 0 );

	if( match_in_progress )
		EndMatch( 1 ); // skip demo, make some other stuff

	cvar_set( "_k_lastmap", ( strnull( map ) ? "" : map ) );
	trap_cvar_set_float( "_k_players", CountPlayers());
	trap_cvar_set_float( "_k_pow_last", Get_Powerups() );

/* removed k_srvcfgmap
	// exec configs/maps/out/mapname.cfg
	if ( cvar( "k_srvcfgmap" ) && strneq( g_globalvars.mapname, name ) ) {
		char *cfg_name = va("configs/maps/out/%s.cfg", g_globalvars.mapname);

		if ( can_exec( cfg_name ) )
			localcmd( "exec %s\n", cfg_name );
	}
*/
}


//===========================================================================
// Physics
// 
//===========================================================================
////////////////
// GlobalParams:
// time
// self
// other
///////////////
void G_EdictTouch()
{
	self = PROG_TO_EDICT( g_globalvars.self );
	other = PROG_TO_EDICT( g_globalvars.other );
	if ( self->s.v.touch )
	{
/*
#ifdef DEBUG
	        if(self->s.v.classname && other->s.v.classname)
	        	if(!strcmp(self->s.v.classname,"player")||!strcmp(other->s.v.classname,"player"))
	         G_bprint(2, "touch %s <-> %s\n", self->s.v.classname,other->s.v.classname);
#endif
*/
		( ( void ( * )() ) ( self->s.v.touch ) ) ();
	} else
	{
		G_Printf( "Null touch func" );
	}
}

////////////////
// GlobalParams:
// time
// self
// other=world
///////////////
void G_EdictThink()
{
	self = PROG_TO_EDICT( g_globalvars.self );
	other = PROG_TO_EDICT( g_globalvars.other );
	if ( self->s.v.think )
	{
		( ( void ( * )() ) ( self->s.v.think ) ) ();
	} else
	{
		G_Printf( "Null think func" );
	}

}

////////////////
// GlobalParams:
// time
// self=pusher
// other=check
// if the pusher has a "blocked" function, call it
// otherwise, just stay in place until the obstacle is gone
///////////////
void G_EdictBlocked()
{
	self = PROG_TO_EDICT( g_globalvars.self );
	other = PROG_TO_EDICT( g_globalvars.other );

	if ( self->s.v.blocked )
	{
		( ( void ( * )() ) ( self->s.v.blocked ) ) ();
	} else
	{
		//G_Printf("Null blocked func");
	}

}


void ClearGlobals()
{
	damage_attacker = damage_inflictor = activator = self = other = newmis = world;
}
