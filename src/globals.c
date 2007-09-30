/*
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

#include "g_local.h"

// WARNING: globals are cleared every map change

float framechecks;	    // if timedemo/uptime bugs are tolerated
float k_attendees;      // stores number of players on server - used in 'ready' checking
float k_berzerk;        // boolean - whether or not the game is currently in berzerk time
float k_berzerkenabled; // actually stores the amount of berzerk time at the end of match
float k_captains;	    // number of captains
float k_captainturn;	// which captain comes in line to pick
float k_checkx = 0;		// global which set to true when some time spend after match start
float k_force;          // used in forcing matchstart
float k_maxspeed;       // used to store server maxspeed to allow switching by admins
float k_oldmaxspeed;    // used to store old value of maxspeed prior to freezing the map
float k_showscores;     // whether or not should print the scores or not
						// now k_showscores != 0  only in team mode exactly with two teams
float k_nochange = 0;   // used to indicate if frags changes somehow since last time 'scores' command was called
float k_standby;        // if server is in standy mode
float k_sudden_death;	// to mark if sudden death overtime is currently the case
float k_teamid = 0;
float k_userid = 0;		// somethink like numbers of ghosts + 1
float k_whonottime;     // NOT_SURE: 
float match_in_progress = 0;    // if a match has begun
float match_start_time; // time when match has been started
float match_over;       // boolean - whether or not the match stats have been printed at end of game
gedict_t *newcomer = g_edicts;     // stores last player who joined

int   k_overtime = 0;   // is overtime is going on

float current_maxfps;	// current value of serverinfo maxfps

// { jawn mode
int	k_jawnmode;			// is server in jawn mode
int k_teleport_cap = 24;	// cap for keeping velocity through tele
// }
int	k_practice;			// is server in practice mode
int	k_matchLess;		// is server in matchLess mode
gameType_t 	  k_mode;   // game type: DUEL, TP, FFA
int	k_lastvotedmap;		// last voted map, used for agree command?

// { CTF
int k_ctf_custom_models;// if server has flag/grapple models you can enable them here
						// http://www.quakeworld.us/ult/ctf/pak0.pak  (only 300kb)
						// if not we use old style keys and ax/voreball for grapple
#ifdef CTF_RELOADMAP
qboolean k_ctf;			// is ctf was active at map load
#endif
// }

int k_allowed_free_modes; // reflect appropriate cvar - but changed only at map load

// { cmd flood ptotect

int   k_cmd_fp_count;    // 10 commands allowed ..
float k_cmd_fp_per;      // per 4 seconds
float k_cmd_fp_for;      // skip commands for 5 seconds
int   k_cmd_fp_kick;     // kick after 4 warings
int   k_cmd_fp_dontkick; // if 1 - don't use kick
int   k_cmd_fp_disabled; // if 1 - don't use cmd floodprot

// }

qboolean	vw_available; // vwep extension available
qboolean	vw_enabled; // vweps enabled

float k_sv_locktime; // some time before non VIP players can't connect, spectators not affected

// { rocket arena

float		time_to_start;	// time to start match
int			ra_match_fight;	// have winner and loser fighting
int			k_rocketarena;	// is RA active or not, since we must catch changes, we can't use cvar("k_rocketarena")
	
// }

int	jumpf_flag; // falling velocity criteria

float	f_check; // is we in state of some f_xxx check

float lastTeamLocationTime; // next udate for CheckTeamStatus()

qboolean first_rl_taken; // true when some one alredy took rl

int sv_minping; // used to broadcast changes
