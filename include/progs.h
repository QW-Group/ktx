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

#include "progdefs.h"

#define MAX_ROUTE_NODES		20 // max race checkpoints per race (including start and finish checkpoints)
#define MAX_ROUTES			20 // max race route per map

#define LGCMODE_MAX_DISTANCE 700
#define LGCMODE_DISTANCE_BUCKETS 20
#define LGCMODE_BUCKET_DISTANCE  (LGCMODE_MAX_DISTANCE / LGCMODE_DISTANCE_BUCKETS)

typedef struct shared_edict_s
{
	entvars_t v;	// C exported fields from progs
// other fields from progs come immediately after
} edict_t;

struct gedict_s;
typedef void (*th_die_funcref_t)(void);
typedef void (*th_pain_funcref_t)(struct gedict_s*, float);

// { SP
typedef void (*th_sp_funcref_t)(void);
// }

typedef enum
{
	ctNone = 0,
	ctPlayer,
	ctSpec
} clientType_t;

typedef enum
{
	wpNONE = 0,
	wpAXE,
	wpSG,
	wpSSG,
	wpNG,
	wpSNG,
	wpGL,
	wpRL,
	wpLG,
	wpMAX
} weaponName_t;

typedef enum
{
	lgcUndershaft = 0,
	lgcNormal = 1,
	lgcOvershaft = 2
} lgc_state_t;

typedef struct wpType_s
{
	int hits;			// hits with this weapon, for SG and SSG this is count of bullets
	int rhits;			// real hits for this weapon (direct + splash), used for RL and GL only
	int vhits;			// virtual hits for this weapon (direct + splash, do not care about pent and such), used for RL and GL only
	int attacks;		// all attacks with this weapon, for SG and SSG this is count of bullets

	int kills;			// kills with this weapon
	int deaths;			// deaths from this weapon
	int tkills;			// team kills with this weapon
	int suicides;		// suicides with this weapon

	int ekills;			// killed enemies which contain this weapon in inventory
	int drops;			// number of packs dropped which contain this weapon
	int tooks;			// took this weapon and doesn't have this weapon before took (weapon from packs counted too)
	int stooks;			// spawned items taken (backpacks not included), and didn't have weapon beforehand
	int ttooks;			// total taken, even if you already had this weapon
	int sttooks;		// spawned items taken (backpacks not included), even if you already had this weapon

	int edamage;		// damage to enemies
	int tdamage;		// damage to team-mates

	int enemyjustkilled;
	int lastfraghits;			// count of hits for lg per frag
	int lastfragattacks;		// count of attacks for lg per frag
	int lastfragdisplayhits;	// stores count of last frag hits
	int lastfragdisplayattacks;	// stores count of last frag attacks

	float time;			// total time u have some weapon
} wpType_t;

typedef enum
{
	itNONE = 0,
	itHEALTH_15,
	itHEALTH_25,
	itHEALTH_100,
	itGA,
	itYA,
	itRA,
	itQUAD,
	itPENT,
	itRING,
	itMAX
} itemName_t;

typedef struct itType_s
{
	int tooks;	// taken count
	float time;	// total time u have some item
} itType_t;

// store player statistic here, like taken armors etc...
typedef struct player_stats_s
{
	wpType_t wpn[wpMAX];
	itType_t itm[itMAX];

	float dmg_t;			// damage taken
	float dmg_g;			// damage given
	float dmg_g_rl;			// virtual given rl damage
	float dmg_team;			// damage to team
	float dmg_self;			// damage to own player
	float dmg_eweapon;		// damage to enemy weapons
	float dmg_tweapon;		// damage to team weapons
// { k_dmgfrags
	float dmg_frags;		// frags awarded from damage (CA)
// }

	int ot_a;				// overtime armor value
//	float	ot_at;			// overtime armor type
	int ot_items;			// overtime items
	int ot_h;				// overtime health

	int spawn_frags;
	int handicap;
	int transferred_RLpacks;
	int transferred_LGpacks;

	// ctf stats
	int ctf_points;			// use frags - this to calculate efficiency for ctf
	int caps;				// flag captures
	int f_defends;			// flag defends
	int c_defends;			// carrier defends
	int c_frags;			// frags on enemy carriers
	int pickups;			// flag pickups
	int returns;			// flag returns
	float res_time;			// time with resistance rune
	float str_time;			// time with strength rune
	float hst_time;			// time with haste rune
	float rgn_time;			// time with regen rune

	// midair stats
	int mid_stomps;			// stomps done on other players
	int mid_bronze;			// bronze midairs
	int mid_silver;			// silver midairs
	int mid_gold;			// gold midairs
	int mid_platinum;		// platinum midairs
	int mid_total;			// total of midairs
	int mid_bonus;			// total of bonuses
	float mid_totalheight;	// maximum height of midairs
	float mid_maxheight;	// maximum height of midairs
	float mid_avgheight;	// average height of midairs

	// spree stats
	int spree_current;		// number of frags since our last death
	int spree_current_q;	// number of frags in current quad run
	int spree_max;			// largest spree throughout game
	int spree_max_q;		// largest quad spree throughout game

	// time stats
	float control_time;		// time spent in control

	// rocket arena
	int wins;				//number of wins they have
	int loses;				//number of loses they have

	// velocity stats
	float velocity_max;		// maximum velocity
	float velocity_sum;		// summary velocity from all frames
	int vel_frames;			// number of frames

	// instagib stats
	int i_height;			// Cumulated height  of airgibs
	int i_maxheight;
	int i_cggibs;			// Kills with coil gun
	int i_axegibs;			// Kills with axe
	int i_stompgibs;		// Kills with stomp
	int i_multigibs;		//
	int i_airgibs;			// Number of airgibs
	int i_maxmultigibs;		//
	int i_rings;			//

	// lgc stats
	int lgc_undershaft;		// cells fired before hitting target
	int lgc_overshaft;		// cells fired after killing target

} player_stats_t;

typedef enum
{
	etNone = 0,
	etCaptain,
	etCoach,
	etAdmin
} electType_t;

// store player votes here
typedef struct vote_s
{
	int brk;
	int elect;
	int map;
	int pickup;
	int rpickup;
	int teamoverlay;
	int nospecs;
	int coop;
	int hooksmooth;
	int hookfast;
	int hookclassic;
	int hookcrhook;
	int antilag;
	int privategame;
	//int kick_unauthed;
	int swapall;

	electType_t elect_type;	// election type
	float elect_block_till;	// block election for this time
} vote_t;

// demo marker
typedef struct demo_marker_s
{
	float time;
	char markername[64];
} demo_marker_t;

// cmd flood protection

#define MAX_FP_CMDS (10)

typedef struct fp_cmd_s
{
	float locked;
	float cmd_time[MAX_FP_CMDS];
	int last_cmd;
	int warnings;
} fp_cmd_t;

// store wregs here

#define MAX_WREGS		(256)
#define MAX_WREG_IMP	(9)

typedef struct wreg_s
{
	qbool init;
	int attack;
	int impulse[MAX_WREG_IMP];
} wreg_t;

// {

#define MAX_PLRFRMS (77*15) /* 77 frames for each of 15 seconds */

typedef struct
{
	float time;
//	float	modelindex;
	vec3_t origin;
	vec3_t angles;
	float frame;
	float effects;
//	vec3_t	v_angle;
	float colormap;
} plrfrm_t;

// }

// player position
#define MAX_POSITIONS (5)

typedef struct pos_s
{
	vec3_t velocity;
	vec3_t origin;
	vec3_t v_angle;
} pos_t;

typedef enum
{
	atNone = 0,
	atBest,
	atPow,
	atKTPRO
} autoTrackType_t;

// { rocket arena
typedef enum
{
	raNone = 0,
	raWinner,
	raLoser,
	raQue
} raPlayerType_t;
// }

typedef struct teamplay_memory_s
{
	unsigned long item;			// classname of saved item
	vec3_t location;			// location of item
	float time;					// time details were saved
	int flags;					// flags about the particular item
} teamplay_memory_t;

typedef struct teamplay_preferences_s
{
	char need_weapons[10];	// weapons (ezQuake: <var>)
	int need_health;		// threshold for announcing player needs health
} teamplay_preferences_t;

typedef struct teamplay_s
{
	teamplay_memory_t took;
	teamplay_memory_t point;

	unsigned long enemy_items;	// powerup flags, updated when enemy powerup (or eyes) visible
	float enemy_itemtime;		// time when enemy_items was last set
	vec3_t enemy_location;		// location of player when enemy_items was last set
	int enemy_count;			// number of enemies in pvs (also eyes)
	int teammate_count;			// number of teammates in pvs

	// Last location (FIXME: last_death_location)
	float death_time;
	vec3_t death_location;
	int death_items;
	int death_weapon;

	// Client's preferences
	teamplay_preferences_t preferences;
} teamplay_t;

#define NEED_WEAPONS_DEFAULT "87"

#define it_quad		(1 << 0)
#define it_pent		(1 << 1)
#define it_ring		(1 << 2)
#define it_suit		(1 << 3)
#define it_ra		(1 << 4)
#define it_ya		(1 << 5)
#define it_ga		(1 << 6)
#define it_mh		(1 << 7)
#define it_health	(1 << 8)
#define it_lg		(1 << 9)
#define it_rl		(1 << 10)
#define it_gl		(1 << 11)
#define it_sng		(1 << 12)
#define it_ng		(1 << 13)
#define it_ssg		(1 << 14)
#define it_pack		(1 << 15)
#define it_cells	(1 << 16)
#define it_rockets	(1 << 17)
#define it_nails	(1 << 18)
#define it_shells	(1 << 19)
#define it_flag		(1 << 20)
#define it_teammate	(1 << 21)
#define it_enemy	(1 << 22)
#define it_eyes		(1 << 23)
#define it_sentry	(1 << 24)
#define it_disp		(1 << 25)
#define it_quaded	(1 << 26)
#define it_pented	(1 << 27)
#define it_rune1	(1 << 28)
#define it_rune2	(1 << 29)
#define it_rune3	(1 << 30)
#define it_rune4	((unsigned int) (1 << 31))
#define NUM_ITEMFLAGS 32

#define it_runes	(it_rune1|it_rune2|it_rune3|it_rune4)
#define it_powerups	(it_quad|it_pent|it_ring|it_suit)
#define it_weapons	(it_lg|it_rl|it_gl|it_sng|it_ssg)
#define it_armor	(it_ra|it_ya|it_ga)
#define it_ammo		(it_cells|it_rockets|it_nails|it_shells)
#define it_players	(it_teammate|it_enemy|it_eyes)

#define MAX_SPAWN_WEIGHTS (64)
#define HM_MAX_ROUNDS 64

#ifdef BOT_SUPPORT
// frogbots
typedef void (*fb_void_funcref_t)(void);
typedef qbool (*fb_bool_funcref_t)(void);
typedef float (*fb_desire_funcref_t)(struct gedict_s* self, struct gedict_s* other);
typedef qbool (*fb_touch_funcref_t)(struct gedict_s* item, struct gedict_s* player);
typedef void (*fb_taken_funcref_t)(struct gedict_s* item, struct gedict_s* player);
typedef void (*fb_entity_funcref_t)(struct gedict_s* item);

#ifndef NUMBER_MARKERS
#define NUMBER_MARKERS	300
#endif
#ifndef NUMBER_GOALS
#define NUMBER_GOALS	24
#endif
#ifndef NUMBER_ZONES
#define NUMBER_ZONES	24
#endif
#ifndef NUMBER_PATHS
#define NUMBER_PATHS	8
#endif
#ifndef NUMBER_SUBZONES
#define NUMBER_SUBZONES	32
#endif

typedef struct fb_runaway_route_s {
	struct gedict_s* next_marker;
	struct gedict_s* prev_marker;
	float time;
	float score;
} fb_runaway_route_t;

typedef struct fb_path_s {
	struct gedict_s* next_marker;	// next marker in the graph
	float time;						// time to travel if walking (0 if teleporting)
	float rj_time;					// time to travel if using rocket jump
	int flags;						// hints on how to travel to next marker

	short angle_hint;				// When travelling to marker, offset to standard angle (+ = anti-clockwise)
	float rj_pitch;					// ideal pitch angle when rocket jumping
	float rj_yaw;					// ideal yaw angle when rocket jumping (0 for no change)
	int rj_delay;					// number of frames to delay between jumping and firing rocket
} fb_path_t;

typedef struct fb_goal_s {
	struct gedict_s* next_marker;
	float time;
	struct gedict_s* next_marker_rj;
	float rj_time;
} fb_goal_t;

typedef struct fb_subzone_s {
	struct gedict_s* next_marker;
	float time;
	struct gedict_s* next_marker_rj;
	float rj_time;
} fb_subzone_t;

typedef struct fb_zone_s {
	// Standard routing
	struct gedict_s* marker;
	float time;
	struct gedict_s* next;
	struct gedict_s* next_zone;

	// Rocket jumping
	struct gedict_s* marker_rj;
	float rj_time;
	struct gedict_s* next_rj;

	float reverse_time;
	struct gedict_s* reverse_marker;
	struct gedict_s* reverse_next;
	float from_time;
	struct gedict_s* sight_from;
	float sight_from_time;
	struct gedict_s* higher_sight_from;
	float higher_sight_from_time;		// FIXME: Never used?  Was used in runaway code, but in peculiar fashion...
	int task;
} fb_zone_t;

typedef struct fb_botaim_s {
	float scale;		// difference between current viewangle and desired is scaled by this
	float minimum;		// minimum & maximum final variation
	float maximum;
	float multiplier;	// alter
} fb_botaim_t;

typedef struct fb_botskill_s {
	int   skill_level;						// 0-20 as standard
	float dodge_amount;						// left/right strafing
	float lookahead_time;					// how far ahead the bot can think (regarding items respawning etc) 5-30s in original
	float prediction_error;					// affects goal travellling error (lower values => turn up earlier) 1-0 in original.  randomised.
	float look_anywhere;					// 0...1  determines when the bot will look at the enemy's location
	float accuracy;

	float lg_preference;					// 0...1  previously game-wide, look to use LG when possible
	float rl_preference;					// 0...1  previously game-wide, look to use RL when possible

	float visibility;						// cos(fov / 2) ... fov 90 = cos(45) = 0.7071067, fov 120 = cos(60) = 0.5

	qbool attack_respawns;					// fire at respawns if enemy just died

	float min_volatility;
	float max_volatility;
	float reduce_volatility;
	float ownspeed_volatility_threshold;
	float ownspeed_volatility;
	float enemyspeed_volatility_threshold;
	float enemyspeed_volatility;
	float enemydirection_volatility;
	float pain_volatility;					// when bot has taken damage in last second
	float opponent_midair_volatility;		// when opponent is in midair
	float self_midair_volatility;			// when bot is in midair

	float initial_volatility;				// when bot sees player for first time

	float current_volatility;
	float awareness_delay;
	float spawn_move_delay;
	float movement_estimate_error;			// % of time the bot gets wrong when predicting player location

	fb_botaim_t aim_params[2];

	float movement;
	float combat_jump_chance;
	float missile_dodge_time;				// minimum time in seconds before bot dodges missile

	qbool customised;						// if set, customised file

	qbool wiggle_run_dmm4;					// if set, wiggle run on dmm4 (and up)
	int wiggle_run_limit;					// number of frames until bot will switch strafe direction
	float wiggle_toggle;					// % chance of switching direction when being hit
} fb_botskill_t;

// FIXME: Need to break this up into marker fields, client fields and entity fields
//        Currently using way too much memory as a lot of these are invalid for particular entity types
typedef struct fb_entvars_s {
	fb_zone_t zones[NUMBER_ZONES];				// directions to zones
	fb_subzone_t subzones[NUMBER_SUBZONES];		// links to subzones (subzone is unique marker inside a zone)
	fb_goal_t goals[NUMBER_GOALS];				// links to goals
	fb_runaway_route_t runaway[NUMBER_PATHS];	// routes when running away
	fb_path_t paths[NUMBER_PATHS];				// direct links from this marker to next

	int path_state;								// flags for next path, copied from routing definition
	int angle_hint;								// for curl-jumping, angle offset (right-handed, +ve = to left, -ve = to right)

	int index;									// marker number

	float wait;
	float fl_ontrain;							// FIXME: never set (used for frogbot train movement)

	struct gedict_s* touchPlayer;				// last player to touch this object (see below)
	float touchPlayerTime;						// how long the current item will be considered touched by touchPlayer
	int   teamflag;								// This is used to add a teamflag to a goal entity, so it bots on same team ignore item

	float oldwaterlevel;						// used to detect FL_WATERJUMP waterjump...  may not be required?  server will set...
	float oldwatertype;							// FIXME: never read.  server will set, this is in MOVETYPE_STEP code...

	// these determine the strength of each player
	float total_armor;
	float total_damage;
	float firepower;

	float enemy_time;							// Time before bot re-evaluates who is its primary enemy
	float enemy_dist;							// Distance to primary enemy

	int oldsolid;								// need to keep track of this for hazard calculations

	// these determine the desire for items for each player 
	//   (not just for bots ... bot's desire can take enemy's desire into consideration)
	fb_desire_funcref_t desire;
	float desire_armor1;
	float desire_armor2;
	float desire_armorInv;
	float desire_health0;
	float desire_mega_health;
	float desire_supershotgun;
	float desire_nailgun;
	float desire_supernailgun;
	float desire_grenadelauncher;
	float desire_rocketlauncher;
	float desire_lightning;
	float desire_rockets;
	float desire_cells;
	float desire_nails;
	float desire_shells;

	int state;									// WAIT | RUNAWAY | NOTARGET_ENEMY | HELP_TEAMMATE | STATE_BOTTOM (doors) | SHOT_FOR_LUCK
	int camp_state;								// CAMPBOT (FIXME: values set, but read value never acted on)
	float arrow;
	qbool wasinwater;
	float swim_arrow;
	float arrow_time;							// If set in future, bots will avoid this path.  Used to detect grenade/rocket at teleport exit.
	struct gedict_s* arrow_time_setby;			// Which player set the most recent arrow time?
	float arrow_time2;							// If set in future, sustain current direction (used to avoid edges or back away from objects)
	float linked_marker_time;

	vec3_t oldvelocity;
	vec3_t obstruction_normal;
	vec3_t obstruction_direction;				// Instead of storing in rel_pos, store direction to obstruction here (for BotWaterMove)
	qbool  avoiding;							// Avoiding next path marker, due to incoming rocket or arrow_time in future

	qbool fl_marker;							// true if the current item is considered a marker (used when finding all objects in given radius)
	//struct gedict_s* next;
	
	// Goal evaluation
	struct gedict_s* best_goal;
	float best_goal_score;
	float saved_goal_desire;					// the desire for the current goal entity
	float saved_respawn_time;					// seconds until this item respawns (includes current bot's error)
	float saved_goal_time;
	float saved_enemy_time_squared;
	float goal_respawn_time;					// the time when this->best_goal2 will respawn (can be in past)
	float goal_refresh_time;
	float goal_enemy_repel;
	float goal_enemy_desire;
	struct gedict_s* best_goal2;
	float best_score2;
	float best_goal_time;

	struct gedict_s* linked_marker;				// the next path in the route to goalentity
	struct gedict_s* old_linked_marker;			// the previous linked marker
	struct gedict_s* look_object;				// the player/marker/entity that the bot is locked onto
	float frogbot_nextthink;					// when to next run periodic movement logic for this player

	int T;										// flags for this individual marker
	int G_;										// assigned goal number for this marker [1-based...]
	int Z_;										// assigned zone for this marker
	int S_;										// subzone for this marker

	vec3_t fixed_size;							// fixed dimensions for this marker.  if dimension is 0, default marker size used

	struct gedict_s* virtual_goal;

	struct gedict_s* zone_stack_next;
	struct gedict_s* Z_head;
	struct gedict_s* Z_next;

	float path_normal_;

	fb_bool_funcref_t pickup;					// return true if a player would pickup an item as they touch it
	float weapon_refresh_time;

	struct gedict_s* prev_touch_marker;			// the last touch marker processed
	struct gedict_s* touch_marker;				// the last marker touched, while we were expecting to touch something (ignored during rocket jumps)
	float touch_distance;						// distance from player to touch marker.  used when multiple touch events fired in same frame
	float touch_marker_time;					// if < time, run a brute force closest-marker search for marker the player is closest to

	// These settings dictate the 'skill' of the bot
	fb_botskill_t skill;

	// These control the bot's next command
	qbool firing;								// does the bot want to attack this frame?
	qbool jumping;								// does the bot want to jump this frame?
	int desired_weapon_impulse;					// impulse to send the next time the player
	int random_desired_weapon_impulse;
	vec3_t desired_angle;						// for 'perfect' aim, this is where the bot wants to be aiming
	qbool botchose;								// next_impulse is valid
	int next_impulse;							// the impulse to send in next command

	vec3_t virtual_mins;						// true bounds of the object (items are markers, so size is boosted)
	vec3_t virtual_maxs;						// true bounds of the object

	vec3_t dir_move_;							// the direction the bot wants to move in
	float dir_speed;							// the magnitude of vector the bot wants to move in
	int wiggle_run_dir;							// when wiggle-running, going left or right
	qbool wiggle_increasing;					// dictates direction, positive or negative
	vec3_t last_cmd_direction;					// the direction the bot did move in (scaled for speed)
	float ledge_backup_time;

	// Bot's missile parameters
	struct gedict_s* missile_dodge;				// rocket belonging to look_object

	int tread_water_count;						// number of frames spent treading water

	vec3_t predict_origin;						// origin of enemy, or where the bot thinks they will land
	qbool predict_shoot;						// make a prediction shot this frame?

	// frogbot logic (move out of entity)
	qbool allowedMakeNoise;						// if false, paths involving picking up an item are penalised

	// Rocket jumping logic
	qbool canRocketJump;						// will consider rocket jump routes
	qbool rocketJumping;						// in middle of rocket jumping
	int rocketJumpPathFrameDelay;				// value rocketJumpPathFrameDelay
	int rocketJumpFrameDelay;					// active delay between jumping and firing
	int rocketJumpAngles[2];					// pitch/yaw for rocket jump angle
	int lavaJumpState;							// keep track of submerge/rise/fire sequence

	// Editor
	int last_jump_frame;						// framecount when player last jumped.  used to help setting rj fields

	qbool bot_evade;							//

	float help_teammate_time;
	float frogwatermove_time;
	float up_finished;							// Swimming, used to keep pushing up for a period during lavajump
	int botnumber;								// bots[botnumber]

	float last_cmd_sent;
	struct gedict_s* prev_look_object;			// stores whatever the bot was looking at last frame

	// Item event functions
	fb_touch_funcref_t item_touch;				// called whenever an item is touched
	fb_taken_funcref_t item_taken;				// called whenever an item is taken
	fb_taken_funcref_t item_affect;				// called whenever an item affects a player (mega-health)
	fb_entity_funcref_t item_respawned;			// called whenever an item respawns
	fb_entity_funcref_t item_placed;			// called when item has been placed in the map

	// Player event functions
	fb_entity_funcref_t ammo_used;				// Whenever ammo is updated

	qbool be_quiet;
	qbool enemy_visible;
	float last_death;							// Last time this player died

	struct gedict_s* virtual_enemy;
	vec3_t rocket_endpos;						// where an incoming rocket will explode.

	// Debugging
	qbool debug;								// If set, trace logic
	qbool debug_path;							// Set by "debug botpath" command
	qbool debug_path_rj;						// Set by "debug botpath" command: can rocket jump
	float debug_path_start;						// Set by "debug botpath" command: time debug started
	struct gedict_s* fixed_goal;				// Set by "debug botpath" command: target goal entity to move to
	vec3_t prev_velocity;						// used by "debug botpath" command: keep track of acceleration
	float last_spec_cp;							// last spectator centerprint

	// Navigation
	struct gedict_s* door_entity;				// actual door entity (we spawn markers in doorway for navigation)

	// Teamplay
	float last_mm2_status;						// last time this bot reported
	float last_mm2_spot;						// last time this player had powerup reported by enemy
	float last_mm2_spot_attempt;				// last time this bot tried to report enemy powerup

	qbool waterjumping;							// true if the bot should waterjump
	int dbg_countdown;							// bot will be stationary for x frames

	// Aiming
	float last_rndaim[2];
	float last_rndaim_time;
	float min_fire_time;						// time before bot will fire
	float min_move_time;						// time before bot will move (used off spawn)

	// Stored on missile to detect where item will explode
	vec3_t missile_forward;
	vec3_t missile_right;
	float missile_spawntime;

	// last time this player was hurt
	float last_hurt;

	float cmd_msec_lost;
	int cmd_msec_last;
} fb_entvars_t;
#endif

//typedef (void(*)(gedict_t *)) one_edict_func;
typedef struct gedict_s
{
	edict_t s;

//fields that used to be pointers are now byte offsets to these pointers...
	string_t classname;
	string_t model;
	func_t touch;
	func_t use;
	func_t think;
	func_t blocked;
	string_t weaponmodel;
	string_t netname;
	string_t target;
	string_t targetname;
	string_t message;
	string_t noise;
	string_t noise1;
	string_t noise2;
	string_t noise3;

//custom fields
	int isMissile;							// treat some objects as missiles.

	float vw_index;
	float maxspeed, gravity;
	int isBot;
	float brokenankle;						// player can't jump for a while after falling and get damaged
	char *mdl;
	char *killtarget;
	int worldtype;							// 0=medieval 1=metal 2=base
	char *map;
//player
	int walkframe;

//
// quakeed fields
//
	int style;
	float speed;
	int state;
	int voided;
//items.c
	float healamount, healtype;
//ammo
	int aflag;
//
// doors, etc
//
	vec3_t movement;
	vec3_t dest, dest1, dest2;
	vec3_t pos1, pos2;
	vec3_t mangle;
	float t_length, t_width, height;
	float wait;								// time from firing to restarting
	float delay;							// time from activation to firing
	struct gedict_s *trigger_field;			// door's trigger entity
	struct gedict_s *movetarget;
	float pausetime;
	char *noise4;
	float count;
//
// misc
//
	float cnt;								// misc flag

	void (*think1)(void);						//calcmove
	vec3_t finaldest;
//combat
	float dmg;
	float lip;
	float attack_finished;
	float pain_finished;

	float invincible_finished;
	float invisible_finished;
	float super_damage_finished;
	float radsuit_finished;

	float invincible_time, invincible_sound;
	float invisible_time, invisible_sound;
	float super_time, super_sound;
	float rad_time;
	float fly_sound;

	float axhitme;
	float show_hostile;						// set to time+0.2 whenever a client fires a
	// weapon or takes damage.  Used to alert
	// monsters that otherwise would let the player go
	float jump_flag;						// player jump flag
	float swim_flag;						// player swimming sound flag
	float air_finished;						// when time > air_finished, start drowning
	float bubble_count;						// keeps track of the number of bubbles
	int deathtype;							// keeps track of how the player died
	float dmgtime;

	th_die_funcref_t th_die;
	th_pain_funcref_t th_pain;

// below is KTX fields

// { SP

	//
	// monster ai
	//
	th_sp_funcref_t th_stand;
	th_sp_funcref_t th_walk;
	th_sp_funcref_t th_run;
	th_sp_funcref_t th_missile;
	th_sp_funcref_t th_melee;

	th_sp_funcref_t th_respawn; 			// for nightmare mode

	struct gedict_s *oldenemy;				// mad at this player before taking damage

	float lefty;

	float search_time;
	float attack_state;

	float monster_desired_spawn_time;		// in nightmare mode monster desire respawn at this time after last death
	vec3_t oldangles;						// for nightmare skill, need remember monster angles before respawn again

// }

	float cnt2;								// NOT_SURE: cnt2 = seconds left?
	float dead_time;						// time at which player last died - used in autorespawn
	float spawn_time;						// time at which player last spawned,
											// NOTE: ->spawn_time also used in spawn() function, so we know when edict was spawned
	float connect_time;						// time at which player connect
	float deaths;							// number of times player died
	float efficiency;						// stores player efficiency rating
	float friendly;							// number of times player killed teammates
	float kills;							// number of times player killed enemy
	float suicides;							// number of times player killed themselves
	float ready;							// if a player is ready or not
	char *killer;							// name of player who last killed player
	char *victim;							// name of player last killed

	char *backpack_player_name;				// player who dropped the backpack
	char backpack_team_name[32 /*MAX_TEAM_NAME*/];	// team associated with the backpack

	struct gedict_s *k_lastspawn;			// NOT_SURE: spot at which player last spawned?
// { kick mode
	struct gedict_s *k_playertokick;		// player selected to be kicked
	float k_kicking;						// if player is in kick mode
// }
	float k_1spawn;							// NOT_SURE: used in kteams respawn code...
	float k_accepted;						// NOT_SURE:
	int k_added;							// NOT_SURE: stores the entered admin code?
	int k_adminc;							// number of digits of admin code still to enter
	int k_admin;							// if player is an admin, flags
	float k_adm_lasttime;					// store time of last attempt getting admin rights
	int k_captain;							// if player is team captain
	int k_coach;							// if player is team coach
	float k_flag;							// flagvalue to customise settings such as sounds/autoscreenshot
	float k_msgcount;						// NOT_SURE: last time mod printed a message to client?
	float k_picked;							// NOT_SURE:
	int k_pauseRequests;					// number of 'pause' commands the player executed during a match
	int k_stuff;							// if player has received stuffed aliases|commands
// k_stuff flags
#define STUFF_MAPS     (1<<0)
#define STUFF_COMMANDS (1<<1)

	float k_teamnumber;						// team the player is a member of, 1 = team1, 2 = team2
	float k_teamnum;						// NOT_SURE:

	float lastwepfired;

//	float maxspeed;							// Used to set Maxspeed on a player moved from old qw defs.qc

	float suicide_time;						// can't suicide sooner than this

// ILLEGALFPS[
	// Zibbo's frametime checking code
	float real_time;
	float uptimebugpolicy;

	float fAverageFrameTime;
	float fFrameCount;
	float fDisplayIllegalFPS;
	float fCurrentFrameTime;
	float fLowestFrameTime;
	float fHighestFrameTime;
	float fIllegalFPSWarnings;
// ILLEGALFPS]

	float shownick_time;					// used to force centerprint is off at desired time
	clientType_t ct;						// client type for client edicts
// { timing
	float k_lastPostThink;					// last time PlayerPostThink or SpectatorThink was called
	float k_timingWarnTime;					// time of last client is timing warning
	int k_timingTakedmg;					// .... so we can restore
	int k_timingSolid;						// .... so we can restore
	int k_timingMovetype;					// .... so we can restore
// }

// { stats
	int wp_stats;							// show +wp_stats or not
	float wp_stats_time;					// used to force centerprint is off at desired time
	int sc_stats;							// show +scores or not
	float sc_stats_time;					// used to force centerprint is off at desired time

	player_stats_t ps;						// store player statistic here, like taken armors etc...

	float control_start_time;				// time when player gained control
	float it_pickup_time[itMAX];			// total possession time of items
	float wp_pickup_time[wpMAX];			// total possession time of weapons
// }

// { mvd demo weapon stats
	int wpstats_mask;						// bit mask, like ( 1 << wpSG | 1 << wpLG ), tell us which weapon stats need to be sent to demo at some point
// }

	int need_clearCP;						// if this true, clear center print at certain cases

// { ghost stuff
	int ghost_slot;							// now ktx put ghost in players scoreboards in one of free slots - store this slot
	float ghost_dt;							// ghost drop time - time when player dropped
	int ghost_clr;							// color of dropped player
// }

	vote_t v;								// player votes stored here

// { spec stuff
	int favx[MAX_CLIENTS];					// here stored players number for appropriate favX_add/Xfav_go commands
	int fav[MAX_CLIENTS];					// here stored players number for fav_add/next_fav commands
	autoTrackType_t autotrack;				// is autotrack or auto_pow or ktpro autorack
	qbool apply_ktpro_autotrack;			// if we use ktpro's autotrack, that a hint to apply pov switch
	struct gedict_s *autotrack_hint;		// per spectator autotrack hint, this helps switch pov more precise
	struct gedict_s *wizard;				// for specs, link to the entity which represent wizard
	int last_goal;							// just store here self->s.v.goal from last spec frame, so we can catch pov switch

// }

	pos_t pos[MAX_POSITIONS];				// for player pos_save / pos_move
	float pos_move_time;					// for player pos_save / pos_move

// { CTF
	struct gedict_s *hook;					// grapple
	qbool on_hook;						// are we on the grapple?
	qbool hook_out;						// is the grapple in flight?
	int ctf_flag;						// do we have a rune or flag?
	float regen_time;					// time to update health if regen rune
	float rune_sound_time;					// dont spam rune sounds (1 per second)
	float carrier_frag_time;				// used for carrier assists
	float return_flag_time;					// used for flag return assists
	float rune_notify_time;					// already have a rune spam prevention
	float carrier_hurt_time;				// time we last hurt enemy carrier
	float rune_pickup_time;					// time we picked up current rune
	float hook_damage_time;					// manage dps dealt to hooked enemies
	float hook_cancel_time;					// delay cancel on throw with smooth hook
	float hook_reset_time;					// marker for grapple reset (decoupled from `attack_finished`)
	float hook_pullspeed;					// accelerate grapple velocity
	float hook_pullspeed_accel;				// number by which to increment velocity
	char *last_rune;					// name of last rune we send to client
	float items2;						// using  ZQ_ITEMS2 extension in mvdsv we can use per client sigils for runes

//
// TF -- well, we does not support TF but require it for loading TF map as CTF map.
//
	int team_no;							// team number, as I got TF support it from 1 to 4. (up to 4 teams).
// }
//
	int i_agmr; 							// Instagib AirGib Master rune

	qbool was_jump;

// { wreg
	qbool wreg_attack;						// client simulate +attack via "cmd wreg" feature
	wreg_t *wreg; // [256]
// }

// { communication commands
	struct gedict_s *s_last_to;				// last direct msg to
	struct gedict_s *s_last_from;			// last direct msf from
// }

	fp_cmd_t fp_c;							// cmd flood protection
	fp_cmd_t fp_s;							// say flood protection

	float tdeath_time;						// when player invoke spawn_tdeath(...), help reduce double telefrags

// { rocket arena
	float idletime;							// how long they can idle
	float lasttime;							// last time idle was checked
	float laststattime;						// time of last status update
	raPlayerType_t ra_pt;					// ra player type
// }

// { Clan Arena/Wipepout
	struct gedict_s *track_target;			// who are we tracking?
	qbool ca_alive;
	qbool ca_ready;
	qbool can_respawn;
	qbool in_play;							// is player still fighting?
	qbool in_limbo;							// waiting to respawn during wipeout
	qbool last_alive_active;				// if last alive timer is active
	qbool no_pain;							// if player can take any damage to health or armor
	float ca_round_frags;
	float ca_round_kills;
	float ca_round_dmg;
	float ca_round_dmgt;
	float ca_round_deaths;
	float ca_round_glhit;
	float ca_round_glfired;
	float ca_round_rlhit;
	float ca_round_rldirect;
	float ca_round_lghit;
	float ca_round_lgfired;
	float alive_time;						// number of seconds player is in play
	float time_of_respawn;					// server time player respawned or round started
	float seconds_to_respawn;				// number of seconds until respawn
	float escape_time;						// number of seconds after "escaping"
	char *teamcolor;						// color of player's team
	char cptext[100];						// centerprint for player
	int ca_ammo_grenades;					// grenade ammo
	int tracking_enabled;
	int round_deaths;						// number of times player has died in the round
	int round_kills;						// number of kills in the round
	int spawn_delay;						// total delay between death and spawn
// }

// {
	float pb_time;
	float pb_old_time;
	int pb_frame;							// frame which currently must be or already played
	qbool is_playback;

	float rec_start_time;					// time when record starts
	int rec_count;							// count of recorded frames
	qbool is_recording;

	struct gedict_s *pb_ent;				// enitity which used to show our model during playback

	plrfrm_t *plrfrms;						// [MAX_PLRFRMS] for record trix
// }

// { for /cmd callalias <alias time>
	char *callalias;						// store alias name
	float callalias_time;					// time when we must activate that alias
// }

	char *f_checkbuf;						// for /cmd check f_xxx

	// Yawnmode variables
	vec3_t old_vel;							// store pre physicsthink velocity
	float spawn_weights[MAX_SPAWN_WEIGHTS];	// spawn point weights used by "fair respawns"

	// yeah its lame, but better than checking setinfo each time.
	int ezquake_version;

// midair
	float mid_top_height;
	float mid_top_avgheight;
	float mid_top_score;
	float mid_top_kills;
	float mid_top_stomps;
	float mid_top_streak;
	float mid_top_spawnfrag;
	float mid_top_rl_efficiency;

// { race
	int race_id; 							// used by checkpoints,
											// start checkpoint have id = 0,
											// intermediate checkpoints have it from 1 to xxx,
											// and end checkpoint have id xxx + 1
	float race_volume;						// how loud to play sound() when you tocuh this checkpoint
	int race_effects; 						// apply this effects when checkpoint is touched
	int race_RouteNodeType;					// this is actually must be raceRouteNodeType_t
											// but unwilling to move type definition out of race.c so using int

	int race_ready;							// is player ready for race
	int race_chasecam;						// does player want to follow other racers in line
	int race_chasecam_freelook;				// disable server forcing v_angle on client
	int race_chasecam_view;					// does follow uses the chasecam or 1st person view
	int race_afk;							// counter for afk starts
	qbool racer;							// this player is actively racing right now
	qbool race_participant;					// this player participated in the current race
	qbool muted;							// this player produces no sound
	int hideentity;							// links to entity to hide in eye chasecam
	qbool hideplayers;						// if set, all players hidden (read by mvdsv)
	qbool hideplayers_default;				// racer can choose to have this on or off
	int race_closest_guide_pos;				// when guide route loaded, this is closest position

	// race_route_start entity fields
	char *race_route_name;					// the name of the route
	char *race_route_description;			// the description of the route
	int race_route_timeout;					// the maximum time to complete the route
	int race_route_weapon_mode;				// the weapon mode - see raceWeapoMode_t
	int race_route_falsestart_mode;			// the falsestart mode - see raceFalseStartMode_t
	float race_route_start_yaw;				// the player's initial yaw angle
	float race_route_start_pitch;			// the player's initial pitch angle
	int race_flags;							// flags affecting this particular object (teleports only atm)
	int race_track;							// ent# of person this player is tracking (0 for default)
// }

	int trackent;							// pseudo spectating for players.
// { bloodfest
	qbool bloodfest_boss;					// is monsters is bloodfest boss?
// }

	qbool dropitem;							// true if placed with "dropitem" command.
// { hoonymode
	struct gedict_s *k_hoonyspawn;			// hoonymode: the player's spawn on even-numbered round
	struct gedict_s *k_hoony_new_spawn;		// hoonymode: the player's spawn for the current round
	float initial_spawn_delay;				// should be 'taken' at game start and spawn after delay
	int hoony_timelimit;					// maximum time for each round
	char *hoony_defaultwinner;				// if round time expires, who wins the round?  if null then both players score point
	byte hoony_results[HM_MAX_ROUNDS];		// store results of each round
	int hoony_prevfrags;					// frags at end of previous round
	int hoony_nomination;					// links player -> nominated spawn and vice versa
	int hoony_spawn_order;					// spawn order when shuffling
// }

#ifdef BOT_SUPPORT
// { frogbots
	fb_entvars_t fb;
// }
#endif

// { teamplay extensions, for server-side mm2
	teamplay_t tp;
	unsigned int tp_flags;
// }

// { lgc
	lgc_state_t lgc_state;
	int lgc_distance_misses[LGCMODE_DISTANCE_BUCKETS];
	int lgc_distance_hits[LGCMODE_DISTANCE_BUCKETS];
// }

// { 
	// let mvdsv know when player has teleported, and adjust for high-ping
	int teleported;
// }

// {
	// Allow teleports to vote for maps
	string_t ktx_votemap;
// }

// {
	qbool spawn_effect_queued;
// }

// { hiprot fields
	int rotate_type;                        // internal, rotate(0), movewall(1), setorigin(2), see hiprot.c
	vec3_t neworigin;                       // internal, origin tracking
	vec3_t rotate;                          // rotation angle
	vec3_t finalangle;                      // normalized version of 'angles'
	float endtime;                          // internal, animation tracking
	float duration;                         // internal, animation tracking
	string_t group;                         // linking of rotating brushes
	string_t path;                          // from ent field 'target', as 'path' to mirror original source
	string_t event;                         // events that may happen at path corners
// }

// { func_bob
	float distance;                         // distance between vantage points
	float waitmin;                          // speed-up factor, >0, see func_bob.c for defaults
	float waitmin2;                         // slowdown factor, >0, see func_bob.c for defaults
// }

// { ambient_general
	float volume;                           // attenuation, see misc.c for defaults
// }

// { trigger_heal
	float healmax;                          // maximum health see triggers.c for defaults
	float healtimer;                        // internal timer for tracking health replenishment interval
// }

// { csqc
	func_t SendEntity;
// }
} gedict_t;

typedef enum
{
	raceWeaponUnknown = 0,					// this must never happens
	raceWeaponNo,							// weapon not allowed. NOTE: must be second in this enum!!!
	raceWeaponAllowed,						// weapon allowed
	raceWeapon2s,							// weapon allowed after 2s
	raceWeaponMAX
} raceWeapoMode_t;

typedef enum
{
	raceFalseStartUnknown = 0,				// this must never happens
	raceFalseStartNo,						// no false start, player is stuck until 'go'
	raceFalseStartYes,						// false start allowed, player can move anytime before 'go'
	raceFalseStartMAX
} raceFalseStartMode_t;

typedef enum
{
	nodeUnknown = 0,						// this node is fucked by lame coding, I mean this must never happens
	nodeStart,								// this node is start of race route
	nodeCheckPoint,							// this node is intermediate
	nodeEnd,								// this node is end of race route
	nodeMAX
} raceRouteNodeType_t;

typedef struct
{
	raceRouteNodeType_t type;				// race route node type
	vec3_t ang;								// this is only need for start node, so we can set player view angles
	vec3_t org;								// node origin in 3D space
	vec3_t sizes;							// dimensions (if 0, default)
} raceRouteNode_t;

typedef struct
{
	char name[128];							// NOTE: this will probably FUCK QVM!!!
	char desc[128];							// NOTE: this will probably FUCK QVM!!!
	int cnt;								// how much nodes we actually have, no more than MAX_ROUTE_NODES
	float timeout;							// when timeout of race must occur
	raceWeapoMode_t weapon;					// weapon mode
	raceFalseStartMode_t falsestart;		// start mode
	raceRouteNode_t node[MAX_ROUTE_NODES];	// nodes of this route, fixed array, yeah I'm lazy
} raceRoute_t;

typedef struct
{
	float time;
	char racername[64];
	char demoname[64];
	float distance;
	float maxspeed;
	float avgspeed;
	float avgcount;
	char date[64];
	raceWeapoMode_t weaponmode;				// weapon mode
	raceFalseStartMode_t startmode;			// start mode
	int playernumber;
	int position;
} raceRecord_t;

typedef enum
{
	raceNone = 0,							// race is inactive
	raceCD,									// race in count down state
	raceActive,								// race is active
} raceStatus_t;

#define NUM_CHASECAMS	4 					// number of camera views defined
#define NUM_BESTSCORES	10

typedef struct
{
	raceRecord_t records[NUM_BESTSCORES];	// array of best scores information
	raceRecord_t currentrace[MAX_CLIENTS];	// curent score information

	int cnt;								// how much we actually have of routes, no more than MAX_ROUTES
	int active_route;						// which route active right now
	raceRoute_t route[MAX_ROUTES];			// fixed array of routes

// { count down
	int cd_cnt;								// 4 3 2 1 GO!
	float cd_next_think;					//
// }

	int timeout_setting;					// just how long timeout
	float timeout;							// when timeout of race must occur
	float start_time;						// when race was started

// {
	char top_nick[64];						// NOTE: this will probably FUCK QVM!!!
	int top_time;
// }

	int next_race_time;						// used for centerprint, help us print it not each server frame but more rare, each 100ms or something

	qbool warned;							// do we warned why we can't start race
	qbool race_recording;					// is race being recorded?
	int next_racer;							// this is queue of racers

	raceWeapoMode_t weapon;					// weapon mode

	raceFalseStartMode_t falsestart;		// start type

	raceStatus_t status;					// race status

	int pacemaker_time;						// time of the current pacemaker route
	char pacemaker_nick[64];				// name of the current pacemaker

	int racers_complete;					// number of players finished current race
	int last_completion_time;				// time of last player to complete the race
	int last_completion_eq;					// number of players equalling the last completion time
	int racers_competing;					// number of players starting current race

	int rounds;								// number of rounds in this match
	int round_number;						// current round number
} race_t;

extern race_t race;							// whole race struct
