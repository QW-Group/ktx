//
// race.c - race implementation, yeah got inspiration from ktpro (c) (tm) (etc)
//

#include "g_local.h"

#define MAX_TXTLEN	64
#define TOP_FILE_VERSION 2
#define POS_FILE_VERSION 2

#define RACEFLAG_TOUCH_RACEFAIL 1
#define RACEFLAG_TOUCH_RACEEND  2
#define RACEFLAG_ENTITY_KEEP    4

#define RACE_MAX_CAPTURES     600
#define RACE_CAPTURE_FPS       10
#define RACE_GUIDE_BASE_ENT    32
#define RACE_JUMP_INDICATORS    4

#define RACE_PACEMAKER_JUMPS_CVAR      "k_race_pace_jumps"
#define RACE_PACEMAKER_LEGAL_RECORD    "k_race_pace_legal"
#define RACE_PACEMAKER_HEADSTART_CVAR  "k_race_pace_headstart"
#define RACE_PACEMAKER_RESOLUTION_CVAR "k_race_pace_resolution"
#define RACE_PACEMAKER_ENABLED_CVAR    "k_race_pace_enabled"
#define RACE_SIMULTANEOUS_CVAR         "k_race_simultaneous"
#define RACE_PACEMAKER_HEADSTART_MIN  0.00f
#define RACE_PACEMAKER_HEADSTART_MAX  1.00f
#define RACE_PACEMAKER_HEADSTART_INCR 0.25f
#define RACE_PACEMAKER_RESOLUTION_MIN  0
#define RACE_PACEMAKER_RESOLUTION_MAX  3
#define RACE_PACEMAKER_RESOLUTION_INCR 1

#define RACE_PACEMAKER_TRAIL_COUNT     12

#define RACE_INCR_PARAMS(name) (RACE_PACEMAKER_##name##_CVAR),(RACE_PACEMAKER_##name##_INCR),(RACE_PACEMAKER_##name##_MIN),(RACE_PACEMAKER_##name##_MAX)

typedef struct race_capture_pos_s {
	float race_time;
	vec3_t origin;
	vec_t angles[2];
} race_capture_pos_t;

typedef struct race_capture_jump_s {
	float race_time;
	vec3_t origin;
} race_capture_jump_t;

typedef struct race_capture_s {
	race_capture_pos_t positions[1200];
	race_capture_jump_t jumps[100];
	int position_count;
	int jump_count;
} race_capture_t;

typedef struct race_capture_playback_s {
	race_capture_t capture;
	int position;
	int jump;
} race_capture_playback_t;

race_capture_t player_captures[MAX_CLIENTS];
race_capture_playback_t guide;

static void update_jump_markers(float race_time, int guide_start, int resolution);
static gedict_t* race_jump_indicators[RACE_JUMP_INDICATORS];
static float race_jump_indicator_times[RACE_JUMP_INDICATORS];

void ktpro_autotrack_on_powerup_take (gedict_t *racer);
void race_cancel( qbool cancelrecord, const char *fmt, ... );
void race_start( qbool cancelrecord, const char *fmt, ... );
void race_unready_all(void);
void r_route( void );
void race_record( void );
void race_stoprecord( qbool cancel );
void race_remove_ent( void );
void race_set_players_movetype_and_etc( void );
void race_cleanmap( void );
void unmute_all_players( void );
void HideSpawnPoints();
void ShowSpawnPoints();
void kill_race_idler( void );
void write_topscores( void );
void read_topscores( void );
void init_scores( void );

qbool is_rules_change_allowed( void );
qbool race_command_checks( void );
qbool race_is_started( void );

static void race_update_pacemaker(void);
static void race_clear_pacemaker(void);
static void race_init_capture(void);
static void race_save_position(void);
static void race_finish_capture(qbool store, char* filename);
static qbool race_pacemaker_enabled(void);
static void race_pacemaker_race_start(void);
static void race_remove_pacemaker_indicator(void);
static void race_make_active_racer(gedict_t* r, gedict_t* s);
static qbool race_end(gedict_t* racer);
static char* race_position_string(int position);
static gedict_t* race_find_race_participants(gedict_t* p);
static qbool race_simultaneous(void);
static void race_update_closest_positions(void);

void StatsToFile ();

fileHandle_t race_fhandle = -1;
race_t			race; // whole race struct

char *classname_for_nodeType( raceRouteNodeType_t nodeType );

#define RACE_INVALID_RECORD_TIME 999999
#define RACE_DEFAULT_TIMEOUT 60
#define RACE_MAX_TIMEOUT 3600

static int next_route = -1; // STATIC

//============================================

int get_server_port ( void )
{
	char *ip, *port;
	int i = 0;

	if ( strnull( ip = cvar_string( "sv_local_addr" ) ) || strnull( port = strchr(ip, ':') ) || !(i = atoi(port + 1)) )
		return 27500;
	else
		return i;
}
	
qbool isRACE( void )
{
	return ( cvar("k_race") );
}

static qbool is_valid_record ( raceRecord_t* record )
{
	return record->time < RACE_INVALID_RECORD_TIME;
}

static int read_record_param( int param )
{
	char arg_1[64] = { 0 };

	if ( trap_CmdArgc() <= param )
		return 0;

	trap_CmdArgv( param, arg_1, sizeof( arg_1 ) );

	return bound(0, atoi( arg_1 ) - 1, NUM_BESTSCORES - 1 );
}

void ToggleRace( void )
{
	if ( !isRACE() )
		if ( !is_rules_change_allowed() )
			return;

	if ( !isRACE() )
	{
		if ( !isFFA() )
		{
			UserMode( -6 );
		}
	}

	if ( CountPlayers() && race_is_started() )
		return;

	cvar_toggle_msg( self, "k_race", redtext("race") );

	apply_race_settings();
}

// hard coded default settings for RACE
static char race_settings[] =
	"sv_silentrecord 1\n"
	"deathmatch 4\n"
	"srv_practice_mode 1\n"
	"lock_practice 1\n"
	"allow_toggle_practice 0\n"
	"sv_demotxt 0\n"
	"k_spw 1\n"
	"k_noitems 1\n"
	"pm_airstep 0\n"
	"serverinfo ktxmode race\n";

static char norace_settings[] =
	"sv_silentrecord 0\n"
	"lock_practice 0\n"
	"srv_practice_mode 0\n"
	"allow_toggle_practice 5\n"
	"serverinfo ktxmode \"\"\n";

void apply_race_settings( void )
{
    char buf[1024*4];
	char *cfg_name;

	if ( !isRACE() )
	{
		race_stoprecord( true );

		unmute_all_players();

		// turn off race settings.
		trap_readcmd( norace_settings, buf, sizeof(buf) );
		G_cprint("%s", buf);

		// Execute configs/reset.cfg and set k_defmode.
		execute_rules_reset();

		return;
	}

	// turn on race settings.
	trap_readcmd( race_settings, buf, sizeof(buf) );
	G_cprint("%s", buf);

	cfg_name = va("configs/usermodes/race/default.cfg");
	if ( can_exec( cfg_name ) )
	{
		trap_readcmd( va("exec %s\n", cfg_name), buf, sizeof(buf) );
		G_cprint("%s", buf);
	}

	cfg_name = va("configs/usermodes/race/%s.cfg", g_globalvars.mapname);
	if ( can_exec( cfg_name ) )
	{
		trap_readcmd( va("exec %s\n", cfg_name), buf, sizeof(buf) );
		G_cprint("%s", buf);
	}

	G_cprint("\n");
}

	
void race_cleanmap( void )
{
	gedict_t *p;
	for( p = world; (p = nextent(p)); )
	{
		if (   streq( p->s.v.classname, "weapon_nailgun" )
			|| streq( p->s.v.classname, "weapon_supernailgun" )
			|| streq( p->s.v.classname, "weapon_supershotgun" )
			|| streq( p->s.v.classname, "weapon_rocketlauncher" )
			|| streq( p->s.v.classname, "weapon_grenadelauncher" )
			|| streq( p->s.v.classname, "weapon_lightning" )
			|| streq( p->s.v.classname, "item_shells" )
			|| streq( p->s.v.classname, "item_spikes" )
			|| streq( p->s.v.classname, "item_rockets" )
			|| streq( p->s.v.classname, "item_cells" )
			|| streq( p->s.v.classname, "item_health" )
			|| streq( p->s.v.classname, "item_armor1")
			|| streq( p->s.v.classname, "item_armor2")
			|| streq( p->s.v.classname, "item_armorInv")
			|| streq( p->s.v.classname, "item_artifact_invulnerability")
			|| streq( p->s.v.classname, "item_artifact_envirosuit")
			|| streq( p->s.v.classname, "item_artifact_invisibility")
			|| streq( p->s.v.classname, "item_artifact_super_damage")
			|| streq( p->s.v.classname, "item_armor1" )
			|| streq( p->s.v.classname, "item_armor2" )
			|| streq( p->s.v.classname, "item_armorInv") )
		{
			ent_remove ( p );
			continue;
		}

		if ( p->race_flags & RACEFLAG_ENTITY_KEEP )
			continue;

		if ( streq( p->s.v.classname, "door") )
		{
			ent_remove( p );
			continue;
		}
 	}
}

//===========================================

int race_time( void )
{
	if ( race.status != raceActive )
		return 0; // count time only when race in state raceActive

	return (g_globalvars.time - race.start_time) * 1000;
}

void setwepall( gedict_t *p )
{
	gedict_t *swap;

	p->s.v.ammo_nails   = 255;
	p->s.v.ammo_shells  = 255;
	p->s.v.ammo_rockets = 255;
	p->s.v.ammo_cells   = 255;
	p->s.v.items = IT_AXE | IT_SHOTGUN | IT_SUPER_SHOTGUN | IT_NAILGUN | IT_SUPER_NAILGUN |
					   IT_GRENADE_LAUNCHER | IT_ROCKET_LAUNCHER | IT_LIGHTNING;
	p->lastwepfired = 0;

	swap = self;
	self = p;

	if ( !( (int)self->s.v.weapon & (int)self->s.v.items ) )
		self->s.v.weapon = IT_ROCKET_LAUNCHER;

	W_SetCurrentAmmo ();
	self = swap;
}

void setwepnone( gedict_t *p )
{
	gedict_t *swap;

	p->s.v.ammo_nails   = 0;
	p->s.v.ammo_shells  = 0;
	p->s.v.ammo_rockets = 0;
	p->s.v.ammo_cells   = 0;
	p->s.v.items = 0;
	p->lastwepfired = 0;

	swap = self;
	self = p;

	self->s.v.weapon = W_BestWeapon();
	W_SetCurrentAmmo();

	self = swap;
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
	int i;

	memset( &race, 0, sizeof( race ) );

	race.timeout_setting = RACE_DEFAULT_TIMEOUT;

	race.warned = true;
	race.status = raceNone;

	race.weapon = raceWeaponAllowed;
	race.falsestart = raceFalseStartNo;

	for ( i = 0; i < NUM_BESTSCORES; i++ )
	{
		race.records[i].time = RACE_INVALID_RECORD_TIME;
	}
}

// clean up, so we can start actual match and there will be no some shit around
void race_shutdown( char *msg )
{
	race_cancel( true, msg );
	race_remove_ent();
	race_unready_all();
	if ( cvar( "k_spm_show" ) )
		ShowSpawnPoints();
}

// make all players not ready for race, silently
void race_unready_all( void )
{
	gedict_t *p;

	for ( p = world; ( p = find_plr( p ) ); )
		p->race_ready = 0;
}

//============================================

qbool race_route_add_start( void )
{
	if ( race.cnt < 0 || race.cnt >= MAX_ROUTES )
		return false;

	race.route[race.cnt].weapon  = raceWeaponAllowed; // default allowed
	race.route[race.cnt].timeout = 20; // default 20 sec
	race.route[race.cnt].falsestart  = raceFalseStartNo; // default with falsestart

	return true;
}

void race_route_add_end( void )
{
	if ( race.cnt < 0 || race.cnt >= MAX_ROUTES )
		G_Error( "race_route_add_end: race.cnt %d", race.cnt );

	race.cnt++;
}

raceRouteNode_t* race_add_route_node(float x, float y, float z, float pitch, float yaw, raceRouteNodeType_t	type)
{
	int node_idx;

	if ( race.cnt < 0 || race.cnt >= MAX_ROUTES )
		G_Error( "race_add_route_node: race.cnt %d", race.cnt );

	node_idx = race.route[race.cnt].cnt;

	if ( node_idx < 0 || node_idx >= MAX_ROUTE_NODES )
		return NULL; // we are full

	race.route[race.cnt].node[node_idx].type = type;
	race.route[race.cnt].node[node_idx].org[0] = x;
	race.route[race.cnt].node[node_idx].org[1] = y;
	race.route[race.cnt].node[node_idx].org[2] = z;
	race.route[race.cnt].node[node_idx].ang[0] = pitch;
	race.route[race.cnt].node[node_idx].ang[1] = yaw;
	race.route[race.cnt].node[node_idx].ang[2] = 0;
	VectorClear (race.route[race.cnt].node[node_idx].sizes);

	race.route[race.cnt].cnt++; // one more node

	return &race.route[race.cnt].node[node_idx];
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

	race.route[race.cnt].timeout = bound(1, timeout, 999);
}

void race_set_route_falsestart_mode( raceFalseStartMode_t falsestart )
{
	switch ( falsestart )
	{
		case raceFalseStartNo:
		case raceFalseStartYes:
		break; // known

		default: G_Error( "race_set_route_falsestart_mode: wrong type %d", falsestart );
	}

	race.route[race.cnt].falsestart = falsestart;
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

gedict_t* race_find_race_participants(gedict_t* p)
{
	p = find_plr(p);
	while (p && !p->race_participant) {
		p = find_plr(p);
	}
	return p;
}

gedict_t* race_find_racer(gedict_t* p)
{
	p = find_plr(p);
	while (p && !p->racer) {
		p = find_plr(p);
	}
	return p;
}

gedict_t *race_get_racer( void )
{
	return race_find_racer(world);
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

void race_record( void )
{
	if ( race.cd_cnt && cvar("k_race_autorecord") )
	{
		StartDemoRecord(); // start demo recording
		race.race_recording = true;
	}

}

void race_stoprecord( qbool cancel )
{
	if ( race.race_recording )
	{
		if (cancel)
		{
			localcmd ("cancel\n");  // stop recording demo and discard it
		}
		else
		{
			StatsToFile ();
			localcmd ("stop\n"); // stop recording demo and keep it
		}

		race.race_recording = false;
	}
}

//============================================

char *race_falsestart_mode( int start )
{
	switch ( start )
	{
		case raceFalseStartNo:
		return "no falsestart";

		case raceFalseStartYes:
		return "falsestart enabled";

		default: G_Error( "race_falsestart_mode: wrong race.falsestart %d", start );
	}

	return ""; // keep compiler silent
}

char *race_weapon_mode( int weapon )
{
	switch ( weapon )
	{
		case raceWeaponNo:
		return "disallowed";

		case raceWeaponAllowed:
		return "allowed";

		case raceWeapon2s:
		return "allowed after 2s";

		default: G_Error( "race_weapon_mode: wrong race.weapon %d", weapon );
	}

	return ""; // keep compiler silent
}

qbool race_weapon_allowed( gedict_t *p )
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
		case nodeCheckPoint:		return "checkpoint";
		case nodeEnd:			return "finish checkpoint";

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
	if ( cvar("k_race_custom_models") )
	{
		switch ( nodeType )
		{
			case nodeStart:		return "progs/start.mdl";
			case nodeCheckPoint:	return "progs/check.mdl";
			case nodeEnd:		return "progs/finish.mdl";

			default:	G_Error( "model_for_nodeType: wrong nodeType %d", nodeType );
		}
	}
	else
	{
		switch ( nodeType )
		{
			case nodeStart:			return "progs/invulner.mdl";
			case nodeCheckPoint:	return "progs/w_s_key.mdl";
			case nodeEnd:			return "progs/invulner.mdl";

			default:	G_Error( "model_for_nodeType: wrong nodeType %d", nodeType );
		}
	}

	return ""; // keep compiler silent
}

char *touch_sound_for_nodeType( raceRouteNodeType_t nodeType )
{
	switch ( nodeType )
	{
		case nodeStart:			return "items/protect3.wav";
		case nodeCheckPoint:		return "items/damage.wav";
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
		case nodeCheckPoint:		return "items/itembk2.wav";
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
		case nodeCheckPoint:		return 0.3;
		case nodeEnd:			return 0.5f;

		default:	G_Error( "volume_for_touch_sound_for_nodeType: wrong nodeType %d", nodeType );
	}

	return 1; // keep compiler silent
}


float blink_effects_for_nodeType( raceRouteNodeType_t nodeType )
{
	switch ( nodeType )
	{
		case nodeStart:			return ( EF_BLUE | EF_GREEN );
		case nodeCheckPoint:		return EF_BLUE;
		case nodeEnd:			return ( EF_BLUE | EF_GREEN );

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

void race_check_racer_falsestart( qbool nextracer )
{
	gedict_t *e;
	gedict_t *racer;

	for (racer = world; racer = race_find_racer(racer); /**/) {
		for (e = world; (e = ez_find(e, "race_cp_start")); ) {
			if ((racer->s.v.origin[0] != e->s.v.origin[0])
				&& (racer->s.v.origin[1] != e->s.v.origin[1])) {
				if (nextracer) {
					G_bprint(PRINT_HIGH, "\20%s\21 false-started\n", racer->s.v.netname);
					race_end(racer);
				}
				else {
					G_sprint(racer, 2, "Come back here!\n");
					VectorCopy(e->s.v.origin, racer->s.v.origin);
					VectorSet(racer->s.v.velocity, 0, 0, 0);
				}
			}
		}
	}
}

void kill_race_idler( void )
{
	gedict_t *e;
	gedict_t *racer;
	
	for (racer = world; racer = race_find_racer(racer); /**/) {
		// FIXME: Keep track of which start, any map with multiple starts would break
		for (e = world; (e = ez_find(e, "race_cp_start")); ) {
			if (racer->s.v.origin[0] >= e->s.v.absmin[0] && racer->s.v.origin[0] <= e->s.v.absmax[0] &&
				racer->s.v.origin[1] >= e->s.v.absmin[1] && racer->s.v.origin[1] <= e->s.v.absmax[1]) {
				racer->race_afk++;

				if (racer->race_afk < 3) {
					G_bprint(PRINT_HIGH, "Run aborted, %s was %s to start\n", racer->s.v.netname, redtext("too slow"));
					race_end(racer);
				}
				else {
					G_bprint(PRINT_HIGH, "%s was %s of line-up for %s\n", racer->s.v.netname, redtext("kicked out"), redtext("idling"));
					race_end(racer);
					racer->race_ready = 0;
				}
			}
			else {
				racer->race_afk = 0;
			}
		}
	}
}

void race_brighten_checkpoints( void )
{
	int i;
	gedict_t *e;
	gedict_t *racer;
	int furthest_checkpoint = 0;
	
	for (racer = world; racer = race_find_race_participants(racer); /**/) {
		furthest_checkpoint = max(furthest_checkpoint, racer->race_id);
	}

	for ( i = 1; i < nodeMAX; i++ )
	{
		char *classname = classname_for_nodeType( i );

		for ( e = world; ( e = ez_find( e, classname ) ); )
		{
			e->s.v.effects   = 0; // remove effects
			e->s.v.nextthink = 0; // stop thinking

			if ( e->race_id == furthest_checkpoint )
				e->s.v.effects = ( EF_GREEN ); // set some green light for next checkpoint
			else
				e->s.v.effects = ( EF_RED ); // turn all others red
		}
	}
}

void race_dim_checkpoints( void )
{
	int i;
	gedict_t *e;

	for ( i = 1; i < nodeMAX; i++ )
	{
		char *classname = classname_for_nodeType( i );

		for ( e = world; ( e = ez_find( e, classname ) ); )
		{
			e->s.v.effects   = 0; // remove effects
			e->s.v.nextthink = 0; // stop thinking
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
	e->s.v.nextthink = g_globalvars.time + 5;
	e->s.v.think = ( func_t ) race_blink_think;
}

//===========================================

void race_VelocityForDamage( float scale, vec3_t dir, vec3_t v )
{
	if ( vlen( dir ) > 0 )
	{
		vec3_t ang;

		VectorCopy( dir, v );

		VectorNormalize( v );

		vectoangles( v, ang );
		trap_makevectors( ang );

//		VectorMA( v, crandom() * 0.3, g_globalvars.v_right, v );
		VectorMA( g_globalvars.v_forward, crandom() * 0.3, g_globalvars.v_right, v );
		VectorMA( v, crandom() * 0.3, g_globalvars.v_up,    v );

		VectorNormalize( v );
		VectorScale( v, scale, v );
	}
	else
	{
		v[0] = 100 * crandom();
		v[1] = 100 * crandom();
		v[2] = 20 + 10 * g_random();

		VectorNormalize( v );
		VectorScale( v, scale, v );
	}

	return;
}

void race_meat_touch()
{
	// sound( self, CHAN_WEAPON, "zombie/z_miss.wav", 1, ATTN_NORM );	// bounce sound

	if ( self->s.v.velocity[0] == 0 && self->s.v.velocity[1] == 0 && self->s.v.velocity[2] == 0 )
		VectorClear( self->s.v.avelocity );
}

void race_spawn_meat( gedict_t *player, char *gibname, float vel )
{
	gedict_t	*newent;

	newent = spawn();

	VectorCopy( player->s.v.origin, newent->s.v.origin );

	setmodel( newent, gibname );
	setsize( newent, 0, 0, 0, 0, 0, 0 );
	race_VelocityForDamage( vel, player->s.v.velocity, newent->s.v.velocity );
	newent->s.v.movetype		= MOVETYPE_BOUNCE;
	newent->isMissile			= true;
	newent->s.v.solid			= SOLID_TRIGGER;
	newent->s.v.avelocity[0]	= g_random() * 600;
	newent->s.v.avelocity[1]	= g_random() * 600;
	newent->s.v.avelocity[2]	= g_random() * 600;
	newent->s.v.think			= ( func_t ) SUB_Remove;
	newent->s.v.nextthink		= g_globalvars.time + 6 + g_random() * 10;

	newent->s.v.touch			= ( func_t ) race_meat_touch;
}

//===========================================

void race_sprint_checkpoint( gedict_t *player, gedict_t *cp )
{
	if ( cp->race_RouteNodeType == nodeCheckPoint )
		G_sprint( player, 2, "%s \220%d\221\n", redtext( name_for_nodeType( cp->race_RouteNodeType ) ), cp->race_id );
	else
		G_sprint( player, 2, "%s\n", redtext( name_for_nodeType( cp->race_RouteNodeType ) ) );
}

static void race_end_point_touched(gedict_t* self, gedict_t* other)
{
	int player_num = NUM_FOR_EDICT(other) - 1;
	float speed, frac;

	// 
	if (race.currentrace[player_num].time) {
		return;
	}

	// They got to the end
	other->race_closest_guide_pos = guide.capture.position_count;

	// spawn a bit of meat
	speed = max(600, vlen(other->s.v.velocity));
	frac = bound(0, 0.8, 1); // non random fraction of the speed
	race_spawn_meat(other, "progs/gib1.mdl", frac * speed + g_random() * (1.0 - frac) * speed);
	race_spawn_meat(other, "progs/gib2.mdl", frac * speed + g_random() * (1.0 - frac) * speed);
	race_spawn_meat(other, "progs/gib3.mdl", frac * speed + g_random() * (1.0 - frac) * speed);
	race_spawn_meat(other, "progs/h_player.mdl", frac * speed + g_random() * (1.0 - frac) * speed);

	// If multi-racing, report on position for this race
	if (race.racers_competing > 1) {
		char* position;
		if (race.last_completion_time < race_time()) {
			race.racers_complete += race.last_completion_eq + 1;
			position = race_position_string(race.racers_complete);
			G_bprint(PRINT_HIGH, "\20%s\21 finished%s%s \20%.3fs\21\n", other->s.v.netname, strnull(position) ? "" : " in ", position, race_time() / 1000.0f);
			race.last_completion_eq = 0;
		}
		else {
			position = race_position_string(race.racers_complete);
			G_bprint(PRINT_HIGH, "\20%s\21 also finished%s \20%.3f\21\n", other->s.v.netname, strnull(position) ? "" : " in ", position, race_time() / 1000.0f);
			++race.last_completion_eq;
		}

		if (race.racers_complete == 1) {
			sound(other, CHAN_ITEM, "boss2/sight.wav", 1, ATTN_NONE);
		}
		else {
			sound(other, CHAN_ITEM, "ambience/thunder1.wav", 1, ATTN_NONE);
		}
	}

	// Add marker for demos/clients
	{
		stuffcmd(other, "//ktx race end %f %f %f %f\n",
			race.currentrace[player_num].time,
			race.currentrace[player_num].distance,
			race.currentrace[player_num].maxspeed,
			race.currentrace[player_num].avgcount ? race.currentrace[player_num].avgspeed / race.currentrace[player_num].avgcount : 0
		);

		race.currentrace[player_num].time = race_time(); // stop run timer
	}

	race_end(other);
}

static void race_over(void)
{
	char demoFileName[MAX_OSPATH];
	int i, timeposition, nameposition;
	qbool istopscore;
	char* pos;
	gedict_t* racer = NULL;
	qbool keep_demo = false;
	qbool blocked_record = race_pacemaker_enabled() && !cvar(RACE_PACEMAKER_LEGAL_RECORD);
	qbool debug = cvar("developer");

	strlcpy(demoFileName, cvar_string("serverdemo"), sizeof(demoFileName));
	pos = strchr(demoFileName, '.');
	if (pos) {
		*pos = '\0';
	}

	istopscore = false;
	read_topscores();

	if (debug) {
		G_bprint(PRINT_HIGH, "Race over: %d participants\n", race.racers_competing);
	}
	for (racer = world; racer = race_find_race_participants(racer); /**/) {
		int player_num = NUM_FOR_EDICT(racer) - 1;
		if (debug) {
			G_bprint(PRINT_HIGH, "Player: %d, %s\n", player_num, racer->s.v.netname);
		}

		if (!race.currentrace[player_num].time) {
			// They didn't set a time
			if (debug) {
				G_bprint(PRINT_HIGH, "- didn't set a time\n");
			}
			continue;
		}

		// first, let's see if run time gets into top scores and if name is already ranked
		timeposition = nameposition = -1;
		for (i = 0; i < NUM_BESTSCORES; i++) {
			if (race.currentrace[player_num].time < race.records[i].time) {
				if (timeposition == -1) {
					timeposition = i;
				}
			}

			if (streq(race.records[i].racername, racer->s.v.netname)) {
				nameposition = i;
			}
		}
		if (debug) {
			G_bprint(PRINT_HIGH, "- time %.3f, position %d, nameposition %d\n", race.currentrace[player_num].time, timeposition, nameposition);
		}

		// run time is within top scores range
		if (timeposition != -1) {
			// if player didn't beat his own record
			if ((nameposition < timeposition) && (nameposition != -1)) {
				if (race.racers_competing == 1) {
					sound(racer, CHAN_ITEM, "ambience/thunder1.wav", 1, ATTN_NONE);
					G_bprint(PRINT_HIGH, "Run %s in %s%s\n%s couldn't beat %s best time\n",
						redtext("finished"),
						dig3s("%.3f", race.currentrace[player_num].time / 1000),
						redtext("s"),
						racer->s.v.netname,
						g_his(racer)
					);
				}
			}
			else {
				if (!blocked_record) {
					// if player beat his own record or is not yet ranked
					if (nameposition == -1) {
						nameposition = (NUM_BESTSCORES - 1);
					}

					if (is_valid_record(&race.records[nameposition])) {
						// let's remove the old demo
						if (!strnull(race.records[nameposition].demoname)) {
							int demo_references = 0;

							// one demo might now contain records for more than one player, so
							//   only remove if this record is the last reference
							for (i = 0; i < sizeof(race.records) / sizeof(race.records[0]); ++i) {
								if (streq(race.records[nameposition].demoname, race.records[i].demoname)) {
									++demo_references;
								}
							}

							if (debug) {
								G_bprint(PRINT_HIGH, "%s has %d references\n", race.records[nameposition].demoname, demo_references);
							}
							if (demo_references == 1) {
								localcmd(va("sv_demoremove %s\n", race.records[nameposition].demoname));
							}
						}
					}

					// move old top scores down
					for (i = nameposition; i > timeposition; i--) {
						race.records[i] = race.records[i - 1];
					}

					// add new top score
					race.records[i].time = race.currentrace[player_num].time;
					strlcpy(race.records[i].racername, racer->s.v.netname, sizeof(race.records[i].racername));
					if (race.race_recording) {
						strlcpy(race.records[i].demoname, demoFileName, sizeof(race.records[i].demoname));
					}
					else {
						memset(race.records[i].demoname, 0, sizeof(race.records[i].demoname));
					}
					race.records[i].distance = race.currentrace[player_num].distance;
					race.records[i].maxspeed = race.currentrace[player_num].maxspeed;
					race.records[i].avgspeed = race.currentrace[player_num].avgspeed / race.currentrace[player_num].avgcount;
					race.records[i].weaponmode = race.weapon;
					race.records[i].startmode = race.falsestart;
					if (!QVMstrftime(race.records[i].date, sizeof(race.records[i].date), "%Y-%m-%d %H:%M:%S", 0))
						race.records[i].date[0] = 0; // bad date

					// save scores in file
					write_topscores();

					if (race.racers_competing == 1) {
						if (!i) {
							// first place! we go the extra mile
							sound(racer, CHAN_ITEM, "boss2/sight.wav", 1, ATTN_NONE);
							strlcpy(race.top_nick, racer->s.v.netname, sizeof(race.top_nick));
						}
						else {
							sound(racer, CHAN_ITEM, "ambience/thunder1.wav", 1, ATTN_NONE);
						}
					}

					if (!i) {
						race.top_time = race.currentrace[player_num].time;
					}
				}

				if (race.racers_competing == 1) {
					G_bprint(2, "Run %s in %s%s\n",
						redtext("finished"),
						dig3s("%.3f", race.currentrace[player_num].time / 1000),
						redtext("s")
					);
				}

				G_bprint(PRINT_HIGH, "%s %s %s record\n",
					racer->s.v.netname,
					blocked_record ? "would have taken" : "took the",
					race_position_string(timeposition + 1)
				);

				keep_demo = timeposition == 0 || !blocked_record;
			}
		}
		else {
			// run time did not make it to top scores
			if (race.racers_competing == 1) {
				sound(racer, CHAN_ITEM, "boss2/idle.wav", 1, ATTN_NONE);
				G_bprint(PRINT_HIGH, "Run %s in %s%s\n",
					redtext("finished"),
					dig3s("%.3f", race.currentrace[player_num].time / 1000),
					redtext("s")
				);
			}
		}
	}
	race_finish_capture(keep_demo, demoFileName);
	race_start(!keep_demo, "");
}

void race_node_touch()
{
	if ( other->ct != ctPlayer )
		return;

	// no run in progress nor starting
	if ( !race.status )
	{
		if ( self->attack_finished >= g_globalvars.time )
			return; // still in node touch cooldown

		self->attack_finished = g_globalvars.time + 5; // touch cooldown of 5s
		race_blink_node( self );
		sound( other, CHAN_ITEM, self->s.v.noise, self->race_volume, ATTN_NONE );
		race_sprint_checkpoint( other, self ); // display checkpoint type or order
	}

	// run in progress or starting
	if ( race.status != raceActive )
		return; // starting run countdown

	if ( !other->racer )
		return; // can't touch if not the racer

	if ( self->race_id < other->race_id )
		return; // node already touched during run

	if ( self->race_id == other->race_id )
	{
		// racer touched checkpoint in right order
		if (self->race_RouteNodeType == nodeEnd) {
			race_end_point_touched(self, other);
		}
		else if (self->race_RouteNodeType == nodeCheckPoint) {
			int player_num = NUM_FOR_EDICT(other) - 1;

			stuffcmd(other, "//ktx race cp %d %f %f %f %f\n",
				other->race_id,
				race.currentrace[player_num].time,
				race.currentrace[player_num].distance,
				race.currentrace[player_num].maxspeed,
				race.currentrace[player_num].avgcount ? race.currentrace[player_num].avgspeed / race.currentrace[player_num].avgcount : 0
			);
		}

		// if its not start checkpoint do something "cute"
		if ( self->race_id )
		{
			// do some sound
			sound( other, CHAN_ITEM, "knight/sword2.wav", 1, ATTN_NONE );
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

		if (self->race_RouteNodeType == nodeCheckPoint) {
			G_bprint(PRINT_HIGH, "\20%s\21: %s \220%d\221 touched in wrong order\n", other->s.v.netname, redtext(name_for_nodeType(self->race_RouteNodeType)), self->race_id);
			race_end(other);
		}
		else {
			G_bprint(PRINT_HIGH, "\20%s\21: \220%s\221 touched in wrong order\n", other->s.v.netname, redtext(name_for_nodeType(self->race_RouteNodeType)));
			race_end(other);
		}

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
	if (VectorCompareF (node->sizes, 0, 0, 0))
		setsize (e, PASSVEC3 (VEC_HULL_MIN), PASSVEC3 (VEC_HULL_MAX));
	else
		setsize (e, -node->sizes[0] / 2, -node->sizes[1] / 2, -node->sizes[2] / 2, node->sizes[0] / 2, node->sizes[1] / 2, node->sizes[2] / 2);
	e->s.v.solid		= SOLID_TRIGGER;
	e->s.v.movetype		= MOVETYPE_NONE;
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
		p->race_participant = p->racer = false;
		p->race_id = 0;
	}
}

void race_cancel( qbool cancelrecord, const char *fmt, ... )
{
	va_list argptr;
	char    text[1024];

	va_start( argptr, fmt );
	Q_vsnprintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	race_stoprecord( cancelrecord );

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

void init_scores( void )
{
    int i;

    for ( i = 0; i < NUM_BESTSCORES; i++ )
    {
		race.records[i].time = RACE_INVALID_RECORD_TIME;
		race.records[i].racername[0] = '\0';
		race.records[i].demoname[0] = '\0';
		race.records[i].distance = 0;
		race.records[i].maxspeed = 0;
		race.records[i].avgspeed = 0;
		race.records[i].date[0] = '\0';
		race.records[i].weaponmode = race.weapon;
		race.records[i].startmode = race.falsestart;
    }
}

void display_scores( void )
{
    int i;

	if ( !race_command_checks() )
		return;

	G_sprint(self, 2,  "\n\235\236\236\236\236\236\236\236\236\236\236\236\236\236\237%s %02d\235\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext( "top"), NUM_BESTSCORES );
	G_sprint( self, 2, "pos.  time      name\n" );

    for ( i = 0; i < NUM_BESTSCORES; i++ )
    {
		if ( ! is_valid_record( &race.records[i] ) )
		{
			G_sprint( self, 2, " %02d      -         -\n", i + 1 );
		}
		else
		{
			if ( streq( race.records[i].racername, self->s.v.netname ) )
				G_sprint( self, 2, " %02d \215 %07.3f%s  %s\n", i + 1, race.records[i].time / 1000.0, redtext( "s" ), race.records[i].racername );
			else
				G_sprint( self, 2, " %02d   %07.3f%s  %s\n", i + 1, race.records[i].time / 1000.0, redtext( "s" ), race.records[i].racername );
		}
    }
}

void race_display_line( void )
{
    int i = 0;
    gedict_t *p;

	if ( !race_command_checks() )
		return;

    G_sprint( self, 2, "=== %s ===\n", redtext( "Line-up") );

    for ( p = world; ( p = find_plr( p ) ); )
    {
        if ( p->race_ready )
        {
            i++;
            if ( p->racer )
                G_sprint( self, 2, "%2d \215 %s\n", i, p->s.v.netname );
            else
                G_sprint( self, 2, "%2d   %s\n", i, p->s.v.netname );
        }
    }

    if ( !i )
        G_sprint( self, 2, "    (Empty)    \n" );
}

//============================================

qbool race_can_go( qbool cancel )
{
	gedict_t *racer;

	// can't go on, noone ready
	if ( !race_count_ready_players() )
	{
		if ( cancel )
		{
			race_cancel( true, "Race in standby, no players in line\n" );
			race_dim_checkpoints();
		}

		return false;
	}

	// can't go on, no start checkpoint
	if ( !ez_find( world, classname_for_nodeType( nodeStart ) ) )
	{
		if ( cancel )
		{
			race_cancel( true, "Race in standby, no %s\n", name_for_nodeType( nodeStart ) );
		}

		return false;
	}

	// can't go on, no end checkpoint
	if ( !ez_find( world, classname_for_nodeType( nodeEnd ) ) )
	{
		if ( cancel )
		{
			race_cancel( true, "Race in standby, no %s\n", name_for_nodeType( nodeEnd ) );
		}

		return false;
	}

	if ( race.status )
	{
		// Multi-person racing: this is fine, means we can't find even one racer
		if ( !race_get_racer() )
		{
			if ( cancel )
			{
				race_start( true, "Race aborted, racer vanished\n" );
			}
			return false;
		}
	}

	if ( race.status == raceActive )
	{
		int timeouts = 0;
		int deaths = 0;
		gedict_t* timeout_plr = NULL;
		qbool race_ended = false;

		// Multi-person racing: end individual racer's races if they die
		for (racer = world; racer = race_find_racer(racer); /**/) {
			if (racer->s.v.health <= 0 && cancel) {
				// do some sound
				k_respawn(racer, false);
				G_bprint(PRINT_HIGH, "\20%s\21 died\n", racer->s.v.netname);
				race_ended |= race_end(racer);
			}

			// Timeout everyone still racing
			if (race.timeout < g_globalvars.time) {
				if (cancel) {
					race_ended |= race_end(racer);
					timeout_plr = racer;
					++timeouts;
				}
			}
		}

		// Reduce number of announcements
		if (timeouts || deaths) {
			// do some sound
			sound(world, CHAN_ITEM, "boss2/idle.wav", 1, ATTN_NONE);
			if (timeouts == 1 && timeout_plr) {
				G_bprint(PRINT_HIGH, "\20%s\21 couldn't finish in time\n", timeout_plr->s.v.netname);
			}
			else if (timeouts > 1) {
				G_bprint(PRINT_HIGH, "\20%d\21 players couldn't finish in time\n", timeouts);
			}
		}

		// Race officially ends once everyone has stopped
		return !race_ended;
	}

	return true;
}

//
// qbool restart:
// true  - means continue current competition, just select next racer in line, keep best results.
// false - means start completely new race, reset best result and etc.
void race_start( qbool cancelrecord, const char *fmt, ... )
{
	va_list argptr;
	char    text[1024];
	extern	void ktpro_autotrack_on_race_status_changed (void);

	gedict_t *r, *n, *s;

	va_start( argptr, fmt );
	Q_vsnprintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	// cancel it first, this will clear something what probably was't cleared before
	race_cancel( cancelrecord, text );

	// switch status to coutdown
	race.status = raceCD;

	// set countdown timer
	race.cd_cnt = 3;

	race.cd_next_think = g_globalvars.time;

	if ( !race.timeout_setting )
		race.timeout_setting = RACE_DEFAULT_TIMEOUT;

	s = ez_find( world, classname_for_nodeType( nodeStart ) );
	if ( !s )
	{
		race_shutdown( "race_start: can't find start checkpoint, shutdown race\n" );
		return;
	}

	if (race_simultaneous()) {
		// Everyone in queue gets to race
		race.racers_competing = 0;
		for (r = world; r = find_plr(r); /**/) {
			if (r->ct == ctPlayer && r->race_ready) {
				race_make_active_racer(r, s);
				++race.racers_competing;
			}
		}
	}
	else {
		r = race_get_from_line();
		if ( !r ) {
			race_shutdown( "race_start: race_get_from_line() == NULL, shutdown race\n" );
			return;
		}

		n = race_set_next_in_line();
		if ( !n ) {
			race_shutdown( "race_start: race_set_next_in_line() == NULL, shutdown race\n" );
			return;
		}

		race_make_active_racer(r, s);

		if ( n != r ) {
			G_sprint( n, 2, "Your are %s in line!\n", redtext( "next" ) );
		}

		race.racers_competing = 1;
	}

	// set light on next checkpoint
	race_brighten_checkpoints();

	// set proper move type for players
	race_set_players_movetype_and_etc();

	// remove some projectiles
	remove_projectiles();

	// autotrack - force switch pov to racer
	ktpro_autotrack_on_race_status_changed();

	// create pacemaker entity and announce to players
	race_pacemaker_race_start();

	race.last_completion_eq = race.racers_complete = 0;
}

static void race_make_active_racer(gedict_t* r, gedict_t* s)
{
	// mark him as racer
	r->racer = r->race_participant = true;
	ktpro_autotrack_on_powerup_take(r);
	//G_bprint( 2, "%s is starting his race!\n", r->s.v.netname );

	// make sure not hooked onto something at previous spot
	if ( r->hook_out )
		GrappleReset( r->hook );
	r->hook_out = false;
	r->on_hook = false;

	// clear velocity
	SetVector( r->s.v.velocity, 0, 0, 0 );

	// set proper angles
	VectorCopy(s->s.v.v_angle, r->s.v.angles);
	VectorCopy(s->s.v.v_angle, r->s.v.v_angle);
	r->s.v.fixangle = true;

	// set proper origin
	setorigin( r, PASSVEC3( s->s.v.origin ) );

	// telefrag anyone at this origin
	teleport_player( r, r->s.v.origin, r->s.v.angles, TFLAGS_SND_DST );
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

	if ( race.status && p->race_chasecam && !p->racer )
		setwepnone( p );
	else
		setwepall( p );

	switch ( race.status )
	{
		case raceNone:
			p->s.v.movetype	= MOVETYPE_WALK;
			p->s.v.solid = SOLID_BBOX;
			p->muted = false;
			setmodel( p, "progs/player.mdl" );
			break;

		case raceCD:
			if (race.falsestart == raceFalseStartNo) {
				p->s.v.movetype = (p->racer ? MOVETYPE_NONE : MOVETYPE_WALK);
			}
			else {
				p->s.v.movetype = MOVETYPE_WALK;
			}
			p->s.v.solid = SOLID_NOT;
			p->muted = ( p->racer ? false : true );
			setmodel( p, ( p->racer ? "progs/player.mdl" : "" ) );
			break;

		case raceActive:
			p->s.v.movetype	=  MOVETYPE_WALK;
			p->s.v.solid = SOLID_NOT;
			p->muted = ( p->racer ? false : true );
			setmodel( p, ( p->racer ? "progs/player.mdl" : "" ) );
			break;

		default:
			G_Error( "race_set_one_player_movetype_and_etc: unknown race.status %d", race.status );
	}
}

void unmute_all_players( void )
{
 	gedict_t *p;

	for ( p = world; ( p = find_plr( p ) ); )
		p->muted = false;
}

void race_set_players_movetype_and_etc( void )
{
 	gedict_t *p;

	for ( p = world; ( p = find_plr( p ) ); )
		race_set_one_player_movetype_and_etc( p );
}

void race_chasecam_freelook_change ( void )
{
	if ( !race_command_checks() )
		return;

	self->race_chasecam_freelook = !self->race_chasecam_freelook;

	switch ( self->race_chasecam_freelook )
	{
		case 0:
			G_sprint( self, 2, "Chasecam freelook %s\n", redtext("disabled") );
			return;
		case 1:
			G_sprint( self, 2, "Chasecam freelook %s\n", redtext("enabled") );
			return;
		default:
			return;
	}
}

void race_chasecam_change ( void )
{
	if ( !race_command_checks() )
		return;

	if ( self->racer )
		return;

	self->race_chasecam_view++;
	if ( self->race_chasecam_view == NUM_CHASECAMS )
		self->race_chasecam_view = 0;

	switch (self->race_chasecam_view )
	{
		case 0:
			G_sprint( self, 2, "Chasecam is in %s view mode\n", redtext("1st person") );
			break;
		case 1:
			G_sprint( self, 2, "Chasecam is in %s view mode\n", redtext("3rd person") );
			break;
		case 2:
			G_sprint( self, 2, "Chasecam is in %s view mode\n", redtext("hawk eye") );
			break;
		case 3:
			G_sprint( self, 2, "Chasecam is in %s view mode\n", redtext("backpack ride") );
			break;
		default:
			G_sprint( self, 2, "Chasecam position has not beem defined, keep cycling\n" );
	}
}

// FIXME: Allow player to toggle which racer they follow
void race_follow( void )
{
    gedict_t *racer = race_get_racer();
	vec3_t delta;
	float vlen;
    int follow_distance;
    int upward_distance;

    if ( !racer )
        return; // no racer found

    if ( !self->racer && self->race_chasecam )
    {
        switch ( self->race_chasecam_view )
        {
            case 1: // 3rd person
                follow_distance = -120;
                upward_distance = 50;
				self->hideentity = 0;
            	VectorCopy( racer->s.v.v_angle, self->s.v.angles);
                break;
            case 2: // hawk eye
                follow_distance = -50;
                upward_distance = 300;
				self->hideentity = 0;
				self->s.v.angles[0] = 90;
				self->s.v.angles[1] = racer->s.v.angles[1];
                break;
            case 3: // backpack ride
                follow_distance = -10;
                upward_distance = 0;
				self->hideentity = EDICT_TO_PROG( racer ) ;
				self->s.v.angles[0] = - racer->s.v.angles[0];
				self->s.v.angles[1] = 180;
                break;
            case 0: // 1st person - ok
                follow_distance = -10;
                upward_distance = 0;
				self->hideentity = EDICT_TO_PROG( racer );  // in this mode we want to hide racer model for watcher's view
            	VectorCopy( racer->s.v.v_angle, self->s.v.angles);
				break;
            default:
				return;
        }

		if ( !self->race_chasecam_freelook )
			self->s.v.fixangle = true; // force client v_angle

        trap_makevectors( racer->s.v.angles );
        VectorMA (racer->s.v.origin, follow_distance, g_globalvars.v_forward, self->s.v.origin);
        VectorMA (self->s.v.origin, upward_distance, g_globalvars.v_up, self->s.v.origin);

        // avoid positionning in walls
        traceline( PASSVEC3( racer->s.v.origin ), PASSVEC3( self->s.v.origin ), false, racer );
		VectorCopy( g_globalvars.trace_endpos, self->s.v.origin );

		if ( g_globalvars.trace_fraction == 1 )
        {
			VectorCopy(g_globalvars.trace_endpos, self->s.v.origin);
            VectorMA (self->s.v.origin, 10, g_globalvars.v_forward, self->s.v.origin);
		}
		else
		{
			VectorSubtract( g_globalvars.trace_endpos, racer->s.v.origin, delta );
			vlen = VectorLength( delta );
			vlen = vlen - 40;
			VectorNormalize( delta );
			VectorScale( delta, vlen, delta );
			VectorAdd( racer->s.v.origin, delta, self->s.v.origin );
		}

		//if ( trap_pointcontents( PASSVEC3( self->s.v.origin ) ) == CONTENT_SKY )
		//	G_bprint( 2, "SKY!\n" );

		// smooth playing for ezq / zq
		self->s.v.movetype = MOVETYPE_LOCK;
    }

	if ( !self->racer && !self->race_chasecam )
	{
		// restore movement and show racer entity
		self->s.v.movetype = MOVETYPE_WALK;
		self->hideentity = 0;
	}
}

void race_think( void )
{
	gedict_t *racer;
	gedict_t *p, *n = NULL;

	race_update_pacemaker();

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
			race_start( true, "" );

			return;

		case raceCD:

			// something wrong
			if ( !race_can_go( true ) )
				return;

			if (!race.race_recording)
				race_record();

			// countdown in progress
			if ( race.cd_next_think >= g_globalvars.time )
				return;

			racer = race_get_racer();
			// must not never happens because we have race_can_go() above
			if ( !racer )
			{
				race_start( true, "Run aborted, racer vanished\n" );
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
					if (race_find_racer(racer)) {
						snprintf(cp_buf, sizeof(cp_buf), "Racing in: %s\n", dig3(race.cd_cnt));
					}
					else {
						snprintf(cp_buf, sizeof(cp_buf), "%s racing in: %s\n", racer->s.v.netname, dig3(race.cd_cnt));
					}
					snprintf ( tmp, sizeof( tmp ), "weapon: %s\n\n", redtext( race_weapon_mode( race.weapon ) ) );
					strlcat( cp_buf, tmp, sizeof( cp_buf ) );

					if ( !strnull( race.top_nick ) )
					{
						snprintf ( tmp, sizeof( tmp ),	"best run: %s%s (by %s)", dig3s( "%.3f", race.top_time / 1000.0 ), redtext( "s" ), race.top_nick );
						strlcat( cp_buf, tmp, sizeof( cp_buf ) );
					}

					if ( !strnull( race.pacemaker_nick ))
					{
						snprintf ( tmp, sizeof( tmp ),	"\npacemaker: %s%s (by %s)", dig3s( "%.3f", race.pacemaker_time / 1000.0 ), redtext( "s" ), race.pacemaker_nick );
						strlcat( cp_buf, tmp, sizeof( cp_buf ) );
					}

					G_cp2all( cp_buf );

					// check for falsestarts
					race_check_racer_falsestart( false );

					// FIXME: yeah, nice make some utility for that
					for (p = world; (p = find_client(p)); ) {
						stuffcmd(p, "play buttons/switch04.wav\n");
					}

					race.cd_next_think = g_globalvars.time + 1; // set next think one second later
					race.cd_cnt--;
				}

				return;
			}

			G_cp2all("GO!");

			// FIXME: yeah, nice make some utility for that
			for (p = world; (p = find_client(p)); ) {
				//stuffcmd (p, "play enforcer/enfire.wav\n");
				stuffcmd(p, "play weapons/pkup.wav\n");  // I like this one better -- deurk.
			}

			race.status = raceActive; // countdown ends, now we ready for race
			memset(race.currentrace, 0, sizeof(race.currentrace)); // initiate distance
			race_init_capture();

			// check for falsestarts
	   		race_check_racer_falsestart( true );

			race.start_time		= g_globalvars.time;
			race.timeout		= g_globalvars.time + max( 1 , race.timeout_setting );
			race.next_race_time = 500; // do not print race time for first 500 ms, so we can see "Go!"

			// set proper move type for players
			race_set_players_movetype_and_etc();

			{
				char date[64];
				if (!QVMstrftime(date, sizeof(date), "%Y%m%d%H%M%S", 0))
					date[0] = '\0';

				for (racer = world; racer = race_find_racer(racer); /**/) {
					stuffcmd(racer, "//ktx race start %d %d %s\n",
						race.weapon,
						race.falsestart,
						date
					);
				}
			}

			return;

		case raceActive:

			// anti-idling
			if ( ( race_time() > race.start_time + 3000 ) && ( race_time() < race.start_time + 4000 ) )
				kill_race_idler();

			// something wrong
			if ( !race_can_go( true ) )
				return;

			race_save_position();

			if ( race_time() >= race.next_race_time )
			{
				vec3_t tmp;

				race.next_race_time = race_time() + 100; // update race time each 100 ms

				for (racer = world; racer = race_find_racer(racer); /**/) {
					int player_num = NUM_FOR_EDICT(racer) - 1;
					raceRecord_t* raceStats = &race.currentrace[player_num];
					float current_velocity = vlen(racer->s.v.velocity);

					VectorSubtract(racer->s.v.origin, racer->s.v.oldorigin, tmp);
					raceStats->distance += vlen(tmp);

					raceStats->maxspeed = max(raceStats->maxspeed, current_velocity);
					raceStats->avgspeed += current_velocity;
					raceStats->avgcount++;
				}

				race_update_closest_positions();

				if (race.racers_competing == 1) {
					racer = race_get_racer();
					n = race_get_from_line();
					for( p = world; (p = find_client( p )); )
					{
						if ( p->racer ) {
							G_centerprint( p, "%s", dig3s( "time: %.1f", race_time() / 1000.0 ) );
						}
						else {
							G_centerprint( p, "following %s\n%s\nspeed: %4.1f\ntime: %s",
								racer->s.v.netname,
								( n == p ) ? redtext( "== you're next in line-up ==" ) : "",
								vlen( racer->s.v.velocity ),
								dig3s( "%3.1f", race_time() / 1000.0 )
							);
						}
					}
				}
				else {
					for (p = world; p = find_client(p); /**/) {
						if (p->racer) {
							G_centerprint(p, "%s", dig3s("time: %.1f", race_time() / 1000.0));
						}
						else {
							int player_num = NUM_FOR_EDICT(p) - 1;
							qbool time_set = p->ct == ctPlayer && p->race_participant && race.currentrace[player_num].time;

							if (time_set) {
								G_centerprint(p, "%s %.3f %s\ntime: %s",
									redtext("== Race over: "),
									race.currentrace[player_num].time / 1000.0,
									redtext("=="),
									dig3s("%3.1f", race_time() / 1000.0)
								);
							}
							else if (p->ct == ctPlayer && p->race_participant) {
								G_centerprint(p, "%s\ntime: %s",
									redtext("== Race ended - please wait =="),
									dig3s("%3.1f", race_time() / 1000.0)
								);
							}
							else {
								G_centerprint(p, "%s\ntime: %s",
									redtext("== Race in progress =="),
									dig3s("%3.1f", race_time() / 1000.0)
								);
							}
						}
					}
				}
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

static void race_route_now_custom( void )
{
	init_scores ();
	race.active_route = 0; // mark this is a custom route now
	race_clear_pacemaker();
}

void r_Xset( float t )
{
	gedict_t				*e;
	raceRouteNode_t			node;

	if ( !race_command_checks() )
		return;

	if ( race_is_started() )
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
	{
		G_bprint( 2, "%s \220%d\221 set\n", redtext( name_for_nodeType( node.type ) ), e->race_id );
		G_bprint( 2, "Coordinates: %6.1f %6.1f %6.1f\n", e->s.v.origin[0], e->s.v.origin[1], e->s.v.origin[2] );
	}
	else if ( node.type == nodeStart )
	{
		G_bprint( 2, "%s set\n", redtext( name_for_nodeType( node.type ) ) );
		G_bprint( 2, "Coordinates: %6.1f %6.1f %6.1f\n", e->s.v.origin[0], e->s.v.origin[1], e->s.v.origin[2] );
		G_bprint( 2, "Direction: %6.1f %6.1f\n", e->s.v.v_angle[0], e->s.v.v_angle[1] );
	}
	else
	{
		G_bprint( 2, "%s set\n", redtext( name_for_nodeType( node.type ) ) );
		G_bprint( 2, "Coordinates: %6.1f %6.1f %6.1f\n", e->s.v.origin[0], e->s.v.origin[1], e->s.v.origin[2] );
	}

	race_route_now_custom();  // mark this is a custom route now
}

void r_cdel( )
{
	gedict_t				*e;
	int						cnt, id;
	char					*classname = classname_for_nodeType( nodeCheckPoint );

	if ( !race_command_checks() )
		return;

	if ( race_is_started() )
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

void set_player_race_follow( gedict_t *e, int follow )
{
	if ( follow )
	{
		if ( e->race_chasecam )
			return;

		G_sprint( self, 2, "Your %s is now %s\n", redtext( "chasecam"), redtext( "enabled" ) );

		e->race_chasecam = 1;
		if ( !e->racer )
			setwepnone( e );
	}
	else
	{
		if ( !e->race_chasecam )
			return;

		G_sprint( self, 2, "Your %s is now %s\n", redtext( "chasecam"), redtext( "disabled" ) );

		e->race_chasecam = 0;
		setwepall( e );
		SetVector( e->s.v.velocity, 0, 0, 0 );
	}
}

void set_player_race_ready(	gedict_t *e, int ready )
{
	if ( ready )
	{
		if ( e->race_ready )
			return;

		G_bprint( 2, "%s %s the line-up\n", e->s.v.netname, redtext("joined") );
		e->race_ready = 1;
		e->race_afk = 0;

		race.warned = false; // so we get warning why race can't be started
	}
	else
	{
		if ( !e->race_ready )
			return;

		G_bprint( 2, "%s %s the line-up\n", e->s.v.netname, redtext("left") );
		e->race_ready = 0;
	}
}

qbool race_command_checks( void )
{
	if ( !isRACE() )
	{
		G_sprint( self, 2, "Command only available in %s mode (type /%s to activate it)\n", redtext( "race" ), redtext( "race" ) );
		return false;
	}

	return true;
}

qbool race_is_started( void )
{
	if ( race.status )
	{
		G_sprint( self, 2, "Can't use that command while %s is in progress, wait for all players to leave the line-up\n", redtext( "race" ) );
		return true;
	}

	return false;
}

void r_changefollowstatus( float t )
{
	if ( !race_command_checks() )
		return;

	if ( self->racer )
		return;

	switch ( (int)t )
	{
		case 1: // rfollow
			set_player_race_follow( self, 1 );
			return;
		case 2: // rnofollow
			set_player_race_follow( self, 0 );
			return;
		case 3: // rftoggle
			set_player_race_follow( self, !self->race_chasecam );
			return;
		default:
			return;
	}
}

void r_changestatus( float t )
{
	if ( !race_command_checks() )
		return;

	if ( self->ct == ctSpec )
		return;

	switch ( (int)t )
	{
		case 1: // race_ready
			set_player_race_ready( self, 1 );

			return;

		case 2: // race_break

			if (self->racer && race.status) {
				G_bprint(PRINT_HIGH, "%s has quit the race\n", self->s.v.netname);
				race_end(self);
			}

			set_player_race_ready( self, 0 );

			return;

		case 3: // race_toggle

			if (self->racer && race.status) {
				G_bprint(PRINT_HIGH, "%s has quit the race\n", self->s.v.netname);
				race_end(self);
			}

			set_player_race_ready( self, !self->race_ready );

			return;

		case 4: // race_cancel

  	  		if ( !self->racer )
				return;

			if ( !race.status )
				return;

			// do some sound
			sound( self, CHAN_ITEM, "boss2/idle.wav", 1, ATTN_NONE );
			G_bprint(PRINT_HIGH, "%s aborted %s run\n", self->s.v.netname, g_his(self));
			race_end(self);

			return;

		default:
			return;
	}
}

void r_timeout( )
{
	char	arg_1[64];

	if ( !race_command_checks() )
		return;

	if ( race_is_started() )
		return;

	trap_CmdArgv( 1, arg_1, sizeof( arg_1 ) );

	race.timeout_setting = atoi( arg_1 );

	if ( ! race.timeout_setting )
		race.timeout_setting = RACE_DEFAULT_TIMEOUT;

	race.timeout_setting = bound(1, race.timeout_setting, RACE_MAX_TIMEOUT );

	G_bprint(2, "%s set race time limit to %ss\n", self->s.v.netname, dig3( race.timeout_setting ) );
}

void race_download_record_demo( void )
{
	int record = read_record_param( 1 );

	if ( !race_command_checks() )
		return;

	if ( ! is_valid_record( &race.records[record] ) )
	{
		G_sprint(self, 2, "record not found\n" );
		return;
	}

	if ( strnull( race.records[record].demoname ) )
	{
		G_sprint(self, 2, "demo for record #%d is not available\n", record + 1 );
		return;
	}

	stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "download \"demos/%s.mvd\"\n", race.records[record].demoname);
}

void display_record_details( )
{
	int record = read_record_param( 1 );

	if ( !race_command_checks() )
		return;

	if ( ! is_valid_record( &race.records[record] ) )
	{
		G_sprint(self, 2, "record not found\n" );
		return;
    }

	G_sprint(self, 2,  "\n\235\236\236\236\236\236\236\236\236\236\236\236\236\237%s %s\235\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext( "record") , dig3s( "%02d", record + 1 ) );
	G_sprint(self, 2, "time: %s\n", dig3s( "%7.3f%s", race.records[record].time / 1000, redtext( "s" ) ) );
	G_sprint(self, 2, "racer: %s\n", race.records[record].racername );
	G_sprint(self, 2, "demo: %s\n", redtext( race.records[record].demoname ) );
	G_sprint(self, 2, "distance: %s\n", dig3s( "%.1f", race.records[record].distance ) );
	G_sprint(self, 2, "max speed: %s\n", dig3s( "%.0f", race.records[record].maxspeed ) );
	G_sprint(self, 2, "avg speed: %s\n", dig3s( "%.0f", race.records[record].avgspeed ) );
	G_sprint(self, 2, "date: %s\n", redtext( race.records[record].date ) );
	G_sprint(self, 2, "weapon: %s\n", redtext( race_weapon_mode( race.records[record].weaponmode ) ) );
	G_sprint(self, 2, "falsestart: %s\n", redtext( race_falsestart_mode( race.records[record].startmode ) ) );
}

void r_falsestart( )
{
	if ( !race_command_checks() )
		return;

	if ( race_is_started() )
		return;

	race.falsestart++;

	if ( race.falsestart < raceFalseStartNo || race.falsestart >= raceFalseStartMAX )
		race.falsestart = raceFalseStartNo;

	G_bprint(2, "%s set race start mode to %s\n", self->s.v.netname, redtext( race_falsestart_mode( race.falsestart ) ) );

	read_topscores();
	race_clear_pacemaker();
}

void r_all_break ( void )
{
	if ( !race_command_checks() )
		return;

	race_unready_all();
	G_bprint(2, "%s has %s the race to stop\n", self->s.v.netname, redtext( "forced" ) );
}

void r_clear_route( void )
{
	gedict_t *p;

	if ( !race_command_checks() )
		return;

	if ( race_is_started() )
		return;

	for ( p = world; ( p = find_plr( p ) ); )
	{
		setwepall( p );
		p->muted = 0;
	}
	race_remove_ent();

	G_bprint(2, "%s cleared the current route\n", self->s.v.netname );
	race_clear_pacemaker();
}

void r_mode( )
{
	if ( !race_command_checks() )
		return;

	if ( race_is_started() )
		return;

	race.weapon++;

	if ( race.weapon < raceWeaponNo || race.weapon >= raceWeaponMAX )
		race.weapon = raceWeaponNo;

	G_bprint(2, "%s set race weapon mode to %s\n", self->s.v.netname, redtext( race_weapon_mode( race.weapon ) ) );

	read_topscores();
	race_clear_pacemaker();
}

qbool race_load_route( int route )
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
	race.timeout_setting	= bound( 1, race.route[ route ].timeout, RACE_MAX_TIMEOUT );
	race.active_route		= route + 1; // mark this is not custom route now

	read_topscores();

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

		G_sprint( p, 2, "%s: %s\n", redtext( "weapon" ), race_weapon_mode( race.weapon ) );
	}
	else
	{
		G_bprint(    2, "\235\236\236\236\236\237 %s \235\236\236\236\236\237\n", race_route_name() );
		G_bprint(    2, "%s %2d \220tl: %ssec\221\n", redtext("route"), race.active_route, dig3( race.timeout_setting ) );

		if ( race.active_route )
			G_bprint(    2, "\220%s\221\n", race_route_desc() );

		G_bprint(    2, "%s: %s\n", redtext( "weapon" ), race_weapon_mode( race.weapon ) );
	}
}

void r_route( void )
{
	if ( !race_command_checks() )
		return;

	if ( race_is_started() )
		return;

	HideSpawnPoints();
	race_cleanmap();

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

		if (self->ct == ctPlayer)
			G_bprint( 2, "Failed to load route %d by %s\n", next_route + 1, self->s.v.netname );
		else
			G_bprint( 2, "Server failed to load route %d\n", next_route + 1 );

		return;
	}

	if (self->ct == ctPlayer)
	{
		race_print_route_info( NULL );
		G_bprint( 2, "route loaded by %s\n", self->s.v.netname );
	}
	else
	{
		race_print_route_info( NULL );
		G_bprint( 2, "Server loaded route %d\n", next_route );
	}

	race_clear_pacemaker();
}

void r_print( )
{
	if ( !race_command_checks() )
		return;

	race_print_route_info( self );
}

void race_fclose(void)
{
	if ( race_fhandle < 0 )
		return;

	trap_FS_CloseFile( race_fhandle );
	race_fhandle = -1;
}

void race_fwopen( const char *fmt, ... )
{
	va_list argptr;
	char	text[MAX_TXTLEN] = {0};

	va_start( argptr, fmt );
	Q_vsnprintf( text, sizeof(text), fmt, argptr );
	va_end( argptr );

	text[sizeof(text)-1] = 0;

	if ( trap_FS_OpenFile( text, &race_fhandle, FS_WRITE_BIN ) < 0 )
	{
		race_fhandle = -1;
		//G_bprint( 2, "Failed to open file: %s\n", text );
		return;
	}

	//G_bprint( 2, "Succesfully opened file: %s\n", text );
}

void race_fropen( const char *fmt, ... )
{
	va_list argptr;
	char	text[MAX_TXTLEN] = {0};

	va_start( argptr, fmt );
	Q_vsnprintf( text, sizeof(text), fmt, argptr );
	va_end( argptr );

	text[sizeof(text)-1] = 0;

	if ( trap_FS_OpenFile( text, &race_fhandle, FS_READ_BIN ) < 0 )
	{
		race_fhandle = -1;
		//G_bprint( 2, "Failed to open file: %s\n", text );
		return;
	}

	//G_bprint( 2, "Succesfully opened file: %s\n", text );
}

void race_fprintf( const char *fmt, ... )
{
	va_list argptr;
	char	text[MAX_TXTLEN] = {0};

	if ( race_fhandle < 0 )
		return;

	va_start( argptr, fmt );
	Q_vsnprintf( text, sizeof(text), fmt, argptr );
	va_end( argptr );

	text[sizeof(text)-1] = 0;

	trap_FS_WriteFile( text, strlen(text), race_fhandle );
}

void write_topscores( void )
{
	int i;

	if ( !race.active_route )
		return;

	if ( cvar("k_race_times_per_port") )
		race_fwopen( "race/race[%s_r%02d]-w%1ds%1d_%d.top", g_globalvars.mapname, race.active_route, race.weapon, race.falsestart, get_server_port() );
	else
		race_fwopen( "race/race[%s_r%02d]-w%1ds%1d.top", g_globalvars.mapname, race.active_route, race.weapon, race.falsestart );

	if ( race_fhandle < 0 )
		return;

	race_fprintf( "version %d\n", TOP_FILE_VERSION );
	race_fprintf( "%d\n", NUM_BESTSCORES );

    for ( i = 0; i < NUM_BESTSCORES; i++ )
	{
		race_fprintf( "%f\n", race.records[i].time );
		race_fprintf( "%s\n", race.records[i].racername );
		race_fprintf( "%s\n", race.records[i].demoname );
		race_fprintf( "%f\n", race.records[i].distance );
		race_fprintf( "%f\n", race.records[i].maxspeed );
		race_fprintf( "%f\n", race.records[i].avgspeed );
		race_fprintf( "%s\n", race.records[i].date );
		race_fprintf( "%d\n", race.records[i].weaponmode );
		race_fprintf( "%d\n", race.records[i].startmode );
		race_fprintf( "%d\n", race.records[i].playernumber );
	}
	race_fclose();
}

int race_fgetc( void )
{
   	char c;
	int retval;

	if ( race_fhandle < 0 )
		return -2;

	retval = trap_FS_ReadFile( &c, 1, race_fhandle );
	//G_bprint( 2, "====> Read char: %d\n", c );

	return ( retval == 1 ? c : -1 );
}

char *race_fgets( char *buf, int limit )
{
	int c = '\0';
	char *string;

	if ( race_fhandle < 0 )
		return NULL;

	string = buf;
	while ( --limit > 0 && ( ( c = race_fgetc() ) != -1 ) )
		if ( ( *string++ = c ) == '\n' )
			break;
	*string = '\0';
	//G_bprint( 2, "====> Read string: %s\n", buf );

	return ( ( c == -1 ) && ( string = buf ) ) ? NULL : buf;
}

void read_topscores( void )
{
	char line[MAX_TXTLEN] = {0};
	int cnt, max;
	int format_version = 1;

	if ( !race.active_route )
		return;

	if ( cvar("k_race_times_per_port") )
		race_fropen( "race/race[%s_r%02d]-w%1ds%1d_%d.top", g_globalvars.mapname, race.active_route, race.weapon, race.falsestart, get_server_port() );
	else
		race_fropen( "race/race[%s_r%02d]-w%1ds%1d.top", g_globalvars.mapname, race.active_route, race.weapon, race.falsestart );

	if ( race_fhandle >= 0 )
	{
		race_fgets( line, MAX_TXTLEN );
		if (!strncmp(line, "version ", sizeof("version ") - 1)) {
			format_version = atoi(line + sizeof("version ") - 1);
			race_fgets( line, MAX_TXTLEN );
		}
		max = atoi( line );
		for ( cnt = 0; cnt < max; cnt++ )
		{
			race_fgets( line, MAX_TXTLEN );
			race.records[cnt].time = atof( line );
			race_fgets( line, MAX_TXTLEN );
			strlcpy( race.records[cnt].racername, line, strlen( line ) );
			race_fgets( line, MAX_TXTLEN );
			strlcpy( race.records[cnt].demoname, line, strlen( line ) );
			race_fgets( line, MAX_TXTLEN );
			race.records[cnt].distance = atof( line );
			race_fgets( line, MAX_TXTLEN );
			race.records[cnt].maxspeed = atof( line );
			race_fgets( line, MAX_TXTLEN );
			race.records[cnt].avgspeed = atof( line );
			race_fgets( line, MAX_TXTLEN );
			strlcpy( race.records[cnt].date, line, strlen( line ) );
			race_fgets( line, MAX_TXTLEN );
			race.records[cnt].weaponmode = atoi( line );
			race_fgets( line, MAX_TXTLEN );
			race.records[cnt].startmode = atoi( line );
			if (format_version >= 2) {
				race_fgets(line, MAX_TXTLEN);
				race.records[cnt].playernumber = atoi(line);
			}
			else {
				race.records[cnt].playernumber = -1;
			}
		}

		race.top_time = race.records[0].time;
		strlcpy(race.top_nick, race.records[0].racername, sizeof(race.top_nick));
	}
	else
	{
		init_scores();
		race.top_nick[0] = 0;
		race.top_time = RACE_INVALID_RECORD_TIME;
	}

	race_fclose();
}

void ChasecamToggleButton( void )
{
	if ( !( ( ( int ) ( self->s.v.flags ) ) & FL_ATTACKRELEASED ) )
		return;

	self->s.v.flags = (int)self->s.v.flags & ~FL_ATTACKRELEASED;

	r_changefollowstatus ( (float) 3 );
}

void race_set_teleport_flags_by_name ( const char* name, int flags )
{
	gedict_t* ent;

	for (ent = world; ent = ez_find (ent, "trigger_teleport"); )
	{
		if (streq (ent->s.v.target, name ))
		{
			ent->race_flags = flags;
		}
	}
}

void race_route_create( void )
{
	gedict_t* route[MAX_ROUTE_NODES] = { 0 };
	gedict_t* current = self, *next = NULL;
	int route_nodes = 0;
	int i = 0;

	// If race mode not enabled, just ignore quietly
	if ( !isRACE() ) {
		return;
	}

	// Name & description are mandatory, ignore route if not specified
	if (strnull(self->race_route_name) || strnull(self->race_route_description)) {
		G_bprint(2, "Route name/description not specified\n");
		return;
	}

	// weapon-mode must be valid
	if (self->race_route_weapon_mode <= raceWeaponUnknown || self->race_route_weapon_mode >= raceWeaponMAX) {
		G_bprint(2, "Route weapon mode not valid\n");
		return;
	}

	// false-start-mode must be valid
	if (self->race_route_falsestart_mode <= raceFalseStartUnknown || self->race_route_falsestart_mode >= raceFalseStartMAX) {
		G_bprint(2, "Route falsestart mode not valid\n");
		return;
	}

	while (current && current != world)
	{
		// flag current entity to be removed next frame
		if (streq(current->s.v.classname, "race_route_marker") || streq(current->s.v.classname, "race_route_start")) {
			SUB_RM_01(current);
		}

		// route is too long, ignore
		if (route_nodes >= sizeof(route) / sizeof(route[0])) {
			G_bprint(2, "Route too long\n");
			return;
		}

		// route is circular?  ignore
		for (i = 0; i < route_nodes; ++i)
		{
			if (route[i] == current) {
				G_bprint(2, "Circular route detected\n");
				return;
			}
		}

		// add to route
		route[route_nodes++] = current;

		// no targetname => end
		if ((current->race_flags & RACEFLAG_TOUCH_RACEEND) || strnull(current->s.v.target))
			break;

		// move to next target
		current = find(world, FOFS(s.v.targetname), current->s.v.target);

		// next target must be route marker, or end the race
		if (current && !((current->race_flags & RACEFLAG_TOUCH_RACEEND) || streq(current->s.v.classname, "race_route_marker"))) {
			G_bprint(2, "Expected route marker, found %s instead\n", current->s.v.classname);
			return;
		}
	}

	// must have at least start and end
	if (route_nodes < 2) {
		G_bprint(2, "Route too short (%d nodes)\n", route_nodes);
		return;
	}

	// Create route
	if ( !race_route_add_start() ) {
		G_bprint(2, "Couldn't create new route\n");
		return;
	}

	// Add path
	for (i = 0; i < route_nodes; ++i)
	{
		raceRouteNodeType_t nodeType = nodeCheckPoint;
		raceRouteNode_t* node = NULL;
		if (i == 0)
			nodeType = nodeStart;
		else if (i == route_nodes - 1)
			nodeType = nodeEnd;

		node = race_add_route_node(
			route[i]->s.v.origin[0],
			route[i]->s.v.origin[1],
			route[i]->s.v.origin[2],
			route[i]->race_route_start_pitch,
			route[i]->race_route_start_yaw,
			nodeType
		);
		if (node)
			VectorCopy (route[i]->s.v.size, node->sizes);
	}

	race_set_route_name( self->race_route_name, self->race_route_description );
	race_set_route_timeout( self->race_route_timeout );
	race_set_route_weapon_mode( (raceWeapoMode_t)self->race_route_weapon_mode );
	race_set_route_falsestart_mode( (raceFalseStartMode_t)self->race_route_falsestart_mode );
	race_route_add_end();

	if (next_route < 0)
		r_route();
}

void SP_race_route_start( void )
{
	self->s.v.nextthink = g_globalvars.time + 0.001f;	// create route once all markers have spawned
	self->s.v.think = ( func_t ) race_route_create;
}

void race_add_standard_routes( void )
{
	if ( streq( g_globalvars.mapname, "mvdsv-kg" ) )
	{
		if ( !race_route_add_start() )
			return; // can't add anymore route

		race_add_route_node(  118.5, -1345.5, 284.6, 5.2, 120.7, nodeStart );
		race_add_route_node( -572.9,  -276.8, 330.9,   0,     0, nodeCheckPoint );
		race_add_route_node( -111.9,   725.3, 466.0,   0,     0, nodeCheckPoint );
		race_add_route_node(  546.5,  -134.5, 566.0,   0,     0, nodeCheckPoint );
		race_add_route_node(  509.0, -1483.3, 706.6,   0,     0, nodeCheckPoint );
		race_add_route_node( -515.8, -1152.6, 856.4,   0,     0, nodeCheckPoint );
		race_add_route_node(  120.9,  -829.8, 889.4,   0,     0, nodeCheckPoint );
		race_add_route_node(      0,   -74.1, 979.5,   0,     0, nodeEnd );

		race_set_route_name( "Bunny to the top!", "castle\215pads\215platform" );
		race_set_route_timeout( 30 );
		race_set_route_weapon_mode( raceWeaponAllowed );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "dm1" ) )
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

		race_set_route_name( redtext("around dm1"), "ya\215ng\215mh\215ssg\215ya" );
		race_set_route_timeout( 30 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

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

		race_set_route_name( redtext("ihm's quad run"), "high rl\215quad\215water\215lower\215ra mh" );
		race_set_route_timeout( 35 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

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

		race_set_route_name( redtext("around dm2"), "water\215quad low\215rl low\215ng\215stairs" );
		race_set_route_timeout( 35 );
		race_set_route_weapon_mode( raceWeaponAllowed );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "dm3" ) )
	{
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node( 1880.1,  884.2, -213.0, 8.3, -92.1, nodeStart );
		race_add_route_node(  701.0,  237.5,   89.2,   0,     0, nodeCheckPoint );
		race_add_route_node(  261.0, -709.0,  328.0,   0,     0, nodeEnd );

		race_set_route_name( redtext("ra fly"), "pent mh\215window\215ra top" );
		race_set_route_timeout( 35 );
		race_set_route_weapon_mode( raceWeaponAllowed );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();

		//===========
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node( 1499.6,  498.9,  -88.0, 18.4, -3.4, nodeStart );
		race_add_route_node( 1400.3,  -30.1, -168.0,    0,    0, nodeCheckPoint );
		race_add_route_node(  581.8,    0.2, -168.0,    0,    0, nodeCheckPoint );
		race_add_route_node(  261.0, -709.0,  328.0,    0,    0, nodeEnd );

		race_set_route_name( redtext("ra run"), "rl\215center\215ra top" );
		race_set_route_timeout( 30 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

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

		race_set_route_name( redtext("around dm3"), "rl\215hi bridge\215quad\215ya\215quad\215under lifts\215sng mh\215ra top" );
		race_set_route_timeout( 50 );
		race_set_route_weapon_mode( raceWeaponAllowed );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "dm4" ) )
	{
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node( 1199.9, -603.0,  24.0, 1.8, 178.4, nodeStart );
		race_add_route_node( -144.0, -227.0, -72.0,   0,     0, nodeEnd );

		race_set_route_name( redtext("base dm4"), "ya\215quad tele" );
		race_set_route_timeout( 15 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();

		//===========
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node( 1199.9,  -603.0,   24.0, 1.8, 178.4, nodeStart );
		race_add_route_node(  337.8,  -673.6,   36.7,   0,     0, nodeCheckPoint );
		race_add_route_node(  296.6, -1205.2,    9.5,   0,     0, nodeCheckPoint );
		race_add_route_node(  336.2,  -664.8, -104.0,   0,     0, nodeCheckPoint );
		race_add_route_node(  -68.6,   611.1, -296.0,   0,     0, nodeEnd );

		race_set_route_name( redtext("mh run"), "ya\215top tele\215gl\215mh" );
		race_set_route_timeout( 20 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

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

		race_set_route_name( redtext("quad strafe"), "ya\215top tele\215gl\215lg\215quad\215quad tele" );
		race_set_route_timeout( 20 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

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

		race_set_route_name( redtext("around dm5"), "ya\215gl\215rl\215tele rl\215above gl\215sng" );
		race_set_route_timeout( 35 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "dm6" ) )
	{
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node(   55.2, -2005.1,  88.0, -15.2, 49.0, nodeStart );
		race_add_route_node(  450.2, -1484.5, 256.0,     0,    0, nodeCheckPoint );
		race_add_route_node( 1735.0,  -345.0, 168.0,     0,    0, nodeEnd );

		race_set_route_name( redtext("base dm6"), "ga\215gl\215ra" );
		race_set_route_timeout( 15 );
		race_set_route_weapon_mode( raceWeaponAllowed );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();

		//===========
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node(  209.8, -1247.9, 112.3, 3.4, 43.0, nodeStart );
		race_add_route_node( 1359.7,  -427.7,  40.0,   0,    0, nodeCheckPoint );
		race_add_route_node( 1108.6, -1316.2, 256.0,   0,    0, nodeCheckPoint );
		race_add_route_node(  813.4, -1081.0, 256.0,   0,    0, nodeCheckPoint );
		race_add_route_node( 1015.9,  -867.3, 256.0,   0,    0, nodeCheckPoint );
		race_add_route_node( 1632.2,  -160.4, 168.0,   0,    0, nodeEnd );

		// p3's tube run
		race_set_route_name( "\360\263\247\363 tube run", "mh\215rl ra\215gl circle\215ra" );
		race_set_route_timeout( 25 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "slide1" ) )
	{
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node(   64, -3428,  3352, 0, 90, nodeStart );
		race_add_route_node( -896, -1152, -3840, 0,  0, nodeEnd );

		race_set_route_name( "Slide 1", "top \215 bottom" );
		race_set_route_timeout( 40 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "slide2" ) )
	{
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node( -1600,  2380, 2736, 0, -90, nodeStart );
		race_add_route_node(  1821,  -714,  920, 0,   0, nodeEnd );


		race_set_route_name( "Slide 2", "top \215 bottom" );
		race_set_route_timeout( 40 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "slide3" ) )
	{
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node( -1920,  2208, -160, 0, 0, nodeStart );
		race_add_route_node(  2504,  2246, -160, 0, 0, nodeCheckPoint );
		race_add_route_node( -1460, -1023, -160, 0, 0, nodeCheckPoint );
		race_add_route_node( -2336,  2203, -160, 0, 0, nodeEnd );

		race_set_route_name( "Slide 3", "start \215 end" );
		race_set_route_timeout( 40 );
		race_set_route_weapon_mode( raceWeaponAllowed );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "slide4" ) )
	{
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node( 3555,  3547, 3736, 0, -180, nodeStart );
		race_add_route_node( 3124, -2785, -616, 0,    0, nodeEnd );

		race_set_route_name( "Slide 4", "top \215 bottom" );
		race_set_route_timeout( 40 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "slide5" ) )
	{
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node( 2367, 3364,  3224, 0, -90, nodeStart );
		race_add_route_node( 1597,  185, -1256, 0,   0, nodeEnd );

		race_set_route_name( "Slide 5", "top \215 bottom" );
		race_set_route_timeout( 40 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "slide6" ) )
	{
		raceRouteNode_t* checkpoint;

		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node(     0,    0, 3534, 0, 0, nodeStart );
		race_add_route_node( -1084, 1958,   18, 0, 0, nodeEnd );

		race_set_route_name( "Slide 6", "top \215 bottom" );
		race_set_route_timeout( 40 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();

		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node(     0,    0, 3534, 0, 0, nodeStart );
		checkpoint = race_add_route_node(  2605,  800, 3485, 0, 0, nodeCheckPoint );
		if (checkpoint)
			VectorSet (checkpoint->sizes, 430, 400, 1030);
		checkpoint = race_add_route_node(  1025,  -2275, 2200, 0, 0, nodeCheckPoint );
		if (checkpoint)
			VectorSet (checkpoint->sizes, 500, 400, 400);
		race_add_route_node( -1084, 1958,   18, 0, 0, nodeEnd );

		race_set_route_name( "Slide6: NO SHORTCUTS", "top \215 bottom" );
		race_set_route_timeout( 40 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "slide7" ) )
	{
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node(  194,    0,  2072, 0, 0, nodeStart );
		race_add_route_node( 2903, 2239, -3272, 0, 0, nodeEnd );

		race_set_route_name( "Slide 7", "top \215 bottom" );
		race_set_route_timeout( 40 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "slide8" ) )
	{
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node(    0,  -85,   20, 0, 90, nodeStart );
		race_add_route_node( -777, 2527, -360, 0,  0, nodeCheckPoint );
		race_add_route_node( -456,  -64, -876, 0,  0, nodeEnd );

		race_set_route_name( "Slide 8", "top \215 bottom" );
		race_set_route_timeout( 40 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "slide9" ) )
	{
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node(   484,   -5,  3602, 0, 180, nodeStart );
		race_add_route_node( -3809, 2450, -1109, 0,   0, nodeEnd );

		race_set_route_name( "Slide 9", "top \215 bottom" );
		race_set_route_timeout( 60 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "subslide" ) )
	{
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node( -1792, -320, 2264, 0, 0, nodeStart );
		race_add_route_node( -1775,  545,  472, 0, 0, nodeEnd );

		race_set_route_name( "Subslide", "top \215 bottom" );
		race_set_route_timeout( 25 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "slstart" ) )
	{
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node(  -286, -504, 3544, 0, 90, nodeStart );
		race_add_route_node( -3024, -223, 1432, 0,  0, nodeEnd );

		race_set_route_name( "Slstart", "top \215 bottom" );
		race_set_route_timeout( 25 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_set_teleport_flags_by_name( "teleskill", RACEFLAG_TOUCH_RACEFAIL );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "slidefox" ) )
	{
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node(  362, 542, -1700, 90, -90, nodeStart );
		race_add_route_node( -271, 996, -1588,  0,   0, nodeEnd );

		race_set_route_name( "Slidefox", "top \215 bottom" );
		race_set_route_timeout( 60 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "cmt3" ) )
	{
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node(  518, 1192, -520, 0, 140, nodeStart );
		race_add_route_node(  541, 1692, -528, 0,   0, nodeCheckPoint );
		race_add_route_node( 1085, 1612, -688, 0,   0, nodeCheckPoint );
		race_add_route_node(  839,  713, -896, 0,   0, nodeCheckPoint );
		race_add_route_node(  442,  714, -592, 0,   0, nodeCheckPoint );
		race_add_route_node( 1000,  839, -520, 0,   0, nodeCheckPoint );
		race_add_route_node( 1300,  122, -528, 0,   0, nodeCheckPoint );
		race_add_route_node(  988,  122, -816, 0,   0, nodeCheckPoint );
		race_add_route_node(  565, -331, -816, 0,   0, nodeCheckPoint );
		race_add_route_node( -438,   97, -344, 0,   0, nodeCheckPoint );
		race_add_route_node( -994,   53, -471, 0,   0, nodeEnd );

		race_set_route_name( "JohnNy_cz's Deutschmaschine race", "spawn \215 pent \215 lg \215 quad \215 ra \215 rl" );
		race_set_route_timeout( 40 );
		race_set_route_weapon_mode( raceWeapon2s );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "rawspeed" ) )
	{
		if ( !race_route_add_start() )
			return; // we are full

		race_add_route_node(  -126,  -154,   312, 0, 90, nodeStart );
		race_add_route_node( -3195, -3422, -1432, 0,  0, nodeEnd );

		race_set_route_name( "Raw Speed", "start\215end" );
		race_set_route_timeout( 40 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();
	}
	else if ( streq( g_globalvars.mapname, "2bfree" ) )
	{
		if ( !race_route_add_start() )
			return;

		race_add_route_node( 1460, 1000,  72, 0, 90, nodeStart );
		race_add_route_node( 1310,   50, 603, 0, 0, nodeEnd );

		race_set_route_name( "So you want to bunny?", "start\215end" );
		race_set_route_timeout( 60 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();
	}
    else if ( streq( g_globalvars.mapname, "race1" ) )
    {
        if ( !race_route_add_start() )
            return;

        race_add_route_node(    0,   0,   -40, 0, 0, nodeStart );
        race_add_route_node( 1376, 382, -3700, 0, 0, nodeEnd );

        race_set_route_name( "Pillars", "start\215finish" );
        race_set_route_timeout( 40 );
        race_set_route_weapon_mode( raceWeaponNo );
        race_set_route_falsestart_mode( raceFalseStartNo );

        race_route_add_end();
    }
    else if ( streq( g_globalvars.mapname, "race2" ) )
    {
        if ( !race_route_add_start() )
            return;

        race_add_route_node(   0,    8,    88, 0, 0, nodeStart );
        race_add_route_node( 1013, 3486, -1240, 0, 0, nodeEnd );

        race_set_route_name( "Waste Warehouse", "start\215finish" );
        race_set_route_timeout( 40 );
        race_set_route_weapon_mode( raceWeaponNo );
        race_set_route_falsestart_mode( raceFalseStartNo );

        race_route_add_end();
    }
    else if ( streq( g_globalvars.mapname, "race3" ) )
    {
        if ( !race_route_add_start() )
            return;

        race_add_route_node( -352, 352, 2072, 0, 0, nodeStart );
        race_add_route_node( -353, -92, -680, 0, 0, nodeEnd );

        race_set_route_name( "The Cube", "start\215finish" );
        race_set_route_timeout( 30 );
        race_set_route_weapon_mode( raceWeaponNo );
        race_set_route_falsestart_mode( raceFalseStartNo );

        race_route_add_end();
    }
    else if ( streq( g_globalvars.mapname, "race4" ) )
    {
        if ( !race_route_add_start() )
            return;

        race_add_route_node( -2816, 2816,  2712, 0, 0, nodeStart );
        race_add_route_node( -2918, 1790, -1115, 0, 0, nodeEnd );

        race_set_route_name( "Curve Control", "start\215finish" );
        race_set_route_timeout( 40 );
        race_set_route_weapon_mode( raceWeaponNo );
        race_set_route_falsestart_mode( raceFalseStartNo );

        race_route_add_end();
    }
    else if ( streq( g_globalvars.mapname, "race5" ) )
    {
	    if ( !race_route_add_start() )
            return;

        race_add_route_node( -1882, 2400, 2840, 0, -180, nodeStart );
        race_add_route_node(  2463, -1793,  98, 0,    0, nodeEnd );

        race_set_route_name( "Spin Cycle", "start\215finish" );
        race_set_route_timeout( 40 );
        race_set_route_weapon_mode( raceWeaponNo );
        race_set_route_falsestart_mode( raceFalseStartNo );

        race_route_add_end();
    }
    else if ( streq( g_globalvars.mapname, "race6" ) )
    {
        if ( !race_route_add_start() )
            return;

        race_add_route_node( -2848,   0, 2872, 0, 0, nodeStart );
        race_add_route_node(     0, 965, -985, 0, 0, nodeEnd );

        race_set_route_name( "The Tube", "start\215finish" );
        race_set_route_timeout( 40 );
        race_set_route_weapon_mode( raceWeaponNo );
        race_set_route_falsestart_mode( raceFalseStartNo );

        race_route_add_end();
    }
    else if ( streq( g_globalvars.mapname, "race7" ) )
    {
        if ( !race_route_add_start() )
            return;

        race_add_route_node( -2880, -768, 2520, 0, 0, nodeStart );
        race_add_route_node( -1378,  794,  344, 0, 0, nodeEnd );

        race_set_route_name( "Upslider", "start\215finish" );
        race_set_route_timeout( 40 );
        race_set_route_weapon_mode( raceWeaponNo );
        race_set_route_falsestart_mode( raceFalseStartNo );

        race_route_add_end();
    }
    else if ( streq( g_globalvars.mapname, "race8" ) )
    {
        if ( !race_route_add_start() )
            return;

        race_add_route_node( -1440,  2304, 1496, 0, 0, nodeStart );
        race_add_route_node( -1479, -1788, -1000, 0, 0, nodeEnd );

        race_set_route_name( "Canyon Run", "start\215finish" );
        race_set_route_timeout( 60 );
        race_set_route_weapon_mode( raceWeaponAllowed );
        race_set_route_falsestart_mode( raceFalseStartNo );

        race_route_add_end();
    }
    else if ( streq( g_globalvars.mapname, "race9" ) )
    {
        if ( !race_route_add_start() )
            return;

        race_add_route_node( -2736, 2568, 2776, 0, 0, nodeStart );
        race_add_route_node( -2981, 688,  -296, 0, 0, nodeEnd );

        race_set_route_name( "Forgotten Path", "start\215finish" );
        race_set_route_timeout( 90 );
        race_set_route_weapon_mode( raceWeaponNo );
        race_set_route_falsestart_mode( raceFalseStartNo );

        race_route_add_end();
    }
    else if ( streq( g_globalvars.mapname, "race10" ) )
    {
        if ( !race_route_add_start() )
            return;

        race_add_route_node( -2988,    0,    88, 0, 0, nodeStart );
        race_add_route_node(  1146, 2300, -2448, 0, 0, nodeEnd );

        race_set_route_name( "Place of Death", "start\215finish" );
        race_set_route_timeout( 40 );
        race_set_route_weapon_mode( raceWeaponNo );
        race_set_route_falsestart_mode( raceFalseStartNo );

        race_route_add_end();
    }
    else if ( streq( g_globalvars.mapname, "escape2a" ) )
    {
        if ( !race_route_add_start() )
            return;

        race_add_route_node( -3633, -1819,  3748, 0, 0, nodeStart );
        race_add_route_node(  1847, -2046, -3613, 0, 0, nodeEnd );

        race_set_route_name( "Space Odyssey", "start\215finish" );
        race_set_route_timeout( 90 );
        race_set_route_weapon_mode( raceWeaponNo );
        race_set_route_falsestart_mode( raceFalseStartNo );

        race_route_add_end();
    }
	else if (streq (g_globalvars.mapname, "hoppa2"))
	{
		if ( !race_route_add_start() )
			return;

		race_add_route_node(  1200, -3075,  -24, 0, 0, nodeStart );
		race_add_route_node(  1168, -1375,  130, 0, 0, nodeEnd );

		race_set_route_name( "One way", "start\215finish" );
		race_set_route_timeout( 60 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();

		race_set_teleport_flags_by_name ("tele_1", RACEFLAG_TOUCH_RACEFAIL);
	}
	else if (streq (g_globalvars.mapname, "speedrush"))
	{
		if ( !race_route_add_start() )
			return;

		race_add_route_node(  -955, -1140,  2010, 0, 90, nodeStart );
		race_add_route_node( -2675, -1155, -2984, 0, 0, nodeEnd );

		race_set_route_name( "Rush", "start\215finish" );
		race_set_route_timeout( 60 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();

		if ( !race_route_add_start() )
			return;

		race_add_route_node(  -955, -1140,  2010, 0, 90, nodeStart );
		race_add_route_node( -2675, -1155, -2984, 0, 0, nodeEnd );

		race_set_route_name( "Rush (+weapons)", "start\215finish" );
		race_set_route_timeout( 60 );
		race_set_route_weapon_mode( raceWeaponAllowed );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();

		race_set_teleport_flags_by_name ( "killer", RACEFLAG_TOUCH_RACEFAIL );
		race_set_teleport_flags_by_name ( "start", RACEFLAG_TOUCH_RACEEND );
	}
	else if (streq (g_globalvars.mapname, "zjumps")) {
		if ( !race_route_add_start() )
			return;

		race_add_route_node( -3740,  3950,  3865, 0, -90, nodeStart );
		race_add_route_node( -3880,  2720,  3830, 0, 0, nodeEnd );

		race_set_route_name( "Zzzzzzz", "start\215finish" );
		race_set_route_timeout( 60 );
		race_set_route_weapon_mode( raceWeaponNo );
		race_set_route_falsestart_mode( raceFalseStartNo );

		race_route_add_end();

		race_set_teleport_flags_by_name ( "start", RACEFLAG_TOUCH_RACEFAIL );
	}
}

qbool race_handle_event (gedict_t* player, gedict_t* entity, const char* eventName)
{
	int player_num = NUM_FOR_EDICT(player) - 1;
	if ( !player->racer )
		return false; // can't touch if not the racer

	if (streq (eventName, "touch")) {
		if (entity->race_flags & RACEFLAG_TOUCH_RACEFAIL)
		{
			// do some sound
			sound( player, CHAN_ITEM, "boss2/idle.wav", 1, ATTN_NONE );

			race_start( true, "Run aborted\n" );

			return true;
		}
		else if (entity->race_flags & RACEFLAG_TOUCH_RACEEND)
		{
			race_end_point_touched (entity, player);

			return true;
		}
	}
	else if (streq(eventName, "jump") && race.status == raceActive) {
		race_capture_t* capture = &player_captures[player_num];
		if (capture->jump_count < sizeof(capture->jumps) / sizeof(capture->jumps[0])) {
			capture->jumps[capture->jump_count].race_time = g_globalvars.time - race.start_time;
			VectorCopy(player->s.v.origin, capture->jumps[capture->jump_count].origin);
			++capture->jump_count;
		}
	}
	else if (streq(eventName, "watermove")) {
		if ( ( player->s.v.watertype == CONTENT_LAVA ) || ( player->s.v.watertype == CONTENT_SLIME ) ) {
			if ( player->racer && race.status ) {
				G_bprint(PRINT_HIGH, "%s failed %s run\n", player->s.v.netname, g_his(player));
				race_end(player);
				return true;
			}
		}
	}
	else if (streq(eventName, "kill")) {
		if ( player->racer && race.status ) {
			G_bprint(PRINT_HIGH, "%s canceled %s run\n", player->s.v.netname, g_his(player));
			race_end(player);
			return true;
		}
		else if ( player->race_chasecam ) {
			return true;
		}
	}

	return false;
}

static float race_toggle_incr_cvar(char* cvar_name, float incr, float min, float max)
{
	float current = cvar(cvar_name);
	current += incr;
	if (current < min || current > max)
		current = min;
	cvar_fset(cvar_name, current);
	return current;
}

void race_pacemaker(void)
{
	int position = 0;
	char buffer[128];
	qbool ignoring_lines = false;
	int file_version = 1;

	if (!race_command_checks())
		return;

	if (race.status)
	{
		G_sprint(self, PRINT_HIGH, "Cannot change pacemaker settings while race is active.\n");
		return;
	}

	trap_CmdArgv(1, buffer, sizeof(buffer));
	if (streq(buffer, "headstart")) {
		float new_headstart = race_toggle_incr_cvar(RACE_INCR_PARAMS(HEADSTART));
		G_bprint(PRINT_HIGH, "%s changes pacemaker headstart to \20%.2fs\21\n", self->s.v.netname, new_headstart);
		return;
	}
	else if (streq(buffer, "trail")) {
		float new_resolution = race_toggle_incr_cvar(RACE_INCR_PARAMS(RESOLUTION));
		if (new_resolution) {
			G_bprint(PRINT_HIGH, "%s changes pacemaker trail to \20%.2fs\21\n", self->s.v.netname, new_resolution / 10.0);
		}
		else {
			G_bprint(PRINT_HIGH, "%s changes pacemaker trail \20off\21\n", self->s.v.netname);
		}
		return;
	}
	else if (streq(buffer, "jumps")) {
		qbool enabled = !cvar(RACE_PACEMAKER_JUMPS_CVAR);

		G_bprint(PRINT_HIGH, "%s changes pacemaker jump indicators \20%s\21\n", self->s.v.netname, enabled ? "on" : "off");

		cvar_fset(RACE_PACEMAKER_JUMPS_CVAR, enabled ? 1 : 0);
		return;
	}
	else if (streq(buffer, "off") || (guide.capture.position_count != 0 && trap_CmdArgc() == 1)) {
		G_bprint(PRINT_HIGH, "%s disables the pacemaker\n", self->s.v.netname);
		cvar_fset(RACE_PACEMAKER_ENABLED_CVAR, 0);
		memset(&guide, 0, sizeof(guide));
		return;
	}
	else {
		position = 0;
		if (trap_CmdArgc() == 2) {
			position = atoi(buffer);

			if (position == 0 && buffer[0] != '0') {
				G_sprint(self, PRINT_HIGH, "Unknown pacemaker command '%s'.\n", buffer);
				return;
			}

			--position;
		}

		if (position < 0 || position >= sizeof(race.records) / sizeof(race.records[0]) || race.records[position].time >= RACE_INVALID_RECORD_TIME)
		{
			G_sprint(self, PRINT_HIGH, "No race record #%2d.\n", position + 1);
			return;
		}
	}

	// Try and load
	race_fropen("race/%s.pos", race.records[position].demoname);
	if (race_fhandle < 0)
	{
		G_sprint(self, PRINT_HIGH, "Unable to load positions for record #%2d.\n", position + 1);
		return;
	}

	race_clear_pacemaker();
	ignoring_lines = false;
	while (race_fgets(buffer, sizeof(buffer)))
	{
		char* ch = buffer;
		qbool error = false;
		int i;

		if (!strncmp(buffer, "version ", sizeof("version ") - 1)) {
			file_version = atoi(buffer + sizeof("version ") - 1);
			ignoring_lines = true;
		}

		if (!strncmp(buffer, "player ", sizeof("player ") - 1)) {
			int this_player = atoi(buffer + sizeof("player ") - 1);

			ignoring_lines = (this_player == race.records[position].playernumber);
		}

		if (ignoring_lines) {
			continue;
		}

		if (!strncmp(buffer, "jump,", 5)) {
			race_capture_jump_t* jump;

			if (guide.capture.jump_count >= sizeof(guide.capture.jumps) / sizeof(guide.capture.jumps[0])) {
				continue;
			}

			jump = &guide.capture.jumps[guide.capture.jump_count];

			ch = buffer + 5;
			jump->race_time = atof(ch);
			ch = strchr(ch, ',');

			for (i = 0; i < 3; ++i) {
				if (error = !ch)
					break;
				jump->origin[i] = atof(ch + 1);
				ch = strchr(ch + 1, ',');
			}

			if (!error) {
				++guide.capture.jump_count;
			}
		}
		else {
			race_capture_pos_t* capture;
			if (guide.capture.position_count >= sizeof(guide.capture.positions) / sizeof(guide.capture.positions[0])) {
				continue;
			}

			capture = &guide.capture.positions[guide.capture.position_count];
			capture->race_time = atof(ch);
			ch = strchr(ch, ',');

			for (i = 0; i < 3; ++i) {
				if (error = !ch)
					break;
				capture->origin[i] = atof(ch + 1);
				ch = strchr(ch + 1, ',');
			}
			for (i = 0; i < 2; ++i) {
				if (error = !ch)
					break;
				capture->angles[i] = atof(ch + 1);
				ch = strchr(ch + 1, ',');
			}

			if (!error) {
				++guide.capture.position_count;
			}
		}
	}

	race_fclose();

	guide.position = guide.jump = 0;
	strlcpy(race.pacemaker_nick, race.records[position].racername, sizeof(race.pacemaker_nick));
	race.pacemaker_time = race.records[position].time;

	G_bprint(PRINT_HIGH, "%s sets pacemaker \20%s\21 (%.3f)\n", self->s.v.netname, race.records[position].racername, race.records[position].time / 1000.0f);
	cvar_fset(RACE_PACEMAKER_ENABLED_CVAR, 1);
}

qbool race_pacemaker_enabled(void)
{
	return cvar(RACE_PACEMAKER_ENABLED_CVAR) && guide.capture.position_count;
}

static void race_pacemaker_announce(gedict_t* pacemaker)
{
	gedict_t* p;

	for (p = world; p = find_client(p); /**/) {
		if (pacemaker) {
			stuffcmd(p, "//ktx race pm %d\n", NUM_FOR_EDICT(pacemaker));
		}
		else {
			stuffcmd(p, "//ktx race pm 0\n");
		}
	}
}

static gedict_t* race_pacemaker_entity(qbool create_if_needed)
{
	gedict_t* ent = ez_find(world, "race_pacemaker");

	if (ent == NULL && create_if_needed) {
		ent = spawn();
		ent->s.v.classname = "race_pacemaker";
		ent->walkframe = 0;
		ent->s.v.frame = 0;
		ent->s.v.movetype = MOVETYPE_NONE;
		ent->s.v.flags = 0;
		setmodel(ent, "progs/player.mdl");
		setorigin(ent, 0, 0, 0);
		setsize(ent, PASSVEC3( VEC_HULL_MIN ), PASSVEC3( VEC_HULL_MAX ));
	}

	return ent;
}

static race_capture_pos_t* race_store_position(race_capture_t* capture, float time, float x, float y, float z, float angle_x, float angle_y)
{
	race_capture_pos_t* pos;

	if (capture->position_count >= RACE_MAX_CAPTURES)
		return NULL;

	pos = &capture->positions[capture->position_count];
	pos->race_time = time;
	VectorSet(pos->origin, x, y, z);
	pos->angles[0] = angle_x;
	pos->angles[1] = angle_y;

	++capture->position_count;
	return pos;
}

static void race_save_position(void)
{
	race_capture_pos_t* pos;
	float next_time;
	vec3_t diff, new_origin;
	gedict_t* racer;
	float race_time = g_globalvars.time - race.start_time;

	for (racer = world; racer = race_find_racer(racer); /**/) {
		int player_num = NUM_FOR_EDICT(racer) - 1;
		race_capture_t* capture = &player_captures[player_num];

		if (capture->position_count == 0 || capture->position_count >= RACE_MAX_CAPTURES - 1)
			return;

		pos = &capture->positions[capture->position_count - 1];
		next_time = pos->race_time + 1.0 / RACE_CAPTURE_FPS;
		while (pos && race_time > next_time)
		{
			float frac = (next_time - pos->race_time) / (race_time - pos->race_time);

			VectorSubtract(racer->s.v.origin, pos->origin, diff);
			VectorMA(pos->origin, frac, diff, new_origin);
			pos = race_store_position(capture, next_time, PASSVEC3(new_origin), racer->s.v.angles[0], racer->s.v.angles[1]);
			next_time += 1.0 / RACE_CAPTURE_FPS;
		}
	}
}

static void race_init_capture(void)
{
	gedict_t* racer;

	memset(&player_captures, 0, sizeof(player_captures));
	for (racer = world; racer = race_find_racer(racer); /**/) {
		int player_num = NUM_FOR_EDICT(racer) - 1;

		race_store_position(&player_captures[player_num], 0, PASSVEC3(racer->s.v.origin), racer->s.v.angles[0], racer->s.v.angles[1]);	
	}
}

static void race_finish_capture(qbool store, char* filename)
{
	// Multi-racing: only store positions for those setting a record?
	gedict_t* racer;

	if (store && race.race_recording) {
		race_fwopen("race/%s.pos", filename);
		race_fprintf("version %d\n", POS_FILE_VERSION);

		for (racer = world; racer = race_find_race_participants(racer); /**/) {
			int player_num = NUM_FOR_EDICT(racer) - 1;
			race_capture_t* capture = &player_captures[player_num];

			// Didn't set a time?  Skip.
			if (!race.currentrace[player_num].time)
				continue;

			race_fprintf("player %d\n", player_num);
			race_store_position(capture, g_globalvars.time - race.start_time, PASSVEC3(racer->s.v.origin), racer->s.v.angles[0], racer->s.v.angles[1]);
			if (race_fhandle >= 0) {
				int i = 0;
				for (i = 0; i < capture->position_count; ++i) {
					race_capture_pos_t* pos = &capture->positions[i];

					race_fprintf("%.3f,%.1f,%.1f,%.1f,%.1f,%.1f\n", pos->race_time, PASSVEC3(pos->origin), pos->angles[0], pos->angles[1]);
				}
				for (i = 0; i < capture->jump_count; ++i) {
					race_capture_jump_t* jump = &capture->jumps[i];

					race_fprintf("jump,%.3f,%.1f,%.1f,%.1f\n", jump->race_time, PASSVEC3(jump->origin));
				}
			}
		}

		race_fclose();
	}

	memset(&player_captures, 0, sizeof(player_captures));
}

static void race_remove_jump_markers(void)
{
	int i;
	for (i = 0; i < RACE_JUMP_INDICATORS; ++i) {
		if (race_jump_indicators[i]) {
			ent_remove(race_jump_indicators[i]);
			race_jump_indicators[i] = NULL;
		}
	}
	guide.jump = 0;
}

static void race_reset_pacemaker_route(void)
{
	gedict_t* plr;

	race_remove_pacemaker_indicator();
	for (plr = world; plr = find_plr(plr); /**/) {
		plr->race_closest_guide_pos = 0;
	}
	race_remove_jump_markers();
}

static void race_pacemaker_race_start(void)
{
	gedict_t* racer;

	for (racer = world; racer = race_find_racer(racer); /**/) {
		racer->race_closest_guide_pos = 0;
	}

	if (race_pacemaker_enabled()) {
		gedict_t* ent = race_pacemaker_entity(true);
		if (ent) {
			race_pacemaker_announce(ent);
			setorigin(ent, PASSVEC3(guide.capture.positions[0].origin));
		}
	}
	else {
		race_remove_pacemaker_indicator();
	}
	race_remove_jump_markers();
}

// Sets race_closest_guide_pos to the nearest guide location for each racer
static void race_update_closest_positions(void)
{
	gedict_t* racer;
	int i;

	if (!guide.capture.position_count) {
		// No guide loaded
		return;
	}

	for (racer = world; racer = race_find_racer(racer); /**/) {
		if (racer->race_closest_guide_pos < guide.capture.position_count) {
			float closest_distance = 0.0f;
			vec3_t diff;
			int max_record;

			VectorSubtract(racer->s.v.origin, guide.capture.positions[racer->race_closest_guide_pos].origin, diff);
			closest_distance = VectorLength(diff);

			// Hmm - this could break if the fastest record goes backwards for more than 10 frames...
			// Previously it searched from the end, but with more than one racer?...
			max_record = min(racer->race_closest_guide_pos + 10, guide.capture.position_count);
			for (i = racer->race_closest_guide_pos + 1; i < max_record; ++i) {
				float distance;

				VectorSubtract(racer->s.v.origin, guide.capture.positions[i].origin, diff);
				distance = VectorLength(diff);

				if (distance <= closest_distance) {
					closest_distance = distance;
					racer->race_closest_guide_pos = i;
					max_record = min(racer->race_closest_guide_pos + 10, guide.capture.position_count);
				}
			}
		}
	}
}

static void race_update_pacemaker(void)
{
	float race_time;
	float frac;
	qbool advanced = false;
	int i;
	qbool removal_required = !race_pacemaker_enabled();

	if (framecount < 5) {
		return;
	}

	switch (race.status) {
	case raceNone:
		race_time = -10.0f;
		removal_required = true;
		break;
	case raceActive:
		race_time = g_globalvars.time - race.start_time;
		break;
	case raceCD:
		if (race.cd_cnt) {
			guide.jump = guide.position = 0;
		}
		race_time = -(race.cd_cnt + (race.cd_next_think - g_globalvars.time));
		break;
	}

	race_time += bound(RACE_PACEMAKER_HEADSTART_MIN, cvar(RACE_PACEMAKER_HEADSTART_CVAR), RACE_PACEMAKER_HEADSTART_MAX);

	// If we're finished or player disabled feature, remove the guide
	if (removal_required) {
		race_reset_pacemaker_route();
		return;
	}

	// We want a guide, but haven't started racing yet
	if (race_time <= 0) {
		guide.position = 0;
	}

	// Advance pointer until at correct record
	while (guide.position < guide.capture.position_count - 1 && guide.capture.positions[guide.position + 1].race_time < race_time) {
		++guide.position;
		advanced = true;
	}

	// If the guide ends the map, remove pacemaker, leave the guides
	if (guide.position >= guide.capture.position_count - 1) {
		race_remove_pacemaker_indicator();
	}

	// Guide capture position
	if (race_time > 0 && guide.position >= 0 && guide.position < guide.capture.position_count - 1) {
		race_capture_pos_t* cur = &guide.capture.positions[guide.position];
		race_capture_pos_t* next = &guide.capture.positions[guide.position + 1];
		vec3_t direction;

		// Sanity...
		if (next->race_time > cur->race_time) {
			gedict_t* ent = race_pacemaker_entity(true);
			vec3_t new_origin;

			frac = (race_time - cur->race_time) / (next->race_time - cur->race_time);
			VectorSubtract(next->origin, cur->origin, direction);

			VectorMA(cur->origin, frac, direction, new_origin);

			setorigin(ent, PASSVEC3(new_origin));
			vectoangles(direction, ent->s.v.angles);

			if (advanced) {
				if (ent->walkframe >= 6)
					ent->walkframe = 0;
				if (direction[0] < 0)
					ent->s.v.frame = 11 - ent->walkframe;
				else
					ent->s.v.frame = 6 + ent->walkframe;
				++ent->walkframe;
			}
		}
	}

	if (race.status) {
		int guide_start = 0;
		int guide_end = 0;

		int resolution = (int)bound(RACE_PACEMAKER_RESOLUTION_MIN, cvar(RACE_PACEMAKER_RESOLUTION_CVAR), RACE_PACEMAKER_RESOLUTION_MAX);
		if (resolution) {
			// From the pacemaker's position backwards
			guide_start = max(0, guide.position - RACE_PACEMAKER_TRAIL_COUNT * resolution);
			if (race.racers_competing == 1) {
				// In single race, we draw from player forwards, up to the pacemaker
				gedict_t* racer = race_get_racer();

				if (racer) {
					guide_start = min(guide_start, racer->race_closest_guide_pos);
				}
			}

			// Adjust for resolution, to stop LG jumping around as we move forwards
			guide_start = (guide_start / resolution) * resolution;
			guide_end = min(guide.position, min(guide_start + resolution * RACE_PACEMAKER_TRAIL_COUNT, guide.capture.position_count));

			// Create lightning trail for next part of route
			if (guide_start || guide_end) {
				int num = 0;

				for (i = guide_start; i < guide_end; i += resolution, ++num) {
					int next = min(i + resolution, guide.capture.position_count - 1);

					WriteByte(MSG_MULTICAST, SVC_TEMPENTITY);
					WriteByte(MSG_MULTICAST, TE_LIGHTNING2);
					WriteShort(MSG_MULTICAST, RACE_GUIDE_BASE_ENT + num);
					WriteCoord(MSG_MULTICAST, guide.capture.positions[i].origin[0]);
					WriteCoord(MSG_MULTICAST, guide.capture.positions[i].origin[1]);
					WriteCoord(MSG_MULTICAST, guide.capture.positions[i].origin[2]);
					WriteCoord(MSG_MULTICAST, guide.capture.positions[next].origin[0]);
					WriteCoord(MSG_MULTICAST, guide.capture.positions[next].origin[1]);
					WriteCoord(MSG_MULTICAST, guide.capture.positions[next].origin[2]);
					trap_multicast(PASSVEC3(guide.capture.positions[i].origin), MULTICAST_PHS);
				}

				update_jump_markers(race_time, guide_start, resolution);
			}
		}
	}
}

static void update_jump_markers(float race_time, int guide_start, int resolution)
{
	float guide_race_time = guide_start >= guide.capture.position_count ? 99999.9f : guide.capture.positions[guide_start].race_time;
	qbool jumps_enabled = cvar(RACE_PACEMAKER_JUMPS_CVAR) && resolution;
	int i;

	// Remove old indicators
	while (race_jump_indicators[0] && (race_jump_indicator_times[0] < guide_race_time || !jumps_enabled)) {
		if (race_jump_indicators[0]) {
			ent_remove(race_jump_indicators[0]);
		}

		memcpy(race_jump_indicator_times, race_jump_indicator_times + 1, sizeof(race_jump_indicator_times[0]) * (RACE_JUMP_INDICATORS - 1));
		memcpy(race_jump_indicators, race_jump_indicators + 1, sizeof(race_jump_indicators[0]) * (RACE_JUMP_INDICATORS - 1));
		race_jump_indicators[RACE_JUMP_INDICATORS - 1] = NULL;
		race_jump_indicator_times[RACE_JUMP_INDICATORS - 1] = -1.0f;
	}

	// Create new indicators
	if (jumps_enabled && guide.jump < guide.capture.jump_count && guide.capture.jumps[guide.jump].race_time <= race_time) {
		for (i = 0; i < RACE_JUMP_INDICATORS; ++i) {
			if (!race_jump_indicators[i]) {
				gedict_t* ent = spawn();
				ent->s.v.classname = "race_pacemaker_jump";
				ent->s.v.movetype = MOVETYPE_NONE;
				ent->s.v.solid = SOLID_NOT;
				if (k_ctf_custom_models) {
					setmodel(ent, "progs/star.mdl");
				}
				else {
					setmodel(ent, "progs/lavaball.mdl");
				}
				setorigin(ent, PASSVEC3(guide.capture.jumps[guide.jump].origin));
				setsize(ent, PASSVEC3(VEC_HULL_MIN), PASSVEC3(VEC_HULL_MAX));
				droptofloor(ent);
				setorigin(ent, ent->s.v.origin[0], ent->s.v.origin[1], ent->s.v.origin[2] - 16);

				race_jump_indicators[i] = ent;
				race_jump_indicator_times[i] = guide.capture.jumps[guide.jump].race_time;

				++guide.jump;
				break;
			}
		}
	}
}

static void race_clear_pacemaker(void)
{
	memset(&guide, 0, sizeof(guide));
	race_remove_pacemaker_indicator();
}

static void race_remove_pacemaker_indicator(void)
{
	gedict_t* ent;
	while (ent = race_pacemaker_entity(false)) {
		ent_remove(ent);
		race_pacemaker_announce(NULL);
	}
}

qbool race_end(gedict_t* racer)
{
	racer->racer = false;
	racer->muted = true;
	race_set_one_player_movetype_and_etc(racer);

	if (race_get_racer() == NULL) {
		race_over();
		return true;
	}

	return false;
}

static char* race_position_string(int position)
{
	static char buffer[64];
	char* positions[] = {
		"1st place",
		"2nd place",
		"3rd place",
		"4th place",
		"5th place",
		"6th place",
		"7th place",
		"8th place",
		"9th place",
		"10th place"
	};

	if (position >= 1 && position <= sizeof(positions) / sizeof(positions[0])) {
		return positions[position - 1];
	}
	return "";
}

static qbool race_simultaneous(void)
{
	return cvar(RACE_SIMULTANEOUS_CVAR);
}

void race_player_post_think(void)
{
	if ( isRACE() )
	{
		// test for multirace
		self->s.v.solid = SOLID_NOT;
		setorigin (self, PASSVEC3( self->s.v.origin ) );
	}

	race_follow();
}

static int race_encode_user_command(gedict_t* player)
{
	int result = (player->s.v.button0 ? 1 : 0) + (player->s.v.button2 ? 2 : 0);

	if (player->movement[0] > 0)
		result += 4;
	else if (player->movement[0] < 0)
		result += 8;

	if (player->movement[1] > 0)
		result += 16;
	else if (player->movement[1] < 0)
		result += 32;

	if (player->movement[2] > 0)
		result += 64;
	else if (player->movement[2] < 0)
		result += 128;

	return result;
}

void race_player_pre_think(void)
{
	if ( isRACE() )
	{
		// Set this player to solid so we trigger checkpoints & teleports during move
		self->s.v.solid = (race.status == raceNone || self->racer ? SOLID_SLIDEBOX : SOLID_NOT);
		setorigin (self, PASSVEC3( self->s.v.origin ) );

		if ( self->ct == ctPlayer && !self->racer && race.status )
		{
			if ( self->race_chasecam )
			{
				self->s.v.flags = ( ( int ) ( self->s.v.flags ) ) | FL_JUMPRELEASED;
			}
		}
		else if ( self->ct == ctPlayer && self->racer && race.status )
		{
			stuffcmd_flags (self, STUFFCMD_DEMOONLY, "//ucmd %f %d\n", g_globalvars.time, race_encode_user_command(self));
		}
	}
}

void race_simultaneous_toggle(void)
{
	if ( !race_command_checks() )
		return;

	if ( race_is_started() )
		return;

	cvar_toggle_msg(self, RACE_SIMULTANEOUS_CVAR, redtext("simultaneous racing"));
}
