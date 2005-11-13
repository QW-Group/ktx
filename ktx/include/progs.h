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
 *  $Id: progs.h,v 1.4 2005/11/13 18:45:48 qqshka Exp $
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


// store player statistic here, like taken armors etc...
typedef struct player_stats_s {

// h_xxx hits
// a_xxx all attacks
	int   h_axe;	// :]
	int   a_axe;
	int   h_sg;
	int   a_sg;
	int   h_ssg;
	int   a_ssg;
	int   h_ng;
	int   a_ng;
	int   h_sng;
	int   a_sng;
	int   h_gl;
	int   a_gl;
	int   h_rl;
	int   a_rl;
	int   h_lg;
	int   a_lg;

	int   ra; //    red armors taken count
	int   ya; // yellow armors taken count
	int   ga; //  green armors taken count
	int   mh; //  mega healths taken count
	int   quad; // taken count
	int   pent; // taken count
	int   ring; // taken count

	float    dmg_t; // damage taken
	float    dmg_g; // damage given

	int		ot_a;	 // overtime armor value
//	float	ot_at;   // overtime armor type
	int     ot_items;// overtime items
	int		ot_h;	 // overtime health

	int		spawn_frags;

} player_stats_t;


//typedef (void(*)(gedict_t *)) one_edict_func;
typedef struct gedict_s {
	edict_t         s;
	//custom fields
	float           maxspeed, gravity;
	int             isBot;
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
	char           *deathtype;	// keeps track of how the player died
	float           dmgtime;

	th_die_func_t   th_die;
	th_pain_func_t  th_pain;

#ifdef KTEAMS

	float	cnt2;            // NOT_SURE: cnt2 = seconds left?
	float	dead_time;       // time at which player last died - used in autorespawn
	float	spawn_time;		 // time at which player last spawned
	float	deaths;          // number of times player died
	float	efficiency;      // stores player efficiency rating
	float	fraggie;         // NOT_SURE: used to return frags to rejoining players?
	float	friendly;        // number of times player killed teammates
	float	ready;           // if a player is ready or not
	char	*killer;         // name of player who last killed player
	char	*victim;         // name of player last killed

	struct gedict_s *k_lastspawn;    // NOT_SURE: spot at which player last spawned?
	struct gedict_s *k_playertokick; // player selected to be kicked
	float	k_1spawn;        // NOT_SURE: used in kteams respawn code...
	float	k_666;           // if player has 666 respawn protection
	float	k_accepted;      // NOT_SURE:
	float	k_added;         // NOT_SURE: stores the entered admin code?
	float	k_adminc;        // number of digits of admin code still to enter
	float	k_admin;         // if player is an admin
	float	k_captain;       // if player is team captain
//	float	k_domapstuff;    // NOT_SURE: if spec has 'done' map stuff?
	float	k_flag;          // flagvalue to customise settings such as sounds/autoscreenshot
	float	k_kicking;       // if player is in kick mode
	float	k_msgcount;      // NOT_SURE: last time mod printed a message to client?
	float	k_picked;        // NOT_SURE: 
	float	k_pickup;        // NOT_SURE: if player wants pickup game or not?
	float	k_ptime;         // stores player nexttime value when server is paused
	float	k_stuff;         // if player has received stuffed aliases
	float	k_teamnumber;    // team the player is a member of, 1 = team1, 2 = team2
	float	k_teamnum;       // NOT_SURE:
	float	k_track;         // NOT_SURE: something to do with spectator tracking code
	float	k_vote2;         // stores player election vote decision
	float	k_vote;          // stores player map vote decision
	char	*kick_ctype;     // if selected player to kick is player or spectator
	
	// Zibbo's frametime checking code
	float real_time;
	float uptimebugpolicy;
	float lastwepfired;
	
//	float maxspeed;       // Used to set Maxspeed on a player moved from old qw defs.qc

	float suicide_time;	// can't suicide sooner than this

// ILLEGALFPS[abortelect
	float fAverageFrameTime;
	float fFrameCount;
	float fDisplayIllegalFPS;
	float fLowestFrameTime;
	float fIllegalFPSWarnings;
// ILLEGALFPS]
	
	float brokenankle;		// player can't jump for a while after falling and get damaged
	float shownick_time;	// used to force centerprint is off at desired time
	int	  vip;				// store vip level/flags here
	int   cmd_selectMap;	// store cmd cm <value> here
	int   k_spectator;		// true if spectator
	int   k_player;		    // true if player
// --> timing
	float k_lastPostThink;  // last time PlayerPostThink or SpectatorThink was called
	float k_timingWarnTime; // time of last client is timing warning
	int   k_timingEffects;  // store here effects which was before detecting player is lagged, so we can restore in to the future
	int   k_timingTakedmg;  // .... so we can restore
	int   k_timingSolid;    // .... so we can restore
	int   k_timingMovetype; // .... so we can restore
// <-- timing
	int   k_voteUnpause;    // store here vote for unpause, for particular player
// --> stats
	int   wp_stats;			// show wp_stats or not
	float wp_stats_time;    // used to force centerprint is off at desired time

	player_stats_t ps;		// store player statistic here, like taken armors etc...

// <-- stats

	int   need_clearCP;		// if this true, clear center print at certain cases
#endif

} gedict_t;

