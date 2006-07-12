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
 *  $Id: commands.c,v 1.119 2006/07/12 23:27:54 qqshka Exp $
 */

// commands.c

#include "g_local.h"

int max_cmd_len = 0;

void StuffMainMaps();

void SendMessage(char *name);
float CountRPlayers();
float CountTeams();
void PlayerReady ();
void PlayerBreak ();
void ReqAdmin ();
void AdminForceStart ();
void AdminForceBreak ();
void AdminForcePause ();
void AdminSwapAll ();
void TogglePreWar ();
void ToggleMapLock ();
void ToggleMaster ();
void AdminKick ();
void m_kick ();
void YesKick ();
void DontKick ();
void VoteAdmin();
void VoteYes();
void VoteNo();
void VoteCaptain ();
void RandomPickup();
void ShowDMM();
void ChangeDM(float dmm);
void ChangeLock();
void ChangeOvertime();
void ChangeOvertimeUp();
void ChangeTP();
void ToggleFallBunny ();
// { CTF
void FlagStatus();
void norunes();
void nohook();
void mctf();
// also: TossRune()
//       swapall()
// } CTF
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
void ToggleMidair();
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
void Wp_Reset ();
void Wp_Stats(float on);
void Sc_Stats(float on);
void t_jump (float j_type);
void klist ();
void hdptoggle ();
void handicap ();
void noweapon ();
void tracklist ();
void fpslist ();
void krnd ();
void agree_on_map ();

void favx_add ( float fav_num );
void xfav_go ( float fav_num );
void fav_add ();
void fav_del ();
void fav_all_del ();
void fav_next ();
void fav_show ();
void AutoTrack ( float autoTrackType );
void next_best ();
void next_pow ();
void Pos_Show ();
void Pos_Save ();
void Pos_Move ();
void Pos_Set ( float set_type );
void Sh_Speed ();
void lastscores ();

void motd_show ();

void TogglePractice();

void infolock ();
void infospec ();
void moreinfo ();

void s_p_cmd();
void s_lr_cmd( float l );
void s_t_cmd();
void mmode();
void multi();
void cmdinfo();
void cmduinfo();
void cmd_wreg();

void ClientKill();

void sv_time();
void GrenadeMode();
void ToggleReady();

void fp_toggle ();

void dlist();
void dinfo();

void sv_lock ();

// spec
void ShowCamHelp();

void TeamSay(float fsndname);
void TimeDown(float t);
void TimeUp(float t);
void TimeSet(float t);

// CD - commands descriptions

const char CD_NODESC[] = "no desc";

#define CD_COMMANDS   "show commands list"
#define CD_SCORES     "print match time/scores"
#define CD_STATS      "show player stats"
#define CD_EFFI       "show player efficiencies"
#define CD_OPTIONS    "match control commands"
#define CD_READY      "when you feel ready"
#define CD_BREAK      "unready / vote matchend"
#define CD_STATUS     "show server settings"
#define CD_STATUS2    "more server settings"
#define CD_WHO        "player teamlist"
#define CD_WHOSKIN    "player skinlist"
#define CD_WHONOT     "players not ready"
#define CD_LIST       "whonot to everyone"
#define CD_WHOVOTE    "info on received votes"
#define CD_SPAWN      "toggle spawn modes"
#define CD_POWERUPS   "quad, \x98\x98\x98, ring & suit"
#define CD_DISCHARGE  "underwater discharges"
#define CD_DM         "show deathmatch mode"
#define CD_DMM1       "set deathmatch mode 1"
#define CD_DMM2       "set deathmatch mode 2"
#define CD_DMM3       "set deathmatch mode 3"
#define CD_DMM4       "set deathmatch mode 4"
#define CD_DMM5       "set deathmatch mode 5"
#define CD_TP         "change teamplay mode"
#define CD_TIMEDOWN1  "-1 mins match time"
#define CD_TIMEUP1    "+1 mins match time"
#define CD_TIMEDOWN   "-5 mins match time"
#define CD_TIMEUP     "+5 mins match time"
#define CD_FALLBUNNY  "toggle fallbunny"
#define CD_FRAGSDOWN  "-10 fraglimit"
#define CD_FRAGSUP    "+10 fraglimit"
#define CD_DROPQUAD   "drop quad when killed"
#define CD_DROPRING   "drop ring when killed"
#define CD_DROPPACK   "drop pack when killed"
#define CD_SILENCE    "toggle spectator talk"
#define CD_RESET      "set defaults"
#define CD_REPORT     "simple teamplay report"
#define CD_RULES      "show game rules"
#define CD_LOCKMODE   "change locking mode"
#define CD_MAPS       "list custom maps"
#define CD_SPAWN666   "2 secs of \x98\x98\x98 on respawn"
#define CD_ADMIN      "toggle admin-mode"
#define CD_FORCESTART "force match to start"
#define CD_FORCEBREAK "force match to end"
#define CD_FORCEPAUSE "toggle pausemode"
#define CD_PICKUP     "vote for pickup game"
#define CD_PREWAR     "playerfire before game"
#define CD_LOCKMAP    "(un)lock current map"
#define CD_MASTER     "toggle mastermode"
#define CD_SPEED      "toggle sv_maxspeed"
#define CD_FAIRPACKS  "best/last weapon dropped"
#define CD_ABOUT      "mod's info"
#define CD_SHOWNICK   "pointed player's info"
#define CD_TIME5      "set timelimit to 5 mins"
#define CD_TIME10     "set timelimit to 10 mins"
#define CD_TIME15     "set timelimit to 15 mins"
#define CD_TIME20     "set timelimit to 20 mins"
#define CD_TIME25     "set timelimit to 25 mins"
#define CD_TIME30     "set timelimit to 30 mins"
#define CD_BERZERK    "toggle berzerk mode"
#define CD_KSOUND1    (CD_NODESC) // useless command now
#define CD_KSOUND2    (CD_NODESC) // useless command now
#define CD_KSOUND3    (CD_NODESC) // useless command now
#define CD_KSOUND4    (CD_NODESC) // useless command now
#define CD_KSOUND5    (CD_NODESC) // useless command now
#define CD_KSOUND6    (CD_NODESC) // useless command now
#define CD_QIZMO      "qizmo related commands"
#define CD_MESSAGES   "fun message commands"
#define CD_KILLER     "message to killer"
#define CD_VICTIM     "message to victim"
#define CD_NEWCOMER   "message to last player joined"
#define CD_QLAG       "lag settings"
#define CD_QENEMY     "enemy vicinity reporting"
#define CD_QPOINT     "point function"
#define CD_KICK       "toggle kick mode"
#define CD_MKICK      "multi kick"
#define CD_Y          "yes kick"
#define CD_N          "don't kick"
#define CD_OVERTIME   "toggle overtime mode"
#define CD_OVERTIMEUP "change overtime length"
#define CD_ELECT      "toggle admin election"
#define CD_YES        "give vote"
#define CD_NO         "withdraws vote"
#define CD_CAPTAIN    "toggle captain election"
#define CD_FREEZE     "(un)freeze the map"
#define CD_RPICKUP    "vote random team pickup"
#define CD_1ON1       "duel settings"
#define CD_2ON1       "2 on 2 settings"
#define CD_3ON1       "3 on 3 settings"
#define CD_4ON1       "4 on 4 settings"
#define CD_10ON10     "10 on 10 settings"
#define CD_FFA        "FFA settings"
#define CD_CTF        "CTF settings"
#define CD_UNPAUSE    "vote unpause game"
#define CD_PRACTICE   "toggle practice mode"
#define CD_WP_RESET   "clear weapon stats"
#define CD_PLS_WP_STATS "start print weapon stats"
#define CD_MNS_WP_STATS (CD_NODESC) // obvious
#define CD_TKFJUMP      "toggle allow kfjump"
#define CD_TKRJUMP      "toggle allow krjump"
#define CD_KLIST        "mod's list of users"
#define CD_HDPTOGGLE    "toggle allow handicap"
#define CD_HANDICAP     "toggle handicap level"
#define CD_NOWEAPON     "toggle allow any weapon"
#define CD_CAM          "camera help text"
#define CD_TRACKLIST    "trackers list"
#define CD_FPSLIST      "fps list"
#define CD_FAV1_ADD     "add player to slot  1"
#define CD_FAV2_ADD     "add player to slot  2"
#define CD_FAV3_ADD     "........etc.........."
#define CD_FAV4_ADD     (CD_NODESC) // skip
#define CD_FAV5_ADD     (CD_NODESC) // skip
#define CD_FAV6_ADD     (CD_NODESC) // skip 
#define CD_FAV7_ADD     (CD_NODESC) // skip 
#define CD_FAV8_ADD     (CD_NODESC) // skip 
#define CD_FAV9_ADD     (CD_NODESC) // skip 
#define CD_FAV10_ADD    (CD_NODESC) // skip
#define CD_FAV11_ADD    (CD_NODESC) // skip
#define CD_FAV12_ADD    (CD_NODESC) // skip
#define CD_FAV13_ADD    (CD_NODESC) // skip
#define CD_FAV14_ADD    (CD_NODESC) // skip
#define CD_FAV15_ADD    (CD_NODESC) // skip
#define CD_FAV16_ADD    (CD_NODESC) // skip
#define CD_FAV17_ADD    (CD_NODESC) // skip
#define CD_FAV18_ADD    (CD_NODESC) // skip
#define CD_FAV19_ADD    "add player to slot 19"
#define CD_FAV20_ADD    "add player to slot 20"
#define CD_1FAV_GO      "set pov to slot  1"
#define CD_2FAV_GO      "set pov to slot  2"
#define CD_3FAV_GO      ".......etc........"
#define CD_4FAV_GO      (CD_NODESC) // skip
#define CD_5FAV_GO      (CD_NODESC) // skip 
#define CD_6FAV_GO      (CD_NODESC) // skip 
#define CD_7FAV_GO      (CD_NODESC) // skip 
#define CD_8FAV_GO      (CD_NODESC) // skip 
#define CD_9FAV_GO      (CD_NODESC) // skip 
#define CD_10FAV_GO     (CD_NODESC) // skip
#define CD_11FAV_GO     (CD_NODESC) // skip
#define CD_12FAV_GO     (CD_NODESC) // skip
#define CD_13FAV_GO     (CD_NODESC) // skip
#define CD_14FAV_GO     (CD_NODESC) // skip
#define CD_15FAV_GO     (CD_NODESC) // skip
#define CD_16FAV_GO     (CD_NODESC) // skip
#define CD_17FAV_GO     (CD_NODESC) // skip
#define CD_18FAV_GO     (CD_NODESC) // skip
#define CD_19FAV_GO     "set pov to slot 19"
#define CD_20FAV_GO     "set pov to slot 20"
#define CD_FAV_ADD      "add player to fav list"
#define CD_FAV_DEL      "remove player from fav list"
#define CD_FAV_ALL_DEL  "clear fav list"
#define CD_FAV_NEXT     "set pov to next fav"
#define CD_FAV_SHOW     "show fav list"
#define CD_PLS_SCORES   "show match time and score"
#define CD_MNS_SCORES   (CD_NODESC) // obvious
#define CD_AUTOTRACK    "auto tracking"
#define CD_AUTO_POW     "auto tracking powerups"
#define CD_NEXT_BEST    "set pov to next best player"
#define CD_NEXT_POW     "set pov to next powerup"
#define CD_LASTSCORES   "print last games scores"
#define CD_RND          "select random value"
#define CD_AGREE        "agree on last map vote"
#define CD_POS_SHOW     "info about saved position"
#define CD_POS_SAVE     "save current position"
#define CD_POS_MOVE     "move to saved position"
#define CD_POS_ORIGIN   "set position origin"
#define CD_POS_ANGLES   "set position angles"
#define CD_POS_VELOCITY "set position velocity"
#define CD_SH_SPEED     "toggle use show speed"
#define CD_TOSSRUNE     "drop rune (CTF)"
#define CD_FLAGSTATUS   "show flags status (CTF)"
#define CD_NOHOOK       "toggle hook (CTF)"
#define CD_NORUNES      "toggle runes (CTF)"
#define CD_MCTF         "disable hook+runes (CTF)"
#define CD_MOTD         "show motd"
#define CD_INFOLOCK     "toggle specinfo perms"
#define CD_INFOSPEC     "toggle spectator infos"
#define CD_MOREINFO     "receiving more info"
#define CD_S_P			"direct player say"
#define CD_S_L          "continue last s-p u done"
#define CD_S_R          "reply to last s-p u got"
#define CD_S_T          "say to group of players"
#define CD_S_M          "multi say" // anyone have better description?
#define CD_MMODE        "switch message mode"
#define CD_MULTI        "change/print multi set"
#define CD_KINFO        "set self params for mod"
#define CD_KUINFO       "examine someone params"
#define CD_WREG         "register reliable wpns"
#define CD_KILL         "invoke suicide"
#define CD_MIDAIR       "midair settings"
#define CD_TIME         "show server time"
#define CD_GREN_MODE    "grenades mode"
#define CD_TOGGLEREADY  "just toggle ready"
#define CD_FP           "change floodprot level"
#define CD_DLIST        "show demo list"
#define CD_DINFO        "show demo info"
#define CD_LOCK         "temprorary lock server"
#define CD_SWAPALL      "swap teams for ctf"
// { RA
#define CD_RA_BREAK     "toggle RA line status"
#define CD_RA_EFFI      "RA players efficiencies"
#define CD_RA_POS       "RA line position"
// }

void dummy() {}

cmd_t cmds[] = {
	{ "commands",    ShowCmds,			        0    , CF_BOTH | CF_MATCHLESS, CD_COMMANDS },
	{ "scores",      PrintScores,		        0    , CF_BOTH | CF_MATCHLESS, CD_SCORES },
	{ "stats",       PlayerStats,               0    , CF_BOTH | CF_MATCHLESS, CD_STATS },
	{ "effi",        PlayerStats,               0    , CF_BOTH | CF_MATCHLESS, CD_EFFI },
	{ "options",     ShowOpts,                  0    , CF_PLAYER, CD_OPTIONS },
	{ "ready",       PlayerReady,               0    , CF_PLAYER, CD_READY },
	{ "break",       PlayerBreak,               0    , CF_PLAYER | CF_MATCHLESS, CD_BREAK },
	{ "status",      ModStatus,                 0    , CF_BOTH | CF_MATCHLESS, CD_STATUS },
	{ "status2",     ModStatus2,                0    , CF_BOTH | CF_MATCHLESS, CD_STATUS2 },
	{ "who",         PlayerStatus,              0    , CF_BOTH, CD_WHO },
	{ "whoskin",     PlayerStatusS,             0    , CF_BOTH | CF_MATCHLESS, CD_WHOSKIN },
	{ "whonot",      PlayerStatusN,             0    , CF_BOTH, CD_WHONOT },
	{ "list",        ListWhoNot,                0    , CF_PLAYER | CF_SPC_ADMIN, CD_LIST },
	{ "whovote",     ModStatusVote,             0    , CF_BOTH | CF_MATCHLESS, CD_WHOVOTE },
	{ "spawn",       ToggleRespawns,            0    , CF_PLAYER | CF_SPC_ADMIN, CD_SPAWN },
	{ "powerups",    TogglePowerups,            0    , CF_PLAYER | CF_SPC_ADMIN, CD_POWERUPS },
	{ "discharge",   ToggleDischarge,           0    , CF_PLAYER, CD_DISCHARGE },
	{ "dm",          ShowDMM,                   0    , CF_PLAYER, CD_DM },
	{ "dmm1",        ChangeDM,                  1    , CF_PLAYER | CF_SPC_ADMIN, CD_DMM1 },
	{ "dmm2",        ChangeDM,                  2    , CF_PLAYER | CF_SPC_ADMIN, CD_DMM2 },
	{ "dmm3",        ChangeDM,                  3    , CF_PLAYER | CF_SPC_ADMIN, CD_DMM3 },
	{ "dmm4",        ChangeDM,                  4    , CF_PLAYER | CF_SPC_ADMIN, CD_DMM4 },
	{ "dmm5",        ChangeDM,                  5    , CF_PLAYER | CF_SPC_ADMIN, CD_DMM5 },
	{ "tp",          ChangeTP,                  0    , CF_PLAYER | CF_SPC_ADMIN, CD_TP },
	{ "timedown1",   TimeDown,				  1.0f   , CF_PLAYER | CF_SPC_ADMIN, CD_TIMEDOWN1 },
	{ "timeup1",     TimeUp,				  1.0f   , CF_PLAYER | CF_SPC_ADMIN, CD_TIMEUP1 },
	{ "timedown",    TimeDown,				  5.0f   , CF_PLAYER | CF_SPC_ADMIN, CD_TIMEDOWN },
	{ "timeup",      TimeUp,				  5.0f   , CF_PLAYER | CF_SPC_ADMIN, CD_TIMEUP },
	{ "fallbunny",   ToggleFallBunny,           0    , CF_PLAYER | CF_SPC_ADMIN, CD_FALLBUNNY },
	{ "fragsdown",   FragsDown,                 0    , CF_PLAYER, CD_FRAGSDOWN },
	{ "fragsup",     FragsUp,                   0    , CF_PLAYER, CD_FRAGSUP },
	{ "dropquad",    ToggleDropQuad,            0    , CF_PLAYER, CD_DROPQUAD },
	{ "dropring",    ToggleDropRing,            0    , CF_PLAYER, CD_DROPRING },
	{ "droppack",    ToggleDropPack,            0    , CF_PLAYER, CD_DROPPACK },
	                                             
	{ "silence",     ToggleSpecTalk,            0    , CF_PLAYER | CF_SPC_ADMIN, CD_SILENCE },
	{ "reset",       ResetOptions,              0    , CF_PLAYER | CF_SPC_ADMIN, CD_RESET },
	{ "report",      ReportMe,                  0    , CF_PLAYER, CD_REPORT },
	{ "rules",       ShowRules,                 0    , CF_PLAYER | CF_MATCHLESS, CD_RULES },
	{ "lockmode",    ChangeLock,                0    , CF_PLAYER | CF_SPC_ADMIN, CD_LOCKMODE },
	{ "maps",        ShowMaps,                  0    , CF_PLAYER | CF_SPC_ADMIN | CF_MATCHLESS, CD_MAPS},
	{ "spawn666",    ToggleRespawn666,          0    , CF_PLAYER, CD_SPAWN666},
	{ "admin",       ReqAdmin,                  0    , CF_BOTH | CF_PARAMS, CD_ADMIN },
	{ "forcestart",  AdminForceStart,           0    , CF_BOTH_ADMIN, CD_FORCESTART },
	{ "forcebreak",  AdminForceBreak,           0    , CF_BOTH_ADMIN, CD_FORCEBREAK },
#ifndef NO_K_PAUSE
	{ "forcepause",  AdminForcePause,           0    , CF_BOTH_ADMIN, CD_FORCEPAUSE },
#endif
	{ "pickup",      VotePickup,                0    , CF_PLAYER, CD_PICKUP }, 
	{ "prewar",      TogglePreWar,              0    , CF_BOTH_ADMIN, CD_PREWAR },
	{ "lockmap",     ToggleMapLock,             0    , CF_BOTH_ADMIN, CD_LOCKMAP },
	{ "master",      ToggleMaster,              0    , CF_BOTH_ADMIN, CD_MASTER },
	{ "speed",       ToggleSpeed,               0    , CF_PLAYER, CD_SPEED },
	{ "fairpacks",   ToggleFairPacks,           0    , CF_PLAYER, CD_FAIRPACKS },
	{ "about",       ShowVersion,               0    , CF_BOTH | CF_MATCHLESS, CD_ABOUT },
	{ "shownick",    ShowNick,                  0    , CF_PLAYER, CD_SHOWNICK },
	{ "time5",       TimeSet,		  	 	  5.0f   , CF_PLAYER, CD_TIME5 },
	{ "time10",      TimeSet,		  	     10.0f   , CF_PLAYER, CD_TIME10 },
	{ "time15",      TimeSet,		  	     15.0f   , CF_PLAYER, CD_TIME15 },
	{ "time20",      TimeSet,                20.0f   , CF_PLAYER, CD_TIME20 },
	{ "time25",      TimeSet,                25.0f   , CF_PLAYER, CD_TIME25 },
	{ "time30",      TimeSet,                30.0f   , CF_PLAYER, CD_TIME30 },
	{ "berzerk",     ToggleBerzerk,             0    , CF_PLAYER, CD_BERZERK },
	                                             
	{ "ksound1",     TeamSay,   			    1    , CF_PLAYER, CD_KSOUND1 },
	{ "ksound2",     TeamSay,   			    2    , CF_PLAYER, CD_KSOUND2 },
	{ "ksound3",     TeamSay,   			    3    , CF_PLAYER, CD_KSOUND3 },
	{ "ksound4",     TeamSay,   			    4    , CF_PLAYER, CD_KSOUND4 },
	{ "ksound5",     TeamSay,   			    5    , CF_PLAYER, CD_KSOUND5 },
	{ "ksound6",     TeamSay,   			    6    , CF_PLAYER, CD_KSOUND6 },
	                                           
	{ "qizmo",       ShowQizmo,                 0    , CF_PLAYER, CD_QIZMO },
	                                             
	{ "messages",    ShowMessages,              0    , CF_PLAYER | CF_MATCHLESS, CD_MESSAGES },
	{ "killer",      SendKillerMsg,             0    , CF_PLAYER | CF_MATCHLESS, CD_KILLER },
	{ "victim",      SendVictimMsg,             0    , CF_PLAYER | CF_MATCHLESS, CD_VICTIM },
	{ "newcomer",    SendNewcomerMsg,           0    , CF_PLAYER | CF_MATCHLESS, CD_NEWCOMER },
	                                             
	{ "qlag",        ToggleQLag,                0    , CF_PLAYER | CF_SPC_ADMIN, CD_QLAG },
	{ "qenemy",      ToggleQEnemy,              0    , CF_PLAYER | CF_SPC_ADMIN, CD_QENEMY },
	{ "qpoint",      ToggleQPoint,              0    , CF_PLAYER | CF_SPC_ADMIN, CD_QPOINT },
	                                          
	{ "kick",        AdminKick,                 0    , CF_BOTH_ADMIN/* FIXME: interference with ezq server kick command | CF_PARAMS */, CD_KICK },
	{ "mkick",       m_kick,                    0    , CF_BOTH_ADMIN | CF_PARAMS, CD_MKICK },
	{ "y",           YesKick,                   0    , CF_BOTH_ADMIN, CD_Y },
	{ "n",           DontKick,                  0    , CF_BOTH_ADMIN, CD_N },
	{ "overtime",    ChangeOvertime,            0    , CF_PLAYER | CF_SPC_ADMIN, CD_OVERTIME },
	{ "overtimeup",  ChangeOvertimeUp,          0    , CF_PLAYER | CF_SPC_ADMIN, CD_OVERTIMEUP },
	{ "elect",       VoteAdmin,                 0    , CF_BOTH, CD_ELECT },
	{ "yes",         VoteYes,                   0    , CF_PLAYER, CD_YES },
	{ "no",          VoteNo,                    0    , CF_PLAYER, CD_NO },
	{ "captain",     VoteCaptain,               0    , CF_PLAYER, CD_CAPTAIN },
	{ "freeze",      ToggleFreeze,              0    , CF_PLAYER, CD_FREEZE },
	{ "rpickup",     RandomPickup,              0    , CF_PLAYER | CF_SPC_ADMIN, CD_RPICKUP },

	{ "1on1",        UserMode,                  1	 , CF_PLAYER | CF_SPC_ADMIN, CD_1ON1 },
	{ "2on2",        UserMode,                  2	 , CF_PLAYER | CF_SPC_ADMIN, CD_2ON1 },
	{ "3on3",        UserMode,                  3	 , CF_PLAYER | CF_SPC_ADMIN, CD_3ON1 },
	{ "4on4",        UserMode,                  4	 , CF_PLAYER | CF_SPC_ADMIN, CD_4ON1 },
	{ "10on10",      UserMode,                  5	 , CF_PLAYER | CF_SPC_ADMIN, CD_10ON10 },
	{ "ffa",         UserMode,                  6	 , CF_PLAYER | CF_SPC_ADMIN, CD_FFA },
	{ "ctf",         UserMode,                  7    , CF_PLAYER | CF_SPC_ADMIN, CD_CTF },

#ifndef NO_K_PAUSE
	{ "unpause",     VoteUnpause,               0    , CF_PLAYER, CD_UNPAUSE },
#endif
	{ "practice",    TogglePractice,            0    , CF_PLAYER | CF_SPC_ADMIN, CD_PRACTICE },
	{ "wp_reset",    Wp_Reset,                  0    , CF_PLAYER, CD_WP_RESET },
	{ "+wp_stats",   Wp_Stats,                  2    , CF_BOTH | CF_MATCHLESS, CD_PLS_WP_STATS },
	{ "-wp_stats",   Wp_Stats,                  1    , CF_BOTH | CF_MATCHLESS, CD_MNS_WP_STATS },
	{ "tkfjump",     t_jump,                    1    , CF_BOTH_ADMIN, CD_TKFJUMP },
	{ "tkrjump",     t_jump,                    2    , CF_BOTH_ADMIN, CD_TKRJUMP },
	{ "klist",       klist,                     0    , CF_BOTH | CF_MATCHLESS, CD_KLIST },
	{ "hdptoggle",   hdptoggle,                 0    , CF_BOTH_ADMIN, CD_HDPTOGGLE },
	{ "handicap",    handicap,                  0    , CF_PLAYER | CF_PARAMS | CF_MATCHLESS, CD_HANDICAP },
	{ "noweapon",    noweapon,                  0    , CF_PLAYER | CF_PARAMS | CF_SPC_ADMIN, CD_NOWEAPON },

	{ "cam",         ShowCamHelp,               0    , CF_SPECTATOR | CF_MATCHLESS, CD_CAM },

	{ "tracklist",   tracklist,                 0    , CF_BOTH | CF_MATCHLESS, CD_TRACKLIST },
	{ "fpslist",     fpslist,                   0    , CF_BOTH | CF_MATCHLESS, CD_FPSLIST },

	{ "fav1_add",    favx_add,                  1    , CF_SPECTATOR, CD_FAV1_ADD },
	{ "fav2_add",    favx_add,                  2    , CF_SPECTATOR, CD_FAV2_ADD },
	{ "fav3_add",    favx_add,                  3    , CF_SPECTATOR, CD_FAV3_ADD },
	{ "fav4_add",    favx_add,                  4    , CF_SPECTATOR, CD_FAV4_ADD },
	{ "fav5_add",    favx_add,                  5    , CF_SPECTATOR, CD_FAV5_ADD },
	{ "fav6_add",    favx_add,                  6    , CF_SPECTATOR, CD_FAV6_ADD },
	{ "fav7_add",    favx_add,                  7    , CF_SPECTATOR, CD_FAV7_ADD },
	{ "fav8_add",    favx_add,                  8    , CF_SPECTATOR, CD_FAV8_ADD },
	{ "fav9_add",    favx_add,                  9    , CF_SPECTATOR, CD_FAV9_ADD },
	{ "fav10_add",   favx_add,                 10    , CF_SPECTATOR, CD_FAV10_ADD },
	{ "fav11_add",   favx_add,                 11    , CF_SPECTATOR, CD_FAV11_ADD },
	{ "fav12_add",   favx_add,                 12    , CF_SPECTATOR, CD_FAV12_ADD },
	{ "fav13_add",   favx_add,                 13    , CF_SPECTATOR, CD_FAV13_ADD },
	{ "fav14_add",   favx_add,                 14    , CF_SPECTATOR, CD_FAV14_ADD },
	{ "fav15_add",   favx_add,                 15    , CF_SPECTATOR, CD_FAV15_ADD },
	{ "fav16_add",   favx_add,                 16    , CF_SPECTATOR, CD_FAV16_ADD },
	{ "fav17_add",   favx_add,                 17    , CF_SPECTATOR, CD_FAV17_ADD },
	{ "fav18_add",   favx_add,                 18    , CF_SPECTATOR, CD_FAV18_ADD },
	{ "fav19_add",   favx_add,                 19    , CF_SPECTATOR, CD_FAV19_ADD },
	{ "fav20_add",   favx_add,                 20    , CF_SPECTATOR, CD_FAV20_ADD },
	{ "1fav_go",     xfav_go,                   1    , CF_SPECTATOR, CD_1FAV_GO },
	{ "2fav_go",     xfav_go,                   2    , CF_SPECTATOR, CD_2FAV_GO },
	{ "3fav_go",     xfav_go,                   3    , CF_SPECTATOR, CD_3FAV_GO },
	{ "4fav_go",     xfav_go,                   4    , CF_SPECTATOR, CD_4FAV_GO },
	{ "5fav_go",     xfav_go,                   5    , CF_SPECTATOR, CD_5FAV_GO },
	{ "6fav_go",     xfav_go,                   6    , CF_SPECTATOR, CD_6FAV_GO },
	{ "7fav_go",     xfav_go,                   7    , CF_SPECTATOR, CD_7FAV_GO },
	{ "8fav_go",     xfav_go,                   8    , CF_SPECTATOR, CD_8FAV_GO },
	{ "9fav_go",     xfav_go,                   9    , CF_SPECTATOR, CD_9FAV_GO },
	{ "10fav_go",    xfav_go,                  10    , CF_SPECTATOR, CD_10FAV_GO },
	{ "11fav_go",    xfav_go,                  11    , CF_SPECTATOR, CD_11FAV_GO },
	{ "12fav_go",    xfav_go,                  12    , CF_SPECTATOR, CD_12FAV_GO },
	{ "13fav_go",    xfav_go,                  13    , CF_SPECTATOR, CD_13FAV_GO },
	{ "14fav_go",    xfav_go,                  14    , CF_SPECTATOR, CD_14FAV_GO },
	{ "15fav_go",    xfav_go,                  15    , CF_SPECTATOR, CD_15FAV_GO },
	{ "16fav_go",    xfav_go,                  16    , CF_SPECTATOR, CD_16FAV_GO },
	{ "17fav_go",    xfav_go,                  17    , CF_SPECTATOR, CD_17FAV_GO },
	{ "18fav_go",    xfav_go,                  18    , CF_SPECTATOR, CD_18FAV_GO },
	{ "19fav_go",    xfav_go,                  19    , CF_SPECTATOR, CD_19FAV_GO },
	{ "20fav_go",    xfav_go,                  20    , CF_SPECTATOR, CD_20FAV_GO },
	{ "fav_add",     fav_add,                   0    , CF_SPECTATOR | CF_MATCHLESS, CD_FAV_ADD },
	{ "fav_del",     fav_del,                   0    , CF_SPECTATOR | CF_MATCHLESS, CD_FAV_DEL },
	{ "fav_all_del", fav_all_del,               0    , CF_SPECTATOR | CF_MATCHLESS, CD_FAV_ALL_DEL },
	{ "fav_next",    fav_next,                  0    , CF_SPECTATOR | CF_MATCHLESS, CD_FAV_NEXT },
	{ "fav_show",    fav_show,                  0    , CF_SPECTATOR | CF_MATCHLESS, CD_FAV_SHOW },
	{ "+scores",     Sc_Stats,                  2    , CF_BOTH | CF_MATCHLESS, CD_PLS_SCORES },
	{ "-scores",     Sc_Stats,                  1    , CF_BOTH | CF_MATCHLESS, CD_MNS_SCORES },
	{ "autotrack",   AutoTrack,            atBest    , CF_SPECTATOR | CF_MATCHLESS, CD_AUTOTRACK },
	{ "auto_pow",    AutoTrack,             atPow    , CF_SPECTATOR | CF_MATCHLESS, CD_AUTO_POW },
	{ "next_best",   next_best,                 0    , CF_SPECTATOR | CF_MATCHLESS, CD_NEXT_BEST },
	{ "next_pow",    next_pow,                  0    , CF_SPECTATOR | CF_MATCHLESS, CD_NEXT_POW },
	{ "lastscores",  lastscores,                0    , CF_BOTH, CD_LASTSCORES },
	{ "rnd",         krnd,                		0    , CF_BOTH | CF_PARAMS, CD_RND },
	{ "agree",       agree_on_map,        		0    , CF_PLAYER | CF_MATCHLESS, CD_AGREE },
	{ "pos_show",    Pos_Show,                  0    , CF_BOTH | CF_PARAMS, CD_POS_SHOW },
	{ "pos_save",    Pos_Save,                  0    , CF_BOTH | CF_PARAMS, CD_POS_SAVE },
	{ "pos_move",    Pos_Move,                  0    , CF_BOTH | CF_PARAMS, CD_POS_MOVE },
// VVD: For trick chiters! :-)
// Need to think out how to limit using Pos_Set for tricking.
// May be to ban pos_velocity?
	{ "pos_origin",  Pos_Set,                   1    , CF_BOTH | CF_PARAMS, CD_POS_ORIGIN },
	{ "pos_angles",  Pos_Set,                   2    , CF_BOTH | CF_PARAMS, CD_POS_ANGLES },
//	{ "pos_velocity",Pos_Set,                   3    , CF_BOTH | CF_PARAMS, CD_POS_VELOCITY },
	{ "sh_speed",    Sh_Speed,                  0    , CF_BOTH, CD_SH_SPEED },
// { CTF commands
	{ "tossrune",    TossRune,                  0    , CF_PLAYER, CD_TOSSRUNE },
	{ "nohook",      nohook,                    0    , CF_PLAYER | CF_SPC_ADMIN, CD_NOHOOK },
	{ "norunes",     norunes,                   0    , CF_PLAYER | CF_SPC_ADMIN, CD_NORUNES },
	{ "mctf",        mctf,                      0    , CF_PLAYER | CF_SPC_ADMIN, CD_MCTF },
	{ "flagstatus",  FlagStatus,                0    , CF_BOTH, CD_FLAGSTATUS },
	{ "swapall",     AdminSwapAll,              0    , CF_BOTH_ADMIN, CD_SWAPALL },
// }
	{ "motd",        motd_show,                 0    , CF_BOTH | CF_MATCHLESS, CD_MOTD },
	{ "infolock",    infolock,                  0    , CF_BOTH_ADMIN, CD_INFOLOCK },
	{ "infospec",    infospec,                  0    , CF_PLAYER | CF_SPC_ADMIN, CD_INFOSPEC },
	{ "moreinfo",    moreinfo,                  0    , CF_SPECTATOR | CF_MATCHLESS, CD_MOREINFO },
	{ "s-p",         dummy,                     0    , CF_BOTH | CF_MATCHLESS | CF_PARAMS, CD_S_P },
	{ "s-l",         dummy,                     0    , CF_BOTH | CF_MATCHLESS | CF_PARAMS, CD_S_L },
	{ "s-r",         dummy,                     0    , CF_BOTH | CF_MATCHLESS | CF_PARAMS, CD_S_R },
	{ "s-t",         dummy,                     0    , CF_BOTH | CF_MATCHLESS | CF_PARAMS, CD_S_T },
	{ "s-m",         dummy,                     0    , CF_BOTH | CF_MATCHLESS | CF_PARAMS, CD_S_M },
	{ "mmode",       mmode,                     0    , CF_BOTH | CF_MATCHLESS | CF_PARAMS, CD_MMODE },
	{ "multi",       multi,                     0    , CF_BOTH | CF_MATCHLESS | CF_PARAMS, CD_MULTI },
	{ "kinfo",       cmdinfo,                   0    , CF_BOTH | CF_MATCHLESS | CF_PARAMS, CD_KINFO },
	{ "kuinfo",      cmduinfo,                  0    , CF_BOTH | CF_MATCHLESS | CF_PARAMS, CD_KUINFO },
	{ "wreg",        cmd_wreg,                  0    , CF_BOTH | CF_MATCHLESS | CF_PARAMS, CD_WREG },
	{ "kill",        ClientKill,                0    , CF_PLAYER | CF_MATCHLESS, CD_KILL },
	{ "mid_air",     ToggleMidair,              0    , CF_PLAYER | CF_SPC_ADMIN, CD_MIDAIR },
	{ "time",        sv_time,                   0    , CF_BOTH | CF_MATCHLESS, CD_TIME },
	{ "gren_mode",   GrenadeMode,               0    , CF_PLAYER | CF_SPC_ADMIN, CD_GREN_MODE },
	{ "toggleready", ToggleReady,               0    , CF_PLAYER, CD_TOGGLEREADY },
	{ "fp",          fp_toggle,                 0    , CF_BOTH_ADMIN, CD_FP },
	{ "dlist",       dlist,                     0    , CF_BOTH | CF_MATCHLESS | CF_PARAMS, CD_DLIST },
	{ "dinfo",       dinfo,                     0    , CF_BOTH | CF_MATCHLESS | CF_PARAMS, CD_DINFO },
	{ "lock",        sv_lock,                   0    , CF_BOTH_ADMIN, CD_LOCK },
// { RA
	{ "ra_break",    ra_break,                  0    , CF_PLAYER, CD_RA_BREAK },
	{ "ra_effi",     ra_PrintStats,             0    , CF_BOTH, CD_RA_EFFI },
	{ "ra_pos",      ra_PrintPos,               0    , CF_PLAYER, CD_RA_POS }
// }
};

int cmds_cnt = sizeof( cmds ) / sizeof( cmds[0] );

int DoCommand(int icmd)
{
	int spc = self->k_spectator;

	if ( !( icmd >= 0 && icmd < cmds_cnt ) )
		return DO_OUT_OF_RANGE_CMDS;

	if ( k_matchLess && !(cmds[icmd].cf_flags & CF_MATCHLESS) )
		return DO_CMD_DISALLOWED_MATCHLESS; // cmd does't allowed in matchLess mode

	if ( spc ) { // spec
		if ( !(cmds[icmd].cf_flags & CF_SPECTATOR) )
			return DO_WRONG_CLASS; // cmd not for spectator
		if ( (cmds[icmd].cf_flags & CF_SPC_ADMIN) && !is_adm(self) ) {
			G_sprint(self, 2, "You are not an admin\n");
			return DO_ACCESS_DENIED; // admin rights required
		}
	}
	else { // player
		if ( !(cmds[icmd].cf_flags & CF_PLAYER) )
			return DO_WRONG_CLASS; // cmd not for player
		if ( (cmds[icmd].cf_flags & CF_PLR_ADMIN) && !is_adm(self) ) {
			G_sprint(self, 2, "You are not an admin\n");
			return DO_ACCESS_DENIED; // admin rights required
		}
	}

	if ( strnull( cmds[icmd].name ) || !( cmds[icmd].f ) )
		return DO_FUNCTION_IS_WRONG;

	if ( isCmdFlood( self ) )
		return DO_FLOOD_PROTECT;

	if (cmds[icmd].arg)
		( ( void ( * )(float) ) ( cmds[icmd].f ) ) ( cmds[icmd].arg );
	else
		( cmds[icmd].f )  ();

	return icmd;
}

// VVD: Need for executing commands by 'cmd <CMD_NAME> <ARG1> ... <ARGn>',
// because '<CMD_NAME> <ARG1> ... <ARGn>' work only with last ezQuake qw client.
int DoCommand_Name(char *cmd_name)
{
	int i;

	if ( strnull( cmd_name ) )
		return DO_OUT_OF_RANGE_CMDS;

	for ( i = 0; i < cmds_cnt; ++i ) {
		if ( streq(cmds[i].name, cmd_name) )
			return DoCommand( i );
	}

	return DO_OUT_OF_RANGE_CMDS;
}

qboolean isCmdFlood(gedict_t *p)
{
	int idx;
	float cmd_time;

	if ( k_cmd_fp_disabled || p->connect_time + 5 > g_globalvars.time )
		return false; // cmd flood protect is disabled or skip near connect time due to tons of "cmd info" commands is done

	idx = bound(0, p->fp_c.last_cmd, MAX_FP_CMDS-1);
	cmd_time = p->fp_c.cmd_time[idx];

	if ( g_globalvars.time < p->fp_c.locked )
	{
		G_sprint(p, 2, "command floodprot (%d sec)\n",
										(int)(p->fp_c.locked - g_globalvars.time + 1));
		return true; // flooder
	}

	if ( cmd_time && ( g_globalvars.time - cmd_time < k_cmd_fp_per ) )
	{
		G_sprint(p, 2, "You are a command flooder man!\n");

		p->fp_c.locked = g_globalvars.time + k_cmd_fp_for;

		if ( !k_cmd_fp_dontkick ) {
			if ( k_cmd_fp_kick - p->fp_c.warnings > 1 ) {
				G_sprint(p, 2, "%d warnings to kick\n", k_cmd_fp_kick - p->fp_c.warnings);
			}
			else if ( k_cmd_fp_kick - p->fp_c.warnings == 1 ) {
				G_sprint(p, 2, "next time you will be kicked\n");
			}
			else if ( k_cmd_fp_kick - p->fp_c.warnings < 1 ) {
				if ( p->k_player || ( p->k_spectator && !match_in_progress ) )
					G_bprint(2,"%s is a command flooooder!!!\n"
						   	   "and will be kicked\n", getname(p));

				G_sprint(p, 2, "Go away!\n");

				stuffcmd(p, "disconnect\n"); // FIXME: stupid way
			}
		}

		p->fp_c.warnings += 1;

		return true; // flooder
	}

	p->fp_c.cmd_time[idx] = g_globalvars.time;

	if ( ++idx >= k_cmd_fp_count )
		idx = 0;

	p->fp_c.last_cmd = idx;

	return false;
}


// check if players client support params in aliases
qboolean isSupport_Params(gedict_t *p)
{
	char *clinfo = ezinfokey( p, "*client" );

	if ( !strnull( clinfo ) && strstr(clinfo, "ezQuake") ) // seems only ezQuake support 
		return true;

	return false;
}

void StuffAliases()
{
// stuffing for numbers, hope no flooding out
	int i;

	for ( i = 1; i <= 16; i++ )
		stuffcmd(PROG_TO_EDICT( self->s.v.owner ), "alias %d impulse %d\n", i, i);

	if ( PROG_TO_EDICT( self->s.v.owner )->k_spectator ) {
		stuffcmd(PROG_TO_EDICT( self->s.v.owner ), "alias next_fav fav_next\n");
	}
	else {
		stuffcmd(PROG_TO_EDICT( self->s.v.owner ), "alias notready break\n");
		stuffcmd(PROG_TO_EDICT( self->s.v.owner ), "alias kfjump \"impulse 156;+jump;wait;-jump\"\n");
		stuffcmd(PROG_TO_EDICT( self->s.v.owner ), "alias krjump \"impulse 164;+jump;wait;-jump\"\n");
	}
}

float StuffDeltaTime(int iDelta)
{
	iDelta = bound ( 1, ( iDelta ? iDelta : 4 ), 5); // i.e. default is 4
	return 0.01f * (float)iDelta;
}

// just check is this cmd valid for class of this player
// admin rights skipped here
qboolean isValidCmdForClass( int icmd, qboolean isSpec )
{
	if ( icmd < 0 || icmd >= cmds_cnt )
		return false;

	if ( k_matchLess && !(cmds[icmd].cf_flags & CF_MATCHLESS) )
		return false; // cmd does't allowed in matchLess mode

	// split class
	if ( isSpec ) { // spec
		if ( !(cmds[icmd].cf_flags & CF_SPECTATOR) )
			return false; // cmd not for spec
	}
	else { // player
		if ( !(cmds[icmd].cf_flags & CF_PLAYER) )
			return false; // cmd not for player
	}

	return true;
}

// check if this cmd require admin rights
qboolean isCmdRequireAdmin( int icmd, qboolean isSpec )
{
	if ( icmd < 0 || icmd >= cmds_cnt )
		return false;

	// split class
	if ( isSpec ) { // spec
		if ( cmds[icmd].cf_flags & CF_SPC_ADMIN )
			return true; // cmd require admin rights
	}
	else { // player
		if ( cmds[icmd].cf_flags & CF_PLR_ADMIN )
			return true; // cmd require admin rights
	}

	return false;
}

void StuffModCommands()
{
	int i, limit;
	char *name, *params;
	qboolean spc = PROG_TO_EDICT( self->s.v.owner )->k_spectator;
	qboolean support_params = isSupport_Params( PROG_TO_EDICT( self->s.v.owner ) );
	float dt = StuffDeltaTime( iKey( PROG_TO_EDICT( self->s.v.owner ), "ss" ) );

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

		if ( !isValidCmdForClass( i, spc )  // cmd does't valid for this class of player or matchless mode does't have this command
			 || cmds[i].f == dummy // cmd have't function, ie u must not stuff alias for this cmd
		    ) {
			limit++;
			continue;
		}

		params = ( (cmds[i].cf_flags & CF_PARAMS) && support_params ) ? " %0" : "";

		stuffcmd(PROG_TO_EDICT( self->s.v.owner ), "alias %s cmd %03d%s\n", name, (int)i, params);
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

		if ( (int)strlen( cmds[i].name ) > max_cmd_len )
			max_cmd_len = strlen( cmds[i].name );

		if ( cmds[i].cf_flags & CF_PLR_ADMIN )
			cmds[i].cf_flags |= CF_PLAYER;    // this let simplify cmds[] table

		if ( cmds[i].cf_flags & CF_SPC_ADMIN )
			cmds[i].cf_flags |= CF_SPECTATOR; // this let simplify cmds[] table
	}
}   

qboolean check_master()
{
	if( cvar( "k_master" ) && !is_adm(self) ) {
		G_sprint(self, 2, "console: command is locked\n");
		return true;
	}

	return false;
}

void Do_ShowCmds( qboolean adm_req )
{
	qboolean first = true;
	int i, l;
	char *name, dots[64];

	for( i = 0; i >= 0 && i < cmds_cnt; i++ ) {

		name = cmds[i].name;

		if ( strnull(cmds[i].description) || cmds[i].description == CD_NODESC )
			continue; // command does't have description

		if ( !isValidCmdForClass( i, self->k_spectator ) )
			continue; // cmd does't valid for this class of player or matchless mode does't have this command

		if ( adm_req != isCmdRequireAdmin( i, self->k_spectator ) )
			continue; 

		l = max_cmd_len - strlen(name);
		l = bound( 0, l, sizeof(dots)-1 );
		memset( (void*)dots, (int)'.', l);
		dots[l] = 0;

		if ( first ) {
			first = false;

			G_sprint(self, 2, "\n%s commands for %s:\n\n", 
				( adm_req ? redtext("admin") : redtext("common") ),
				( self->k_spectator ? redtext("spectator") : redtext("player") ));
		}

		G_sprint(self, 2, "%s%s %s\n", redtext(name), dots, cmds[i].description);
	}
}

void ShowCmds()
{
	Do_ShowCmds( false ); // show common commands
	Do_ShowCmds( true );  // show admin commands

/*
	if ( self->k_spectator )
		SShowCmds();
	else 
		PShowCmds();
*/
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
			"βεςϊεςλ.... toggle berzerk mode\n"
			"%s..... toggle midair mode\n", redtext("midair"));
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
	char buf[2048] = {0};

	strlcat(buf, va("Running %s v.%s build:%s by %s\n\n", redtext(MOD_NAME), dig3s(MOD_VERSION),
						dig3s("%05d", build_number()), redtext("KTX dev. team")), sizeof(buf));
	strlcat(buf, va("Based on %s\n", redtext("Kombat teams 2.21")), sizeof(buf));
	strlcat(buf, "by kemiKal, Cenobite, Sturm and Fang\n\n", sizeof(buf));
	strlcat(buf, va("Source at:\n%s", MOD_URL), sizeof(buf));

	G_sprint(self, 2, "%s\n", buf);
}

void ChangeOvertime()
{
	float f1, f2;

	if ( match_in_progress )
		return;

	if( check_master() )
		return;

    f1 = bound(0, cvar( "k_overtime" ), 2);
    f2 = bound(0, cvar( "k_exttime" ), 999);

    if( !f1 )
    {
		cvar_fset("k_overtime", 1);

		if( !f2 ) 
			cvar_fset("k_exttime", (f2 = 1));

		G_bprint(2, "%s: time based\n", redtext("Overtime"));
		G_bprint(2, "%s: %d minute%s\n", redtext("Overtime length"), (int)f2, count_s(f2));
    }
    else if( f1 == 1 )
	{
		cvar_fset("k_overtime", 2);
		G_bprint(2, "%s: sudden death\n", redtext("Overtime"));
	}
    else if( f1 == 2 )
    {
		cvar_fset("k_overtime", 0);
		G_bprint(2, "%s: off\n", redtext("Overtime"));
    }
}

void ChangeOvertimeUp ()
{
	int k_exttime = cvar( "k_exttime" );

	if( match_in_progress )
		return;

	if( check_master() )
		return;

	k_exttime++;

	if ( k_exttime >= 11 || k_exttime <= 0 )
		k_exttime = 1;

	cvar_fset( "k_exttime", k_exttime );

	G_bprint(2, "%s %d %s%s\n", 
		redtext("Overtime length set to"), k_exttime, redtext("minute"), redtext(count_s( k_exttime )));
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

	if ( !strnull( name ) ) {

		p = find( world, FOFCLSN, "player" );
		while( ( p && strneq( p->s.v.netname, name ) ) || p == self )
			p = find( p, FOFCLSN, "player" );
    
		if( p ) {
			G_bprint(3, "%s: %s", self->s.v.netname, ezinfokey(self, "premsg"));
			G_bprint(3, name);
			G_bprint(3,"%s\n", ezinfokey(self, "postmsg"));

			return;
		}
	}

	G_sprint(self, 2, "No name to display\n");
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
	int i;

	if ( strnull(tog) || strnull(key) )
		G_Error("PrintToggle1 null");

	G_sprint(self, 2, tog);

	i = streq(key, "k_pow") ? Get_Powerups() : bound(0, cvar( key ), 1);

	if( !i )
		G_sprint(self, 2, "Off ");
	else if ( i == 1 )
		G_sprint(self, 2, "On  ");
	else
		G_sprint(self, 2, "Jam ");
}

void PrintToggle2( char *tog, char *key )
{
	if ( strnull(tog) || strnull(key) )
		G_Error("PrintToggle2 null");

	G_sprint(self, 2, tog);

	if( cvar( key ) )
		G_sprint(self, 2, "On\n");
	else
		G_sprint(self, 2, "Off\n");
}

//Ceno
void PrintToggle3(char *tog, char *key)
{
	int i;

	if ( strnull(tog) || strnull(key) )
		G_Error("PrintToggle3 null");

	G_sprint(self, 2, tog);

	i = cvar( key );

	if( !i )
		G_sprint(self, 2, "Off ");
	else if( i == 1 )
		G_sprint(self, 2, "On  ");
	else if( i == 2 )
		G_sprint(self, 2, "Lst ");
	else
		G_sprint(self, 2, "Unk ");
}

void ModStatus ()
{
	int votes;
	gedict_t *p;

	G_sprint(self, 2, "%-14s %-3d\n", redtext("Maxspeed"), (int)k_maxspeed);
	G_sprint(self, 2, "%-14s %-3d ",  redtext("Deathmatch"), (int)deathmatch);
	G_sprint(self, 2, "%-14s %-3d\n", redtext("Teamplay"), (int)tp_num());
	G_sprint(self, 2, "%-14s %-3d ",  redtext("Timelimit"), (int)timelimit);
	G_sprint(self, 2, "%-14s %-3d\n", redtext("Fraglimit"), (int)fraglimit);
	PrintToggle1(redtext("Powerups       "), "k_pow");
	PrintToggle2(va("%s \x98\x98\x98    ", redtext("Respawn")), "k_666");
	PrintToggle1(redtext("Drop Quad      "), "dq");
	PrintToggle2(redtext("Drop Ring      "), "dr");
	PrintToggle3(redtext("Fair Backpacks "), "k_frp");
	PrintToggle2(redtext("Drop Backpacks "), "dp");
	PrintToggle1(redtext("Discharge      "), "k_dis");
	PrintToggle2(redtext("Berzerk mode   "), "k_bzk");

	G_sprint(self, 2, "%-14s %-3.3s ",  redtext("spec info perm"), (mi_adm_only() ? "Adm" : "All"));
	G_sprint(self, 2, "%-14s %-3.3s\n", redtext("more spec info"), (mi_on() ? "On" : "Off"));

	if( match_in_progress == 1 ) {
		p = find(world, FOFCLSN, "timer" );
		if ( p )
			G_sprint(self, 2, "The match will start in %d second%s\n", (int)p->cnt2, count_s(p->cnt2));
		return;
	}

	if( (votes = get_votes( OV_ELECT )) )
		G_sprint(self, 2, "%s election in progress:\x90%d/%d\x91 vote%s\n",
							 redtext(get_elect_type_str()), votes,
							 get_votes_req( OV_ELECT, false ), count_s(votes));

	if( k_captains == 2 )
		G_sprint(self, 2, "%s in progress\n", redtext("Team picking"));

	if( floor( k_captains ) == 1 ) 
		G_sprint(self, 2, "\x90\x31\x91 %s present\n", redtext("captain"));

	if( match_in_progress == 2 ) {
		if( k_sudden_death )                
			G_sprint(self, 2, "%s overtime in progress\n", redtext("Sudden death"));
		else {
			p = find(world, FOFCLSN, "timer");
			if ( p )
				G_sprint(self, 2, "Match in progress\n"
								  "\x90%s\x91 full minute%s left\n",
									dig3(p->cnt - 1), count_s(p->cnt));
		}
	}
}

void ModStatus2()
{
	int i;
	char *ot = "";

	i = cvar( "k_spw" );
	if( i == 2 )
		G_sprint(self, 2, redtext("Kombat Teams Respawns\n"));
	else if ( i == 1 )
		G_sprint(self, 2, redtext("KT SpawnSafety\n"));
	else if ( i == 0 )
		G_sprint(self, 2, redtext("Normal QW Respawns\n"));
	else
		G_sprint(self, 2, redtext("Unknown Respawns\n"));

	if( isDuel() )
		G_sprint(self, 2, "%s: duel\n", redtext("Server mode"));
	else if ( isFFA() )
		G_sprint(self, 2, "%s:  FFA\n", redtext("Server mode"));
	else if ( isCTF() ) {
		G_sprint(self, 2, "%s:  CTF\n", redtext("Server mode"));
		G_sprint(self, 2, "%s: %s\n", redtext("Server locking"),
					 (!lock ? "off" : (lock == 2 ? "all" : (lock == 1 ? "team" : "unknown"))));
		G_sprint(self, 2, "%s: hook %s, runes %s\n", redtext("CTF settings"),
							 OnOff(cvar("k_ctf_hook")), OnOff(cvar("k_ctf_runes")));
	}
	else if ( isTeam() ) {
		G_sprint(self, 2, "%s: team\n", redtext("Server mode"));
		G_sprint(self, 2, "%s: %s\n", redtext("Server locking"),
					(!lock ? "off" : (lock == 2 ? "all" : (lock == 1 ? "team" : "unknown"))));
	}
	else
		G_sprint(self, 2, "%s: unknown\n", redtext("Server mode"));

	if( !match_in_progress )
		G_sprint(self, 2, "%s (%s: %d %s: %d %s: %d)\n", 
					redtext("Teaminfo"), redtext("cur"), (int)CountRTeams(),
					redtext("min"), (int)cvar("k_lockmin"),
					redtext("max"), (int)cvar("k_lockmax"));

	G_sprint(self, 2, "%s: %s\n", redtext("Spectalk"), OnOff( cvar("k_spectalk") ));

	i = cvar( "k_exttime" );
	switch ( (int)cvar( "k_overtime" ) ) {
		case 0:  ot = "off"; break;
		case 1:  ot = va("%d minute%s", i, count_s(i)); break;
		case 2:  ot = "sudden death"; break;
		default: ot	= "unknown"; break;
	}
	G_sprint(self, 2, "%s: %s\n", redtext("Overtime"), ot);

	i = iKey( world, "fpd" );

	G_sprint(self, 2, "%s: %s\n", redtext("QiZmo lag"),             OnOff( i &   8 ));
	G_sprint(self, 2, "%s: %s\n", redtext("QiZmo timers"),          OnOff( i &   2 ));
	G_sprint(self, 2, "%s: %s\n", redtext("QiZmo enemy reporting"), OnOff( i &  32 ));
	G_sprint(self, 2, "%s: %s\n", redtext("QiZmo pointing"),        OnOff( i & 128 ));

	G_sprint(self, 2, "%s: %s\n", redtext("Admin election"),
							 Allowed(cvar( "k_allowvoteadmin" )));

	G_sprint(self, 2, "%s: %s\n", redtext("Check frametimes"), Enabled( framechecks ));

	switch ( (int)cvar("k_prewar") ) {
		case  1: ot = "players may fire before match"; break;
		case  2: ot = "players may fire and jump when ready"; break;
		case  0:
		default: ot = "players may not fire before match"; break;
	}
	G_sprint(self, 2, "%s: %s\n", redtext("Prewar"), ot);

	if ( k_sv_locktime ) {
		int seconds = k_sv_locktime - g_globalvars.time;
		G_sprint(self, 2, "%s: %d second%s\n",
				 redtext("server is temporary locked"), seconds, count_s(seconds));
	}

	if ( k_cmd_fp_disabled ) 
		G_sprint(self, 2, "%s: off\n", redtext("Command floodprot"));
	else {
		G_sprint(self, 2, "%s: %d commands allowed per %d sec.,"
						   " skip commands for %d sec., ", redtext("Command floodprot"), 
								k_cmd_fp_count, (int)k_cmd_fp_per, (int) k_cmd_fp_for);

		if ( k_cmd_fp_dontkick ) 
			G_sprint(self, 2, "cmdfp kick disabled\n");
		else
			G_sprint(self, 2, "kick after %d warn.\n", k_cmd_fp_kick);
	}
}

void ModStatusVote()
{
	qboolean voted = false;
	int votes, i;
	int from;
	gedict_t *p;

	if ( k_matchLess || !match_in_progress )
	if ( vote_get_maps () >= 0 ) {
		voted = true;

		G_sprint(self, 2, "%s:\n", redtext("Map voting"));
		
		for( i = 0; i < MAX_CLIENTS; i++) {
			if (!maps_voted[i].map_id)
				break;

			G_sprint(self, 2, "\x90%s\x91 %2d vote%s\n", GetMapName( maps_voted[i].map_id ),
								maps_voted[i].map_votes, count_s(maps_voted[i].map_votes) );

			for( from = 0, p = world; (p = find_plrspc(p, &from)); )
				if ( p->v.map == maps_voted[i].map_id )
					G_sprint(self, 2, " %s\n", p->s.v.netname);
		}
	}

	if( !(get_elect_type() == etCaptain && match_in_progress) ) // does't show captain ellection in game
	if( (votes = get_votes( OV_ELECT )) ) {
		voted = true;

		G_sprint(self, 2, "\x90%d/%d\x91 vote%s for %s election:\n", votes, 
			get_votes_req( OV_ELECT, false ), count_s(votes), redtext(get_elect_type_str()) );

		for( from = 0, p = world; (p = find_plrspc(p, &from)); )
			if ( p->v.elect )
				G_sprint(self, 2, "%s%s\n", 
				(p->v.elect_type != etNone) ? "\x87" : " ", p->s.v.netname);
	}

	if ( !match_in_progress )
	if ( (votes = get_votes( OV_PICKUP ))) {
		voted = true;

		G_sprint(self, 2, "\x90%d/%d\x91 vote%s for a %s game:\n", votes,
			 get_votes_req( OV_PICKUP, false ), count_s(votes), redtext("pickup"));

		for( from = 0, p = world; (p = find_plrspc(p, &from)); )
			if ( p->v.pickup )
				G_sprint(self, 2, " %s\n", p->s.v.netname);
	}

	if ( !match_in_progress )
	if ( (votes = get_votes( OV_RPICKUP ))) {
		voted = true;

		G_sprint(self, 2, "\x90%d/%d\x91 vote%s for a %s game:\n", votes,
			 get_votes_req( OV_RPICKUP, false ), count_s(votes), redtext("rpickup"));

		for( from = 0, p = world; (p = find_plrspc(p, &from)); )
			if ( p->v.rpickup )
				G_sprint(self, 2, " %s\n", p->s.v.netname);
	}

	if( k_matchLess || match_in_progress == 2 )
	if ( (votes = get_votes( OV_BREAK )) ) {
		voted = true;

		G_sprint(self, 2, "\x90%d/%d\x91 vote%s for stopping:\n", votes,
			get_votes_req( OV_BREAK, false ), count_s(votes));

		for( from = 0, p = world; (p = find_plrspc(p, &from)); )
			if ( p->v.brk )
				G_sprint(self, 2, " %s\n", p->s.v.netname);
	}

	if ( voted )
		G_sprint(self, 2, "%s\n", redtext("--------------"));
	else
		G_sprint(self, 2, "%s\n", redtext("No election going on"));
}

char *OnePlayerStatus( gedict_t *p, gedict_t *e_self )
{
	char *team_str = (isTeam() ? va(" \x90%4.4s\x91", getteam( p )) : "");

	e_self = (e_self ? e_self : world);

	return va( "%s%s%s %s%s",
	 			( p->ready ? "\x86" : "\x87" ),	( is_adm( p ) ? "\xC1" : " " ),
				team_str, getname( p ), ( p == e_self ? redtext( " \x8D you" ) : "" ) );
}

void PlayerStatus()
{
	qboolean found = false;
	gedict_t *p;

	if( match_in_progress ) {
		G_sprint(self, 2, "Game in progress\n");
		return;
	}
	
	for ( p = world; (p = find( p, FOFCLSN, "player" )); ) {
		if ( !found )
			G_sprint(self, 2, "Players list:\n"
							  "\n");
		G_sprint(self, 2, "%s\n", OnePlayerStatus( p, self ));
		found = true;
	}
			
	G_sprint(self, 2, "%s\n", (found ? "" : "no players"));
}

void PlayerStatusS()
{
	qboolean found = false;
	gedict_t *p;

	for ( p = world; (p = find( p, FOFCLSN, "player" )); ) {
		if ( !found )
			G_sprint(self, 2, "Players skins list:\n"
							  "\n");
		G_sprint(self, 2, "\x90%10s\x91 %s\n", ezinfokey(p, "skin"), p->s.v.netname);
		found = true;
	}
			
	G_sprint(self, 2, "%s\n", (found ? "" : "no players"));
}

void PlayerStatusN()
{
	qboolean found = false;
	gedict_t *p;

	if( match_in_progress ) {
		G_sprint(self, 2, "Game in progress\n");
		return;
	}

	if( CountRPlayers() == CountPlayers() ) {
		G_sprint(self, 2, "All players ready\n");
		return;
	}

	for ( p = world; (p = find( p, FOFCLSN, "player" )); ) {
		if ( p->ready )
			continue;

		if ( !found )
			G_sprint(self, 2, "Players %s ready:\n"
						  "\n", redtext("not"));

		G_sprint(self, 2, "%s\n", OnePlayerStatus( p, self ));
		found = true;
	}
			
	G_sprint(self, 2, "%s\n", (found ? "" : "can't find not ready players"));
}

// broadcast not ready players

void ListWhoNot()
{
	qboolean found = false;
	gedict_t *p, *p2;
	int from;

	if( match_in_progress ) {
		G_sprint(self, 2, "Game in progress\n");
		return;
	}

	if( CountRPlayers() == CountPlayers() ) {
		G_sprint(self, 2, "All players ready\n");
		return;
	}

	if( self->k_player && !self->ready ) 
	{
		G_sprint(self, 2, "Ready yourself first\n");
		return;
	}	

	if( k_whonottime && g_globalvars.time < k_whonottime + 10 )
	{
        G_sprint(self, 2, "Only one %s in 10 seconds\n", redtext("list"));
		return;
	}

	k_whonottime = g_globalvars.time;

	for ( p = world; (p = find( p, FOFCLSN, "player" )); ) {
		if ( p->ready )
			continue;

		if ( !found )
			G_bprint(2, "Players %s ready:\n"
					    "\n", redtext("not")); // broadcast

		for ( from = 0, p2 = world; (p2 = find_plrspc(p2, &from)); )
			G_sprint(p2, 2, "%s\n", OnePlayerStatus( p, p2 ));

		found = true;
	}

	if ( found )			
		G_bprint(2, "\n"); // broadcats
	else
		G_sprint(self, 2, "can't find not ready players\n"); // self
}

void ResetOptions()
{
//	char *s1;

	if( match_in_progress )
		return;

	if( check_master() )
		return;

//	s1 = getteam( self );

#if 1 // TODO: make commented code safe or remove it
	{
		char *cfg_name = "configs/reset.cfg";
		char buf[1024*4];
		int um_idx;

		cvar_fset("_k_last_xonx", 0); // forget last XonX command

		if ( can_exec( cfg_name ) ) {
			trap_readcmd( va("exec %s\n", cfg_name), buf, sizeof(buf) );
			G_cprint("%s", buf);
		}

		if ( ( um_idx = um_idx_byname( cvar_string("k_defmode") ) ) >= 0 )
			UserMode( -(um_idx + 1) ); // force exec configs for default user mode
	}
#else
/*
	if( !is_adm(self) || strnull( s1 ) ) {
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
	int votes;

	if( match_in_progress )
		return;

	if( check_master() )
		return;

	if( k_captains ) {
		G_sprint(self, 2, "No pickup when captain stuffing\n");
		return;
	}

	self->v.pickup = !self->v.pickup;

	G_bprint(2, "%s %s %s%s\n", self->s.v.netname, 
					redtext("says"), (self->v.pickup ? "pickup!" : "no pickup"),
					((votes = get_votes_req( OV_PICKUP, true )) ? va(" (%d)", votes) : ""));

	vote_check_pickup ();
}

void ReportMe()
{
	gedict_t *p;
	char *t1, *t2 , *wt, *pa1, *pa2;
	float f1, flag = 0;

	if( !strnull( ezinfokey(self, "k_nick") ) || !strnull( ezinfokey(self, "k") ) )
		flag = 1;

	pa1 = "";
	pa2 = ": ";

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
			t1 = getteam( self );
			t2 = getteam( p );

			if( streq( t1, t2 ) ) {
				if( flag ) {
					t1 = ezinfokey(self, "k_nick");

					if ( strnull( t1 ) )
						t1 = ezinfokey(self, "k");

					G_sprint(p, 3, "%s: ", t1);
				}
				else
					G_sprint(p, 3, "%s%s%s", pa1, self->s.v.netname, pa2);

				if( self->s.v.armorvalue )
					G_sprint(p, 3, "%s:%d", armor_type((int)self->s.v.items),
									 (int)self->s.v.armorvalue);
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
	int k_spw = bound(0, cvar( "k_spw" ), 2);

	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	if ( ++k_spw > 2 )
		k_spw = 0;

	cvar_fset( "k_spw", k_spw );

	if( k_spw == 0 )
		G_bprint(2, "Normal QW respawns (avoid spawnfrags)\n");
	else if( k_spw == 1 )
		G_bprint(2, "KT SpawnSafety\n");
	else if( k_spw == 2 )
		G_bprint(2, "Kombat Teams respawns\n");
	else
		G_bprint(2, "Unknown respawns\n");
}

void ToggleRespawn666()
{
	if( match_in_progress )
		return;

	if( check_master() )
		return;

	cvar_toggle_msg( self, "k_666", va("%s \x98\x98\x98", redtext("Respawn")) );
}

void TogglePowerups()
{
	int k_pow = bound(0, cvar( "k_pow" ), 2); // here we are not using Get_Powerups

	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	if ( ++k_pow > 2 )
		k_pow = 0;

	cvar_fset("k_pow", k_pow);

	if ( !k_pow )
		G_bprint(2, "%s disabled\n", redtext("Powerups"));
	else if ( k_pow == 1 )
		G_bprint(2, "%s enabled\n", redtext("Powerups"));
	else if ( k_pow == 2 )
		G_bprint(2, "%s enabled (timer jammer)\n", redtext("Powerups"));
	else
		G_bprint(2, "%s unknown\n", redtext("Powerups"));
}

void ToggleDischarge()
{
	if( match_in_progress )
		return;

	if( check_master() )
		return;

	cvar_toggle_msg( self, "k_dis", redtext("discharges") );
}

void ShowDMM()
{
	G_sprint(self, 2, "Deathmatch %s\n", dig3(deathmatch));
}

void ChangeDM(float dmm)
{
	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	if ( deathmatch == (int)dmm ) {
		G_sprint(self, 2, "%s%s already set\n", redtext("dmm"), dig3(deathmatch));
		return;
	}

	// if leaving dmm4 force midair off
	if ( deathmatch == 4 )
		cvar_set( "k_midair", "0" );

	deathmatch = bound(1, (int)dmm, 5);

	cvar_set("deathmatch", va("%d", (int)deathmatch));

	G_bprint(2, "Deathmatch %s\n", dig3(deathmatch));
}

void ChangeTP()
{
	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	if( !isTeam() ) {
		G_sprint(self, 3, "console: non team mode disallows you to change teamplay setting\n");
		return;
	}

	teamplay = bound (1, teamplay, 3);

	teamplay++;

	if ( teamplay == 4 )
		teamplay = 1;

	cvar_fset("teamplay", (int)teamplay);

	G_bprint(2, "Teamplay %s\n", dig3(teamplay));
}

void TimeDown(float t)
{
//	char *tmp;

	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	timelimit -= t;

	if ( timelimit < 3 )
		timelimit = 3;

	cvar_set("timelimit", va("%d", (int)timelimit));

	G_bprint(2, "Νατγθ μεξητθ σετ το %d νιξυτεσ\n", (int)timelimit);
}

void TimeUp(float t)
{
	int top = 0;

	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	if (t == 5 && timelimit < 5)
		timelimit = 5;
	else
		timelimit += t;

	top = cvar( "k_timetop" );

	if( timelimit > top )
		timelimit = top;

	cvar_fset( "timelimit", (int)timelimit );

	G_bprint(2, "Νατγθ μεξητθ σετ το %d νιξυτεσ\n", (int)timelimit);
}

void TimeSet(float t)
{
	int top;

	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	timelimit = t;

	top = cvar( "k_timetop" );

	if( timelimit > top )
		timelimit = top;

	cvar_fset( "timelimit", (int)timelimit );

	G_bprint(2, "Νατγθ μεξητθ σετ το %d νιξυτεσ\n", (int)timelimit);
}

void FragsDown()
{
	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	fraglimit -= 10;

	if( fraglimit < 0 )
		fraglimit = 0;

	cvar_set("fraglimit", va("%d", (int)(fraglimit)));
	G_bprint(2, "Ζςαημινιτ σετ το %d\n", (int)(fraglimit));
}

void FragsUp()
{
	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	fraglimit += 10;

	if(fraglimit > 100)
		fraglimit = 100;

	cvar_set("fraglimit", va("%d", (int)(fraglimit)));
	G_bprint(2, "Ζςαημινιτ σετ το %d\n", (int)(fraglimit));
}

void ToggleDropQuad()
{
	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	cvar_toggle_msg( self, "dq", redtext("DropQuad") );
}

void ToggleDropRing()
{
	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	cvar_toggle_msg( self, "dr", redtext("DropRing") );
}

void ToggleDropPack()
{
	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	cvar_toggle_msg( self, "dp", redtext("DropPacks") );
}

void ToggleFairPacks()
{
	int k_frp = bound(0, cvar("k_frp"), 2);

    if ( match_in_progress )
		return;

	if( check_master() )
		return;

	if ( ++k_frp > 2 )
		k_frp = 0;

	cvar_fset( "k_frp", k_frp );

	if( !k_frp )
		G_bprint(2, "%s disabled\n", redtext("Fairpacks"));
	else if( k_frp == 1 )
		G_bprint(2, "%s enabled - drop best weapon\n", redtext("Fairpacks"));
	else if( k_frp == 2 )
		G_bprint(2, "%s enabled - drop last fired weapon\n", redtext("Fairpacks"));
	else
		G_bprint(2, "%s - unknown\n", redtext("Fairpacks"));
}

void ToggleSpeed()
{
	gedict_t *p;

	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	if( k_maxspeed != 320 )
		k_maxspeed = 320;
	else
		k_maxspeed = bound(0, cvar( "k_highspeed" ), 9999);

	G_bprint(2, "%s %d\n", redtext("Maxspeed set to"), (int)k_maxspeed);
	cvar_fset("sv_maxspeed", k_maxspeed);

	for( p = world; (p = find( p, FOFCLSN, "player" )); )
		p->maxspeed = k_maxspeed;
}

void ToggleBerzerk()
{
	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	cvar_toggle_msg( self, "k_bzk", redtext("BerZerk mode") );
}


void ToggleSpecTalk()
{
	int k_spectalk = !cvar( "k_spectalk" ), fpd = iKey( world, "fpd" );

	if ( match_in_progress && !is_adm( self ) )
		return;

	k_spectalk = bound(0, k_spectalk, 1);

	if( match_in_progress == 2 ) {

		fpd = ( k_spectalk ) ? (fpd & ~64) : (fpd | 64);

		localcmd("serverinfo fpd %d\n", fpd );
		cvar_fset( "sv_spectalk", k_spectalk );
		cvar_fset(  "k_spectalk", k_spectalk );

		if( k_spectalk )
			G_bprint(2, "Spectalk on: %s\n", redtext("players can now hear spectators"));
		else
			G_bprint(2, "Spectalk off: %s\n", redtext("players can no longer hear spectators"));

		return;

	} else {
		if( check_master() )
			return;

		cvar_fset( "k_spectalk", k_spectalk );

		if( k_spectalk )
			G_bprint(2, "Spectalk on: %s\n", redtext("players can hear spectators during game"));
		else
			G_bprint(2, "Spectalk off: %s\n", redtext("players cannot hear spectators during game"));
	}
}

void ShowRules()
{
	if( isDuel() )
		G_sprint(self, 2, "Server is in duel mode.\n");
	else if ( isCTF() )
		G_sprint(self, 2, "Server is in CTF mode.\n"
						  "Additional commands/impulses:\n"
						  "impulse 22 : Grappling Hook\n"
						  "tossrune   : Toss your current rune\n"
						  "flagstatus : Displays flag information\n");
	else if ( isFFA() )
		G_sprint(self, 2, "Server is in FFA mode.\n");
	else if ( isTeam() )
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
	else
		G_sprint(self, 2, "Server is in unknown mode.\n");

	if( cvar( "k_bzk" ) )
		G_sprint(self, 2,
			"\nBERZERK mode is activated!\n"
			"This means that when only %d seconds\n"
			"remains of the game, all players\n"
			"gets QUAD/OCTA powered.\n", (int)bound(0, cvar( "k_btime" ), 9999) );

	G_sprint(self, 2, "\n");
}

void ChangeLock()
{
	if ( match_in_progress )
		return;

	if( check_master() )
		return;

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
	char *sndname = va("ktsound%d.wav", (int)fsndname);

    p = find( world, FOFCLSN, "player" );
	while( p ) {
		if( p != self && (isTeam() || isCTF()) && !strnull( p->s.v.netname )
			&& ( iKey( p, "kf" ) & KF_KTSOUNDS ) 
		   ) {
			if( streq( getteam( self ), getteam( p ) ) ) {
				char *t1 = ezinfokey(p, "k_sdir");
				stuffcmd(p, "play %s%s\n", (strnull( t1 ) ? "" : va("%s/", t1)), sndname);
			}
		}

		p = find( p, FOFCLSN, "player" );
	}
}

void PrintScores()
{
	int minutes, seconds;
	gedict_t *p;

	if ( intermission_running ) {
		G_sprint(self, 2, "Intermission\n");
		return;
	}

	if( !match_in_progress ) {
		G_sprint(self, 2, "no game - no scores\n");
		return;
	}

	if( match_in_progress == 1 ) {
		G_sprint(self, 2, "Countdown\n");
		return;
	}

	if( k_sudden_death ) {
		G_sprint(self, 2, "Sudden death %s\n", redtext("overtime in progress"));
	}
	else {
		if ( fraglimit && (p = get_ed_scores1()) ) {
			int diff = fraglimit - p->s.v.frags;

			if ( diff >= 0 )
				G_sprint(self, 2, "Frags left: \x90%s\x91\n", dig3s("%2d", diff));
		}
	}

	if( (p = find(world, FOFCLSN, "timer")) ) {
		minutes = p->cnt;
		seconds = p->cnt2;
		if( seconds == 60 )
			seconds = 0;
		else
			minutes--;

		// we can't use dig3 here because of zero padding, so using dig3s
		G_sprint(self, 2, "\x90%s:%s\x91 remaining\n", 
							dig3s("%02d", minutes), dig3s("%02d", seconds));
	}

	if( k_showscores ) {
		int s1 = get_scores1();
		int s2 = get_scores2();
		char *t1 = cvar_string( "_k_team1" );
		char *t2 = cvar_string( "_k_team2" );

		G_sprint(self, 2, "%s \x90%s\x91 = %s\n", redtext("Team"),
								 (s1 > s2 ? t1 : t2), dig3(s1 > s2 ? s1 : s2));
		G_sprint(self, 2, "%s \x90%s\x91 = %s\n", redtext("Team"),
								 (s1 > s2 ? t2 : t1), dig3(s1 > s2 ? s2 : s1));
	}
}


void PlayerStats()
{
	gedict_t *p, *p2;
	char *tmp, *stats;
	int i, pL = 0, tL = 0;

	if( match_in_progress != 2 ) {
		G_sprint(self, 2, "no game - no statistics\n");
		return;
	}

	for ( p = world; (p = find( p, FOFCLSN, "player" )); ) {
		pL = max(pL, strlen(p->s.v.netname));
		tL = max(tL, strlen(getteam(p)));
	}
	pL = bound( 0, pL, 10 );
	tL = bound( 0, tL, 4 );

	G_sprint(self, 2, "%s:\n"
					  "%s %s %s \217  %s\n",
					   redtext("Player statistics"),
					   redtext("Frags"), redtext("rank"), isTeam() ? redtext("friendkills ") : "",
					   redtext("efficiency"));

	G_sprint(self, 2, "\235\236\236\236\236\236\236\236\236\236\236"
				  	  "\236\236\236\236\236\236\236\236\236\236\236\236\236%s\237\n",
				  	  (isTeam() ? "\236\236\236\236\236\236\236\236\236\236\236\236" : ""));

	for ( p = world; (p = find(p, FOFCLSN, "player" )); ) {
		if( !p->ready )
			continue; // already served

		tmp = getteam( p );

		for ( p2 = world; (p2 = find(p2, FOFCLSN, "player" )); ) {
			if( !p2->ready || strneq( tmp, getteam( p2 ) ) )
				continue; // already served or not on the same team

			if ( !isDuel() ) { // [team name]
				G_sprint(self, 2, "\220%.4s\221 ", tmp);
				for ( i = strlen(tmp); i < tL; i++ )
					G_sprint(self, 2, " ");
 			}

			G_sprint(self, 2, "%.10s ", p2->s.v.netname); // player name
			for ( i = strlen(p2->s.v.netname); i < pL; i++ )
				G_sprint(self, 2, " ");

			stats = va("%d", ( !isCTF() ? (int)p2->s.v.frags : (int)(p2->s.v.frags - p2->ps.ctf_points)));
			G_sprint(self, 2, "%3s", stats); // frags

			stats = va("%d", ( !isCTF() ? (int)(p2->s.v.frags - p2->deaths) : (int)(p2->s.v.frags - p2->ps.ctf_points - p2->deaths)));
			G_sprint(self, 2, "%4s ", stats); // rank

			if ( isTeam() ) { // friendkills
				stats = va("%d", (int)p2->friendly);
				G_sprint(self, 2, "%2s ", stats);
			}

			if ( isCTF() )
			{
				if ( p2->s.v.frags - p2->ps.ctf_points < 1 )
					p2->efficiency = 0;
				else
					p2->efficiency = (p2->s.v.frags - p2->ps.ctf_points) / (p2->s.v.frags - p2->ps.ctf_points + p2->deaths) * 100;
			}
			else
			{
				if( p2->s.v.frags < 1 )
					p2->efficiency = 0;
				else
					p2->efficiency = p2->s.v.frags / (p2->s.v.frags + p2->deaths) * 100;
			}
			
			stats = va("%3.1f", p2->efficiency);
			G_sprint(self, 2, "\217 %5s%%\n", stats); // effi

			p2->ready = 0; // mark as served
		}
	}

	for( p = world; (p = find( p, FOFCLSN, "player" )); )
		p->ready = 1; // because this is a hack, restore ready field
}

void ToggleQLag()
{
	int fpd = iKey( world, "fpd" );

	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	fpd ^= 8;

	localcmd("serverinfo fpd %d\n", fpd);

	G_bprint(2, "%s %s\n", 
			redtext("QiZmo lag settings"), ( (fpd & 8) ? "in effect" : "not in effect" ));
}

void ToggleQEnemy()
{
	int fpd = iKey( world, "fpd" );

	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	fpd ^= 32;

	localcmd("serverinfo fpd %d\n", fpd);

	G_bprint(2, "%s %s\n", 
			redtext("QiZmo enemy reporting"), Allowed( fpd & 32 ));
}

void ToggleQPoint()
{
	int fpd = iKey( world, "fpd" );

	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	fpd ^= 128;

	localcmd("serverinfo fpd %d\n", fpd);

	G_bprint(2, "%s %s\n", 
			redtext("QiZmo pointing"), Enabled( fpd & 128 ));
}

void ToggleFreeze()
{
	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	cvar_toggle_msg( self, "k_freeze", redtext(cvar("k_freeze") ? "unfreezes map" : "freezes map") );
}

// qqshka: pointing code stolen from Zquake

void ShowNick()
{
	gedict_t	*p, *bp = NULL;
	char		*s1, *s2, *pups, *kn, buf[256] = {0};
	float		best;
	vec3_t		forward, right, up;
	vec3_t		ang;
	vec3_t		vieworg, entorg;
	int			itms, i;

	if ( !match_in_progress )
		;  // allow shownick in prewar anyway
	else if ( !isTeam() && !isCTF() )
		return;

	ang[0] = self->s.v.v_angle[0];
	ang[1] = self->s.v.v_angle[1];
	ang[2] = 0;

	AngleVectors (ang, forward, right, up);

	VectorCopy (self->s.v.origin, vieworg);
	vieworg[2] += 16 /* was 22 */;	// adjust for view height

	best = -1;

	s1 = getteam( self );

	for ( p = world; (p = find(p, FOFCLSN, "player")); )
	{
		vec3_t	v, v2, v3;
		float dist, miss, rank;

		if ( ISDEAD(p) )
			continue; // ignore dead

		if ( p == self )
			continue; // ignore self

		if ( strnull( p->s.v.netname ) )
			continue; // ignore not really players

		if ( p->s.v.modelindex != modelindex_player )
			continue; // ignore non player mdl index (EYES)

		s2 = getteam( p );

		if ( !match_in_progress )
			;  // allow shownick in prewar anyway
		else if ( strneq( s1, s2 ) )
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

	if ((int)bp->s.v.armorvalue)
		s1 = va("%s:%d ", redtext( armor_type( itms ) ), (int)bp->s.v.armorvalue);
	else
		s1 = "";

	if      ( itms & IT_ROCKET_LAUNCHER )  s2 = va(" %s:%d", redtext("rl"), (int)bp->s.v.ammo_rockets);
	else if ( itms & IT_LIGHTNING )        s2 = va(" %s:%d", redtext("lg"), (int)bp->s.v.ammo_cells);
	else if ( itms & IT_GRENADE_LAUNCHER ) s2 = va(" %s:%d", redtext("gl"), (int)bp->s.v.ammo_rockets);
	else if ( itms & IT_SUPER_NAILGUN )    s2 = va(" %s:%d", redtext("sng"),(int)bp->s.v.ammo_nails);
	else if ( itms & IT_SUPER_SHOTGUN )    s2 = va(" %s:%d", redtext("ssg"),(int)bp->s.v.ammo_shells);
	else if ( itms & IT_NAILGUN )          s2 = va(" %s:%d", redtext("ng"), (int)bp->s.v.ammo_nails);
	else if ( itms & IT_SHOTGUN )          s2 = va(" %s:%d", redtext("sg"), (int)bp->s.v.ammo_shells);
	else if ( itms & IT_AXE )              s2 = redtext(" axe!"); // just in case :]
	else
		s2 = "";

	kn = ezinfokey( bp, "k_nick" );
	if ( strnull( kn ) )
		kn = ezinfokey( bp, "k" );

	if ( (i = iKey( self, "ln" )) > 0 ) {
		i = bound(0, i, sizeof(buf)-1 );
		memset( (void*)buf, (int)'\n', i);
		buf[i] = 0;
	}

	if ( !match_in_progress )	// simple shownick in prewar
		strlcat(buf, va( "%s\n", ( strnull( kn ) ? bp->s.v.netname : kn )), sizeof(buf));
	else
		strlcat(buf, va(	"%s" // if powerups present \n is too
						"%s%s:%d%s\n"
							"%s\n" , pups,
						 	s1, redtext("h"), (int)bp->s.v.health, s2,
									( strnull( kn ) ? bp->s.v.netname : kn )), sizeof(buf));

	if ( (i = iKey( self, "ln" )) < 0 ) {
		int offset = strlen(buf);
		i = bound(0, -i, (int)sizeof(buf) - offset - 1);
		memset( (void*)(buf + offset), (int)'\n', i);
		buf[i+offset] = 0;
	}

	G_centerprint(self, "%s", buf);

	self->need_clearCP  = 1;
	self->shownick_time = g_globalvars.time + 0.8; // clear centerprint at this time
}

// qqshka

// below predefined settings for usermodes
// I ripped this from ktpro

// common settings for all user modes
const char common_um_init[] =
	"maxclients 8\n"
	"k_disallow_weapons 16\n"			// disallow gl in dmm4 by default

//	"localinfo k_new_mode 0\n" 			// UNKNOWN ktpro
//	"localinfo k_fast_mode 0\n"			// UNKNOWN ktpro
//	"localinfo k_safe_rj 0\n"           // UNKNOWN ktpro

	"k_spec_info 1\n"					// allow spectators receive took info during game
	"k_rocketarena 0\n"					// rocket arena
	"k_midair 0\n"						// midair off
//	"localinfo k_instagib 0\n"			// not implemented
//	"localinfo k_new_spw 0\n"			// ktpro feature

	"fraglimit 0\n"						// fraglimit %)
	"k_666 0\n"							// respawn 666
	"dp 1\n"							// drop pack
	"dq 0\n"							// drop quad
	"dr 0\n"							// drop ring
	"k_frp 0\n"							// fairpacks
	"k_spectalk 0\n"					// silence
	"k_dis 1\n"							// discharge on
	"k_bzk 0\n"							// berzerk
	"k_spw 2\n"							// affect spawn type
	"k_dmgfrags 0\n"					// damage frags off
      
	"k_membercount 0\n"					// some unlimited values
	"k_lockmin 0\n"						// some unlimited values
	"k_lockmax 64\n";         			// some unlimited values


const char _1on1_um_init[] =
	"maxclients 2\n"
	"timelimit  10\n"					//
	"teamplay   0\n"					//
	"deathmatch 3\n"					//
	"k_overtime 1\n"					// overtime type
	"k_exttime 3\n"						// overtime 3mins
	"k_pow 0\n"							// powerups
	"k_membercount 0\n"					// no efect in duel
	"k_lockmin 0\n"						// no efect in duel
	"k_lockmax 0\n"           			// no efect in duel
	"k_mode 1\n";

const char _2on2_um_init[] =
	"maxclients 4\n"
	"floodprot 10 1 1\n"				//
	"localinfo k_fp 1\n"				//
	"timelimit  10\n"					//
	"teamplay   2\n"					//
	"deathmatch 3\n"					//
	"k_overtime 1\n"					// overtime type
	"k_exttime 3\n"						// extende time for overtime
	"k_pow 1\n"							//
	"k_membercount 1\n"					// minimum number of players in each team
	"k_lockmin 1\n"						//
	"k_lockmax 2\n"           			//
	"k_mode 2\n";

const char _3on3_um_init[] =
	"maxclients 6\n"
	"floodprot 10 1 1\n"
	"localinfo k_fp 1\n"
	"timelimit  15\n"
	"teamplay   2\n"
	"deathmatch 1\n"
	"k_pow 1\n"
	"k_membercount 2\n"					// minimum number of players in each team
	"k_lockmin 1\n"						//
	"k_lockmax 2\n"           			//
	"k_overtime 1\n"
	"k_exttime 5\n"
	"k_mode 2\n";

const char _4on4_um_init[] =
	"maxclients 8\n"
	"floodprot 10 1 1\n"
	"localinfo k_fp 1\n"
	"timelimit  20\n"
	"teamplay   2\n"
	"deathmatch 1\n"
	"k_pow 1\n"
	"k_membercount 3\n"					// minimum number of players in each team
	"k_lockmin 1\n"						//
	"k_lockmax 2\n"           			//
	"k_overtime 1\n"
	"k_exttime 5\n"
	"k_mode 2\n";

const char _10on10_um_init[] =
	"maxclients 20\n"
	"floodprot 10 1 1\n"
	"localinfo k_fp 1\n"
	"timelimit  20\n"
	"teamplay   2\n"
	"deathmatch 1\n"
	"k_pow 1\n"
	"k_membercount 5\n"					// minimum number of players in each team
	"k_lockmin 1\n"						//
	"k_lockmax 2\n"           			//
	"k_overtime 1\n"
	"k_exttime 5\n"
	"k_mode 2\n";

const char ffa_um_init[] =
	"maxclients 26\n"
	"timelimit  20\n"
	"teamplay   0\n"
	"deathmatch 3\n"
	"dq 1\n"
	"dr 1\n"
	"k_pow 1\n"
	"k_membercount 0\n"					// no effect in ffa
	"k_lockmin 0\n"						// no effect in ffa
	"k_lockmax 0\n"           			// no effect in ffa
//	"k_overtime 1\n"
//	"k_exttime 5\n"
	"k_mode 3\n";

const char ctf_um_init[] =
	"maxclients 16\n"
	"timelimit  20\n"
	"teamplay   2\n"
	"deathmatch 3\n"
	"k_dis 2\n"						// no out of water discharges in ctf          
	"k_pow 1\n"
	"k_spw 1\n"
	"k_membercount 0\n"          
	"k_lockmin 1\n"                                         
	"k_lockmax 2\n"                                 
	"k_overtime 1\n"
	"k_exttime 5\n"
	"k_mode 4\n"
	"k_ctf_hook 1\n"				// hook on
	"k_ctf_runes 1\n";				// runes on


usermode um_list[] =
{
	{ "1on1", 	"\x93 on \x93",			_1on1_um_init,		UM_1ON1},
	{ "2on2",	"\x94 on \x94",			_2on2_um_init,		UM_2ON2},
	{ "3on3",	"\x95 on \x95",			_3on3_um_init,		UM_3ON3},
	{ "4on4",	"\x96 on \x96",			_4on4_um_init,		UM_4ON4},
	{ "10on10",	"\x93\x92 on \x93\x92",	_10on10_um_init,	UM_10ON10},
	{ "ffa",	"ffa",					ffa_um_init,		UM_FFA},
	{ "ctf",	"ctf",					ctf_um_init,		UM_CTF}
};

int um_cnt = sizeof (um_list) / sizeof (um_list[0]);

// return -1 if not found
int um_idx_byname(char *name)
{
	int i;

	if ( strnull( name ) )
		return -1; // not found

	for( i = 0; i < um_cnt; i++ )
		if ( streq( name, um_list[i].name) )
			return i;

	return -1; // not found
}

// for user call this like UserMode( 1 )
// for server call like UserMode( -1 )
void UserMode(float umode)
{
	const char *um;
	char buf[1024*4];
	char *cfg_name;
	qboolean sv_invoked = false;

	int k_free_mode = ( k_matchLess ? 5 : cvar( "k_free_mode" ) );

	if ( !k_matchLess ) // allow for matchless mode
	if ( match_in_progress )
		return;

	if ( umode < 0 ) {
		sv_invoked = true;
		umode *= -1;
	}

	umode = (int)umode - 1;

	if ( umode < 0 || umode >= um_cnt ) {
		G_bprint(2, "UserMode: unknown mode\n");
		return;
	}

	um = um_list[(int)umode].name;

	if ( streq(um, "ffa") && k_matchLess && cvar("k_use_matchless_dir") )
		um = "matchless"; // use configs/usermodes/matchless instead of configs/usermodes/ffa in matchless mode

	if ( sv_invoked ) {
		if ( !k_matchLess ) // allow for matchless mode
		if ( cvar( "k_master" ) ) {
			G_bprint(2, "UserMode: sv %s discarded due to k_master\n", um);
			return;
		}
	}
	else if( check_master() )
		return;

//for 1on1 / 2on2 / 4on4 and ffa commands manipulation
//0 - noone, 1 - admins, 2 elected admins too
//3 - only real real judges, 4 - elected judges too
//5 - all players

// hmm, I didn't understand how k_free_mode affect this command,
// so implement how i think this must be, it is like some sort of access control
	if ( sv_invoked ) {
		if ( k_free_mode != 5 ) {
			G_bprint(2, "UserMode: sv %s discarded due to k_free_mode\n", um);
			return;
		}
	}
	else {
		switch ( k_free_mode ) {
			case 0:	G_sprint(self, 2, "%s can use this command\n", redtext("noone"));
					return;
			case 1:
			case 2:	if ( !is_adm( self ) ) {
						G_sprint(self, 2, "you must be an %s\n", redtext("admin"));
						return;
					}
					break;
			case 3:
			case 4:	if ( !is_adm( self ) ) {
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
	}

// ok u have access, but is this command really allowed at all?

	if ( !(um_list[(int)umode].um_flags & k_allowed_free_modes) ) {

		if ( sv_invoked )
			G_bprint(2, "UserMode: sv %s discarded due to k_allowed_free_modes\n", um);
		else
			G_sprint(self, 2, "server configuration %s this command\n", redtext("lock"));

		return;
	}

	// We invoke ctf command.
	// So we need check ready players, and if they have wrong teams, discard ctf command
	// untill they type break or fix team names.
	// After ctf mode activated no one can have wrong team and be ready at the same time.
	if ( !isCTF() && (um_list[(int)umode].um_flags & UM_CTF) ) {
		gedict_t	*p;

		for( p = world; (p = find ( p, FOFCLSN, "player" )); )
			if ( p->ready && (!streq(getteam(p), "blue") && !streq(getteam(p), "red")) )
			{
				if ( sv_invoked )
					G_bprint(2, "UserMode: sv %s discarded due to ready players have not red or blue team\n", um);
				else
					G_sprint(self, 2, "command discarded due to ready players have not red or blue team\n"
									  "either force they fix team or be not ready\n" );

				return;
			}
	}

	if ( !k_matchLess ) // do not show for matchless mode
		G_bprint(2, "%s %s\n", redtext(um_list[(int)umode].displayname), redtext("settings enabled"));

	trap_readcmd( common_um_init, buf, sizeof(buf) );
	G_cprint("%s", buf);

	trap_readcmd( um_list[(int)umode].initstring, buf, sizeof(buf) );
	G_cprint("%s", buf);

	cfg_name = "configs/usermodes/default.cfg";
	if ( can_exec( cfg_name ) ) {
		trap_readcmd( va("exec %s\n", cfg_name), buf, sizeof(buf) );
		G_cprint("%s", buf);
	}
	cfg_name = va("configs/usermodes/%s/default.cfg", um);
	if ( can_exec( cfg_name ) ) {
		trap_readcmd( va("exec %s\n", cfg_name), buf, sizeof(buf) );
		G_cprint("%s", buf);
	}
	cfg_name = va("configs/usermodes/%s.cfg", g_globalvars.mapname);
	if ( can_exec( cfg_name ) ) {
		trap_readcmd( va("exec %s\n", cfg_name), buf, sizeof(buf) );
		G_cprint("%s", buf);
	}
	cfg_name = va("configs/usermodes/%s/%s.cfg", um, g_globalvars.mapname);
	if ( can_exec( cfg_name ) ) {
		trap_readcmd( va("exec %s\n", cfg_name), buf, sizeof(buf) );
		G_cprint("%s", buf);
	}

// { RA hack
	if ( streq(um, "1on1") ) {
		cfg_name = va("configs/usermodes/%s/ra/%s.cfg", um, g_globalvars.mapname);
		if ( can_exec( cfg_name ) ) {
			trap_readcmd( va("exec %s\n", cfg_name), buf, sizeof(buf) );
			G_cprint("%s", buf);
		}
	}
// }

	G_cprint("\n");

	cvar_fset("_k_last_xonx", umode+1); // save last XonX command
}

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
		G_sprint(self, 2, "You have already voted\n");
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

// ok, a bit complicated
// this routine may change map if srv_practice_mode == 0 and mapname is not NULL
void SetPractice(int srv_practice_mode, const char *mapname)
{
	if ( match_in_progress )
		G_Error ("SetPractice: match_in_progress");

	k_practice = srv_practice_mode;
	cvar_fset("srv_practice_mode", srv_practice_mode);

	if ( k_practice )
		G_bprint(2, "%s\n", redtext("Server in practice mode"));
	else {
		G_bprint(2, "%s\n", redtext("Server in normal mode"));
		if ( mapname ) // mapname may be "" i.e empty, reload current map in this case
			changelevel( ( strnull( mapname ) ? g_globalvars.mapname : mapname ) );
	}
}

void TogglePractice()
{
	int lock_practice         = cvar( "lock_practice" );
	int allow_toggle_practice = cvar( "allow_toggle_practice" );

	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	if(     lock_practice == 2 /* server locked in current practice mode */
		|| (lock_practice != 0 && lock_practice != 1) /* unknown lock type, ignore command */
	  ) {
		G_sprint(self, 3, "console: command is locked\n");
		return;
	}

	if ( k_force || find ( world, FOFCLSN, "idlebot" ) )
		return;  // cmon, no practice if forcestart or idlebot active

//0 - noone, 1 - admins, 2 elected admins too
//3 - only real real judges, 4 - elected judges too
//5 - all players

// implement how i think this must be, it is like some sort of access control

	switch ( allow_toggle_practice ) {
		case 0:	G_sprint(self, 2, "%s can use this command\n", redtext("noone"));
				return;
		case 1:
		case 2:	if ( !is_adm( self ) ) {
					G_sprint(self, 2, "you must be an %s\n", redtext("admin"));
					return;
				}
				break;
		case 3:
		case 4:	if ( !is_adm( self ) ) {
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

// ok u have access
	SetPractice( !k_practice, "" ); // reload current map if needed
}

// allow reset weapon stats in prewar
void Wp_Reset ( )
{
	if ( match_in_progress )
		return;

	self->ps.h_axe = self->ps.a_axe = self->ps.h_sg = self->ps.a_sg  = 0;
	self->ps.h_ssg = self->ps.a_ssg = 0;
	self->ps.h_ng  = self->ps.a_ng  = self->ps.h_sng= self->ps.a_sng = 0;
	self->ps.h_gl  = self->ps.a_gl  = 0;
	self->ps.h_rl  = self->ps.a_rl  = self->ps.h_lg = self->ps.a_lg  = 0;
}

void Wp_Stats(float on)
{
	on--;

	self->wp_stats = (int)on;
	self->wp_stats_time = g_globalvars.time; // force show/hide
}

void Sc_Stats(float on)
{
	on--;

	self->sc_stats = (int)on;
	self->sc_stats_time = g_globalvars.time; // force show/hide
}

void W_WeaponFrame();

void kfjump ()
{
	int button0 = self->s.v.button0;

	if ( cvar( "k_disallow_kfjump" ) ) {
		G_sprint(self, 2, "%s is disabled\n", redtext("kfjump"));
		return;
	}

	self->s.v.impulse = 7;		 // select switch to rl
	self->s.v.button0 = 1;		 // force attack button
	self->s.v.v_angle[1] += 180; // turn 180
	W_WeaponFrame ();			 // switch to rl and fire
	self->s.v.v_angle[1] -= 180; // turn back
	self->s.v.button0 = button0; // restore button state
}

void krjump ()
{
	int button0 = self->s.v.button0;
	float va_x = self->s.v.v_angle[0];

	if ( cvar( "k_disallow_krjump" ) ) {
		G_sprint(self, 2, "%s is disabled\n", redtext("krjump"));
		return;
	}

	self->s.v.impulse = 7;		 // select switch to rl
	self->s.v.button0 = 1;		 // force attack button
	self->s.v.v_angle[0] = 80;   // look down much as possible, qw block this at 80
	W_WeaponFrame ();			 // switch to rl and fire
	self->s.v.v_angle[0] = va_x; // restore
	self->s.v.button0 = button0; // restore button state
}

void t_jump (float j_type)
{
	char *jt, *cv_jt, cjt = j_type == 1 ? 'f' : 'r';

	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	jt    = va("k%cjump", cjt);
	cv_jt = va("k_disallow_k%cjump", cjt);

	trap_cvar_set_float( cv_jt, !cvar( cv_jt ) );
	G_bprint(2, "%s %s %s\n", self->s.v.netname, redtext( Enables( !cvar( cv_jt ) ) ),
							  redtext( jt ) );
}

void klist ( )
{
	int i, hdc;
	gedict_t *p = world;
	char *track;

	for( i = 0, p = world; (p = find(p, FOFCLSN, "player")); i++ ) {
		if ( !i ) {
			G_sprint(self, 2, "Clients list: %s\n", redtext( "players" ) );
			G_sprint(self, 2, "%s %s %s %s %s %s\n",
 						redtext( "id" ), redtext( "ad" ), redtext( "vip" ),
						redtext( "hdp" ), redtext( "team" ), redtext( "name" ) );
		}

		hdc = GetHandicap(p);

		G_sprint(self, 2, "%2d|%2s|%3d|%3s|%4.4s|%s\n", GetUserID( p ),
						(is_adm( p ) ? redtext("A") : ""), VIP( p ),
						(hdc == 100 ? "off" : va("%d%%", hdc)), getteam( p ), getname( p ));
	}

	if (i)
		G_sprint(self, 2, "%s %2d found %s\n", redtext("--"), i, redtext("-------------") );

	for( i = 0, p = world; (p = find(p, FOFCLSN, "spectator")); i++ ) {
		if ( !i ) {
			G_sprint(self, 2, "Clients list: %s\n", redtext( "spectators" ) );
			G_sprint(self, 2, "%s %s %s %s\n",
 						redtext( "id" ), redtext( "ad" ), redtext( "vip" ),
						redtext( "name" ) );
		}

		track = TrackWhom( p );

		G_sprint(self, 2, "%2d|%2s|%3d|%s%s\n", GetUserID( p ),
						(is_adm( p ) ? redtext("A") : ""), VIP( p ), getname( p ),
						(strnull(track) ? "" : va(" \x8D %s", track)) );
	}

	if (i)
		G_sprint(self, 2, "%s %2d found %s\n", redtext("--"), i, redtext("-------------") );

	for( i = 0, p = world; (p = find(p, FOFCLSN, "ghost")); i++ ) {
		if ( !i ) {
			G_sprint(self, 2, "Clients list: %s\n", redtext( "ghosts" ) );
			G_sprint(self, 2, "%s %s %s\n",
						 redtext( "frags" ), redtext( "team" ), redtext( "name" ) );
		}
		G_sprint(self, 2, "%5d|%4.4s|%s\n",	(int)p->s.v.frags, getteam( p ), getname( p ));
	}

	if (i)
		G_sprint(self, 2, "%s %2d found %s\n", redtext("--"), i, redtext("-------------") );

	for( i = 0, p = world + 1; p <= world + MAX_CLIENTS; p++ ) {
		if ( streq(track = ezinfokey(p, "*state"), "zombie") )
			;
		else if ( streq(track, "preconnected") || streq(track, "connected") )
			track = "connecting";
		else
			continue; // continue due to player spawned(connected) or free or in unknown state

		if ( !i ) {
			G_sprint(self, 2, "Clients list: %s\n", redtext( "unconnected" ) );
			G_sprint(self, 2, "%s %s %-10s %s\n",
						 redtext( "id" ), redtext( "vip" ), redtext( "state" ), redtext( "name" ) );
		}

		G_sprint(self, 2, "%2d|%3d|%-10.10s|%s\n", 
					iKey(p, "*userid"), // can't use GetUserID here
					VIP( p ), track, (strnull( p->s.v.netname ) ? "!noname!" : p->s.v.netname));

		i++;
	}

	if (i)
		G_sprint(self, 2, "%s %2d found %s\n", redtext("--"), i, redtext("-------------") );
}

void hdptoggle ()
{
	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	trap_cvar_set_float( "k_lock_hdp", !cvar( "k_lock_hdp" ) );
	G_bprint(2, "%s %s %s\n", self->s.v.netname,
				redtext( Allows( !cvar( "k_lock_hdp" ) ) ), redtext( "handicap" ) );
}

void handicap ()
{
	char arg_2[1024];
	int hdc = GetHandicap(self);

	if ( trap_CmdArgc() == 2 ) {
		trap_CmdArgv( 1, arg_2, sizeof( arg_2 ) );
		hdc = atoi(arg_2);
	}
	else if ( hdc > 85 )
		hdc = 85;
	else if ( hdc > 70 )
		hdc = 70;
	else if ( hdc > 55 )
		hdc = 55;
	else if ( hdc > 40 )
		hdc = 40;
	else
		hdc = 100;

	SetHandicap(self, hdc);
}

void show_disallowed_weapons( int k_disallow_weapons )
{
	char *dwp = str_noweapon( k_disallow_weapons );

	G_sprint(self, 2, "weapons disallowed:%s\n", 
				( strnull( dwp ) ? redtext( " none" ) : redtext( dwp ) ) );
}

void noweapon ()
{
	char arg_2[1024];
	int	k_disallow_weapons = (int)cvar("k_disallow_weapons") & DA_WPNS;

	if ( match_in_progress ) {
		if ( deathmatch == 4 ) // match started, show info and return
			show_disallowed_weapons( k_disallow_weapons );
		return;
	}

	if( check_master() )
		return;

	if ( deathmatch != 4 ) {
		G_sprint(self, 2, "command allowed in %s only\n", redtext("dmm4"));
		return;
	}

	if ( trap_CmdArgc() == 1 ) { // no arguments, show info and return
		show_disallowed_weapons( k_disallow_weapons );
		return;
	}

	// one argument
	if ( trap_CmdArgc() == 2 ) {
		char *nwp = NULL;
		int bit = 0;

		trap_CmdArgv( 1, arg_2, sizeof( arg_2 ) );

		if ( streq( nwp = "axe", arg_2 ) )
			k_disallow_weapons ^= bit = IT_AXE;
		else if ( streq( nwp = "sg", arg_2 ) )
			k_disallow_weapons ^= bit = IT_SHOTGUN;
		else if ( streq( nwp = "ssg", arg_2 ) )
			k_disallow_weapons ^= bit = IT_SUPER_SHOTGUN;
		else if ( streq( nwp = "ng", arg_2 ) )
			k_disallow_weapons ^= bit = IT_NAILGUN;
		else if ( streq( nwp = "sng", arg_2 ) )
			k_disallow_weapons ^= bit = IT_SUPER_NAILGUN;
		else if ( streq( nwp = "gl", arg_2 ) )
			k_disallow_weapons ^= bit = IT_GRENADE_LAUNCHER;
		else if ( streq( nwp = "rl", arg_2 ) )
			k_disallow_weapons ^= bit = IT_ROCKET_LAUNCHER;
		else if ( streq( nwp = "lg", arg_2 ) )
			k_disallow_weapons ^= bit = IT_LIGHTNING;

		if ( bit ) {
			G_bprint(2, "%s %s %s\n", self->s.v.netname,
				redtext( Allows( !(k_disallow_weapons & bit) ) ), redtext( nwp ) );
			trap_cvar_set_float( "k_disallow_weapons", k_disallow_weapons );
		}
		else {
			G_sprint(self, 2, "unknown weapon name %s\n", redtext(arg_2));
		}
		return;
	}
}

void tracklist ( )
{
	int i;
	gedict_t *p;
	char *track;
	char *nt = redtext(" not tracking");

	for( i = 0, p = world; (p = find(p, FOFCLSN, "spectator")); i++ ) {
		if ( !i )
			G_sprint(self, 2, "%s:\n", redtext( "Trackers list" ) );

		track = TrackWhom( p );

		G_sprint(self, 2, "%15s%s\n", getname( p ),
							(strnull(track) ? nt : va(" \x8D %s", track)) );
	}

	if ( !i )
		G_sprint(self, 2, "No spectators present\n" );
}

void fpslist ( )
{
	int i;
	gedict_t *p;
	float cur, max, min, avg;

	for( i = 0, p = world; (p = find(p, FOFCLSN, "player")); i++ ) {
		if ( !i ) {
			G_sprint(self, 2, "Players %s list:\n", redtext("FPS") );
			G_sprint(self, 2, "         name:(cur \x8f max \x8f min \x8f avg)\n");
			G_sprint(self, 2, "\n");
		}

		cur = p->fCurrentFrameTime ? ( 1.0f / p->fCurrentFrameTime ) : 0;
		max = p->fLowestFrameTime ? ( 1.0f / p->fLowestFrameTime ) : 0;
		min = p->fHighestFrameTime ? ( 1.0f / p->fHighestFrameTime ) : 0;

		avg = p->fFrameCount ? ( p->fAverageFrameTime / p->fFrameCount ) : 0;
		avg = avg ? (1.0f / avg) : 0;

		G_sprint(self, 2, "%13s: %3d \x8f %3d \x8f %3d \x8f%5.1f\n", getname( p ),
				Q_rint(cur), Q_rint(max), Q_rint(min), avg);
	}

	if ( !i )
		G_sprint(self, 2, "No players present\n" );
}

// This is designed for pickup games and creates totally random teams(ish)
// It creates teams thus :
// Team red  color  4 skin ""
// team blue color 13 skin ""
void RandomPickup ()
{
    int votes;
	
	if( match_in_progress )
        return;

	if( check_master() )
		return;

	if( k_captains ) {
		G_sprint(self, 2, "No random pickup when captain stuffing\n");
		return;
	}

    // Dont need to bother if less than 4 players
    if( CountPlayers() < 4 )
    {
        G_sprint(self, 2, "You need at least 4 players to do this.\n");
        return;
    }

	self->v.rpickup = !self->v.rpickup;

	G_bprint(2, "%s %s!%s\n", self->s.v.netname, 
			(self->v.rpickup ? redtext("votes for rpickup") :
							   redtext("withdraws %s rpickup vote", g_his(self))),
			((votes = get_votes_req( OV_RPICKUP, true )) ? va(" (%d)", votes) : ""));

	vote_check_rpickup ();
}

// { spec tracking stuff 

qboolean fav_del_do(gedict_t *s, gedict_t *p, char *prefix);
qboolean favx_del_do(gedict_t *s, gedict_t *p, char *prefix);

// this is called from ClientDisconnect - so disconnected players
// removed from spectators favourites
void del_from_specs_favourites(gedict_t *rm)
{
	int from;
	gedict_t *p;

	for( from = 1, p = world; (p = find_plrspc(p, &from)); ) {
		fav_del_do(p, rm, "auto: ");
		favx_del_do(p, rm, "auto: ");
	}
}

void fav_add( )
{
	int fav_num, free_num;
	gedict_t *goal = PROG_TO_EDICT(self->s.v.goalentity);
	int diff = (int)(goal - world);

	if ( !goal->k_player || diff < 1 || diff > MAX_CLIENTS ) {
		G_sprint(self, 2, "fav_add: you are %s player!\n", redtext("not tracking"));
		return;
	}

	for ( free_num = -1, fav_num = 0; fav_num < MAX_CLIENTS; fav_num++ )
		if ( self->fav[fav_num] == diff ) {
			G_sprint(self, 2, "fav_add: %s %s added to favourites\n", goal->s.v.netname,
																	redtext("already"));
			return;
		}
		else if ( free_num < 0 && !self->fav[fav_num] ) { // ok - found free slot
			free_num = fav_num;
		}

	fav_num = free_num + 1;

	if ( fav_num < 1 || fav_num > MAX_CLIENTS ) { // must not happen
		G_sprint(self, 2, "fav_add: oops, all slots busy? Can't add.\n");
		return;
	}
	
	G_sprint(self, 2, "fav_add: %s added to favourites\n", goal->s.v.netname);

	self->fav[(int)fav_num - 1] = diff;
}


// s - for whom remove
// p - who removed
qboolean fav_del_do(gedict_t *s, gedict_t *p, char *prefix)
{
	qboolean removed = false;
	int fav_num;

	if ( !s || !p )
		return false;

	for ( fav_num = 0; fav_num < MAX_CLIENTS; fav_num++ )
		if ( s->fav[fav_num] && (world + s->fav[fav_num]) == p ) {
			if ( removed == false ) // show info one time
				G_sprint(s, 2, "%s%s removed from favourites\n", 
							prefix, (strnull(p->s.v.netname) ? "-someone-" : p->s.v.netname));

			s->fav[fav_num] = 0;
			removed = true; // does't break, so if this player multiple times in favourites
							// he will removed anyway, must not happend really
		}

	return removed;
}

// s - for whom remove
// p - who removed
qboolean favx_del_do(gedict_t *s, gedict_t *p, char *prefix)
{
	qboolean removed = false;
	int fav_num;

	if ( !s || !p )
		return false;

	for ( fav_num = 0; fav_num < MAX_CLIENTS; fav_num++ )
		if ( s->favx[fav_num] && (world + s->favx[fav_num]) == p ) {
			G_sprint(s, 2, "%s%s removed from \x90slot %2d\x91\n", 
				prefix, (strnull(p->s.v.netname) ? "-someone-" : p->s.v.netname), fav_num + 1);

			s->favx[fav_num] = 0;
			removed = true; // does't break, so if this player multiple times in favourites
							// he will removed anyway
		}

	return removed;
}

void fav_del( )
{
	gedict_t *goal = PROG_TO_EDICT(self->s.v.goalentity);
	int diff = (int)(goal - world);

	if ( !goal->k_player || diff < 1 || diff > MAX_CLIENTS ) {
		G_sprint(self, 2, "fav_del: you are %s player!\n", redtext("not tracking"));
		return;
	}

	if ( fav_del_do(self, goal, "fav_del: ") )
		return;

	G_sprint(self, 2, "fav_del: %s is %s favourites\n", goal->s.v.netname, redtext("not in"));
}

void fav_all_del( )
{
	qboolean deleted = false;
	int fav_num;

	for ( fav_num = 0; fav_num < MAX_CLIENTS; fav_num++ )
		if ( self->fav[fav_num] ) {
			self->fav[fav_num] = 0;
			deleted = true;
		}

	G_sprint(self, 2, "Favourites list %sdeleted\n", (deleted ? "" : redtext("already ")));
}

void favx_add( float fav_num )
{
	gedict_t *goal = PROG_TO_EDICT(self->s.v.goalentity);
	int diff = (int)(goal - world);

	if ( fav_num < 1 || fav_num > MAX_CLIENTS )
		return;

	if ( !goal->k_player || diff < 1 || diff > MAX_CLIENTS ) {
		G_sprint(self, 2, "fav add: you are %s player!\n", redtext("not tracking"));
		return;
	}
	
	G_sprint(self, 2, "fav add: %s added to \x90slot %d\x91\n", goal->s.v.netname, (int)fav_num);

	self->favx[(int)fav_num - 1] = diff;
}

void fav_next( )
{
	int pl_num, fav_num, first_fav, desired_fav;
	gedict_t *goal = PROG_TO_EDICT(self->s.v.goalentity), *p;
	int diff = (int)(goal - world);

	for ( fav_num = 0; fav_num < MAX_CLIENTS; fav_num++ )
		if ( self->fav[fav_num] )
			break;

	if ( fav_num >= MAX_CLIENTS ) {
		G_sprint(self, 2, "fav_next: favourites list is %s\n", redtext("empty"));
		return;
	}

	desired_fav = -2;
	first_fav = fav_num; // remember

	if ( !( !goal->k_player || diff < 1 || diff > MAX_CLIENTS ) ) {
 		// ok - tracking player, so if goal in favourites switch to the next favourite,
 		// if goal not in favourites switch to the first favourite
		for ( fav_num = first_fav; fav_num < MAX_CLIENTS; fav_num++ )
			if ( desired_fav == -2 && self->fav[fav_num] == diff ) {
				desired_fav = -1; // found goal in favourites, search now next fav
			}
			else if ( desired_fav == -1 && self->fav[fav_num] ) {
				desired_fav = fav_num; // found next fav in favourites
				break;
			}
	}

	if ( desired_fav >= 0 )
		fav_num = desired_fav + 1;
	else
		fav_num = first_fav + 1;

	if ( fav_num < 1 || fav_num > MAX_CLIENTS ) {
		G_sprint(self, 2, "fav_next: internal error, fav_num %d\n", fav_num);
		return; // sanity - must not happen
	}

	pl_num = self->fav[fav_num - 1];

	if ( pl_num < 1 || pl_num > MAX_CLIENTS ) {
		G_sprint(self, 2, "fav_next: internal error, slot %d\n", pl_num);
		return;
	}

	p = world + pl_num;

	if ( !p->k_player ) {
		G_sprint(self, 2, "fav_next: can't find player\n");
		return;
	}

	if ( PROG_TO_EDICT( self->s.v.goalentity ) == p ) {
		G_sprint(self, 2, "fav_next: already observing...\n");
		return;
	}
	
	stuffcmd( self, "track %d\n", GetUserID( p ) );
}

void xfav_go( float fav_num )
{
	gedict_t *p;
	int pl_num;

	if ( fav_num < 1 || fav_num > MAX_CLIENTS )
		return;

	pl_num = self->favx[(int)fav_num - 1];

	if ( pl_num < 1 || pl_num > MAX_CLIENTS ) {
		G_sprint(self, 2, "fav go: \x90slot %d\x91 is not defined\n", (int)fav_num);
		return;
	}

	p = world + pl_num;

	if ( !p->k_player ) {
		G_sprint(self, 2, "fav go: \x90slot %d\x91 can't find player\n", (int)fav_num);
		return;
	}

	if ( PROG_TO_EDICT( self->s.v.goalentity ) == p ) {
		G_sprint(self, 2, "fav go: already observing...\n");
		return;
	}
	
	stuffcmd( self, "track %d\n", GetUserID( p ) );
}

void fav_show( )
{
	gedict_t *p;
	qboolean first, showed = false;
	int fav_num, diff;

	for ( first = true, fav_num = 0; fav_num < MAX_CLIENTS; fav_num++ )
		if ( (diff = self->favx[fav_num]) ) {
		    p = world + diff;
			if ( !p->k_player || strnull( p->s.v.netname ) )
				continue;

			if ( first ) {
				G_sprint(self, 2, "%s \x90%s\x91 %s:\n", redtext("Favourites"),
							redtext("slots based"), redtext("list"));
				first = false;
			}

			G_sprint(self, 2, " \x90slot %2d\x91 \x8D %s\n", fav_num + 1, p->s.v.netname);
			showed = true;
		}

	if ( showed )
		G_sprint(self, 2, "\n");

	for ( first = true, fav_num = 0; fav_num < MAX_CLIENTS; fav_num++ )
		if ( (diff = self->fav[fav_num]) ) {
		    p = world + diff;
			if ( !p->k_player || strnull( p->s.v.netname ) )
				continue;

			if ( first ) {
				G_sprint(self, 2, "%s:\n", redtext("Favourites list"));
				first = false;
			}

			G_sprint(self, 2, " %s\n", p->s.v.netname);
			showed = true;
		}

	if ( !showed )
		G_sprint(self, 2, "Favourites list %s or nothing to show\n", redtext("empty"));
}

void DoAutoTrack( )
{
	gedict_t *p = NULL, *goal;
	int id;

	if ( self->autotrack == atBest )
		p = get_ed_best1();
	else if ( self->autotrack == atPow )
		p = get_ed_bestPow();

	if ( !p )
		return;

	goal = PROG_TO_EDICT( self->s.v.goalentity );

	if ( goal->k_player && ISDEAD( goal ) && g_globalvars.time - goal->dead_time < 2 )
		return; // do not switch instantly autotrack pov after tracked player die, wait a few seconds or untill them respawn

	if ( goal == p )
		return; // already track this player

	if ( ( id = GetUserID( p ) ) > 0 )
		stuffcmd( self, "track %d\n", id );
}

void AutoTrack( float autoTrackType )
{
	autoTrackType_t at = self->autotrack; // save auto track type before turn off or switch to other type

	if ( autoTrackType == self->autotrack || autoTrackType == atNone )
		self->autotrack = atNone; // turn off
	else
		self->autotrack = at = autoTrackType; // switch auto track type

	G_sprint(self, 2, "%s %s\n", redtext(at == atBest ? "Autotrack" : 
			(at == atPow ? "Auto_pow" : "AutoUNKNOWN" )), OnOff(self->autotrack));
}

void next_best ()
{
	gedict_t *b1 = get_ed_best1(), *b2 = get_ed_best2();
	gedict_t *goal = PROG_TO_EDICT( self->s.v.goalentity ), *to;
	int id;

	if ( !b1 ) {
		G_sprint(self, 2, "%s: can't do this now\n", redtext("next_best"));
		return;
	}

	b2 = b2 ? b2 : b1;

	to = b1;
	if ( goal == b1 )
		to = b2;
	else if ( goal == b2 )
		to = b1;

	if ( ( id = GetUserID( to ) ) > 0 )
		stuffcmd( self, "track %d\n", id );
}

void next_pow ()
{
	gedict_t *goal = PROG_TO_EDICT( self->s.v.goalentity ), *to, *first, *p;
	qboolean nextBreak = false;
	int id;

	to = first = NULL;

	for ( p = world; (p = find( p , FOFCLSN, "player" )); ) {

		if ( ISDEAD(p) )
			continue;

		if ( !(    ( p->invincible_finished >= g_globalvars.time )
				|| ( p->super_damage_finished >= g_globalvars.time )
				|| ( p->invisible_finished >= g_globalvars.time )
				|| ( p->radsuit_finished >= g_globalvars.time )
			  )
		   )
			continue;

		if ( nextBreak ) {
			to = p;
			break;
		}

		if ( !first )
			first = p;

		if ( goal == p ) {
			nextBreak = true;
			continue;
		}

	}

	to = to ? to: first;

	if ( !to ) {
		G_sprint(self, 2, "%s: can't find poweruped player\n", redtext("next_pow"));
		return;
	}

	if ( ( id = GetUserID( to ) ) > 0 )
		stuffcmd( self, "track %d\n", id );
}

// }  spec tracking stuff 


//================================================
// pos_show/pos_save/pos_move/pos_set_* commands {
//================================================
// common functions
#define Pos_Disallowed()	(match_in_progress || k_pause || intermission_running)
// parse pos_show/pos_save/pos_move <number>
int Pos_Get_idx()
{
	char arg_2[1024];

	if ( trap_CmdArgc() == 2 ) {
		trap_CmdArgv( 1, arg_2, sizeof( arg_2 ) );
		return bound( 0, atoi(arg_2) - 1, MAX_POSITIONS - 1 );
	}

	return 0;
}
// Show functions
void Pos_Show ()
{
	int idx;
	pos_t *pos;

	if ( Pos_Disallowed() ) 
		return;

	pos = &(self->pos[idx = Pos_Get_idx()]);

	G_sprint(self, 2, "Position: %d\n", idx + 1);
	G_sprint(self, 2, "velocity: %9.2f %9.2f %9.2f\n", PASSVEC3(pos->velocity));
	G_sprint(self, 2, "  origin: %9.2f %9.2f %9.2f\n", PASSVEC3(pos->origin));
	G_sprint(self, 2, " v_angle: %9.2f %9.2f %9.2f\n", PASSVEC3(pos->v_angle));

	G_sprint(self, 2, "    Self:\n");
	G_sprint(self, 2, "velocity: %9.2f %9.2f %9.2f\n", PASSVEC3(self->s.v.velocity));
	G_sprint(self, 2, "  origin: %9.2f %9.2f %9.2f\n", PASSVEC3(self->s.v.origin));
	G_sprint(self, 2, " v_angle: %9.2f %9.2f %9.2f\n", PASSVEC3(self->s.v.v_angle));
}
// Save
#define Pos_Save_origin(pos)	VectorCopy(self->s.v.origin, (pos)->origin)
#define Pos_Save_angles(pos)	VectorCopy(self->s.v.v_angle, (pos)->v_angle)
#define Pos_Save_velocity(pos)	VectorCopy(self->s.v.velocity, (pos)->velocity)
// pos_save
void Pos_Save ()
{
	int idx;
	pos_t *pos;

	if ( Pos_Disallowed() ) 
		return;

	pos = &(self->pos[idx = Pos_Get_idx()]);

	Pos_Save_origin (pos);
	Pos_Save_angles (pos);
	Pos_Save_velocity (pos);

	G_sprint(self, 2, "Position %d was saved\n", idx + 1);
}

// Move & Set functions
extern vec3_t	VEC_HULL_MIN;
extern vec3_t	VEC_HULL_MAX;
qboolean Pos_Set_origin (pos_t *pos)
{
	gedict_t *p;

	if ( VectorCompare(pos->origin, VEC_ORIGIN) ) {
		G_sprint(self, 2, "Save your position first\n");
		return true;
	}

	if ( VectorCompare(pos->origin, self->s.v.origin) )
		return true;

    if (!self->k_spectator)
	{
		TraceCapsule( PASSVEC3( pos->origin ), PASSVEC3( pos->origin ), false, self,
					  PASSVEC3(VEC_HULL_MIN), PASSVEC3(VEC_HULL_MAX));

		p = PROG_TO_EDICT( g_globalvars.trace_ent );

		if (    g_globalvars.trace_startsolid
			 || ( p != self && p != world && (p->s.v.solid == SOLID_BSP || p->s.v.solid == SOLID_SLIDEBOX) )
	       ) {
			G_sprint(self, 2, "Can't move, location occupied\n");
			return true;
		}
	}

	// VVD: Don't want tele at pos_move - trick with tele is not good-looking. :-)
	//spawn_tfog(self->s.v.origin);
	//spawn_tfog(pos->origin);
	setorigin (self, PASSVEC3( pos->origin ) ); // u can't just copy, use setorigin

	return false;
}
#define Pos_Set_angles(pos)		{ \
									VectorCopy((pos)->v_angle, self->s.v.angles); \
									VectorCopy((pos)->v_angle, self->s.v.v_angle); \
									self->s.v.fixangle = true; \
								}
#define Pos_Set_velocity(pos)	VectorCopy((pos)->velocity, self->s.v.velocity)

// pos_move
void Pos_Move ()
{
	int idx;
	pos_t *pos;

	if ( Pos_Disallowed() ) 
		return;

	if ( self->pos_move_time && self->pos_move_time + 1 > g_globalvars.time ) {
		G_sprint(self, 2, "Only one move per second allowed\n");
		return;
	}
	self->pos_move_time = g_globalvars.time;

	pos = &(self->pos[idx = Pos_Get_idx()]);

	if (Pos_Set_origin (pos))
		return;
	Pos_Set_angles (pos);
	Pos_Set_velocity (pos);

	G_sprint(self, 2, "Position %d was restored\n", idx + 1);
}

// parse arguments for pos_set_*
void Pos_Parse_Set (vec3_t *x)
{
	char arg[1024];
	int i;

	for (i = 0; i < 3; ++i) {
		trap_CmdArgv( i + 1, arg, sizeof( arg ) );

		if ( strneq(arg, "*") )
			(*x)[i] = atof(arg);
	}
}

// pos_set_origin/pos_set_angles/pos_set_velocity
void Pos_Set (float set_type)
{
// VVD: For trick chiters! :-)
// Need to think out how to limit using Pos_Set for tricking.
// May be to ban pos_set_velocity?
	pos_t pos;

	if ( Pos_Disallowed() ) 
		return;

	if ( trap_CmdArgc() != 4 ) {
		G_sprint(self, 2, "Usage: pos_{origin|angles"/*|velocity*/"} x1 x2 x3\n"
						  "use '*' for no changes\n");
		return;
	}

	if ( self->pos_move_time && self->pos_move_time + 1 > g_globalvars.time ) {
		G_sprint(self, 2, "Only one move per second allowed\n");
		return;
	}
	self->pos_move_time = g_globalvars.time;

	switch ((int)set_type)
	{
		case 1:
			Pos_Save_origin(&pos);
			Pos_Parse_Set(&(pos.origin));
			Pos_Set_origin(&pos);
			break;
		case 2:
			Pos_Save_angles(&pos);
			Pos_Parse_Set(&(pos.v_angle));
			Pos_Set_angles(&pos);
			break;
		case 3:
			Pos_Save_velocity(&pos);
			Pos_Parse_Set(&(pos.velocity));
			Pos_Set_velocity(&pos);
			break;
		default:
			return;
	}

	G_sprint(self, 2, "Position was seted\n");
}
//================================================
// pos_show/pos_save/pos_move/pos_set_* commands }
//================================================

void Sh_Speed ()
{
	stuffcmd(self, "cmd info kf %d\n", (iKey( self, "kf" ) ^ KF_SPEED));
}

// /motd command

void PMOTDThink();
void SMOTDThink();

void motd_show ()
{
	gedict_t *motd;
	int owner = EDICT_TO_PROG( self );

	if ( !k_matchLess ) // show motd in matchLess mode even match in progress
	if ( match_in_progress )
		return;

	for( motd = world; (motd = find(motd, FOFCLSN, "motd")); )
		if ( owner == motd->s.v.owner ) {
			G_sprint(self, 2, "Already showing motd\n");
			return;
		}

	motd = spawn();
	motd->s.v.classname = "motd";
	motd->s.v.owner = EDICT_TO_PROG( self );
	// select MOTD for spectator or player
	motd->s.v.think = ( func_t ) ( self->k_spectator ? SMOTDThink : PMOTDThink );
	motd->s.v.nextthink = g_globalvars.time + 0.1;
	motd->attack_finished = g_globalvars.time + 10;
}

void krnd ()
{
	int argc, i;
	char arg_x[1024], buf[2048] = {0};

	if ( match_in_progress ) 
		return;

	if( check_master() )
		return;

	if ( ( argc = trap_CmdArgc() ) < 2 ) {
		G_sprint(self, 2, "usage: rnd <1st 2nd ...>\n");
		return;
	}
	
	for( buf[0] = 0, i = 1; i < argc; i++ ) {
		trap_CmdArgv( i, arg_x, sizeof( arg_x ) );

		strlcat(buf, arg_x, sizeof(buf));
		strlcat(buf, ( i + 1 < argc ? ", " : "" ), sizeof(buf));
	}

	G_bprint(2, "%s %s %s:\n"
				"\x90%s\x91\n", redtext("Random select by"), getname( self ), redtext("from"),
					buf );

	trap_CmdArgv( i_rnd(1, argc-1) , arg_x, sizeof( arg_x ) );

	G_bprint(2, "selected: \x90%s\x91\n", arg_x);
}

void agree_on_map ( )
{
	if ( !k_lastvotedmap )
		return; // no map voted

	self->cmd_selectMap = k_lastvotedmap; // emulate as we select last voted map
	SelectMap(); // <- there will be all checks about match_in_progress and etc
	self->cmd_selectMap = 0;
}

// { lastscores stuff

void lastscore_add ()
{
	gedict_t *p;
	int from;
	int i, s1 = 0, s2 = 0;
	int k_ls = bound(0, cvar("__k_ls"), MAX_LASTSCORES-1);
	char *e1, *e2, t1[128] = {0}, t2[128] = {0}, *name, *timestr;
	lsType_t lst = lsUnknown;

	e1 = e2 = "";
	
	if ( isRA() ) {
		lst = lsUnknown; // i'm dunno how lastscores must look in RA mode
	}
	else if   ( isDuel() ) {
		lst = lsDuel;

		for( i = from = 0, p = world; (p = find_plrghst( p, &from )) && i < 2; i++ ) {
			if ( !i ) { // info about first dueler
				e1 = getname( p );
				s1 = p->s.v.frags;
			}
			else {	   // about second
				e2 = getname( p );
				s2 = p->s.v.frags;
			}
		}
	}
	else if ( ( isTeam() || isCTF() ) && k_showscores ) {
		lst = isTeam() ? lsTeam : lsCTF;

		e1 = cvar_string( "_k_team1" );
		s1 = get_scores1();
		e2 = cvar_string( "_k_team2" );
		s2 = get_scores2();

		// players from first team
		for( t1[0] = from = 0, p = world; (p = find_plrghst( p, &from )); )
			if ( streq( getteam( p ), e1 ) && !strnull( name = getname( p ) ) )
				strlcat(t1, va(" %s", name), sizeof(t1));

		// players from second team
		for( t2[0] = from = 0, p = world; (p = find_plrghst( p, &from )); )
			if ( streq( getteam( p ), e2 ) && !strnull( name = getname( p ) ) )
				strlcat(t2, va(" %s", name), sizeof(t2));
	}

	if ( strnull( e1 ) || strnull( e2 ) )
		lst = lsUnknown;

	if ( lst == lsUnknown ) // sorry but something wrong
		return;

	if ( !strnull(timestr = ezinfokey(world, "date_str")) )
		timestr += 4; // qqshka - hud 320x240 fix, :(

	cvar_fset(va("__k_ls_m_%d", k_ls), lst);
	cvar_set(va("__k_ls_e1_%d", k_ls), e1);
	cvar_set(va("__k_ls_e2_%d", k_ls), e2);
	cvar_set(va("__k_ls_t1_%d", k_ls), t1);
	cvar_set(va("__k_ls_t2_%d", k_ls), t2);
	cvar_set(va("__k_ls_s_%d", k_ls), va("%3d:%-3d \x8D %-8.8s %13.13s", s1, s2, 
										g_globalvars.mapname, timestr));

	cvar_fset("__k_ls", ++k_ls % MAX_LASTSCORES);
}

void lastscores ()
{
	int i, j, cnt;
	int k_ls = bound(0, cvar("__k_ls"), MAX_LASTSCORES-1);
	char *e1, *e2, *le1, *le2, *t1, *t2, *lt1, *lt2, *sc;
	lsType_t last = lsUnknown;
	lsType_t cur  = lsUnknown;

	e1 = e2 = le1 = le2 = t1 = t2 = lt1 = lt2 = "";

	for ( j = k_ls, cnt = i = 0; i < MAX_LASTSCORES; i++,	j = (j+1 >= MAX_LASTSCORES) ? 0: j+1 ) {

		cur = cvar(va("__k_ls_m_%d", j));
		e1 = cvar_string(va("__k_ls_e1_%d", j));
		e2 = cvar_string(va("__k_ls_e2_%d", j));
		t1 = cvar_string(va("__k_ls_t1_%d", j));
		t2 = cvar_string(va("__k_ls_t2_%d", j));
		sc = cvar_string(va("__k_ls_s_%d", j));

		if ( cur == lsUnknown || strnull( e1 ) || strnull( e2 ) )
			continue;

		if (    cur != last // changed game mode
			 || (strneq(le1 , e1) || strneq(le2 , e2)) // changed teams, duelers
		   ) {
			lt1 = lt2 = ""; // show teams members again
			G_sprint(self, 2, "\x90%s %s %s\x91%s\n", e1, redtext("vs"), e2, 
									(cur == lsCTF ? redtext(" CTF") : ""));
		}

		// if team mode show members.
		// generally show members one time while show scores for each played map,
		// but if squad changed from previuos map, show members again,
		// so we know which squad played each map.
		if ( cur == lsTeam || cur == lsCTF ) {
			if ( strneq(lt1 , t1) ) // first team
				G_sprint(self, 2, " %4.4s:%s\n", e1, t1);

			if ( strneq(lt2 , t2) ) // second team
				G_sprint(self, 2, " %4.4s:%s\n", e2, t2);
		}

		G_sprint(self, 2, "  %s\n", sc);

		last = cur;
		le1 = e1;
		le2 = e2;
		lt1 = t1;
		lt2 = t2;
		cnt++;
	}

	if ( cnt )
		G_sprint(self, 2, "\n"
						  "Lastscores: %d entry%s found\n", cnt, count_s(cnt));
	else
		G_sprint(self, 2, "Lastscores data empty\n");
}

// } lastscores stuff

// { spec moreinfo

qboolean mi_on ()
{
	return ( (int)cvar("k_spec_info") & MI_ON );
}

qboolean mi_adm_only ()
{
	return ( (int)cvar("k_spec_info") & MI_ADM_ONLY );
}

#define MI_POW (IT_INVISIBILITY | IT_INVULNERABILITY | IT_SUIT | IT_QUAD)
#define MI_ARM (IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3)
#define MI_WPN (IT_SHOTGUN | IT_SUPER_SHOTGUN | IT_NAILGUN | IT_SUPER_NAILGUN \
		        | IT_GRENADE_LAUNCHER | IT_ROCKET_LAUNCHER | IT_LIGHTNING )
#define MI_WPN3 (IT_GRENADE_LAUNCHER | IT_ROCKET_LAUNCHER | IT_LIGHTNING)

typedef struct mi_levels_s {
	int			items;
	const char  *desc;
} mi_levels_t;

mi_levels_t mi_levels[] = {
	{ 0, "Receiving extra infos: \317\346\346" },
	{ MI_POW | MI_ARM | IT_SUPERHEALTH | IT_ROCKET_LAUNCHER, "Receiving powerups\217armors\217mh\217rl" },
	{ MI_POW | MI_ARM | IT_SUPERHEALTH | MI_WPN3, "Receiving powerups\217armors\217mh\217rl\217gl\217lg" },
	{ MI_POW | MI_ARM | IT_SUPERHEALTH | MI_WPN, "Receiving powerups\217armors\217mh\217weapons"},
	{ MI_POW, "Receiving only powerups" }
};

int mi_levels_cnt = sizeof( mi_levels ) / sizeof( mi_levels[0] );

void mi_print( gedict_t *tooker, int it, char *msg )
{
	char *t_team;
	gedict_t *p;
	int from, level;
	qboolean adm = mi_adm_only ();

	if ( !mi_on() )
		return; // spec info is turned off

	t_team = getteam( tooker );

	for( from = 1 /* spec */, p = world; (p = find_plrspc (p, &from)); ) {
		if ( adm && !is_adm( p ) )
			continue; // configured send only for admins

		level = iKey(p, "mi"); // get spec setup

		if (level < 0)
			level = 0;
		if (level > mi_levels_cnt-1 )
			level = 0;

		if ( !(it & mi_levels[level].items) )
			continue;

		if ( isTeam() || isCTF() )
			G_sprint(p, 2, "\x84\x90%4.4s\x91 %s\n", t_team, msg);
		else
			G_sprint(p, 2, "%s\n", msg);
	}
}

void moreinfo()
{
	int level;

	if ( !mi_on() ) {
		G_sprint(self, 2, "Spec info is turned off by server\n");
		return;
	}

	level = iKey(self, "mi") + 1;

	if ( level > mi_levels_cnt - 1 )
		level = 0;
	if ( level < 0 )
		level = 0;

	cmdinfo_setkey( self, "mi", va("%d", level) );
}

void info_mi_update( gedict_t *p, char *from, char *to )
{
	int level  = atoi(to);
	int olevel = atoi(from);

	if ( !mi_on() )
		return;

	if ( level == olevel )
		return;

	if ( level > mi_levels_cnt - 1 )
		level = 0;
	if ( level < 0 )
		level = 0;

	G_sprint(p, 2, "%s\n", mi_levels[level].desc);
}

void infolock ( )
{
	int k_spec_info = cvar("k_spec_info");

	if ( match_in_progress ) 
		return;

	if( check_master() )
		return;

	if ( !is_adm( self ) ) {
		G_sprint(self, 2, "You are not an admin\n");
		return;
	}

	k_spec_info ^= MI_ADM_ONLY;
	cvar_fset("k_spec_info", k_spec_info);

	if ( mi_adm_only() ) 
		G_bprint(2, "Only %s can receive specinfos\n", redtext("admins"));
	else 
		G_bprint(2, "All %s can receive specinfos\n", redtext("spectators"));
}

void infospec ( )
{
	int k_spec_info = cvar("k_spec_info");

	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	k_spec_info ^= MI_ON;
	cvar_fset("k_spec_info", k_spec_info);

	G_bprint(2, "Extra info for spectators %s\n", redtext(OnOff(mi_on())));
}

// }

// { wreg

void wreg_usage()
{
	G_sprint(self, 2, "usage: cmd wreg [[char] [[+/-]weapon order]]\n");
}

void wreg_showslot(	wreg_t *w, int slot )
{
	int i;
	char *sign, order[MAX_WREG_IMP+1];

	if ( !w->init ) {
		G_sprint(self, 2, "slot \"%c\" - unregistered\n", (char)slot);
		return;
	}

	sign = "";
	if (w->attack > 0 )
		sign = "+";
	else if (w->attack < 0 )
		sign = "-";

	for ( order[0] = i = 0; i < MAX_WREG_IMP && w->impulse[i]; i++ )
		order[i] = '0' + w->impulse[i];
	order[i] = 0;

	G_sprint(self, 2, "slot \"%c\" - \"%s%s\"\n", (char)slot, sign, order);
}

void cmd_wreg()
{
	int		argc = trap_CmdArgc(), attack = 0, imp[MAX_WREG_IMP], i, cnt;
	char	arg_1[64], arg_2[64], *tmp = arg_2;
	byte    c;
	wreg_t  *w;

	if ( !self->wreg )
		return;

	if ( argc == 1 ) {
		qboolean found = false;

		G_sprint(self, 2, "list of registered weapons:\n");

		for ( i = 0; i < MAX_WREGS; i++ ) {
			w = &(self->wreg[i]);
			if ( !w->init )
				continue;

			found = true;
			wreg_showslot( w, i );
		}

		if ( !found )
			G_sprint(self, 2, "none\n");

		return;
	}

	trap_CmdArgv( 1, arg_1, sizeof( arg_1 ) );

	if ( strnull(arg_1) ) {
		wreg_usage();
		G_sprint(self, 2, "empty char\n");
		return;
	}

	if ( strlen(arg_1) > 1 ) {
		wreg_usage();
		G_sprint(self, 2, "char can be only one byte\n");
		return;
	}

	c = arg_1[0];

	if ( c <= 0 || c > 175 || c >= MAX_WREGS ) {
		wreg_usage();
		G_sprint(self, 2, "\"%c\" - illegal char!\n", (char)c);
		return;
	}

	w = &(self->wreg[c]);

	if ( argc == 2 ) {
		wreg_showslot( w, c );
		return;
	}

	if ( argc != 3 ) {
		wreg_usage();
		return; // something wrong
	}

	trap_CmdArgv( 2, arg_2, sizeof( arg_2 ) );

	if ( strnull(arg_2) ) {
		if ( w->init ) {
			memset( w, 0, sizeof( wreg_t ) ); // clear
			w->init = false;
			G_sprint(self, 2, "slot \"%c\" - unregistered\n", (char)c);
		}
		else {
			wreg_usage();
			G_sprint(self, 2, "empty weapon order\n");
		}
		return;
	}

	for ( cnt = i = 0; i < MAX_WREGS; i++ ) {
		if ( !(self->wreg[i].init) )
			continue;

		if ( ++cnt >= 20 ) {
			G_sprint(self, 2, "too many wregs, discard registration\n");
			return;
		}
	}

	if ( strlen(arg_2) > 10 ) { // 10 == strlen("+987654321")
		wreg_usage();
		G_sprint(self, 2, "too long weapon order\n");
		return;
	}

	if ( tmp[0] == '+' ) {
		tmp++;
		attack = 1;
	}
	else if ( tmp[0] == '-' ) {
		tmp++;
		attack = -1;
	}

	if ( !strnull( tmp ) && !only_digits( tmp ) ) {
		wreg_usage();
		G_sprint(self, 2, "illegal character in weapon order\n");
		return;
	}

	for ( i = 0; i < MAX_WREG_IMP && !strnull(tmp); tmp++ ) {
		if ( tmp[0] == '0' ) // do not confuse with '\0'
			continue;

		imp[i] = tmp[0] - '0';
		i++;
	}

	// ok we parse wreg command, and all ok, init it
	memset( w, 0, sizeof( wreg_t ) ); // clear

	w->init   = true;
	w->attack = attack;

	for ( i--; i >= 0 && i < MAX_WREG_IMP; i-- )
		w->impulse[i] = imp[i];

	G_sprint(self, 2, "slot \"%c\" - registered\n", (char)c);
}

void cmd_wreg_do( byte c )
{
	qboolean warn;
	int j;
	wreg_t *w;

	if ( !self->wreg || c >= MAX_WREGS )
		return;

	w = &(self->wreg[c]);

	if ( !w->init ) {
		G_sprint(self, 2, "unregistered wreg char - \"%c\"\n", (char)c);
		return;
	}

//	G_sprint(self, 2, "wreg char - %c, i - %d %d %d\n", (char)c, w->impulse[0], w->impulse[1], w->impulse[2]);

	if ( w->attack > 0 ) {
		self->wreg_attack = 1;

		if( self->k_spectator )
			stuffcmd(self, "+attack\n");
	}
	else if ( w->attack < 0 ) {
		self->wreg_attack = 0;

		if( self->k_spectator )
			stuffcmd(self, "-attack\n");
	}

	if( self->k_spectator )
		return;

	for ( j = 0; j < MAX_WREG_IMP && w->impulse[j]; j++ ) {

		if ( j + 1 >= MAX_WREG_IMP || !w->impulse[j + 1] )
			warn = true; // warn about no weapon or ammo if this last impulse in array
		else
			warn = false;

		if ( W_CanSwitch( w->impulse[j], warn ) ) {
			self->s.v.impulse = w->impulse[j];
			return;
		}
	}
}

// }

void ToggleMidair()
{
	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	// Can't enable midair unless dmm4 is set first
	if ( !cvar("k_midair") && deathmatch != 4 ) {
		G_sprint( self, 2, "Midair requires dmm4\n");
		return;
	}

	cvar_toggle_msg( self, "k_midair", redtext("Midair") );
}

void sv_time()
{
	char *tm;

	if ( !strnull( tm = ezinfokey(world, "date_str") ) )
		G_sprint(self, 2, "%s\n", tm);
}

void GrenadeMode()
{
	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	// Can't toggle unless dmm4 is set first
	if ( deathmatch != 4 ) {
		G_sprint( self, 2, "gren_mode requires dmm4\n");
		return;
	}

	cvar_toggle_msg( self, "k_dmm4_gren_mode", redtext("grenade mode") );
}

void ToggleReady()
{
	if ( self->ready )
		PlayerBreak();
	else 
		PlayerReady();
}

void dlist()
{
	stuffcmd(self, "cmd demolist %s\n", params_str(1, -1));
}

void dinfo()
{
	stuffcmd(self, "cmd demoinfo %s\n", params_str(1, -1));
}
