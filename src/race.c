//
// race.c - race implementation, yeah got inspiration from ktpro (c) (tm) (etc)
//

#include "g_local.h"

// ok, lets define some funny types for race

typedef enum
{
	raceWeaponUnknown = 0,	// this must never happens
	raceWeaponNo,			// weapon not allowed. NOTE: must be second in this enum!!!
	raceWeaponAllowed,		// weapon allowed
	raceWeapon2s,			// weapon allowed after 2s
	raceWeaponMAX

} raceWeapoMode_t;

typedef enum
{
	nodeUnknown = 0,		// this node is fucked by lame coding, I mean this must never happens
	nodeStart,				// this node is start of race route
	nodeCheckPoint,			// this node is intermediate
	nodeEnd,				// this node is end of race route
	nodeMAX

} raceRouteNodeType_t;

typedef struct
{
	raceRouteNodeType_t		type;			// race route node type
	vec3_t					ang;			// this is only need for start node, so we can set player view angles
	vec3_t					org;			// node origin in 3D space

} raceRouteNode_t;

#define MAX_ROUTE_NODES		50 // why not

typedef struct
{
	char					name[128]; 				// NOTE: this will probably FUCK QVM!!!
	char					desc[128]; 				// NOTE: this will probably FUCK QVM!!!
	int						cnt;				   	// how much nodes we actually have, no more than MAX_ROUTE_NODES
	float					timeout;				// when timeout of race must occur
	raceWeapoMode_t			weapon;					// weapon mode
	raceRouteNode_t			node[MAX_ROUTE_NODES];	// nodes of this route, fixed array, yeah I'm lazy

} raceRoute_t;

typedef enum
{
	raceNone = 0,		// race is inactive
	raceCD,				// race in count down state
	raceActive,			// race is active

} raceStatus_t;


#define MAX_ROUTES			10 // why not

typedef struct
{
	int						cnt;				// how much we actually have of routes, no more than MAX_ROUTES
	int						active_route;		// which route active right now
	raceRoute_t				route[MAX_ROUTES];	// fixed array of routes

// { count down
	int						cd_cnt;				// 4 3 2 1 GO!
	float					cd_next_think;		//
// }

	int						timeout_setting;	// just how long timeout
	float					timeout;			// when timeout of race must occur
	float					start_time;			// when race was started

// {
	char					top_nick[64];		// NOTE: this will probably FUCK QVM!!!
	int						top_time;
// }

	int						next_race_time;		// used for centerprint, help us print it not each server frame but more rare, each 100ms or something

	qboolean				warned;				// do we warned why we can't start race
	int						next_racer;			// this is queue of racers

	raceWeapoMode_t			weapon;				// weapon mode

	raceStatus_t			status;				// race status

} race_t;


race_t			race; // whole race struct


//============================================
//
// Define some prototypes
//
//============================================

void race_cancel( const char *fmt, ... );
void race_start( qboolean restart, const char *fmt, ... );
void race_unready_all(void);

void race_remove_ent( void );

void race_set_players_movetype_and_etc( void );

char *classname_for_nodeType( raceRouteNodeType_t nodeType );

//============================================

// this is more like a HOOK, doesn't used internally in race.c but used outside,
// outside of race.c file we does't need to know in which exact sate we are, but in some cases inside race.c we need to know 
// is it contdown or active state and this function _doesn't_ help us in such cases. Argh, this comment supposed to make it more clean, but seems I failed.
qboolean isRACE( void )
{
	if ( match_in_progress || match_over )
		return false;

	return !!race.status;
}

//===========================================

int race_time( void )
{
	if ( race.status != raceActive )
		return 0; // count time only when race in state raceActive

	return (g_globalvars.time - race.start_time) * 1000;
}

char *race_route_name( void )
{
	int idx;

	if ( race.cnt < 0 || race.cnt >= MAX_ROUTES )
		G_Error( "race_route_name: race.cnt %d", race.cnt );

	idx = race.active_route - 1;

	if ( idx < 0 || idx >= race.cnt )
		return "custom";

	return race.route[ idx ].name;
}

char *race_route_desc( void )
{
	int idx;

	if ( race.cnt < 0 || race.cnt >= MAX_ROUTES )
		G_Error( "race_route_desc: race.cnt %d", race.cnt );

	idx = race.active_route - 1;

	if ( idx < 0 || idx >= race.cnt )
		return "custom";

	return race.route[ idx ].desc;
}

//============================================

void race_init( void )
{
	memset( &race, 0, sizeof( race ) );

	race.timeout_setting = 20; // default is 20 sec

	race.warned = true;
	race.status = raceNone;

	race.weapon = raceWeaponAllowed;
}

// clean up, so we can start actual match and there will be no some shit around
void race_shutdown( char *msg )
{
	race_cancel( msg );
	race_remove_ent();
	race_unready_all();
}

// make all players not ready for race, silently
void race_unready_all(void)
{
	gedict_t *p;

	for ( p = world; ( p = find_plr( p ) ); )
		p->race_ready = 0;
}


//============================================

qboolean race_route_add_start( void )
{
	if ( race.cnt < 0 || race.cnt >= MAX_ROUTES )
		return false;

	race.route[race.cnt].weapon  = raceWeaponAllowed; // default allowed
	race.route[race.cnt].timeout = 20; // default 20 sec

	return true;
}

void race_route_add_end( void )
{
	if ( race.cnt < 0 || race.cnt >= MAX_ROUTES )
		G_Error( "race_route_add_end: race.cnt %d", race.cnt );

	race.cnt++;
}

qboolean race_add_route_node(float x, float y, float z, float pitch, float yaw, raceRouteNodeType_t	type)
{
	int node_idx;

	if ( race.cnt < 0 || race.cnt >= MAX_ROUTES )
		G_Error( "race_add_route_node: race.cnt %d", race.cnt );

	node_idx = race.route[race.cnt].cnt;

	if ( node_idx < 0 || node_idx >= MAX_ROUTE_NODES )
		return false; // we are full

	race.route[race.cnt].node[node_idx].type = type;
	race.route[race.cnt].node[node_idx].org[0] = x;
	race.route[race.cnt].node[node_idx].org[1] = y;
	race.route[race.cnt].node[node_idx].org[2] = z;
	race.route[race.cnt].node[node_idx].ang[0] = pitch;
	race.route[race.cnt].node[node_idx].ang[1] = yaw;
	race.route[race.cnt].node[node_idx].ang[2] = 0;

	race.route[race.cnt].cnt++; // one more node

	return true;
}

void race_set_route_name( char *name, char *desc )
{
	if ( race.cnt < 0 || race.cnt >= MAX_ROUTES )
		G_Error( "race_set_route_name: race.cnt %d", race.cnt );

	strlcpy( race.route[race.cnt].name, name, sizeof(race.route[0].name) );
	strlcpy( race.route[race.cnt].desc, desc, sizeof(race.route[0].desc) );
}

void race_set_route_timeout( float timeout )
{
	if ( race.cnt < 0 || race.cnt >= MAX_ROUTES )
		G_Error( "race_set_route_timeout: race.cnt %d", race.cnt );

	race.route[race.cnt].timeout = bound(1, timeout, 60 * 60);
}

void race_set_route_weapon_mode( raceWeapoMode_t weapon )
{
	if ( race.cnt < 0 || race.cnt >= MAX_ROUTES )
		G_Error( "race_set_route_weapon_mode: race.cnt %d", race.cnt );

	switch ( weapon )
	{
		case raceWeaponNo:
		case raceWeaponAllowed:
		case raceWeapon2s:
		break; // known

		default: G_Error( "race_set_route_weapon_mode: wrong weapon %d", weapon );
	}

	race.route[race.cnt].weapon = weapon;
}

void race_add_standart_routes( void )
{
	if ( streq( g_globalvars.mapname, "dm1" ) )
	{
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node( -385.2, 1417.6, 24.0, 4.1, 2.7, nodeStart );
		race_add_route_node( -398.8, 1595.6, 28.0,   0,   0, nodeCheckPoint );
		race_add_route_node( -664.7, 1104.4, 67.0,   0,   0, nodeCheckPoint );
		race_add_route_node(   83.6,  630.0, 51.0,   0,   0, nodeCheckPoint );
		race_add_route_node(  939.3, 1311.8, 95.6,   0,   0, nodeCheckPoint );
		race_add_route_node(  405.6, 1565.6, 62.2,   0,   0, nodeCheckPoint );
		race_add_route_node(    4.1, 1437.4, 24.0,   0,   0, nodeEnd );

		race_set_route_name( "áòïõîä äí±", "ya\215ng\215mh\215ssg\215ya" );
		race_set_route_timeout( 30 );
		race_set_route_weapon_mode( raceWeaponNo );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "dm2" ) )
	{
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node( 1309.7,  -903.8, 344.0, 5.2, 87.8, nodeStart );
		race_add_route_node( 1498.8,  -713.5, 184.0,   0,    0, nodeCheckPoint );
		race_add_route_node( 1896.2,  -634.4, 184.0,   0,    0, nodeCheckPoint );
		race_add_route_node( 2613.7,  -218.5, 120.0,   0,    0, nodeCheckPoint );
		race_add_route_node( 2714.6, -1735.3, 120.0,   0,    0, nodeCheckPoint );
		race_add_route_node( 2445.9, -1979.4, 120.0,   0,    0, nodeCheckPoint );
		race_add_route_node( 2246.4, -2473.3, 200.0,   0,    0, nodeEnd );

		race_set_route_name( "Éèí§ó ñõáäòõî", "highrl\215quad\215water\215lower\215ramh" );
		race_set_route_timeout( 35 );
		race_set_route_weapon_mode( raceWeaponNo );

		race_route_add_end();

		//===========
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node( 2114.0,  -100.9,   24.0, -10.4, -109.4, nodeStart );
		race_add_route_node( 2014.6,  -469.5,  140.7,     0,      0, nodeCheckPoint );
		race_add_route_node( 1696.7, -1463.7,   28.7,     0,      0, nodeCheckPoint );
		race_add_route_node( 2612.7,  -677.0,  144.0,     0,      0, nodeCheckPoint );
		race_add_route_node( 2304.8,  -152.6,   67.6,     0,      0, nodeCheckPoint );
		race_add_route_node( 2397.6,    86.0,   41.2,     0,      0, nodeCheckPoint );
		race_add_route_node( 2246.2,  -194.7, -136.0,     0,      0, nodeEnd );

		race_set_route_name( "áòïõîä äí²", "water\215quadlow\215rllow\215ng\215stairs" );
		race_set_route_timeout( 35 );
		race_set_route_weapon_mode( raceWeaponAllowed );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "dm3" ) )
	{
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node( 1880.1,  884.2, -213.0, 8.3, -92.1, nodeStart );
		race_add_route_node(  701.0,  237.5,   89.2,   0,     0, nodeCheckPoint );
		race_add_route_node(  261.0, -709.0,  328.0,   0,     0, nodeEnd );

		race_set_route_name( "òáæìù", "pentmh\215window\215ratop" );
		race_set_route_timeout( 35 );
		race_set_route_weapon_mode( raceWeaponAllowed );

		race_route_add_end();

		//===========
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node( 1499.6,  498.9,  -88.0, 18.4, -3.4, nodeStart );
		race_add_route_node( 1400.3,  -30.1, -168.0,    0,    0, nodeCheckPoint );
		race_add_route_node(  581.8,    0.2, -168.0,    0,    0, nodeCheckPoint );
		race_add_route_node(  261.0, -709.0,  328.0,    0,    0, nodeEnd );

		race_set_route_name( "òáòõî", "rl\215center\215ratop" );
		race_set_route_timeout( 30 );
		race_set_route_weapon_mode( raceWeaponNo );

		race_route_add_end();

		//===========
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node( 1520.0,  432.0, -88.0, 0, 0, nodeStart );
		race_add_route_node( 1252.7, -513.5,  20.1, 0, 0, nodeCheckPoint );
		race_add_route_node(  642.0, -511.0,  88.0, 0, 0, nodeCheckPoint );
		race_add_route_node(  590.5,  125.6,  96.6, 0, 0, nodeCheckPoint );
		race_add_route_node(  202.7,  593.8,  56.0, 0, 0, nodeCheckPoint );
		race_add_route_node( -711.8,   95.7, 206.4, 0, 0, nodeCheckPoint );
		race_add_route_node( -221.7, -561.6,  20.6, 0, 0, nodeCheckPoint );
		race_add_route_node(  261.0, -709.0, 328.0, 0, 0, nodeEnd );

		race_set_route_name( "áòïõîääí³", "rl\215hibridge\215quadya\215quadunderlifts\215sngmh\215ratop" );
		race_set_route_timeout( 50 );
		race_set_route_weapon_mode( raceWeaponAllowed );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "dm4" ) )
	{
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node( 1199.9, -603.0,  24.0, 1.8, 178.4, nodeStart );
		race_add_route_node( -144.0, -227.0, -72.0,   0,     0, nodeEnd );

		race_set_route_name( "âáóå äí´", "ya\215quadtele" );
		race_set_route_timeout( 15 );
		race_set_route_weapon_mode( raceWeaponNo );

		race_route_add_end();

		//===========
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node( 1199.9,  -603.0,   24.0, 1.8, 178.4, nodeStart );
		race_add_route_node(  337.8,  -673.6,   36.7,   0,     0, nodeCheckPoint );
		race_add_route_node(  296.6, -1205.2,    9.5,   0,     0, nodeCheckPoint );
		race_add_route_node(  336.2,  -664.8, -104.0,   0,     0, nodeCheckPoint );
		race_add_route_node(  -68.6,   611.1, -296.0,   0,     0, nodeEnd );

		race_set_route_name( "íèòõî", "ya\215toptele\215gl\215mh" );
		race_set_route_timeout( 20 );
		race_set_route_weapon_mode( raceWeaponNo );

		race_route_add_end();

		//===========
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node( 1199.9,  -603.0,   24.0, 1.8, 178.4, nodeStart );
		race_add_route_node(  337.8,  -673.6,   36.7,   0,     0, nodeCheckPoint );
		race_add_route_node(  296.6, -1205.2,    9.5,   0,     0, nodeCheckPoint );
		race_add_route_node(  336.2,  -664.8, -104.0,   0,     0, nodeCheckPoint );
		race_add_route_node(  360.2,   -77.6,  -60.3,   0,     0, nodeCheckPoint );
		race_add_route_node(  200.5,   -41.5, -104.0,   0,     0, nodeCheckPoint );
		race_add_route_node( -129.3,  -580.4,  -72.0,   0,     0, nodeEnd );

		race_set_route_name( "ñõáäóôòáæå", "ya\215toptele\215gl\215lg\215quad\215quadtele" );
		race_set_route_timeout( 20 );
		race_set_route_weapon_mode( raceWeaponNo );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "dm5" ) )
	{
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node( 657.5, -296.0, 120.0, 11.7, -87.8, nodeStart );
		race_add_route_node( 205.1, -778.5, 120.0,    0,     0, nodeCheckPoint );
		race_add_route_node( -87.4,  509.4,  79.5,    0,     0, nodeCheckPoint );
		race_add_route_node( 496.0,  369.6, 222.4,    0,     0, nodeCheckPoint );
		race_add_route_node( -84.3, -760.4, 246.6,    0,     0, nodeCheckPoint );
		race_add_route_node( 874.3, -210.2, 232.6,    0,     0, nodeCheckPoint );
		race_add_route_node( 141.1, -223.6, 216.0,    0,     0, nodeEnd );

		race_set_route_name( "áòïõîä äíµ", "ya\215gl\215rl\215telerl\215abovegl\215sng" );
		race_set_route_timeout( 35 );
		race_set_route_weapon_mode( raceWeaponNo );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "dm6" ) )
	{
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node(   55.2, -2005.1,  88.0, -15.2, 49.0, nodeStart );
		race_add_route_node(  450.2, -1484.5, 256.0,     0,    0, nodeCheckPoint );
		race_add_route_node( 1735.0,  -345.0, 168.0,     0,    0, nodeEnd );

		race_set_route_name( "âáóå äí¶", "ga\215gl\215ra" );
		race_set_route_timeout( 15 );
		race_set_route_weapon_mode( raceWeaponAllowed );

		race_route_add_end();

		//===========
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node(  217.3, -1188.5, 112.0, 3.4, 43.0, nodeStart );
		race_add_route_node( 1359.7,  -427.7,  40.0,   0,    0, nodeCheckPoint );
		race_add_route_node( 1108.6, -1316.2, 256.0,   0,    0, nodeCheckPoint );
		race_add_route_node( 1015.9,  -867.3, 256.0,   0,    0, nodeCheckPoint );
		race_add_route_node(  813.4, -1081.0, 256.0,   0,    0, nodeCheckPoint );
		race_add_route_node( 1632.2,  -160.4, 168.0,   0,    0, nodeEnd );

		race_set_route_name( "ð³§ó tuberun", "mh\215rlra\215glcircle\215ra" );
		race_set_route_timeout( 25 );
		race_set_route_weapon_mode( raceWeaponNo );

		race_route_add_end();
	}
}

//============================================

gedict_t *race_get_racer( void )
{
	gedict_t *p;

	for ( p = world; ( p = find_plr( p ) ); )
		if ( p->racer )
			return p;

	return NULL;
}

//===========================================

// remove all entitys with particular classname
void ent_remove_by_classname( char *classname )
{
	gedict_t *e;

	for ( e = world; ( e = ez_find( e, classname ) ); )
		ent_remove( e );
}

// remove all possibile race entitys
void race_remove_ent( void )
{
	int i;

	for ( i = 1; i < nodeMAX; i++ )
		ent_remove_by_classname( classname_for_nodeType( i ) );
}

//============================================

char *race_weapon_mode( void )
{
	switch ( race.weapon )
	{
		case raceWeaponNo:
		return "no";

		case raceWeaponAllowed:
		return "allowed";

		case raceWeapon2s:
		return "after 2s";

		default: G_Error( "race_weapon_mode: wrong race.weapon %d", race.weapon );
	}

	return ""; // keep compiler silent
}

qboolean race_weapon_allowed( gedict_t *p )
{
	if ( !race.status )
		return true; // not a race, so allowed

	// below is case of RACE is somehow in progress

	if ( race.status != raceActive )
		return false; // allowe weapon in active state only

	if ( !p->racer )
		return false; // not a racer

	switch ( race.weapon )
	{
		case raceWeaponNo:
		return false;

		case raceWeaponAllowed:
		return true;

		case raceWeapon2s:
		return ( race_time() >= 2000 ? true : false );

		default: G_Error( "race_weapon_allowed: wrong race.weapon %d", race.weapon );
	}

	return false; // keep compiler silent
}

//============================================

char *name_for_nodeType( raceRouteNodeType_t nodeType )
{
	switch ( nodeType )
	{
		case nodeStart:			return "start checkpoint";
		case nodeCheckPoint:	return "checkpoint";
		case nodeEnd:			return "end checkpoint";

		default:	G_Error( "name_for_nodeType: wrong nodeType %d", nodeType );
	}

	return ""; // keep compiler silent
}

char *classname_for_nodeType( raceRouteNodeType_t nodeType )
{
	switch ( nodeType )
	{
		case nodeStart:			return "race_cp_start";
		case nodeCheckPoint:	return "race_cp";
		case nodeEnd:			return "race_cp_end";

		default:	G_Error( "classname_for_nodeType: wrong nodeType %d", nodeType );
	}

	return ""; // keep compiler silent
}

char *model_for_nodeType( raceRouteNodeType_t nodeType )
{
	switch ( nodeType )
	{
		case nodeStart:			return "progs/invulner.mdl";
		case nodeCheckPoint:	return "progs/w_s_key.mdl";
		case nodeEnd:			return "progs/invulner.mdl";

		default:	G_Error( "model_for_nodeType: wrong nodeType %d", nodeType );
	}

	return ""; // keep compiler silent
}

char *touch_sound_for_nodeType( raceRouteNodeType_t nodeType )
{
	switch ( nodeType )
	{
		case nodeStart:			return "items/protect3.wav";
		case nodeCheckPoint:	return "items/damage.wav";
		case nodeEnd:			return "items/suit.wav";

		default:	G_Error( "touch_sound_for_nodeType: wrong nodeType %d", nodeType );
	}

	return ""; // keep compiler silent
}

char *spawn_sound_for_nodeType( raceRouteNodeType_t nodeType )
{
	switch ( nodeType )
	{
		case nodeStart:			return "items/protect.wav";
		case nodeCheckPoint:	return "items/itembk2.wav";
		case nodeEnd:			return "items/protect.wav";

		default:	G_Error( "spawn_sound_for_nodeType: wrong nodeType %d", nodeType );
	}

	return ""; // keep compiler silent
}

float volume_for_touch_sound_for_nodeType( raceRouteNodeType_t nodeType )
{
	switch ( nodeType )
	{
		case nodeStart:			return 0.5f;
		case nodeCheckPoint:	return 0.3;
		case nodeEnd:			return 0.5f;

		default:	G_Error( "volume_for_touch_sound_for_nodeType: wrong nodeType %d", nodeType );
	}

	return 1; // keep compiler silent
}


float blink_effects_for_nodeType( raceRouteNodeType_t nodeType )
{
	switch ( nodeType )
	{
		case nodeStart:			return ( EF_RED | EF_BLUE );
		case nodeCheckPoint:	return EF_BLUE;
		case nodeEnd:			return EF_RED;

		default:	G_Error( "volume_for_touch_sound_for_nodeType: wrong nodeType %d", nodeType );
	}

	return 0; // keep compiler silent
}

//============================================

// count start + end + intermediate checkpoints
int checkpoints_count( void )
{
	int cnt = 0, i;

	for ( i = 1; i < nodeMAX; i++ )
		cnt += find_cnt( FOFCLSN, classname_for_nodeType( i ) );

	return cnt;
}

//============================================

// count ready for race players
int race_count_ready_players( void )
{
	int cnt;
	gedict_t *p;

	for ( cnt = 0, p = world; ( p = find_plr( p ) ); )
		if ( p->race_ready )
			cnt++;

	return cnt;
}

//===========================================

void race_brighten_checkpoints( void )
{
	int i;
	gedict_t *e;
	gedict_t *racer = race_get_racer();

	if ( !racer )
		return;

	for ( i = 1; i < nodeMAX; i++ )
	{
		char *classname = classname_for_nodeType( i );

		for ( e = world; ( e = ez_find( e, classname ) ); )
		{
			e->s.v.effects   = 0; // remove effects
			e->s.v.nextthink = 0; // stop thinking

			if ( racer->race_id && e->race_id == racer->race_id )
				e->s.v.effects = EF_DIMLIGHT; // set some light for next checkpoint
		}
	}
}

void race_blink_think ()
{
	// remove "cute" effects
	self->s.v.effects = 0;
}

void race_blink_node( gedict_t *e )
{
	// set some "cute" effects
	e->s.v.effects = e->race_effects;

	// reset it to something normal late
	e->s.v.nextthink = g_globalvars.time + 1.8;
	e->s.v.think = ( func_t ) race_blink_think;
}


//===========================================

void race_sprint_checkpoint( gedict_t *player, gedict_t *cp )
{
	if ( cp->race_RouteNodeType == nodeCheckPoint )
		G_sprint( player, 2, "%s \220%d\221\n", redtext( name_for_nodeType( cp->race_RouteNodeType ) ), cp->race_id );
	else
		G_sprint( player, 2, "%s\n", redtext( name_for_nodeType( cp->race_RouteNodeType ) ) );
}

void race_node_touch()
{
	if ( other->ct != ctPlayer )
		return;

	if ( !race.status ) // race in some idle state
	{
		if ( self->attack_finished >= g_globalvars.time )
			return; // not ready

		self->attack_finished = g_globalvars.time + 5; // allow touch it again but later

		// set "cute" effects
		race_blink_node( self );

		// do some sound
		sound( other, CHAN_ITEM, self->s.v.noise, self->race_volume, ATTN_NONE );

		// sprint
		race_sprint_checkpoint( other, self );
	}

	// ok, race in some NON idle state

	if ( race.status != raceActive )
		return; // we are must be in coutdown state, so nothing to do here

	if ( !other->racer )
		return; // do not allow touch checkpoint if not a raced player

	if ( self->race_id < other->race_id )
	{
		// racer must be alredy touched this checkpoint
		//G_sprint( other, 2, "Alredy touched this checkpoint \220%d\221\n", self->race_id );
		return;
	}

	if ( self->race_id == other->race_id )
	{
		// racer touched checkpoint in right order

		if ( self->race_RouteNodeType == nodeEnd )
		{
			// racer touch end node, FINISH!
			if ( race_time() < race.top_time )
			{
				// do some sound
				sound( other, CHAN_ITEM, "boss2/sight.wav", 1, ATTN_NONE );

				// save nick
				strlcpy(race.top_nick, other->s.v.netname, sizeof(race.top_nick));
				// save best time
				race.top_time = race_time();

				race_start( true, "Race %s in %sms,\n"
								  "new top time by %s\n", redtext( "finished" ), dig3( race_time() ), other->s.v.netname );
			}
			else if ( race_time() == race.top_time )
			{
				// do some sound
				sound( other, CHAN_ITEM, "boss2/sight.wav", 1, ATTN_NONE );

				race_start( true, "Race %s in %sms, equal top time\n", redtext( "finished" ), dig3( race_time() ) );
			}
			else
			{
				// do some sound
				sound( other, CHAN_ITEM, "ambience/thunder1.wav", 1, ATTN_NONE );

				race_start( true, "Race %s in %sms\n", redtext( "finished" ), dig3( race_time() ) );
			}

			return;
		}

		// if its not start checkpoint do something "cute"
		if ( self->race_id )
		{
			// do some sound
			sound( other, CHAN_ITEM, "knight/sword2.wav", 1, ATTN_NONE );

			race_sprint_checkpoint( other, self );
		}

		other->race_id++; // bump our id, so we can touch next checkpoint

		race_brighten_checkpoints(); // set light on next checkpoint

		return;
	}

	if ( self->race_id > other->race_id )
	{
		// racer touched checkpoint in WRONG order

		// do some sound
		sound( other, CHAN_ITEM, "boss2/idle.wav", 1, ATTN_NONE );

		if ( self->race_RouteNodeType == nodeCheckPoint )
			race_start( true, "Race restarted, %s \220%d\221 in wrong order\n", redtext( name_for_nodeType( self->race_RouteNodeType ) ), self->race_id );
		else
			race_start( true, "Race restarted, %s in wrong order\n", redtext( name_for_nodeType( self->race_RouteNodeType ) ) );

		return;
	}
}


//===========================================

// this will fix id for end checkpoint
void race_fix_end_checkpoint( void )
{
	// get end checkpoint
	gedict_t *e = ez_find( world, classname_for_nodeType( nodeEnd ) );

	if ( e )
		e->race_id = 1 + find_cnt( FOFCLSN, classname_for_nodeType( nodeCheckPoint ) ); // pretty simple logic
}

// spawn/putInGame race route node
gedict_t *spawn_race_node( raceRouteNode_t *node )
{
	gedict_t *e;
	char *classname = classname_for_nodeType( node->type );

	if ( checkpoints_count() >= MAX_ROUTE_NODES )
		G_Error( "spawn_race_node: can't add more, unexpected" );

	// free previos points if any, except intermediate checkpoint
	if ( node->type != nodeCheckPoint )
		ent_remove_by_classname( classname );

	e = spawn();

	switch ( node->type )
	{
		case nodeCheckPoint:
			e->race_id = 1 + find_cnt( FOFCLSN, classname ); // pretty simple logic
			break;

		case nodeStart:			
		case nodeEnd:
			break;

		default:	G_Error( "spawn_race_node: wrong node->type %d", node->type );
	}

	setmodel( e, model_for_nodeType( node->type ) );
	setsize( e, PASSVEC3( VEC_HULL_MIN ), PASSVEC3( VEC_HULL_MAX ) );
	e->s.v.solid		= SOLID_TRIGGER;
	e->s.v.movetype		= MOVETYPE_FLY;
	e->s.v.flags		= FL_ITEM;
	e->s.v.classname	= classname;
	e->s.v.noise		= touch_sound_for_nodeType( node->type );
	e->race_volume		= volume_for_touch_sound_for_nodeType( node->type );
	e->race_effects		= blink_effects_for_nodeType( node->type );
	e->s.v.touch		= ( func_t ) race_node_touch;
	e->attack_finished  = g_globalvars.time + 1; // + 1 so it not touched immidiatelly, look race_node_touch() for more info
	e->race_RouteNodeType = (int)node->type; // ah, cast

	// play spawn sound
	sound( e, CHAN_AUTO, spawn_sound_for_nodeType( node->type ), 1, ATTN_NONE );

	VectorCopy( node->ang, e->s.v.v_angle );

	setorigin( e, PASSVEC3( node->org ) );

	// this will fix id for end checkpoint
	race_fix_end_checkpoint();

	return e;
}

//============================================

// set some race related fields for all players
void race_clear_race_fields( void )
{
	gedict_t *p;

	for ( p = world; ( p = find_plr( p ) ); )
	{
		p->racer = false;
		p->race_id = 0;
	}
}

void race_cancel( const char *fmt, ... )
{
	va_list argptr;
	char    text[1024];

	va_start( argptr, fmt );
	Q_vsnprintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	G_cp2all( "" ); // clear centerprint

	if ( !strnull( text ) )
		G_bprint( 2, "%s", text );

	race_clear_race_fields();

	race.status = raceNone;
	race.warned = true;

	// set proper move type for players
	race_set_players_movetype_and_etc();
}

//============================================
//
// RACE QUEUE helpers
//
//============================================

// for internal usage
static gedict_t *_race_line( int offset )
{
	int i, idx;
	int c = max(0, race.next_racer + offset);
	gedict_t *p = g_edicts + 1; // p - start of players

	for( i = 0; i < MAX_CLIENTS; i++ )
	{
		idx = ( c + i ) % MAX_CLIENTS;

		if ( p[idx].ct == ctPlayer && p[idx].race_ready )
		{
			race.next_racer = idx;
			return &(p[idx]);
		}
	}

	return NULL;
}

// get someone from race queue line
gedict_t *race_get_from_line( void )
{
	return _race_line( 0 );
}

// this dude will be next in line, in most cases
gedict_t *race_set_next_in_line( void )
{
	return _race_line( 1 );
}

//============================================

qboolean race_can_go( qboolean cancel )
{
	// can't go on, noone ready
	if ( !race_count_ready_players() )
	{
		if ( cancel )
			race_cancel( "Race canceled, no ready players\n" );

		return false;
	}

	// can't go on, no start checkpoint
	if ( !ez_find( world, classname_for_nodeType( nodeStart ) ) )
	{
		if ( cancel )
			race_cancel( "Race canceled, no %s\n", name_for_nodeType( nodeStart ) );

		return false;
	}

	// can't go on, no end checkpoint
	if ( !ez_find( world, classname_for_nodeType( nodeEnd ) ) )
	{
		if ( cancel )
			race_cancel( "Race canceled, no %s\n", name_for_nodeType( nodeEnd ) );

		return false;
	}

	if ( race.status )
	{
		if ( !race_get_racer() )
		{
			if ( cancel )
				race_start( true, "Race restarted, racer not found\n" );

			return false;
		}
	}

	if ( race.status == raceActive )
	{
		if ( race.timeout < g_globalvars.time )
		{
			if ( cancel )
			{
				// do some sound
				sound( world, CHAN_ITEM, "boss2/idle.wav", 1, ATTN_NONE );

				race_start( true, "Race restarted, timeout\n" );
			}

			return false;
		}
	}

	return true;
}

//
// qboolean restart:
// true  - means continue current competition, just select next racer in line, keep best results.
// false - means start completely new race, reset best result and etc.
void race_start( qboolean restart, const char *fmt, ... )
{
	va_list argptr;
	char    text[1024];
	extern	void ktpro_autotrack_on_race_status_changed (void);

	gedict_t *r, *n, *s;

	va_start( argptr, fmt );
	Q_vsnprintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	// cancel it first, this will clear something what probably was't cleared before
	race_cancel( text );

	// switch status to coutdown
	race.status = raceCD;

	// set countdown timers
	race.cd_cnt = 4;
	race.cd_next_think = g_globalvars.time;

	if ( !race.timeout_setting )
		race.timeout_setting = 20; // default is 20 s

	// if this is not restart, then reset best time
	if ( !restart )
	{
		race.top_nick[0] = 0;
		race.top_time = 999999999;
	}

	r = race_get_from_line();

	if ( !r )
	{
		race_shutdown( "race_start: race_get_from_line() == NULL, shutdown race\n" );
		return;
	}

	n = race_set_next_in_line();

	if ( !n )
	{
		race_shutdown( "race_start: race_set_next_in_line() == NULL, shutdown race\n" );
		return;
	}

	s = ez_find( world, classname_for_nodeType( nodeStart ) );

	if ( !s )
	{
		race_shutdown( "race_start: can't find start checkpoint, shutdown race\n" );
		return;
	}

	// mark him as racer
	r->racer = true;

	// clear velocity

	SetVector( r->s.v.velocity, 0, 0, 0 );

	// set proper angles
	VectorCopy(s->s.v.v_angle, r->s.v.angles);
	VectorCopy(s->s.v.v_angle, r->s.v.v_angle);
	r->s.v.fixangle = true;

	// set proper origin
	setorigin( r, PASSVEC3( s->s.v.origin ) );

	// telefrag anyone at this origin
	play_teleport( r );
	spawn_tdeath( r->s.v.origin, r );

	if ( n != r )
	{
		G_bprint( 2, "%s is next in line\n", n->s.v.netname );
	}

	// set light on next checkpoint
	race_brighten_checkpoints();

	// set proper move type for players
	race_set_players_movetype_and_etc();

	// remove some projectiles
	remove_projectiles();

	// autotrack - force switch pov to racer
	ktpro_autotrack_on_race_status_changed();
}


//============================================
//
// RACE "THINK"
//
//============================================

// well, this set some fields on players which help to not block racer
void race_set_one_player_movetype_and_etc( gedict_t *p )
{
	if ( match_in_progress || match_over )
		return;

	switch ( race.status )
	{
		case raceNone:
		p->s.v.movetype	= MOVETYPE_WALK;
		p->s.v.solid 	= SOLID_SLIDEBOX;
		setmodel( p, "progs/player.mdl" );

		break;

		case raceCD:
		p->s.v.movetype	= ( p->racer ? MOVETYPE_NONE : MOVETYPE_FLY );
		p->s.v.solid	= ( p->racer ? SOLID_SLIDEBOX : SOLID_NOT );
		setmodel( p, ( p->racer ? "progs/player.mdl" : "" ) );

		break;

		case raceActive:
		p->s.v.movetype	= ( p->racer ? MOVETYPE_WALK : MOVETYPE_FLY );
		p->s.v.solid	= ( p->racer ? SOLID_SLIDEBOX : SOLID_NOT );
		setmodel( p, ( p->racer ? "progs/player.mdl" : "" ) );

		break;

		default:

		G_Error( "race_set_one_player_movetype_and_etc: unknown race.status %d", race.status );
	}
}

void race_set_players_movetype_and_etc( void )
{
 	gedict_t *p;

	for ( p = world; ( p = find_plr( p ) ); )
		race_set_one_player_movetype_and_etc( p );
}

void race_think( void )
{
	gedict_t *racer;
	gedict_t *p;

	if ( match_in_progress || match_over )
	{
		race.status = raceNone;
		return;
	}

	switch ( race.status )
	{
		case raceNone:

		// can't start
		if ( !race_can_go( !race.warned ) )
			return;

		// advance race status to countdown
		race_start( false, "" );

		return;

		case raceCD:

		// something wrong
		if ( !race_can_go( true ) )
			return;

		// countdown in progress
		if ( race.cd_next_think >= g_globalvars.time )
			return;

		racer = race_get_racer();
		// must not never happens because we have race_can_go() above
		if ( !racer )
		{
			race_start( true, "Race restarted, racer vanishes\n" );
			return;
		}

		// countdown still in progress
		if ( race.cd_cnt > 0 )
		{
			if ( race.cd_next_think < g_globalvars.time )
			{
				char cp_buf[1024] = { 0 }, tmp[512] = { 0 };

				gedict_t *racer = race_get_racer();


				// ok, time for next "tick" in coutdown
				snprintf( cp_buf, sizeof( cp_buf ),	"Race in: %s\n\n"
						 							"Racer: %s\n\n", dig3( race.cd_cnt ), racer->s.v.netname );

				snprintf ( tmp, sizeof( tmp ), "Weapon: %s\n", redtext( race_weapon_mode() ) );
				strlcat( cp_buf, tmp, sizeof( cp_buf ) );

				if ( !strnull( race.top_nick ) )
				{
					snprintf ( tmp, sizeof( tmp ),	"Top time: %sms\n"
													"Top player: %s\n", dig3( race.top_time ), race.top_nick );

					strlcat( cp_buf, tmp, sizeof( cp_buf ) );
				}

				G_cp2all( cp_buf );

				// FIXME: yeah, nice make some utility for that
				for( p = world; (p = find_client( p )); )
					stuffcmd (p, "play buttons/switch04.wav\n");

				race.cd_next_think = g_globalvars.time + 1; // set next think one second later
				race.cd_cnt--;
			}
			
			return;
		}

		G_cp2all("Go!");

		// FIXME: yeah, nice make some utility for that
		for( p = world; (p = find_client( p )); )
			stuffcmd (p, "play enforcer/enfire.wav\n");

		race.status = raceActive; // countdown ends, now we ready for race

		race.start_time		= g_globalvars.time;
		race.timeout		= g_globalvars.time + max( 1 , race.timeout_setting );
		race.next_race_time = 500; // do not print race time for first 500 ms, so we can see "Go!"

		// set proper move type for players
		race_set_players_movetype_and_etc();

		return;

		case raceActive:

		// something wrong
		if ( !race_can_go( true ) )
			return;

		if ( race_time() >= race.next_race_time )
		{
			race.next_race_time = race_time() + 100; // update race time each 100 ms
			G_cp2all( "%s", dig3( race_time() ) );
		}

		return;

		default:

		G_Error( "race_think: unknown race.status %d", race.status );
	}
}

//============================================
//
// race commands
//
//============================================

void race_route_now_custom( void )
{
	race.active_route = 0; // mark this is a custom route now
}

void r_Xset( float t )
{
	gedict_t				*e;
	raceRouteNode_t			node;

	if ( match_in_progress || match_over || race.status )
		return;

	if ( check_master() )
		return;

	if ( checkpoints_count() >= MAX_ROUTE_NODES )
	{
		G_sprint( self, 2, "Can't add more checkpoints!\n" );
		return;
	}

	// zeroing all fields
	memset( &node, 0, sizeof(node) );

	node.type = (raceRouteNodeType_t) t; // magic cast
	VectorCopy( self->s.v.v_angle, node.ang );
	VectorCopy( self->s.v.origin, node.org );

	e = spawn_race_node( &node );

	if ( node.type == nodeCheckPoint )
		G_bprint( 2, "%s \220%d\221 set\n", redtext( name_for_nodeType( node.type ) ), e->race_id );
	else
		G_bprint( 2, "%s set\n", redtext( name_for_nodeType( node.type ) ) );

	race_route_now_custom();  // mark this is a custom route now
}

void r_ccdel( )
{
	gedict_t				*e;
	int						cnt, id;
	char					*classname = classname_for_nodeType( nodeCheckPoint );

	if ( match_in_progress || match_over || race.status )
		return;

	if ( check_master() )
		return;

	cnt = find_cnt( FOFCLSN, classname );

	if ( !cnt )
	{
		G_sprint( self, 2, "Can't find any %s\n", redtext( name_for_nodeType( nodeCheckPoint ) ) );
		return;
	}

	// get highest id
	id = 0;
	for ( e = world; ( e = ez_find( e, classname ) ); )
		id = max(id, e->race_id);

	// and now remove it
	for ( e = world; ( e = ez_find( e, classname ) ); )
	{
		if ( id == e->race_id )
		{
			ent_remove( e );
			break;
		}
	}

	// this will fix id for end checkpoint
	race_fix_end_checkpoint();

	G_bprint( 2, "%s \220%d\221 removed\n", redtext( name_for_nodeType( nodeCheckPoint ) ), id );

	race_route_now_custom();  // mark this is a custom route now
}

void set_player_race_ready(	gedict_t *e, int ready )
{
	if ( ready )
	{
		if ( e->race_ready )
			return; // alredy ready

		G_bprint( 2, "%s is %s for %s\n", e->s.v.netname, redtext("ready"), redtext("race"));

		e->race_ready = 1;

		race.warned = false; // so we get warning why race can't be started
	}
	else
	{
		if ( !e->race_ready )
			return; // alredy NOT ready

		G_bprint( 2, "%s is %s for %s\n", e->s.v.netname, redtext("not ready"), redtext("race"));
		e->race_ready = 0;
	}
}

void r_changestatus( float t )
{
	if ( match_in_progress || match_over )
		return;

	if ( check_master() )
		return;

	switch ( (int)t )
	{
		case 1: // rready
		set_player_race_ready( self, 1 );

		return;

		case 2: // rbreak
		set_player_race_ready( self, 0 );

		return;

		case 3: // rtoggle
		set_player_race_ready( self, !self->race_ready );

		return;

		case 4: // rcancel

		if ( !self->racer )
			return; // FUCK U!

		if ( !race.status )
			return; // same

		// do some sound
		sound( self, CHAN_ITEM, "boss2/idle.wav", 1, ATTN_NONE );

		race_start( true, "Race canceled by %s\n", self->s.v.netname );

		return;

		default:
		return;
	}
}

void r_timeout( )
{
	char	arg_1[64];

	if ( match_in_progress || match_over || race.status )
		return;

	if ( check_master() )
		return;

	trap_CmdArgv( 1, arg_1, sizeof( arg_1 ) );

	race.timeout_setting = atoi( arg_1 );

	if ( race.timeout_setting )
		race.timeout_setting = 20; // default is 20 sec

	race.timeout_setting = bound(1, atoi( arg_1 ), 60 * 60 );

	G_bprint(2, "%s set race timeout to %ss\n", self->s.v.netname, dig3( race.timeout_setting ) );
}

void r_mode( )
{
	if ( match_in_progress || match_over || race.status )
		return;

	if ( check_master() )
		return;

	race.weapon++;

	if ( race.weapon < raceWeaponNo || race.weapon >= raceWeaponMAX )
		race.weapon = raceWeaponNo;

	G_bprint(2, "%s set race weapon mode to %s\n", self->s.v.netname, redtext( race_weapon_mode() ) );
}

qboolean race_load_route( int route )
{
	int i;

	if ( route < 0 || route >= race.cnt || route >= MAX_ROUTES )
		return false;

	// remove all checkpoints before load 
	race_remove_ent();

	for ( i = 0; i < race.route[ route ].cnt && i < MAX_ROUTE_NODES; i++ )
	{
		spawn_race_node( &race.route[ route ].node[ i ] );
	}

	race.weapon 			= race.route[ route ].weapon;
	race.timeout_setting	= bound( 1, race.route[ route ].timeout, 60 * 60 );
	race.active_route		= route + 1; // mark this is not custom route now

	return true;
}

void race_print_route_info( gedict_t *p )
{
	if ( p )
	{
		G_sprint( p, 2, "\235\236\236\236\236\237 %s \235\236\236\236\236\237\n", race_route_name() );
		G_sprint( p, 2, "%s %2d \220tl: %ssec\221\n", redtext("route"), race.active_route, dig3( race.timeout_setting ) );

		if ( race.active_route )
			G_sprint( p, 2, "\220%s\221\n", race_route_desc() );

		G_sprint( p, 2, "%s: %s\n", redtext( "weapon" ), race_weapon_mode() );
	}
	else
	{
		G_bprint(    2, "\235\236\236\236\236\237 %s \235\236\236\236\236\237\n", race_route_name() );
		G_bprint(    2, "%s %2d \220tl: %ssec\221\n", redtext("route"), race.active_route, dig3( race.timeout_setting ) );

		if ( race.active_route )
			G_bprint(    2, "\220%s\221\n", race_route_desc() );

		G_bprint(    2, "%s: %s\n", redtext( "weapon" ), race_weapon_mode() );
	}		
}

void r_route( )
{
	static int next_route = 0; // STATIC

	if ( match_in_progress || match_over )
		return;

	if ( check_master() )
		return;

	if ( race.status )
	{
		G_sprint( self, 2, "Can't do that, race in progress\n");
		return;
	}

	if ( race.cnt < 1 )
	{
		G_sprint( self, 2, "No routes defined for this map\n");
		return;
	}

	next_route++;

	if ( next_route < 0 || next_route >= race.cnt )
		next_route = 0;

	if ( !race_load_route( next_route ) )
	{
		// we failed to load, clean it a bit then
		race_remove_ent();
		race_route_now_custom();

		G_bprint( 2, "Failed to load route %d by %s\n", next_route + 1, self->s.v.netname );

		return;
	}

	race_print_route_info( NULL );
	G_bprint( 2, "route loaded by %s\n", self->s.v.netname );
}

void r_print( )
{
	if ( match_in_progress || match_over )
		return;

	race_print_route_info( self );
}

