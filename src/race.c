//
// race.c - race implementation, yeah got inspiration from ktpro (c) (tm) (etc)
//

#include "g_local.h"

void SP_info_intermission();

#define MAX_TXTLEN	128
#define TOP_FILE_VERSION 2
#define POS_FILE_VERSION 2

#define RACEFLAG_TOUCH_RACEFAIL 1
#define RACEFLAG_TOUCH_RACEEND  2
#define RACEFLAG_ENTITY_KEEP    4

#define RACE_MAX_CAPTURES     600
#define RACE_CAPTURE_FPS       10
#define RACE_GUIDE_BASE_ENT    (MAX_CLIENTS + 1)
#define RACE_JUMP_INDICATORS    4

#define RACE_PACEMAKER_JUMPS_CVAR      "k_race_pace_jumps"
#define RACE_PACEMAKER_LEGAL_RECORD    "k_race_pace_legal"
#define RACE_PACEMAKER_HEADSTART_CVAR  "k_race_pace_headstart"
#define RACE_PACEMAKER_RESOLUTION_CVAR "k_race_pace_resolution"
#define RACE_PACEMAKER_ENABLED_CVAR    "k_race_pace_enabled"
#define RACE_SIMULTANEOUS_CVAR         "k_race_simultaneous"
#define RACE_SCORINGSYSTEM_CVAR        "k_race_scoring_system"
#define RACE_MATCH_CVAR                "k_race_match"
#define RACE_MATCH_ROUNDS_CVAR         "k_race_match_rounds"
#define RACE_ROUTE_NUMBER_CVAR         "k_race_route_number"
#define RACE_ROUTE_MAPNAME_CVAR        "k_race_route_mapname"
#define RACE_PACEMAKER_HEADSTART_MIN   0.00f
#define RACE_PACEMAKER_HEADSTART_MAX   1.00f
#define RACE_PACEMAKER_HEADSTART_INCR  0.25f
#define RACE_PACEMAKER_RESOLUTION_MIN  0
#define RACE_PACEMAKER_RESOLUTION_MAX  3
#define RACE_PACEMAKER_RESOLUTION_INCR 1

#define RACE_PACEMAKER_TRAIL_COUNT     12
#define RACE_MIN_MATCH_ROUNDS           3
#define RACE_MAX_MATCH_ROUNDS          21

#define RACE_INCR_PARAMS(name) (RACE_PACEMAKER_##name##_CVAR),(RACE_PACEMAKER_##name##_INCR),(RACE_PACEMAKER_##name##_MIN),(RACE_PACEMAKER_##name##_MAX)

void UserMode_SetMatchTag(char *matchtag);

typedef struct race_capture_pos_s
{
	float race_time;
	vec3_t origin;
	vec_t angles[2];
} race_capture_pos_t;

typedef struct race_capture_jump_s
{
	float race_time;
	vec3_t origin;
} race_capture_jump_t;

typedef struct race_capture_s
{
	race_capture_pos_t positions[1200];
	race_capture_jump_t jumps[100];
	int position_count;
	int jump_count;
} race_capture_t;

typedef struct race_capture_playback_s
{
	race_capture_t capture;
	int position;
	int jump;
} race_capture_playback_t;

race_capture_t player_captures[MAX_CLIENTS];
race_capture_playback_t guide;

typedef struct race_player_match_info_s
{
	race_capture_t best_run_capture;
	int wins;
	int completions;
	float times[RACE_MAX_MATCH_ROUNDS + 1];
	float best_time;
	float total_time;
	float total_distance;
} race_player_match_info_t;

static race_player_match_info_t player_match_info[MAX_CLIENTS];

static void update_jump_markers(float race_time, int guide_start, int resolution);
static gedict_t *race_jump_indicators[RACE_JUMP_INDICATORS];
static float race_jump_indicator_times[RACE_JUMP_INDICATORS];

void ktpro_autotrack_on_powerup_take(gedict_t *racer);
static void race_cancel(qbool cancelrecord, const char *fmt, ...) PRINTF_FUNC(2);
static void race_start(qbool cancelrecord, const char *fmt, ...) PRINTF_FUNC(2);
static void race_fwopen(const char *fmt, ...) PRINTF_FUNC(1);
static void race_fropen(const char *fmt, ...) PRINTF_FUNC(1);
static void race_fprintf(const char *fmt, ...) PRINTF_FUNC(1);
void race_unready_all(void);
void r_route(void);
void race_record(void);
void race_stoprecord(qbool cancel);
void race_remove_ent(void);
void race_set_players_movetype_and_etc(void);
void race_cleanmap(void);
void unmute_all_players(void);
void HideSpawnPoints();
void ShowSpawnPoints();
void kill_race_idler(void);
void write_topscores(void);
void read_topscores(void);
void init_scores(void);
qbool race_match_mode(void);
qbool race_match_started(void);
int race_award_points(int position, int participants);

gedict_t* race_find_race_participants(gedict_t *p);
qbool race_pacemaker_enabled(void);
qbool is_rules_change_allowed(void);
qbool race_command_checks(void);
qbool race_is_started(void);

static void race_update_pacemaker(void);
static void race_clear_pacemaker(void);
static void race_init_capture(void);
static void race_save_position(void);
static void race_finish_capture(qbool store, int player_num);
static void race_pacemaker_race_start(void);
static void race_remove_pacemaker_indicator(void);
static void race_make_active_racer(gedict_t *r, gedict_t *s);
static qbool race_end(gedict_t *racer, qbool valid, qbool complete);
static char* race_position_string(int position);
static qbool race_simultaneous(void);
static void race_update_closest_positions(void);
static void race_match_round_end(void);

static float race_vlen(vec3_t velocity)
{
	return sqrt(velocity[0] * velocity[0] + velocity[1] * velocity[1]);
}

fileHandle_t race_fhandle = -1;
race_t race; // whole race struct

char* classname_for_nodeType(raceRouteNodeType_t nodeType);

#define RACE_INVALID_RECORD_TIME 999999
#define RACE_DEFAULT_TIMEOUT 60
#define RACE_MAX_TIMEOUT 3600

static int next_route = -1; // STATIC

//============================================

static void set_usercmd_trace(gedict_t* p, qbool on)
{
	int userId = atoi(ezinfokey(p, "*userid"));

	if (userId)
	{
		localcmd("sv_usercmdtrace %d %s\n", userId, on ? "on" : "off");
		trap_executecmd();
	}
}

static void clearall_usercmds_settings(void)
{
	gedict_t* p;

	for (p = world; (p = find_plr(p)); )
	{
		set_usercmd_trace(p, false);
	}
}

//============================================

static int get_server_port(void)
{
	char *ip, *port;
	int i = 0;

	if (strnull(ip = cvar_string("sv_local_addr")) || strnull(port = strchr(ip, ':'))
			|| !(i = atoi(port + 1)))
	{
		return 27500;
	}
	else
	{
		return i;
	}
}

static const char* race_filename(const char *extension)
{
	static char filename[128];

	if (cvar("k_race_times_per_port"))
	{
		snprintf(filename, sizeof(filename), "race/race[%s_r%02d]-w%1ds%1d_%d.%s",
					mapname, race.active_route, race.weapon, race.falsestart,
					get_server_port(), extension);
	}
	else
	{
		snprintf(filename, sizeof(filename), "race/race[%s_r%02d]-w%1ds%1d.%s",
					mapname, race.active_route, race.weapon, race.falsestart,
					extension);
	}

	return filename;
}

qbool isRACE(void)
{
	return (cvar("k_race"));
}

static qbool is_valid_record(raceRecord_t *record)
{
	return (record->time < RACE_INVALID_RECORD_TIME);
}

static int read_record_param(int param)
{
	char arg_1[64] =
		{ 0 };

	if (trap_CmdArgc() <= param)
	{
		return 0;
	}

	trap_CmdArgv(param, arg_1, sizeof(arg_1));

	return bound(0, atoi(arg_1) - 1, NUM_BESTSCORES - 1);
}

void ToggleRace(void)
{
	if (!isRACE() && bots_enabled())
	{
		G_sprint(self, PRINT_HIGH, "Disable bots first with %s\n", redtext("/botcmd disable"));

		return;
	}

	if (!isRACE() && !is_rules_change_allowed())
	{
		return;
	}

	if (!isRACE())
	{
		if (!isFFA())
		{
			UserMode(-6);
		}
	}

	if (CountPlayers() && race_is_started())
	{
		return;
	}

	cvar_toggle_msg(self, "k_race", redtext("race"));

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
	"serverinfo ktxmode race\n"
	"k_race_pace_jumps 0\n"
	"k_race_pace_legal 0\n"
	"k_race_pace_headstart 0.5\n"
	"k_race_pace_resolution 2\n"
	"k_race_pace_enabled 0\n"
	"k_race_simultaneous 1\n"
	"k_race_scoring_system 0\n"
	"k_race_match 0\n"
	"k_race_match_rounds 9\n"
	"timelimit 0\n"
	"fraglimit 0\n"
	"qtv_sayenabled 1\n";

static char norace_settings[] =
	"sv_silentrecord 0\n"
	"lock_practice 0\n"
	"srv_practice_mode 0\n"
	"allow_toggle_practice 5\n"
	"serverinfo ktxmode \"\"\n"
	"qtv_sayenabled 0\n";

void apply_race_settings(void)
{
	char buf[1024 * 4];
	char *cfg_name;

	clearall_usercmds_settings();

	if (!isRACE())
	{
		race_stoprecord(true);

		unmute_all_players();

		// turn off race settings.
		trap_readcmd(norace_settings, buf, sizeof(buf));
		G_cprint("%s", buf);

		// Execute configs/reset.cfg and set k_defmode.
		execute_rules_reset();

		return;
	}

	// turn on race settings.
	UserMode_SetMatchTag("");

	trap_readcmd(race_settings, buf, sizeof(buf));
	G_cprint("%s", buf);

	cfg_name = va("configs/usermodes/race/default.cfg");
	if (can_exec(cfg_name))
	{
		trap_readcmd(va("exec %s\n", cfg_name), buf, sizeof(buf));
		G_cprint("%s", buf);
	}

	cfg_name = va("configs/usermodes/race/%s.cfg", mapname);
	if (can_exec(cfg_name))
	{
		trap_readcmd(va("exec %s\n", cfg_name), buf, sizeof(buf));
		G_cprint("%s", buf);
	}

	G_cprint("\n");
}

void race_cleanmap(void)
{
	gedict_t *p;

	for (p = world; (p = nextent(p));)
	{
		if (streq(p->classname, "weapon_nailgun") || streq(p->classname, "weapon_supernailgun")
				|| streq(p->classname, "weapon_supershotgun")
				|| streq(p->classname, "weapon_rocketlauncher")
				|| streq(p->classname, "weapon_grenadelauncher")
				|| streq(p->classname, "weapon_lightning") || streq(p->classname, "item_shells")
				|| streq(p->classname, "item_spikes") || streq(p->classname, "item_rockets")
				|| streq(p->classname, "item_cells") || streq(p->classname, "item_health")
				|| streq(p->classname, "item_armor1") || streq(p->classname, "item_armor2")
				|| streq(p->classname, "item_armorInv")
				|| streq(p->classname, "item_artifact_invulnerability")
				|| streq(p->classname, "item_artifact_envirosuit")
				|| streq(p->classname, "item_artifact_invisibility")
				|| streq(p->classname, "item_artifact_super_damage")
				|| streq(p->classname, "item_armor1") || streq(p->classname, "item_armor2")
				|| streq(p->classname, "item_armorInv"))
		{
			ent_remove(p);
			continue;
		}

		if (p->race_flags & RACEFLAG_ENTITY_KEEP)
		{
			continue;
		}

		if (streq(p->classname, "door"))
		{
			ent_remove(p);
			continue;
		}
	}
}

//===========================================

int race_time(void)
{
	if (race.status != raceActive)
	{
		return 0; // count time only when race in state raceActive
	}

	return ((g_globalvars.time - race.start_time) * 1000);
}

void setwepall(gedict_t *p)
{
	gedict_t *swap;

	p->s.v.ammo_nails = 255;
	p->s.v.ammo_shells = 255;
	p->s.v.ammo_rockets = 255;
	p->s.v.ammo_cells = 255;
	p->s.v.items = IT_AXE | IT_SHOTGUN | IT_SUPER_SHOTGUN | IT_NAILGUN | IT_SUPER_NAILGUN |
	IT_GRENADE_LAUNCHER | IT_ROCKET_LAUNCHER | IT_LIGHTNING;
	p->lastwepfired = 0;

	swap = self;
	self = p;

	if (!((int)self->s.v.weapon & (int)self->s.v.items))
	{
		self->s.v.weapon = IT_ROCKET_LAUNCHER;
	}

	W_SetCurrentAmmo();
	self = swap;
}

void setwepnone(gedict_t *p)
{
	gedict_t *swap;

	p->s.v.ammo_nails = 0;
	p->s.v.ammo_shells = 0;
	p->s.v.ammo_rockets = 0;
	p->s.v.ammo_cells = 0;
	p->s.v.items = 0;
	p->lastwepfired = 0;

	swap = self;
	self = p;

	self->s.v.weapon = W_BestWeapon();
	W_SetCurrentAmmo();

	self = swap;
}

char* race_route_name(void)
{
	int idx;

	if ((race.cnt < 0) || (race.cnt >= MAX_ROUTES))
	{
		G_Error("race_route_name: race.cnt %d", race.cnt);
	}

	idx = race.active_route - 1;

	if ((idx < 0) || (idx >= race.cnt))
	{
		return "custom";
	}

	return race.route[idx].name;
}

char* race_route_desc(void)
{
	int idx;

	if ((race.cnt < 0) || (race.cnt >= MAX_ROUTES))
	{
		G_Error("race_route_desc: race.cnt %d", race.cnt);
	}

	idx = race.active_route - 1;

	if ((idx < 0) || (idx >= race.cnt))
	{
		return "custom";
	}

	return race.route[idx].desc;
}

//============================================

void race_init(void)
{
	int i;

	memset(&race, 0, sizeof(race));

	race.timeout_setting = RACE_DEFAULT_TIMEOUT;

	race.warned = true;
	race.status = raceNone;

	race.weapon = raceWeaponAllowed;
	race.falsestart = raceFalseStartNo;

	for (i = 0; i < NUM_BESTSCORES; i++)
	{
		race.records[i].time = RACE_INVALID_RECORD_TIME;
	}

	race.rounds = bound(RACE_MIN_MATCH_ROUNDS, cvar(RACE_MATCH_ROUNDS_CVAR), RACE_MAX_MATCH_ROUNDS);
}

// clean up, so we can start actual match and there will be no some shit around
void race_shutdown(char *msg)
{
	race_cancel(true, "%s", msg);
	race_remove_ent();
	race_unready_all();
	if (cvar("k_spm_show"))
	{
		ShowSpawnPoints();
	}

	race_set_players_movetype_and_etc();
}

// make all players not ready for race, silently
void race_unready_all(void)
{
	gedict_t *p;

	for (p = world; (p = find_plr(p));)
	{
		p->race_ready = 0;
	}
}

//============================================

qbool race_route_add_start(void)
{
	if ((race.cnt < 0) || (race.cnt >= MAX_ROUTES))
	{
		return false;
	}

	race.route[race.cnt].weapon = raceWeaponAllowed; // default allowed
	race.route[race.cnt].timeout = 20; // default 20 sec
	race.route[race.cnt].falsestart = raceFalseStartNo; // default with falsestart

	return true;
}

void race_route_add_end(void)
{
	if ((race.cnt < 0) || (race.cnt >= MAX_ROUTES))
	{
		G_Error("race_route_add_end: race.cnt %d", race.cnt);
	}

	race.cnt++;
}

raceRouteNode_t* race_add_route_node(float x, float y, float z, float pitch, float yaw,
										raceRouteNodeType_t type)
{
	int node_idx;

	if ((race.cnt < 0) || (race.cnt >= MAX_ROUTES))
	{
		G_Error("race_add_route_node: race.cnt %d", race.cnt);
	}

	node_idx = race.route[race.cnt].cnt;

	if ((node_idx < 0) || (node_idx >= MAX_ROUTE_NODES))
	{
		return NULL; // we are full
	}

	race.route[race.cnt].node[node_idx].type = type;
	race.route[race.cnt].node[node_idx].org[0] = x;
	race.route[race.cnt].node[node_idx].org[1] = y;
	race.route[race.cnt].node[node_idx].org[2] = z;
	race.route[race.cnt].node[node_idx].ang[0] = pitch;
	race.route[race.cnt].node[node_idx].ang[1] = yaw;
	race.route[race.cnt].node[node_idx].ang[2] = 0;
	VectorClear(race.route[race.cnt].node[node_idx].sizes);

	race.route[race.cnt].cnt++; // one more node

	return &race.route[race.cnt].node[node_idx];
}

void race_set_route_name(char *name, char *desc)
{
	if ((race.cnt < 0) || (race.cnt >= MAX_ROUTES))
	{
		G_Error("race_set_route_name: race.cnt %d", race.cnt);
	}

	strlcpy(race.route[race.cnt].name, name, sizeof(race.route[0].name));
	strlcpy(race.route[race.cnt].desc, desc, sizeof(race.route[0].desc));
}

void race_set_route_timeout(float timeout)
{
	if ((race.cnt < 0) || (race.cnt >= MAX_ROUTES))
	{
		G_Error("race_set_route_timeout: race.cnt %d", race.cnt);
	}

	race.route[race.cnt].timeout = bound(1, timeout, 999);
}

void race_set_route_falsestart_mode(raceFalseStartMode_t falsestart)
{
	switch (falsestart)
	{
		case raceFalseStartNo:
		case raceFalseStartYes:
			break; // known

		default:
			G_Error("race_set_route_falsestart_mode: wrong type %d", falsestart);
	}

	race.route[race.cnt].falsestart = falsestart;
}

void race_set_route_weapon_mode(raceWeapoMode_t weapon)
{
	if ((race.cnt < 0) || (race.cnt >= MAX_ROUTES))
	{
		G_Error("race_set_route_weapon_mode: race.cnt %d", race.cnt);
	}

	switch (weapon)
	{
		case raceWeaponNo:
		case raceWeaponAllowed:
		case raceWeapon2s:
			break; // known

		default:
			G_Error("race_set_route_weapon_mode: wrong weapon %d", weapon);
	}

	race.route[race.cnt].weapon = weapon;
}

gedict_t* race_find_race_participants(gedict_t *p)
{
	p = find_plr(p);
	while (p && !p->race_participant)
	{
		p = find_plr(p);
	}

	return p;
}

gedict_t* race_find_racer(gedict_t *p)
{
	p = find_plr(p);
	while (p && !p->racer)
	{
		p = find_plr(p);
	}

	return p;
}

gedict_t* race_get_racer(void)
{
	return race_find_racer(world);
}

//===========================================

// remove all entitys with particular classname
void ent_remove_by_classname(char *classname)
{
	gedict_t *e;

	for (e = world; (e = ez_find(e, classname));)
	{
		ent_remove(e);
	}
}

// remove all possibile race entitys
void race_remove_ent(void)
{
	int i;

	for (i = 1; i < nodeMAX; i++)
	{
		ent_remove_by_classname(classname_for_nodeType(i));
	}
}

void race_record(void)
{
	if (race.cd_cnt && cvar("k_race_autorecord"))
	{
		if (!race_match_mode())
		{
			StartDemoRecord(); // start demo recording
		}

		race.race_recording = true;
	}
}

void race_stoprecord(qbool cancel)
{
	if (race.race_recording)
	{
		if (cancel)
		{
			localcmd("sv_democancel\n");  // stop recording demo and discard it
		}
		else
		{
			StatsToFile();
			localcmd("sv_demostop\n"); // stop recording demo and keep it
		}

		race.race_recording = false;
	}
}

//============================================

char* race_falsestart_mode(int start)
{
	switch (start)
	{
		case raceFalseStartNo:
			return "no falsestart";

		case raceFalseStartYes:
			return "falsestart enabled";

		default:
			G_Error("race_falsestart_mode: wrong race.falsestart %d", start);
	}

	return ""; // keep compiler silent
}

char* race_weapon_mode(int weapon)
{
	switch (weapon)
	{
		case raceWeaponNo:
			return "disallowed";

		case raceWeaponAllowed:
			return "allowed";

		case raceWeapon2s:
			return "allowed after 2s";

		default:
			G_Error("race_weapon_mode: wrong race.weapon %d", weapon);
	}

	return ""; // keep compiler silent
}

qbool race_weapon_allowed(gedict_t *p)
{
	if (!race.status)
	{
		return true; // not a race, so allowed
	}

	// below is case of RACE is somehow in progress

	if (race.status != raceActive)
	{
		return false; // allowe weapon in active state only
	}

	if (!p->racer)
	{
		return false; // not a racer
	}

	switch (race.weapon)
	{
		case raceWeaponNo:
			return false;

		case raceWeaponAllowed:
			return true;

		case raceWeapon2s:
			return ((race_time() >= 2000) ? true : false);

		default:
			G_Error("race_weapon_allowed: wrong race.weapon %d", race.weapon);
	}

	return false; // keep compiler silent
}

//============================================

char* name_for_nodeType(raceRouteNodeType_t nodeType)
{
	switch (nodeType)
	{
		case nodeStart:
			return "start checkpoint";

		case nodeCheckPoint:
			return "checkpoint";

		case nodeEnd:
			return "finish checkpoint";

		default:
			G_Error("name_for_nodeType: wrong nodeType %d", nodeType);
	}

	return ""; // keep compiler silent
}

char* classname_for_nodeType(raceRouteNodeType_t nodeType)
{
	switch (nodeType)
	{
		case nodeStart:
			return "race_cp_start";

		case nodeCheckPoint:
			return "race_cp";

		case nodeEnd:
			return "race_cp_end";

		default:
			G_Error("classname_for_nodeType: wrong nodeType %d", nodeType);
	}

	return ""; // keep compiler silent
}

char* model_for_nodeType(raceRouteNodeType_t nodeType)
{
	if (cvar("k_race_custom_models"))
	{
		switch (nodeType)
		{
			case nodeStart:
				return "progs/start.mdl";

			case nodeCheckPoint:
				return "progs/check.mdl";

			case nodeEnd:
				return "progs/finish.mdl";

			default:
				G_Error("model_for_nodeType: wrong nodeType %d", nodeType);
		}
	}
	else
	{
		switch (nodeType)
		{
			case nodeStart:
				return "progs/invulner.mdl";

			case nodeCheckPoint:
				return "progs/w_s_key.mdl";

			case nodeEnd:
				return "progs/invulner.mdl";

			default:
				G_Error("model_for_nodeType: wrong nodeType %d", nodeType);
		}
	}

	return ""; // keep compiler silent
}

char* touch_sound_for_nodeType(raceRouteNodeType_t nodeType)
{
	switch (nodeType)
	{
		case nodeStart:
			return "items/protect3.wav";

		case nodeCheckPoint:
			return "items/damage.wav";

		case nodeEnd:
			return "items/suit.wav";

		default:
			G_Error("touch_sound_for_nodeType: wrong nodeType %d", nodeType);
	}

	return ""; // keep compiler silent
}

char* spawn_sound_for_nodeType(raceRouteNodeType_t nodeType)
{
	switch (nodeType)
	{
		case nodeStart:
			return "items/protect.wav";

		case nodeCheckPoint:
			return "items/itembk2.wav";

		case nodeEnd:
			return "items/protect.wav";

		default:
			G_Error("spawn_sound_for_nodeType: wrong nodeType %d", nodeType);
	}

	return ""; // keep compiler silent
}

float volume_for_touch_sound_for_nodeType(raceRouteNodeType_t nodeType)
{
	switch (nodeType)
	{
		case nodeStart:
			return 0.5f;

		case nodeCheckPoint:
			return 0.3;

		case nodeEnd:
			return 0.5f;

		default:
			G_Error("volume_for_touch_sound_for_nodeType: wrong nodeType %d", nodeType);
	}

	return 1; // keep compiler silent
}

float blink_effects_for_nodeType(raceRouteNodeType_t nodeType)
{
	switch (nodeType)
	{
		case nodeStart:
			return ( EF_BLUE | EF_GREEN);

		case nodeCheckPoint:
			return EF_BLUE;

		case nodeEnd:
			return ( EF_BLUE | EF_GREEN);

		default:
			G_Error("volume_for_touch_sound_for_nodeType: wrong nodeType %d", nodeType);
	}

	return 0; // keep compiler silent
}

//============================================

// count start + end + intermediate checkpoints
int checkpoints_count(void)
{
	int cnt = 0, i;

	for (i = 1; i < nodeMAX; i++)
	{
		cnt += find_cnt( FOFCLSN, classname_for_nodeType(i));
	}

	return cnt;
}

//============================================

// count ready for race players
int race_count_ready_players(void)
{
	int cnt;
	gedict_t *p;

	for (cnt = 0, p = world; (p = find_plr(p));)
	{
		if (p->race_ready)
		{
			cnt++;
		}
	}

	return cnt;
}

//===========================================

void race_check_racer_falsestart(qbool nextracer)
{
	gedict_t *e;
	gedict_t *racer;

	for (racer = world; (racer = race_find_racer(racer)); /**/)
	{
		for (e = world; (e = ez_find(e, "race_cp_start"));)
		{
			if ((racer->s.v.origin[0] != e->s.v.origin[0])
					&& (racer->s.v.origin[1] != e->s.v.origin[1]))
			{
				if (nextracer)
				{
					G_bprint(PRINT_HIGH, "\20%s\21 false-started\n", racer->netname);
					if (race_end(racer, false, false))
					{
						return;
					}
				}
				else
				{
					G_sprint(racer, 2, "Come back here!\n");
					VectorCopy(e->s.v.origin, racer->s.v.origin);
					VectorSet(racer->s.v.velocity, 0, 0, 0);
				}
			}
		}
	}
}

void kill_race_idler(void)
{
	gedict_t *e;
	gedict_t *racer;

	for (racer = world; (racer = race_find_racer(racer)); /**/)
	{
		// FIXME: Keep track of which start, any map with multiple starts would break
		for (e = world; (e = ez_find(e, "race_cp_start"));)
		{
			vec3_t test_point;
			VectorAdd(racer->s.v.origin, racer->s.v.view_ofs, test_point);

			if ((test_point[0] >= e->s.v.absmin[0]) && (test_point[0] <= e->s.v.absmax[0])
					&& (test_point[1] >= e->s.v.absmin[1]) && (test_point[1] <= e->s.v.absmax[1])
					&& (test_point[2] >= e->s.v.absmin[2]) && (test_point[2] <= e->s.v.absmax[2]))
			{
				racer->race_afk++;

				if (race_match_mode())
				{
					G_bprint(PRINT_HIGH, "\20%s\21 was %s to start\n", racer->netname,
								redtext("too slow"));
					if (race_end(racer, false, false))
					{
						return;
					}
				}
				else if (racer->race_afk < 3)
				{
					G_bprint(PRINT_HIGH, "\20%s\21 was %s to start\n", racer->netname,
								redtext("too slow"));
					if (race_end(racer, false, false))
					{
						return;
					}
				}
				else
				{
					G_bprint(PRINT_HIGH, "%s was %s of line-up for %s\n", racer->netname,
								redtext("kicked out"), redtext("idling"));
					racer->race_ready = 0;
					if (race_end(racer, false, false))
					{
						return;
					}
				}
			}
			else
			{
				racer->race_afk = 0;
			}
		}
	}
}

void race_brighten_checkpoints(void)
{
	int i;
	gedict_t *e;
	gedict_t *racer;
	int furthest_checkpoint = 0;

	for (racer = world; (racer = race_find_race_participants(racer)); /**/)
	{
		furthest_checkpoint = max(furthest_checkpoint, racer->race_id);
	}

	for (i = 1; i < nodeMAX; i++)
	{
		char *classname = classname_for_nodeType(i);

		for (e = world; (e = ez_find(e, classname));)
		{
			e->s.v.effects = 0; // remove effects
			e->s.v.nextthink = 0; // stop thinking

			if (e->race_id == furthest_checkpoint)
			{
				e->s.v.effects = ( EF_GREEN); // set some green light for next checkpoint
			}
			else
			{
				e->s.v.effects = ( EF_RED); // turn all others red
			}
		}
	}
}

void race_dim_checkpoints(void)
{
	int i;
	gedict_t *e;

	for (i = 1; i < nodeMAX; i++)
	{
		char *classname = classname_for_nodeType(i);

		for (e = world; (e = ez_find(e, classname));)
		{
			e->s.v.effects = 0; // remove effects
			e->s.v.nextthink = 0; // stop thinking
		}
	}
}

void race_blink_think()
{
	// remove "cute" effects
	self->s.v.effects = 0;
}

void race_blink_node(gedict_t *e)
{
	// set some "cute" effects
	e->s.v.effects = e->race_effects;

	// reset it to something normal late
	e->s.v.nextthink = g_globalvars.time + 5;
	e->think = (func_t) race_blink_think;
}

//===========================================

void race_VelocityForDamage(float scale, vec3_t dir, vec3_t v)
{
	if (vlen(dir) > 0)
	{
		vec3_t ang;

		VectorCopy(dir, v);

		VectorNormalize(v);

		vectoangles(v, ang);
		trap_makevectors(ang);

//		VectorMA( v, crandom() * 0.3, g_globalvars.v_right, v );
		VectorMA(g_globalvars.v_forward, crandom() * 0.3, g_globalvars.v_right, v);
		VectorMA(v, crandom() * 0.3, g_globalvars.v_up, v);

		VectorNormalize(v);
		VectorScale(v, scale, v);
	}
	else
	{
		v[0] = 100 * crandom();
		v[1] = 100 * crandom();
		v[2] = 20 + 10 * g_random();

		VectorNormalize(v);
		VectorScale(v, scale, v);
	}

	return;
}

void race_meat_touch()
{
	// sound( self, CHAN_WEAPON, "zombie/z_miss.wav", 1, ATTN_NORM );	// bounce sound

	if ((self->s.v.velocity[0] == 0) && (self->s.v.velocity[1] == 0) && (self->s.v.velocity[2] == 0))
	{
		VectorClear(self->s.v.avelocity);
	}
}

void race_spawn_meat(gedict_t *player, char *gibname, float vel)
{
	gedict_t *newent;

	newent = spawn();

	VectorCopy(player->s.v.origin, newent->s.v.origin);

	setmodel(newent, gibname);
	setsize(newent, 0, 0, 0, 0, 0, 0);
	race_VelocityForDamage(vel, player->s.v.velocity, newent->s.v.velocity);
	newent->s.v.movetype = MOVETYPE_BOUNCE;
	newent->isMissile = true;
	newent->s.v.solid = SOLID_TRIGGER;
	newent->s.v.avelocity[0] = g_random() * 600;
	newent->s.v.avelocity[1] = g_random() * 600;
	newent->s.v.avelocity[2] = g_random() * 600;
	newent->think = (func_t) SUB_Remove;
	newent->s.v.nextthink = g_globalvars.time + 6 + g_random() * 10;

	newent->touch = (func_t) race_meat_touch;
}

//===========================================

void race_sprint_checkpoint(gedict_t *player, gedict_t *cp)
{
	if (cp->race_RouteNodeType == nodeCheckPoint)
	{
		G_sprint(player, 2, "%s \220%d\221\n", redtext(name_for_nodeType(cp->race_RouteNodeType)),
					cp->race_id);
	}
	else
	{
		G_sprint(player, 2, "%s\n", redtext(name_for_nodeType(cp->race_RouteNodeType)));
	}
}

static void race_end_point_touched(gedict_t *self, gedict_t *other)
{
	int player_num = NUM_FOR_EDICT(other) - 1;
	float speed, frac;

	// 
	if (race.currentrace[player_num].time)
	{
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
	if (race.racers_competing > 1)
	{
		char *positionString;
		if (race.last_completion_time < race_time())
		{
			race.racers_complete += race.last_completion_eq + 1;
			positionString = race_position_string(race.racers_complete);
			G_bprint(PRINT_HIGH, "\20%s\21 finished%s%s \20%.3fs\21\n", other->netname,
						strnull(positionString) ? "" : " in ", positionString,
						race_time() / 1000.0f);
			race.last_completion_eq = 0;
		}
		else
		{
			positionString = race_position_string(race.racers_complete);
			G_bprint(PRINT_HIGH, "\20%s\21 also finished%s%s \20%.3fs\21\n", other->netname,
						strnull(positionString) ? "" : " in ", positionString,
						race_time() / 1000.0f);
			++race.last_completion_eq;
		}

		if (race.racers_complete == 1)
		{
			sound(other, CHAN_ITEM, "boss2/sight.wav", 1, ATTN_NONE);
		}
		else
		{
			sound(other, CHAN_ITEM, "ambience/thunder1.wav", 1, ATTN_NONE);
		}
	}

	// Add marker for demos/clients
	{
		stuffcmd(
				other,
				"//ktx race end %f %f %f %f %d\n",
				race_time() / 1000.0f,
				race.currentrace[player_num].distance,
				race.currentrace[player_num].maxspeed,
				race.currentrace[player_num].avgcount ?
						race.currentrace[player_num].avgspeed
								/ race.currentrace[player_num].avgcount :
						0,
				race.racers_complete);

		race.currentrace[player_num].time = race_time(); // stop run timer
		race.currentrace[player_num].position = race.racers_complete;
	}

	race_end(other, true, true);
}

static void race_over(void)
{
	char demoFileName[MAX_OSPATH];
	int i, timeposition, nameposition;
	int best_time_position = 1000;
	int best_player_num = -1;
	char *pos;
	gedict_t *racer = NULL;
	qbool keep_demo = false;
	qbool blocked_record = race_pacemaker_enabled() && !cvar(RACE_PACEMAKER_LEGAL_RECORD);
	qbool debug = cvar("developer");

	strlcpy(demoFileName, cvar_string("serverdemo"), sizeof(demoFileName));
	pos = strstr(demoFileName, ".mvd");
	if (pos)
	{
		*pos = '\0';
	}

	read_topscores();
	clearall_usercmds_settings();

	if (debug)
	{
		G_bprint(PRINT_HIGH, "Race over: %d participants\n", race.racers_competing);
	}

	for (racer = world; (racer = race_find_race_participants(racer)); /**/)
	{
		int player_num = NUM_FOR_EDICT(racer) - 1;

		if (debug)
		{
			G_bprint(PRINT_HIGH, "Player: %d, %s\n", player_num, racer->netname);
		}

		if (!race.currentrace[player_num].time)
		{
			// They didn't set a time
			if (debug)
			{
				G_bprint(PRINT_HIGH, "- didn't set a time\n");
			}

			continue;
		}

		// first, let's see if run time gets into top scores and if name is already ranked
		timeposition = nameposition = -1;
		for (i = 0; i < NUM_BESTSCORES; i++)
		{
			if (race.currentrace[player_num].time < race.records[i].time)
			{
				if (timeposition == -1)
				{
					timeposition = i;
				}
			}

			if (streq(race.records[i].racername, racer->netname))
			{
				nameposition = i;
			}
		}

		if (debug)
		{
			G_bprint(PRINT_HIGH, "- time %.3f, position %d, nameposition %d\n",
						race.currentrace[player_num].time, timeposition, nameposition);
		}

		// run time is within top scores range
		if (timeposition != -1)
		{
			// if player didn't beat his own record
			if ((nameposition < timeposition) && (nameposition != -1))
			{
				if (race.racers_competing == 1)
				{
					sound(racer, CHAN_ITEM, "ambience/thunder1.wav", 1, ATTN_NONE);
					G_bprint(PRINT_HIGH, "Run %s in %s%s\n%s couldn't beat %s best time\n",
								redtext("finished"),
								dig3s("%.3f", race.currentrace[player_num].time / 1000),
								redtext("s"), racer->netname, g_his(racer));
				}
			}
			else
			{
				if (!blocked_record)
				{
					// if player beat his own record or is not yet ranked
					if (nameposition == -1)
					{
						nameposition = (NUM_BESTSCORES - 1);
					}

					if (is_valid_record(&race.records[nameposition]))
					{
						// let's remove the old demo
						if (!strnull(race.records[nameposition].demoname))
						{
							int demo_references = 0;

							// one demo might now contain records for more than one player, so
							//   only remove if this record is the last reference
							for (i = 0; i < sizeof(race.records) / sizeof(race.records[0]); ++i)
							{
								if (streq(race.records[nameposition].demoname,
											race.records[i].demoname))
								{
									++demo_references;
								}
							}

							if (debug)
							{
								G_bprint(PRINT_HIGH, "%s has %d references\n",
											race.records[nameposition].demoname, demo_references);
							}

							if (demo_references == 1)
							{
								localcmd("sv_demoremove %s\n", race.records[nameposition].demoname);
							}
						}
					}

					// move old top scores down
					for (i = nameposition; i > timeposition; i--)
					{
						race.records[i] = race.records[i - 1];
					}

					// add new top score
					race.records[i].time = race.currentrace[player_num].time;
					strlcpy(race.records[i].racername, racer->netname,
							sizeof(race.records[i].racername));
					if (race.race_recording)
					{
						strlcpy(race.records[i].demoname, demoFileName,
								sizeof(race.records[i].demoname));
					}
					else
					{
						memset(race.records[i].demoname, 0, sizeof(race.records[i].demoname));
					}

					race.records[i].distance = race.currentrace[player_num].distance;
					race.records[i].maxspeed = race.currentrace[player_num].maxspeed;
					race.records[i].avgspeed = race.currentrace[player_num].avgspeed
							/ race.currentrace[player_num].avgcount;
					race.records[i].weaponmode = race.weapon;
					race.records[i].startmode = race.falsestart;
					if (!QVMstrftime(race.records[i].date, sizeof(race.records[i].date),
										"%Y-%m-%d %H:%M:%S", 0))
					{
						race.records[i].date[0] = 0; // bad date
					}

					// save scores in file
					write_topscores();

					if (race.racers_competing == 1)
					{
						if (!i)
						{
							// first place! we go the extra mile
							sound(racer, CHAN_ITEM, "boss2/sight.wav", 1, ATTN_NONE);
						}
						else
						{
							sound(racer, CHAN_ITEM, "ambience/thunder1.wav", 1, ATTN_NONE);
						}
					}

					if (!i)
					{
						race.top_time = race.currentrace[player_num].time;
						strlcpy(race.top_nick, racer->netname, sizeof(race.top_nick));
					}
				}

				if (race.racers_competing == 1)
				{
					G_bprint(2, "Run %s in %s%s\n", redtext("finished"),
								dig3s("%.3f", race.currentrace[player_num].time / 1000),
								redtext("s"));
				}

				G_bprint(PRINT_HIGH, "%s %s %s record\n", racer->netname,
							blocked_record ? "would have taken" : "took the",
							race_position_string(timeposition + 1));

				if (timeposition <= best_time_position)
				{
					best_player_num = player_num;
					best_time_position = timeposition;
				}

				keep_demo |= !blocked_record;
			}
		}
		else
		{
			// run time did not make it to top scores
			if (race.racers_competing == 1)
			{
				sound(racer, CHAN_ITEM, "boss2/idle.wav", 1, ATTN_NONE);
				G_bprint(PRINT_HIGH, "Run %s in %s%s\n", redtext("finished"),
							dig3s("%.3f", race.currentrace[player_num].time / 1000), redtext("s"));
			}
		}
	}

	if (race_match_started())
	{
		race_match_round_end();
	}
	else
	{
		// Continue match with next run
		race_finish_capture(best_time_position == 0 && !blocked_record, best_player_num);
		race_start(!keep_demo, "%s", "");
	}
}

void race_node_touch()
{
	if (other->ct != ctPlayer)
	{
		return;
	}

	// no run in progress nor starting
	if (!race.status)
	{
		if (self->attack_finished >= g_globalvars.time)
		{
			return; // still in node touch cooldown
		}

		self->attack_finished = g_globalvars.time + 5; // touch cooldown of 5s
		race_blink_node(self);
		sound(other, CHAN_ITEM, self->noise, self->race_volume, ATTN_NONE);
		race_sprint_checkpoint(other, self); // display checkpoint type or order
	}

	// run in progress or starting
	if (race.status != raceActive)
	{
		return; // starting run countdown
	}

	if (!other->racer)
	{
		return; // can't touch if not the racer
	}

	if (self->race_id < other->race_id)
	{
		return; // node already touched during run
	}

	if (self->race_id == other->race_id)
	{
		// racer touched checkpoint in right order
		if (self->race_RouteNodeType == nodeEnd)
		{
			race_end_point_touched(self, other);
		}
		else if (self->race_RouteNodeType == nodeCheckPoint)
		{
			int player_num = NUM_FOR_EDICT(other) - 1;

			stuffcmd(
					other,
					"//ktx race cp %d %f %f %f %f\n",
					other->race_id,
					race_time() / 1000.0f,
					race.currentrace[player_num].distance,
					race.currentrace[player_num].maxspeed,
					race.currentrace[player_num].avgcount ?
							race.currentrace[player_num].avgspeed
									/ race.currentrace[player_num].avgcount :
							0);
		}

		// if its not start checkpoint do something "cute"
		if (self->race_id)
		{
			// do some sound
			sound(other, CHAN_ITEM, "knight/sword2.wav", 1, ATTN_NONE);
		}

		other->race_id++; // bump our id, so we can touch next checkpoint

		race_brighten_checkpoints(); // set light on next checkpoint

		return;
	}

	if (self->race_id > other->race_id)
	{
		// racer touched checkpoint in WRONG order

		// do some sound
		sound(other, CHAN_ITEM, "boss2/idle.wav", 1, ATTN_NONE);

		if (self->race_RouteNodeType == nodeCheckPoint)
		{
			G_bprint(PRINT_HIGH, "\20%s\21: %s \220%d\221 touched in wrong order\n", other->netname,
						redtext(name_for_nodeType(self->race_RouteNodeType)), self->race_id);
		}
		else
		{
			G_bprint(PRINT_HIGH, "\20%s\21: \220%s\221 touched in wrong order\n", other->netname,
						redtext(name_for_nodeType(self->race_RouteNodeType)));
		}

		race_end(other, true, false);

		return;
	}
}

//===========================================

// this will fix id for end checkpoint
void race_fix_end_checkpoint(void)
{
	// get end checkpoint
	gedict_t *e = ez_find(world, classname_for_nodeType(nodeEnd));

	if (e)
	{
		e->race_id = 1 + find_cnt( FOFCLSN, classname_for_nodeType(nodeCheckPoint)); // pretty simple logic
	}
}

// spawn/putInGame race route node
gedict_t* spawn_race_node(raceRouteNode_t *node)
{
	gedict_t *e;
	char *classname = classname_for_nodeType(node->type);

	if (checkpoints_count() >= MAX_ROUTE_NODES)
	{
		G_Error("spawn_race_node: can't add more, unexpected");
	}

	// free previos points if any, except intermediate checkpoint
	if (node->type != nodeCheckPoint)
	{
		ent_remove_by_classname(classname);
	}

	e = spawn();

	switch (node->type)
	{
		case nodeCheckPoint:
			e->race_id = 1 + find_cnt( FOFCLSN, classname); // pretty simple logic
			break;

		case nodeStart:
		case nodeEnd:
			break;

		default:
			G_Error("spawn_race_node: wrong node->type %d", node->type);
	}

	setmodel(e, model_for_nodeType(node->type));
	if (VectorCompareF(node->sizes, 0, 0, 0))
	{
		setsize(e, PASSVEC3(VEC_HULL_MIN), PASSVEC3(VEC_HULL_MAX));
	}
	else
	{
		setsize(e, -node->sizes[0] / 2, -node->sizes[1] / 2, -node->sizes[2] / 2,
				node->sizes[0] / 2, node->sizes[1] / 2, node->sizes[2] / 2);
	}

	e->s.v.solid = SOLID_TRIGGER;
	e->s.v.movetype = MOVETYPE_NONE;
	e->s.v.flags = FL_ITEM;
	e->classname = classname;
	e->noise = touch_sound_for_nodeType(node->type);
	e->race_volume = volume_for_touch_sound_for_nodeType(node->type);
	e->race_effects = blink_effects_for_nodeType(node->type);
	e->touch = (func_t) race_node_touch;
	e->attack_finished = g_globalvars.time + 1; // + 1 so it not touched immidiatelly, look race_node_touch() for more info
	e->race_RouteNodeType = (int)node->type; // ah, cast

	// play spawn sound
	sound(e, CHAN_AUTO, spawn_sound_for_nodeType(node->type), 1, ATTN_NONE);

	VectorCopy(node->ang, e->s.v.v_angle);

	setorigin(e, PASSVEC3(node->org));

	// this will fix id for end checkpoint
	race_fix_end_checkpoint();

	return e;
}

//============================================

// set some race related fields for all players
void race_clear_race_fields(void)
{
	gedict_t *p;

	for (p = world; (p = find_plr(p));)
	{
		p->race_participant = p->racer = false;
		p->hideplayers = p->hideplayers_default;
		p->race_id = 0;
	}

	for (p = world; (p = find_spc(p));)
	{
		p->race_participant = p->racer = false;
		p->hideplayers = p->hideplayers_default;
		p->race_id = 0;
	}
}

static void race_cancel(qbool cancelrecord, const char *fmt, ...)
{
	va_list argptr;
	char text[1024];

	if (!race_match_started())
	{
		va_start(argptr, fmt);
		Q_vsnprintf(text, sizeof(text), fmt, argptr);
		va_end(argptr);

		race_stoprecord(cancelrecord);

		G_cp2all("%s", ""); // clear centerprint

		if (!strnull(text))
		{
			G_bprint(2, "%s", text);
		}
	}

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
static gedict_t* _race_line(int offset)
{
	int i, idx;
	int c = max(0, race.next_racer + offset);
	gedict_t *p = g_edicts + 1; // p - start of players

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		idx = (c + i) % MAX_CLIENTS;

		if (p[idx].ct == ctPlayer && p[idx].race_ready)
		{
			race.next_racer = idx;

			return &(p[idx]);
		}
	}

	return NULL;
}

// get someone from race queue line
gedict_t* race_get_from_line(void)
{
	return _race_line(0);
}

// this dude will be next in line, in most cases
gedict_t* race_set_next_in_line(void)
{
	return _race_line(1);
}

void init_scores(void)
{
	int i;

	for (i = 0; i < NUM_BESTSCORES; i++)
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

void display_scores(void)
{
	int i;

	if (!race_command_checks())
	{
		return;
	}

	G_sprint(
			self,
			2,
			"\n\235\236\236\236\236\236\236\236\236\236\236\236\236\236\237%s %02d\235\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("top"), NUM_BESTSCORES);
	G_sprint(self, 2, "pos.  time      name\n");

	for (i = 0; i < NUM_BESTSCORES; i++)
	{
		if (!is_valid_record(&race.records[i]))
		{
			G_sprint(self, 2, " %02d      -         -\n", i + 1);
		}
		else
		{
			if (streq(race.records[i].racername, self->netname))
			{
				G_sprint(self, 2, " %02d \215 %07.3f%s  %s\n", i + 1, race.records[i].time / 1000.0,
							redtext("s"), race.records[i].racername);
			}
			else
			{
				G_sprint(self, 2, " %02d   %07.3f%s  %s\n", i + 1, race.records[i].time / 1000.0,
							redtext("s"), race.records[i].racername);
			}
		}
	}
}

void race_display_line(void)
{
	int i = 0;
	gedict_t *p;

	if (!race_command_checks())
	{
		return;
	}

	G_sprint(self, 2, "=== %s ===\n", redtext("Line-up"));

	for (p = world; (p = find_plr(p));)
	{
		if (p->race_ready)
		{
			i++;
			if (p->racer)
			{
				G_sprint(self, 2, "%2d \215 %s\n", i, p->netname);
			}
			else
			{
				G_sprint(self, 2, "%2d   %s\n", i, p->netname);
			}
		}
	}

	if (!i)
	{
		G_sprint(self, 2, "    (Empty)    \n");
	}
}

//============================================

qbool race_can_go(qbool cancel)
{
	gedict_t *racer;

	// can't go on, noone ready
	if (!race_count_ready_players())
	{
		if (cancel)
		{
			race_cancel(true, "Race in standby, no players in line\n");
			race_dim_checkpoints();
		}

		return false;
	}

	// can't go on, no start checkpoint
	if (!ez_find(world, classname_for_nodeType(nodeStart)))
	{
		if (cancel)
		{
			race_cancel(true, "Race in standby, no %s\n", name_for_nodeType(nodeStart));
		}

		return false;
	}

	// can't go on, no end checkpoint
	if (!ez_find(world, classname_for_nodeType(nodeEnd)))
	{
		if (cancel)
		{
			race_cancel(true, "Race in standby, no %s\n", name_for_nodeType(nodeEnd));
		}

		return false;
	}

	if (race.status)
	{
		// Multi-person racing: this is fine, means we can't find even one racer
		if (!race_get_racer())
		{
			if (cancel)
			{
				race_start(true, "Race aborted, racer vanished\n");
			}

			return false;
		}
	}

	if (race.status == raceActive)
	{
		int timeouts = 0;
		int deaths = 0;
		gedict_t *timeout_plr = NULL;
		qbool race_ended = false;

		// Multi-person racing: end individual racer's races if they die
		for (racer = world; (racer = race_find_racer(racer)); /**/)
		{
			if (racer->s.v.health <= 0 && cancel)
			{
				// do some sound
				race_ended |= race_end(racer, true, false);
				G_bprint(PRINT_HIGH, "\20%s\21 died\n", racer->netname);
				k_respawn(racer, false);

				if (race_ended)
				{
					break;
				}
			}

			// Timeout everyone still racing
			if (race.timeout < g_globalvars.time)
			{
				if (cancel)
				{
					race_ended |= race_end(racer, true, false);
					timeout_plr = racer;
					++timeouts;

					if (race_ended)
					{
						break;
					}
				}
			}
		}

		// Reduce number of announcements
		if (timeouts || deaths)
		{
			// do some sound
			sound(world, CHAN_ITEM, "boss2/idle.wav", 1, ATTN_NONE);
			if (timeouts == 1 && timeout_plr)
			{
				G_bprint(PRINT_HIGH, "\20%s\21 couldn't finish in time\n", timeout_plr->netname);
			}
			else if (timeouts > 1)
			{
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
static void race_start(qbool cancelrecord, const char *fmt, ...)
{
	va_list argptr;
	char text[1024];
	extern void ktpro_autotrack_on_race_status_changed(void);

	gedict_t *r, *n, *s;

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	// cancel it first, this will clear something what probably wasn't cleared before
	race_cancel(cancelrecord, "%s", text);

	// switch status to coutdown
	race.status = raceCD;

	// set countdown timer
	race.cd_cnt = 3;

	race.cd_next_think = g_globalvars.time;

	if (!race.timeout_setting)
	{
		race.timeout_setting = RACE_DEFAULT_TIMEOUT;
	}

	s = ez_find(world, classname_for_nodeType(nodeStart));
	if (!s)
	{
		race_shutdown("race_start: can't find start checkpoint, shutdown race\n");

		return;
	}

	if (race_simultaneous())
	{
		// Everyone in queue gets to race
		race.racers_competing = 0;
		for (r = world; (r = find_plr(r)); /**/)
		{
			if ((r->ct == ctPlayer) && r->race_ready)
			{
				race_make_active_racer(r, s);
				++race.racers_competing;
			}
		}
	}
	else
	{
		race.racers_competing = 0;

		r = race_get_from_line();
		if (r)
		{
			race_make_active_racer(r, s);

			n = race_set_next_in_line();
			if (n && (n != r))
			{
				G_sprint(n, 2, "You are %s in line!\n", redtext("next"));
			}

			race.racers_competing = 1;
		}
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

static void race_make_active_racer(gedict_t *r, gedict_t *s)
{
	int player_num = NUM_FOR_EDICT(r) - 1;

	// mark him as racer
	r->racer = r->race_participant = true;
	r->hideplayers = r->hideplayers_default;
	ktpro_autotrack_on_powerup_take(r);
	//G_bprint( 2, "%s is starting his race!\n", r->netname );

	// make sure not hooked onto something at previous spot
	if (r->hook_out)
	{
		GrappleReset(r->hook);
	}

	r->hook_out = false;
	r->on_hook = false;

	// clear velocity
	SetVector(r->s.v.velocity, 0, 0, 0);

	// set proper angles
	VectorCopy(s->s.v.v_angle, r->s.v.angles);
	VectorCopy(s->s.v.v_angle, r->s.v.v_angle);
	r->s.v.fixangle = true;

	// set proper origin
	setorigin(r, PASSVEC3(s->s.v.origin));

	// telefrag anyone at this origin
	teleport_player(r, r->s.v.origin, r->s.v.angles, TFLAGS_SND_DST);
	memset(&race.currentrace[player_num], 0, sizeof(race.currentrace[player_num]));
}

//============================================
//
// RACE "THINK"
//
//============================================

// well, this set some fields on players which help to not block racer
void race_set_one_player_movetype_and_etc(gedict_t *p)
{
	if (match_over)
	{
		return;
	}

	if (race.status && p->race_chasecam && !p->racer)
	{
		setwepnone(p);
	}
	else
	{
		setwepall(p);
	}

	switch (race.status)
	{
		case raceNone:
			p->s.v.movetype = MOVETYPE_WALK;
			if (p->s.v.solid != SOLID_BBOX)
			{
				p->s.v.solid = SOLID_BBOX;
				setorigin(p, PASSVEC3(p->s.v.origin));
			}

			p->muted = false;
			setmodel(p, "progs/player.mdl");
			set_usercmd_trace(p, false);
			break;

		case raceCD:
			if (race.falsestart == raceFalseStartNo)
			{
				p->s.v.movetype = (p->racer ? MOVETYPE_NONE : MOVETYPE_WALK);
			}
			else
			{
				p->s.v.movetype = MOVETYPE_WALK;
			}

			if (p->s.v.solid != SOLID_NOT)
			{
				p->s.v.solid = SOLID_NOT;
				setorigin(p, PASSVEC3(p->s.v.origin));
			}

			p->muted = (p->racer ? false : true);
			setmodel(p, (p->racer ? "progs/player.mdl" : ""));
			set_usercmd_trace(p, false);
			break;

		case raceActive:
			p->s.v.movetype = MOVETYPE_WALK;
			if (p->s.v.solid != SOLID_NOT)
			{
				p->s.v.solid = SOLID_NOT;
				setorigin(p, PASSVEC3(p->s.v.origin));
			}

			p->muted = (p->racer ? false : true);
			setmodel(p, (p->racer ? "progs/player.mdl" : ""));
			set_usercmd_trace(p, p->racer);
			break;

		default:
			G_Error("race_set_one_player_movetype_and_etc: unknown race.status %d", race.status);
	}
}

void unmute_all_players(void)
{
	gedict_t *p;

	for (p = world; (p = find_plr(p));)
	{
		p->muted = false;
	}
}

void race_set_players_movetype_and_etc(void)
{
	gedict_t *p;

	for (p = world; (p = find_plr(p));)
	{
		race_set_one_player_movetype_and_etc(p);
	}
}

void race_chasecam_freelook_change(void)
{
	if (!race_command_checks())
	{
		return;
	}

	self->race_chasecam_freelook = !self->race_chasecam_freelook;

	switch (self->race_chasecam_freelook)
	{
		case 0:
			G_sprint(self, 2, "Chasecam freelook %s\n", redtext("disabled"));
			return;

		case 1:
			G_sprint(self, 2, "Chasecam freelook %s\n", redtext("enabled"));
			return;

		default:
			return;
	}
}

void race_chasecam_change(void)
{
	if (!race_command_checks())
	{
		return;
	}

	if (self->racer)
	{
		return;
	}

	self->race_chasecam_view++;
	if (self->race_chasecam_view == NUM_CHASECAMS)
	{
		self->race_chasecam_view = 0;
	}

	switch (self->race_chasecam_view)
	{
		case 0:
			G_sprint(self, 2, "Chasecam is in %s view mode\n", redtext("1st person"));
			break;

		case 1:
			G_sprint(self, 2, "Chasecam is in %s view mode\n", redtext("3rd person"));
			break;

		case 2:
			G_sprint(self, 2, "Chasecam is in %s view mode\n", redtext("hawk eye"));
			break;

		case 3:
			G_sprint(self, 2, "Chasecam is in %s view mode\n", redtext("backpack ride"));
			break;

		default:
			G_sprint(self, 2, "Chasecam position has not beem defined, keep cycling\n");
	}
}

static void race_advance_chasecam_for_plr(gedict_t *plr)
{
	gedict_t *first_racer = race_get_racer();
	gedict_t *racer = first_racer;

	if (!first_racer)
	{
		return;
	}

	if (plr->race_track && (g_edicts[plr->race_track].ct != ctPlayer))
	{
		plr->race_track = NUM_FOR_EDICT(racer);
	}

	if (!plr->race_track)
	{
		plr->race_track = NUM_FOR_EDICT(racer);
	}
	else
	{
		racer = race_find_racer(&g_edicts[plr->race_track]);
		if (!racer)
		{
			racer = first_racer;
		}

		plr->race_track = NUM_FOR_EDICT(racer);
	}
}

static gedict_t* race_find_chasecam_for_plr(gedict_t *plr, gedict_t *racer)
{
	if (plr->race_track)
	{
		int player_num = plr->race_track;

		if ((player_num >= 1) && (player_num <= MAX_CLIENTS))
		{
			gedict_t *tracked = &g_edicts[player_num];
			while (tracked && (tracked->ct == ctPlayer) && !tracked->racer)
			{
				tracked = race_find_racer(tracked);
			}

			if (tracked)
			{
				racer = tracked;
			}
		}
	}

	plr->race_track = NUM_FOR_EDICT(racer);

	return racer;
}

void race_follow(void)
{
	gedict_t *racer = race_get_racer();
	vec3_t delta;
	float vlen;
	int follow_distance;
	int upward_distance;

	if (!racer)
	{
		return; // no racer found
	}

	if (!self->racer && self->race_chasecam)
	{
		racer = race_find_chasecam_for_plr(self, racer);

		switch (self->race_chasecam_view)
		{
			case 1: // 3rd person
				follow_distance = -120;
				upward_distance = 50;
				self->hideentity = 0;
				VectorCopy(racer->s.v.v_angle, self->s.v.angles);
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
				self->hideentity = EDICT_TO_PROG(racer);
				self->s.v.angles[0] = -racer->s.v.angles[0];
				self->s.v.angles[1] = 180;
				break;

			case 0: // 1st person - ok
				follow_distance = -10;
				upward_distance = 0;
				self->hideentity = EDICT_TO_PROG(racer); // in this mode we want to hide racer model for watcher's view
				VectorCopy(racer->s.v.v_angle, self->s.v.angles);
				break;

			default:
				return;
		}

		if (!self->race_chasecam_freelook)
		{
			self->s.v.fixangle = true; // force client v_angle
		}

		trap_makevectors(racer->s.v.angles);
		VectorMA(racer->s.v.origin, follow_distance, g_globalvars.v_forward, self->s.v.origin);
		VectorMA(self->s.v.origin, upward_distance, g_globalvars.v_up, self->s.v.origin);

		// avoid positionning in walls
		traceline(PASSVEC3(racer->s.v.origin), PASSVEC3(self->s.v.origin), false, racer);
		VectorCopy(g_globalvars.trace_endpos, self->s.v.origin);

		if (g_globalvars.trace_fraction == 1)
		{
			VectorCopy(g_globalvars.trace_endpos, self->s.v.origin);
			VectorMA(self->s.v.origin, 10, g_globalvars.v_forward, self->s.v.origin);
		}
		else
		{
			VectorSubtract(g_globalvars.trace_endpos, racer->s.v.origin, delta);
			vlen = VectorLength(delta);
			vlen = vlen - 40;
			VectorNormalize(delta);
			VectorScale(delta, vlen, delta);
			VectorAdd(racer->s.v.origin, delta, self->s.v.origin);
		}

		//if ( trap_pointcontents( PASSVEC3( self->s.v.origin ) ) == CONTENT_SKY )
		//	G_bprint( 2, "SKY!\n" );

		// smooth playing for ezq / zq
		self->s.v.movetype = MOVETYPE_LOCK;
	}

	if (!self->racer && !self->race_chasecam)
	{
		// restore movement and show racer entity
		self->s.v.movetype = MOVETYPE_WALK;
		self->hideentity = 0;
	}
	else if (self->racer)
	{
		self->hideentity = 0;
	}
}

void race_think(void)
{
	gedict_t *racer;
	gedict_t *p, *n = NULL;

	race_update_pacemaker();

	if (match_over)
	{
		race.status = raceNone;

		return;
	}

	switch (race.status)
	{
		case raceNone:
			// can't start
			if (!race_can_go(!race.warned))
			{
				return;
			}

			// advance race status to countdown
			race_start(true, "%s", "");

			return;

		case raceCD:
			// something wrong
			if (!race_can_go(true))
			{
				return;
			}

			if (!race.race_recording)
			{
				race_record();
			}

			// countdown in progress
			if (race.cd_next_think >= g_globalvars.time)
			{
				return;
			}

			racer = race_get_racer();
			// must not never happens because we have race_can_go() above
			if (!racer)
			{
				race_start(true, "Run aborted, racer vanished\n");

				return;
			}

			// countdown still in progress
			if (race.cd_cnt > 0)
			{
				if (race.cd_next_think < g_globalvars.time)
				{
					char cp_buf[1024] =
						{ 0 }, tmp[512] =
						{ 0 };

					gedict_t *racer = race_get_racer();

					// ok, time for next "tick" in coutdown
					if (race_find_racer(racer))
					{
						snprintf(cp_buf, sizeof(cp_buf), "Racing in: %s\n", dig3(race.cd_cnt));
					}
					else
					{
						snprintf(cp_buf, sizeof(cp_buf), "%s racing in: %s\n", racer->netname,
									dig3(race.cd_cnt));
					}

					snprintf(tmp, sizeof(tmp), "weapon: %s\n\n",
								redtext(race_weapon_mode(race.weapon)));
					strlcat(cp_buf, tmp, sizeof(cp_buf));

					if (race_match_mode())
					{
						strlcat(cp_buf, "round: ", sizeof(cp_buf));

						if (race.round_number >= race.rounds)
						{
							strlcat(cp_buf, redtext("final\n"), sizeof(cp_buf));
						}
						else
						{
							snprintf(tmp, sizeof(tmp), "%d/%d\n", race.round_number + 1,
										race.rounds);
							strlcat(cp_buf, tmp, sizeof(cp_buf));
						}
					}
					else
					{
						if (!strnull(race.top_nick))
						{
							snprintf(tmp, sizeof(tmp), "best run: %s%s (by %s)",
										dig3s("%.3f", race.top_time / 1000.0), redtext("s"),
										race.top_nick);
							strlcat(cp_buf, tmp, sizeof(cp_buf));
						}

						if (!strnull(race.pacemaker_nick))
						{
							snprintf(tmp, sizeof(tmp), "\npacemaker: %s%s (by %s)",
										dig3s("%.3f", race.pacemaker_time / 1000.0), redtext("s"),
										race.pacemaker_nick);
							strlcat(cp_buf, tmp, sizeof(cp_buf));
						}
					}

					G_cp2all("%s", cp_buf);

					// check for falsestarts
					race_check_racer_falsestart(false);

					// FIXME: yeah, nice make some utility for that
					for (p = world; (p = find_client(p));)
					{
						stuffcmd(p, "play buttons/switch04.wav\n");
					}

					race.cd_next_think = g_globalvars.time + 1; // set next think one second later
					race.cd_cnt--;
				}

				return;
			}

			G_cp2all("GO!");

			// FIXME: yeah, nice make some utility for that
			for (p = world; (p = find_client(p));)
			{
				//stuffcmd (p, "play enforcer/enfire.wav\n");
				stuffcmd(p, "play weapons/pkup.wav\n");  // I like this one better -- deurk.
			}

			race.status = raceActive; // countdown ends, now we ready for race
			memset(race.currentrace, 0, sizeof(race.currentrace)); // initiate distance
			race_init_capture();

			// check for falsestarts
			race_check_racer_falsestart(true);

			race.start_time = g_globalvars.time;
			race.timeout = g_globalvars.time + max(1, race.timeout_setting);
			race.next_race_time = 500; // do not print race time for first 500 ms, so we can see "Go!"

			// set proper move type for players
			race_set_players_movetype_and_etc();
			{
				char date[64];

				if (!QVMstrftime(date, sizeof(date), "%Y%m%d%H%M%S", 0))
					date[0] = '\0';

				for (racer = world; (racer = race_find_racer(racer)); /**/)
				{
					stuffcmd(racer, "//ktx race start %d %d %s\n", race.weapon, race.falsestart,
								date);
				}
			}

			return;

		case raceActive:

			// anti-idling
			if ((race_time() > (race.start_time + 3000)) && (race_time() < (race.start_time + 4000)))
			{
				kill_race_idler();
			}

			// something wrong
			if (!race_can_go(true))
			{
				return;
			}

			race_save_position();

			if (race_time() >= race.next_race_time)
			{
				vec3_t tmp;

				race.next_race_time = race_time() + 100; // update race time each 100 ms

				for (racer = world; (racer = race_find_racer(racer)); /**/)
				{
					int player_num = NUM_FOR_EDICT(racer) - 1;
					raceRecord_t *raceStats = &race.currentrace[player_num];
					float current_velocity = vlen(racer->s.v.velocity);

					VectorSubtract(racer->s.v.origin, racer->s.v.oldorigin, tmp);
					raceStats->distance += vlen(tmp);

					raceStats->maxspeed = max(raceStats->maxspeed, current_velocity);
					raceStats->avgspeed += current_velocity;
					raceStats->avgcount++;
				}

				race_update_closest_positions();

				if (race.racers_competing == 1)
				{
					racer = race_get_racer();
					n = race_get_from_line();
					for (p = world; (p = find_client(p));)
					{
						if (p->racer)
						{
							G_centerprint(p, "%s", dig3s("time: %.1f", race_time() / 1000.0));
						}
						else
						{
							G_centerprint(p, "following %s\n%s\nspeed: %4.1f\ntime: %s",
											racer->netname,
											(n == p) ? redtext("== you're next in line-up ==") : "",
											race_vlen(racer->s.v.velocity),
											dig3s("%3.1f", race_time() / 1000.0));
						}
					}
				}
				else
				{
					for (p = world; (p = find_client(p)); /**/)
					{
						if (p->racer)
						{
							G_centerprint(p, "%s", dig3s("time: %.1f", race_time() / 1000.0));
						}
						else
						{
							int player_num = NUM_FOR_EDICT(p) - 1;
							qbool time_set = p->ct == ctPlayer && p->race_participant
									&& race.currentrace[player_num].time;
							char *tracking_text = "";
							char *tracking_speed = "";

							if (p->race_chasecam && (p->race_track >= 1)
									&& (p->race_track <= MAX_CLIENTS))
							{
								gedict_t *chasing = &g_edicts[p->race_track];

								if ((chasing->ct == ctPlayer) && chasing->racer)
								{
									tracking_text = va("following %s\n", chasing->netname);
									tracking_speed = va("speed: %4.1f\n",
														race_vlen(chasing->s.v.velocity));
								}
							}

							if (time_set)
							{
								G_centerprint(p, "%s%s %.3f %s\n%stime: %s", tracking_text,
												redtext("== Race over: "),
												race.currentrace[player_num].time / 1000.0,
												redtext("=="), tracking_speed,
												dig3s("%3.1f", race_time() / 1000.0));
							}
							else if ((p->ct == ctPlayer) && p->race_participant)
							{
								G_centerprint(p, "%s%s\n%stime: %s", tracking_text,
												redtext("== Race over: please wait =="),
												tracking_speed,
												dig3s("%3.1f", race_time() / 1000.0));
							}
							else
							{
								G_centerprint(p, "%s%s\n%stime: %s", tracking_text,
												redtext("== Race in progress =="), tracking_speed,
												dig3s("%3.1f", race_time() / 1000.0));
							}
						}
					}
				}
			}

			return;

		default:

			G_Error("race_think: unknown race.status %d", race.status);
	}
}

//============================================
//
// race commands
//
//============================================

static void race_route_now_custom(void)
{
	init_scores();
	race.active_route = 0; // mark this is a custom route now
	race_clear_pacemaker();

	cvar_fset(RACE_ROUTE_NUMBER_CVAR, -1);
	cvar_set(RACE_ROUTE_MAPNAME_CVAR, "");
}

void r_Xset(float t)
{
	gedict_t *e;
	raceRouteNode_t node;

	if (!race_command_checks())
	{
		return;
	}

	if (race_is_started())
	{
		return;
	}

	if (checkpoints_count() >= MAX_ROUTE_NODES)
	{
		G_sprint(self, 2, "Can't add more checkpoints!\n");

		return;
	}

	// zeroing all fields
	memset(&node, 0, sizeof(node));

	node.type = (raceRouteNodeType_t) t; // magic cast
	VectorCopy(self->s.v.v_angle, node.ang);
	VectorCopy(self->s.v.origin, node.org);

	e = spawn_race_node(&node);

	if (node.type == nodeCheckPoint)
	{
		G_bprint(2, "%s \220%d\221 set\n", redtext(name_for_nodeType(node.type)), e->race_id);
		G_bprint(2, "Coordinates: %6.1f %6.1f %6.1f\n", e->s.v.origin[0], e->s.v.origin[1],
					e->s.v.origin[2]);
	}
	else if (node.type == nodeStart)
	{
		G_bprint(2, "%s set\n", redtext(name_for_nodeType(node.type)));
		G_bprint(2, "Coordinates: %6.1f %6.1f %6.1f\n", e->s.v.origin[0], e->s.v.origin[1],
					e->s.v.origin[2]);
		G_bprint(2, "Direction: %6.1f %6.1f\n", e->s.v.v_angle[0], e->s.v.v_angle[1]);
	}
	else
	{
		G_bprint(2, "%s set\n", redtext(name_for_nodeType(node.type)));
		G_bprint(2, "Coordinates: %6.1f %6.1f %6.1f\n", e->s.v.origin[0], e->s.v.origin[1],
					e->s.v.origin[2]);
	}

	race_route_now_custom();  // mark this is a custom route now
}

void r_cdel()
{
	gedict_t *e;
	int cnt, id;
	char *classname = classname_for_nodeType(nodeCheckPoint);

	if (!race_command_checks())
	{
		return;
	}

	if (race_is_started())
	{
		return;
	}

	cnt = find_cnt( FOFCLSN, classname);

	if (!cnt)
	{
		G_sprint(self, 2, "Can't find any %s\n", redtext(name_for_nodeType(nodeCheckPoint)));

		return;
	}

	// get highest id
	id = 0;
	for (e = world; (e = ez_find(e, classname));)
	{
		id = max(id, e->race_id);
	}

	// and now remove it
	for (e = world; (e = ez_find(e, classname));)
	{
		if (id == e->race_id)
		{
			ent_remove(e);
			break;
		}
	}

	// this will fix id for end checkpoint
	race_fix_end_checkpoint();

	G_bprint(2, "%s \220%d\221 removed\n", redtext(name_for_nodeType(nodeCheckPoint)), id);

	race_route_now_custom();  // mark this is a custom route now
}

void set_player_race_follow(gedict_t *e, int follow)
{
	if (follow)
	{
		if (e->race_chasecam)
		{
			return;
		}

		G_sprint(self, 2, "Your %s is now %s\n", redtext("chasecam"), redtext("enabled"));

		e->race_chasecam = 1;
		if (!e->racer)
		{
			setwepnone(e);
		}
	}
	else
	{
		if (!e->race_chasecam)
		{
			return;
		}

		G_sprint(self, 2, "Your %s is now %s\n", redtext("chasecam"), redtext("disabled"));

		e->race_chasecam = 0;
		setwepall(e);
		SetVector(e->s.v.velocity, 0, 0, 0);
	}
}

void set_player_race_ready(gedict_t *e, int ready)
{
	if (ready)
	{
		if (e->race_ready)
		{
			return;
		}

		G_bprint(2, "%s %s the line-up\n", e->netname, redtext("joined"));
		e->race_ready = 1;
		e->race_afk = 0;

		race.warned = false; // so we get warning why race can't be started
	}
	else
	{
		if (!e->race_ready)
		{
			return;
		}

		G_bprint(2, "%s %s the line-up\n", e->netname, redtext("left"));
		e->race_ready = 0;
	}
}

qbool race_command_checks(void)
{
	if (!isRACE())
	{
		G_sprint(self, 2, "Command only available in %s mode (type /%s to activate it)\n",
					redtext("race"), redtext("race"));

		return false;
	}

	return true;
}

qbool race_is_started(void)
{
	if (race.status)
	{
		G_sprint(
				self,
				2,
				"Can't use that command while %s is in progress, wait for all players to leave the line-up\n",
				redtext("race"));

		return true;
	}

	return false;
}

void r_changefollowstatus(float t)
{
	if (!race_command_checks())
	{
		return;
	}

	if (self->racer)
	{
		return;
	}

	switch ((int)t)
	{
		case 1: // rfollow
			set_player_race_follow(self, 1);
			return;

		case 2: // rnofollow
			set_player_race_follow(self, 0);
			return;

		case 3: // rftoggle
			set_player_race_follow(self, !self->race_chasecam);
			return;

		default:
			return;
	}
}

void r_changestatus(float t)
{
	qbool match_enabled = race_match_mode();

	if (!race_command_checks())
	{
		return;
	}

	if (self->ct == ctSpec)
	{
		return;
	}

	switch ((int)t)
	{
		case 1: // race_ready
			if (match_enabled && race.status)
			{
				G_sprint(self, PRINT_HIGH, "Cannot join match in progress\n");

				return;
			}

			set_player_race_ready(self, 1);

			return;

		case 2: // race_break
			if (self->racer && race.status)
			{
				G_bprint(PRINT_HIGH, "%s has quit the race\n", self->netname);
				race_end(self, true, false);
			}

			set_player_race_ready(self, 0);

			return;

		case 3: // race_toggle
			if (self->racer && race.status)
			{
				G_bprint(PRINT_HIGH, "%s has quit the race\n", self->netname);
				race_end(self, true, false);
			}

			set_player_race_ready(self, !self->race_ready);

			return;

		case 4: // race_cancel
			if (!self->racer)
			{
				return;
			}

			if (!race.status)
			{
				return;
			}

			// do some sound
			sound(self, CHAN_ITEM, "boss2/idle.wav", 1, ATTN_NONE);
			G_bprint(PRINT_HIGH, "%s aborted %s run\n", self->netname, g_his(self));
			race_end(self, true, false);

			return;

		default:
			return;
	}
}

void r_timeout()
{
	char arg_1[64];

	if (!race_command_checks())
	{
		return;
	}

	if (race_is_started())
	{
		return;
	}

	trap_CmdArgv(1, arg_1, sizeof(arg_1));

	race.timeout_setting = atoi(arg_1);

	if (!race.timeout_setting)
	{
		race.timeout_setting = RACE_DEFAULT_TIMEOUT;
	}

	race.timeout_setting = bound(1, race.timeout_setting, RACE_MAX_TIMEOUT);

	G_bprint(2, "%s set race time limit to %ss\n", self->netname, dig3(race.timeout_setting));
}

void race_download_record_demo(void)
{
	int record = read_record_param(1);

	if (!race_command_checks())
	{
		return;
	}

	if (!is_valid_record(&race.records[record]))
	{
		G_sprint(self, 2, "record not found\n");

		return;
	}

	if (strnull(race.records[record].demoname))
	{
		G_sprint(self, 2, "demo for record #%d is not available\n", record + 1);

		return;
	}

	stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "download \"demos/%s.mvd\"\n",
					race.records[record].demoname);
}

void display_record_details()
{
	int record = read_record_param(1);

	if (!race_command_checks())
	{
		return;
	}

	if (!is_valid_record(&race.records[record]))
	{
		G_sprint(self, 2, "record not found\n");

		return;
	}

	G_sprint(
			self,
			2,
			"\n\235\236\236\236\236\236\236\236\236\236\236\236\236\237%s %s\235\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("record"), dig3s("%02d", record + 1));
	G_sprint(self, 2, "time: %s\n",
				dig3s("%7.3f%s", race.records[record].time / 1000, redtext("s")));
	G_sprint(self, 2, "racer: %s\n", race.records[record].racername);
	G_sprint(self, 2, "demo: %s\n", redtext(race.records[record].demoname));
	G_sprint(self, 2, "distance: %s\n", dig3s("%.1f", race.records[record].distance));
	G_sprint(self, 2, "max speed: %s\n", dig3s("%.0f", race.records[record].maxspeed));
	G_sprint(self, 2, "avg speed: %s\n", dig3s("%.0f", race.records[record].avgspeed));
	G_sprint(self, 2, "date: %s\n", redtext(race.records[record].date));
	G_sprint(self, 2, "weapon: %s\n", redtext(race_weapon_mode(race.records[record].weaponmode)));
	G_sprint(self, 2, "falsestart: %s\n",
				redtext(race_falsestart_mode(race.records[record].startmode)));
}

void r_falsestart()
{
	if (!race_command_checks())
	{
		return;
	}

	if (race_is_started())
	{
		return;
	}

	race.falsestart++;

	if ((race.falsestart < raceFalseStartNo) || (race.falsestart >= raceFalseStartMAX))
	{
		race.falsestart = raceFalseStartNo;
	}

	G_bprint(2, "%s set race start mode to %s\n", self->netname,
				redtext(race_falsestart_mode(race.falsestart)));

	read_topscores();
	race_clear_pacemaker();
}

void r_all_break(void)
{
	if (!race_command_checks())
	{
		return;
	}

	race_unready_all();
	G_bprint(2, "%s has %s the race to stop\n", self->netname, redtext("forced"));
}

void r_clear_route(void)
{
	gedict_t *p;

	if (!race_command_checks())
	{
		return;
	}

	if (race_is_started())
	{
		return;
	}

	for (p = world; (p = find_plr(p));)
	{
		setwepall(p);
		p->muted = 0;
	}

	race_remove_ent();

	G_bprint(2, "%s cleared the current route\n", self->netname);
	race_clear_pacemaker();
}

void r_mode()
{
	if (!race_command_checks())
	{
		return;
	}

	if (race_is_started())
	{
		return;
	}

	race.weapon++;

	if ((race.weapon < raceWeaponNo) || (race.weapon >= raceWeaponMAX))
	{
		race.weapon = raceWeaponNo;
	}

	G_bprint(2, "%s set race weapon mode to %s\n", self->netname,
				redtext(race_weapon_mode(race.weapon)));

	read_topscores();
	race_clear_pacemaker();
	race_route_now_custom();
}

qbool race_load_route(int route)
{
	int i;

	if ((route < 0) || (route >= race.cnt) || (route >= MAX_ROUTES))
	{
		return false;
	}

	// remove all checkpoints before load
	race_remove_ent();

	for (i = 0; i < race.route[route].cnt && i < MAX_ROUTE_NODES; i++)
	{
		spawn_race_node(&race.route[route].node[i]);
	}

	race.weapon = race.route[route].weapon;
	race.timeout_setting = bound(1, race.route[route].timeout, RACE_MAX_TIMEOUT);
	race.active_route = route + 1; // mark this is not custom route now

	read_topscores();

	if (!strnull(cvar_string("sv_www_address")))
	{
		localcmd("\nsv_web_postfile ServerApi/UploadTopFile \"\" %s\n", race_filename("top"));
		trap_executecmd();
	}

	return true;
}

void race_print_route_info(gedict_t *p)
{
	if (p)
	{
		G_sprint(p, 2, "\235\236\236\236\236\237 %s \235\236\236\236\236\237\n", race_route_name());
		G_sprint(p, 2, "%s %2d \220tl: %ssec\221\n", redtext("route"), race.active_route,
					dig3(race.timeout_setting));

		if (race.active_route)
		{
			G_sprint(p, 2, "\220%s\221\n", race_route_desc());
		}

		G_sprint(p, 2, "%s: %s\n", redtext("weapon"), race_weapon_mode(race.weapon));
	}
	else
	{
		G_bprint(2, "\235\236\236\236\236\237 %s \235\236\236\236\236\237\n", race_route_name());
		G_bprint(2, "%s %2d \220tl: %ssec\221\n", redtext("route"), race.active_route,
					dig3(race.timeout_setting));

		if (race.active_route)
		{
			G_bprint(2, "\220%s\221\n", race_route_desc());
		}

		G_bprint(2, "%s: %s\n", redtext("weapon"), race_weapon_mode(race.weapon));
	}
}

void r_route(void)
{
	if (!race_command_checks())
	{
		return;
	}

	if (race_is_started())
	{
		return;
	}

	HideSpawnPoints();
	race_cleanmap();

	if (race.cnt < 1)
	{
		G_sprint(self, 2, "No routes defined for this map\n");

		return;
	}

	// If server-side toggle and map matches, load correct route
	if ((self->ct != ctPlayer) && streq(cvar_string(RACE_ROUTE_MAPNAME_CVAR), mapname))
	{
		next_route = cvar(RACE_ROUTE_NUMBER_CVAR);
	}
	else
	{
		next_route++;
	}

	if ((next_route < 0) || (next_route >= race.cnt))
	{
		next_route = 0;
	}

	if (!race_load_route(next_route))
	{
		// we failed to load, clean it a bit then
		race_remove_ent();
		race_route_now_custom();

		if (self->ct == ctPlayer)
		{
			G_bprint(2, "Failed to load route %d by %s\n", next_route + 1, self->netname);
		}
		else
		{
			G_bprint(2, "Server failed to load route %d\n", next_route + 1);
		}

		return;
	}

	if (self->ct == ctPlayer)
	{
		race_print_route_info( NULL);
		G_bprint(2, "route loaded by %s\n", self->netname);
	}
	else
	{
		race_print_route_info( NULL);
		G_bprint(2, "Server loaded route %d\n", next_route);
	}

	cvar_fset(RACE_ROUTE_NUMBER_CVAR, next_route);
	cvar_set(RACE_ROUTE_MAPNAME_CVAR, mapname);
	race_clear_pacemaker();
}

void r_print()
{
	if (!race_command_checks())
	{
		return;
	}

	race_print_route_info(self);
}

void race_fclose(void)
{
	if (race_fhandle < 0)
	{
		return;
	}

	trap_FS_CloseFile(race_fhandle);
	race_fhandle = -1;
}

static void race_fwopen(const char *fmt, ...)
{
	va_list argptr;
	char text[MAX_TXTLEN] =
		{ 0 };

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	text[sizeof(text) - 1] = 0;

	if (trap_FS_OpenFile(text, &race_fhandle, FS_WRITE_BIN) < 0)
	{
		race_fhandle = -1;
		//G_bprint( 2, "Failed to open file: %s\n", text );

		return;
	}

	//G_bprint( 2, "Succesfully opened file: %s\n", text );
}

static void race_fropen(const char *fmt, ...)
{
	va_list argptr;
	char text[MAX_TXTLEN] =
		{ 0 };

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	text[sizeof(text) - 1] = 0;

	if (trap_FS_OpenFile(text, &race_fhandle, FS_READ_BIN) < 0)
	{
		race_fhandle = -1;
		//G_bprint( 2, "Failed to open file: %s\n", text );

		return;
	}

	//G_bprint( 2, "Succesfully opened file: %s\n", text );
}

static void race_fprintf(const char *fmt, ...)
{
	va_list argptr;
	char text[MAX_TXTLEN] =
		{ 0 };

	if (race_fhandle < 0)
	{
		return;
	}

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	text[sizeof(text) - 1] = 0;

	trap_FS_WriteFile(text, strlen(text), race_fhandle);
}

void write_topscores(void)
{
	int i;

	if (!race.active_route)
	{
		return;
	}

	race_fwopen("%s", race_filename("top"));
	if (race_fhandle < 0)
	{
		return;
	}

	race_fprintf("version %d\n", TOP_FILE_VERSION);
	race_fprintf("%d\n", NUM_BESTSCORES);

	for (i = 0; i < NUM_BESTSCORES; i++)
	{
		race_fprintf("%f\n", race.records[i].time);
		race_fprintf("%s\n", race.records[i].racername);
		race_fprintf("%s\n", race.records[i].demoname);
		race_fprintf("%f\n", race.records[i].distance);
		race_fprintf("%f\n", race.records[i].maxspeed);
		race_fprintf("%f\n", race.records[i].avgspeed);
		race_fprintf("%s\n", race.records[i].date);
		race_fprintf("%d\n", race.records[i].weaponmode);
		race_fprintf("%d\n", race.records[i].startmode);
		race_fprintf("%d\n", race.records[i].playernumber);
	}

	race_fclose();
}

int race_fgetc(void)
{
	char c;
	int retval;

	if (race_fhandle < 0)
	{
		return -2;
	}

	retval = trap_FS_ReadFile(&c, 1, race_fhandle);
	//G_bprint( 2, "====> Read char: %d\n", c );

	return ((retval == 1) ? c : -1);
}

char* race_fgets(char *buf, int limit)
{
	int c = '\0';
	char *string;

	if (race_fhandle < 0)
	{
		return NULL;
	}

	string = buf;
	while (--limit > 0 && ((c = race_fgetc()) != -1))
	{
		if ((*string++ = c) == '\n')
		{
			break;
		}
	}

	*string = '\0';
	//G_bprint( 2, "====> Read string: %s\n", buf );

	return ((c == -1) && (string = buf)) ? NULL : buf;
}

void read_topscores(void)
{
	char line[MAX_TXTLEN] =
		{ 0 };
	int cnt, max;
	int format_version = 1;

	if (!race.active_route)
	{
		return;
	}

	race_fropen("%s", race_filename("top"));
	if (race_fhandle >= 0)
	{
		race_fgets(line, MAX_TXTLEN);
		if (!strncmp(line, "version ", sizeof("version ") - 1))
		{
			format_version = atoi(line + sizeof("version ") - 1);
			race_fgets(line, MAX_TXTLEN);
		}

		max = atoi(line);
		for (cnt = 0; cnt < max; cnt++)
		{
			race_fgets(line, MAX_TXTLEN);
			race.records[cnt].time = atof(line);
			race_fgets(line, MAX_TXTLEN);
			strlcpy(race.records[cnt].racername, line, strlen(line));
			race_fgets(line, MAX_TXTLEN);
			strlcpy(race.records[cnt].demoname, line, strlen(line));
			race_fgets(line, MAX_TXTLEN);
			race.records[cnt].distance = atof(line);
			race_fgets(line, MAX_TXTLEN);
			race.records[cnt].maxspeed = atof(line);
			race_fgets(line, MAX_TXTLEN);
			race.records[cnt].avgspeed = atof(line);
			race_fgets(line, MAX_TXTLEN);
			strlcpy(race.records[cnt].date, line, strlen(line));
			race_fgets(line, MAX_TXTLEN);
			race.records[cnt].weaponmode = atoi(line);
			race_fgets(line, MAX_TXTLEN);
			race.records[cnt].startmode = atoi(line);
			if (format_version >= 2)
			{
				race_fgets(line, MAX_TXTLEN);
				race.records[cnt].playernumber = atoi(line);
			}
			else
			{
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

void ChasecamToggleButton(void)
{
	if (!(((int)(self->s.v.flags)) & FL_ATTACKRELEASED))
	{
		return;
	}

	self->s.v.flags = (int)self->s.v.flags & ~FL_ATTACKRELEASED;

	r_changefollowstatus((float) 3);
}

void race_set_teleport_flags_by_name(const char *name, int flags)
{
	gedict_t *ent;

	for (ent = world; (ent = ez_find(ent, "trigger_teleport"));)
	{
		if (streq(ent->target, name))
		{
			ent->race_flags = flags;
		}
	}
}

void race_route_create(void)
{
	gedict_t *route[MAX_ROUTE_NODES] =
		{ 0 };
	gedict_t *current = self;
	int route_nodes = 0;
	int i = 0;

	// If race mode not enabled, just ignore quietly
	if (!isRACE())
	{
		return;
	}

	// Name & description are mandatory, ignore route if not specified
	if (strnull(self->race_route_name) || strnull(self->race_route_description))
	{
		G_bprint(2, "Route name/description not specified\n");

		return;
	}

	// weapon-mode must be valid
	if ((self->race_route_weapon_mode <= raceWeaponUnknown)
			|| (self->race_route_weapon_mode >= raceWeaponMAX))
	{
		G_bprint(2, "Route weapon mode not valid\n");

		return;
	}

	// false-start-mode must be valid
	if ((self->race_route_falsestart_mode <= raceFalseStartUnknown)
			|| (self->race_route_falsestart_mode >= raceFalseStartMAX))
	{
		G_bprint(2, "Route falsestart mode not valid\n");

		return;
	}

	while (current && current != world)
	{
		// flag current entity to be removed next frame
		if (streq(current->classname, "race_route_marker")
				|| streq(current->classname, "race_route_start"))
		{
			SUB_RM_01(current);
		}

		// route is too long, ignore
		if (route_nodes >= (sizeof(route) / sizeof(route[0])))
		{
			G_bprint(2, "Route too long\n");

			return;
		}

		// route is circular?  ignore
		for (i = 0; i < route_nodes; ++i)
		{
			if (route[i] == current)
			{
				G_bprint(2, "Circular route detected\n");

				return;
			}
		}

		// add to route
		route[route_nodes++] = current;

		// no targetname => end
		if ((current->race_flags & RACEFLAG_TOUCH_RACEEND) || strnull(current->target))
		{
			break;
		}

		// move to next target
		current = find(world, FOFS(targetname), current->target);

		// next target must be route marker, or end the race
		if (current
				&& !((current->race_flags & RACEFLAG_TOUCH_RACEEND)
						|| streq(current->classname, "race_route_marker")))
		{
			G_bprint(2, "Expected route marker, found %s instead\n", current->classname);

			return;
		}
	}

	// must have at least start and end
	if (route_nodes < 2)
	{
		G_bprint(2, "Route too short (%d nodes)\n", route_nodes);

		return;
	}

	// Create route
	if (!race_route_add_start())
	{
		G_bprint(2, "Couldn't create new route\n");

		return;
	}

	// Add path
	for (i = 0; i < route_nodes; ++i)
	{
		raceRouteNodeType_t nodeType = nodeCheckPoint;
		raceRouteNode_t *node = NULL;
		if (i == 0)
		{
			nodeType = nodeStart;
		}
		else if (i == (route_nodes - 1))
		{
			nodeType = nodeEnd;
		}

		node = race_add_route_node(route[i]->s.v.origin[0], route[i]->s.v.origin[1],
									route[i]->s.v.origin[2], route[i]->race_route_start_pitch,
									route[i]->race_route_start_yaw, nodeType);
		if (node)
		{
			VectorCopy(route[i]->s.v.size, node->sizes);
		}
	}

	race_set_route_name(self->race_route_name, self->race_route_description);
	race_set_route_timeout(self->race_route_timeout);
	race_set_route_weapon_mode((raceWeapoMode_t) self->race_route_weapon_mode);
	race_set_route_falsestart_mode((raceFalseStartMode_t) self->race_route_falsestart_mode);
	race_route_add_end();

	if (next_route < 0)
	{
		r_route();
	}
}

void SP_race_route_start(void)
{
	self->s.v.nextthink = g_globalvars.time + 0.001f;// create route once all markers have spawned
	self->think = (func_t) race_route_create;
}

void race_spawn_intermission(float x, float y, float z, float ang_x, float ang_y, float ang_z)
{
	gedict_t *intermission = spawn();
	gedict_t *oldself = self;
	if (!intermission)
	{
		return;
	}

	intermission->classname = "info_intermission";
	VectorSet(intermission->s.v.origin, x, y, z);
	VectorSet(intermission->mangle, ang_x, ang_y, ang_z);
	self = intermission;
	SP_info_intermission();
	self = oldself;
}

void race_add_standard_routes(void)
{
	char line[MAX_TXTLEN];
	char token[MAX_TXTLEN];
	qbool in_route_def = false;
	int lineNumber = 0;
	size_t i = 0;
	char *entityfile;
	qbool valid_file = true;

	entityfile = cvar_string("k_entityfile");
	if (!strnull(entityfile))
	{
		race_fropen("race/routes/%s.route", entityfile);
	}
	else
	{
		race_fropen("race/routes/%s.route", mapname);
	}

	if (race_fhandle >= 0)
	{
		while (race_fgets(line, MAX_TXTLEN))
		{
			int args;

			++lineNumber;
			trap_CmdTokenize(line);
			args = trap_CmdArgc();
			if (!args)
			{
				continue;
			}

			trap_CmdArgv(0, token, MAX_TXTLEN);

			if (!strcmp(token, "race_route_add_start"))
			{
				if (in_route_def)
				{
					G_bprint(
							PRINT_HIGH,
							"#%02d: Invalid route file: race_route_add_start in route definition\n",
							lineNumber);
					valid_file = false;
					break;
				}

				if (!(in_route_def = race_route_add_start()))
				{
					// Silently return, as previous
					G_bprint(PRINT_HIGH, "#%02d: Routes ignored, limit is %d routes/map\n",
								lineNumber, MAX_ROUTES);
					break;
				}
			}
			else if (!strcmp(token, "race_add_route_node"))
			{
				vec3_t position;
				vec3_t angles;

				if (!in_route_def)
				{
					G_bprint(
							PRINT_HIGH,
							"#%02d: Invalid route file: race_add_route_node outside of route definition\n",
							lineNumber);
					valid_file = false;
					break;
				}

				if (args != 6)
				{
					G_bprint(
							PRINT_HIGH,
							"#%02d: Invalid route file: race_add_route_node should have 5 arguments, found %d\n",
							lineNumber, args - 1);
					valid_file = false;
					break;
				}

				for (i = 1; i < 6; ++i)
				{
					trap_CmdArgv(i, token, MAX_TXTLEN);
					if (i <= 3)
					{
						position[i - 1] = atof(token);
					}
					else
					{
						angles[i - 4] = atof(token);
					}
				}

				if (race.route[race.cnt].cnt == 0)
				{
					race_add_route_node(PASSVEC3(position), angles[0], angles[1], nodeStart);
				}
				else
				{
					if (race.route[race.cnt].cnt > 1)
					{
						race.route[race.cnt].node[race.route[race.cnt].cnt - 1].type =
								nodeCheckPoint;
					}
					race_add_route_node(PASSVEC3(position), angles[0], angles[1], nodeEnd);
				}
			}
			else if (!strcmp(token, "race_set_route_name"))
			{
				char route_name[128];
				char route_description[128];
				int j;

				if (args != 3)
				{
					G_bprint(
							PRINT_HIGH,
							"#%02d: Invalid route file: race_set_route_name should have 2 arguments, found %d\n",
							lineNumber, args - 1);
					valid_file = false;
					break;
				}

				if (!in_route_def)
				{
					G_bprint(
							PRINT_HIGH,
							"#%02d: Invalid route file: race_set_route_name outside of route definition\n",
							lineNumber);
					valid_file = false;
					break;
				}

				trap_CmdArgv(1, token, MAX_TXTLEN);
				strlcpy(route_name, token, sizeof(route_name));
				trap_CmdArgv(2, token, MAX_TXTLEN);
				for (i = 0, j = 0; i < strlen(token); ++i)
				{
					if (token[i] == '\\' && token[i + 1] == '\\')
					{
						token[j++] = '\\';
						++i;
					}
					else if (token[i]
							== '\\'&& isdigit(token[i + 1]) && isdigit(token[i + 2]) && isdigit(token[i + 3]))
					{
						token[j++] = 16 * (token[i + 1] - '0') + 8 * (token[i + 2] - '0')
								+ (token[i + 3] - '0');
						i += 3;
					}
					else
					{
						token[j++] = token[i];
					}
				}

				token[j] = '\0';
				strlcpy(route_description, token, sizeof(route_description));

				race_set_route_name(route_name, route_description);
			}
			else if (!strcmp(token, "race_set_route_timeout"))
			{
				if (!in_route_def)
				{
					G_bprint(
							PRINT_HIGH,
							"#%02d: Invalid route file: race_set_route_timeout outside of route definition\n",
							lineNumber);
					valid_file = false;
					break;
				}

				if (args != 2)
				{
					G_bprint(
							PRINT_HIGH,
							"#%02d: Invalid route file: race_set_route_timeout: expected 1 argument, found %d\n",
							lineNumber, args);
					valid_file = false;
					break;
				}

				trap_CmdArgv(1, token, MAX_TXTLEN);
				if (atof(token) > 0)
				{
					race_set_route_timeout(atof(token));
				}
			}
			else if (!strcmp(token, "race_set_route_weapon_mode"))
			{
				if (!in_route_def)
				{
					G_bprint(
							PRINT_HIGH,
							"#%02d: Invalid route file: race_set_route_weapon_mode outside of route definition\n",
							lineNumber);
					valid_file = false;
					break;
				}

				if (args != 2)
				{
					G_bprint(
							PRINT_HIGH,
							"#%02d: Invalid route file: race_set_route_weapon_mode: expected 1 argument, found %d\n",
							lineNumber, args);
					valid_file = false;
					break;
				}

				trap_CmdArgv(1, token, MAX_TXTLEN);
				if (!strcmp(token, "raceWeaponNo"))
				{
					race_set_route_weapon_mode(raceWeaponNo);
				}
				else if (!strcmp(token, "raceWeaponAllowed"))
				{
					race_set_route_weapon_mode(raceWeaponAllowed);
				}
				else if (!strcmp(token, "raceWeapon2s"))
				{
					race_set_route_weapon_mode(raceWeapon2s);
				}
				else
				{
					G_bprint(
							PRINT_HIGH,
							"#%02d: Invalid route file: race_set_route_weapon_mode: invalid argument %s\n",
							lineNumber, token);
					valid_file = false;
					break;
				}
			}
			else if (!strcmp(token, "race_set_route_falsestart_mode"))
			{
				if (!in_route_def)
				{
					G_bprint(
							PRINT_HIGH,
							"#%02d: Invalid route file: race_set_route_falsestart_mode outside of route definition\n",
							lineNumber);
					valid_file = false;
					break;
				}

				if (args != 2)
				{
					G_bprint(
							PRINT_HIGH,
							"#%02d: Invalid route file: race_set_route_falsestart_mode: expected 1 argument, found %d\n",
							lineNumber, args);
					valid_file = false;
					break;
				}

				trap_CmdArgv(1, token, MAX_TXTLEN);
				if (!strcmp(token, "raceFalseStartNo"))
				{
					race_set_route_falsestart_mode(raceFalseStartNo);
				}
				else if (!strcmp(token, "raceFalseStartYes"))
				{
					race_set_route_falsestart_mode(raceFalseStartYes);
				}
				else
				{
					G_bprint(
							PRINT_HIGH,
							"#%02d: Invalid route file: race_set_route_falsestart_mode: invalid argument %s\n",
							lineNumber, token);
					valid_file = false;
					break;
				}
			}
			else if (!strcmp(token, "race_route_add_end"))
			{
				if (!in_route_def)
				{
					G_bprint(
							PRINT_HIGH,
							"#%02d: Invalid route file: race_route_add_end outside of route definition\n",
							lineNumber);
					valid_file = false;
					break;
				}

				race_route_add_end();
				in_route_def = false;
			}
			else if (!strcmp(token, "race_set_teleport_flags_by_name"))
			{
				char name[128];

				if (in_route_def)
				{
					G_bprint(
							PRINT_HIGH,
							"#%02d: Invalid route file: race_set_teleport_flags_by_name inside route definition\n",
							lineNumber);
					valid_file = false;
					break;
				}

				if (args != 3)
				{
					G_bprint(
							PRINT_HIGH,
							"#%02d: Invalid route file: race_set_teleport_flags_by_name: expected 2 arguments, found %d\n",
							lineNumber, args);
					valid_file = false;
					break;
				}

				trap_CmdArgv(1, name, sizeof(name));
				trap_CmdArgv(2, token, sizeof(token));
				if (!strcmp(token, "RACEFLAG_TOUCH_RACEFAIL"))
				{
					race_set_teleport_flags_by_name(name, RACEFLAG_TOUCH_RACEFAIL);
				}
				else if (!strcmp(token, "RACEFLAG_TOUCH_RACEEND"))
				{
					race_set_teleport_flags_by_name(name, RACEFLAG_TOUCH_RACEEND);
				}
			}
			else if (!strcmp(token, "race_set_node_size"))
			{
				if (!in_route_def)
				{
					G_bprint(
							PRINT_HIGH,
							"#%02d: Invalid route file: race_set_node_size outside of route definition\n",
							lineNumber);
					valid_file = false;
					break;
				}

				if (args != 4)
				{
					G_bprint(
							PRINT_HIGH,
							"#%02d: Invalid route file: race_set_node_size: expected 3 arguments, found %d\n",
							lineNumber, args);
					valid_file = false;
					break;
				}

				if (race.route[race.cnt].cnt == 0)
				{
					G_bprint(PRINT_HIGH,
								"#%02d: Invalid route file: race_set_node_size: no node to amend\n",
								lineNumber);
					valid_file = false;
					break;
				}

				for (i = 1; i < 4; ++i)
				{
					trap_CmdArgv(i, token, MAX_TXTLEN);
					race.route[race.cnt].node[race.route[race.cnt].cnt - 1].sizes[i - 1] = atof(
							token);
				}
			}
			else
			{
				G_bprint(PRINT_HIGH, "#%02d: Error: unknown route instruction %s\n", lineNumber,
							token);
				valid_file = false;
				break;
			}
		}

		if (!valid_file)
		{
			memset(&race, 0, sizeof(race));
		}
		race_fclose();
	}
}

qbool race_handle_event(gedict_t *player, gedict_t *entity, const char *eventName)
{
	int player_num = NUM_FOR_EDICT(player) - 1;

	if (!player->racer)
	{
		return false; // can't touch if not the racer
	}

	if (streq(eventName, "touch"))
	{
		if (entity->race_flags & RACEFLAG_TOUCH_RACEFAIL)
		{
			// do some sound
			sound(player, CHAN_ITEM, "boss2/idle.wav", 1, ATTN_NONE);

			race_end(player, true, false);

			return true;
		}
		else if (entity->race_flags & RACEFLAG_TOUCH_RACEEND)
		{
			race_end_point_touched(entity, player);

			return true;
		}
	}
	else if (streq(eventName, "jump") && race.status == raceActive)
	{
		race_capture_t *capture = &player_captures[player_num];
		if (capture->jump_count < (sizeof(capture->jumps) / sizeof(capture->jumps[0])))
		{
			capture->jumps[capture->jump_count].race_time = g_globalvars.time - race.start_time;
			VectorCopy(player->s.v.origin, capture->jumps[capture->jump_count].origin);
			++capture->jump_count;
		}
	}
	else if (streq(eventName, "watermove"))
	{
		if ((player->s.v.watertype == CONTENT_LAVA) || (player->s.v.watertype == CONTENT_SLIME))
		{
			if (player->racer && race.status)
			{
				G_bprint(PRINT_HIGH, "%s failed %s run\n", player->netname, g_his(player));
				race_end(player, true, false);

				return true;
			}
		}
	}
	else if (streq(eventName, "kill"))
	{
		if (player->racer && race.status)
		{
			if (!race_simultaneous() || (race.status >= raceActive))
			{
				G_bprint(PRINT_HIGH, "%s canceled %s run\n", player->netname, g_his(player));
				race_end(player, true, false);
			}

			return true;
		}
		else if (player->race_chasecam)
		{
			return true;
		}
	}

	return false;
}

static float race_toggle_incr_cvar(char *cvar_name, float incr, float min, float max)
{
	float current = cvar(cvar_name);

	current += incr;
	if (current < min || current > max)
	{
		current = min;
	}

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
	{
		return;
	}

	if (race.status)
	{
		G_sprint(self, PRINT_HIGH, "Cannot change pacemaker settings while race is active.\n");

		return;
	}

	trap_CmdArgv(1, buffer, sizeof(buffer));
	if (streq(buffer, "headstart"))
	{
		float new_headstart = race_toggle_incr_cvar(RACE_INCR_PARAMS(HEADSTART));

		G_bprint(PRINT_HIGH, "%s changes pacemaker headstart to \20%.2fs\21\n", self->netname,
					new_headstart);

		return;
	}
	else if (streq(buffer, "trail"))
	{
		float new_resolution = race_toggle_incr_cvar(RACE_INCR_PARAMS(RESOLUTION));

		if (new_resolution)
		{
			G_bprint(PRINT_HIGH, "%s changes pacemaker trail to \20%.2fs\21\n", self->netname,
						new_resolution / 10.0);
		}
		else
		{
			G_bprint(PRINT_HIGH, "%s changes pacemaker trail \20off\21\n", self->netname);
		}

		return;
	}
	else if (streq(buffer, "jumps"))
	{
		qbool enabled = !cvar(RACE_PACEMAKER_JUMPS_CVAR);

		G_bprint(PRINT_HIGH, "%s changes pacemaker jump indicators \20%s\21\n", self->netname,
					enabled ? "on" : "off");

		cvar_fset(RACE_PACEMAKER_JUMPS_CVAR, enabled ? 1 : 0);

		return;
	}
	else if (streq(buffer, "off") || ((guide.capture.position_count != 0) && (trap_CmdArgc() == 1)))
	{
		G_bprint(PRINT_HIGH, "%s disables the pacemaker\n", self->netname);
		cvar_fset(RACE_PACEMAKER_ENABLED_CVAR, 0);
		memset(&guide, 0, sizeof(guide));
		memset(&race.pacemaker_nick, 0, sizeof(race.pacemaker_nick));

		return;
	}
	else
	{
		position = 0;
	}

	// Try and load
	race_fropen("%s", race_filename("pos"));
	if (race_fhandle < 0)
	{
		G_sprint(self, PRINT_HIGH, "Unable to load pacemaker record.\n");

		return;
	}

	race_clear_pacemaker();
	ignoring_lines = false;
	while (race_fgets(buffer, sizeof(buffer)))
	{
		char *ch = buffer;
		qbool error = false;
		int i;

		if (!strncmp(buffer, "version ", sizeof("version ") - 1))
		{
			file_version = atoi(buffer + sizeof("version ") - 1);
			if (file_version > POS_FILE_VERSION)
			{
				G_sprint(self, PRINT_HIGH,
							"Position file is later version (expected %d, found %d)\n",
							POS_FILE_VERSION, file_version);
				guide.capture.position_count = 0;

				return;
			}

			ignoring_lines = true;
		}

		if (!strncmp(buffer, "player ", sizeof("player ") - 1))
		{
			int this_player = atoi(buffer + sizeof("player ") - 1);

			ignoring_lines = (this_player != race.records[position].playernumber);
		}

		if (ignoring_lines)
		{
			continue;
		}

		if (!strncmp(buffer, "jump,", 5))
		{
			race_capture_jump_t *jump;

			if (guide.capture.jump_count
					>= sizeof(guide.capture.jumps) / sizeof(guide.capture.jumps[0]))
			{
				continue;
			}

			jump = &guide.capture.jumps[guide.capture.jump_count];

			ch = buffer + 5;
			jump->race_time = atof(ch);
			ch = strchr(ch, ',');

			for (i = 0; i < 3; ++i)
			{
				if ((error = !ch))
					break;
				jump->origin[i] = atof(ch + 1);
				ch = strchr(ch + 1, ',');
			}

			if (!error)
			{
				++guide.capture.jump_count;
			}
		}
		else
		{
			race_capture_pos_t *capture;
			if (guide.capture.position_count
					>= (sizeof(guide.capture.positions) / sizeof(guide.capture.positions[0])))
			{
				continue;
			}

			capture = &guide.capture.positions[guide.capture.position_count];
			capture->race_time = atof(ch);
			ch = strchr(ch, ',');

			for (i = 0; i < 3; ++i)
			{
				if ((error = !ch))
				{
					break;
				}

				capture->origin[i] = atof(ch + 1);
				ch = strchr(ch + 1, ',');
			}
			for (i = 0; i < 2; ++i)
			{
				if ((error = !ch))
				{
					break;
				}

				capture->angles[i] = atof(ch + 1);
				ch = strchr(ch + 1, ',');
			}

			if (!error)
			{
				++guide.capture.position_count;
			}
		}
	}

	race_fclose();

	guide.position = guide.jump = 0;
	strlcpy(race.pacemaker_nick, race.records[position].racername, sizeof(race.pacemaker_nick));
	race.pacemaker_time = race.records[position].time;

	G_bprint(PRINT_HIGH, "%s sets pacemaker \20%s\21 (%.3f)\n", self->netname,
				race.records[position].racername, race.records[position].time / 1000.0f);
	cvar_fset(RACE_PACEMAKER_ENABLED_CVAR, 1);
}

qbool race_pacemaker_enabled(void)
{
	return cvar(RACE_PACEMAKER_ENABLED_CVAR) && guide.capture.position_count;
}

static void race_pacemaker_announce(gedict_t *pacemaker)
{
	gedict_t *p;

	for (p = world; (p = find_client(p)); /**/)
	{
		if (pacemaker)
		{
			stuffcmd(p, "//ktx race pm %d\n", NUM_FOR_EDICT(pacemaker));
		}
		else
		{
			stuffcmd(p, "//ktx race pm 0\n");
		}
	}
}

static gedict_t* race_pacemaker_entity(qbool create_if_needed)
{
	gedict_t *ent = ez_find(world, "race_pacemaker");

	if ((ent == NULL) && create_if_needed)
	{
		ent = spawn();
		ent->classname = "race_pacemaker";
		ent->walkframe = 0;
		ent->s.v.frame = 0;
		ent->s.v.movetype = MOVETYPE_NONE;
		ent->s.v.flags = 0;
		setmodel(ent, "progs/player.mdl");
		setorigin(ent, 0, 0, 0);
		setsize(ent, PASSVEC3(VEC_HULL_MIN), PASSVEC3(VEC_HULL_MAX));
	}

	return ent;
}

static race_capture_pos_t* race_store_position(race_capture_t *capture, float time, float x,
												float y, float z, float angle_x, float angle_y)
{
	race_capture_pos_t *pos;

	if (capture->position_count >= RACE_MAX_CAPTURES)
	{
		return NULL;
	}

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
	race_capture_pos_t *pos;
	float next_time;
	vec3_t diff, new_origin;
	gedict_t *racer;
	float race_time = g_globalvars.time - race.start_time;

	for (racer = world; (racer = race_find_racer(racer)); /**/)
	{
		int player_num = NUM_FOR_EDICT(racer) - 1;
		race_capture_t *capture = &player_captures[player_num];

		if ((capture->position_count == 0) || (capture->position_count >= (RACE_MAX_CAPTURES - 1)))
		{
			return;
		}

		pos = &capture->positions[capture->position_count - 1];
		next_time = pos->race_time + 1.0 / RACE_CAPTURE_FPS;
		while (pos && race_time > next_time)
		{
			float frac = (next_time - pos->race_time) / (race_time - pos->race_time);

			VectorSubtract(racer->s.v.origin, pos->origin, diff);
			VectorMA(pos->origin, frac, diff, new_origin);
			pos = race_store_position(capture, next_time, PASSVEC3(new_origin),
										racer->s.v.angles[0], racer->s.v.angles[1]);
			next_time += 1.0 / RACE_CAPTURE_FPS;
		}
	}
}

static void race_init_capture(void)
{
	gedict_t *racer;

	memset(&player_captures, 0, sizeof(player_captures));
	for (racer = world; (racer = race_find_racer(racer)); /**/)
	{
		int player_num = NUM_FOR_EDICT(racer) - 1;

		race_store_position(&player_captures[player_num], 0, PASSVEC3(racer->s.v.origin),
							racer->s.v.angles[0], racer->s.v.angles[1]);
	}
}

static void race_finish_capture(qbool store, int player_num)
{
	if (store && player_num >= 0 && player_num < MAX_CLIENTS)
	{
		gedict_t *racer = &g_edicts[player_num + 1];
		race_capture_t *capture =
				race_match_mode() ?
						&player_match_info[player_num].best_run_capture :
						&player_captures[player_num];

		race_fwopen("%s", race_filename("pos"));
		race_fprintf("version %d\n", POS_FILE_VERSION);

		race_store_position(capture, g_globalvars.time - race.start_time,
							PASSVEC3(racer->s.v.origin), racer->s.v.angles[0],
							racer->s.v.angles[1]);

		race_fprintf("player %d\n", player_num);
		if (race_fhandle >= 0)
		{
			int i = 0;

			for (i = 0; i < capture->position_count; ++i)
			{
				race_capture_pos_t *pos = &capture->positions[i];

				race_fprintf("%.3f,%.1f,%.1f,%.1f,%.1f,%.1f\n", pos->race_time,
								PASSVEC3(pos->origin), pos->angles[0], pos->angles[1]);
			}

			for (i = 0; i < capture->jump_count; ++i)
			{
				race_capture_jump_t *jump = &capture->jumps[i];

				race_fprintf("jump,%.3f,%.1f,%.1f,%.1f\n", jump->race_time, PASSVEC3(jump->origin));
			}
		}

		race_fclose();
	}

	memset(&player_captures, 0, sizeof(player_captures));
}

static void race_remove_jump_markers(void)
{
	int i;

	for (i = 0; i < RACE_JUMP_INDICATORS; ++i)
	{
		if (race_jump_indicators[i])
		{
			ent_remove(race_jump_indicators[i]);
			race_jump_indicators[i] = NULL;
		}
	}

	guide.jump = 0;
}

static void race_reset_pacemaker_route(void)
{
	gedict_t *plr;

	race_remove_pacemaker_indicator();
	for (plr = world; (plr = find_plr(plr)); /**/)
	{
		plr->race_closest_guide_pos = 0;
	}
	race_remove_jump_markers();
}

static void race_pacemaker_race_start(void)
{
	gedict_t *racer;

	for (racer = world; (racer = race_find_racer(racer)); /**/)
	{
		racer->race_closest_guide_pos = 0;
	}

	if (race_pacemaker_enabled())
	{
		gedict_t *ent = race_pacemaker_entity(true);

		if (ent)
		{
			race_pacemaker_announce(ent);
			setorigin(ent, PASSVEC3(guide.capture.positions[0].origin));
		}
	}
	else
	{
		race_remove_pacemaker_indicator();
	}

	race_remove_jump_markers();
}

// Sets race_closest_guide_pos to the nearest guide location for each racer
static void race_update_closest_positions(void)
{
	gedict_t *racer;
	int i;

	if (!guide.capture.position_count)
	{
		// No guide loaded
		return;
	}

	for (racer = world; (racer = race_find_racer(racer)); /**/)
	{
		if (racer->race_closest_guide_pos < guide.capture.position_count)
		{
			float closest_distance = 0.0f;
			vec3_t diff;
			int max_record;

			VectorSubtract(racer->s.v.origin,
							guide.capture.positions[racer->race_closest_guide_pos].origin, diff);
			closest_distance = VectorLength(diff);

			// Hmm - this could break if the fastest record goes backwards for more than 10 frames...
			// Previously it searched from the end, but with more than one racer?...
			max_record = min(racer->race_closest_guide_pos + 10, guide.capture.position_count);
			for (i = racer->race_closest_guide_pos + 1; i < max_record; ++i)
			{
				float distance;

				VectorSubtract(racer->s.v.origin, guide.capture.positions[i].origin, diff);
				distance = VectorLength(diff);

				if (distance <= closest_distance)
				{
					closest_distance = distance;
					racer->race_closest_guide_pos = i;
					max_record = min(racer->race_closest_guide_pos + 10,
										guide.capture.position_count);
				}
			}
		}
	}
}

static void race_update_pacemaker(void)
{
	float race_time = 0;
	float frac;
	qbool advanced = false;
	int i;
	qbool removal_required = !race_pacemaker_enabled();

	if (framecount < 5)
	{
		return;
	}

	switch (race.status)
	{
		case raceNone:
			race_time = -10.0f;
			removal_required = true;
			break;

		case raceActive:
			race_time = g_globalvars.time - race.start_time;
			break;

		case raceCD:
			if (race.cd_cnt)
			{
				guide.jump = guide.position = 0;
			}
			race_time = -(race.cd_cnt + (race.cd_next_think - g_globalvars.time));
			break;
	}

	race_time += bound(RACE_PACEMAKER_HEADSTART_MIN, cvar(RACE_PACEMAKER_HEADSTART_CVAR),
						RACE_PACEMAKER_HEADSTART_MAX);

	// If we're finished or player disabled feature, remove the guide
	if (removal_required)
	{
		race_reset_pacemaker_route();
		return;
	}

	// We want a guide, but haven't started racing yet
	if (race_time <= 0)
	{
		guide.position = 0;
	}

	// Advance pointer until at correct record
	while ((guide.position < (guide.capture.position_count - 1))
			&& (guide.capture.positions[guide.position + 1].race_time < race_time))
	{
		++guide.position;
		advanced = true;
	}

	// If the guide ends the map, remove pacemaker, leave the guides
	if (guide.position >= (guide.capture.position_count - 1))
	{
		race_remove_pacemaker_indicator();
	}

	// Guide capture position
	if ((race_time > 0) && (guide.position >= 0) && (guide.position < (guide.capture.position_count - 1)))
	{
		race_capture_pos_t *cur = &guide.capture.positions[guide.position];
		race_capture_pos_t *next = &guide.capture.positions[guide.position + 1];
		vec3_t direction;

		// Sanity...
		if (next->race_time > cur->race_time)
		{
			gedict_t *ent = race_pacemaker_entity(true);
			vec3_t new_origin;

			frac = (race_time - cur->race_time) / (next->race_time - cur->race_time);
			VectorSubtract(next->origin, cur->origin, direction);

			VectorMA(cur->origin, frac, direction, new_origin);

			setorigin(ent, PASSVEC3(new_origin));
			vectoangles(direction, ent->s.v.angles);

			if (advanced)
			{
				if (ent->walkframe >= 6)
				{
					ent->walkframe = 0;
				}

				if (direction[0] < 0)
				{
					ent->s.v.frame = 11 - ent->walkframe;
				}
				else
				{
					ent->s.v.frame = 6 + ent->walkframe;
				}

				++ent->walkframe;
			}
		}
	}

	if (race.status)
	{
		int guide_start = 0;
		int guide_end = 0;

		int resolution = (int)bound(RACE_PACEMAKER_RESOLUTION_MIN,
										cvar(RACE_PACEMAKER_RESOLUTION_CVAR),
										RACE_PACEMAKER_RESOLUTION_MAX);
		if (resolution)
		{
			// From the pacemaker's position backwards
			guide_start = max(0, guide.position - RACE_PACEMAKER_TRAIL_COUNT * resolution);
			if (race.racers_competing == 1)
			{
				// In single race, we draw from player forwards, up to the pacemaker
				gedict_t *racer = race_get_racer();

				if (racer)
				{
					guide_start = min(guide_start, racer->race_closest_guide_pos);
				}
			}

			// Adjust for resolution, to stop LG jumping around as we move forwards
			guide_start = (guide_start / resolution) * resolution;
			guide_end = min(
					guide.position,
					min(guide_start + resolution * RACE_PACEMAKER_TRAIL_COUNT,
						guide.capture.position_count));

			// Create lightning trail for next part of route
			if (guide_start >= 0 && guide_end > 0)
			{
				int num = 0;

				for (i = guide_start; i < guide_end; i += resolution, ++num)
				{
					int next = (int)bound(0, i + resolution, guide.capture.position_count - 1);

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
	float guide_race_time =
			guide_start >= guide.capture.position_count ?
					99999.9f : guide.capture.positions[guide_start].race_time;
	qbool jumps_enabled = cvar(RACE_PACEMAKER_JUMPS_CVAR) && resolution;
	int i;

	// Remove old indicators
	while (race_jump_indicators[0]
			&& ((race_jump_indicator_times[0] < guide_race_time) || !jumps_enabled))
	{
		if (race_jump_indicators[0])
		{
			ent_remove(race_jump_indicators[0]);
		}

		memmove(race_jump_indicator_times, race_jump_indicator_times + 1,
				sizeof(race_jump_indicator_times[0]) * (RACE_JUMP_INDICATORS - 1));
		memmove(race_jump_indicators, race_jump_indicators + 1,
				sizeof(race_jump_indicators[0]) * (RACE_JUMP_INDICATORS - 1));
		race_jump_indicators[RACE_JUMP_INDICATORS - 1] = NULL;
		race_jump_indicator_times[RACE_JUMP_INDICATORS - 1] = -1.0f;
	}

	// Create new indicators
	if ((jumps_enabled && guide.jump < guide.capture.jump_count)
			&& (guide.capture.jumps[guide.jump].race_time <= race_time))
	{
		for (i = 0; i < RACE_JUMP_INDICATORS; ++i)
		{
			if (!race_jump_indicators[i])
			{
				gedict_t *ent = spawn();
				ent->classname = "race_pacemaker_jump";
				ent->s.v.movetype = MOVETYPE_NONE;
				ent->s.v.solid = SOLID_NOT;
				if (k_ctf_custom_models)
				{
					setmodel(ent, "progs/star.mdl");
				}
				else
				{
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
	gedict_t *ent;
	while ((ent = race_pacemaker_entity(false)))
	{
		ent_remove(ent);
		race_pacemaker_announce(NULL);
	}
}

static qbool race_end(gedict_t *racer, qbool valid, qbool complete)
{
	racer->racer = false;
	racer->muted = true;
	race_set_one_player_movetype_and_etc(racer);

	if (valid && !strnull(cvar_string("sv_www_address")) && !strnull(racer->netname))
	{
		const char *map = cvar_string(RACE_ROUTE_MAPNAME_CVAR);
		int route_number = cvar(RACE_ROUTE_NUMBER_CVAR);

		if (!strnull(map) && (route_number >= 0))
		{
			localcmd(
					"\n" // why new line?
					"sv_web_post ServerApi/LogRaceAttempt \"\" map %s routeNumber %d weap %d fs %d racer %s time %.3f complete %s\n",
					map, route_number, race.weapon, race.falsestart, racer->netname,
					race_time() / 1000.0f,
					complete && !race_pacemaker_enabled() ? "true" : "false");
			trap_executecmd();
		}
	}

	if (race_get_racer() == NULL)
	{
		race_over();

		return true;
	}

	return false;
}

static char* race_position_string(int position)
{
	char *positions[] =
		{ "1st place", "2nd place", "3rd place", "4th place", "5th place", "6th place", "7th place",
				"8th place", "9th place", "10th place" };

	if ((position >= 1) && (position <= (sizeof(positions) / sizeof(positions[0]))))
	{
		return positions[position - 1];
	}

	return "";
}

static qbool race_simultaneous(void)
{
	// We don't support matches where the players go turn about
	return (race_match_mode() || cvar(RACE_SIMULTANEOUS_CVAR));
}

void race_player_post_think(void)
{
	if (isRACE())
	{
		// test for multirace
		self->s.v.solid = SOLID_NOT;
		setorigin(self, PASSVEC3(self->s.v.origin));
	}

	race_follow();
}

static int race_encode_user_command(gedict_t *player)
{
	int result = (player->s.v.button0 ? 1 : 0) + (player->s.v.button2 ? 2 : 0);

	if (player->movement[0] > 0)
	{
		result += 4;
	}
	else if (player->movement[0] < 0)
	{
		result += 8;
	}

	if (player->movement[1] > 0)
	{
		result += 16;
	}
	else if (player->movement[1] < 0)
	{
		result += 32;
	}

	if (player->movement[2] > 0)
	{
		result += 64;
	}
	else if (player->movement[2] < 0)
	{
		result += 128;
	}

	return result;
}

void race_player_pre_think(void)
{
	if (isRACE())
	{
		// Set this player to solid so we trigger checkpoints & teleports during move
		self->s.v.solid = (race.status == raceNone || self->racer ? SOLID_SLIDEBOX : SOLID_NOT);
		if ((self->s.v.mins[0] == 0) || (self->s.v.mins[1] == 0))
		{
			// This can happen if the world 'squashes' a SOLID_NOT entity, mvdsv will turn into corpse
			setsize(self, PASSVEC3(VEC_HULL_MIN), PASSVEC3(VEC_HULL_MAX));
		}

		setorigin(self, PASSVEC3(self->s.v.origin));

		if ((self->ct == ctPlayer) && !self->racer && race.status)
		{
			if (self->race_chasecam)
			{
				if (self->s.v.button2)
				{
					if (((int)(self->s.v.flags)) & FL_JUMPRELEASED)
					{
						self->s.v.flags = (int)self->s.v.flags & ~FL_JUMPRELEASED;

						race_advance_chasecam_for_plr(self);
					}
				}
				else
				{
					self->s.v.flags = ((int)(self->s.v.flags)) | FL_JUMPRELEASED;
				}
			}
		}
		else if ((self->ct == ctPlayer) && self->racer && race.status)
		{
			// If server supports storing user commands in hidden packets, don't bother with these
			if (!(sv_extensions & SV_EXTENSIONS_MVDHIDDEN))
			{
				stuffcmd_flags(self, STUFFCMD_DEMOONLY, "//ucmd %f %d %d\n", g_globalvars.time,
								race_encode_user_command(self), NUM_FOR_EDICT(self));
			}
		}
	}
}

void race_simultaneous_toggle(void)
{
	if (!race_command_checks())
	{
		return;
	}

	if (race_is_started())
	{
		return;
	}

	cvar_toggle_msg(self, RACE_SIMULTANEOUS_CVAR, redtext("simultaneous racing"));
}

// match rules

// ... scoring systems

typedef struct race_score_system_s
{
	char *name;                 // friendly name, appears in countdown (keep <= 9 characters)
	int positions[10];          // points allocated for being 1st, 2nd, 3rd etc
	int complete;               // points allocated for completing the course
	int beating;                // points allocated for each opponent beaten
	int dnf_penalty;            // penalise anyone failing to finish (-1? - not use atm)
	int round_max_diff;     // in duels, end early if frag_diff > round_max_diff * rounds_remaining
} race_score_system_t;

// These are very basic, no competitions to base systems on
static race_score_system_t scoring_systems[] =
{
	// Winner only: 1 frag for winning, for winner after x rounds (hoony-mode stylee)
	{ "Win Only",
	{ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 1 },
	// Scaled system: 1 frag for completing run, 1 frag for each opponent beaten, 1 bonus frag for winner
	//   (better for more players... also gives penalty in duels to risky play, 3-0 instead of 3-1)
	{ "Scaled",
	{ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1, 1, 0, 3 },
	// Formula1: points allocated by position (winning-bonus/losing-penalty increases - for 4+ players really)
	{ "Formula1",
	{ 25, 18, 15, 12, 10, 8, 6, 4, 2, 1 }, 0, 0, 0, 25 }
};

#define NUM_SCORING_SYSTEMS (sizeof(scoring_systems) / sizeof(scoring_systems[0]))

void race_scoring_system_toggle(void)
{
	int current = bound(0, (int)cvar(RACE_SCORINGSYSTEM_CVAR), NUM_SCORING_SYSTEMS - 1);

	if (!race_command_checks())
	{
		return;
	}

	if (race_is_started() || match_in_progress)
	{
		return;
	}

	current = (current + 1) % NUM_SCORING_SYSTEMS;
	cvar_fset(RACE_SCORINGSYSTEM_CVAR, current);
	G_bprint(PRINT_HIGH, "%s enabled the \20%s\21 scoring system\n", self->netname,
				scoring_systems[current].name);
}

int race_award_points(int position, int participants)
{
	int current = bound(0, (int)cvar(RACE_SCORINGSYSTEM_CVAR), NUM_SCORING_SYSTEMS - 1);
	race_score_system_t *system = &scoring_systems[current];
	int points = 0;

	if (!race_match_mode())
	{
		return 0;
	}

	if (position <= 0)
	{
		points += system->dnf_penalty;
	}
	else if (position >= 1 && position <= sizeof(system->positions) / sizeof(system->positions[0]))
	{
		points = system->complete;

		points += system->positions[position - 1];

		// This also gives points for coming joint = with another player... change?
		points += (participants - position) * system->beating;
	}

	return points;
}

char* race_scoring_system_name(void)
{
	int current = bound(0, (int)cvar(RACE_SCORINGSYSTEM_CVAR), NUM_SCORING_SYSTEMS - 1);

	if (!race_match_mode())
	{
		return "???";
	}

	return scoring_systems[current].name;
}

// configuration

qbool race_match_mode(void)
{
	return cvar(RACE_MATCH_CVAR);
}

void race_match_toggle(void)
{
	if (!race_command_checks())
	{
		return;
	}

	if (race_is_started())
	{
		return;
	}

	cvar_toggle_msg(self, RACE_MATCH_CVAR, redtext("match mode"));
	cvar_fset("sv_silentrecord", cvar(RACE_MATCH_CVAR) ? 0 : 1);
}

static int TeamSorter(const void *lhs_, const void *rhs_)
{
	const race_stats_score_t *lhs = (const race_stats_score_t*) lhs_;
	const race_stats_score_t *rhs = (const race_stats_score_t*) rhs_;

	if (lhs->score > rhs->score)
	{
		return -1;
	}

	if (rhs->score > lhs->score)
	{
		return 1;
	}

	return strcmp(lhs->name, rhs->name);
}

static void race_match_stats_print(char *title, race_stats_score_t *scores, int team_count)
{
	int i;

	G_bprint(
			2,
			"\n%s\n\n\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext(title));

	for (i = 0; i < team_count; ++i)
	{
		G_bprint(PRINT_HIGH, "\20%s\21: %d points\n", scores[i].name, scores[i].score);
		G_bprint(PRINT_HIGH, "  Completed %d rounds, won %d\n", scores[i].completions,
					scores[i].wins);
		if (scores[i].completions)
		{
			G_bprint(PRINT_HIGH, "  Average time:  %8.3f\n",
						(scores[i].total_time / 1000.0f) / scores[i].completions);
			G_bprint(PRINT_HIGH, "  Average speed: %8.3f\n",
						(scores[i].total_distance / 1000.0f) / scores[i].completions);
			G_bprint(PRINT_HIGH, "  Best time:     %8.3f\n", scores[i].best_time / 1000.0f);
		}
	}
}

static void race_match_stats_apply(race_stats_score_t *stats, gedict_t *player)
{
	int p_num = NUM_FOR_EDICT(player) - 1;

	stats->score += player->s.v.frags;
	stats->wins += player_match_info[p_num].wins;
	stats->completions += player_match_info[p_num].completions;
	if (player_match_info[p_num].best_time)
	{
		if (stats->best_time == 0)
		{
			stats->best_time = player_match_info[p_num].best_time;
		}
		else
		{
			stats->best_time = min(stats->best_time, player_match_info[p_num].best_time);
		}
	}

	if (player_match_info[p_num].completions)
	{
		stats->total_time += player_match_info[p_num].total_time;
		stats->total_distance += player_match_info[p_num].total_distance;
	}
}

static void race_sort_teams(race_stats_score_t *teams, int count)
{
#ifdef Q3_VM
	qbool any_changes = true;

	// bubble-sort, lovely
	while (any_changes)
	{
		int i;

		any_changes = false;
		for (i = 0; i < count - 1; ++i)
		{
			int comp = TeamSorter(&teams[i], &teams[i + 1]);

			if (comp > 0)
			{
				race_stats_score_t temp = teams[i];
				teams[i] = teams[i + 1];
				teams[i + 1] = temp;
				any_changes = true;
			}
		}
	}
#else
	qsort(teams, count, sizeof(teams[0]), TeamSorter);
#endif
}

static void race_match_team_stats(void)
{
	gedict_t *p, *p2;
	int teams_found = 0;
	race_stats_score_t teams[MAX_CLIENTS] =
		{
			{ 0 } };

	// keep track of players we've processed
	for (p = world; (p = race_find_race_participants(p)); /**/)
	{
		p->cnt = 0;
	}

	for (p = world; (p = race_find_race_participants(p)); /**/)
	{
		if (!p->cnt)
		{
			char *team = getteam(p);

			teams[teams_found].name = team;
			race_match_stats_apply(&teams[teams_found], p);

			for (p2 = p; (p2 = race_find_race_participants(p2)); /**/)
			{
				if (!p2->cnt)
				{
					char *team2 = getteam(p2);

					if (streq(team, team2))
					{
						race_match_stats_apply(&teams[teams_found], p2);
						p2->cnt = 1;
					}
				}
			}

			++teams_found;
		}
	}

	race_sort_teams(teams, teams_found);

	race_match_stats_print("Team scores", teams, teams_found);
}

static race_stats_score_t player_stats[MAX_CLIENTS] =
{
	{ 0 }
};

static int players_found = 0;

static void race_match_player_stats(void)
{
	gedict_t *p;

	memset(player_stats, 0, sizeof(player_stats));
	players_found = 0;

	for (p = world; (p = race_find_race_participants(p)); /**/)
	{
		player_stats[players_found].name = p->netname;
		race_match_stats_apply(&player_stats[players_found], p);
		++players_found;
	}

	race_match_stats_print("Player scores", player_stats, players_found);
}

race_stats_score_t* race_get_player_stats(int *players)
{
	*players = players_found;

	return player_stats;
}

void race_match_stats(void)
{
	// TODO: Think up and keep track of stats we want summarised/match
	if (teamplay)
	{
		race_match_team_stats();
	}

	race_match_player_stats();
}

void race_match_start(void)
{
	gedict_t *p;

	// convert all match-ready players to race-ready
	for (p = world; (p = find_plr(p)); /**/)
	{
		p->race_ready = p->ready;
	}

	race.round_number = 0;
	memset(player_match_info, 0, sizeof(player_match_info));
	race_start(false, "%s", "");
}

qbool race_match_started(void)
{
	return (race_match_mode() && (race.status || match_in_progress));
}

void race_switch_usermode(const char *displayName, int players_per_team)
{
	int maxClients = (players_per_team < 0 ? 26 : players_per_team * 2);

	if (!race_command_checks())
	{
		return;
	}

	if (race_is_started())
	{
		return;
	}

	if (players_per_team == 0)
	{
		G_sprint(self, 2, "%s is not a supported race mode\n", displayName);

		return;
	}

	if (match_in_progress)
	{
		G_sprint(self, 2, "Command is locked while %s is in progress\n", redtext("match"));

		return;
	}

	if (!race_match_mode())
	{
		race_match_toggle();
	}

	cvar_fset("maxclients", maxClients);
	cvar_fset("k_maxclients", maxClients);
	cvar_fset("teamplay", players_per_team > 0 ? 3 : 0);
	if (players_per_team < 0)
	{
		cvar_fset("k_mode", gtFFA);
	}
	else if (players_per_team == 1)
	{
		cvar_fset("k_mode", gtDuel);
	}
	else
	{
		cvar_fset("k_mode", gtTeam);
	}

	G_bprint(PRINT_HIGH, "%s %s %s\n", displayName, redtext("settings enabled by"), self->netname);
}

// Return true if new countdown should be started
static void race_match_round_end(void)
{
	gedict_t *racer;

	gedict_t *ed1, *ed2;
	int score1, score2;
	int sc;

	// Award points for this round
	for (racer = world; (racer = race_find_race_participants(racer)); /**/)
	{
		int player_num = NUM_FOR_EDICT(racer) - 1;

		racer->s.v.frags += race_award_points(race.currentrace[player_num].position,
												race.racers_competing);
		if (race.currentrace[player_num].time)
		{
			++player_match_info[player_num].completions;

			if ((player_match_info[player_num].best_time == 0)
					|| (player_match_info[player_num].best_time > race.currentrace[player_num].time))
			{
				memcpy(&player_match_info[player_num].best_run_capture,
						&player_captures[player_num], sizeof(race_capture_t));
				player_match_info[player_num].best_time = race.currentrace[player_num].time;
			}

			player_match_info[player_num].total_time += race.currentrace[player_num].time;
			player_match_info[player_num].total_distance += race.currentrace[player_num].distance;
			if (race.currentrace[player_num].position == 1)
			{
				++player_match_info[player_num].wins;
			}
		}
	}

	ed1 = get_ed_scores1();
	ed2 = get_ed_scores2();
	score1 = get_scores1();
	score2 = get_scores2();
	if ((isDuel() || isFFA()) && ed1 && ed2)
	{
		score1 = ed1->s.v.frags;
		score2 = ed2->s.v.frags;
	}

	sc = score1 - score2;

	// Advance round pointer
	race.round_number = min(race.round_number + 1, race.rounds);
	k_nochange = false;

	if (race.round_number >= race.rounds)
	{
		if (sc == 0)
		{
			if (!teamplay)
			{
				// Remove anyone apart from the top-scorers for deciding round
				for (racer = world; (racer = race_find_race_participants(racer)); /**/)
				{
					racer->race_ready = (racer->s.v.frags == score1);
				}
			}
			else
			{
				// Remove members of all teams not currently on top score...

				// TODO: Support more than 2 teams, consistently with match mode in KTX
				//    Can't see anything to remove losing teams on sudden death mode in
				//    standard teamplay, so leaving them here too...  Does mean that a
				//    third place team could win the match in final round
			}
		}
		else
		{
			// We have a winner, end the race
			race_finish_capture(false, -1);
			EndMatch(false);

			return;
		}
	}
	else if (!teamplay)
	{
		// Standard round... in duels and ffa, can finish as soon as leading player has unassailable lead
		int rounds_remaining = (race.rounds - race.round_number);
		int system_no = bound(0, (int)cvar(RACE_SCORINGSYSTEM_CVAR), NUM_SCORING_SYSTEMS - 1);

		race_score_system_t *system = &scoring_systems[system_no];
		if (system->round_max_diff && ((rounds_remaining * system->round_max_diff) < sc))
		{
			// We have winner, end the race
			G_bprint(PRINT_HIGH, "%d points (%d rounds) available... ending match\n",
						rounds_remaining * system->round_max_diff, rounds_remaining);
			race_finish_capture(false, -1);
			EndMatch(false);

			return;
		}
	}

	// Start next run
	race_start(false, "%s", "");
}

qbool race_can_cancel_demo(void)
{
	if (!isRACE())
	{
		return true;
	}

	// Always save demos as soon as a run is completed
	return ((race.round_number == 0) && (race.racers_complete == 0));
}

int race_count_votes_req(float percentage)
{
	int racers_ready;

	// No-one ready or map recently started, so behave as normal
	if ((g_globalvars.time < 10) || !(racers_ready = race_count_ready_players()))
	{
		return ceil(percentage * CountPlayers());
	}

	return racers_ready;
}

qbool race_allow_map_vote(gedict_t *player)
{
	int racers_ready;

	// No-one ready or map recently started, so behave as normal
	if ((g_globalvars.time < 10) || !(racers_ready = race_count_ready_players()))
	{
		return true;
	}

	return ((racers_ready == 0) || player->race_ready);
}

void race_hide_players_toggle(void)
{
	if (!race_command_checks())
	{
		return;
	}

	self->hideplayers_default = !self->hideplayers_default;

	G_sprint(self, PRINT_HIGH, "Racers %s during race\n",
				self->hideplayers_default ? redtext("hidden") : redtext("shown"));

	if (race.status)
	{
		self->hideplayers = self->hideplayers_default;
	}
}
