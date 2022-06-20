//
// Clan Arena related
//

#include "g_local.h"

static int round_num;
static int team1_score;
static int team2_score;
static int pause_count;
static int pause_time;
static int round_time;
static qbool do_endround_stuff = false;
static qbool print_stats = false;

void track_player(gedict_t *observer);
void enable_player_tracking(gedict_t *e, int follow);
void r_changetrackingstatus(float t);
void CA_PrintScores(void);
void CA_TeamsStats(void);
void print_player_stats(qbool series_over);
void CA_OnePlayerStats(gedict_t *p, qbool series_over);
void EndRound(int alive_team);
void show_tracking_info(gedict_t *p);

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

// returns 0 if player has at least one alive teammate
// otherwise returns number of seconds until next teammate respawns
int last_alive_time(gedict_t *player)
{
	gedict_t *p;
	int time = 0;

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

	if (!player->last_alive_active && (time > 0))
	{
		player->last_alive_active = true;

		stuffcmd(player, "play misc/medkey.wav\n");
	}

	else if (time == 0)
	{
		player->last_alive_active = false;
	}

	return time;
}

int enemy_last_alive_time(gedict_t *player)
{
	gedict_t *p;
	int time = 0;
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

void SM_PrepareCA(void)
{
	gedict_t *p;

	if (!isCA())
	{
		return;
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

qbool isCA()
{
	return (isTeam() && cvar("k_clan_arena"));
}

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

// hard coded default settings for CA
static char ca_settings[] = "k_clan_arena_rounds 9\n"
		"k_clan_arena_max_respawns 0\n"
		"dp 0\n"				// don't drop packs
		"teamplay 4\n"
		"deathmatch 5\n"
		"timelimit 0\n"			// no time limit
		"maxclients 8\n"
		"k_maxclients 8\n"
		"k_pow 0\n"
		"k_overtime 0\n"
		"k_spectalk 1\n"		// enable spec talk by default
		"k_exttime 0\n"	
		"k_spw 1\n"				// KT Safety spawns (important for CA)
		"k_dmgfrags 1\n"		// 1 "frag" for every 100 damage dealt
		"k_teamoverlay 1\n"
		"k_membercount 1\n"		// no minimum team size
		"k_noitems 1\n";

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
		sprintf(e->cptext, "%s", "");
		G_centerprint(e, e->cptext);

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

void apply_CA_settings(void)
{
	char buf[1024 * 4];
	char *cfg_name;

	if (!isCA())
	{
		return;
	}

	trap_readcmd(ca_settings, buf, sizeof(buf));
	G_cprint("%s", buf);

	if (cvar("k_clan_arena") == 2)
	{
		cvar_fset("k_clan_arena_max_respawns", 4);
	}

	cfg_name = va("configs/usermodes/ca/default.cfg");
	if (can_exec(cfg_name))
	{
		trap_readcmd(va("exec %s\n", cfg_name), buf, sizeof(buf));
		G_cprint("%s", buf);
	}

	cfg_name = va("configs/usermodes/ca/%s.cfg", mapname);
	if (can_exec(cfg_name))
	{
		trap_readcmd(va("exec %s\n", cfg_name), buf, sizeof(buf));
		G_cprint("%s", buf);
	}

	G_cprint("\n");
}

void ToggleCArena()
{
	int k_clan_arena = bound(0, cvar("k_clan_arena"), 2);
	
	if (!is_rules_change_allowed())
	{
		return;
	}

	if (match_in_progress)
	{
		return;
	}

	if (!isCA())
	{
		// seems we trying turn CA on.
		if (!isTeam())
		{
			G_sprint(self, 2, "Set %s mode first\n", redtext("team"));

			return;
		}
	}

	if (++k_clan_arena > 2)
	{
		k_clan_arena = 0;
	}

	if (!k_clan_arena)
	{
		G_bprint(2, "%s %s %s\n", self->netname, "disables", redtext("Clan Arena"));
	}
	else if (k_clan_arena == 1)
	{
		G_bprint(2, "%s %s %s\n", self->netname, "enables", redtext("Clan Arena"));
	}
	else if (k_clan_arena == 2)
	{
		G_bprint(2, "%s %s %s\n", self->netname, "enables", redtext("Wipeout"));
	}
	else
	{
		G_bprint(2, "%s unknown\n", redtext("Clan Arena"));
	}

	cvar_fset("k_clan_arena", k_clan_arena);
	apply_CA_settings();
}

void ToggleWipeout()
{
	int k_clan_arena = bound(0, cvar("k_clan_arena"), 2);

	if (!is_rules_change_allowed())
	{
		return;
	}

	if (!isCA())
	{
		// seems we trying turn CA on.
		if (!isTeam())
		{
			G_sprint(self, 2, "Set %s mode first\n", redtext("team"));

			return;
		}
	}

	if (k_clan_arena == 2)
	{
		k_clan_arena = 0;
		G_bprint(2, "%s %s %s\n", self->netname, "disables", redtext("Wipeout"));
	}
	else
	{
		k_clan_arena = 2;
		G_bprint(2, "%s %s %s\n", self->netname, "enables", redtext("Wipeout"));
	}

	cvar_fset("k_clan_arena", k_clan_arena);
	apply_CA_settings();
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
		self->spawn_delay = 0;

		setmodel(self, "");
		setorigin(self, PASSVEC3(self->s.v.origin));

		if (self->ca_ready && self->round_deaths <= max_deaths && !ca_round_pause)
		{
			//change colors to light versions
			if (streq(self->teamcolor, "13"))
			{
				SetUserInfo(self, "topcolor", "2", 0);
				SetUserInfo(self, "bottomcolor", "2", 0);
			}
			else
			{
				SetUserInfo(self, "topcolor", "6", 0);
				SetUserInfo(self, "bottomcolor", "6", 0);
			}
		}
		else
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

void CA_ClientObituary(gedict_t *targ, gedict_t *attacker)
{
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

// return 0 if there no alive teams
// return 1 if there one alive team and alive_team point to 1 or 2 wich refering to _k_team1 or _k_team2 cvars
// return 2 if there at least two alive teams
static int CA_check_alive_teams(int *alive_team)
{
	gedict_t *p;
	qbool few_alive_teams = false;
	char *first_team = NULL;

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

		return 1;
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
	char t1score[5];
	char t2score[5];
	char t1need[5];
	char t2need[5];
	char t1status[20];
	char t2status[20];
	char tmp[36] = "";
	char result[100] = "";
	
	sprintf(t1score, "%d", t1_score);
	sprintf(t2score, "%d", t2_score);
	sprintf(t1need, "%d", CA_wins_required() - t1_score);
	sprintf(t2need, "%d", CA_wins_required() - t2_score);
	sprintf(t1status, "%s", !alive_team ? "tied round" : (alive_team == 1 ? "round winner" : ""));
	sprintf(t2status, "%s", !alive_team ? "tied round" : (alive_team == 2 ? "round winner" : ""));

	sprintf(result, "%s", "team   wins need status\n");
	sprintf(tmp, "%s\n", redtext("------ ---- ---- ------------"));
	strcat(result, tmp);
	
	sprintf(tmp, "%s ", team1);
	strcat(result, tmp);
	for (int i = 0; i < (strlen("team  ") - strlen(team1)); i++)
	{
		strcat(result, " "); // add padding so columns line up
	}

	for (int i = 0; i < (strlen("wins") - strlen(t1score)); i++)
	{
		strcat(result, " "); // add padding so columns line up
	}
	sprintf(tmp, "%s ", t1score);
	strcat(result, tmp);

	for (int i = 0; i < (strlen("need") - strlen(t1need)); i++)
	{
		strcat(result, " "); // add padding so columns line up
	}
	sprintf(tmp, "%s %s\n%s ", t1need, t1status, team2);
	strcat(result, tmp);

	for (int i = 0; i < (strlen("team  ") - strlen(team2)); i++)
	{
		strcat(result, " "); // add padding so columns line up
	}
	
	for (int i = 0; i < (strlen("wins") - strlen(t2score)); i++)
	{
		strcat(result, " "); // add padding so columns line up
	}
	sprintf(tmp, "%s ", t2score);
	strcat(result, tmp);

	for (int i = 0; i < (strlen("need") - strlen(t2need)); i++)
	{
		strcat(result, " "); // add padding so columns line up
	}
	sprintf(tmp, "%s %s\n\n", t2need, t2status);
	strcat(result, tmp);

	G_bprint(2, result);
}

void print_player_stats(qbool series_over)
{
	gedict_t *p;
	char tmp[50];
	char header[200];

	sprintf(header, "%s", "\n");
	strcat(header, "sco  damg took  k  d  gl  rh  rd  lg%% player\n");
	sprintf(tmp, "%s\n", redtext("--- ----- ---- -- -- --- --- --- ---- --------"));
	strcat(header, tmp);
	G_bprint(2, header);
	
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
	char tmp[18] = "";
	char result[100] = "";

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
	
	sprintf(score, "%.0f", use_totals ? p->s.v.frags : p->s.v.frags - p->ca_round_frags);
	sprintf(damage, "%.0f", use_totals ? dmg_g : dmg_g - p->ca_round_dmg);
	sprintf(dmg_took, "%.0f", use_totals ? dmg_t : dmg_t - p->ca_round_dmgt);
	sprintf(kills, "%.0f", use_totals ? rkills : rkills - p->ca_round_kills);
	sprintf(deaths, "%.0f", use_totals ? p->deaths : p->deaths - p->ca_round_deaths);
	sprintf(gl_hits, "%.0f", use_totals ? vh_gl : vh_gl - p->ca_round_glhit);
	sprintf(rl_hits, "%.0f", use_totals ? vh_rl : vh_rl - p->ca_round_rlhit);
	sprintf(rl_directs, "%.0f", use_totals ? h_rl : h_rl - p->ca_round_rldirect);
	sprintf(lg_eff, "%.0f", use_totals ? e_lg : round_elg);
	
	for (int i = 0; i < (strlen("sco") - strlen(score)); i++)
	{
		strcat(result, " ");
	}
	sprintf(tmp, "%s ", strneq(score, "0") ? score : "-");
	strcat(result, tmp);

	for (int i = 0; i < (strlen(" damg") - strlen(damage)); i++)
	{
		strcat(result, " "); // add padding so columns line up
	}
	sprintf(tmp, "%s ", strneq(damage, "0") ? damage : "-");
	strcat(result, tmp);

	for (int i = 0; i < (strlen("took") - strlen(dmg_took)); i++)
	{
		strcat(result, " "); // add padding so columns line up
	}
	sprintf(tmp, "%s ", strneq(dmg_took, "0") ? dmg_took : "-");
	strcat(result, tmp);

	for (int i = 0; i < (strlen(" k") - strlen(kills)); i++)
	{
		strcat(result, " "); // add padding so columns line up
	}
	sprintf(tmp, "%s ",  strneq(kills, "0") ? kills : "-");
	strcat(result, tmp);

	for (int i = 0; i < (strlen(" d") - strlen(deaths)); i++)
	{
		strcat(result, " "); // add padding so columns line up
	}
	sprintf(tmp, "%s ", strneq(deaths, "0") ? deaths : "-");
	strcat(result, tmp);

	for (int i = 0; i < (strlen(" gl") - strlen(gl_hits)); i++)
	{
		strcat(result, " "); // add padding so columns line up
	}
	sprintf(tmp, "%s ", strneq(gl_hits, "0") ? gl_hits : "-");
	strcat(result, tmp);

	for (int i = 0; i < (strlen(" rh") - strlen(rl_hits)); i++)
	{
		strcat(result, " "); // add padding so columns line up
	}
	sprintf(tmp, "%s ", strneq(rl_hits, "0") ? rl_hits : "-");
	strcat(result, tmp);

	for (int i = 0; i < (strlen(" rd") - strlen(rl_directs)); i++)
	{
		strcat(result, " "); // add padding so columns line up
	}
	sprintf(tmp, "%s ", strneq(rl_directs, "0") ? rl_directs : "-");
	strcat(result, tmp);

	for (int i = 0; i < (strlen(" lg") - strlen(lg_eff)); i++)
	{
		strcat(result, " "); // add padding so columns line up
	}
	if (strneq(lg_eff, "0"))
	{
		sprintf(tmp, "%s%s ", lg_eff, redtext("%"));
		strcat(result, tmp);
	}
	else{
		strcat(result, " - ");
	}

	snprintf(tmp, 18, "%s\n", getname(p));
	strcat(result, tmp);

	G_bprint(2, result);

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
	
	if(!ca_round_pause)
	{
		ca_round_pause = 1;
		last_count = 999999999;
		pause_time = g_globalvars.time + 8;
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

		if (pause_count < 7)
		{
			if (!alive_team)
			{
				G_cp2all("Round draw!");
			}
			else
			{
				if ((alive_team == 1 && team1_score == (CA_wins_required()-1)) || 
					(alive_team == 2 && team2_score == (CA_wins_required()-1)))
				{
					G_cp2all("Team \x90%s\x91 wins the series!",
							cvar_string(va("_k_team%d", alive_team))); 
				}
				else
				{
					G_cp2all("Team \x90%s\x91 wins the round!",
							cvar_string(va("_k_team%d", alive_team)));
				}
			}
		}

		if (pause_count == 7)
		{
			if (!do_endround_stuff)
			{
				do_endround_stuff = true;
				G_cp2all(" "); // clear any centerprint from during the round

				for (p = world; (p = find_plr(p));)
				{
					if (cvar("k_clan_arena_max_respawns"))
					{
						if (streq(getteam(p), cvar_string(va("_k_team%d", alive_team))))
						{
							stuffcmd(p, "play misc/flagcap.wav\n");
						}

						if (streq(getteam(p), cvar_string(va("_k_team%d", alive_team))) && p->in_play)
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

						if (p->in_limbo)
						{
							SetUserInfo(p, "topcolor", "0", 0);
							SetUserInfo(p, "bottomcolor", "0", 0);
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
	int max_respawns = cvar("k_clan_arena_max_respawns");

	if (!ca_round_pause)
	{
		if (p->ca_ready && p->round_deaths <= max_respawns && !ca_round_pause)
		{
			sprintf(p->cptext, "\n\n\n\n\n\n%s\n\n\n%d\n\n\n seconds to respawn!\n", 
								redtext(p->track_target->netname), p->seconds_to_respawn);

			G_centerprint(p, p->cptext);
		}
		else
		{
			sprintf(p->cptext, "\n\n\n\n\n\n%s\n\n\n\n\n\n\n", 
								redtext(p->track_target->netname));

			G_centerprint(p, p->cptext);
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
		CA_show_greeting(self);
		
		// Set this player to solid so we trigger checkpoints & teleports during move
		self->s.v.solid = (ISDEAD(self) ? SOLID_NOT : SOLID_SLIDEBOX);
		
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

		if (self->in_play)
		{
			self->alive_time = Q_rint(g_globalvars.time - self->time_of_respawn);
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

	if (match_in_progress != 2)
	{
		return;
	}

	round_time = Q_rint(g_globalvars.time - time_to_start);

	// if max_respawns are greater than 0, we're playing wipeout
	if (ra_match_fight == 2 && !ca_round_pause && cvar("k_clan_arena") == 2)
	{
		int max_deaths = cvar("k_clan_arena_max_respawns");
		int last_alive;
		int e_last_alive;
		char str_last_alive[5];
		char str_e_last_alive[5];

		for (p = world; (p = find_plr(p));)
		{
			last_alive = last_alive_time(p);
			e_last_alive = enemy_last_alive_time(p);
			
			if (p->ca_ready && !p->in_play && p->round_deaths <= max_deaths)
			{
				p->in_limbo = true;

				if (!p->spawn_delay)
				{
					int delay = p->round_deaths == 1 ? 5 : (p->round_deaths-1) * 10;
					p->spawn_delay = g_globalvars.time + delay;
				}

				p->seconds_to_respawn = Q_rint(p->spawn_delay - g_globalvars.time);

				if (p->seconds_to_respawn <= 0)
				{
					p->spawn_delay = 0;
					sprintf(p->cptext, "%s\n", "FIGHT!");
					G_centerprint(p, p->cptext);
					k_respawn(p, true);

					p->seconds_to_respawn = 999;
					p->time_of_respawn = g_globalvars.time; // resets alive_time to 0
				}
				else
				{
					if (!p->tracking_enabled)
					{
						sprintf(p->cptext, "\n\n\n\n\n\n\n\n\n%d\n\n\n seconds to respawn!\n", p->seconds_to_respawn);
						G_centerprint(p, p->cptext);
					}
				}
			}
			else if (p->in_play && p->alive_time > 2 && last_alive)
			{
				sprintf(str_last_alive, "%d", last_alive);
				sprintf(p->cptext, "\n\n\n\n\n\n%s\n\n\n%s\n\n\n\n", 
								redtext("stay alive!"), last_alive == 999 ? " " : str_last_alive);

				G_centerprint(p, p->cptext);
			}
			else if (p->in_play && p->alive_time > 2 && e_last_alive)
			{
				sprintf(str_e_last_alive, "%d", e_last_alive);
				sprintf(p->cptext, "\n\n\n\n\n\n%s\n\n\n%s\n\n\n\n", 
								"one enemy left", e_last_alive == 999 ? " " : str_e_last_alive);

				G_centerprint(p, p->cptext);
			}
			else if (p->in_play && p->alive_time > 2)
			{
				sprintf(p->cptext, " ");
				G_centerprint(p, p->cptext);
			}
		}
	}

	// check if there exist only one team with alive players and others are eluminated, if so then its time to start ca countdown
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
					p->round_deaths = 0;
					k_respawn(p, false);
				}

				G_sprint(p, 2, "round \x90%d\x91 starts in 5 seconds.\n", round_num);
				sprintf(p->cptext, " "); // reset any server print from last round
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
					p->time_of_respawn = g_globalvars.time; // resets alive_time to 0
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
