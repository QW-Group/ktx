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
 *  $Id: g_utils.c,v 1.18 2006/02/11 22:11:32 qqshka Exp $
 */

#include "g_local.h"


float CountPlayers();


int NUM_FOR_EDICT( gedict_t * e )
{
	int     b;

	b = ( byte * ) e - ( byte * ) g_edicts;
	b = b / sizeof( gedict_t );

	if ( b < 0 || b >= MAX_EDICTS )
		G_Error( "NUM_FOR_EDICT: bad pointer" );
	return b;
}


float g_random(  )
{
	return ( rand(  ) & 0x7fff ) / ( ( float ) 0x7fff );
}

float crandom(  )
{
	return 2 * ( g_random(  ) - 0.5 );
}

gedict_t *spawn(  )
{
	gedict_t *t = &g_edicts[trap_spawn(  )];

	if ( !t || t == world )
		DebugTrap( "spawn return world\n" );
	return t;
}

void ent_remove( gedict_t * t )
{
	if ( !t || t == world )
		DebugTrap( "BUG BUG BUG remove world\n" );

	if ( NUM_FOR_EDICT( t ) <= MAX_CLIENTS ) // debug
		G_Error ("remove client");

	trap_remove( NUM_FOR_EDICT( t ) );
}



gedict_t *nextent( gedict_t * ent )
{
	int     entn;

	if ( !ent )
		G_Error( "find: NULL start\n" );
	entn = trap_nextent( NUM_FOR_EDICT( ent ) );
	if ( entn )
		return &g_edicts[entn];
	else
		return NULL;
}

/*gedict_t *find( gedict_t * start, int fieldoff, char *str )
{
	gedict_t *e;
	char   *s;

	if ( !start )
		G_Error( "find: NULL start\n" );
	for ( e = nextent( start ); e; e = nextent( e ) )
	{
		s = *( char ** ) ( ( byte * ) e + fieldoff );
		if ( s && !strcmp( s, str ) )
			return e;
	}
	return NULL;
}*/
gedict_t *find( gedict_t * start, int fieldoff, char *str )
{
	return trap_find( start, fieldoff, str );
}


void normalize( vec3_t value, vec3_t newvalue )
{
	float   new;


	new = value[0] * value[0] + value[1] * value[1] + value[2] * value[2];
	new = sqrt( new );

	if ( new == 0 )
		value[0] = value[1] = value[2] = 0;
	else
	{
		new = 1 / new;
		newvalue[0] = value[0] * new;
		newvalue[1] = value[1] * new;
		newvalue[2] = value[2] * new;
	}

}
void aim( vec3_t ret )
{
	VectorCopy( g_globalvars.v_forward, ret );
}

const char    null_str[] = "";

int streq( const char *s1, const char *s2 )
{
	if ( !s1 )
		s1 = null_str;
	if ( !s2 )
		s2 = null_str;
	return ( !strcmp( s1, s2 ) );
}

int strneq( const char *s1, const char *s2 )
{
	if ( !s1 )
		s1 = null_str;
	if ( !s2 )
		s2 = null_str;

	return ( strcmp( s1, s2 ) );
}

int strnull ( const char *s1 )
{
	return (!s1 || !*s1);
}

/*
=================
vlen

scalar vlen(vector)
=================
*/
float vlen( vec3_t value1 )
{
//      float   *value1;
	float   new;


	new = value1[0] * value1[0] + value1[1] * value1[1] + value1[2] * value1[2];
	new = sqrt( new );

	return new;
}

float vectoyaw( vec3_t value1 )
{
	float   yaw;


	if ( value1[1] == 0 && value1[0] == 0 )
		yaw = 0;
	else
	{
		yaw = ( int ) ( atan2( value1[1], value1[0] ) * 180 / M_PI );
		if ( yaw < 0 )
			yaw += 360;
	}

	return yaw;
}


void vectoangles( vec3_t value1, vec3_t ret )
{
	float   forward;
	float   yaw, pitch;


	if ( value1[1] == 0 && value1[0] == 0 )
	{
		yaw = 0;
		if ( value1[2] > 0 )
			pitch = 90;
		else
			pitch = 270;
	} else
	{
		yaw = ( int ) ( atan2( value1[1], value1[0] ) * 180 / M_PI );
		if ( yaw < 0 )
			yaw += 360;

		forward = sqrt( value1[0] * value1[0] + value1[1] * value1[1] );
		pitch = ( int ) ( atan2( value1[2], forward ) * 180 / M_PI );
		if ( pitch < 0 )
			pitch += 360;
	}


	ret[0] = pitch;
	ret[1] = yaw;
	ret[2] = 0;
}


/*
=================
PF_findradius

Returns a chain of entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
gedict_t *findradius( gedict_t * start, vec3_t org, float rad )
{
	gedict_t *ent;
	vec3_t  eorg;
	int     j;

	for ( ent = nextent( start ); ent; ent = nextent( ent ) )
	{
		if ( ent->s.v.solid == SOLID_NOT )
			continue;
		for ( j = 0; j < 3; j++ )
			eorg[j] = org[j] - ( ent->s.v.origin[j] + ( ent->s.v.mins[j] + ent->s.v.maxs[j] ) * 0.5 );
		if ( VectorLength( eorg ) > rad )
			continue;
		return ent;

	}
	return NULL;

}

// just ignore solid field
gedict_t *findradius2( gedict_t * start, vec3_t org, float rad )
{
	gedict_t *ent;
	vec3_t  eorg;
	int     j;

	for ( ent = nextent( start ); ent; ent = nextent( ent ) )
	{
//		if ( ent->s.v.solid == SOLID_NOT )
//			continue;
		for ( j = 0; j < 3; j++ )
			eorg[j] = org[j] - ( ent->s.v.origin[j] + ( ent->s.v.mins[j] + ent->s.v.maxs[j] ) * 0.5 );
		if ( VectorLength( eorg ) > rad )
			continue;
		return ent;

	}
	return NULL;

}


/*
==============
changeyaw

This was a major timewaster in progs, so it was converted to C
==============
*/
void changeyaw( gedict_t * ent )
{
	float   ideal, current, move, speed;

	current = anglemod( ent->s.v.angles[1] );
	ideal = ent->s.v.ideal_yaw;
	speed = ent->s.v.yaw_speed;

	if ( current == ideal )
		return;
	move = ideal - current;
	if ( ideal > current )
	{
		if ( move >= 180 )
			move = move - 360;
	} else
	{
		if ( move <= -180 )
			move = move + 360;
	}
	if ( move > 0 )
	{
		if ( move > speed )
			move = speed;
	} else
	{
		if ( move < -speed )
			move = -speed;
	}

	ent->s.v.angles[1] = anglemod( current + move );
}

/*
==============
PF_makevectors

Writes new values for v_forward, v_up, and v_right based on angles
makevectors(vector)
==============
*/

void makevectors( vec3_t vector )
{
	AngleVectors( vector, g_globalvars.v_forward, g_globalvars.v_right, g_globalvars.v_up );
}

/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/

char	*va(char *format, ...)
{
	va_list		argptr;
	static char		string[MAX_STRINGS][1024];
	static int		index = 0;
	
	index %= MAX_STRINGS;
	va_start (argptr, format);
	vsnprintf (string[index], sizeof(string[0]), format, argptr);
	va_end (argptr);

	string[index][ sizeof( string[0] ) - 1 ] = '\0';

	return string[index++];
}

char *redtext(const char *format, ...)
{
// >>>> like va(...)
	va_list		argptr;
	static char	string[MAX_STRINGS][1024];
	static int		index = 0;
	
	index %= MAX_STRINGS;
	va_start (argptr, format);
	vsnprintf (string[index], sizeof(string[0]), format, argptr);
	va_end (argptr);

	string[index][ sizeof( string[0] ) - 1 ] = '\0';
// <<<<

	{ // convert to red
		unsigned char *i = (unsigned char *) string[index];

		for ( ; *i; i++ )
			if ( *i > 32 && *i < 128 )
				*i |= 128;

		return string[index++];
	}
}

char *dig3(int d)
{
	static char	string[MAX_STRINGS][32];
	static int		index = 0;

	index %= MAX_STRINGS;

	snprintf(string[index], sizeof( string[0] ), "%d", d);
	string[index][ sizeof( string[0] ) - 1 ] = '\0';

	{ // convert to digits
		unsigned char *i = (unsigned char *) (string[index]);

		for ( ; *i; i++ )
				*i += 98;
	}

	return string[index++];
}

char *dig3s(const char *format, ...)
{
// >>>> like va(...)
	va_list		argptr;
	static char	string[MAX_STRINGS][32];
	static int		index = 0;
	
	index %= MAX_STRINGS;
	va_start (argptr, format);
	vsnprintf (string[index], sizeof(string[0]), format, argptr);
	va_end (argptr);

	string[index][ sizeof( string[0] ) - 1 ] = '\0';
// <<<<

	{ // convert to digits
		unsigned char *i = (unsigned char *) (string[index]);

		for ( ; *i; i++ )
				*i += 98;
	}

	return string[index++];
}


/*
==============
print functions
==============
*/
void G_sprint( gedict_t * ed, int level, const char *fmt, ... )
{
	va_list argptr;
	char    text[1024];

	va_start( argptr, fmt );
	vsprintf( text, fmt, argptr );
	va_end( argptr );

	trap_SPrint( NUM_FOR_EDICT( ed ), level, text );
}

void G_bprint( int level, const char *fmt, ... )
{
	va_list argptr;
	char    text[1024];

	va_start( argptr, fmt );
	vsprintf( text, fmt, argptr );
	va_end( argptr );

	trap_BPrint( level, text );
}

void G_cprint( const char *fmt, ... )
{
	va_list argptr;
	char    text[1024*4];

	va_start( argptr, fmt );
	vsprintf( text, fmt, argptr );
	vsnprintf( text, sizeof(text), fmt, argptr );
	va_end( argptr );

	text[sizeof(text)-1] = 0;
	
	trap_conprint( text );
}

void G_centerprint( gedict_t * ed, const char *fmt, ... )
{
	va_list argptr;
	char    text[1024];

	va_start( argptr, fmt );
	vsprintf( text, fmt, argptr );
	va_end( argptr );

	trap_CenterPrint( NUM_FOR_EDICT( ed ), text );
}

// centerprint too all clients
void G_cp2all(const char *fmt, ... )
{
	va_list argptr;
	char    text[1024];

	va_start( argptr, fmt );
	vsnprintf (text, sizeof(text), fmt, argptr);
	text[sizeof(text)-1] = 0;
	va_end( argptr );

	WriteByte(MSG_ALL, SVC_CENTERPRINT);
	WriteString(MSG_ALL, text);
}


void G_dprint( const char *fmt, ... )
{
	va_list argptr;
	char    text[1024];

	va_start( argptr, fmt );
	vsprintf( text, fmt, argptr );
	va_end( argptr );

	trap_DPrintf( text );
}

void localcmd( const char *fmt, ... )
{
	va_list argptr;
	char    text[1024];

	va_start( argptr, fmt );
	vsprintf( text, fmt, argptr );
	va_end( argptr );

	trap_localcmd( text );
}

void stuffcmd( gedict_t * ed, const char *fmt, ... )
{
	va_list argptr;
	char    text[1024];

	va_start( argptr, fmt );
	vsprintf( text, fmt, argptr );
	va_end( argptr );

	trap_stuffcmd( NUM_FOR_EDICT( ed ), text );
}


void setorigin( gedict_t * ed, float origin_x, float origin_y, float origin_z )
{
	trap_setorigin( NUM_FOR_EDICT( ed ), origin_x, origin_y, origin_z );
}

void setsize( gedict_t * ed, float min_x, float min_y, float min_z, float max_x, float max_y, float max_z )
{
	trap_setsize( NUM_FOR_EDICT( ed ), min_x, min_y, min_z, max_x, max_y, max_z );
}

void setmodel( gedict_t * ed, char *model )
{
	trap_setmodel( NUM_FOR_EDICT( ed ), model );
}

void sound( gedict_t * ed, int channel, char *samp, float vol, float att )
{
	trap_sound( NUM_FOR_EDICT( ed ), channel, samp, vol, att );
}

gedict_t *checkclient(  )
{
	return &g_edicts[trap_checkclient(  )];
}
void traceline( float v1_x, float v1_y, float v1_z, float v2_x, float v2_y, float v2_z, int nomonst, gedict_t * ed )
{
	trap_traceline( v1_x, v1_y, v1_z, v2_x, v2_y, v2_z, nomonst, NUM_FOR_EDICT( ed ) );
}

void TraceCapsule( float v1_x, float v1_y, float v1_z, float v2_x, float v2_y, float v2_z, int nomonst, gedict_t * ed ,
			float min_x, float min_y, float min_z, 
			float max_x, float max_y, float max_z)
{
	trap_TraceCapsule( v1_x, v1_y, v1_z, v2_x, v2_y, v2_z, nomonst, NUM_FOR_EDICT( ed ) ,
	min_x, min_y, min_z, max_x, max_y, max_z);
}

int droptofloor( gedict_t * ed )
{
	return trap_droptofloor( NUM_FOR_EDICT( ed ) );
}

int checkbottom( gedict_t * ed )
{
	return trap_checkbottom( NUM_FOR_EDICT( ed ) );
}

void makestatic( gedict_t * ed )
{
	trap_makestatic( NUM_FOR_EDICT( ed ) );
}

void setspawnparam( gedict_t * ed )
{
	trap_setspawnparam( NUM_FOR_EDICT( ed ) );
}

void logfrag( gedict_t * killer, gedict_t * killee )
{
	trap_logfrag( NUM_FOR_EDICT( killer ), NUM_FOR_EDICT( killee ) );
}

// WARNING: this function doest support 'cmd info' keys and this is MUST be so
char *infokey( gedict_t * ed, char *key, char *valbuff, int sizebuff )
{
	trap_infokey( NUM_FOR_EDICT( ed ), key, valbuff, sizebuff );

	return valbuff;
}

char *ezinfokey( gedict_t * ed, char *key )
{
	static char		string[MAX_STRINGS][1024];
	static int		index = 0;

	if ( ed->k_player || ed->k_spectator ) {
		char *v = cmdinfo_getkey( ed, key );

		if ( v ) // key supported so does't need to search in userinfo even key is empty ""
			return v;
	}

	index %= MAX_STRINGS;

	trap_infokey( NUM_FOR_EDICT( ed ), key, string[index], sizeof( string[0] ) );

	return string[index++];
}

int  iKey( gedict_t * ed, char *key )
{
	char		string[128]; // which size will be best?

	if ( ed->k_player || ed->k_spectator ) {
		char *v = cmdinfo_getkey( ed, key );

		if ( v ) // key supported so does't need to search in userinfo even key is empty ""
			return atoi( v );
	}

	trap_infokey( NUM_FOR_EDICT( ed ), key, string, sizeof( string ) );
	return atoi( string );
}

float fKey( gedict_t * ed, char *key )
{
	char		string[128]; // which size will be best?

	if ( ed->k_player || ed->k_spectator ) {
		char *v = cmdinfo_getkey( ed, key );

		if ( v ) // key supported so does't need to search in userinfo even key is empty ""
			return atof( v );
	}

	trap_infokey( NUM_FOR_EDICT( ed ), key, string, sizeof( string ) );
	return atof( string );
}

void WriteEntity( int to, gedict_t * ed )
{
	trap_WriteEntity( to, NUM_FOR_EDICT( ed ) );
}

void WriteByte( int to, int data )
{
	trap_WriteByte( to, data );
}

void WriteShort( int to, int data )
{
	trap_WriteShort( to, data );
}

void WriteString( int to, char *data )
{
	trap_WriteString( to, data );
}

void WriteAngle( int to, float data )
{
	trap_WriteAngle( to, data );
}

void WriteCoord( int to, float data )
{
	trap_WriteCoord( to, data );
}

void disableupdates( gedict_t * ed, float time )
{
	trap_disableupdates( NUM_FOR_EDICT( ed ), time );
}

int walkmove( gedict_t * ed, float yaw, float dist )
{
	gedict_t*saveself,*saveother,*saveactivator;
	int retv;

	saveself	= self ;
	saveother	= other;
	saveactivator = activator;

	retv = trap_walkmove( NUM_FOR_EDICT( ed ), yaw,  dist );

	self 	= saveself;
	other	= saveother;
	activator= saveactivator;
	return retv;
}

float cvar( const char *var )
{
	if ( strnull( var ) )
		G_Error("cvar null");

	return trap_cvar ( var );
}

char *cvar_string( const char *var )
{
	static char		string[MAX_STRINGS][1024];
	static int		index = 0;

	index %= MAX_STRINGS;

	trap_cvar_string( var, string[index], sizeof( string[0] ) );

	return string[index++];
}

void cvar_set( const char *var, const char *val )
{
	if ( strnull( var ) || val == NULL )
		G_Error("cvar_set null");

	trap_cvar_set( var, val );
}

// i'm tired of this shit, so implement this
// return team of edict, edict may has "player" or "ghost" classname
char *getteam( gedict_t * ed )
{
	static char		string[MAX_STRINGS][1024];
	static int		index = 0;
	char 			*team=NULL;

	index %= MAX_STRINGS;

	if ( streq(ed->s.v.classname, "player") || streq(ed->s.v.classname, "spectator") )
		team = ezinfokey(ed, "team");
	else if ( streq(ed->s.v.classname, "ghost") )
		team = ezinfokey(world, va("%d", (int)ed->k_teamnum));
	else {
//		G_Error("getteam: wrong classname %s", ed->s.v.classname);
		team = "";
	}

	string[index][0] = 0;
	strlcat( string[index], team, sizeof( string[0] ) );

	return string[index++];
}

char *getname( gedict_t * ed )
{
	static char		string[MAX_STRINGS][1024];
	static int		index = 0;
	char 			*name=NULL;

	index %= MAX_STRINGS;

	if ( streq(ed->s.v.classname, "player") || streq(ed->s.v.classname, "spectator") )
		name = ed->s.v.netname;
	else if ( streq(ed->s.v.classname, "ghost") )
		name = ezinfokey(world, va("%d", (int)ed->cnt2));
	else
		G_Error("getname: wrong classname %s", ed->s.v.classname);

	string[index][0] = 0;
	strlcat( string[index], name, sizeof( string[0] ) );

	return string[index++];
}

// :)
char *SexStr( gedict_t * ed )
{
	static char		string[MAX_STRINGS][5];
	static int		index = 0;
	char 			*sex="his";

	index %= MAX_STRINGS;

//	if ( !ed->k_player && !ed->k_spectator )
//		G_Error("SexStr: not client, classname %s", ed->s.v.classname);
	if( streq( ezinfokey( ed, "gender" ), "f" ) )
		sex = "her";

	string[index][0] = 0;
	strlcat( string[index], sex, sizeof( string[0] ) );

	return string[index++];
}

// this help me walk from both players and ghosts, made code more simple
// int from = 0;
// gedict_t *p = world ;
// while ( p = find_plr(p, &from) ) {
// ... some code ...
// }

// only already accepted players have classname "player" now

gedict_t *find_plr( gedict_t * start, int *from )
{
	gedict_t *next = find(start, FOFCLSN, *from ? "ghost" : "player" );

	if ( !next && !*from ) {
		*from = 1;
		next = find(world, FOFCLSN, "ghost");
	}

	return next;
}

// this help me walk from both players and specs, made code more simple

gedict_t *find_plrspc( gedict_t * start, int *from )
{
	gedict_t *next = find(start, FOFCLSN, *from ? "spectator" : "player" );

	if ( !next && !*from ) {
		*from = 1;
		next = find(world, FOFCLSN, "spectator");
	}

	return next;
}

gedict_t *player_by_id( int id )
{
	gedict_t *p;

	if ( id < 1 )
		return NULL;

	for ( p = world; p = find( p , FOFCLSN, "player" ); ) {
		if ( id == GetUserID( p ) )
			return p;
	}

	return NULL;
}

gedict_t *player_by_name( const char *name )
{
	gedict_t *p;

	if ( strnull( name ) )
		return NULL;

	for ( p = world; p = find( p , FOFCLSN, "player" ); ) {
		if ( streq(p->s.v.netname, name) )
			return p;
	}

	return NULL;
}

gedict_t *player_by_IDorName( const char *IDname )
{
	gedict_t *p = player_by_id( atoi( IDname ) );

	return (p ? p : player_by_name( IDname ) );
}

char *armor_type( int items )
{
	static char		string[MAX_STRINGS][4];
	static int		index = 0;
	char 			*at;

	index %= MAX_STRINGS;

	if(	     items & IT_ARMOR1 )
		at = "ga";
	else if( items & IT_ARMOR2 )
		at = "ya";
	else if( items & IT_ARMOR3 )
		at = "ra";
	else
		at = "0";

	string[index][0] = 0;
	strlcat( string[index], at, sizeof( string[0] ) );

	return string[index++];
}

qboolean isghost( gedict_t *ed )
{
	return (streq(ed->s.v.classname, "ghost") ? true : false);
}
// gametype >>>
qboolean isDuel( )
{
	return (k_mode == gtDuel ? true : false);
}

qboolean isTeam( )
{
	return (k_mode == gtTeam ? true : false);
}

qboolean isFFA( )
{
	return (k_mode == gtFFA ? true : false);
}

qboolean isUnknown( )
{
	return ((!isDuel() && !isTeam() && !isFFA()) ? true : false);
}

// <<< gametype

void GhostFlag(gedict_t *p)
{
	if ( p && p->ready && streq(p->s.v.classname, "player")
		   && match_in_progress == 2 && !k_matchLess
	   )
		p->k_makeghost = 1;

	return;
}


int GetUserID( gedict_t *p )
{
	if ( !p || (!p->k_player && !p->k_spectator) )
		return 0;

	return iKey(p, "*userid");
}

// get name of player whom spectator 'p' tracking
// if something wrong returned value is ""
char *TrackWhom( gedict_t *p )
{
	static char		string[MAX_STRINGS][32];
	static int		index = 0;
	char 			*name;
	gedict_t 		*goal = NULL;

	index %= MAX_STRINGS;

	if (  p && p->k_spectator
			&& (goal = PROG_TO_EDICT( p->s.v.goalentity )) != world
			&& goal->k_player
       )
		name = getname(goal);
    else
		name = "";

	string[index][0] = 0;
	strlcat( string[index], name, sizeof( string[0] ) );

	return string[index++];
}

int GetHandicap( gedict_t *p )
{
	int hdc = p->ps.handicap < 1 ? 100 : min( 100, p->ps.handicap );

	if ( cvar( "k_lock_hdp" ) )
		return 100;
	else
		return hdc;
}

qboolean SetHandicap( gedict_t *p, int nhdc )
{
	int hdc = GetHandicap( p );

	nhdc = nhdc < 1 ? 100 : min( 100, nhdc );	

	if ( match_in_progress )
		return false;

	if ( cvar( "k_lock_hdp" ) ){
		G_sprint(self, 2, "%s changes are not allowed\n", redtext("handicap"));
		return false;	
	}

	if ( nhdc != hdc ){
		p->ps.handicap = nhdc;
		if ( nhdc == 100 )
			G_bprint(2, "%s turns %s off\n", p->s.v.netname, redtext("handicap"));
		else
			G_bprint(2, "%s uses %s %d%%\n", p->s.v.netname, redtext("handicap"), nhdc);
		return true;
	}

	return false;
}

void changelevel( const char *name )
{
 	if ( strnull( name ) )
		G_Error("changelevel: null");

	if ( k_pause )
		ModPause ( 0 );

	cvar_set( "_k_lastmap", g_globalvars.mapname );
	trap_cvar_set_float( "_k_players", CountPlayers());
	trap_cvar_set_float( "_k_pow_last", Get_Powerups() );

	// exec configs/maps/out/mapname.cfg
	if ( cvar( "k_srvcfgmap" ) && strneq( g_globalvars.mapname, name ) ) {
		localcmd("exec configs/maps/out/%s.cfg\n", g_globalvars.mapname);
//		trap_executecmd ();
	}

	trap_changelevel(name);
}

int Get_Powerups ()
{
	static float k_pow_check = 0;
	static int   k_pow = 0;

	int k_pow_new         = iKey(world, "k_pow");
	int k_pow_min_players = bound(0, cvar( "k_pow_min_players"), 999);
	int k_pow_check_time  = bound(0, cvar( "k_pow_check_time"), 999);

	k_pow_check_time = !k_pow_check_time ? 10 : k_pow_check_time; // default is 10

	if ( !k_pow_new || !k_matchLess || !k_pow_min_players )
		return k_pow = k_pow_new; // no k_pow_min_players if server in normal match mode
								  // no powerups if k_pow == 0
								  // return current value of key 'k_pow' if k_pow_min_players == 0

	if ( k_pow_check > g_globalvars.time )
		return k_pow; // too soon to re-check, so return result of last check

	// ok, time to re-check if powerups is still actual, or we have lack of players

	// because not all players may connected yet, some work around :|
	if ( framecount == 1 ) {
		k_pow_check = g_globalvars.time + 3; // fast re-check. probably time when all players reconnect
		k_pow_new = max(cvar("_k_players"), CountPlayers()) < k_pow_min_players ? 0 : k_pow_new;
		k_pow = cvar( "_k_pow_last" ); // restore k_pow from last level
	}else {
		k_pow_check = g_globalvars.time + k_pow_check_time;
		k_pow_new = CountPlayers() < k_pow_min_players ? 0 : k_pow_new;
	}

	if ( k_pow != k_pow_new ) {
		char *pwr;

		switch ( k_pow_new ) {
			case 0:  pwr = redtext("Off"); break;
			case 1:  pwr = redtext("On");  break;
			case 2:  pwr = redtext("On (Jam)"); break;
			default: pwr = redtext("Unkn"); break;
		}
		
		G_bprint(2, "Server decides turn %s %s\n", redtext("powerups"), pwr);
	}

	return (k_pow = k_pow_new);
}

char *count_s( int cnt )
{
	return (cnt == 1 ? "" : "s");
}

// { some scores stuff

// for team games
int k_scores1 = 0;
int k_scores2 = 0;

// for ffa and duel
gedict_t *ed_scores1 = NULL;
gedict_t *ed_scores2 = NULL;

void ReScores()
{
	gedict_t *p;
	int from;
	char *team1, *team2;

	// DO this by checking if k_nochange is 0. 
	// which is set in ClientObituary in client.c
	if( k_nochange )
		return;

	// ok - scores potentially changed, recalculate

	k_nochange = 1;

	k_scores1 = 0;
	k_scores2 = 0;

	if ( k_showscores ) {
		team1 = ezinfokey( world, "k_team1" );

		for( from = 0, p = world; p = find_plr( p, &from ); ) {
			team2 = getteam(p);

			if( streq( team1, team2 ) )
				k_scores1 += p->s.v.frags;
			else
				k_scores2 += p->s.v.frags;
		}
	}

	ed_scores1 = NULL;
	ed_scores2 = NULL;
	
	if ( ( isDuel() || isFFA() ) && CountPlayers() > 1 ) {
		// no ghost serving
		for ( p = world; p = find( p , FOFCLSN, "player" ); ) {
			if ( !ed_scores1 ) {
				ed_scores1 = p;
				continue;
			}

			if ( !ed_scores2 ) {
				if ( ed_scores1->s.v.frags < p->s.v.frags ) {
					ed_scores2 = ed_scores1;
					ed_scores1 = p;
				}
				else {
					ed_scores2 = p;
				}
				continue;
			}

			if ( ed_scores1->s.v.frags < p->s.v.frags ) {
				ed_scores2 = ed_scores1;
				ed_scores1 = p;
			}
		}

		if ( !ed_scores1 || !ed_scores2 ) {
			ed_scores1 = NULL;
			ed_scores2 = NULL;
		}
	}
}

int get_scores1()
{
	ReScores();

	return k_scores1;
}

int get_scores2()
{
	ReScores();

	return k_scores2;
}

gedict_t *get_ed_scores1()
{
	ReScores();

	return ed_scores1;
}

gedict_t *get_ed_scores2()
{
	ReScores();

	return ed_scores2;
}

// }

