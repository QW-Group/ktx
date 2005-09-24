/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// commands.c

#include "g_local.h"

void StuffMainMaps();

char *GetMapName(float f2);
void SendMessage(char *name);
float CountRPlayers();
float CountPlayers();
float CountRTeams();
float CountTeams();
void PlayerReady ();
void PlayerBreak ();
void ReqAdmin ();
void AdminForceStart ();
void AdminForceBreak ();
void AdminForcePause ();
void TogglePreWar ();
void ToggleMapLock ();
void ToggleMaster ();
void AdminKick ();
void YesKick ();
void DontKick ();
void VoteAdmin();
void VoteYes();
void VoteNo();
void BecomeCaptain ();
void Deathmsg ();
void RandomPickup();
void ShowDMM();
void ChangeDM(float dmm);
void ChangeLock();
void ChangeOvertime();
void ChangeOvertimeUp();
void ChangeTP();
void ToggleFallBunny ();
void FragsDown();
void FragsUp();
void ListWhoNot();
void ModStatus();
void ModStatus2();
void ModStatusVote();
void PlayerStats();
void PlayerStatus();
void PlayerStatusN();
void PlayerStatusS();
void PrintScores();
void ResetOptions();
void ReportMe();
void SendKillerMsg();
void SendNewcomerMsg();
void SendVictimMsg();
void ShowNick();
void ShowCmds();
void ShowMaps();
void ShowMessages();
void ShowOpts();
void ShowQizmo();
void ShowRules();
void ShowVersion();
void ToggleBerzerk();
void ToggleDischarge();
void ToggleDropPack();
void ToggleDropQuad();
void ToggleDropRing();
void ToggleFairPacks();
void ToggleFreeze();
void TogglePowerups();
void ToggleQEnemy();
void ToggleQLag();
void ToggleQPoint();
void ToggleRespawn666();
void ToggleRespawns();
void ToggleSpecTalk();
void ToggleSpeed();
void VotePickup();
void VoteUnpause();
void UserMode(float umode);

// spec
void ShowCamHelp();

void TeamSay(float fsndname);
void TimeDown(float t);
void TimeUp(float t);
void TimeSet(float t);


cmd_t cmds[] = {

	{ "commands",   (func_t) ShowCmds,			        0    , CF_BOTH        },
	{ "scores",     (func_t) PrintScores,		        0    , CF_BOTH        },
	{ "stats",      (func_t) PlayerStats,               0    , CF_BOTH        },
	{ "options",    (func_t) ShowOpts,                  0    , CF_PLAYER      },
	{ "ready",      (func_t) PlayerReady,               0    , CF_PLAYER      },
	{ "break",      (func_t) PlayerBreak,               0    , CF_PLAYER      },
	{ "status",     (func_t) ModStatus,                 0    , CF_BOTH        },
	{ "status2",    (func_t) ModStatus2,                0    , CF_BOTH        },
	{ "who",        (func_t) PlayerStatus,              0    , CF_BOTH        },
	{ "whoskin",    (func_t) PlayerStatusS,             0    , CF_BOTH        },
	{ "whonot",     (func_t) PlayerStatusN,             0    , CF_BOTH        },
	{ "whovote",    (func_t) ModStatusVote,             0    , CF_BOTH        },
	{ "spawn",      (func_t) ToggleRespawns,            0    , CF_PLAYER | CF_SPC_ADMIN },
	{ "powerups",   (func_t) TogglePowerups,            0    , CF_PLAYER | CF_SPC_ADMIN },
	{ "discharge",  (func_t) ToggleDischarge,           0    , CF_PLAYER      },
	{ "dm",         (func_t) ShowDMM,                   0    , CF_PLAYER      },
	{ "dmm1",       (func_t) ChangeDM,                  1    , CF_PLAYER | CF_SPC_ADMIN },
	{ "dmm2",       (func_t) ChangeDM,                  2    , CF_PLAYER | CF_SPC_ADMIN },
	{ "dmm3",       (func_t) ChangeDM,                  3    , CF_PLAYER | CF_SPC_ADMIN },
	{ "dmm4",       (func_t) ChangeDM,                  4    , CF_PLAYER | CF_SPC_ADMIN },
	{ "dmm5",       (func_t) ChangeDM,                  5    , CF_PLAYER | CF_SPC_ADMIN },
	{ "tp",         (func_t) ChangeTP,                  0    , CF_PLAYER | CF_SPC_ADMIN },
	{ "timedown1",  (func_t) TimeDown,				  1.0f   , CF_PLAYER | CF_SPC_ADMIN },
	{ "timeup1",    (func_t) TimeUp,				  1.0f   , CF_PLAYER | CF_SPC_ADMIN },
	{ "timedown",   (func_t) TimeDown,				  5.0f   , CF_PLAYER | CF_SPC_ADMIN },
	{ "timeup",     (func_t) TimeUp,				  5.0f   , CF_PLAYER | CF_SPC_ADMIN },
	{ "fallbunny",  (func_t) ToggleFallBunny,           0    , CF_BOTH_ADMIN  },
	{ "fragsdown",  (func_t) FragsDown,                 0    , CF_PLAYER      },
	{ "fragsup",    (func_t) FragsUp,                   0    , CF_PLAYER      },
	{ "dropquad",   (func_t) ToggleDropQuad,            0    , CF_PLAYER      },
	{ "dropring",   (func_t) ToggleDropRing,            0    , CF_PLAYER      },
	{ "droppack",   (func_t) ToggleDropPack,            0    , CF_PLAYER      },
	                                             
    { "silence",    (func_t) ToggleSpecTalk,            0    , CF_PLAYER | CF_SPC_ADMIN },
	{ "reset",      (func_t) ResetOptions,              0    , CF_PLAYER | CF_SPC_ADMIN },
	{ "report",     (func_t) ReportMe,                  0    , CF_PLAYER      },
	{ "rules",      (func_t) ShowRules,                 0    , CF_PLAYER      },
	{ "lock",       (func_t) ChangeLock,                0    , CF_PLAYER      },
	{ "maps",       (func_t) ShowMaps,                  0    , CF_PLAYER | CF_SPC_ADMIN },
	{ "spawn666",   (func_t) ToggleRespawn666,          0    , CF_PLAYER      },
	{ "admin",      (func_t) ReqAdmin,                  0    , CF_BOTH        },
	{ "forcestart", (func_t) AdminForceStart,           0    , CF_BOTH_ADMIN  },
	{ "forcebreak", (func_t) AdminForceBreak,           0    , CF_BOTH_ADMIN  },
	{ "forcepause", (func_t) AdminForcePause,           0    , CF_BOTH_ADMIN  },
	{ "pickup",     (func_t) VotePickup,                0    , CF_PLAYER      },
	{ "prewar",     (func_t) TogglePreWar,              0    , CF_BOTH_ADMIN  },
	{ "lockmap",    (func_t) ToggleMapLock,             0    , CF_BOTH_ADMIN  },
	{ "master",     (func_t) ToggleMaster,              0    , CF_BOTH_ADMIN  },
	{ "speed",      (func_t) ToggleSpeed,               0    , CF_PLAYER      },
	{ "fairpacks",  (func_t) ToggleFairPacks,           0    , CF_PLAYER      },
	{ "about",      (func_t) ShowVersion,               0    , CF_BOTH        },
	{ "shownick",   (func_t) ShowNick,                  0    , CF_PLAYER      },
	{ "time5",      (func_t) TimeSet,		  	 	  5.0f   , CF_PLAYER      },
	{ "time10",     (func_t) TimeSet,		  	     10.0f   , CF_PLAYER      },
	{ "time15",     (func_t) TimeSet,		  	     15.0f   , CF_PLAYER      },
	{ "time20",     (func_t) TimeSet,                20.0f   , CF_PLAYER      },
	{ "time25",     (func_t) TimeSet,                25.0f   , CF_PLAYER      },
	{ "time30",     (func_t) TimeSet,                30.0f   , CF_PLAYER      },
	{ "berzerk",    (func_t) ToggleBerzerk,             0    , CF_PLAYER      },
	                                             
//	{ "ksound1",    (func_t) TeamSay,   (int)("ktsound1.wav"), CF_PLAYER      },
//	{ "ksound2",    (func_t) TeamSay,   (int)("ktsound2.wav"), CF_PLAYER      },
//	{ "ksound3",    (func_t) TeamSay,   (int)("ktsound3.wav"), CF_PLAYER      },
//	{ "ksound4",    (func_t) TeamSay,   (int)("ktsound4.wav"), CF_PLAYER      },
//	{ "ksound5",    (func_t) TeamSay,   (int)("ktsound5.wav"), CF_PLAYER      },
//	{ "ksound6",    (func_t) TeamSay,   (int)("ktsound6.wav"), CF_PLAYER      },
	                                           
	{ "qizmo",      (func_t) ShowQizmo,               0    , CF_PLAYER      },
	                                           
	{ "messages",   (func_t) ShowMessages,            0    , CF_PLAYER      },
	{ "killer",     (func_t) SendKillerMsg,           0    , CF_PLAYER      },
	{ "victim",     (func_t) SendVictimMsg,           0    , CF_PLAYER      },
	{ "newcomer",   (func_t) SendNewcomerMsg,         0    , CF_PLAYER      },
	                                           
	{ "qlag",       (func_t) ToggleQLag,              0    , CF_PLAYER | CF_SPC_ADMIN },
	{ "qenemy",     (func_t) ToggleQEnemy,            0    , CF_PLAYER | CF_SPC_ADMIN },
	{ "qpoint",     (func_t) ToggleQPoint,            0    , CF_PLAYER | CF_SPC_ADMIN },

    { "kick",       (func_t) AdminKick,               0    , CF_BOTH_ADMIN  },
    { "y",          (func_t) YesKick,                 0    , CF_BOTH_ADMIN  },
    { "n",          (func_t) DontKick,                0    , CF_BOTH_ADMIN  },
    { "list",       (func_t) ListWhoNot,              0    , CF_PLAYER      },
    { "overtime",   (func_t) ChangeOvertime,          0    , CF_PLAYER | CF_SPC_ADMIN },
    { "overtimeup", (func_t) ChangeOvertimeUp,        0    , CF_PLAYER | CF_SPC_ADMIN },
    { "elect",      (func_t) VoteAdmin,               0    , CF_BOTH        },
    { "yes",        (func_t) VoteYes,                 0    , CF_PLAYER      },
    { "no",         (func_t) VoteNo,                  0    , CF_PLAYER      },
    { "captain",    (func_t) BecomeCaptain,           0    , CF_PLAYER      },
    { "freeze",     (func_t) ToggleFreeze,            0    , CF_PLAYER      },
	{ "deathmsg",   (func_t) Deathmsg,                0    , CF_BOTH_ADMIN  },
    { "rpickup",    (func_t) RandomPickup,            0    , CF_BOTH_ADMIN  },

//    { "1on1",       (func_t) UserMode,        (int)("1on1"), CF_PLAYER | CF_SPC_ADMIN },
//    { "2on2",       (func_t) UserMode,        (int)("2on2"), CF_PLAYER | CF_SPC_ADMIN },
//    { "3on3",       (func_t) UserMode,        (int)("3on3"), CF_PLAYER | CF_SPC_ADMIN },
//    { "4on4",       (func_t) UserMode,        (int)("4on4"), CF_PLAYER | CF_SPC_ADMIN },
//    { "10on10",     (func_t) UserMode,      (int)("10on10"), CF_PLAYER | CF_SPC_ADMIN },
//    { "ffa",        (func_t) UserMode,         (int)("ffa"), CF_PLAYER | CF_SPC_ADMIN },

    { "unpause",    (func_t) VoteUnpause,             0    , CF_PLAYER      },

    { "cam",        (func_t) ShowCamHelp,             0    , CF_SPECTATOR   }
};

int cmds_cnt = sizeof( cmds ) / sizeof( cmds[0] );

//
// DoCommand
//
// return -1 if command is out of range in 'cmds' array
// return -2 if wrong class
// return -3 if access denied
// return -4 if function is wrong
//

int DoCommand(int icmd)
{
	int spc = self->k_spectator;
	int adm = (int) self->k_admin;

	if ( !( icmd >= 0 && icmd < cmds_cnt ) )
		return -1;

	if ( spc ) { // spec
		if ( !(cmds[icmd].cf_flags & CF_SPECTATOR) )
			return -2; // cmd not for spectator
		if ( (cmds[icmd].cf_flags & CF_SPC_ADMIN) && adm != 2 )
			return -3; // admin rights required
	}
	else { // player
		if ( !(cmds[icmd].cf_flags & CF_PLAYER) )
			return -2; // cmd not for player
		if ( (cmds[icmd].cf_flags & CF_PLR_ADMIN) && adm != 2 )
			return -3; // admin rights required
	}

	if ( strnull( cmds[icmd].name ) || !( cmds[icmd].f ) )
		return -4;

	if (cmds[icmd].arg)
		( ( void ( * )(float) ) ( cmds[icmd].f ) ) ( cmds[icmd].arg );
	else
		( ( void ( * )() ) ( cmds[icmd].f ) ) ();

	return icmd;
}

void StuffAliases()
{
// stuffing for numbers, hope no flooding out
	int i;

	for ( i = 1; i <= 16; i++ )
		stuffcmd(PROG_TO_EDICT( self->s.v.owner ), "alias %d impulse %d\n", i, i);

	if ( PROG_TO_EDICT( self->s.v.owner )->k_spectator )
		; // none for spectator
	else {
		stuffcmd(PROG_TO_EDICT( self->s.v.owner ), "alias notready break\n");
	}
}

float StuffDeltaTime(int iDelta)
{
	iDelta = bound ( 1, ( iDelta ? iDelta : 4 ), 5); // i.e. default is 4
	return 0.01f * (float)iDelta;
}

void StuffModCommands()
{
	int i, limit, spc = PROG_TO_EDICT( self->s.v.owner )->k_spectator;
	char *name;
	float dt = StuffDeltaTime( atoi ( ezinfokey( PROG_TO_EDICT( self->s.v.owner ), "ss" ) ) );

	if(self->cnt == -1)	{
		StuffAliases(); // stuff impulses based aliases, or just aliases

		self->cnt = 0;
		self->s.v.nextthink = g_globalvars.time + 0.1;
		return;
	}

	i = self->cnt;
	limit = i + STUFFCMDS_PER_PORTION; // # stuffcmd's per portion

	for( ; i <= limit && ( i >= 0 && i < cmds_cnt ); i++ ) {

		name = cmds[i].name;

		if ( spc ) { // spec
			if ( !(cmds[i].cf_flags & CF_SPECTATOR) ) {
//				G_bprint(2, "sp skip: %s\n", name);

				limit++;
				continue; // cmd not for spec
			}
		}
		else { // player
			if ( !(cmds[i].cf_flags & CF_PLAYER) ) {
//				G_bprint(2, "pl skip: %s\n", name);

				limit++;
				continue; // cmd not for player
			}
		}

		stuffcmd(PROG_TO_EDICT( self->s.v.owner ), "alias %s cmd cc %d\n", name, (int)i);
	}

	if( i <= limit /* all commands is stuffed */ )
	{
		stuffcmd(PROG_TO_EDICT( self->s.v.owner ), "echo Commands downloaded\n");

		// no more commands, so setup for stuffing main maps.
		self->cnt = -1;
		self->s.v.think = ( func_t ) StuffMainMaps;
		self->s.v.nextthink = g_globalvars.time + dt;

		return;
	}

	// next time around we'll be starting from the i variable which is the next available command.
	self->cnt = i;
	
	// 'dt' sec delay before next stuffing.
	self->s.v.nextthink = g_globalvars.time + dt;
	
	return;
}

void Init_cmds(void)
{
	int i = 0;

	for( ; i < cmds_cnt; i++ ) {

		if ( strnull( cmds[i].name ) || !( cmds[i].f ) )
			G_Error("Init_cmds: null");

		if ( cmds[i].cf_flags & CF_PLR_ADMIN )
			cmds[i].cf_flags |= CF_PLAYER;    // this let simplify cmds[] table

		if ( cmds[i].cf_flags & CF_SPC_ADMIN )
			cmds[i].cf_flags |= CF_SPECTATOR; // this let simplify cmds[] table
	}
}   
    
void PShowCmds()
{   
	if( self->k_admin == 2 ) {
		G_sprint(self, 2,
		"αδνιξ....... give up admin rights\n"
		"πςεχας...... playerfire before game\n"
		"μογλναπ..... (un)lock current map\n"
		"ζοςγεσταςτ.. force match to start\n"
		"ζοςγεβςεαλ.. force match to end\n"
		"ζοςγεπαυσε.. toggle pausemode\n"
		"ναστες...... toggle mastermode\n"
		"λιγλ........ toggle kick mode\n"
		"δεατθνση.... toggle new messages\n"
		"ςπιγλυπ..... random team pickup\n"
		       "%s... toggle fallbunny\n", redtext("fallbanny"));
	} else {
		G_sprint(self, 2,
		"αβουτ...... kombat teams info\n"
		"ςυμεσ...... show game rules\n"
		"ςεαδω...... when you feel ready\n"
		"βςεαλ...... unready / vote matchend\n"
		"οπτιοξσ.... match control commands\n"
		"ριϊνο...... qizmo related commands\n"
		"νεσσαηεσ... fun message commands\n"
		"στατυσ..... show server settings\n"
		"στατυσ².... more server settings\n"
		"ςεσετ...... set defaults\n"
		"ζςεεϊε..... (un)freeze the map\n"
		"ναπσ....... list custom maps\n"
		"χθο........ player teamlist\n"
		"χθοξοτ..... players not ready\n"
		"μιστ....... whonot to everyone\n"
		"χθοσλιξ.... player skinlist\n"
		"χθοφοτε.... info on received votes\n"
		"πιγλυπ..... vote for pickup game\n"
		"στατσ...... show playerstats\n"
		"οφεςτινε... toggle overtime mode\n"
		"οφεςτινευπ. change overtime length\n"
		"γαπταιξ.... toggle captain election\n");
		
		if( atoi( ezinfokey( world, "k_admins" ) ) ) {
			G_sprint(self, 2, "αδνιξ...... toggle admin-mode\n");
			G_sprint(self, 2, "εμεγτ...... toggle admin election\n");
		}
	}
}

void SShowCmds()
{
	if( self->k_admin == 2 ) {
		G_sprint(self, 2, 
			"αδνιξ....... give up admin rights\n"
			"πςεχας...... playerfire before game\n"
			"μογλναπ..... (un)lock current map\n"
			"ζοςγεσταςτ.. force match to start\n"
			"ζοςγεβςεαλ.. force match to end\n"
			"ζοςγεπαυσε.. toggle pausemode\n"
			"ναστες...... toggle mastermode\n"
			"λιγλ........ toggle kick mode\n"
			"δεατθνση.... toggle new messages\n"
			"ςπιγλυπ..... random team pickup\n"
		           "%s... toggle fallbunny\n", redtext("fallbanny"));
		G_sprint(self, 2,
			"οφεςτινε.... toggle overtime mode\n"
			"οφεςτινευπ.. change overtime length\n"
			"you may also use\n"
			"τπ  δν  ποχεςυπσ  σιμεξγε\n"
			"τινευπ  τινεδοχξ  σπαχξ\n"
			"ρμαη    ρεξενω    ρποιξτ\n"
			"ςεσετ   ναπσ\n");
	}
	else {
		G_sprint(self, 2, 
			"στατυσ..... show match settings\n"
			"στατυσ².... more match settings\n"
			"γαν........ camera help text\n"
			"χθο........ player teamlist\n"
			"χθοξοτ..... players not ready\n"
			"χθοσλιξ.... player skinlist\n"
			"χθοφοτε.... info on received votes\n"
			"κοιξ....... joins game\n"				// FIXME: intersect with client command
			"στατσ...... show playerstats\n");

		if( atoi( ezinfokey( world, "k_admins" ) ) ) {
			G_sprint(self, 2, "αδνιξ...... toggle admin-mode\n");
			G_sprint(self, 2, "εμεγτ...... toggle admin election\n");
		}
	}
}

void ShowCmds()
{
	if ( self->k_spectator )
		SShowCmds();
	else 
		PShowCmds();
}

void ShowOpts()
{
	G_sprint(self, 2,
			"τινεδοχξ±.. -1 mins match time\n"
			"τινευπ±.... +1 mins match time\n"
			"τινεδοχξ... -5 mins match time\n"
			"τινευπ..... +5 mins match time\n"
			"ζςαησδοχξ.. -10 fraglimit\n"
			"ζςαησυπ.... +10 fraglimit\n"
			"δν......... change deathmatch mode\n"
			"τπ......... change teamplay mode\n"
			"δςοπρυαδ... drop quad when killed\n"
			"δςοπςιξη... drop ring when killed\n"
			"δςοππαγλ... drop pack when killed\n"
			"μογλ....... change locking mode\n"
			"σπαχξ...... change spawntype\n"
			"σπεεδ...... toggle sv_maxspeed\n"
			"ποχεςυπσ... quad, , ring & suit\n"
			"σπαχξ¶¶¶... 2 secs of  on respawn\n"
			"ζαιςπαγλσ.. best/last weapon dropped\n"
			"δισγθαςηε.. underwater discharges\n"
			"σιμεξγε.... toggle spectator talk\n"
			"βεςϊεςλ.... toggle berzerk mode\n");
}

void ShowQizmo()
{
	G_sprint(self, 2,
			"ρμαη....... lagsettings\n"
			"ρεξενω..... enemy vicinity reporting\n"
			"ρποιξτ..... point function\n");
}

// ShowMessages and SendMessage command implementations added
void ShowMessages()
{
	G_sprint(self, 2,
			"λιμμες..... who killed you last\n"
			"φιγτιν..... who you last killed\n"
			"ξεχγονες... last player joined\n");
}

void ShowVersion()
{
	G_sprint(self, 3, 
			"\nThis is Kombat Teams 2.21 (QVM version)\n"
			"by kemiKal, Cenobite, Sturm and Fang.\n\n"
			"Source, soundbanks, configs etc. at:\n"
			"http://hosted.barrysworld.com/kteam\n"); // FIXME: really? %)
}

void ChangeOvertime()
{
	float f1, f2;
//    char *tmp;

	if ( match_in_progress )
		return;

	if( atoi( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

    f1 = atoi( ezinfokey( world, "k_overtime" ) );
    f2 = atoi( ezinfokey( world, "k_exttime" ) );

    if( !f1 )
    {
		if( !f2 ) 
		{
			localcmd("localinfo k_exttime 1\n");
			f2 = 1;
		}

		G_bprint(2, "Οφεςτινε: time based\n");
		G_bprint(2, "Οφεςτινε μεξητθ ισ %d νιξυτε%s\n", (int)f2, ( f1== 1 ? "": "σ" ));

		localcmd("localinfo k_overtime 1\n");
    }
    else if( f1 == 1 )
	{
		G_bprint(2, "Οφεςτινε: sudden death\n");
		localcmd("localinfo k_overtime 2\n");
	}
    else if( f1 == 2 )
    {
		G_bprint(2, "Οφεςτινε: off\n");
		localcmd("localinfo k_overtime 0\n");
    }
}

void ChangeOvertimeUp ()
{
	float f1;
//	char *tmp;

	if( match_in_progress )
		return;

	if( atoi( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 )
	{
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	f1 = atoi( ezinfokey( world, "k_exttime" ) );

	if ( f1 == 10 )
		f1 = 1;
    else
		f1++;

	G_bprint(2, "Οφεςτινε μεξητθ σετ το %d νιξυτε%s\n", (int)f1, ( f1==1 ? "" : "σ" ) );

	localcmd("localinfo k_exttime %d\n", (int)f1);
}

void ListWhoNot()
{
	gedict_t *p;
//	char *tmp;
	float f1, f2;

	f1 = CountRPlayers();
	f2 = CountPlayers();
	if( f1 == f2 ) {
		G_sprint(self, 2, "All players ready\n");
		return;
	}

	if( !match_in_progress )
    {
		if( streq(self->s.v.classname, "player") && !self->ready ) 
		{
			G_sprint(self, 2, "Ready yourself first\n");
			return;
		}	

		if( g_globalvars.time < k_whonottime + 10 )
		{
            G_sprint(self, 2, "Only one μιστ in 10 seconds\n");
			return;
		}
		k_whonottime = g_globalvars.time;
		G_bprint(2, "Players ξοτ ready:\n");
		G_bprint(3, "\n");

		p = find( world, FOFCLSN, "player" );
		while( p ) {
			if(!strnull( p->s.v.netname ) && !p->ready) {
				G_bprint(2, "%s%s‘ %s is not ready\n",
					(p->k_admin == 2 ? " ": ""), ezinfokey(p, "team"), p->s.v.netname);
			}

			p = find( p, FOFCLSN, "player" );
		}
    }
	else
		G_sprint(self, 2, "Game in progress\n");
}


void SendKillerMsg()
{
	SendMessage(self->killer);
}

void SendVictimMsg()
{
	SendMessage(self->victim);
}

void SendNewcomerMsg()
{
	SendMessage(newcomer);
}

void SendMessage(char *name)
{
	gedict_t *p;

	if ( strnull( name ) )
		G_Error( "SendMessage null" );

	p = find( world, FOFCLSN, "player" );
	while( ( p && strneq( p->s.v.netname, name ) ) || p == self )
		p = find( p, FOFCLSN, "player" );

	if( p ) {
		G_bprint(3, "%s: %s", self->s.v.netname, ezinfokey(self, "premsg"));
		G_bprint(3, name);
		G_bprint(3,"%s\n", ezinfokey(self, "postmsg"));
	}
	else
		stuffcmd(self, "echo No name to display\n");
}

void ShowMaps()
{
	float f1;

	G_sprint(self, 2, "Vote for maps by typing the mapname,\nfor example \"δν΄\".\n");

	if ( strnull ( ezinfokey( world, va("%d", (int)LOCALINFO_MAPS_LIST_START) ) ) )
		return;

	G_sprint(self, 2, "\n---μιστ οζ γυστον ναπσ\n");

	f1 = LOCALINFO_MAPS_LIST_START;

	for( ; !strnull(ezinfokey(world, va("%d", (int)f1))) && f1 <= LOCALINFO_MAPS_LIST_END; f1++ )
	{
		G_sprint(self, 2, "%s%s",
				ezinfokey( world, va("%d", (int)f1) ), (((int)f1 & 1) ? "\n" : "   "));
	}

	if( (int)f1 & 1 )
		G_sprint(self, 2, "\n");

	G_sprint( self, 2, "---εξδ οζ μιστ\n" );
}

void PrintToggle1( char *tog, char *key )
{
	float f1;

	if ( strnull(tog) || strnull(key) )
		G_Error("PrintToggle1 null");

	G_sprint(self, 2, tog);

	f1 = atoi( ezinfokey( world, key ) );

	if( !f1 )
		G_sprint(self, 2, "Off ");
	else if ( f1 == 1 )
		G_sprint(self, 2, "On  ");
	else
		G_sprint(self, 2, "Jam ");
}

void PrintToggle2( char *tog, char *key )
{
	if ( strnull(tog) || strnull(key) )
		G_Error("PrintToggle2 null");

	G_sprint(self, 2, tog);

	if( atoi( ezinfokey( world, key ) ) )
		G_sprint(self, 2, "On\n");
	else
		G_sprint(self, 2, "Off\n");
}

//Ceno
void PrintToggle3(char *tog, char *key)
{
	float f1;

	if ( strnull(tog) || strnull(key) )
		G_Error("PrintToggle3 null");

	G_sprint(self, 2, tog);

	f1 = atoi( ezinfokey( world, key ) );

	if( !f1 )
		G_sprint(self, 2, "Off ");
	else if( f1 == 1 )
		G_sprint(self, 2, "On  ");
	else
		G_sprint(self, 2, "Lst ");
}

void ModStatus ()
{
	gedict_t *p;

	G_sprint(self, 2, "Ναψσπεεδ       %d\n", (int)k_maxspeed);
	G_sprint(self, 2, "Δεατθνατγθ     %d   ", (int)deathmatch);
	G_sprint(self, 2, "Τεανπμαω       %d\n", (int)teamplay);
	G_sprint(self, 2, "Τινεμινιτ      %3d ", (int)timelimit);
	G_sprint(self, 2, "Ζςαημινιτ      %d\n", (int)fraglimit);
	PrintToggle1("Ποχεςυπσ       ", "k_pow");
	PrintToggle2("Òεσπαχξ     ", "k_666");
	PrintToggle1("Δςοπ Ρυαδ      ", "dq");
	PrintToggle2("Δςοπ Òιξη      ", "dr");
	PrintToggle3("Ζαις Βαγλπαγλσ ", "k_frp");
	PrintToggle2("Δςοπ Βαγλπαγλσ ", "dp");
	PrintToggle1("Δισγθαςηε      ", "k_dis");
	PrintToggle2("Βεςϊεςλ νοδε   ", "k_bzk");

	if( match_in_progress == 1 ) {
		p = find(world, FOFCLSN, "timer" );
		G_sprint(self, 2, "The match will start in %d second%s\n",
						 (int)p->cnt2, ( p->cnt2 != 1 ? "s" : "" ));
		return;
	}

	if( k_velect )
		G_sprint(self, 2, "Election in progress\n"
						  "%d‘ votes received\n", (int) k_velect);

	if( k_captains == 2 )
		G_sprint(self, 2, "Team picking in progress\n");

	if( k_captains >= 1 && k_captains < 2 ) // FIXME: hmmm, may be this mean k_captains == 1 ??? 
		G_sprint(self, 2, "1‘ captain present\n");

	if( match_in_progress == 2 ) {
		if( k_sudden_death )
			G_sprint(self, 2, "Sudden death οφεςτινε ιξ πςοηςεσσ\n");
		else {
			p = find(world, FOFCLSN, "timer");
			if ( p )
				G_sprint(self, 2, "Match in progress\n"
								  "%d‘ full minute%s left\n",
									(int)p->cnt - 1, ( p->cnt != 1 ? "s": ""));
		}
	}
}

void ModStatus2()
{
//	char *tmp;
	float f1, f2;

	f1 = atoi( ezinfokey( world, "k_spw" ) );
	if( f1 == 2 )
		G_sprint(self, 2, "Λονβατ Τεανσ ςεσπαχξσ\n");
	else if ( f1 == 1 )
		G_sprint(self, 2, "ΛΤ ΣπαχξΣαζετω\n");
	else
		G_sprint(self, 2, "Ξοςναμ ΡΧ ςεσπαχξσ\n");

	if( atoi( ezinfokey( world, "k_duel" ) ) )
		G_sprint(self, 2, "Σεςφες νοδε: duel\n");
	else {
		G_sprint(self, 2, "Σεςφες νοδε: team\n");
		G_sprint(self, 2, "Σεςφες μογλιξη: %s\n", 
					(!lock ? "off" : (lock == 2 ? "all" : (lock == 1 ? "team" : "unknown"))));
	}

	if( !match_in_progress ) {
		G_sprint(self, 2, "Τεανιξζο (γυς: %d", (int)CountRTeams());
		G_sprint(self, 2, " νιξ: %s",    ezinfokey(world, "k_lockmin"));
		G_sprint(self, 2, " ναψ: %s)\n", ezinfokey(world, "k_lockmax"));
	}

	G_sprint(self, 2, "Σπεγταμλ: %s\n", ( atoi(ezinfokey(world, "k_spectalk")) ? "on" : "off" ));

	f1 = atoi( ezinfokey( world, "k_overtime" ) );
	f2 = atoi( ezinfokey( world, "k_exttime" ) );
	G_sprint(self, 2, "Οφεςτινε: %s", ( !f1 ? "off\n" : "" ));
	G_sprint(self, 2, "%s\n", (f1 == 1 ? va("%d minute%s", (int)f2, (f2 != 1 ? "s" : "")) : 
										    "sudden death"));

	f1 = atoi( ezinfokey( world, "fpd" ) );

	G_sprint(self, 2, "ΡιΪνο μαη: %s\n",             ( (int)f1 &   8 ? "off" : "on" ));
	G_sprint(self, 2, "ΡιΪνο τινεςσ: %s\n",          ( (int)f1 &   2 ? "off" : "on" ));
	G_sprint(self, 2, "ΡιΪνο εξενω ςεποςτιξη: %s\n", ( (int)f1 &  32 ? "off" : "on" ));
	G_sprint(self, 2, "ΡιΪνο ποιξτιξη: %s\n",        ( (int)f1 & 128 ? "off" : "on" ));

	f1 = atoi( ezinfokey( world, "k_deathmsg" ) );
	G_sprint(self, 2, "Γυστον δεατθνεσσαηεσ: %s\n", ( f1 ? "on" : "off" ));

	f1 = atoi(ezinfokey(world, "k_allowvoteadmin"));
	G_sprint(self, 2, "Αδνιξ εμεγτιοξ: %s\n", ( f1 ? "allowed" : "disallowed" ));

	G_sprint(self, 2, "Γθεγλ ζςανετινεσ: %s\n", ( framechecks ? "enabled" : "disabled" ));
}

void ModStatusVote()
{
	gedict_t *p;
//	char *s1;
	float f1, f2;

	if( match_in_progress == 2 ) {
		G_sprint(self, 2, "%d φοτε(σ) ζος στοππιξη\n", (int)k_vbreak);
	}
	else {
		f1 = 0;
		f2 = 0;
		p = find( world, FOFCLSN, "player" );
		while( p ) {
			if( !strnull( p->s.v.netname ) ) {
			 	if( p->k_pickup )
					f1++;
				if( p->k_vote == k_vbreak )
					f2++;
			}

			p = find( p, FOFCLSN, "player" );
		}


		G_sprint(self, 2, "%d φοτε(σ) ζος α πιγλυπ ηανε\n", (int)f1);

		if( k_vbreak )
			G_sprint(self, 2, "%d φοτε(σ) ζος ναπ %s\n", (int)f2, GetMapName( k_vbreak ));

		if( k_velect )
			G_sprint(self, 2, "%d φοτε(σ) ζος εμεγτιοξ\n", (int)k_velect);
	}

	G_sprint(self, 2, "--------------\n");
}

void PlayerStatus()
{
	gedict_t *p;
	char *tmp;

	if( !match_in_progress ) {
		p = find( world, FOFCLSN, "player" );
		while( p ) {
			if( !strnull( p->s.v.netname ) ) {
				if( p->k_admin == 2 )
					G_sprint(self, 2, "* ");

				G_sprint(self, 2, p->s.v.netname);

				if(p->ready) {
					tmp = ezinfokey( p, "team" );
					if( strnull( tmp ) )
						G_sprint(self, 2, " is ready\n");
					else
						G_sprint(self, 2, " is in team %s\n", tmp);
				}
				else
					G_sprint(self, 3, " is not ready\n"); // FIMXE: level 3 why?
			}

			p = find( p, FOFCLSN, "player" );
		}
		G_sprint(self, 2, "--------------\n");
	} else
		G_sprint(self, 2, "Game in progress\n");
}

void PlayerStatusS()
{
	gedict_t *p;
//	char *tmp;

	p = find( world, FOFCLSN, "player" );
	while( p ) {
		if( !strnull( p->s.v.netname ) )
			G_sprint(self, 2, "%s‘ %s\n", ezinfokey(p, "skin"), p->s.v.netname);

		p = find( p, FOFCLSN, "player" );
	}
	G_sprint(self, 2, "--------------\n");
}

void PlayerStatusN()
{
	gedict_t *p;
	float f1, f2;

	f1 = CountRPlayers();
	f2 = CountPlayers();
	if( f1 == f2 ) {
		G_sprint(self, 2, "All players ready\n");
		return;
	}

	if( match_in_progress )
		G_sprint(self, 2, "Game in progress\n");
	else {
		G_sprint(self, 2, "Players ξοτ ready:\n\n");
		p = find( world, FOFCLSN, "player" );
		while( p ) {
			if( !strnull( p->s.v.netname ) && !p->ready ) {
				if( p->k_admin == 2 )
					G_sprint(self, 2, " ");
				G_sprint(self, 2, "%s is not ready\n", p->s.v.netname);
			}

			p = find( p, FOFCLSN, "player" );
		}
	}
}

void ResetOptions()
{
	char *s1;

	if( match_in_progress )
		return;

	if(atoi( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	s1 = ezinfokey(self, "team");

#if 1 // TODO: make commented code safe or remove it
		localcmd("exec configs/reset.cfg\n");
#else
/*
	if( self->k_admin != 2 || strnull( s1 ) ) {
		localcmd("exec configs/reset.cfg\n");
	} else {
		localcmd("exec configs/%s.cfg\n", s1); // FIXME: UNSAFE: player can set some dangerous team
		G_bprint(2, "*** \"%s\" server setup by %s\n", s1, self->s.v.netname);
	}
*/
#endif
}

void VotePickup()
{
	gedict_t *p;
	float f1, f2;

	if( match_in_progress )
		return;

	if( k_captains ) {
		G_sprint(self, 2, "No pickup when captain stuffing\n");
		return;
	}

	if( atoi( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	self->k_pickup = 1 - self->k_pickup;

	G_bprint(2, "%s σαωσ %s\n",	self->s.v.netname, (self->k_pickup ? "pickup!!" : "no pickup"));

	f1 = 0;

	p = find( world, FOFCLSN, "player" );
	while( p ) {
		if( !strnull( p->s.v.netname ) && p->k_pickup )
			f1++;

		p = find( p, FOFCLSN, "player" );
	}

	f2 = CountPlayers();
	f2 = ( floor( f2 / 2 ) ) + 1;
	if( self->k_admin < 2 ) {
		if( f1 < f2 )
			return;
		G_bprint(3, "console: a pickup game it is then\n");
	}
	else
		G_bprint(3, "console: admin veto for pickup\n");

	p = find( world, FOFCLSN, "player" );
	while( p ) {
		if( !strnull( p->s.v.netname ) ) {
			stuffcmd(p, "break\n"
						"color 0\n"
						"team \"\"\n"
						"skin base\n");
			p->k_pickup = 0;
		}

		p = find( p, FOFCLSN, "player" );
	}
}

void ReportMe()
{
	gedict_t *p;
	char *t1, *t2 /*, *h */, *at /* , *av */, *wt /* , *wa */, *pa1, *pa2;
	float f1, flag = 0;

	if( !strnull( ezinfokey(self, "k_nick") ) )
		flag = 1;


//	pa1 = "¨";
//	pa2 = "): ";
	pa1 = "";    // qqshka: look better, IMO
	pa2 = ": ";

	if(	(int)self->s.v.items & 8192 )
		at = "ga:";
	if( (int)self->s.v.items & 16384 )
		at = "ya:";
	if( (int)self->s.v.items & 32768 )
		at = "ra:";

	wt = "axe:";
	f1 = 0;

	if( (int)self->s.v.items & 1 ) {
		wt = "sg:";
		f1 = self->s.v.ammo_shells;
	}
	if( (int)self->s.v.items & 4) {
		wt = "ng:";
		f1 = self->s.v.ammo_nails;
	}
	if( (int)self->s.v.items & 2) {
		wt = "ssg:";
		f1 = self->s.v.ammo_shells;
	}
	if( (int)self->s.v.items & 8) {
		wt = "sng:";
		f1 = self->s.v.ammo_nails;
	}
	if( (int)self->s.v.items & 16) {
		wt = "gl:";
		f1 = self->s.v.ammo_rockets;
	}
	if( (int)self->s.v.items & 64) {
		wt = "lg:";
		f1 = self->s.v.ammo_cells;
	}
	if( (int)self->s.v.items & 32) {
		wt = "rl:";
		f1 = self->s.v.ammo_rockets;
	}

    p = find( world, FOFCLSN, "player" );
	while( p ) {
		if( !strnull( p->s.v.netname ) ) {
			t1 = ezinfokey(self, "team");
			t2 = ezinfokey(p, "team");

			if( streq( t1, t2 ) ) {
				if( flag ) {
					t1 = ezinfokey(self, "k_nick");
					G_sprint(p, 3, "%s: ", t1);
				}
				else
					G_sprint(p, 3, "%s%s%s", pa1, self->s.v.netname, pa2);

				if( self->s.v.armorvalue )
					G_sprint(p, 3, "%s%d", at, (int)self->s.v.armorvalue);
				else
					G_sprint(p, 3, "a:0");

				G_sprint(p, 3, "  h:%d  %s%d", (int)self->s.v.health, wt, (int)f1);

				if( (int)self->s.v.items & 524288)
					G_sprint(p, 3, "  …εωεσ…‘");
				if( (int)self->s.v.items & 1048576)
					G_sprint(p, 3, "  ……‘");
				if( (int)self->s.v.items & 4194304)
					G_sprint(p, 3, "  …ρυαδ‘");

				G_sprint(p, 3, "\n");
			}
		}

		p = find( p, FOFCLSN, "player" );
	}
}

void ToggleRespawns()
{
	float tmp;

	if ( match_in_progress )
		return;

	if( atoi( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	tmp = atoi( ezinfokey( world, "k_spw" ) );

	if( tmp == 2 ) {
		G_bprint(2, "Normal QW respawns (avoid spawnfrags)\n");
		localcmd("localinfo k_spw 0\n");
	} else if( tmp == 0 ) {
		G_bprint(2, "KT SpawnSafety\n");
		localcmd("localinfo k_spw 1\n");
	} else {
		G_bprint(2, "Kombat Teams respawns\n");
		localcmd("localinfo k_spw 2\n");
	}
}

void ToggleRespawn666()
{
	float tmp;

	if( match_in_progress )
		return;

	if( atoi( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	tmp = atoi( ezinfokey( world, "k_666" ) );

	G_bprint(2, "Òεσπαχξ  ");

	if( tmp != 0 ) {
		G_bprint(2, "disabled\n");
		localcmd("localinfo k_666 0\n");
	} else {
		G_bprint(2, "enabled\n");
		localcmd("localinfo k_666 1\n");
	}
}

void TogglePowerups()
{
	float tmp;
	gedict_t *p, *old;

	if ( match_in_progress )
		return;
	if ( atoi( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	tmp = atoi( ezinfokey( world, "k_pow" ) );

	G_bprint(2, "Ποχεςυπσ ");

	if( tmp == 2 ) {
		G_bprint(2, "disabled\n");
		localcmd("localinfo k_pow 0\n");
	    p = findradius(world, VEC_ORIGIN, 999999);
		while(p) {
			old = findradius(p, VEC_ORIGIN, 999999);;

			if( streq(p->s.v.classname, "item_artifact_invulnerability" ) )
				p->s.v.effects = p->s.v.effects - ((int)p->s.v.effects & 128);
			if( streq(p->s.v.classname, "item_artifact_super_damage" ) )
				p->s.v.effects = p->s.v.effects - ((int)p->s.v.effects & 64);

			p = old;
		}
	} else if( !tmp ) {
		G_bprint(2, "enabled\n");
		localcmd("localinfo k_pow 1\n");
	    p = findradius(world, VEC_ORIGIN, 999999);
		while(p) {
			old = findradius(p, VEC_ORIGIN, 999999);;

			if( streq( p->s.v.classname, "item_artifact_invulnerability" ) )
				p->s.v.effects = (int)p->s.v.effects | 128;
			if( streq( p->s.v.classname, "item_artifact_super_damage" ) )
				p->s.v.effects = (int)p->s.v.effects | 64;

			p = old;
		}
	} else {
		G_bprint(2, "enabled (timer jammer)\n");
		localcmd("localinfo k_pow 2\n");
	}
}

void ToggleDischarge()
{
	float tmp;

	if( match_in_progress )
		return;
	if( atoi( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	tmp = atoi( ezinfokey( world, "k_dis" ) );

	G_bprint(2, "Δισγθαςηεσ ");

	if( tmp != 0 ) {
		G_bprint(2, "disabled\n");
		localcmd("localinfo k_dis 0\n");
	} else {
		G_bprint(2, "enabled\n");
		localcmd("localinfo k_dis 1\n");
	}
}

#define DMM_NUM		((char)(146) + (char)(deathmatch))

void ShowDMM()
{
	G_sprint(self, 2, "Deathmatch %c\n", DMM_NUM);
}

void ChangeDM(float dmm)
{
	if ( match_in_progress )
		return;

	if ( atoi ( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	if ( deathmatch == (int)dmm ) {
		G_sprint(self, 2, "%s%c already set\n", redtext("dmm"), DMM_NUM);
		return;
	}

	deathmatch = bound(1, (int)dmm, 5);

	cvar_set("deathmatch", va("%d", (int)deathmatch));

	G_bprint(2, "Deathmatch %c\n", DMM_NUM);
}

void ChangeTP()
{
	char *tmp;

	if ( match_in_progress )
		return;
	if ( atoi ( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	if( atoi( ezinfokey( world, "k_duel" ) ) ) {
		G_sprint(self, 3, "console: duel mode disallows you to change teamplay setting\n");
		return;
	}

	teamplay++;

	if ( teamplay == 4 )
		teamplay = 0;

	cvar_set("teamplay", va("%d", (int)teamplay));
	if( teamplay == 0 )      tmp = "’";
	else if( teamplay == 1 ) tmp = "“";
	else if( teamplay == 2 ) tmp = "”";
	else tmp = "•";

	G_bprint(2, "Teamplay %s\n", tmp);
}

void TimeDown(float t)
{
//	char *tmp;

	if ( match_in_progress )
		return;
	if ( atoi ( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	timelimit = timelimit - t;

	if ( timelimit < 5 )
		timelimit = 5;

	cvar_set("timelimit", va("%d", (int)timelimit));

	G_bprint(2, "Νατγθ μεξητθ σετ το %d νιξυτεσ\n", (int)timelimit);
}

void TimeUp(float t)
{
//	char *tmp;
	float top;

	if ( match_in_progress )
		return;
	if ( atoi ( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	timelimit = timelimit + t;

	top = atoi( ezinfokey( world, "k_timetop" ) );
	if( timelimit > top )
		timelimit = top;

	cvar_set("timelimit", va("%d", (int)timelimit));

	G_bprint(2, "Νατγθ μεξητθ σετ το %d νιξυτεσ\n", (int)timelimit);
}

void TimeSet(float t)
{
//	char *tmp;


	float top;

	if ( match_in_progress )
		return;
	if ( atoi ( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	timelimit = t;

	top = atoi( ezinfokey( world, "k_timetop" ) );
	if( timelimit > top )
		timelimit = top;

	cvar_set("timelimit", va("%d", (int)timelimit));

	G_bprint(2, "Νατγθ μεξητθ σετ το %d νιξυτεσ\n", (int)timelimit);
}

void FragsDown()
{
	if ( match_in_progress )
		return;
	if ( atoi ( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	fraglimit -= 10;

	if( fraglimit < 0 )
		fraglimit = 0;

	cvar_set("fraglimit", va("%d", (int)(fraglimit)));
	G_bprint(2, "Ζςαημινιτ σετ το %d\n", (int)(fraglimit));
}

void FragsUp()
{
//	char *tmp;

	if ( match_in_progress )
		return;
	if ( atoi ( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	fraglimit += 10;

	if(fraglimit > 100)
		fraglimit = 100;

	cvar_set("fraglimit", va("%d", (int)(fraglimit)));
	G_bprint(2, "Ζςαημινιτ σετ το %d\n", (int)(fraglimit));
}

void ToggleDropQuad()
{
	float dq;

	if ( match_in_progress )
		return;
	if ( atoi ( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	dq = atoi( ezinfokey( world, "dq" ) );

	if(dq != 0) {
		localcmd("localinfo dq 0\n");
		G_bprint(2, "ΔςοπΡυαδ off\n");
		return;
	}
	localcmd("localinfo dq 1\n");
	G_bprint(2, "ΔςοπΡυαδ on\n");
}

void ToggleDropRing()
{
	float dr;

	if ( match_in_progress )
		return;
	if ( atoi ( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	dr = atoi( ezinfokey( world, "dr" ) );

	if( dr ) {
		localcmd("localinfo dr 0\n");
		G_bprint(2, "ΔςοπÒιξη off\n");
		return;
	}
	localcmd("localinfo dr 1\n");
	G_bprint(2, "ΔςοπÒιξη on\n");
}

void ToggleDropPack()
{
	float dp;

	if ( match_in_progress )
		return;
	if ( atoi ( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	dp = atoi( ezinfokey( world, "dp" ) );

	if(dp != 0) {
		localcmd("localinfo dp 0\n");
		G_bprint(2, "ΔςοπΠαγλσ off\n");
		return;
	}
	localcmd("localinfo dp 1\n");
	G_bprint(2, "ΔςοπΠαγλσ on\n");
}

void ToggleFairPacks()
{
	float f1;

    if ( match_in_progress )
		return;
	if ( atoi ( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	f1 = atoi( ezinfokey( world, "k_frp" ) );

	if( !f1)
	{
		localcmd("localinfo k_frp 1\n");
		G_bprint(2, "Ζαιςπαγλσ enabled - drop best weapon.\n");
		return;
	}
	else if( f1==1 )
	{
		localcmd("localinfo k_frp 2\n");
		G_bprint(2, "Ζαιςπαγλσ enabled - drop last fired weapon.\n");
		return;
	}
	else if( f1==2 )
	{
		localcmd("localinfo k_frp 0\n");
		G_bprint(2, "Ζαιςπαγλσ disabled\n");
		return;
	}
}

void ToggleSpeed()
{
//	char *s1;
	gedict_t *p;

	if ( match_in_progress )
		return;
	if ( atoi ( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	if( k_maxspeed != 320 )
		k_maxspeed = 320;
	else
		k_maxspeed = atoi( ezinfokey( world, "k_highspeed" ) );

	G_bprint(2, "Ναψσπεεδ σετ το %d\n", (int)k_maxspeed);
	cvar_set("sv_maxspeed", va("%d", (int)k_maxspeed));

	p = find( world, FOFCLSN, "player" );
	while( p ) {
		p->maxspeed = k_maxspeed;
		p = find( p, FOFCLSN, "player" );
	}
}

void ToggleBerzerk()
{
	float tmp;

	if ( match_in_progress )
		return;
	if ( atoi ( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	tmp = atoi( ezinfokey( world, "k_bzk" ) );

	if( tmp != 0 ) {
		localcmd("localinfo k_bzk 0\n");
		G_bprint(2, "ΒεςΪεςλ νοδε off\n");
		return;
	}
	localcmd("localinfo k_bzk 1\n");
	G_bprint(2, "ΒεςΪεςλ νοδε on\n");
}

#ifdef VWEP_TEST
void ToggleVwep ()
{
    float tmp;

    if ( match_in_progress )
        return;
    if ( atoi ( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
        G_sprint(self, 3, "console: command is locked\n");
        return;
    }
    tmp = stof(ezinfokey(world, "vwep"));
    if(tmp != 0) {
        localcmd("localinfo vwep 0\n");
        G_bprint(2, "vwep νοδε off\nrequires map restart\n");   //TODO: change vwep for red text version
        return;
    }
    localcmd("localinfo vwep 1\n");
    G_bprint(2, "vwep νοδε on\nrequires map restart\n");
}
#endif

void ToggleSpecTalk()
{
	float tmp, tmp2;
//	char *s1;

	if ( match_in_progress && self->k_admin < 2 )
		return;

	tmp2 = atoi( ezinfokey( world, "fpd" ) );

	tmp2 = tmp2 - ((int)tmp2 & 64);

	if( match_in_progress == 2 ) {
		tmp = atoi( ezinfokey( world, "k_spectalk" ) );
		G_bprint(2, "Spectalk ");

		if(tmp != 0) {
			tmp2 = tmp2 + 64;

			localcmd("sv_spectalk 0\n");
			localcmd("serverinfo fpd %d\n", (int)tmp2);
			localcmd("localinfo k_spectalk 0\n");
			G_bprint(2, "off: πμαωεςσ γαξ ξο μοξηες θεας σπεγτατοςσ\n");
			return;
		}
		localcmd("sv_spectalk 1\n");
		localcmd("serverinfo fpd %d\n", (int)tmp2);
		localcmd("localinfo k_spectalk 1\n");
		G_bprint(2, "on: πμαωεςσ γαξ ξοχ θεας σπεγτατοςσ\n");
		return;
	} else {
		if ( atoi ( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
			G_sprint(self, 3, "console: command is locked\n");
			return;
		}

		tmp = atoi( ezinfokey( world, "k_spectalk" ) );
		G_bprint(2, "Spectalk ");

		if(tmp != 0) {
			localcmd("localinfo k_spectalk 0\n");
			G_bprint(2, "off: πμαωεςσ γαξξοτ θεας σπεγτατοςσ δυςιξη ηανε\n");
			return;
		}
		localcmd("localinfo k_spectalk 1\n");
		G_bprint(2, "on: πμαωεςσ γαξ θεας σπεγτατοςσ δυςιξη ηανε\n");
	}
}

void ShowRules()
{
	if( atoi( ezinfokey( world, "k_duel" ) ) )
		G_sprint(self, 2, "Server is in duelmode.\n");
	else
		G_sprint(self, 2,
			"Bind ςεποςτ to a key.\n"
			"Pressing that key will send\n"
			"your status to your teammates.\n"
			"Typing σγοςεσ during game\n"
			"will print time left and teamscores.\n"
			"Also available during game\n"
			"are στατσ and εζζιγιεξγω.\n\n"
			"Remember that telefragging a teammate\n"
			"does not result in frags.\n");

	if( atoi( ezinfokey( world, "k_bzk" ) ) != 0 )
		G_sprint(self, 2,
			"\nBERZERK mode is activated!\n"
			"This means that when only %d seconds\n"
			"remains of the game, all players\n"
			"gets QUAD/OCTA powered.\n", atoi( ezinfokey(world, "k_btime") ) );

	G_sprint(self, 2, "\n");
}

void ChangeLock()
{
	if ( match_in_progress )
		return;
	if ( atoi ( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	lock++;

    if( lock > 2 )
		lock = 0;
	if( lock == 0 )
		G_bprint(2, "Σεςφες μογλιξη off\n");
	else if( lock == 2 )
		G_bprint(2, "Σεςφες μογλεδ - players cannot connect during game\n");
	else if( lock == 1 )
		G_bprint(2, "Τεανμογλ οξ - only players in existing teams can connect during game\n");
}

void TeamSay(float fsndname)
{
	gedict_t *p;
	char *t1, *t2;

	char *sndname = *(char **) &fsndname; // oh, thanks Tonik %)

    p = find( world, FOFCLSN, "player" );
	while( p ) {
		if( p != self && teamplay && !strnull( p->s.v.netname ) &&
			 ( atoi( ezinfokey( p, "k_flags" ) ) & 1 ) ) {
			t1 = ezinfokey(self, "team");
			t2 = ezinfokey(p, "team");

			if( streq( t1, t2 ) ) {
				stuffcmd(p, "play ");
				t1 = ezinfokey(p, "k_sdir");

				if( !strnull( t1 ) )
					stuffcmd(p, "%s/", t1);
				stuffcmd(p, "%s\n", sndname);
			}
		}

		p = find( p, FOFCLSN, "player" );
	}
}

void PrintScores()
{
	float f1 = 0, f2;
	char *tmp, *tmp2;
	gedict_t *p;


	if( match_in_progress != 2 ) {
		G_sprint(self, 2, "no game - no scores.\n");
		return;
	}
	
	// Check to see if scores changed from last time you hit scores command.
	// DO this by checking if k_nochange is 0. 
	// which is set in ClientObituary in client.qc
	if( k_showscores && k_nochange == 0 )
    {
		k_scores1 = 0;
		k_scores2 = 0;

		p = find( world, FOFCLSN, "player" );
		while( p ) {
			if( !strnull( p->s.v.netname ) && p->k_accepted == 2 ) {
				f1++;
				tmp  = ezinfokey(world, "k_team1");
				tmp2 = ezinfokey(p, "team");
				if( streq( tmp, tmp2) )
					k_scores1 += p->s.v.frags;
				else
					k_scores2 += p->s.v.frags;
			}

			p = find( p, FOFCLSN, "player" );
		}

		p = find(world, FOFCLSN, "ghost");
		while( p ) {
			tmp2 = ezinfokey(world, va("%d", (int)p->k_teamnum));
			tmp  = ezinfokey(world, "k_team1");
			if( streq (tmp, tmp2) )
				k_scores1 += p->s.v.frags;
			else
				k_scores2 += p->s.v.frags;

			p = find(p, FOFCLSN, "ghost");
		}

		k_nochange = 1;
	}

	if( k_sudden_death )
		G_sprint(self, 2, "Sudden death οφεςτινε ιξ πςοηςεσσ\n");

	p = find(world, FOFCLSN, "timer");
	if( p ) {
		f1 = p->cnt;
		f2 = p->cnt2;
		if( f2 == 60 )
			f2 = 0;
		else
			f1--;

		if( f1 )
			G_sprint(self, 2, "%d‘ full minute%s", (int)f1, ( f1 > 1 ? "s" : ""));
		else
			G_sprint(self, 2, "%d‘ second%", (int)f2, ( f2 > 1 ? "s" : ""));

		G_sprint(self, 2, " left\n");
	}

	if( k_showscores ) {
		if( k_scores1 > k_scores2 ) {
			G_sprint(self, 2, "Τεαν %s‘ = %d\n", ezinfokey(world, "k_team1"), (int)k_scores1);
			G_sprint(self, 2, "Τεαν %s‘ = %d\n", ezinfokey(world, "k_team2"), (int)k_scores2);
		} else {
			G_sprint(self, 2, "Τεαν %s‘ = %d\n", ezinfokey(world, "k_team2"), (int)k_scores2);
			G_sprint(self, 2, "Τεαν %s‘ = %d\n", ezinfokey(world, "k_team1"), (int)k_scores1);
		}
	}
}


void PlayerStats()
{
	float f1;
	gedict_t *p, *p2;
	char *tmp, *tmp2;

	if( match_in_progress != 2 ) {
		G_sprint(self, 2, "no game - no statistics.\n");
		return;
	}

	f1 = CountTeams();
	G_sprint(self, 2, "Πμαωες στατιστιγσ:\n"
					  "Ζςαησ (ςαξλ) ");
	if( !atoi( ezinfokey( world, "k_duel" ) ) && f1 && teamplay )
		G_sprint(self, 2, "ζςιεξδλιμμσ ");

	G_sprint(self, 2, " εζζιγιεξγω\n");

	if( !atoi( ezinfokey( world, "k_duel" ) ) && f1 && teamplay )
		G_sprint(self, 2, "");

	G_sprint(self, 2, "\n");

	p = find( world, FOFCLSN, "player" );
	while( p ) {
		if( !strnull( p->s.v.netname ) && p->k_accepted == 2 && p->ready ) {
			p2 = p;

			while ( p2 ) {
				tmp  = ezinfokey(p, "team");
				tmp2 = ezinfokey(p2, "team");

				if( streq( tmp, tmp2 ) ) {
					G_sprint(self, 2, "%s‘ %s:  %d(%d) ", tmp2, p2->s.v.netname,
						(int)p2->s.v.frags, (int)(p2->s.v.frags - p2->deaths));

					if( !atoi( ezinfokey(world, "k_duel" ) ) && f1 && teamplay )
						G_sprint(self, 2, "%d ", (int)p2->friendly);

					if(p2->s.v.frags < 1)
						p2->efficiency = 0;
					else
						p2->efficiency = p2->s.v.frags / (p2->s.v.frags + p2->deaths) * 100;

					if( floor( p2->efficiency ) == p2->efficiency)
							G_sprint(self, 2, " ");
					else
						G_sprint(self, 2, "");
					G_sprint(self, 2, "%3.1f%%\n", p2->efficiency);

					p2->ready = 0;
				}

				p2 = find(p2, FOFCLSN, "player");
			}
		}

		p = find( p, FOFCLSN, "player" );
	}

	p = find( world, FOFCLSN, "player" );
	while( p ) {
		p->ready = 1;

		p = find( p, FOFCLSN, "player" );
	}
}

void ToggleQLag()
{
	float f1, f2;
//	char *s1;

	if ( match_in_progress )
		return;
	if ( atoi ( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	f1 = atoi( ezinfokey( world, "fpd" ) );

	f2 = f1 - ((int)f1 & 8);

	if((int)f1 & 8) {
		localcmd("serverinfo fpd %d\n", (int)f2);
		G_bprint(2, "ΡιΪνο μαη σεττιξησ in effect\n");

		return;
	} else {
		f2 = (int)f2 | 8;
		localcmd("serverinfo fpd %d\n", (int)f2);
		G_bprint(2, "ΡιΪνο μαη σεττιξησ not in effect\n");
	}
}

void ToggleQEnemy()
{
	float f1, f2;
//	char *s1;

	if ( match_in_progress )
		return;
	if ( atoi ( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	f1 = atoi( ezinfokey( world, "fpd" ) );

	f2 = f1 - ((int)f1 & 32);

	if( (int)f1 & 32) {
		localcmd("serverinfo fpd %d\n", (int)f2);
		G_bprint(2, "ΡιΪνο εξενω ςεποςτιξη allowed\n");

		return;
	} else {
		f2 = (int)f2 | 32;
		localcmd("serverinfo fpd %d\n", (int)f2);
		G_bprint(2, "ΡιΪνο εξενω ςεποςτιξη disallowed\n");
	}
}

void ToggleQPoint()
{
	float f1, f2;
//	char *s1;

	if ( match_in_progress )
		return;
	if ( atoi ( ezinfokey( world, "k_master" ) ) && self->k_admin < 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	f1 = atoi( ezinfokey( world, "fpd" ) );

	f2 = f1 - ((int)f1 & 128);

	if( (int)f1 & 128 ) {
		localcmd("serverinfo fpd %d\n", (int)f2);
		G_bprint(2, "ΡιΪνο ποιξτιξη enabled\n");

		return;
	} else {
		f2 =  (int)f2 | 128;
		localcmd("serverinfo fpd %d\n", (int)f2);
		G_bprint(2, "ΡιΪνο ποιξτιξη disabled\n");
	}
}

void ToggleFreeze()
{
	float f1;

	if ( match_in_progress )
		return;
	if( atoi( ezinfokey( world, "k_master" ) ) && self->k_admin != 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	f1 = atoi( ezinfokey( world, "k_freeze" ) );

	if( f1 ) {
		localcmd("localinfo k_freeze 0\n");
		G_bprint(2, "%s unfreezes map\n", self->s.v.netname);
	} else {
		localcmd("localinfo k_freeze 1\n");
		G_bprint(2, "%s freezes map\n", self->s.v.netname);
	}
}



float CaptainImpulses()
{
// s: return 0 if captain mode is set and no captain things were entered
	if(k_captains == 2 && self->s.v.impulse != 79) {
		if(self->s.v.impulse > 16 || !self->k_captain) return 0;
// s: return 1 if it's a player picker impulse
		return 1;
	}
// s: return 2 if nothing interesting
	return 2;
}

// qqshka: pointing code stolen from Zquake

void ShowNick()
{
	gedict_t	*p, *bp = NULL;
	char		*s1, *s2, *pups, *kn, ln[256];
	float		best;
	vec3_t		forward, right, up;
	vec3_t		ang;
	vec3_t		vieworg, entorg;
	int			itms, i;

	if ( !teamplay )
		return;

	ang[0] = self->s.v.v_angle[0];
	ang[1] = self->s.v.v_angle[1];
	ang[2] = 0;

	AngleVectors (ang, forward, right, up);

	VectorCopy (self->s.v.origin, vieworg);
	vieworg[2] += 16 /* was 22 */;	// adjust for view height

	best = -1;

	s1 = ezinfokey ( self, "team" );

	p = find(world, FOFCLSN, "player");

	for ( ; p ; p = find(p, FOFCLSN, "player") )
	{
		vec3_t	v, v2, v3;
		float dist, miss, rank;

		if ( p->s.v.health <= 0 )
			continue; // ignore dead

		if ( p == self )
			continue; // ignore self

		if ( strnull( p->s.v.netname ) || p->k_accepted != 2 )
			continue; // ignore not really players

		if ( p->s.v.modelindex != modelindex_player )
			continue; // ignore non player mdl index (EYES)

		s2 = ezinfokey ( p, "team" );

		if ( strneq( s1, s2 ) )
			continue; // ignore non teammaters


		VectorCopy (p->s.v.origin, entorg);
		entorg[2] += 30;
		VectorSubtract (entorg, vieworg, v);

		dist = DotProduct (v, forward);
		if ( dist < 10 )
			continue;

		VectorScale (forward, dist, v2);
		VectorSubtract (v2, v, v3);
		miss = VectorLength (v3);
		if (miss > 300)
			continue;
		if (miss > dist*1.7)
			continue;		// over 60 degrees off
		if (dist < 3000.0/8.0)
			rank = miss * (dist*8.0*0.0002f + 0.3f);
		else
			rank = miss;
		
		if (rank < best || best < 0) {
			// check if we can actually see the object
			vec3_t	end;
			float	radius;

			radius = 27;
			if ((int)p->s.v.effects & (EF_BLUE|EF_RED|EF_DIMLIGHT|EF_BRIGHTLIGHT))
				radius = 200;

			if (dist <= radius)
				goto ok;

			// FIXME: is it ok to use PM_TraceLine here?
			// physent list might not have been built yet...

			VectorSubtract (vieworg, entorg, v);
			VectorNormalize (v);
			VectorMA (entorg, radius, v, end);
			traceline( PASSVEC3( vieworg ), PASSVEC3( end ), true, self );
			if ( g_globalvars.trace_fraction == 1 )
				goto ok;

			VectorMA (entorg, radius, right, end);
			VectorSubtract (vieworg, end, v);
			VectorNormalize (v);
			VectorMA (end, radius, v, end);
			traceline( PASSVEC3( vieworg ), PASSVEC3( end ), true, self );
			if ( g_globalvars.trace_fraction == 1 )
				goto ok;

			VectorMA (entorg, -radius, right, end);
			VectorSubtract (vieworg, end, v);
			VectorNormalize (v);
			VectorMA (end, radius, v, end);
			traceline( PASSVEC3( vieworg ), PASSVEC3( end ), true, self );
			if ( g_globalvars.trace_fraction == 1 )
				goto ok;

			VectorMA (entorg, radius, up, end);
			VectorSubtract (vieworg, end, v);
			VectorNormalize (v);
			VectorMA (end, radius, v, end);
			traceline( PASSVEC3( vieworg ), PASSVEC3( end ), true, self );
			if ( g_globalvars.trace_fraction == 1 )
				goto ok;

			// use half the radius, otherwise it's possible to see
			// through floor in some places
			VectorMA (entorg, -radius/2, up, end);
			VectorSubtract (vieworg, end, v);
			VectorNormalize (v);
			VectorMA (end, radius, v, end);
			traceline( PASSVEC3( vieworg ), PASSVEC3( end ), true, self );
			if ( g_globalvars.trace_fraction == 1 )
				goto ok;


			continue;	// not visible
ok:
			best = rank;
			bp = p;
		}
	}

	if ( best < 0 || !bp )
		return;

	itms = (int)bp->s.v.items;

	pups = "";

	if ( bp->invincible_finished >= g_globalvars.time )
		pups = va("%s\x90%s\x91", ( *pups ? va("%s ", pups) : ""), redtext("Pent"));
	if ( bp->super_damage_finished > g_globalvars.time )
		pups = va("%s\x90%s\x91", ( *pups ? va("%s ", pups) : ""), redtext("Quad"));
	if ( bp->radsuit_finished > g_globalvars.time )
		pups = va("%s\x90%s\x91", ( *pups ? va("%s ", pups) : ""), redtext("Suit"));

	if ( *pups )
		pups = va("%s\n", pups);

	if      ( itms & IT_ARMOR1 ) s1 = va("%s:%d ", redtext("ga"), (int)bp->s.v.armorvalue);
	else if ( itms & IT_ARMOR2 ) s1 = va("%s:%d ", redtext("ya"), (int)bp->s.v.armorvalue);
	else if ( itms & IT_ARMOR3 ) s1 = va("%s:%d ", redtext("ra"), (int)bp->s.v.armorvalue);
	else
		s1 = "";

	if      ( itms & IT_ROCKET_LAUNCHER )  s2 = va(" %s:%d", redtext("rl"), (int)bp->s.v.ammo_rockets);
	else if ( itms & IT_LIGHTNING )        s2 = va(" %s:%d", redtext("lg"), (int)bp->s.v.ammo_cells);
	else if ( itms & IT_GRENADE_LAUNCHER ) s2 = va(" %s:%d", redtext("gl"), (int)bp->s.v.ammo_rockets);
	else if ( itms & IT_SUPER_NAILGUN )    s2 = va(" %s:%d", redtext("sng"),(int)bp->s.v.ammo_nails);
	else if ( itms & IT_SUPER_SHOTGUN )    s2 = va(" %s:%d", redtext("ssg"),(int)bp->s.v.ammo_shells);
	else if ( itms & IT_NAILGUN )          s2 = va(" %s:%d", redtext("ng"), (int)bp->s.v.ammo_nails);
	else if ( itms & IT_SHOTGUN )          s2 = va(" %s:%d", redtext("sg"), (int)bp->s.v.ammo_shells);
	else if ( itms & IT_AXE )              s2 = redtext(" axe!");
	else
		s2 = "";

	kn = ezinfokey( bp, "k_nick" );

	i = bound(0, atoi ( ezinfokey( self, "ln" ) ), sizeof(ln)-1 );
	memset( (void*)ln, (int)'\n', i);
	ln[i] = 0;

	G_centerprint( self,	"%s"
							"%s" // if powerups present \n is too
						"%s%s:%d%s\n"
							"%s\n", ln
								, pups,
						 s1, redtext("h"), (int)bp->s.v.health, s2,
								( strnull( kn ) ? bp->s.v.netname : kn ));

	self->shownick_time = g_globalvars.time + 0.8; // clear centerprint at this time
}

// qqshka

// below predefined settings for usermodes
// I ripped this from ktpro

// common settings for all user modes
const char common_um_init[] =			
	"localinfo k_new_mode 0\n" 			// UNKNOWN ktpro                                                 
	"localinfo k_fast_mode 0\n"			// UNKNOWN ktpro                                                 
	"localinfo matrix 0\n"              // UNKNOWN ktpro                                                 
	"localinfo k_safe_rj 0\n"           // UNKNOWN ktpro                                                 
	                                                                                                     
	"localinfo spec_info 1\n"			// TODO not implemented yet                                      
										// allow spectators receive took info during game                
                                   		// (they have to use "moreinfo" command to set proper level)     
	"localinfo spec_info_notlock 1\n"	// allow all spectators receive it (0 = only admins)             
	"localinfo k_midair 0\n"			// not implemented                                               
	"localinfo k_instagib 0\n"			// not implemented                                               
	                                                                                                     
	"localinfo k_no_lg 0\n"				// TODO not implemented                                          
	                                                                                                     
	"fraglimit 0\n"						// fraglimit %)                                                  
	"localinfo k_666 0\n"				// respawn 666                                                   
	"localinfo dp 1\n"					// drop pack                                                     
	"localinfo dq 0\n"					// drop quad                                                     
	"localinfo dr 0\n"					// drop ring                                                     
	"localinfo k_frp 0\n"				// fairpacks                                                     
	"localinfo k_spectalk 0\n"			// silence                                                       
	"localinfo k_dis 1\n"				// discharge on                                                  
	"localinfo k_lockmin 2\n"			// minimum number of teams in game                               
	"localinfo k_bzk 0\n"				// berzerk                                                       
	"localinfo k_spw 0\n"				// affect spawn type                                             
	"localinfo k_new_spw 0\n";			// ktpro feature                                                 

const char _1on1_um_init[] =
	"timelimit 10\n"					//
	"teamplay 0\n"						//
	"deathmatch 3\n"					//
	"localinfo k_deathmatch 3\n"		// TODO not implemented
	"localinfo k_membercount 1\n"		// minimum number of players in each team
	"localinfo k_overtime 2\n"			// overtime type
	"localinfo k_pow 0\n";				// powerups

const char _2on2_um_init[] =
	"floodprot 9 1 1\n"					//
	"localinfo k_fp 1\n"				// TODO not implemented
	"timelimit 10\n"					//
	"teamplay 2\n"						//
	"deathmatch 3\n"					//
	"localinfo k_deathmatch 3\n"		//
	"localinfo k_membercount 1\n"		//
	"localinfo k_overtime 1\n"			// overtime type
	"localinfo k_exttime 2\n"			// extende time for overtime
	"localinfo k_pow 1\n";				//

const char _3on3_um_init[] =
	"floodprot 9 1 1\n"
	"localinfo k_fp 1\n"
	"timelimit 20\n"
	"teamplay 2\n"
	"deathmatch 1\n"
	"localinfo k_deathmatch 1\n"
	"localinfo k_membercount 2\n"
	"localinfo k_pow 1\n"
	"localinfo k_overtime 1\n"
	"localinfo k_exttime 5\n";

const char _4on4_um_init[] =
	"floodprot 9 1 1\n"
	"localinfo k_fp 1\n"
	"timelimit 20\n"
	"teamplay 2\n"
	"deathmatch 1\n"
	"localinfo k_deathmatch 1\n"
	"localinfo k_membercount 3\n"
	"localinfo k_pow 1\n"
	"localinfo k_overtime 1\n"
	"localinfo k_exttime 5\n";

const char _10on10_um_init[] =
	"floodprot 9 1 1\n"
	"localinfo k_fp 1\n"
	"timelimit 30\n"
	"teamplay 2\n"
	"deathmatch 1\n"
	"localinfo k_deathmatch 1\n"
	"localinfo k_membercount 5\n"
	"localinfo k_pow 1\n"
	"localinfo k_overtime 1\n"
	"localinfo k_exttime 5\n";

const char ffa_um_init[] =
	"timelimit 20\n"
	"teamplay 0\n"
	"localinfo k_lockmin 0\n"
	"localinfo k_membercount 1\n"
	"localinfo dq 1\n"
	"localinfo dr 1\n"
	"localinfo k_pow 1\n"
	"localinfo k_dis 0\n";	// hmm, really?


#define UM_1ON1		( 1<<0  )
#define UM_2ON2		( 1<<1  )
#define UM_3ON3		( 1<<2  )
#define UM_4ON4		( 1<<3  )
#define UM_10ON10	( 1<<4  )
#define UM_FFA		( 1<<5  )

typedef struct usermode_s {
	const char 	  *name;
	const char 	  *displayname;
	const char    *initstring;
	int		um_flags;
} usermode;

usermode um_list[] =
{
	{ "1on1", 	"1 on 1",	_1on1_um_init,		UM_1ON1},
	{ "2on2",	"2 on 2",	_2on2_um_init,		UM_2ON2},
	{ "3on3",	"3 on 3",	_3on3_um_init,		UM_3ON3},
	{ "4on4",	"4 on 4",	_4on4_um_init,		UM_4ON4},
	{ "10on10",	"10 on 10",	_10on10_um_init,	UM_10ON10},
	{ "ffa",	"ffa",		ffa_um_init,		UM_FFA}
};

int um_cnt = sizeof (um_list) / sizeof (um_list[0]);

void UserMode(float umode)
{
	char *um = *(char**) &umode; // oh, thanks Tonik %)
	int k_free_mode = atoi( ezinfokey( world, "k_free_mode" ) );
	int k_allowed_free_modes = atoi( ezinfokey( world, "k_allowed_free_modes" ) );
	int i;

	if ( match_in_progress )
		return;

	if( atoi( ezinfokey( world, "k_master" ) ) && self->k_admin != 2 ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	if ( strnull( um ) )
		return;

//for 1on1 / 2on2 / 4on4 and ffa commands manipulation
//0 - noone, 1 - admins, 2 elected admins too
//3 - only real real judges, 4 - elected judges too
//5 - all players

// hmm, I didn't understand how k_free_mode affect this command,
// so implement how i think this must be, it is like some sort of access control

	switch ( k_free_mode ) {
		case 0:	G_sprint(self, 2, "%s can use this command\n", redtext("noone"));
				return;
		case 1:
		case 2:	if ( self->k_admin != 2 ) {
					G_sprint(self, 2, "you must be an %s\n", redtext("admin"));
					return;
				}
				break;
		case 3:
		case 4:	if ( self->k_admin != 2 ) {
					G_sprint(self, 2, "%s is not implemented in this mode\n", redtext("judges"));
					G_sprint(self, 2, "you must be an %s\n", redtext("admin"));
					return;
				}
				break;
		case 5:
				break;
		default:
				G_sprint(self, 2, "server is misconfigured, command %s\n", redtext("skipped"));
				return;
	}

// ok u have access, but is this command really allowed at all?

	for ( i = 0; i < um_cnt; i++ )
		if ( streq(um, um_list[i].name) && (um_list[i].um_flags & k_allowed_free_modes))
			break;

	if ( i >= um_cnt ) {
		G_sprint(self, 2, "server configuration %s this command\n", redtext("lock"));
		return;
	}

	G_bprint(2, "%s %s\n", redtext(um_list[i].displayname), redtext("settings enabled"));

	localcmd( common_um_init );

	if ( self->k_admin == 2 ) // some admin features, may be overwriten by um_list[i].initstring
	{
		// introduce 'k_umfallbunny', which is just control which value
		// must be set to 'k_fallbunny' after XonX
		int k_umfallbunny = bound ( 0, atoi( ezinfokey( world, "k_umfallbunny" ) ), 1);
		localcmd("localinfo k_fallbunny %d\n", k_umfallbunny);
	}

	localcmd( um_list[i].initstring );

	// TODO: IMO possible check existence of each file, so don't spam in logs like "can't find etc..."
	localcmd("exec configs/usermodes/default.cfg\n");
	localcmd("exec configs/usermodes/%s.cfg\n", g_globalvars.mapname);
	localcmd("exec configs/usermodes/%s/default.cfg\n", um);
	localcmd("exec configs/usermodes/%s/%s.cfg\n", um, g_globalvars.mapname);
}

void ModPause (int pause);

#define UNPAUSEGUARD ( "unpauseGuard" )


void VoteUnpauseClean()
{
	gedict_t *p = find(world, FOFCLSN, "player");

	while( p ) {
		p->k_voteUnpause = 0; // just for sanity
		p = find(p, FOFCLSN, "player");
	}

	ent_remove( self );
}

void VoteUnpauseThink()
{
	gedict_t *p;
	float f1 = 0, f2 = floor( (float)CountPlayers() / 2 ) + 1;

	if ( !k_pause ) { // admin unpaused?
		VoteUnpauseClean();
		return;
	}

	p = find(world, FOFCLSN, "player");
	while( p ) {
		if( p->k_voteUnpause )
			f1++;

		p = find(p, FOFCLSN, "player");
	}

	if ( f1 >= f2 ) {
		G_bprint(2, "Server unpaused the game\n");
		ModPause( 0 );
		VoteUnpauseClean();
		return;
	}

	if ( self->cnt <= 0 ) {
		G_bprint(2, "The unpause voting has timed out, aborting\n");
		VoteUnpauseClean();
		return;
	}

	self->s.v.nextthink = g_globalvars.time + 0.5;
	self->cnt -= 0.5;
}

void VoteUnpause ()
{
	gedict_t *unpauseGuard, *p;
	float f1 = 0, f2 = floor( (float)CountPlayers() / 2 ) + 1;

	if ( k_pause != 2 )
		return;

	if( self->k_voteUnpause ) {
		G_sprint(self, 2, "You are already voted\n"); // FIXME: voted voting, spell check plzzz
		return;
	}

// one guard per server
	unpauseGuard = find(world, FOFCLSN, UNPAUSEGUARD );
	if( !unpauseGuard ) {
		unpauseGuard = spawn();
		unpauseGuard->s.v.owner = EDICT_TO_PROG( world );
		unpauseGuard->s.v.classname = UNPAUSEGUARD;
		unpauseGuard->s.v.think = ( func_t ) VoteUnpauseThink;
		unpauseGuard->s.v.nextthink = g_globalvars.time + 0.5;
		unpauseGuard->cnt = 60; // Check the 1 minute timeout for vote

		p = find(world, FOFCLSN, "player");
		while( p ) {
			p->k_voteUnpause = 0; // reset players
			p = find(p, FOFCLSN, "player");
		}
	}

	self->k_voteUnpause = 1;

	p = find(world, FOFCLSN, "player");
	while( p ) {
		if( p->k_voteUnpause )
			f1++;

		p = find(p, FOFCLSN, "player");
	}

	G_bprint(2, "%s %s\n", self->s.v.netname, redtext("votes for unpause!"));
    if ( f1 < f2 )
		G_bprint(2, "%d more vote%s needed\n", (int)(f2 - f1),  ( (int)(f2 - f1) == 1 ? "" : "s") );
}

