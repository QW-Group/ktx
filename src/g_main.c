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
 *  $Id$
 */

#include "g_local.h"
#include "rng.h"
/* global 4 fix
 entity          self;
 entity          other;
 entity          world;
 entity          newmis;                         // if this is set, the entity that just
 // run created a new missile that should
 // be simulated immediately
 entity          trace_ent;
 entity          msg_entity;                             // destination of single entity writes

 */
void initialise_spawned_ent(gedict_t *ent);

gedict_t g_edicts[MAX_EDICTS];	//768
gedict_t *world = g_edicts;
gedict_t *self, *other;

//short shortvar=0xfedc;
globalvars_t g_globalvars;
field_t expfields[] =
{
	{ "vw_index", 		FOFS(vw_index), 	F_FLOAT },
	{ "movement", 		FOFS(movement), 	F_VECTOR },
	{ "maxspeed", 		FOFS(maxspeed), 	F_FLOAT },
	{ "gravity", 		FOFS(gravity), 		F_FLOAT },
	{ "isBot", 			FOFS(isBot), 		F_INT },
	{ "brokenankle", 	FOFS(brokenankle), 	F_FLOAT },
	{ "items2", 		FOFS(items2), 		F_FLOAT },
	{ "hideentity", 	FOFS(hideentity), 	F_INT },
	{ "trackent", 		FOFS(trackent), 	F_INT },
	{ "hideplayers", 	FOFS(hideplayers), 	F_INT },
// FTE does not support this.
// We does not really have to explicitly disable them since server engine should ignore unsupported fields.
// But Spike insists on this. Probably for clarity.
#ifndef FTESV
	{ "mod_admin", 		FOFS(k_admin), 		F_INT },
	{ "teleported", 	FOFS(teleported), 	F_INT },
#endif
	{ NULL }
};

char mapname[64];
static char worldmodel[64] = "worldmodel"; // FIXME: consider replacing usage of this with mapname.

// Client names assigned in:
// * G_InitGame()
// * GAME_CLIENT_CONNECT
// * ClientUserInfoChanged()
static char netnames[MAX_CLIENTS][CLIENT_NAME_LEN];

static char callalias_buf[MAX_CLIENTS][CALLALIAS_SIZE];
static char f_checks[MAX_CLIENTS][F_CHECK_SIZE];

static wreg_t wregs[MAX_CLIENTS][MAX_WREGS];
static plrfrm_t plrfrms[MAX_CLIENTS][MAX_PLRFRMS];

gameData_t gamedata =
	{ (edict_t*) g_edicts, sizeof(gedict_t), &g_globalvars, expfields,
	GAME_API_VERSION,
	MAX_EDICTS };

float starttime;
int g_matchstarttime;
int sv_extensions;
void G_InitGame(int levelTime, int randomSeed);
void G_ShutDown(void);
void StartFrame(int time);
qbool ClientCommand(void);
qbool ClientUserInfoChanged(int after);
void G_EdictTouch(void);
void G_EdictThink(void);
void G_EdictBlocked(void);
void ClearGlobals(void);
void PausedTic(int duration);

qbool ClientSay(qbool isTeamSay);

void RemoveMOTD(void);
void ShowVersion(void);

static qbool check_ezquake(gedict_t *p);

void SaveLevelStartParams(gedict_t *e);

qbool FTE_sv = false;

static qbool G_InitExtensions(void);

/*
 ================
 vmMain

 This is the only way control passes into the module.
 This must be the very first function compiled into the .q3vm file
 ================
 */
intptr_t VISIBILITY_VISIBLE vmMain(
				int command,
				int arg0, int arg1, int arg2, int arg3,	int arg4,
				int arg5, int arg6,	int arg7, int arg8, int arg9, int arg10, int arg11)
{
	int api_ver;

	switch (command)
	{
		case GAME_INIT:
			ClearGlobals();
			api_ver = trap_GetApiVersion();
#if defined(idx64) || defined(PR_ALWAYS_REFS)
			// We set references
			cvar_fset("sv_pr2references", 1);
#endif
			if (api_ver < GAME_API_VERSION)
			{
				G_cprint("Mod requires API_VERSION %d or higher, server have %d\n",
							GAME_API_VERSION, api_ver);

				return 0;
			}

			if (strstr(ezinfokey(world, "*version"), "FTE"))
			{
				G_cprint("^2KTX: FTE server detected, yay!\n");
				FTE_sv = true;
			}

			if (!G_InitExtensions())
				return 0;

			G_InitGame(arg0, arg1);

			return (intptr_t)(&gamedata);

		case GAME_LOADENTS:
			ClearGlobals();
			G_SpawnEntitiesFromString();

			return 1;

		case GAME_START_FRAME:
			ClearGlobals();
			if (arg1)
			{
#ifdef BOT_SUPPORT
				extern void BotStartFrame(void);

				if (bots_enabled())
				{
					BotStartFrame();
				}
#endif
			}
			else
			{
				if (framecount == 0)
				{
					infokey(world, "mapname", mapname, sizeof(mapname));
					infokey(world, "modelname", worldmodel, sizeof(worldmodel));
					world->model = worldmodel;
				}

				StartFrame(arg0);
			}

			return 1;

		case GAME_CLIENT_CONNECT:
			ClearGlobals();
			self = PROG_TO_EDICT(g_globalvars.self);

			// Init client name.
			self->netname = netnames[NUM_FOR_EDICT(self)-1];
			infokey(self, "name", self->netname, CLIENT_NAME_LEN);

			self->last_rune = "setme";
			self->classname = ""; // at least empty classname
			self->connect_time = g_globalvars.time;
			self->k_lastspawn = world; // set safe value
			self->k_msgcount = g_globalvars.time;

			self->wreg = wregs[(int)(self - world) - 1];
			self->plrfrms = plrfrms[(int)(self - world) - 1];

			memset(self->wreg, 0, sizeof(wreg_t) * MAX_WREGS);     // clear
			memset(self->plrfrms, 0, sizeof(plrfrm_t) * MAX_PLRFRMS); // clear

			self->callalias = callalias_buf[(int)(self - world) - 1];
			self->callalias[0] = 0;
			self->f_checkbuf = f_checks[(int)(self - world) - 1];
			self->f_checkbuf[0] = 0;

			check_ezquake(self);

			if (strnull(ezinfokey(self, "*is"))) // show this only ones at connect time
			{
				ShowVersion();
			}

			if (arg0)
			{
				SpectatorConnect();
			}
			else
			{
				ClientConnect();
			}

			if (self->k_accepted)
			{
				cmdinfo_infoset(self);
			}

			update_ghosts();
			self->hideplayers_default = (self->ezquake_version < 4957);

			return 1;

		case GAME_PUT_CLIENT_IN_SERVER:
			ClearGlobals();
			self = PROG_TO_EDICT(g_globalvars.self);
			if (!self->k_accepted)
			{
				return 1;
			}

			SaveLevelStartParams(self);
			if (!arg0)
			{
				k_respawn(self, false);
			}
			else
			{
				PutSpectatorInServer();
			}

			return 1;

		case GAME_CLIENT_DISCONNECT:
			ClearGlobals();
			self = PROG_TO_EDICT(g_globalvars.self);
			RemoveMOTD(); // remove MOTD entitys
			s_lr_clear(self);
			if (arg0)
			{
				SpectatorDisconnect();
			}
			else
			{
				ClientDisconnect();
			}

// { guarantee this set to some safe values after client disconnect
			self->classname = "";
			self->ct = ctNone;
			self->k_accepted = 0;
// }
			update_ghosts();

			return 1;

		case GAME_SETNEWPARMS:
			ClearGlobals();
			SetNewParms();

			return 1;

		case GAME_CLIENT_PRETHINK:
			ClearGlobals();
			self = PROG_TO_EDICT(g_globalvars.self);
			if (!self->k_accepted)
			{
				return 1;
			}

			if (self->wreg_attack) // client simulate +attack via "cmd wreg" feature
			{
				self->s.v.button0 = true;
			}

			if (k_bloodfest)
			{
				extern void bloodfest_client_think(void);
				bloodfest_client_think();
			}

			if (!arg0)
			{
				PlayerPreThink();
			}

			return 1;

		case GAME_CLIENT_POSTTHINK:
			ClearGlobals();
			self = PROG_TO_EDICT(g_globalvars.self);
			if (!self->k_accepted)
			{
				return 1;
			}

			self->k_lastPostThink = g_globalvars.time;
			if (self->wreg_attack) // client simulate +attack via "cmd wreg" feature
			{
				self->s.v.button0 = true;
			}

			if (!arg0)
			{
				PlayerPostThink();
			}
			else
			{
				SpectatorThink();
			}

			BothPostThink();

			return 1;

		case GAME_EDICT_TOUCH:
			ClearGlobals();
			G_EdictTouch();

			return 1;

		case GAME_EDICT_THINK:
			ClearGlobals();
			G_EdictThink();

			return 1;

		case GAME_EDICT_BLOCKED:
			ClearGlobals();
			G_EdictBlocked();

			return 1;

		case GAME_SETCHANGEPARMS:
			ClearGlobals();
			self = PROG_TO_EDICT(g_globalvars.self);
			SetChangeParms();

			return 1;

		case GAME_CLIENT_COMMAND:
			ClearGlobals();

			return ClientCommand();

		case GAME_CLIENT_USERINFO_CHANGED:
			// Called when client userinfo is changed.
			// Return non zero zero if such userinfo invalid.
			// argv(0) = "setinfo", argv(1) = key, argv(2) = value.
			// Userinfo change performed in two stages,
			// first stage is when server and mod validating if new userinfo is allowed,
			// during this stage argument 'after' is zero, after userinfo allowed
			// server execute this function second time with non zero 'after' argument.
			ClearGlobals();
			self = PROG_TO_EDICT(g_globalvars.self);
			if (!self->k_accepted)
			{
				return 0; // cmon, u r zombie or etc...
			}

			// allow change even not connected, or disconnected
			return ClientUserInfoChanged(arg0);

		case GAME_SHUTDOWN:
			// called before level change/spawn
			// qqshka: YES, REALLY COOL QVM FEATURE COMPARING TO QC, U CAN CATCH LEVEL CHANGE
			//         INVOKED FROM SERVER CONSOLE !!!
			ClearGlobals();
			G_ShutDown();

			return 0;

		case GAME_CONSOLE_COMMAND:
			// called on server console command "mod"
			// params like GAME_CLIENT_COMMAND, but argv(0) always "mod"
			ClearGlobals();

			return 0;		//ConsoleCommand();

		case GAME_CLIENT_SAY:
			// called on user /say or /say_team
			// arg0 non zero if say_team
			// return non zero if say/say_team handled by mod
			// params like GAME_CLIENT_COMMAND
			ClearGlobals();

			return ClientSay(arg0);

		case GAME_PAUSED_TIC:
			// called every frame when the game is paused
			ClearGlobals();
			PausedTic(arg0);

			return 0;

		case GAME_CLEAR_EDICT:
			// Don't ClearGlobals() as this will be called during spawn()
			initialise_spawned_ent(PROG_TO_EDICT(g_globalvars.self));

			return 0;
	}

	return 0;
}

//===================================================================
void G_Printf(const char *fmt, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	trap_DPrintf(text);
}

void G_Error(const char *fmt, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	trap_Error(text);
}

//===================================================================

void G_InitGame(int levelTime, int randomSeed)
{
	int 		i;

	g_random_seed(randomSeed);
	framecount = 0;
	starttime = levelTime * 0.001;
	G_Printf("Init Game\n");
	G_InitMemory();
	memset(g_edicts, 0, sizeof(gedict_t) * MAX_EDICTS);
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		g_edicts[i + 1].netname = netnames[i];
	}

	GetMapList();

	ra_init_que();

	Init_cmds();

	race_init();

	// put mod version in serverinfo
	localcmd("serverinfo \"%s\" \"%s\"\n", MOD_SERVERINFO_MOD_KEY, MOD_VERSION);

	// set mod information cvars
	cvar_set("qwm_name", MOD_NAME);
	cvar_set("qwm_fullname", MOD_FULLNAME);
	cvar_set("qwm_version", MOD_VERSION);
	cvar_set("qwm_buildnum", (GIT_COMMIT ? GIT_COMMIT : "unknown"));
	cvar_set("qwm_platform", QW_PLATFORM_SHORT);
	cvar_set("qwm_builddate", MOD_BUILD_DATE);
	cvar_set("qwm_homepage", MOD_URL);

	sv_extensions = cvar("sv_mod_extensions");
}

void G_ShutDown(void)
{
	extern int IsMapInCycle(char *map);
	extern qbool force_map_reset;

	char *map = mapname;

	AbortElect();

	if (!cvar("lock_practice") && k_practice)  // #practice mode#
	{
		SetPractice(0, NULL); // return server to normal mode
	}

	if (match_in_progress)
	{
		EndMatch(1); // skip demo, make some other stuff
	}

	cvar_set("_k_lastmap", (strnull(map) || force_map_reset ? "" : map));
	cvar_fset("_k_pow_last", Get_Powerups());
}

//===========================================================================
// Physics
//
//===========================================================================
////////////////
// GlobalParams:
// time
// self
// other
///////////////
void G_EdictTouch(void)
{
	self = PROG_TO_EDICT(g_globalvars.self);
	other = PROG_TO_EDICT(g_globalvars.other);
	if (self->touch)
	{
		/*
		 #ifdef DEBUG
		 if(self->classname && other->classname)
		 if(!strcmp(self->classname,"player")||!strcmp(other->classname,"player"))
		 G_bprint(2, "touch %s <-> %s\n", self->classname,other->classname);
		 #endif
		 */
		((void(*)(void))(self->touch))();
	}
	else
	{
		G_Printf("Null touch func");
	}
}

////////////////
// GlobalParams:
// time
// self
// other=world
///////////////
void G_EdictThink(void)
{
	self = PROG_TO_EDICT(g_globalvars.self);
	other = PROG_TO_EDICT(g_globalvars.other);
	if (self->think)
	{
		((void(*)(void))(self->think))();
	}
	else
	{
		G_Printf("Null think func");
	}

}

////////////////
// GlobalParams:
// time
// self=pusher
// other=check
// if the pusher has a "blocked" function, call it
// otherwise, just stay in place until the obstacle is gone
///////////////
void G_EdictBlocked(void)
{
	self = PROG_TO_EDICT(g_globalvars.self);
	other = PROG_TO_EDICT(g_globalvars.other);

	if (self->blocked)
	{
		((void(*)(void))(self->blocked))();
	}
	else
	{
		//G_Printf("Null blocked func");
	}

}

void ClearGlobals(void)
{
	damage_attacker = damage_inflictor = activator = self = other = newmis = world;
}

//===========================================================================

// yeah its lame, but better than checking setinfo each time.
static qbool check_ezquake(gedict_t *p)
{
	char *clinfo = ezinfokey(p, "*client");

	if (!strnull(clinfo) && strstr(clinfo, "ezQuake")) // seems ezQuake
	{
		while (Q_isalpha(clinfo[0]))
		{
			clinfo++;
		}

		p->ezquake_version = atoi(clinfo);

		return true;
	}

	return false;
}

//===========================================================================

qbool haveextensiontab[G_EXTENSIONS_LAST-G_EXTENSIONS_FIRST];

static qbool G_InitExtensions(void)
{
	qbool success = true;
	struct
	{
		const char *name;
		int id;
	} exttraps[] =
	{
		{"SetExtField",			G_SETEXTFIELD},
		{"GetExtField",			G_GETEXTFIELD},
		{"ChangeLevelHub",		G_CHANGELEVEL_HUB},
		{"URI_Query",			G_URI_QUERY},
		{"particleeffectnum",	G_PARTICLEEFFECTNUM},
		{"trailparticles",		G_TRAILPARTICLES},
		{"pointparticles",		G_POINTPARTICLES},
		{"clientstat",			G_CLIENTSTAT},
		{"pointerstat",			G_POINTERSTAT},
		{"MapExtFieldPtr",		G_MAPEXTFIELDPTR},
		{"SetExtFieldPtr",		G_SETEXTFIELDPTR},
		{"GetExtFieldPtr",		G_GETEXTFIELDPTR},
	};
	int i;
	for (i = 0; i < sizeof(exttraps)/sizeof(exttraps[0]); i++)
	{
		haveextensiontab[exttraps[i].id-G_EXTENSIONS_FIRST] = trap_Map_Extension(exttraps[i].name, exttraps[i].id)>=0;
	}

	//
	// Here you should check extensions which required and terminate server if something is missed.
	//

	return success;
}
