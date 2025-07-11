//
// Clan Arena related
//

#include "g_local.h"

typedef struct wipeout_spawn_config_t
{
	vec3_t origin;          // spawn point coordinates
	char *name;             // spawn point name (for debugging)
	float custom_radius;    // custom radius for this spawn (0 = use default)
} wipeout_spawn_config;

typedef struct wipeout_map_spawns_t
{
	char *mapname;
	wipeout_spawn_config *spawns;
	int spawn_count;
} wipeout_map_spawns;

// Some spawns require a custom radius to prevent abuse
// Using 0 defaults to a radius of 84 units
static wipeout_spawn_config dm3_spawns[] = {
	{ { -880, -232, -16 },	"tele/sng",	128 },
	{ { 192, -208, -176 },	"big>ra",	0   },
	{ { 1472, -928, -24 },	"ya box",	0   },
	{ { 1520, 432, -88 },	"rl",		190 },
	{ { -632, -680, -16 },	"tele/ra",	128 },
	{ { 512, 768, 216 }, 	"lifts",	128 }
};

static wipeout_map_spawns wipeout_spawn_configs[] = {
	{ "dm3", dm3_spawns, sizeof(dm3_spawns) / sizeof(dm3_spawns[0]) },
	{ NULL, NULL, 0 }  // terminator
};

static int round_num;
static int team1_score;
static int team2_score;
static int pause_count;
static int pause_time;
static int round_time;
static int loser_team;
static qbool do_endround_stuff = false;
static qbool print_stats = false;
static float loser_respawn_time = 999; 	// number of seconds before a teammate would've respawned

void track_player(gedict_t *observer);
void enable_player_tracking(gedict_t *e, int follow);
void r_changetrackingstatus(float t);
void CA_PrintScores(void);
void CA_TeamsStats(void);
void CA_SendTeamInfo(gedict_t *t);
void print_player_stats(qbool series_over);
void CA_OnePlayerStats(gedict_t *p, qbool series_over);
void CA_AddLatePlayer(gedict_t *p, char *team);
void EndRound(int alive_team);
void show_tracking_info(gedict_t *p);

// Wipeout spawn management functions
static wipeout_spawn_config* WO_FindSpawnConfig(vec3_t origin);
float WO_GetSpawnRadius(vec3_t origin);
void WO_InitializeSpawns(void);

gedict_t* ca_find_player(gedict_t *p, gedict_t *observer)
{
	char *team = getteam(observer);
	
	// if the observer is just spectating, or if the round is over
	// then any player can be watched.
	// otherwise, you can only spec teammates
	if (!observer->ca_ready || ca_round_pause)
	{
		p = find_plr(p);
		while (p && !p->in_play)
		{
			p = find_plr(p);
		}
	}
	else
	{
		p = find_plr_same_team(p, team);
		while (p && !p->in_play)
		{
			p = find_plr_same_team(p, team);
		}
	}

	return p;
}

gedict_t* ca_get_player(gedict_t *observer)
{
	return ca_find_player(world, observer);
}

qbool is_rules_change_allowed(void);

int CA_count_ready_players(void)
{
	int cnt;
	gedict_t *p;

	for (cnt = 0, p = world; (p = find_plr(p));)
	{
		if (p->ca_ready)
		{
			cnt++;
		}
	}

	return cnt;
}

int CA_get_score_1(void)
{
	return team1_score;
}

int CA_get_score_2(void)
{
	return team2_score;
}

int calc_respawn_time(gedict_t *p, int offset)
{
	qbool isWipeout = (cvar("k_clan_arena") == 2);
	int max_deaths = cvar("k_clan_arena_max_respawns");
	int time = 999;
	int teamsize = 0;
	int multiple;
	gedict_t *p2;

	// count players on team
	for (p2 = world; (p2 = find_plr_same_team(p2, getteam(p)));)
	{
		teamsize++;
	}

	p->is_solo = teamsize == 1 ? 1 : 0;

	multiple = bound(3, teamsize+1, 6);	// first respawn won't take more than 6 seconds regardless of team size

	if (isWipeout && (p->round_deaths+offset <= max_deaths))
	{
		// if 4 players on team, the spawn times are 5, 10, 20, 30
		// if 3 players on team, the spawn times are 4, 8, 16, 24
		// if 2 players on team, the spawn times are 3, 6, 12, 18
		time = p->round_deaths+offset == 1 ? multiple : (p->round_deaths-1+offset) * (multiple*2);
	}

	// If you're the only player on your team, you get one free instant respawn on first death
	if (isWipeout && p->is_solo && p->round_deaths+offset == 1) {
		time = 0;
	}

	return time;
}

// returns 0 if player has at least one alive teammate
// otherwise returns number of seconds until next teammate respawns
float last_alive_time(gedict_t *player)
{
	gedict_t *p;
	float time = 0;

	for (p = world; (p = find_plr_same_team(p, getteam(player)));)
	{
		if (p->ca_ready)
		{
			if (p->in_play && p != player)
			{
				time = 0;
				break;
			}
			else if (!p->in_play)
			{
				if (!time || p->seconds_to_respawn < time)
				{
					time = p->seconds_to_respawn;
				}
			}
		}
	}

	// this checks to see if there's already a last_alive_countdown in progress
	// because we only want to play the audio once at the begining of the countdown
	// Only play the audio if the player is alive
	if (!player->last_alive_active && (time > 0) && player->in_play)
	{
		player->last_alive_active = true;

		stuffcmd(player, "play misc/medkey.wav\n");
	}

	else if (time == 0)
	{
		if (player->last_alive_active && player->in_play)
		{
			player->escape_time = g_globalvars.time; // start timer for escape time
		}

		player->last_alive_active = false;
	}

	return time;
}

float enemy_last_alive_time(gedict_t *player)
{
	gedict_t *p;
	float time = 0;
	int alive_enemies = 0;

	for (p = world; (p = find_plr(p));)
	{
		if (p->ca_ready && strneq(getteam(p), getteam(player)))
		{
			if (p->in_play)
			{
				alive_enemies++;
			}
			else if (!p->in_play)
			{
				if (!time || p->seconds_to_respawn < time)
				{
					time = p->seconds_to_respawn;
				}
			}
		}
	}

	return alive_enemies > 1 ? 0 : time;
}

float team_last_alive_time(int team)
{
	gedict_t *p;
	float time = 999;
	char* team_name = team ? (team == 1 ? cvar_string("_k_team1") : cvar_string("_k_team2")) : "";

	for (p = world; (p = find_plr_same_team(p, team_name));)
	{
		if (p->ca_ready && !p->in_play)
		{
			if (p->seconds_to_respawn < time)
			{
				time = p->seconds_to_respawn;
			}
		}
	}

	return time;
}

void SM_PrepareCA(void)
{
	gedict_t *p;

	if (!isCA())
	{
		return;
	}

	if (cvar("k_clan_arena") == 2)
	{
		WO_InitializeSpawns(); // init wipeout spawns
	}

	team1_score = team2_score = 0;
	round_num = 1;

	for (p = world; (p = find_plr(p));)
	{
		if (p->ct == ctPlayer && p->ready)
		{
			p->ca_ready = p->ready;
			p->seconds_to_respawn = 0;
			p->teamcolor = NULL;
		}
	}
}

int CA_wins_required(void)
{
	int k_clan_arena_rounds = bound(3, cvar("k_clan_arena_rounds"), 101);

	k_clan_arena_rounds += (k_clan_arena_rounds % 2) ? 0 : 1;

	return ((k_clan_arena_rounds + 1) / 2);
}

qbool isCA(void)
{
	return (isTeam() && cvar("k_clan_arena"));
}

// Used to determine value of ca_alive when PutClientInServer() is called
qbool CA_CheckAlive(gedict_t *p)
{
	if (p)
	{
		if (!match_in_progress)
		{
			return true;
		}
		else if (!p->ca_ready && !match_over)
		{
			return false;
		}
		else if (ra_match_fight != 2 || p->in_limbo)
		{
			return true;
		}
		else {
			return false;
		}
	}
	else
	{
		return false;
	}
}

void CA_AddLatePlayer(gedict_t *p, char *team)
{
	p->ready = 1;
	p->ca_ready = 1;
	p->lj_accepted = 1; // Set flag to allow team change in FixPlayerTeam

	SetUserInfo(p, "team", team, 0);
	stuffcmd_flags(p, STUFFCMD_IGNOREINDEMO, "team \"%s\"\n", team);
	G_bprint(2, "%s late-joined team \x90%s\x91\n", p->netname, team);

	p->lj_accepted = 0;  			// clear the flag immediately
	p->ljteam[0] = '\0';			// clear the requested team name
	p->can_respawn = false;			// can't join mid-round
	p->seconds_to_respawn = 999; 	// don't show countdown
}

void CA_MatchBreak(void)
{
	gedict_t *p;

	// reset these so a new game can be started right away
	ca_round_pause = 0;
	ra_match_fight = 0;
	print_stats = false;
	do_endround_stuff = false;

	// stop recording demo
	localcmd("sv_demostop\n"); 

	// respawn all players
	for (p = world; (p = find_plr(p));)
	{
		if (p->ct == ctPlayer)
		{
			k_respawn(p, false);
		}

		p->ca_ready = 0;	// this needs to be reset
	}
}

void track_player(gedict_t *observer)
{
	gedict_t *player = ca_get_player(observer);
	vec3_t delta;
	float vlen;
	int follow_distance;
	int upward_distance;

	if (player && !observer->in_play && observer->tracking_enabled)
	{
		if (observer->track_target && observer->track_target->in_play)
		{
			// is the observer not playing or is the round over?
			if (!observer->ca_ready || ca_round_pause)
			{	
				player = observer->track_target;
			}
			// otherwise is the target on the observer's team?
			else if (streq(getteam(observer), getteam(observer->track_target)))
			{
				player = observer->track_target;
			}
			// if not, find a different player to watch
			else
			{
				observer->track_target = player;
			}
		}
		else
		{
			observer->track_target = player;
		}

		// { spectate in 1st person
		follow_distance = -10;
		upward_distance = 0;
		observer->hideentity = EDICT_TO_PROG(player); // in this mode we want to hide player model for watcher's view
		VectorCopy(player->s.v.v_angle, observer->s.v.angles);
		// }

		observer->s.v.fixangle = true; // force client v_angle (disable in 3rd person view)

		trap_makevectors(player->s.v.angles);
		VectorMA(player->s.v.origin, follow_distance, g_globalvars.v_forward, observer->s.v.origin);
		VectorMA(observer->s.v.origin, upward_distance, g_globalvars.v_up, observer->s.v.origin);

		// avoid positionning in walls
		traceline(PASSVEC3(player->s.v.origin), PASSVEC3(observer->s.v.origin), false, player);
		VectorCopy(g_globalvars.trace_endpos, observer->s.v.origin);

		if (g_globalvars.trace_fraction == 1)
		{
			VectorCopy(g_globalvars.trace_endpos, observer->s.v.origin);
			VectorMA(observer->s.v.origin, 10, g_globalvars.v_forward, observer->s.v.origin);
		}
		else
		{
			VectorSubtract(g_globalvars.trace_endpos, player->s.v.origin, delta);
			vlen = VectorLength(delta);
			vlen = vlen - 40;
			VectorNormalize(delta);
			VectorScale(delta, vlen, delta);
			VectorAdd(player->s.v.origin, delta, observer->s.v.origin);
		}

		// set observer's health/armor/ammo/weapon to match the player's
		observer->s.v.ammo_nails = player->s.v.ammo_nails;
		observer->s.v.ammo_shells = player->s.v.ammo_shells;
		observer->s.v.ammo_rockets = player->s.v.ammo_rockets;
		observer->s.v.ammo_cells = player->s.v.ammo_cells;
		observer->s.v.currentammo = player->s.v.currentammo;
		observer->s.v.armorvalue = player->s.v.armorvalue;
		observer->s.v.armortype = player->s.v.armortype;
		observer->s.v.health = player->s.v.health;
		observer->s.v.items = player->s.v.items;
		observer->s.v.weapon = player->s.v.weapon;
		observer->weaponmodel = player->weaponmodel;
		observer->s.v.weaponframe = player->s.v.weaponframe;

		// smooth playing for ezq / zq
		observer->s.v.movetype = MOVETYPE_LOCK;

		show_tracking_info(observer);
	}

	if (!player || !observer->tracking_enabled)
	{
		// restore movement and show racer entity
		observer->s.v.movetype = MOVETYPE_NOCLIP;
		observer->hideentity = 0;

		// set health/item values back to nothing
		observer->s.v.ammo_nails = 0;
		observer->s.v.ammo_shells = 0;
		observer->s.v.ammo_rockets = 0;
		observer->s.v.ammo_cells = 0;
		observer->s.v.currentammo = 0;
		observer->s.v.armorvalue = 0;
		observer->s.v.armortype = 0;
		observer->s.v.health = 100;
		observer->s.v.items = 0;
	}
}

void enable_player_tracking(gedict_t *e, int follow)
{
	if (follow)
	{
		if (e->tracking_enabled)
		{
			return;
		}

		G_sprint(e, 2, "tracking %s\n", redtext("enabled"));
		e->tracking_enabled = 1;
	}
	else
	{
		if (!e->tracking_enabled)
		{
			return;
		}

		G_sprint(e, 2, "tracking %s\n", redtext("disabled"));
		snprintf(e->cptext, sizeof(e->cptext), "%s", "");
		G_centerprint(e, "%s", e->cptext);

		e->tracking_enabled = 0;
		SetVector(e->s.v.velocity, 0, 0, 0);
	}
}

void r_changetrackingstatus(float t)
{
	switch ((int)t)
	{
		case 1: // rfollow
			enable_player_tracking(self, 1);
			return;

		case 2: // rnofollow
			enable_player_tracking(self, 0);
			return;

		case 3: // rftoggle
			enable_player_tracking(self, !self->tracking_enabled);
			return;

		default:
			return;
	}
}

void ClanArenaTrackingToggleButton(void)
{
	if ((self->ct == ctPlayer) && ISDEAD(self) && !self->ca_ready)
	{
		if (self->s.v.button0)
		{
			if (!(((int)(self->s.v.flags)) & FL_ATTACKRELEASED))
			{
				return;
			}

			self->s.v.flags = (int)self->s.v.flags & ~FL_ATTACKRELEASED;

			r_changetrackingstatus((float) 3);
		}
		else
		{
			self->s.v.flags = ((int)(self->s.v.flags)) | FL_ATTACKRELEASED;
		}

		return;
	}
}

void CA_PutClientInServer(void)
{
	if (!isCA())
	{
		return;
	}

	// set CA self params
	if (match_in_progress == 2)
	{
		int items;

		self->s.v.ammo_nails = 200;
		self->s.v.ammo_shells = 100;
		self->s.v.ammo_rockets = 50;
		self->s.v.ammo_cells = 150;

		self->s.v.armorvalue = 200;
		self->s.v.armortype = 0.8;
		self->s.v.health = 100;

		self->ca_ammo_grenades = 6;

		items = 0;
		items |= IT_AXE;
		items |= IT_SHOTGUN;
		items |= IT_NAILGUN;
		items |= IT_SUPER_NAILGUN;
		items |= IT_SUPER_SHOTGUN;
		items |= IT_ROCKET_LAUNCHER;
		items |= IT_GRENADE_LAUNCHER;
		items |= IT_LIGHTNING;
		items |= IT_ARMOR3; // add red armor

		self->s.v.items = items;

		// { remove invincibility/quad if any
		self->invincible_time = 0;
		self->invincible_finished = 0;
		self->super_time = 0;
		self->super_damage_finished = 0;
		// }

		// must reset this to 0 or spectated player from 
		// previous round will be invisible
		self->hideentity = 0;

		// reset escape_time, last_alive, and regen_timer every spawn
		self->escape_time = 0;
		self->last_alive_active = false;
		self->regen_timer = 0;

		// default to spawning with rl
		self->s.v.weapon = IT_ROCKET_LAUNCHER;

		self->in_play = true;
		self->in_limbo = false;

		if (!self->teamcolor && self->ca_ready)
		{
			// if your team is "red" or "blue", set color to match
			if (streq(getteam(self), "red") || streq(getteam(self), "blue"))
			{
				self->teamcolor = streq(getteam(self), "red") ? "4" : "13";
			}
			// if a "red" team exists, you aren't on it, so set color to blue
			else if (streq(cvar_string("_k_team1"), "red") || streq(cvar_string("_k_team2"), "red"))
			{
				self->teamcolor = "13";
			}
			// if a "blue" team exists, you aren't on it, so set color to red
			else if (streq(cvar_string("_k_team1"), "blue") || streq(cvar_string("_k_team2"), "blue"))
			{
				self->teamcolor = "4";
			}
			// neither "red" nor "blue" teams exist, so default to team1 being red and team2 blue
			else 
			{
				self->teamcolor = streq(cvar_string("_k_team1"), getteam(self)) ? "4" : "13";
			}
		}

		SetUserInfo(self, "topcolor", self->teamcolor ? self->teamcolor : "0", 0);
		SetUserInfo(self, "bottomcolor", self->teamcolor ? self->teamcolor : "0", 0);
		stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "color %s\n", self->teamcolor ? self->teamcolor : "0");
	}

	// set to ghost if dead
	if (ISDEAD(self))
	{
		int max_deaths = cvar("k_clan_arena_max_respawns");

		self->s.v.solid = SOLID_NOT;
		self->s.v.movetype = MOVETYPE_NOCLIP;
		self->vw_index = 0;

		self->s.v.armorvalue = 0;
		self->s.v.health = 100;

		// tracking enabled by default
		self->tracking_enabled = 1;

		self->in_play = false;
		self->round_deaths++; //increment death count for wipeout
		self->in_limbo = (self->ca_ready) && (self->round_deaths <= max_deaths) && self->can_respawn;
		self->spawn_delay = 0;

		setmodel(self, "");
		setorigin(self, PASSVEC3(self->s.v.origin));

		if (!self->in_limbo || ca_round_pause)
		{
			// Change color to white if dead or not playing
			SetUserInfo(self, "topcolor", "0", 0);
			SetUserInfo(self, "bottomcolor", "0", 0);
			stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "color %s\n", "0");
		}
		
	}
}

qbool CA_can_fire(gedict_t *p)
{
	if (!p)
	{
		return false;
	}

	if (!isCA())
	{
		return true;
	}

	if (!ra_match_fight && p->ready)
	{
		return true;	// allow fire during prewar if /ready
	}

	return (ISLIVE(p) && (ra_match_fight == 2) && time_to_start && (g_globalvars.time >= time_to_start));
}

void CA_show_greeting(gedict_t *self)
{
	char* mode = cvar("k_clan_arena") == 2 ? "Wipeout!" : "Clan Arena!";

	if (self->ct == ctPlayer && !match_in_progress)
	{
		if (!self->ready)
		{
			G_centerprint(self, "Welcome to %s\n\n\n%s %s", 
							mode,
							"set your team and type",
							redtext("/ready"));
		}
		else{
			G_centerprint(self, "%s\n\n\n%s", 
							"You are ready!",
							"waiting for other players");
		}
		
	}
}

void CA_UpdateClients(void)
{
	gedict_t *p;
	static double lastupdate = 0;

	if (g_globalvars.time - lastupdate < 0.5)
	{
		return;
	}

	lastupdate = g_globalvars.time;

	for (p = world; (p = find_client(p));)
	{
		CA_SendTeamInfo(p);
	}
}

// sends player state information to client for use in teaminfo or other hud elements
void CA_SendTeamInfo(gedict_t *t)
{
	int cl;
	int cnt;
	int origin0;
	int origin1;
	int origin2;
	int h;
	int a;
	int items;
	int shells;
	int nails;
	int rockets;
	int cells;
	int camode;
	int deadtype;
	int timetospawn;
	int kills;
	int deaths;
	int max_deaths;
	int track_target;
	gedict_t *p, *s;
	char *tm, *nick;

	s = ((t->ct == ctSpec) ? PROG_TO_EDICT(t->s.v.goalentity) : t);
	if (s->ct != ctPlayer)
	{
		return;
	}

	tm = getteam(s);

	camode = 1;		// 1 is the only mode right now
	deadtype = 0;
	timetospawn = 0;
	max_deaths = cvar("k_clan_arena_max_respawns");

	for (cnt = 0, p = world; (p = find_plr(p));)
	{
		if (cnt >= 10)
		{
			break;
		}

		if (t->trackent && (t->trackent == NUM_FOR_EDICT(p)))
		{
			continue; // we pseudo speccing such player, no point to send info about him
		}

		if (p->ca_ready || match_in_progress != 2) // be sure to send info if in prewar
		{
			if (match_in_progress == 2)
			{
				if (ISLIVE(p))
				{
					deadtype = 0;	// player is alive/active in round
				}
				else
				{
					deadtype = 1;	// player is dead but will respawn

					if ((p->round_deaths > max_deaths) || (p->seconds_to_respawn == 999))
					{
						deadtype = 2;	// player is dead and won't respawn
					}
				}

				timetospawn = (int)ceil(p->seconds_to_respawn);
			}
		}
		else
		{
			continue; // don't send if player isn't ready/playing
		}
		
		if (strnull(nick = ezinfokey(p, "k_nick"))) // get nick, if any, do not send name, client can guess it too
		{
			nick = ezinfokey(p, "k");
		}

		if (nick[0] && nick[1] && nick[2] && nick[3])
		{
			nick[4] = 0; // truncate nick to 4 symbols
		}

		cnt++;

		cl = NUM_FOR_EDICT(p) - 1;

		if (streq(tm, getteam(p)))
		{
			// only teammates should get health/armor/loc/items information
			origin0 = (int)p->s.v.origin[0];
			origin1 = (int)p->s.v.origin[1];
			origin2 = (int)p->s.v.origin[2];
			h = bound(0, (int)p->s.v.health, 999);
			a = bound(0, (int)p->s.v.armorvalue, 999);
			items = (int)p->s.v.items;
			shells = bound(0, (int)p->s.v.ammo_shells, 999);
			nails = bound(0, (int)p->s.v.ammo_nails, 999);
			rockets = bound(0, (int)p->s.v.ammo_rockets, 999);
			cells = bound(0, (int)p->s.v.ammo_cells, 999);
		}
		else
		{
			origin0 = origin1 = origin2 = h = a = items = shells = nails = rockets = cells = 0;
		}
		
		kills = bound(0, (int)p->round_kills, 999);
		deaths = bound(0, (int)p->round_deaths, 999);
		track_target = p->track_target ? NUM_FOR_EDICT(p->track_target) : -1;

		stuffcmd(t, "//cainfo %d %d %d %d %d %d %d \"%s\" %d %d %d %d %d %d %d %d %d %d\n", cl,
						origin0, origin1, origin2, h, a, items, nick, shells, nails, rockets, cells, 
						camode, deadtype, timetospawn, kills, deaths, track_target);
	}
}

// wipeout: check if dying player survived just long enough for teammate to spawn
void CA_check_escape(gedict_t *targ, gedict_t *attacker)
{
	float escape_time = g_globalvars.time - targ->escape_time;
	gedict_t *p;

	if (escape_time > 0 && escape_time < 0.3)
	{
		for (p = world; (p = find_plr_same_team(p, getteam(targ)));)
		{
			stuffcmd(p, "play ca/hero.wav\n");
		}

		for (p = world; (p = find_plr_same_team(p, getteam(attacker)));)
		{
			stuffcmd(p, "play boss2/idle.wav\n");
		}

		// Player is rewarded with an extra life.
		// Escaping a wipe on the first life results in instant respawn.
		// That's cool, but could be written cleaner in calc_respawn_time(). 
		targ->round_deaths--;

		G_bprint(2, "%s survives by &cff0%.0f&r seconds!\n", targ->netname, escape_time*1000);
	}
}

// wipeout: solo players (one-man teams) who don't take damage for 5 seconds
// after earning a frag get their health/armor/ammo regenerated.
void check_solo_regen(gedict_t *p)
{
	int required_time = p->round_kills * 5;
	int time_since_kill = p->regen_timer ? g_globalvars.time - p->regen_timer : 0;

	if (!p->is_solo)
	{
		return;
	}

	if (p->regen_timer && time_since_kill > required_time)
	{
		// regenerate health/armore/ammo. play megahealth sound or secret sound
		stuffcmd(p, "play misc/secret.wav\n");

		if (!((int)self->s.v.items & IT_ARMOR3))
		{
			p->s.v.items += IT_ARMOR3;
		}
		
		p->s.v.armorvalue = 200;
		p->s.v.armortype = 0.8;
		p->s.v.health = 100;
		p->s.v.ammo_nails = 200;
		p->s.v.ammo_shells = 100;
		p->s.v.ammo_rockets = 50;
		p->s.v.ammo_cells = 150;
		p->ca_ammo_grenades = 6;

		// reset the timer
		p->regen_timer = 0;	
	}
}

void CA_ClientObituary(gedict_t *targ, gedict_t *attacker)
{
	attacker->round_kills++;
	
	if (cvar("k_clan_arena") == 2)	// Wipeout only
	{
		// check if attacker is a solo player and start regen timer
		if (attacker->is_solo && !attacker->regen_timer)
		{
			attacker->regen_timer = g_globalvars.time;
		}

		// check if targ was a lone survivor waiting for teammate to spawn
		CA_check_escape(targ, attacker);
	}

	// int ah, aa;

	// if (!isCA())
	// {
	// 	return;
	// }

	// if (targ->ct != ctPlayer)
	// {
	// 	return; // so below targ is player
	// }

	// if (attacker->ct != ctPlayer)
	// {
	// 	attacker = targ; // seems killed self
	// }

	// ah = attacker->s.v.health;
	// aa = attacker->s.v.armorvalue;

	// if (attacker->ct == ctPlayer)
	// {
	// 	if (attacker != targ)
	// 	{
	// 		// This is classic CA behavior, but maybe we
	// 		// don't want players to know their killer's
	// 		// stats before the round is over. Commenting this
	// 		// out for now.
	// 		// G_sprint(targ, PRINT_HIGH, "%s %s %d %s %d %s\n",
	// 		// 			attacker->netname, redtext("had"), aa,
	// 		// 			redtext("armor and"), ah, redtext("health"));
	// 	}
	// }
}

// check if a team is a solo player on first death
static qbool is_solo_team_first_death(char *team)
{
	gedict_t *p;
	int team_size = 0;
	gedict_t *team_player = NULL;

	if (!team || cvar("k_clan_arena") != 2) // Only applies to wipeout
	{
		return false;
	}

	for (p = world; (p = find_plr_same_team(p, team));)
	{
		if (p->ca_ready)
		{
			team_player = p;
			team_size++;

			if (team_size > 1)
			{
				return false;
			}
		}
	}

	return (team_player
		&& team_player->is_solo				// redundant but free, so why not
		&& team_player->round_deaths <= 1	// "<= 1" avoids a race condition
		&& team_player->can_respawn			// makes sures player didn't /kill
	);
}

// return 0 if there no alive teams
// return 1 if there one alive team and alive_team point to 1 or 2 wich refering to _k_team1 or _k_team2 cvars
// return 2 if there at least two alive teams
static int CA_check_alive_teams(int *alive_team)
{
	gedict_t *p;
	qbool few_alive_teams = false;
	char *first_team = NULL;
	char *dead_team = NULL;

	if (alive_team)
	{
		*alive_team = 0;
	}

	for (p = world; (p = find_plr(p));)
	{
		if (!first_team)
		{
			if (ISLIVE(p))
			{
				first_team = getteam(p); // ok, we found first team with alive players
			}

			continue;
		}

		if (strneq(first_team, getteam(p)))
		{
			if (ISLIVE(p))
			{
				few_alive_teams = true; // we found at least two teams with alive players
				break;
			}
		}
	}

	if (few_alive_teams)
	{
		return 2;
	}

	if (first_team)
	{
		if (alive_team)
		{
			*alive_team = streq(first_team, cvar_string("_k_team1")) ? 1 : 2;
		}

		// Wipeout only:
		// Check if the "dead" team is actually a solo player on first death
		dead_team = streq(first_team, cvar_string("_k_team1")) ? cvar_string("_k_team2") : cvar_string("_k_team1");
		if (is_solo_team_first_death(dead_team))
		{
			return 2; // Both teams still in play - solo player gets instant respawn
		}

		return 1;
	}
	else
	{	
		// Wipeout only:
		// Both teams are "dead" but one or both teams may be a solo player on his first death
		if (is_solo_team_first_death(cvar_string("_k_team1")) || 
		    is_solo_team_first_death(cvar_string("_k_team2")))
		{
			return 2; // At least one team consists of a solo player on first death
		}
	}

	return 0;
}

void CA_PrintScores(void)
{
	int s1 = team1_score;
	int s2 = team2_score;
	char *t1 = cvar_string("_k_team1");
	char *t2 = cvar_string("_k_team2");

	G_sprint(self, 2, "%s \x90%s\x91 = %s\n", redtext("Team"), (s1 > s2 ? t1 : t2),
				dig3(s1 > s2 ? s1 : s2));
	G_sprint(self, 2, "%s \x90%s\x91 = %s\n", redtext("Team"), (s1 > s2 ? t2 : t1),
				dig3(s1 > s2 ? s2 : s1));
}

void CA_TeamsStats(void)
{
	if (team1_score != team2_score)
	{
		G_bprint(2, "%s \x90%s\x91 wins %d to %d\n", redtext("Team"),
					cvar_string(va("_k_team%d", team1_score > team2_score ? 1 : 2)),
					team1_score > team2_score ? team1_score : team2_score,
					team1_score > team2_score ? team2_score : team1_score);
	}
	else
	{
		G_bprint(2, "%s have equal scores %d\n", redtext("Teams"), team1_score);
	}
}

void team_round_summary(int alive_team)
{
	int t1_score = alive_team == 1 ? team1_score + 1 : team1_score;
	int t2_score = alive_team == 2 ? team2_score + 1 : team2_score;
	char *team1 = cvar_string(va("_k_team1"));
	char *team2 = cvar_string(va("_k_team2"));

	int winreq = CA_wins_required();
	int t1_need = winreq - t1_score;
	int t2_need = winreq - t2_score;

	G_bprint(2,
		"team   wins need status\n"
		"%s\n"
		"%-6s %4d %4d %s\n"
		"%-6s %4d %4d %s\n"
		"\n",
		redtext("------ ---- ---- ------------"),
		team1, t1_score, t1_need,
		!alive_team ? "tied round" : (alive_team == 1 ? "round winner" : ""),
		team2, t2_score, t2_need,
		!alive_team ? "tied round" : (alive_team == 2 ? "round winner" : ""));
}

void print_player_stats(qbool series_over)
{
	gedict_t *p;

	G_bprint(2,
		"\nsco  damg took  k  d  gl  rh  rd  lg%% player\n%s\n",
		redtext("--- ----- ---- -- -- --- --- --- ---- --------"));

	for (p = world; (p = find_plr(p));)
	{
		if (p->ready && 
				(streq(getteam(p), cvar_string(va("_k_team1"))) || 
				streq(getteam(p), cvar_string(va("_k_team2"))) ))
		{
			CA_OnePlayerStats(p, series_over);
		}
	}

	G_bprint(2, "\n");
}

void CA_OnePlayerStats(gedict_t *p, qbool series_over)
{
	qbool use_totals = (round_num == 1 || series_over);
	float frags;
	float rkills;
	float dmg_g;
	float dmg_t;
	float vh_rl;
	float h_rl;
	float vh_gl;
	float h_lg;
	float a_lg;
	float e_lg;
	float round_elg;
	char score[10];
	char damage[10];
	char dmg_took[10];
	char kills[10];
	char deaths[10];
	char gl_hits[10];
	char rl_hits[10];
	char rl_directs[10];
	char lg_eff[10];

	frags = p->s.v.frags;
	dmg_g = p->ps.dmg_g;
	dmg_t = p->ps.dmg_t;

	rkills = frags - ((int)(dmg_g/100.0));
	h_rl = p->ps.wpn[wpRL].hits;
	vh_rl = p->ps.wpn[wpRL].vhits;
	vh_gl = p->ps.wpn[wpGL].vhits;
	h_lg = p->ps.wpn[wpLG].hits;
	a_lg = p->ps.wpn[wpLG].attacks;
	e_lg = 100.0 * h_lg / max(1, a_lg);

	if (!use_totals)
	{
		round_elg = 100 * (h_lg - p->ca_round_lghit) / max(1, a_lg - p->ca_round_lgfired);
	}

	snprintf(score, sizeof(score), "%.0f", use_totals ? p->s.v.frags : p->s.v.frags - p->ca_round_frags);
	snprintf(damage, sizeof(damage), "%.0f", use_totals ? dmg_g : dmg_g - p->ca_round_dmg);
	snprintf(dmg_took, sizeof(dmg_took), "%.0f", use_totals ? dmg_t : dmg_t - p->ca_round_dmgt);
	snprintf(kills, sizeof(kills), "%.0f", use_totals ? rkills : rkills - p->ca_round_kills);
	snprintf(deaths, sizeof(deaths), "%.0f", use_totals ? p->deaths : p->deaths - p->ca_round_deaths);
	snprintf(gl_hits, sizeof(gl_hits), "%.0f", use_totals ? vh_gl : vh_gl - p->ca_round_glhit);
	snprintf(rl_hits, sizeof(rl_hits), "%.0f", use_totals ? vh_rl : vh_rl - p->ca_round_rlhit);
	snprintf(rl_directs, sizeof(rl_directs), "%.0f", use_totals ? h_rl : h_rl - p->ca_round_rldirect);
	snprintf(lg_eff, sizeof(lg_eff), "%.0f", use_totals ? e_lg : round_elg);

	G_bprint(2, "%3s %5s %4s %2s %2s %3s %3s %3s %3s%s %s\n",
		strneq(score,      "0") ? score        : "-",
		strneq(damage,     "0") ? damage       : "-",
		strneq(dmg_took,   "0") ? dmg_took     : "-",
		strneq(kills,      "0") ? kills        : "-",
		strneq(deaths,     "0") ? deaths       : "-",
		strneq(gl_hits,    "0") ? gl_hits      : "-",
		strneq(rl_hits,    "0") ? rl_hits      : "-",
		strneq(rl_directs, "0") ? rl_directs   : "-",
		strneq(lg_eff,     "0") ? lg_eff       : "-",
		strneq(lg_eff,     "0") ? redtext("%") : " ",
		getname(p));

	p->ca_round_frags = p->s.v.frags;
	p->ca_round_kills = rkills;
	p->ca_round_dmg = dmg_g;
	p->ca_round_dmgt = dmg_t;
	p->ca_round_deaths = p->deaths;
	p->ca_round_glhit = vh_gl;
	p->ca_round_rlhit = vh_rl;
	p->ca_round_rldirect = h_rl;
	p->ca_round_lghit = h_lg;
	p->ca_round_lgfired = a_lg;
}

void EndRound(int alive_team)
{
	gedict_t *p;
	static int last_count;
	char round_or_series[10] = "";
	
	if(!ca_round_pause)
	{
		ca_round_pause = 1;
		last_count = 999999999;
		pause_time = g_globalvars.time + 8;
		loser_team = alive_team ? (alive_team == 1 ? 2 : 1) : 0;
		loser_respawn_time = loser_team ? team_last_alive_time(loser_team) : 999;
	}

	pause_count = Q_rint(pause_time - g_globalvars.time);

	if (pause_count <= 0)
	{
		ca_round_pause = 0;
		round_num++;
		ra_match_fight = 0;
		do_endround_stuff = false;
		print_stats = false;

		if (!alive_team)
		{
			round_num--; // the round repeats in the case of a draw
		}
		else if (alive_team == 1)
		{
			team1_score++;
		}
		else
		{
			team2_score++;
		}
	}
	else if (pause_count != last_count)
	{
		last_count = pause_count;

		if (pause_count <= 7)
		{
			if (!alive_team)
			{
				G_cp2all("Round draw!");
			}
			else
			{
				snprintf(round_or_series, sizeof(round_or_series),
						 "%s", ((alive_team == 1 && team1_score == (CA_wins_required()-1)) ||
						 (alive_team == 2 && team2_score == (CA_wins_required()-1))) ? "series" : "round");

				if ((loser_respawn_time < 1) && (loser_respawn_time > 0))
				{
					G_cp2all("Team \x90%s\x91 wins the %s!\n\n\nTeam %s needed %.0f ms longer",
						cvar_string(va("_k_team%d", alive_team)), round_or_series, cvar_string(va("_k_team%d", loser_team)), loser_respawn_time*1000); 
				}
				else {
					G_cp2all("Team \x90%s\x91 wins the %s!",
						cvar_string(va("_k_team%d", alive_team)), round_or_series); 
				}	
			}

			if (!do_endround_stuff)
			{
				do_endround_stuff = true;
				G_cp2all(" "); // clear any centerprint from during the round

				for (p = world; (p = find_plr(p));)
				{
					if (cvar("k_clan_arena") == 2)
					{
						if (alive_team && streq(getteam(p), cvar_string(va("_k_team%d", alive_team))))
						{
							stuffcmd(p, "play misc/flagcap.wav\n");
						}

						if (alive_team && streq(getteam(p), cvar_string(va("_k_team%d", alive_team))) && p->in_play)
						{
							if (streq(ezinfokey(p, "topcolor"), "13") && streq(ezinfokey(p, "bottomcolor"), "13"))
							{
								p->super_time = 8;
								p->super_damage_finished = 8;
							}
							else
							{
								p->s.v.items += IT_INVULNERABILITY;
							}
						}
					}
				}
			}
			
		}

		if (pause_count < 5 && !print_stats)
		{
			print_stats = true;

			if (alive_team)
			{
				// print health of last standing players
				for (p = world; (p = find_plr(p));)
				{
					if (p->in_play && streq(getteam(p), cvar_string(va("_k_team%d", alive_team))))
					{
						G_bprint(2, "%s %s %.0f%s%.0f\n", 
							p->netname, 
							redtext("had"), 
							p->s.v.armorvalue,
							redtext("/"),
							p->s.v.health);
					}
				}

				G_bprint(2, "\n%s %s %s", 
							redtext("Team"), 
							cvar_string(va("_k_team%d", alive_team)), 
							redtext("has won the round\n"));
			}
			else
			{
				G_bprint(2, "\n%s", 
							redtext("The round is a draw!\n"));
				
				for (p = world; (p = find_plr(p));)
				{	
					stuffcmd(p, "play ca/sfdraw.wav\n");
				}
			}

			print_player_stats(false);
			team_round_summary(alive_team);
		}

		if (pause_count < 4)
		{
			ra_match_fight = 1; // disable firing
		}
	}
}

void show_tracking_info(gedict_t *p)
{
	if (!ca_round_pause)
	{
		if (p->in_limbo)
		{
			snprintf(p->cptext, sizeof(p->cptext), "\n\n\n\n\n\n%s\n\n\n%d\n\n\n seconds to respawn\n",
								redtext(p->track_target->netname), (int)ceil(p->seconds_to_respawn));

			G_centerprint(p, "%s", p->cptext);
		}
		else
		{
			snprintf(p->cptext, sizeof(p->cptext), "\n\n\n\n\n\n%s\n\n\n\n\n\n\n",
								redtext(p->track_target->netname));

			G_centerprint(p, "%s", p->cptext);
		}
	}
}

static void track_player_next(gedict_t *observer)
{
	gedict_t *first_player = ca_get_player(observer);
	gedict_t *player = first_player;

	if (!first_player)
	{
		return;
	}

	if (observer->track_target && ((observer->track_target)->ct != ctPlayer))
	{
		observer->track_target = player;
	}

	if (!observer->track_target)
	{
		observer->track_target = player;
	}
	else
	{
		player = ca_find_player(observer->track_target, observer);
		if (!player)
		{
			player = first_player;
		}

		observer->track_target = player;
	}
}

void CA_player_pre_think(void)
{
	if (isCA())
	{
		float alive_time = self->in_play ? g_globalvars.time - self->time_of_respawn : 0;

		CA_show_greeting(self);
		
		if ((self->s.v.mins[0] == 0) || (self->s.v.mins[1] == 0))
		{
			// This can happen if the world 'squashes' a SOLID_NOT entity, mvdsv will turn into corpse
			setsize(self, PASSVEC3(VEC_HULL_MIN), PASSVEC3(VEC_HULL_MAX));
		}
		setorigin(self, PASSVEC3(self->s.v.origin));

		if ((self->ct == ctPlayer) && (ISDEAD(self) || !self->in_play))
		{
			if (self->tracking_enabled)
			{
				if (self->s.v.button2)
				{
					if (((int)(self->s.v.flags)) & FL_JUMPRELEASED)
					{
				 		self->s.v.flags = (int)self->s.v.flags & ~FL_JUMPRELEASED;

						track_player_next(self);
					}
				}
				else
				{
					self->s.v.flags = ((int)(self->s.v.flags)) | FL_JUMPRELEASED;
				}
			}
		}

		if (self->ct == ctPlayer && ra_match_fight && !self->in_play)
		{
			track_player(self); // enable tracking by default while dead
		}

		// wipeout: if you're a solo and waiting for health regen
		if (cvar("k_clan_arena") == 2 && self->is_solo && self->regen_timer)
		{
			check_solo_regen(self);
		}

		// take no damage to health/armor within 1 second of respawn or during endround
		if (self->in_play && ((alive_time >= 1) || !self->round_deaths) && !ca_round_pause)
		{
			self->no_pain = false;
		}
		else
		{
			self->no_pain = true;
		}

		// players can't change their color
		if (self->teamcolor && (self->in_play || (!ca_round_pause && self->in_limbo)) && 
			(strneq(ezinfokey(self, "topcolor"), self->teamcolor) || strneq(ezinfokey(self, "bottomcolor"), self->teamcolor)))
		{
			SetUserInfo(self, "topcolor", self->teamcolor, 0);
			SetUserInfo(self, "bottomcolor", self->teamcolor, 0);
		}
		// perma-dead players can't change their color
		else if (self->teamcolor && !self->in_play && (!self->in_limbo || !self->can_respawn || ca_round_pause) && 
			(strneq(ezinfokey(self, "topcolor"), "0") || strneq(ezinfokey(self, "bottomcolor"), "0")))
		{
			SetUserInfo(self, "topcolor", "0", 0);
			SetUserInfo(self, "bottomcolor", "0", 0);
		}
		// players who aren't in the game must be white and have no team
		else if (!self->teamcolor && !self->ca_ready && (match_in_progress == 2) &&
			(strneq(ezinfokey(self, "topcolor"), "0") || strneq(ezinfokey(self, "bottomcolor"), "0") || strneq(ezinfokey(self, "team"), "")))
		{
			SetUserInfo(self, "topcolor", "0", 0);
			SetUserInfo(self, "bottomcolor", "0", 0);
			SetUserInfo(self, "team", "", 0);
		}
	}
}

void CA_spectator_think(void)
{
	gedict_t *p;

	p = PROG_TO_EDICT(self->s.v.goalentity); // who we are spectating

	if (p->ct == ctPlayer && !p->in_play && p->tracking_enabled)
	{
		// if the player you're observing is following someone else, hide the player model
		self->hideentity = EDICT_TO_PROG(p->track_target);
	}
	else
	{
		self->hideentity = 0;
	}
	
	if (p->ct == ctPlayer)
	{
		if (match_in_progress == 2 && ra_match_fight == 2 && round_time > 2 && !ca_round_pause)
		{
			// any centerprint the player sees is sent to the spec
			G_centerprint(self, "%s\n", p->cptext);
		}
	}
}

void CA_Frame(void)
{
	static int last_r;
	int r;
	gedict_t *p;

	if (!isCA() || match_over)
	{
		return;
	}

	CA_UpdateClients();

	if (match_in_progress != 2)
	{
		return;
	}

	round_time = Q_rint(g_globalvars.time - time_to_start);

	// if k_clan_arena is 2, we're playing wipeout
	if (ra_match_fight == 2 && !ca_round_pause && cvar("k_clan_arena") == 2)
	{
		float alive_time;
		int last_alive;
		int e_last_alive;
		char str_last_alive[25];
		char str_e_last_alive[25];

		for (p = world; (p = find_plr(p));)
		{
			alive_time = p->in_play ? g_globalvars.time - p->time_of_respawn : 0;
			last_alive = (int)ceil(last_alive_time(p));
			e_last_alive = (int)ceil(enemy_last_alive_time(p));
			
			if (p->in_limbo)
			{
				if (!p->spawn_delay)
				{
					int delay = calc_respawn_time(p, 0);
					p->spawn_delay = g_globalvars.time + delay;
				}

				p->seconds_to_respawn = p->spawn_delay - g_globalvars.time;

				if (p->seconds_to_respawn <= 0)
				{
					p->spawn_delay = 0;
					snprintf(p->cptext, sizeof(p->cptext), "%s\n", "FIGHT!");
					G_centerprint(p, "%s", p->cptext);
					k_respawn(p, true);

					p->seconds_to_respawn = calc_respawn_time(p, 1);
					p->time_of_respawn = g_globalvars.time; // resets alive_time calculations to 0
				}
				else
				{
					if (!p->tracking_enabled)
					{
						snprintf(p->cptext, sizeof(p->cptext), "\n\n\n\n\n\n\n\n\n%d\n\n\n seconds to respawn\n",
								 (int)ceil(p->seconds_to_respawn));
						G_centerprint(p, "%s", p->cptext);
					}
				}
			}
			// you're a solo player... prioritize regen info
			else if (p->is_solo && p->regen_timer)
			{
				int countdown = max(1, (p->round_kills*5) - (int)(g_globalvars.time - p->regen_timer) + 1 );
				snprintf(p->cptext, sizeof(p->cptext), "\n\n\n\n\n\n%s: %d\n\n\n\n",
								"regenerating health", countdown);

				G_centerprint(p, "%s", p->cptext);
			}
			// both you and the enemy are the last alive on your team
			else if (p->in_play && alive_time > 2 && last_alive && e_last_alive)
			{
				snprintf(str_last_alive, sizeof(str_last_alive), "%d", last_alive);
				snprintf(p->cptext, sizeof(p->cptext), "\n\n\n\n\n\n%s\n\n\n%s\n\n\n\n",
								redtext("1 on 1"), last_alive == 999 ? " " : redtext(str_last_alive));

				G_centerprint(p, "%s", p->cptext);
			}
			// you're the last alive on your team versus two or more enemies... hide!
			else if (p->in_play && alive_time > 2 && last_alive)
			{
				snprintf(str_last_alive, sizeof(str_last_alive), "%d", last_alive);
				snprintf(p->cptext, sizeof(p->cptext), "\n\n\n\n\n\n%s\n\n\n%s\n\n\n\n",
								redtext("stay alive!"), last_alive == 999 ? " " : redtext(str_last_alive));

				G_centerprint(p, "%s", p->cptext);
			}
			// only one enemy remains... find him!
			else if (p->in_play && alive_time > 2 && e_last_alive)
			{
				snprintf(str_e_last_alive, sizeof(str_e_last_alive), "%d", e_last_alive);
				snprintf(p->cptext, sizeof(p->cptext), "\n\n\n\n\n\n%s\n\n\n%s\n\n\n\n",
								"one enemy left", e_last_alive == 999 ? " " : str_e_last_alive);

				G_centerprint(p, "%s", p->cptext);
			}
			else if (p->in_play && alive_time > 2)
			{
				snprintf(p->cptext, sizeof(p->cptext), " ");
				G_centerprint(p, "%s", p->cptext);
			}
		}
	}

	// check if there exist only one team with alive players and others are eliminated, if so then its time to start ca countdown
	if (ra_match_fight == 2 || (ra_match_fight == 1 && ca_round_pause == 1))
	{
		int alive_team = 0;

		switch (CA_check_alive_teams(&alive_team))
		{
			case 0: // DRAW, both teams are dead
				EndRound(0);
				break;

			case 1: // Only one team alive
				EndRound(alive_team);
				break;

			default:
				break; // both teams alive
		}

		if (ra_match_fight == 2)
		{
			return;
		}
	}

	if ((team1_score >= CA_wins_required()) || (team2_score >= CA_wins_required()))
	{		
		int winning_team;

		winning_team = team1_score >= CA_wins_required() ? 1 : 2;

		G_bprint(2, "%s", redtext("series statistics:\n"));
		print_player_stats(true);
		G_bprint(2, "%s %s %s", 
							redtext("Team"), 
							cvar_string(va("_k_team%d", winning_team)), 
							redtext("has won the series!\n\n"));

		EndMatch(0);
		
		return;
	}

	if (!ca_round_pause)
	{
		if (!ra_match_fight)
		{
			// ok start ra timer
			ra_match_fight = 1; // ra countdown
			last_r = 999999999;
			time_to_start = g_globalvars.time + 9;
			G_cp2all(" ");

			for (p = world; (p = find_plr(p));)
			{
				if (p->ca_ready)
				{
					p->can_respawn = true;
					p->round_deaths = 0;
					p->round_kills = 0;
					p->seconds_to_respawn = calc_respawn_time(p, 1);
					k_respawn(p, false);
				}

				G_sprint(p, 2, "round \x90%d\x91 starts in 5 seconds.\n", round_num);
				snprintf(p->cptext, sizeof(p->cptext), " "); // reset any server print from last round
			}
		}

		r = Q_rint(time_to_start - g_globalvars.time);

		if (r <= 0)
		{
			for (p = world; (p = find_client(p));)
			{
				stuffcmd(p, "play ca/sffight.wav\n");

				if (p->ca_ready)
				{
					p->time_of_respawn = g_globalvars.time; // resets alive_time calculations to 0
				}
			}
			
			G_cp2all("%s", "FIGHT!");

			ra_match_fight = 2;

			// rounding suck, so this force readytostart() return true right after FIGHT! is printed
			time_to_start = g_globalvars.time;
		}
		else if (r != last_r)
		{
			last_r = r;

			if (r < 6)
			{
				for (p = world; (p = find_client(p));)
				{
					stuffcmd(p, "play ca/sf%d.wav\n", r);
				}
				
				G_cp2all("%d\n\n\n \x90%s\x91:%s \x90%s\x91:%s\n",
							r, cvar_string("_k_team1"), dig3(team1_score), cvar_string("_k_team2"),
							dig3(team2_score)); // CA_wins_required
			}

			if (r == 6)
			{
				for (p = world; (p = find_client(p));)
				{
					stuffcmd(p, "play ca/sf%d.wav\n", round_num);
				}
			}

			if (r == 7)
			{
				for (p = world; (p = find_client(p));)
				{
					stuffcmd(p, "play ca/sfround.wav\n");
				}
				G_cp2all("round %d", round_num);
			}
		}
	}
}

// Find spawn configuration for a given origin
static wipeout_spawn_config* WO_FindSpawnConfig(vec3_t origin)
{
	int i, j;
	
	if (cvar("k_clan_arena") != 2)  // Only for wipeout mode
	{
		return NULL;
	}
	
	// Find current map configuration
	for (i = 0; wipeout_spawn_configs[i].mapname; i++)
	{
		if (streq(mapname, wipeout_spawn_configs[i].mapname))
		{
			// Search for matching spawn point
			for (j = 0; j < wipeout_spawn_configs[i].spawn_count; j++)
			{
				if (VectorCompare(origin, wipeout_spawn_configs[i].spawns[j].origin))
				{
					return &wipeout_spawn_configs[i].spawns[j];
				}
			}
			break;
		}
	}
	
	return NULL;
}

// Get custom spawn radius for a spawn point
float WO_GetSpawnRadius(vec3_t origin)
{
	wipeout_spawn_config *config = WO_FindSpawnConfig(origin);
	
	if (config && config->custom_radius > 0)
	{
		return config->custom_radius;
	}
	
	return 0;  // Use default radius
}

// Initialize wipeout spawns (can be called to reload configurations)
void WO_InitializeSpawns(void)
{
	if (cvar("k_clan_arena") == 2)
	{
		int i;
		for (i = 0; wipeout_spawn_configs[i].mapname; i++)
		{
			if (streq(mapname, wipeout_spawn_configs[i].mapname))
			{	
				if (cvar("developer"))
				{
					G_bprint(2, "Wipeout: Using custom spawn configuration for %s (%d spawns)\n",
                	mapname, wipeout_spawn_configs[i].spawn_count);
				}
                
				break;
			}
		}
	}
}
