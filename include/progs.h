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

#define	MAX_ENT_LEAFS	16

typedef struct shared_edict_s {
	qboolean        free;
	link_t          area;	// linked to a division node or leaf

	int             num_leafs;
	short           leafnums[MAX_ENT_LEAFS];

	entity_state_t  baseline;

	float           freetime;	// sv.time when the object was freed
	//double                lastruntime;            // sv.time when SV_RunEntity was last
	float           lastruntime1, lastruntime2;	//VM not support double // called for this edict (Tonik)

	entvars_t       v;	// C exported fields from progs
// other fields from progs come immediately after
} edict_t;

struct gedict_s;
typedef void (*th_die_func_t)();
typedef void (*th_pain_func_t)(struct gedict_s *, float);

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

typedef struct wpType_s {
	int hits;			// hits with this weapon, for SG and SSG this is count of bullets
	int attacks;		// all attacks with this weapon, for SG and SSG this is count of bullets

	int kills;			// kills with this weapon
	int deaths;			// deaths from this weapon
	int tkills;			// team kills with this weapon
	int ekills;			// killed enemys which contain this weapon in inventory
	int drops;			// number of packs dropped which contain this weapon
	int tooks;			// took this weapon and does't have this weapon before took (weapon from packs counted too)
	int ttooks;			// total tooked, even u alredy have this weapon

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

typedef struct itType_s {
	int tooks; // taken count
	float time; // total time u have some item

} itType_t;

// store player statistic here, like taken armors etc...
typedef struct player_stats_s {
	wpType_t wpn[wpMAX];
	itType_t itm[itMAX];

	float    dmg_t; // damage taken
	float    dmg_g; // damage given
	float    dmg_team;  // damage to team
// { k_dmgfrags
	float    dmg_frags; // frags awarded from damage (CA)
// }

	int		ot_a;	 // overtime armor value
//	float	ot_at;   // overtime armor type
	int     ot_items;// overtime items
	int		ot_h;	 // overtime health

	int		spawn_frags;

	int		handicap;

	// ctf stats
	int ctf_points; // use frags - this to calculate efficiency for ctf
	int caps;		// flag captures
	int f_defends;  // flag defends
	int c_defends;	// carrier defends
	int c_frags;	// frags on enemy carriers
	int pickups;	// flag pickups
	int returns;	// flag returns
	float res_time;	// time with resistance rune
	float str_time;	// time with strength rune
	float hst_time;	// time with haste rune
	float rgn_time;	// time with regen rune

	// midair stats
	int midairs;	// total number of midairs
	int midairs_s;	// silver midairs
	int midairs_g;	// gold midairs
	int midairs_d;	// diamond midairs

	// spree stats
	int spree_current;		// number of frags since our last death
	int spree_current_q;	// number of frags in current quad run
	int spree_max;			// largest spree throughout game
	int spree_max_q;		// largest quad spree throughout game

	// rocket arena
	int wins;	//number of wins they have
	int loses;	//number of loses they have

	// velocity stats
	float velocity_max;	// maximum velocity
	float velocity_sum;	// summary velocity from all frames
	int vel_frames;		// number of frames

	// instagib stats
	int i_height;	// Cumulated height  of airgibs
	int i_maxheight;	
	int i_cggibs;
	int i_axegibs;
	int i_stompgibs;
	int i_multigibs;
	int i_airgibs;		// Number of airgibs
	int i_maxmultigibs;
	int i_rings;

} player_stats_t;


typedef enum
{
	etNone = 0,
	etCaptain,
	etAdmin
} electType_t;

// store player votes here
typedef struct vote_s {

	int brk;
	int elect;
	int map;
	int pickup;
	int rpickup;

	electType_t elect_type; // election type
	float elect_block_till;	// block election for this time
} vote_t;

// cmd flood protection

#define MAX_FP_CMDS (10)

typedef struct fp_cmd_s {
	float locked;
	float cmd_time[MAX_FP_CMDS];
	int   last_cmd;
	int   warnings;
} fp_cmd_t;

// store wregs here

#define MAX_WREGS    (256)
#define MAX_WREG_IMP (9)

typedef struct wreg_s {
	qboolean init;

	int attack;
	int impulse[MAX_WREG_IMP];
} wreg_t;

// {

#define MAX_PLRFRMS (77*15) /* 77 frames for each of 15 seconds */

typedef struct
{
	float	time;
//	float	modelindex;
	vec3_t	origin;
	vec3_t	angles;
	float	frame;
	float	effects;
//	vec3_t	v_angle;
	float	colormap;
} plrfrm_t;

// }

// player position

#define MAX_POSITIONS (5)

typedef struct pos_s {

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

//typedef (void(*)(gedict_t *)) one_edict_func;
typedef struct gedict_s {
	edict_t         s;

//custom fields

	float           vw_index;
	float           maxspeed, gravity;
	int             isBot;
	float 			brokenankle; // player can't jump for a while after falling and get damaged
	char           *mdl;
	char           *killtarget;
	int             worldtype;	// 0=medieval 1=metal 2=base
	char           *map;
//player
	int             walkframe;

//
// quakeed fields
//
	int             style;
	float           speed;
	int             state;
	int             voided;
//items.c
	float           healamount, healtype;
//ammo
	int             aflag;
//
// doors, etc
//
	vec3_t		movement;
	vec3_t          dest, dest1, dest2;
	vec3_t          pos1, pos2, oldorigin;
	vec3_t          mangle;
	float           t_length, t_width, height;
	float           wait;	// time from firing to restarting
	float           delay;	// time from activation to firing
	struct gedict_s *trigger_field;	// door's trigger entity
	struct gedict_s *movetarget;
	float           pausetime;
	char           *noise4;
	float           count;
//
// misc
//
	float           cnt;	// misc flag

	void            ( *think1 ) ();	//calcmove
	vec3_t          finaldest;
//combat
	float           dmg;
	float           lip;
	float           attack_finished;
	float           pain_finished;

	float           invincible_finished;
	float           invisible_finished;
	float           super_damage_finished;
	float           radsuit_finished;


	float           invincible_time, invincible_sound;
	float           invisible_time, invisible_sound;
	float           super_time, super_sound;
	float           rad_time;
	float           fly_sound;

	float           axhitme;
	float           show_hostile;	// set to time+0.2 whenever a client fires a
	// weapon or takes damage.  Used to alert
	// monsters that otherwise would let the player go
	float           jump_flag;	// player jump flag
	float           swim_flag;	// player swimming sound flag
	float           air_finished;	// when time > air_finished, start drowning
	float           bubble_count;	// keeps track of the number of bubbles
	int				deathtype;	// keeps track of how the player died
	float           dmgtime;

	th_die_func_t   th_die;
	th_pain_func_t  th_pain;

// below is KTX fields

	float	cnt2;            // NOT_SURE: cnt2 = seconds left?
	float	dead_time;       // time at which player last died - used in autorespawn
	float	spawn_time;		 // time at which player last spawned
	float	connect_time;	 // time at which player connect
	float	deaths;          // number of times player died
	float	efficiency;      // stores player efficiency rating
	float	friendly;        // number of times player killed teammates
	float	ready;           // if a player is ready or not
	char	*killer;         // name of player who last killed player
	char	*victim;         // name of player last killed

	struct gedict_s *k_lastspawn;    // NOT_SURE: spot at which player last spawned?
// { kick mode
	struct gedict_s *k_playertokick; // player selected to be kicked
	float	k_kicking;       // if player is in kick mode
// }
	float	k_1spawn;        // NOT_SURE: used in kteams respawn code...
	float	k_666;           // if player has 666 respawn protection
	float	k_accepted;      // NOT_SURE:
	int		k_added;         // NOT_SURE: stores the entered admin code?
	int		k_adminc;        // number of digits of admin code still to enter
	int 	k_admin;         // if player is an admin, flags
	float   k_adm_lasttime;	 // store time of last attempt getting admin rights
	int		k_captain;       // if player is team captain
	float	k_flag;          // flagvalue to customise settings such as sounds/autoscreenshot
	float	k_msgcount;      // NOT_SURE: last time mod printed a message to client?
	float	k_picked;        // NOT_SURE: 
	float	k_ptime;         // stores player nexttime value when server is paused
	float	k_stuff;         // if player has received stuffed aliases
	float	k_teamnumber;    // team the player is a member of, 1 = team1, 2 = team2
	float	k_teamnum;       // NOT_SURE:
	
	float lastwepfired;
	
//	float maxspeed;       // Used to set Maxspeed on a player moved from old qw defs.qc

	float suicide_time;	// can't suicide sooner than this

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

	float shownick_time;	// used to force centerprint is off at desired time
	clientType_t ct;		// client type for client edicts
// { timing
	float k_lastPostThink;  // last time PlayerPostThink or SpectatorThink was called
	float k_timingWarnTime; // time of last client is timing warning
	int   k_timingTakedmg;  // .... so we can restore
	int   k_timingSolid;    // .... so we can restore
	int   k_timingMovetype; // .... so we can restore
// }
	int   k_voteUnpause;    // store here vote for unpause, for particular player
// { stats
	int   wp_stats;			// show +wp_stats or not
	float wp_stats_time;    // used to force centerprint is off at desired time
	int   sc_stats;			// show +scores or not
	float sc_stats_time;    // used to force centerprint is off at desired time

	player_stats_t ps;		// store player statistic here, like taken armors etc...

	float q_pickup_time;	// time then u took quad
	float p_pickup_time;	// time then u took pent
	float r_pickup_time;	// time then u took ring
// }

	int   need_clearCP;		// if this true, clear center print at certain cases

// { ghost stuff
	int   ghost_slot;		// now ktx put ghost in players scoreboards in one of free slots - store this slot
	float ghost_dt;         // ghost drop time - time when player dropped
	int   ghost_clr;		// color of dropped player
// }

	float nthink;			// store here regeneration time of some items

	vote_t v;				// player votes stored here

// { spec stuff
	int   favx[MAX_CLIENTS];    // here stored players number for appropriate favX_add/Xfav_go commands
	int   fav[MAX_CLIENTS];     // here stored players number for fav_add/next_fav commands
	autoTrackType_t autotrack;  // is autotrack or auto_pow or ktpro autorack
	qboolean apply_ktpro_autotrack; // if we use ktpro's autotrack, that a hint to apply pov switch
	struct gedict_s *wizard;    // for specs, link to the entity which represent wizard
	int   last_goal;            // just store here self->s.v.goal from last spec frame, so we can catch pov switch

// }

	pos_t	pos[MAX_POSITIONS];	// for player pos_save / pos_move
	float	pos_move_time;		// for player pos_save / pos_move

// { CTF
	struct gedict_s *hook;         // grapple
	qboolean ctf_sound;            // sound stuff
	qboolean on_hook;              // are we on the grapple?
	qboolean hook_out;             // is the grapple in flight?
	int ctf_flag;                  // do we have a rune or flag?
	float ctf_freeze;              // removes jitterness from grapple 
	float regen_time;              // time to update health if regen rune
	float rune_sound_time;         // dont spam rune sounds (1 per second)
	float carrier_frag_time;       // used for carrier assists
	float return_flag_time;        // used for flag return assists
	float rune_notify_time;        // already have a rune spam prevention
	float carrier_hurt_time;       // time we last hurt enemy carrier
	float rune_pickup_time;        // time we picked up current rune
	char *last_rune;			   // name of last rune we send to client
	float	items2;				   // using  ZQ_ITEMS2 extension in mvdsv we can use per client sigils for runes
// }
//
	int i_agmr; 		// Instagib AirGib Master rune

	qboolean was_jump;

// { wreg
	qboolean wreg_attack;		   // client simulate +attack via "cmd wreg" feature
	wreg_t *wreg; // [256]
// }

// { communication commands
	struct gedict_s *s_last_to;    // last direct msg to
	struct gedict_s *s_last_from;  // last direct msf from
// }

	fp_cmd_t fp_c; // cmd flood protection
	fp_cmd_t fp_s; // say flood protection

	float tdeath_time;	// when player invoke spawn_tdeath(...), help reduce double telefrags

// { rocket arena
	float idletime;		// how long they can idle
	float lasttime;		// last time idle was checked
	float laststattime;	// time of last status update
	raPlayerType_t ra_pt; // ra player type
// }

// {
	float pb_time;
	float pb_old_time;
	int pb_frame; // frame which currently must be or already played
	qboolean is_playback;

	float rec_start_time; // time when record starts
	int rec_count; // count of recorded frames
	qboolean is_recording;

	struct gedict_s *pb_ent; // enitity which used to show our model during playback

	plrfrm_t *plrfrms; // [MAX_PLRFRMS] for record trix
// }

// { for /cmd callalias <alias time>
	char *callalias;		// store alias name
	float callalias_time;	// time when we must activate that alias
// }

	char *f_checkbuf;	// for /cmd check f_xxx

	// Jawnmode variables
	vec3_t old_vel;				// store pre physicsthink velocity
	float  spawn_weights[64];	// spawn point weights used by "fair respawns"

} gedict_t;

