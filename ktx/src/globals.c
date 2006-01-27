#include "g_local.h"

#ifdef KTEAMS

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
	float k_nochange = 0;   // used to indicate if scores changed since last time 'scores' command was called
	float k_oldmaxspeed;    // used to store old value of maxspeed prior to freezing the map
	float k_pause = 0;
	float k_pausetime;      // stores time at which server was paused
	float k_scores1;        // stores team1 combined score
	float k_scores2;        // stores team2 combined score
	float k_showscores;     // whether or not should print the scores or not
	float k_standby;        // if server is in standy mode
	float k_sudden_death;	// to mark if sudden death overtime is currently the case
	float k_teamid = 0;
	float k_userid = 0;		// somethink like numbers of ghosts + 1
	float k_whonottime;     // NOT_SURE: 
	float lock = 0;         // stores whether players can join when a game is already in progress
	float match_in_progress = 0;    // if a match has begun
	float match_over;       // boolean - whether or not the match stats have been printed at end of game
	char *newcomer;        // stores name of last player who joined

	int   k_overtime = 0;  // is overtime is going on

	float server_is_2_3x;	// if true, fix the jump bug via QC
	float current_maxfps;	// current value of serverinfo maxfps

	int       k_practice;	// is server in practice mode
	int       k_matchLess;	// is server in matchLess mode
	gameType_t 	  k_mode;   // game type: DUEL, TP, FFA
#endif
