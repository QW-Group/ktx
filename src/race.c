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
	char					name[64]; // well SD-Angel tells what I can't use it like this in QVM, only like char *name;
	int						cnt;				   // how much nodes we actually have, no more than MAX_ROUTE_NODES
	raceRouteNode_t			node[MAX_ROUTE_NODES]; // nodes of this route, fixed array, yeah I'm lazy

} raceRoute_t;

#define MAX_ROUTES			10 // why not

typedef struct
{
	int						cnt;				// how much we actually have of routes, no more than MAX_ROUTES
	raceRoute_t				route[MAX_ROUTES];	// fixed array of routes

} race_t;


race_t			race; // whole race struct

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

// start + end + intermediate checkpoints

int checkpoints_count( void )
{
	int cnt = 0, i;

	for ( i = 1; i < nodeMAX; i++ )
		cnt += find_cnt( FOFCLSN, classname_for_nodeType( i ) );

	return cnt;
}

//===========================================

void ent_remove_by_classname( char *classname )
{
	gedict_t *e;

	for ( e = world; ( e = ez_find( e, classname ) ); )
		ent_remove( e );
}

//===========================================

void race_remove_ent( void )
{
	int cnt = 0, i;

	for ( i = 1; i < nodeMAX; i++ )
		ent_remove_by_classname( classname_for_nodeType( i ) );
}

//===========================================

void race_fix_end_checkpoint( void )
{
	// get end checkpoint
	gedict_t *e = ez_find( world, classname_for_nodeType( nodeEnd ) );

	if ( e )
		e->race_id = 1 + find_cnt( FOFCLSN, classname_for_nodeType( nodeCheckPoint ) ); // pretty simple logic
}

//===========================================

void race_blink_think ()
{
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

void race_node_touch()
{
	if ( other->ct != ctPlayer )
		return;

	if ( self->attack_finished < g_globalvars.time )
	{
		race_blink_node( self ); // set "cute" effects

		self->attack_finished = g_globalvars.time + 5; // allow touch it again but later
		sound( other, CHAN_ITEM, self->s.v.noise, self->race_volume, ATTN_NONE );

		if ( self->race_RouteNodeType == nodeCheckPoint )
			G_sprint( other, 2, "%s \220%d\221\n", redtext( name_for_nodeType( self->race_RouteNodeType ) ), self->race_id );
		else
			G_sprint( other, 2, "%s\n", redtext( name_for_nodeType( self->race_RouteNodeType ) ) );
	}
}


//===========================================

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
//    e->s.v.effects		= EF_DIMLIGHT;
	e->s.v.classname	= classname;
	e->s.v.noise		= touch_sound_for_nodeType( node->type );
	e->race_volume		= volume_for_touch_sound_for_nodeType( node->type );
	e->race_effects		= blink_effects_for_nodeType( node->type );
	e->s.v.touch		= ( func_t ) race_node_touch;
	e->attack_finished  = g_globalvars.time + 1;
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
//
// race commands
//
//============================================

void r_Xset( float t )
{
	gedict_t				*e;
	raceRouteNode_t			node;

	if ( match_in_progress )
		return;

	if( check_master() )
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

	if ( match_in_progress )
		return;

	if( check_master() )
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

