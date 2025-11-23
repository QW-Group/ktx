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
 *  $Id: g_local.h 562 2007-09-24 10:11:18Z d3urk $
 */

// g_local.h -- local definitions for game module
//#define DEBUG
#define CTF_RELOADMAP /* changing ctf status will force map reload */
//#define HITBOXCHECK /* various changes which help test players hit box */

#include "q_shared.h"
#include "mathlib.h"
#include "progs.h"
#include "g_public.h"
#include "g_consts.h"
#include "g_syscalls.h"
#include "player.h"
#include "rng.h"

#if defined(_WIN32)
#define QW_PLATFORM				"Windows"
#define QW_PLATFORM_SHORT		"w"

#elif defined(__FreeBSD__)
#define QW_PLATFORM				"FreeBSD"
#define QW_PLATFORM_SHORT		"f"

#elif defined(__OpenBSD__)
#define QW_PLATFORM				"OpenBSD"
#define QW_PLATFORM_SHORT		"o"

#elif defined(__NetBSD__)
#define QW_PLATFORM				"NetBSD"
#define QW_PLATFORM_SHORT		"n"

#elif defined(__DragonFly__)
#define QW_PLATFORM				"DragonFly"
#define QW_PLATFORM_SHORT		"d"

#elif defined(__linux__)
#define QW_PLATFORM				"Linux"
#define QW_PLATFORM_SHORT		"l"

#elif defined(__sun__)
#define QW_PLATFORM				"SunOS"
#define QW_PLATFORM_SHORT		"s"

#elif defined(__APPLE__)
#define QW_PLATFORM				"MacOS"
#define QW_PLATFORM_SHORT		"m"

#else
#define QW_PLATFORM				"Unknown"
#define QW_PLATFORM_SHORT		"u"
#endif

#define MOD_NAME				("KTX")
#define MOD_FULLNAME			("KTX: Kombat Teams eXtreme")
#define MOD_VERSION				("1.47-dev")
#define MOD_BUILD_DATE			(__DATE__ ", " __TIME__)
#define MOD_SERVERINFO_MOD_KEY	("ktxver")
#define MOD_URL					("https://github.com/QW-Group/ktx")
#define GIT_COMMIT				("")

// keep lines to 38 chars max
//#define MOD_RELEASE_QUOTE		("This KTX version is dedicated to the\n2022 November QuakeWorld LAN Parties!\nRadomsko QW Lan, Poland, November 9-13\nQHLAN-22, Stockholm, November 17-20\n")
//#define MOD_RELEASE_HASHTAGS	("#COVID19 #StayHomeStaySafe #PlaySomeQW")

// qqshka - hmm, seems in C this is macros
#undef max
#undef min

#ifndef min
//#define min(a,b) ((a) < (b) ? (a) : (b))
float min(float a, float b);
#define KTX_MIN
#endif
#ifndef max
//#define max(a,b) ((a) > (b) ? (a) : (b))
float max(float a, float b);
#define KTX_MAX
#endif

float bound(float a, float b, float c);
//#define bound(a,b,c) ((a) >= (c) ? (a) : (b) < (a) ? (a) : (b) > (c) ? (c) : (b))

//used for bots and combat
#define PASSINTVEC3(x) ((int)x[0]),((int)x[1]),((int)x[2])
#define PASSSCALEDINTVEC3(x,y) ((int)(x[0]*y)),((int)(x[1]*y)),((int)(x[2]*y))

#if defined(DEBUG) || defined(_DEBUG)
#define DebugTrap(x) *(char**)0=x
#else
#define DebugTrap(x) G_Error(x)
#endif

#define MOTD_LINES (15)

#define MAX_LASTSCORES (30)

#define MAX_STRINGS 128

#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS	1024	// max tokens resulting from Cmd_TokenizeString
#define	MAX_TOKEN_CHARS		1024	// max length of an individual token

#define	FOFS(x) ((intptr_t)&(((gedict_t *)0)->x))
#define	GOFS(x) ((intptr_t)&(((globalvars_t *)0)->x))

#define FOFCLSN ( FOFS ( classname ) )

int NUM_FOR_EDICT(gedict_t *e);

// possible disallowed weapons

#define DA_WPNS (IT_AXE|IT_SHOTGUN|IT_SUPER_SHOTGUN|IT_NAILGUN|IT_SUPER_NAILGUN|IT_ROCKET_LAUNCHER|IT_GRENADE_LAUNCHER|IT_LIGHTNING)

extern qbool FTE_sv; // is we run on FTE server

extern gameData_t gamedata;
extern gedict_t g_edicts[];
extern char mapname[64];
extern globalvars_t g_globalvars;
extern gedict_t *world;
extern gedict_t *self, *other;
extern gedict_t *newmis;
extern int timelimit, fraglimit, teamplay, deathmatch, framecount, coop, skill;
extern int sv_extensions;
//extern float	rj;

#define	EDICT_TO_PROG(e) ((byte *)(e) - (byte *)g_edicts)
#define PROG_TO_EDICT(e) ((gedict_t *)((byte *)g_edicts + (e)))

void G_Printf(const char *fmt, ...) PRINTF_FUNC(1);
void G_Error(const char *fmt, ...) PRINTF_FUNC(1);

#define PASSVEC3(x) (x[0]),(x[1]),(x[2])
#define SetVector(v,x,y,z) (v[0]=x,v[1]=y,v[2]=z)

// some types

typedef enum
{
	gtUnknown = 0,
	gtDuel,
	gtTeam,
	gtFFA,
	gtCTF
} gameType_t;

typedef enum
{
	umUnknown = 0,
	um1on1,
	um2on2,
	um3on3,
	um4on4,
	um10on10,
	umFfa,
	umCtf,
	umHooneyM,
	umBlitz2v2,
	umBlitz4v4,
	um2on2on2,
	um3on3on3,
	um4on4on4,
	umXonX
} UserModes_t;

typedef enum
{
	lsUnknown = 0,
	lsDuel,
	lsTeam,
	lsFFA,
	lsCTF,
	lsRA, // note no correspoding gameType_t for lsType
	lsCA,
	lsWO,
	lsHM,
	lsRACE
} lsType_t; // lastscores type

// Spike:
// Extension builtins.
// The order of these don't matter (qqshka: what about QVM builds? look g_syscalls.asm, we need something similar for extensions I guess),
// they'll be blindly assigned to slots without conflicting with extras other engines try to add to core.
// They do need indexes that don't conflict with gameImport_t though.
#define G_EXTENSIONS_FIRST 256
enum
{
	G_SETEXTFIELD = G_EXTENSIONS_FIRST,
	G_GETEXTFIELD,
	G_CHANGELEVEL_HUB,
	G_URI_QUERY,
	G_PARTICLEEFFECTNUM,
	G_TRAILPARTICLES,
	G_POINTPARTICLES,
	G_CLIENTSTAT,
	G_POINTERSTAT,
	G_MAPEXTFIELDPTR,
	G_SETEXTFIELDPTR,
	G_GETEXTFIELDPTR,
	G_SETSENDNEEDED,
	G_EXTENSIONS_LAST
};
extern qbool haveextensiontab[G_EXTENSIONS_LAST-G_EXTENSIONS_FIRST];
#define HAVEEXTENSION(idx) haveextensiontab[(idx) - G_EXTENSIONS_FIRST]

#define DEATHTYPE( _dt_, _dt_str_ ) _dt_,
typedef enum
{

#include "deathtype.h"

} deathType_t;
#undef DEATHTYPE

// g_cmd.c

// g_utils.c

void g_random_seed(int);
float g_random(void);
float crandom(void);
int i_rnd(int from, int to);
float dist_random(float minValue, float maxValue, float spreadFactor);
float next_frame(void);
gedict_t* spawn(void);
void ent_remove(gedict_t *t);
void soft_ent_remove(gedict_t *ent);

gedict_t* nextent(gedict_t *ent);
gedict_t* find(gedict_t *start, int fieldoff, char *str);
gedict_t* ez_find(gedict_t *start, char *str);
int find_cnt(int fieldoff, char *str);
gedict_t* find_idx(int idx, int fieldoff, char *str);
//gedict_t       *findradius( gedict_t * start, vec3_t org, float rad );
gedict_t* findradius_ignore_solid(gedict_t *start, vec3_t org, float rad);
void normalize(vec3_t value, vec3_t newvalue);
float vlen(vec3_t value1);
float vectoyaw(vec3_t value1);
void vectoangles(vec3_t value1, vec3_t ret);
void changeyaw(gedict_t *ent);

char* va(char *format, ...) PRINTF_FUNC(1);
char* redtext(char *format);
char* cleantext(char *format);
char* dig3(int d);
char* dig3s(const char *format, ...) PRINTF_FUNC(1);
char* striphigh(char *format);
char* stripcaps(char *format);

void G_sprint(gedict_t *ed, int level, const char *fmt, ...)  PRINTF_FUNC(3);
void G_sprint_flags(gedict_t *ed, int level, int flags, const char *fmt, ...)  PRINTF_FUNC(4);
void G_bprint(int level, const char *fmt, ...)  PRINTF_FUNC(2);
void G_bprint_flags(int level, int flags, const char *fmt, ...) PRINTF_FUNC(3);
void G_centerprint(gedict_t *ed, const char *fmt, ...) PRINTF_FUNC(2);
/* centerprint to all clients */
void G_cp2all(const char *fmt, ...) PRINTF_FUNC(1);

void G_cprint(const char *fmt, ...) PRINTF_FUNC(1);
void G_dprint(const char *fmt, ...) PRINTF_FUNC(1);

void localcmd(const char *fmt, ...) PRINTF_FUNC(1);

int streq(const char *s1, const char *s2);
int strneq(const char *s1, const char *s2);
int strnull(const char *s1);

/*
 #define strneq(s1,s2)	(strcmp( (s1) ? (s1) : "", (s2) ? (s2) : "" ))
 #define streq(s1,s2)	(!strneq((s1), (s2)))
 #define strnull(s1)		((s1) ? !*(s1) : true)
 */

void aim(vec3_t ret);
void setorigin(gedict_t *ed, float origin_x, float origin_y, float origin_z);
void setsize(gedict_t *ed, float min_x, float min_y, float min_z, float max_x, float max_y,
				float max_z);
void setmodel(gedict_t *ed, char *model);
void sound(gedict_t *ed, int channel, char *samp, float vol, float att);
gedict_t* checkclient(void);
void traceline(float v1_x, float v1_y, float v1_z, float v2_x, float v2_y, float v2_z, int nomonst,
				gedict_t *ed);
void TraceCapsule(float v1_x, float v1_y, float v1_z, float v2_x, float v2_y, float v2_z,
					int nomonst, gedict_t *ed, float min_x, float min_y, float min_z, float max_x,
					float max_y, float max_z);

void stuffcmd(gedict_t *ed, const char *fmt, ...) PRINTF_FUNC(2);
void stuffcmd_flags(gedict_t *ed, int flags, const char *fmt, ...) PRINTF_FUNC(3);
int droptofloor(gedict_t *ed);
int walkmove(gedict_t *ed, float yaw, float dist);
int movetogoal(float dist);
int checkbottom(gedict_t *ed);
void makestatic(gedict_t *ed);
void setspawnparam(gedict_t *ed);
void logfrag(gedict_t *killer, gedict_t *killee);
char* infokey(gedict_t *ed, char *key, char *valbuff, int sizebuff);

char* ezinfokey(gedict_t *ed, char *key);
int iKey(gedict_t *ed, char *key);
float fKey(gedict_t *ed, char *key);

void WriteEntity(int to, gedict_t *ed);

void WriteByte(int to, int data);
void WriteShort(int to, int data);
void WriteLong(int to, int data);
void WriteString(int to, char *data);
void WriteAngle(int to, float data);
void WriteCoord(int to, float data);

float cvar(const char *var);
char* cvar_string(const char *var);
void cvar_set(const char *var, const char *val);
void cvar_fset(const char *var, float val);

#define MAX_TEAM_NAME (32)  //  qqshka: 32 - is max len of team name?

//extern char teams[MAX_CLIENTS][MAX_TEAM_NAME];
int getteams(char teams[MAX_CLIENTS][MAX_TEAM_NAME]);
char* getteam(gedict_t *ed);
char* getname(gedict_t *ed);

char* g_his(gedict_t *ed);
char* g_he(gedict_t *ed);
char* g_himself(gedict_t *ed);

gedict_t* find_client(gedict_t *start);
gedict_t* find_plr(gedict_t *start);
gedict_t* find_plr_same_team(gedict_t *start, char *team);
gedict_t* find_spc(gedict_t *start);
gedict_t* find_plrghst(gedict_t *start, int *from);
gedict_t* find_plrspc(gedict_t *start, int *from);
gedict_t* player_by_id(int id);
gedict_t* player_by_name(const char *name);
gedict_t* player_by_IDorName(const char *IDname);
gedict_t* spec_by_id(int id);
gedict_t* spec_by_name(const char *name);
gedict_t* spec_by_IDorName(const char *IDname);

gedict_t* SpecPlayer_by_IDorName(const char *IDname);
gedict_t* SpecPlayer_by_id(int id);

gedict_t* not_connected_by_id(int id);
gedict_t* not_connected_by_name(const char *name);
gedict_t* not_connected_by_IDorName(const char *IDname);

char* armor_type(int items);

qbool isghost(gedict_t *ed);

qbool isDuel(void);
qbool isTeam(void);
qbool isFFA(void);
qbool isCTF(void);
qbool isUnknown(void);
int tp_num(void);
int GetUserID(gedict_t *p);
char* TrackWhom(gedict_t *p);
int GetHandicap(gedict_t *p);
qbool SetHandicap(gedict_t *p, int nhdc);
void changelevel(const char *name);
char* Get_PowerupsStr(void);
int Get_Powerups(void);

char* count_s(int cnt);
char* Enables(float f);
char* Enabled(float f);
char* Allows(float f);
char* Allowed(float f);
char* OnOff(float f);

int get_scores1(void);
int get_scores2(void);
int get_scores3(void);
gedict_t* get_ed_scores1(void);
gedict_t* get_ed_scores2(void);

gedict_t* get_ed_best1(void);
gedict_t* get_ed_best2(void);
gedict_t* get_ed_bestPow(void);

char* str_noweapon(int k_disallow_weapons);

void cvar_toggle_msg(gedict_t *p, char *cvarName, char *msg);

qbool can_exec(char *name);

void ghostClearScores(gedict_t *g);
void update_ghosts(void);

// { events

void on_enter(void);
void on_connect(void);
void on_match_start(gedict_t *p);
void on_match_end(gedict_t *p);
void on_match_break(gedict_t *p);
void on_admin(gedict_t *p);
void on_unadmin(gedict_t *p);
void on_countdown(gedict_t *p);

void info_ev_update(gedict_t *p, char *from, char *to);
void info_kf_update(gedict_t *p, char *from, char *to);

// }

void cl_refresh_plus_scores(gedict_t *p);
void refresh_plus_scores(void);

void disableupdates(gedict_t *ed, float time);

int only_digits(const char *s);

char* params_str(int from, int to);

char* SD_type_str(void); // sudden death type string

char* respawn_model_name(int mdl_num);
char* respawn_model_name_short(int mdl_num);

int get_fair_pack(void);
int get_fallbunny(void);

void remove_projectiles(void);

void SetUserInfo(gedict_t *p, const char *varname, const char *value, int flags);

void safe_precache_model(char *name);
void safe_precache_sound(char *name);

char* cl_ip(gedict_t *p);

char* clean_string(char *s);

void visible_to(gedict_t *viewer, gedict_t *first, int len, byte *visible);

qbool socd_movement_assisted(gedict_t *p);

// Work around for the fact that QVM dos not support ".*s" in printf() family functions.
// It retuns dots array filled with dots, amount of dots depends of how long cmd name and longest cmd name.
char* make_dots(char *dots, size_t dots_len, int cmd_max_len, char *cmd);

//
// subs.c
void SUB_CalcMove(vec3_t tdest, float tspeed, void (*func)(void));
void SUB_CalcMoveEnt(gedict_t *ent, vec3_t tdest, float tspeed, void (*func)(void));
void SUB_UseTargets(void);
void SetMovedir(void);
void InitTrigger(void);
extern gedict_t *activator;

//
// g_mem.c
void* G_Alloc(int size);
void G_InitMemory(void);

//
// g_spawn.c
void G_SpawnEntitiesFromString(void);
qbool G_SpawnString(const char *key, const char *defaultString, char **out);
qbool G_SpawnFloat(const char *key, const char *defaultString, float *out);
qbool G_SpawnInt(const char *key, const char *defaultString, int *out);
qbool G_SpawnVector(const char *key, const char *defaultString, float *out);
void SUB_Remove(void);
void SUB_RM_01(gedict_t *ent);
void SUB_Null(void);

//world.c
void CopyToBodyQue(gedict_t *ent);
void ClearBodyQue(void);

// client.c
extern vec3_t VEC_ORIGIN;
extern vec3_t VEC_HULL_MIN;
extern vec3_t VEC_HULL_MAX;
extern vec3_t VEC_HULL2_MIN;
extern vec3_t VEC_HULL2_MAX;
extern float intermission_running;
extern float intermission_exittime;
extern gedict_t *intermission_spot;
extern int modelindex_eyes, modelindex_player;

void set_nextmap(char *map);
void GotoNextMap(void);

qbool CheckRate(gedict_t *p, char *newrate);

int tiecount(void);
void Check_SD(gedict_t *p);

void SetChangeParms(void);
void SetNewParms(void);
void ClientConnect(void);
void k_respawn(gedict_t *p, qbool body);
void PutClientInServer(void);
void ClientDisconnect(void);
void PlayerPreThink(void);
void BothPostThink(void); // <- called for player and spec
void PlayerPostThink(void);
qbool PlayerCanPause(gedict_t *p);
void SuperDamageSound(void);

char *Spawn_GetModel(void);
gedict_t* SelectSpawnPoint(char *spawnname);

#define         WP_STATS_UPDATE (0.3f)
void Print_Wp_Stats(void);
#define         SC_STATS_UPDATE (0.8f)
void Print_Scores(void);

// { "new weapon stats"
void WS_Mark(gedict_t *p, weaponName_t wp);
void WS_Reset(gedict_t *p);
void WS_OnSpecPovChange(gedict_t *s, qbool force);
// }

//spectate.c
void SpectatorConnect(void);
void PutSpectatorInServer(void);
void SpectatorDisconnect(void);
void SpectatorThink(void);

// weapons.c
extern int impulse;
float W_BestWeapon(void);
void W_Precache(void);
void W_SetCurrentAmmo(void);
void SpawnBlood(vec3_t, float);
void SpawnMeatSpray(vec3_t org, vec3_t vel);
void W_FireAxe(void);
void W_FireSpikes(float ox);
void W_FireLightning(void);
void LightningDamage(vec3_t p1, vec3_t p2, gedict_t *from, float damage);
qbool W_CanSwitch(int wp, qbool warn);

void FireBullets(float shotcount, vec3_t dir, float spread_x, float spread_y, float spread_z,
					deathType_t deathtype);

void launch_spike(vec3_t org, vec3_t dir);

// match.c
int WeirdCountPlayers(void);
float CountPlayers(void);
float CountBots(void);
float CountRTeams(void);
qbool isCanStart(gedict_t *s, qbool forceMembersWarn);
void StartTimer(void);
void StopTimer(int removeDemo);

char* WpName(weaponName_t wp);

//combat.c
extern gedict_t *damage_attacker, *damage_inflictor;

char* death_type(deathType_t dt);

qbool ISLIVE(gedict_t *e);
qbool ISDEAD(gedict_t *e);

qbool CanDamage(gedict_t *targ, gedict_t *inflictor);

void T_Damage(gedict_t *targ, gedict_t *inflictor, gedict_t *attacker, float damage);
void T_RadiusDamage(gedict_t *inflictor, gedict_t *attacker, float damage, gedict_t *ignore,
					deathType_t dtype);
void T_BeamDamage(gedict_t *attacker, float damage);

//items.c
void DropPowerup(float timeleft, int powerup);
void DropPowerups(void);
void ShowSpawnPoints(void);
void HideSpawnPoints(void);
void DropBackpack(void);

void adjust_pickup_time(float *current, float *total);

//triggers.c
void play_teleport(gedict_t *sndspot);
void spawn_tfog(vec3_t org);

#define	TFLAGS_FOG_SRC				(1<<0)
#define	TFLAGS_FOG_DST				(1<<1)
#define	TFLAGS_FOG_DST_SPAWN		(1<<2)
#define	TFLAGS_SND_SRC				(1<<3)
#define	TFLAGS_SND_DST				(1<<4)
#define	TFLAGS_VELOCITY_ADJUST		(1<<5)

void teleport_player(gedict_t *player, vec3_t origin, vec3_t angles, int flags);

#define TELEDEATH(e) ((e)->deathtype == dtTELE1 || (e)->deathtype == dtTELE2 || (e)->deathtype == dtTELE3)

// runes.c
void DropRune(void);
void SpawnRunes(qbool yes);
void TossRune(void);
void ResistanceSound(gedict_t *player);
void HasteSound(gedict_t *player);
void RegenerationSound(gedict_t *player);

// ctf.c
void PlayerDropFlag(gedict_t *player, qbool tossed);
void RegenFlags(qbool yes);
void AddHook(qbool yes);
void CTF_Obituary(gedict_t *targ, gedict_t *attacker);
void CTF_CheckFlagsAsKeys(void);

// logs.c
void log_open(const char *fmt, ...) PRINTF_FUNC(1);
void log_printf(const char *fmt, ...) PRINTF_FUNC(1);
void log_close(void);

extern fileHandle_t log_handle;

// commands.c
void WillPause(void);
typedef struct cmd_s
{
	char *name;
//	funcref_t	f;
	void (*f)(void);
	float arg;
	int cf_flags;
	const char *description;
} cmd_t;

#define CF_PLAYER			( 1<<0 )						/* command valid for players */
#define CF_SPECTATOR		( 1<<1 )						/* command valid for specs */
#define CF_BOTH				( CF_PLAYER | CF_SPECTATOR )	/* command valid for both: specs and players */
#define CF_PLR_ADMIN		( 1<<2 )						/* client is player, so this command require admin rights */
#define CF_SPC_ADMIN		( 1<<3 )						/* client is spectator, so this command require admin rights */
#define CF_BOTH_ADMIN		( CF_PLR_ADMIN | CF_SPC_ADMIN )	/* this command require admin rights, any way */
#define CF_MATCHLESS		( 1<<4 )						/* command valid for matchLess mode */
#define CF_PARAMS			( 1<<5 )						/* command have some params */
#define CF_NOALIAS			( 1<<6 )						/* command haven't alias and may be accessed only via /cmd commandname */
#define CF_REDIRECT			( 1<<7 )						/* command will be redirected to server as /cmd commandname */
#define CF_MATCHLESS_ONLY	( 1<<8 )						/* command valid for matchLess mode _only_ */
#define CF_CONNECTION_FLOOD ( 1<<9 )						/* allow flood at connection time, say first 30 seconds */

extern cmd_t cmds[];

extern int cmds_cnt; // count of commands in 'cmds'

//
// DoCommand/DoCommand_Name return codes
// return non-negative value if command success
//
#define DO_OUT_OF_RANGE_CMDS			(-1) // if command is out of range in 'cmds' array or not found
#define DO_WRONG_CLASS					(-2) // if wrong class
#define DO_ACCESS_DENIED				(-3) // if access denied
#define DO_FUNCTION_IS_WRONG			(-4) // if function is wrong
#define DO_CMD_DISALLOWED_MATCHLESS		(-5) // if cmd does't allowed in matchLess mode
#define DO_FLOOD_PROTECT				(-6) // if command is blocked due to flood protect
#define DO_CMD_MATCHLESS_ONLY			(-7) // if cmd allowed in matchLess mode _only_

int DoCommand(int icmd);
int DoCommand_Name(char *cmd_name);

qbool isCmdFlood(gedict_t *p);

void Init_cmds(void);

void StuffModCommands(gedict_t *p);

void SetPractice(int srv_practice_mode, const char *map);

void execute_rules_reset(void);

// { usermodes

// um_flags

#define UM_1ON1		( 1<<0  ) // `0000 0000 0000 0000 0000 0000 0000 0001` = 1
#define UM_2ON2		( 1<<1  ) // `0000 0000 0000 0000 0000 0000 0000 0010` = 2
#define UM_3ON3		( 1<<2  ) // `0000 0000 0000 0000 0000 0000 0000 0100` = 4
#define UM_4ON4		( 1<<3  ) // `0000 0000 0000 0000 0000 0000 0000 1000` = 8
#define UM_10ON10	( 1<<4  ) // `0000 0000 0000 0000 0000 0000 0001 0000` = 16
#define UM_FFA		( 1<<5  ) // `0000 0000 0000 0000 0000 0000 0010 0000` = 32
#define UM_CTF		( 1<<6  ) // `0000 0000 0000 0000 0000 0000 0100 0000` = 64
#define UM_1ON1HM	( 1<<7  ) // `0000 0000 0000 0000 0000 0000 1000 0000` = 128
#define UM_2ON2ON2	( 1<<8  ) // `0000 0000 0000 0000 0000 0001 0000 0000` = 256
#define UM_3ON3ON3	( 1<<9  ) // `0000 0000 0000 0000 0000 0010 0000 0000` = 512
#define UM_4ON4ON4	( 1<<10 ) // `0000 0000 0000 0000 0000 0100 0000 0000` = 1024
#define UM_XONX	    ( 1<<11 ) // `0000 0000 0000 0000 0000 1000 0000 0000` = 2048
#define UM_RACEMODE ( 1<<31 ) // `1000 0000 0000 0000 0000 0000 0000 0000` =

typedef struct usermode_s
{
	const char *name;
	const char *displayname;
	const char *initstring;
	int um_flags;
	int race_plrs_per_team;
} usermode;

extern usermode um_list[];
extern int um_cnt;  // count of entrys in 'um_list'
extern UserModes_t current_umode; // current UserMode

// for user call this like UserMode( 1 )
// for server call like UserMode( -1 )
void UserMode(float umode);

int um_idx_byname(char *name); // return -1 if not found
const char *um_name_byidx(UserModes_t idx);

// }

// { spec more info

qbool mi_on(void);
qbool mi_adm_only(void);
void mi_print(gedict_t *tooker, int it, char *msg);
void info_mi_update(gedict_t *p, char *from, char *to);

// }

// { communication commands

void s_lr_clear(gedict_t *dsc);

// }

// vip.c

// qqshka: hmm, like ktpro

#define VIP_NORMAL      ( 1) // normal VIP (default)
#define VIP_NOTKICKABLE ( 2) // not kickable VIP by elected admins
#define	VIP_ADMIN       ( 4) // VIP with admin rights
//		 8 - VIP with demo admin rights
//		16 - VIP with judge rights
#define VIP_RCON		(32) // VIP with rcon admin rights

#define ALLOWED_NOSPECS_VIPS ( VIP_NOTKICKABLE | VIP_ADMIN | VIP_RCON )

int VIP(gedict_t *cl);
int VIP_IsFlags(gedict_t *cl, int flags);
void VIP_ShowRights(gedict_t *cl);

// g_userinfo.c

void cmdinfo_infoset(gedict_t *p);

// vote.c

typedef struct votemap_s
{
	int map_id;
	int map_votes;
	int admins;
} votemap_t;

typedef struct rpickupTeams_s
{
	char name[5];
	char topColor[3];
	char bottomColor[3];
	char stuffCmd[30];
} rpickupTeams_t;

typedef struct suggestcolor_s
{
	int top;
	int bottom;
	int num_userids;
	int userids[MAX_CLIENTS];
} suggestcolor_t;

extern votemap_t maps_voted[];
int vote_get_maps(void);

qbool is_elected(gedict_t *p, electType_t et);

int get_elect_type(void);
char* get_elect_type_str(void);

void vote_clear(int fofs);
int get_votes_req(int fofs, qbool diff);
int get_votes(int fofs);
int get_votes_by_value(int fofs, int value);
int is_admins_vote(int fofs);

void vote_check_map(void);
void vote_check_break(void);
void vote_check_elect(void);
void vote_check_pickup(void);
void vote_check_rpickup(int maxRecursion);
void vote_check_swapall(void);
void vote_check_all(void);

#define	VOTE_FOFS(x) ((intptr_t)&(((vote_t *)0)->x))

#define OV_BREAK ( VOTE_FOFS ( brk ) )
#define OV_ELECT ( VOTE_FOFS ( elect ) )
#define OV_PICKUP ( VOTE_FOFS ( pickup ) )
#define OV_RPICKUP ( VOTE_FOFS ( rpickup ) )
#define OV_MAP ( VOTE_FOFS ( map ) )
#define OV_NOSPECS ( VOTE_FOFS ( nospecs ) )
#define OV_TEAMOVERLAY ( VOTE_FOFS ( teamoverlay ) )
#define OV_COOP ( VOTE_FOFS ( coop ) )
#define OV_HOOKSMOOTH (VOTE_FOFS ( hooksmooth ) )
#define OV_HOOKFAST (VOTE_FOFS ( hookfast ) )
#define OV_HOOKCLASSIC (VOTE_FOFS ( hookclassic ) )
#define OV_HOOKCRHOOK (VOTE_FOFS ( hookcrhook ) )
#define OV_ANTILAG ( VOTE_FOFS ( antilag ) )
#define OV_PRIVATE ( VOTE_FOFS ( privategame ) )
//#define OV_KICKUNAUTHED ( VOTE_FOFS (kick_unauthed) )
#define OV_SWAPALL ( VOTE_FOFS ( swapall ) )
#define MAX_RPICKUP_RECUSION 3
#define MAX_PAUSE_REQUESTS 3

void ElectThink(void);
void AbortElect(void);

// admin.c

// admin flags
#define AF_ADMIN       (1<<0) // elected admin
#define AF_REAL_ADMIN  (1<<1) // pass/vip granted admin (real admin in terms of ktpro)

qbool is_real_adm(gedict_t *p); // is pass/vip granted admin (real admin in terms of ktpro)
qbool is_adm(gedict_t *p);      // is elected admin (admin rigths granted by /elect command)

void KickThink(void);
void ExitKick(gedict_t *kicker);

void BecomeAdmin(gedict_t *p, int adm_flags);
void VoteAdmin(void);

void PlayerStopFire(gedict_t *p);
void do_force_spec(gedict_t *p, gedict_t *admin, qbool spec);

// arena.c

void ra_init_que(void);
gedict_t* ra_que_first(void);
void ra_in_que(gedict_t *p);
void ra_out_que(gedict_t *p);
qbool ra_isin_que(gedict_t *p);
int ra_pos_que(gedict_t *p);
qbool isRA(void); // not game mode, but just modificator of duel
qbool isWinner(gedict_t *p);
qbool isLoser(gedict_t *p);
gedict_t* getWinner(void);
gedict_t* getLoser(void);
void ra_ClientDisconnect(void);
void ra_ClientObituary(gedict_t *targ, gedict_t *attacker);
void ra_PutClientInServer(void);
void RocketArenaPre(void);
qbool readytostart(void);
void ra_Frame(void);
void setfullwep(gedict_t *anent);
void setnowep(gedict_t *anent);

// { ra commands
void ra_PlayerStats(void);
void ra_PrintPos(void);
void ra_break(void);
// }

// clan_arena.c

qbool isCA(void);
qbool CA_CheckAlive(gedict_t *p);
int CA_wins_required(void);
int CA_count_ready_players(void);
int CA_get_score_1(void);
int CA_get_score_2(void);
void SM_PrepareCA(void);
void CA_ClientObituary(gedict_t *targ, gedict_t *attacker);
void CA_MatchBreak(void);
void CA_PrintScores(void);
void CA_TeamsStats(void);
void CA_player_pre_think(void);
void CA_spectator_think(void);
void CA_Frame(void);
void CA_PutClientInServer(void);
void CA_AddLatePlayer(gedict_t *p, char *team);
qbool CA_can_fire(gedict_t *p);

// captain.c

int capt_num(gedict_t *p);

// coach.c

int coach_num(gedict_t *p);
qbool is_coach(gedict_t *p);
void ExitCoach(void);

// maps.c

void StuffMaps(gedict_t *p);

void GetMapList(void);

char* GetMapName(int imp);
int GetMapNum(char *mapname);
void DoSelectMap(int iMap);
void SelectMap(void);
void VoteMap(void);
qbool VoteMapSpecific(char *map);

// match.c

void EndMatch(float skip_log);
void StatsToFile(void);

// grapple.c
void GrappleThrow(void);
void GrappleService(void);
void GrappleReset(gedict_t *rhook);
void CancelHook(gedict_t *owner);
float IncreasePullSpeed(float speed, float incr);
float DecreasePullSpeed(float speed, float decr);

// hoonymode.c

#define HM_PT_FINAL 1
#define HM_PT_SET 2

qbool isHoonyModeDuel(void);
qbool isHoonyModeAny(void);
qbool isHoonyModeTDM(void);

gedict_t* HM_choose_spawn_point(gedict_t *player);
void HM_log_spawn_point(gedict_t *player, gedict_t *spawn);

void HM_draw(void);
void HM_suicide(gedict_t *player);
void HM_next_point(gedict_t *won, gedict_t *lost);
void HM_all_ready(void);
void HM_reset_map(void);
void HM_initialise_rounds(void);
void HM_rounds_adjust(int change);

int HM_current_point_type(void);
int HM_current_point(void);
int HM_rounds(void);
int HM_timelimit(void);
char* HM_lastscores_extra(void);
qbool HM_is_game_over(void);
void HM_name_map_spawn(gedict_t *spawn);
void HM_pick_spawn(void);
void HM_unpick_all_spawns(void);
void HM_roundsup(void);
void HM_roundsdown(void);
void HM_point_stats(void);
void HM_restore_spawns(void);
void HM_match_break(void);
const char* HM_round_explanation(void);
const char* HM_series_explanation(void);

const char* HM_round_results(gedict_t *player);

// race.c

typedef struct race_stats_score_s
{
	char *name;
	int wins;
	int score;
	int completions;
	float best_time;
	float total_time;
	float total_distance;
} race_stats_score_t;

qbool isRACE(void);
void apply_race_settings(void);
void ToggleRace(void);
void RaceCountdownChange(float t);
void StartDemoRecord(void);

qbool race_weapon_allowed(gedict_t *p);

void race_init(void);
void race_shutdown(char *msg);
void race_think(void);

void race_add_standard_routes(void);

void race_set_one_player_movetype_and_etc(gedict_t *p);

gedict_t* race_get_racer(void);

void setwepnone(gedict_t *p);
void setwepall(gedict_t *p);
qbool race_handle_event(gedict_t *player, gedict_t *object, const char *eventName);
void race_player_pre_think(void);
void race_player_post_think(void);
int race_count_votes_req(float percentage);
qbool race_allow_map_vote(gedict_t *player);

gedict_t* race_find_race_participants(gedict_t *p);

qbool race_match_mode(void);
char* race_scoring_system_name(void);
void race_match_stats(void);
race_stats_score_t* race_get_player_stats(int *players);

// globals.c

extern int k_bloodfest;      // blood fest mode
extern float k_killquad;       // killquad mode
extern float framechecks;      // if timedemo/uptime bugs are tolerated
extern float k_attendees;      // stores number of players on server - used in 'ready' checking
extern float k_captains;       // number of captains
extern float k_captainturn;    // which captain comes in line to pick
extern float k_coaches;        // number of coaches
extern float k_checkx;
extern float k_force;          // used in forcing matchstart
extern float k_maxspeed;       // used to store server maxspeed to allow switching by admins
extern float k_oldmaxspeed;    // used to store old value of maxspeed prior to freezing the map
extern float k_showscores;     // whether or not should print the scores or not
extern float k_nochange; // used to indicate if frags changes somehow since last time 'scores' command was called
extern float k_standby;        // if server is in standy mode
extern float k_sudden_death;   // to mark if sudden death overtime is currently the case
extern float k_teamid;
extern float k_userid;
extern float k_whonottime;     // NOT_SURE:
extern float match_in_progress;     // if a match has begun
extern float match_start_time; // time when match has been started
extern float match_end_time;   // time when match is expected to end
extern float match_over; // boolean - whether or not the match stats have been printed at end of game
extern int k_berzerk;          // berzerk mode
extern float k_berzerktime;    // berzerk mode time left
extern gedict_t *newcomer;     // stores last player who joined
extern int k_overtime;       // is overtime is going on

extern float current_maxfps;	// current value of maxfps

// { yawn mode
extern int k_yawnmode;		// is server in yawn mode
extern int k_fallbunny_cap;	// fallbunny cap procent in yawn mode
extern int k_teleport_cap;	// cap for keeping velocity through tele
// }

extern int k_practice;		// is server in practice mode
extern int k_matchLess;	    // is server in matchLess mode
extern int k_matchLess_idle_time;
extern int k_matchLess_idle_warn;

extern gameType_t k_mode;   // game type: DUEL, TP, FFA
extern int k_lastvotedmap;	// last voted map, used for agree command?
// { CTF
extern int k_ctf_custom_models;	// use or not custom models
extern int k_allowed_free_modes; // reflect appropriate cvar - but changed only at map load
#ifdef CTF_RELOADMAP
extern qbool k_ctf;			// is ctf was active at map load
#endif
// }

// { cmd flood protection

extern int k_cmd_fp_count;    // 10 commands allowed ..
extern float k_cmd_fp_per;      // per 4 seconds
extern float k_cmd_fp_for;      // skip commands for 5 seconds
extern int k_cmd_fp_kick;     // kick after 4 warings
extern int k_cmd_fp_dontkick; // if 1 - don't use kick
extern int k_cmd_fp_disabled; // if 1 - don't use cmd floodprot

// }

extern float k_sv_locktime; // some time before non VIP players can't connect, spectators not affected

extern qbool vw_available; // vwep extension available
extern qbool vw_enabled; // vweps enabled

// { rocket arena

extern float time_to_start;	//time to start match
extern int ra_match_fight;	// have winner and loser fighting

// }

// { clan arena

extern int ca_round_pause;

// }

extern int jumpf_flag; // falling velocity criteria

extern float f_check; // is we in state of some f_xxx check

extern float lastTeamLocationTime; // next udate for CheckTeamStatus()

extern qbool first_rl_taken; // true when some one alredy took rl

extern int sv_minping; // used to broadcast changes

// { SP

#define FRAMETIME 0.1

// ANIM() macro idea I got from Tonik
#define ANIM(name, _frame, _next)		\
void name (void) {				\
	self->s.v.frame = _frame;				\
	self->s.v.nextthink = g_globalvars.time + FRAMETIME;	\
	self->think = ( func_t ) _next; }

// sp_client.c
void ExitIntermission(void);
char* ObituaryForMonster(char *attacker_class);

// sp_dog.c
float DogCheckAttack(void);

// sp_demon.c
float DemonCheckAttack(void);

// sp_ogre.c
float OgreCheckAttack(void);

// sp_shambler.c
float ShamCheckAttack(void);

// sp_soldier.c
float SoldierCheckAttack(void);

// sp_wizard.c
float WizardCheckAttack(void);

// sp_ai.c
// FIXME: make them static or get rid of globals...
extern float enemy_vis, enemy_infront, enemy_range;
extern float enemy_yaw;

void FoundTarget(void);

void GetMadAtAttacker(gedict_t *attacker);

void SUB_AttackFinished(float normal);
void SUB_CheckRefire(func_t thinkst);

void ai_stand(void);
void ai_walk(float dist);
void ai_run(float dist);
void ai_pain(float dist);
void ai_face(void);
void ai_charge(float d);
void ai_charge_side(void);
void ai_forward(float dist);
void ai_back(float dist);
void ai_turn(void);
void ai_painforward(float dist);
void ai_melee(void);
void ai_melee_side(void);

// sp_monsters.c
typedef struct bloodfest_s
{
	float monsters_spawn_time;			// is it time to start monsters spawn wave.
	int monsters_to_spawn;			// amount of monsters we want to spawn in this wave.
	qbool spawn_boss;					// spawn boss in this wave.
} bloodfest_t;

extern bloodfest_t g_bloodfest;

// reset bloodfest runtime variables to default values.
void bloodfest_reset(void);

void monster_death_use(void);

void check_monsters_respawn(void);

void walkmonster_start(char *model);
void flymonster_start(char *model);
void swimmonster_start(char *model);

// }

// misc.c
void LaunchLaser(vec3_t org, vec3_t vec);

// identify alternative .ent files by format <map>#<name>.ent
#define K_ENTITYFILE_SEPARATOR '#'

// bots
qbool bots_enabled(void);

// files
fileHandle_t std_fropen(const char *fmt, ...) PRINTF_FUNC(1);
fileHandle_t std_fwopen(const char *fmt, ...) PRINTF_FUNC(1);
int std_fgetc(fileHandle_t handle);
char* std_fgets(fileHandle_t handle, char *buf, int limit);
void std_fclose(fileHandle_t handle);
void std_fprintf(fileHandle_t handle, const char *fmt, ...) PRINTF_FUNC(2);

// teamplay
void TeamplayEventItemTaken(gedict_t *client, gedict_t *item);
void TeamplayDeathEvent(gedict_t *client);
void TeamplayMessage(void);
qbool TeamplayMessageByName(gedict_t *client, const char *message);
void TeamplayGameTick(void);
void LocationInitialise(void);
qbool SameTeam(gedict_t *p1, gedict_t *p2);

#ifndef BOT_SUPPORT
#define bots_enabled() (false)
#else
#include "fb_globals.h"
#endif

#define LGCMODE_VARIABLE "k_lgcmode"

qbool lgc_enabled(void);
void lgc_register_fire_stop(gedict_t *player);
void lgc_register_kill(gedict_t *player);
void lgc_register_miss(vec3_t start, gedict_t *player);
void lgc_register_hit(vec3_t start, gedict_t *player, gedict_t *victim);

#define TOT_MODE_VARIABLE "k_tot_mode"

qbool tot_mode_enabled(void);

// private games
qbool is_private_game(void);
qbool is_logged_in(gedict_t *p);
void private_game_toggle(qbool enable);
void private_game_vote(void);
void vote_check_privategame(void);
qbool private_game_voteable(void);
qbool private_game_by_default(void);

// set when match is starting, otherwise a normal spawn
extern qbool initial_match_spawns;
// highest number of players in all teams, used to allow (re)connect during match
extern int maxPlayerCount;

#define AUTOTRACK_POWERUPS_PREDICT_TIME 2

qbool AllowMonster(gedict_t *e);
