//
// race.c - race implementation, yeah got inspiration from ktpro (c) (tm) (etc)
//

#include "g_local.h"

// ok, lets define some funny types for race

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
	char					name[64]; 				// NOTE: this will probably FUCK QVM!!!
	int						cnt;				   	// how much nodes we actually have, no more than MAX_ROUTE_NODES
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

qboolean isRACE( void )
{
	if ( match_in_progress || match_over )
		return false;

	return !!race.status;
}

//============================================

void race_init( void )
{
	memset( &race, 0, sizeof( race ) );

	race.warned = true;
	race.status = raceNone;
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
	int cnt = 0, i;

	for ( i = 1; i < nodeMAX; i++ )
		ent_remove_by_classname( classname_for_nodeType( i ) );
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

int race_time( void )
{
	if ( race.status != raceActive )
		return 0;

	return (g_globalvars.time - race.start_time) * 1000;
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
				gedict_t *p;
				char cp_buf[1024] = { 0 };

				gedict_t *racer = race_get_racer();


				// ok, time for next "tick" in coutdown
				snprintf( cp_buf, sizeof( cp_buf ),	"Race in: %s\n\n"
						 							"Racer: %s\n\n", dig3( race.cd_cnt ), racer->s.v.netname );

				if ( !strnull( race.top_nick ) )
				{
					char tmp[512] = { 0 };

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
		race.timeout_setting = 20;

	race.timeout_setting = bound(1, atoi( arg_1 ), 60 * 60 );

	G_bprint(2, "%s set race timeout to %ss\n", self->s.v.netname, dig3( race.timeout_setting ) );
}

