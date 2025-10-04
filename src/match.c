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

void NextLevel(void);
void IdlebotForceStart(void);
void StartMatch(void);
void remove_specs_wizards(void);
void lastscore_add(void);
void StartLogs(void);
void StopLogs(void);
void ClearDemoMarkers(void);

void EM_CorrectStats(void);
void MatchEndStats(void);
void SM_PrepareTeamsStats(void);
void SM_PrepareShowscores(void);
static void SM_ExecuteQueuedSpawnEffects(void);

void race_match_start(void);
qbool race_can_cancel_demo(void);

extern int g_matchstarttime;
qbool initial_match_spawns;
int maxPlayerCount;

// Return count of players which have state cs_connected or cs_spawned.
// It is weird because used string comparision so I treat it as slow and idiotic but it return more players than CountPlayers().
int WeirdCountPlayers(void)
{
	gedict_t *p;
	int num;
	char state[16];

	for (num = 0, p = world + 1; p <= world + MAX_CLIENTS; p++)
	{
		infokey(p, "*spectator", state, sizeof(state));

		if (state[0])
		{
			continue; // ignore spectators
		}

		infokey(p, "*state", state, sizeof(state));

		if (streq(state, "connected") || streq(state, "spawned"))
		{
			num++;
		}
	}

	return num;
}

float CountPlayers(void)
{
	gedict_t *p;
	float num = 0;

	for (p = world; (p = find_plr(p));)
	{
		num++;
	}

	return num;
}

float CountBots(void)
{
	gedict_t *p;
	float num = 0;

	for (p = world; (p = find_plr(p));)
	{
		if (p->isBot)
		{
			num++;
		}
	}

	return num;
}

float CountRPlayers(void)
{
	gedict_t *p;
	float num = 0;

	for (p = world; (p = find_plr(p));)
	{
		if (p->ready)
		{
			num++;
		}
	}

	return num;
}

float CountTeams(void)
{
	gedict_t *p, *p2;
	float num = 0;
	char *s = "";

	for (p = world; (p = find_plr(p));)
	{
		p->k_flag = 0;
	}

	for (p = world; (p = find_plr(p));)
	{
		if (p->k_flag || strnull(s = getteam(p)))
		{
			continue;
		}

		p->k_flag = 1;
		num++;

		for (p2 = p; (p2 = find_plr(p2));)
		{
			if (streq(s, getteam(p2)))
			{
				p2->k_flag = 1;
			}
		}
	}
	return num;
}

// return number of teams where at least one member is ready?
float CountRTeams(void)
{
	gedict_t *p, *p2;
	float num = 0;
	char *s = "";

	for (p = world; (p = find_plr(p));)
	{
		p->k_flag = 0;
	}

	for (p = world; (p = find_plr(p));)
	{
		if (p->k_flag || !p->ready || strnull(s = getteam(p)))
		{
			continue;
		}

		p->k_flag = 1;
		num++;

		for (p2 = p; (p2 = find_plr(p2));)
		{
			if (streq(s, getteam(p2)))
			{
				p2->k_flag = 1;
			}
		}
	}

	return num;
}

// count the members in each team and store the max in maxPlayerCount
// and return 0 if at least one team has less members than 'memcnt'
// else return 1 (even we have more members than memcnt, dunno is this bug <- FIXME)
int CheckMembers(float memcnt)
{
	gedict_t *p, *p2;
	float f1;
	int retVal = 1;
	char *s = "";

	maxPlayerCount = 0;
	for (p = world; (p = find_plr(p));)
	{
		p->k_flag = 0;
	}

	for (p = world; (p = find_plr(p));)
	{
		if (p->k_flag)
		{
			continue;
		}

		p->k_flag = 1;
		f1 = 1;

		if (!strnull(s = getteam(p)))
		{
			for (p2 = p; (p2 = find_plr(p2));)
			{
				if (streq(s, getteam(p2)))
				{
					p2->k_flag = 1;
					f1++;
				}
			}
		}

		maxPlayerCount = max(f1, maxPlayerCount);
		if (f1 < memcnt)
		{
			retVal = 0;
		}
	}

	return retVal;
}

extern demo_marker_t demo_markers[];
extern int demo_marker_index;

void ListDemoMarkers(void)
{
	int i = 0;

	if (!demo_marker_index)
	{
		return;
	}

	G_bprint(2, "%s:\n\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
				redtext("Demo markers"));

	for (i = 0; i < demo_marker_index; ++i)
	{
		int total = (int)(demo_markers[i].time - match_start_time);

		G_bprint(2, "%s: %d:%02d \220%s\221\n", redtext("Time"), (total / 60), (total % 60),
					demo_markers[i].markername);
	}

	G_bprint(2, "\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n");
}

void EM_on_MatchEndBreak(int isBreak)
{
	gedict_t *p;

	for (p = world; (p = find_client(p));)
	{
		if (isBreak)
		{
			on_match_break(p);
		}
		else
		{
			on_match_end(p);
		}
	}
}

// WARNING: if we are skip log, we are also delete demo

void EndMatch(float skip_log)
{
	gedict_t *p;
	int old_match_in_progress = match_in_progress;
	char *tmp;
	float f1;
	qbool is_real_match_end = !isHoonyModeAny() || HM_is_game_over();
	qbool f_modified_done = false, f_ruleset_done = false, f_version_done = false;
	char *matchtag = ezinfokey(world, "matchtag");
	qbool has_matchtag = matchtag != NULL && matchtag[0];

	if (match_over || !match_in_progress)
	{
		return;
	}

	match_over = 1;
	k_berzerk = 0;

	remove_projectiles();

	// s: zero the flag
	k_sudden_death = 0;

	if (!strnull(tmp = cvar_string("_k_host")))
	{
		trap_cvar_set("hostname", tmp); // restore host name at match end
	}

	trap_lightstyle(0, "m");

	// spec silence
	{
		int fpd = iKey(world, "fpd");

		fpd = fpd & ~64;
		localcmd("serverinfo fpd %d\n", fpd);

		cvar_fset("sv_spectalk", 1);
	}

	if (isCA())
	{
		CA_MatchBreak();
	}

	if (isHoonyModeAny())
	{
		G_bprint(2, "The point is over\n");
		HM_point_stats();
	}
	else if (deathmatch)
	{
		G_bprint(2, "The match is over\n");
	}

	EM_CorrectStats();

	if (k_bloodfest)
	{
		extern void bloodfest_stats(void);

		// remove any powerup left
		for (p = world; (p = nextent(p));)
		{
			if (streq(p->classname, "item_artifact_invulnerability")
					|| streq(p->classname, "item_artifact_invisibility")
					|| streq(p->classname, "item_artifact_super_damage"))
			{
				ent_remove(p);
				continue;
			}
		}

		bloodfest_stats();
	}

	if ( /* skip_log || */!deathmatch)
	{
		;
	}
	else
	{
		if (is_real_match_end && !isRACE())
		{
			ListDemoMarkers();
		}

		if (is_real_match_end)
		{
			if (!isCA())
			{
				MatchEndStats();
			}

			lastscore_add(); // save game result somewhere, so we can show it later
		}
	}

	if (is_real_match_end)
	{
		for (p = world; (p = find(p, FOFCLSN, "ghost"));)
		{
			ent_remove(p);
		}
	}

	StopTimer(skip_log); // WARNING: if we are skip log, we are also delete demo

	if (is_real_match_end)
	{
		for (f1 = 666; k_teamid >= f1; f1++)
		{
			localcmd("localinfo %d \"\"\n", (int)f1); //removing key
		}

		for (f1 = 1; k_userid >= f1; f1++)
		{
			localcmd("localinfo %d \"\"\n", (int)f1); //removing key
		}
	}

	if (old_match_in_progress == 2)
	{
		for (p = world; (p = find_plr(p));)
		{
			p->ready = 0; // force players be not ready after match is end.

			if (has_matchtag && cvar("k_on_end_f_modified") && !f_modified_done)
			{
				stuffcmd(p, "say f_modified\n");
				f_modified_done = true;
			}
			if (has_matchtag && cvar("k_on_end_f_ruleset") && !f_ruleset_done)
			{
				stuffcmd(p, "say f_ruleset\n");
				f_ruleset_done = true;
			}
			if (has_matchtag && cvar("k_on_end_f_version") && !f_version_done)
			{
				stuffcmd(p, "say f_version\n");
				f_version_done = true;
			}
		}
	}

	EM_on_MatchEndBreak(skip_log);

	if (isHoonyModeAny())
	{
		qbool demomarker_logged = false;

		if (!HM_is_game_over())
		{
			match_over = 0;

			// All bots ready first
			for (p = world; (p = find_plr(p));)
			{
				if (p->isBot)
				{
					p->ready = true;
				}
			}

			for (p = world; (p = find_plr(p));)
			{
				if (!demomarker_logged)
				{
					stuffcmd(p, "//demomark 0 round-%2d\n", HM_current_point());
					demomarker_logged = true;
				}

				stuffcmd(p, "ready\n");
			}
		}
		else
		{
			for (p = world; (p = find_plr(p));)
			{
				stuffcmd(p, "hmstats\n");
			}

			StopLogs();
			NextLevel();
		}
	}
	else
	{
		StopLogs();
		NextLevel();
	}

	// allow ready/break in bloodfest without map reloading.
	if (k_bloodfest || isCA())
	{
		match_over = 0;
	}

	if (is_real_match_end)
	{
		g_matchstarttime = 0;
	}
}

void SaveOvertimeStats(void)
{
	gedict_t *p;

	if (k_overtime)
	{
		for (p = world; (p = find_plr(p));)
		{
			// save overtime stats
			p->ps.ot_a = (int)p->s.v.armorvalue;
			p->ps.ot_items = p->s.v.items; // float
			p->ps.ot_h = (int)p->s.v.health;
		}
	}
}

void CheckOvertime(void)
{
	// SM_PrepareShowscores() is called in StartMatch(), but in matchless mode this happens when the first player joins.
	// Therefore, teams != 2 yet and the function doesn't do anything.
	// This causes matchless CTF to go into perpetual overtime, since "sc = get_scores1() - get_scores2();" calculated below will always return 0.
	// So, we call this function here to handle this scenario.
	gedict_t *timer, *ed1, *ed2 ;
	int teams, players ;
	int sc;
	int k_mb_overtime;
	int k_exttime; 

	SM_PrepareShowscores();

	ed1 = get_ed_scores1(); ed2 = get_ed_scores2();
	teams = CountTeams();
	players = CountPlayers();
	sc = get_scores1() - get_scores2();
	k_mb_overtime = cvar("k_overtime");
	k_exttime = bound(1, cvar("k_exttime"), 999); // at least some reasonable values

	// In hoonymode the round is timing out, not the match - we're effectively always in suddendeath mode
	if (isHoonyModeAny())
	{
		HM_draw();

		return;
	}

	// If 0 no overtime, 1 overtime, 2 sudden death
	// And if its neither then well we exit
	if (!k_mb_overtime || ((k_mb_overtime != 1) && (k_mb_overtime != 2) && (k_mb_overtime != 3)))
	{
		EndMatch(0);

		return;
	}

	// Overtime.
	// Ok we have now decided that the game is ending, so decide overtime wise here what to do.

	if ((isDuel() || isFFA()) && ed1 && ed2)
	{
		sc = ed1->s.v.frags - ed2->s.v.frags;
	}

//	if( k_matchLess ) {
//		k_mb_overtime = 0; // no overtime in matchLess mode
//	}
//	else

	if (lgc_enabled())
	{
		k_mb_overtime = 0; // no overtime in lgc mode
	}

	if ((isTeam() || isCTF()) && (teams != 2))
	{
		k_mb_overtime = 0; // no overtime in case of less than 2 or more than 2 teams
	}
	else if (((isDuel() || isFFA()) && ed1 && ed2) // duel or ffa
			|| ((isTeam() || isCTF()) && (teams == 2) && (players > 1))) // Handle a 2v2 or above team game or 1v1 CTF
	{
		if (((k_mb_overtime == 3) && abs(sc) > 1) // tie-break overtime allowed with one frag difference (c) ktpro
			|| ((k_mb_overtime != 3) && abs(sc) > 0)) // time based or sudden death overtime allowed with zero frag difference
		{
			k_mb_overtime = 0;
		}
	}
	else
	{
		k_mb_overtime = 0;
	}

	if (!k_mb_overtime)
	{
		EndMatch(0);

		return;
	}

	k_overtime = k_mb_overtime;
	SaveOvertimeStats();

	G_bprint(2, "time over, the game is a draw\n");

	if (k_overtime == 1)
	{
		// Ok its increase time
		self->cnt = k_exttime;
		self->cnt2 = 60;
		localcmd("serverinfo status \"%d min left\"\n", (int)self->cnt);

		G_bprint(2, "\220%s\221 minute%s overtime follows\n", dig3(k_exttime), count_s(k_exttime));
		self->s.v.nextthink = g_globalvars.time + 1;
		match_end_time += self->cnt * 60;
	}
	else if (k_overtime == 2)
	{
		k_sudden_death = SD_NORMAL;
		match_end_time = 0;
	}
	else
	{
		k_sudden_death = SD_TIEBREAK;
		match_end_time = 0;
	}

	if (k_sudden_death)
	{
		G_bprint(2, "%s %s\n", SD_type_str(), redtext("overtime begins"));

		// added timer removal at sudden death beginning
		for (timer = world; (timer = find(timer, FOFCLSN, "timer"));)
		{
			ent_remove(timer);
		}
	}
}

// Called every second during a match. cnt = minutes, cnt2 = seconds left.
// Tells the time every now and then.
void TimerThink(void)
{
	gedict_t *p;
	int idle_time;
	float f1, f2;

	//if in matchless mode, check that the user hasn't exceeded k_matchless_max_idle_time
	if (k_matchLess && CountPlayers() && match_in_progress && k_matchLess_idle_time)
	{
		for (p = world; (p = find_plr(p));)
		{
			idle_time = (int)(g_globalvars.time - p->attack_finished);
			if (idle_time > k_matchLess_idle_time)
			{
				G_sprint(
						p,
						2,
						"You were forced to reconnect as spectator by exceeding the maximum idle time of %i seconds.\n",
						k_matchLess_idle_time);
				stuffcmd_flags(p, STUFFCMD_IGNOREINDEMO, "spectator 1\n");
				if (!strnull(ezinfokey(p, "Qizmo")))
				{
					stuffcmd_flags(p, STUFFCMD_IGNOREINDEMO,
									"say ,:dis\nwait;wait;wait; say ,:reconnect\n");
				}
				else
				{
					stuffcmd_flags(p, STUFFCMD_IGNOREINDEMO, "disconnect\nwait;wait;reconnect\n");
				}
			}
			else if (idle_time == k_matchLess_idle_warn)
			{
				G_sprint(
						p,
						2,
						"\207%s You will be forced to spectate if you do not fire within %i seconds!\n",
						redtext("WARNING:"), (k_matchLess_idle_time - k_matchLess_idle_warn));
			}
		}
	}

	if (!k_matchLess && !CountPlayers())
	{
		EndMatch(1);

		return;
	}

	if (k_sudden_death)
	{
		return;
	}

	if ((self->k_teamnum < g_globalvars.time) && !k_checkx)
	{
		k_checkx = 1; // global which set to true when some time spend after match start
	}

	// berzerk
	if (k_berzerktime != 0)
	{
		f1 = k_berzerktime;
		f2 = floor(f1 / 60);
		f1 = f1 - (f2 * 60);
		f2 = f2 + 1;
		if ((self->cnt2 == f1) && (self->cnt == f2))
		{
			G_bprint(2, "BERZERK!!!!\n");
			trap_lightstyle(0, "ob");
			k_berzerk = 1;

			for (p = world; (p = find_plr(p));)
			{
				if (strcmp(p->netname, "") && p->s.v.health > 0)
				{
					p->s.v.items = (int)p->s.v.items | (IT_QUAD | IT_INVULNERABILITY);
					p->super_time = 1;
					p->super_damage_finished = g_globalvars.time + 3600;
					p->invincible_time = 1;
					p->invincible_finished = g_globalvars.time + 2;
				}
			}
		}
	}

	(self->cnt2)--;

	if (!self->cnt2)
	{
		self->cnt2 = 60;
		self->cnt -= 1;

		localcmd("serverinfo status \"%d min left\"\n", (int)self->cnt);

		if (!self->cnt)
		{
			CheckOvertime();

			return;
		}

		G_bprint(2, "\220%s\221 minute%s remaining\n", dig3(self->cnt), count_s(self->cnt));

		self->s.v.nextthink = g_globalvars.time + 1;

		if (k_showscores && !isCA())
		{
			if ((current_umode < um2on2on2) || (current_umode > um4on4on4))
			{
				int sc = get_scores1() - get_scores2();

				if (sc)
				{
					G_bprint(2, "%s \220%s\221 leads by %s frag%s\n", redtext("Team"),
								cvar_string((sc > 0 ? "_k_team1" : "_k_team2")), dig3(abs((int)sc)),
								count_s(abs((int)sc)));
				}
				else
				{
					G_bprint(2, "The game is currently a tie\n");
				}
			}
			else
			{
				// if the current UserMode is 2on2on2, 3on3on3, 4on4on4, we have 3 teams
				int s1 = get_scores1();
				int s2 = get_scores2();
				int s3 = get_scores3();
				int sc;

				if ((s1 > s2) && (s1 > s3))
				{
					// Team 1 is leading
					if (s2 > s3)
					{
						sc = get_scores1() - get_scores2();
					}
					else
					{
						sc = get_scores1() - get_scores3();
					}

					G_bprint(2, "%s \220%s\221 leads by %s frag%s\n", redtext("Team"),
								cvar_string("_k_team1"), dig3(abs((int)sc)),
								count_s(abs((int)sc)));
				}
				else if ((s2 > s1) && (s2 > s3))
				{
					// Team 2 is leading
					if (s1 > s3)
					{
						sc = get_scores2() - get_scores1();
					}
					else
					{
						sc = get_scores2() - get_scores3();
					}

					G_bprint(2, "%s \220%s\221 leads by %s frag%s\n", redtext("Team"),
								cvar_string("_k_team2"), dig3(abs((int)sc)),
								count_s(abs((int)sc)));
				}
				else if ((s3 > s1) && (s3 > s2))
				{
					// Team 3 is leading
					if (s1 > s2)
					{
						sc = get_scores3() - get_scores1();
					}
					else
					{
						sc = get_scores3() - get_scores3();
					}

					G_bprint(2, "%s \220%s\221 leads by %s frag%s\n", redtext("Team"),
								cvar_string("_k_team3"), dig3(abs((int)sc)),
								count_s(abs((int)sc)));
				}
				else
				{
					G_bprint(2, "The game is currently a tie\n");
				}
			}
		}

		return;
	}

	if ((self->cnt == 1) && ((self->cnt2 == 30) || (self->cnt2 == 15) || (self->cnt2 <= 10)))
	{
		G_bprint(2, "\220%s\221 second%s\n", dig3(self->cnt2), count_s(self->cnt2));
	}

	self->s.v.nextthink = g_globalvars.time + 1;
}

// remove/add some items from map regarding dmm and game mode
void SM_PrepareMap(void)
{
	gedict_t *p;

	if (isCTF())
	{
		SpawnRunes(cvar("k_ctf_runes"));
	}

	// this must be removed in any cases
	remove_projectiles();

	for (p = world; (p = nextent(p));)
	{
		// going for the if content record..
		if (isRA() || isRACE() || ((deathmatch == 4) && (cvar("k_instagib") || cvar("k_midair")))
				|| cvar("k_noitems") || k_bloodfest)
		{
			if (streq(p->classname, "weapon_nailgun") || streq(p->classname, "weapon_supernailgun")
					|| streq(p->classname, "weapon_supershotgun")
					|| streq(p->classname, "weapon_rocketlauncher")
					|| streq(p->classname, "weapon_grenadelauncher")
					|| streq(p->classname, "weapon_lightning") || streq(p->classname, "item_shells")
					|| streq(p->classname, "item_spikes") || streq(p->classname, "item_rockets")
					|| streq(p->classname, "item_cells") || streq(p->classname, "item_health")
					|| streq(p->classname, "item_armor1") || streq(p->classname, "item_armor2")
					|| streq(p->classname, "item_armorInv")
					|| streq(p->classname, "item_artifact_invulnerability")
					|| streq(p->classname, "item_artifact_envirosuit")
					|| streq(p->classname, "item_artifact_invisibility")
					|| streq(p->classname, "item_artifact_super_damage"))
			{
				soft_ent_remove(p);
				continue;
			}
		}

		if (deathmatch == 2)
		{
			if (streq(p->classname, "item_armor1") || streq(p->classname, "item_armor2")
					|| streq(p->classname, "item_armorInv"))
			{
				soft_ent_remove(p);
				continue;
			}
		}

		if (deathmatch >= 4)
		{
			if (streq(p->classname, "weapon_nailgun") || streq(p->classname, "weapon_supernailgun")
					|| streq(p->classname, "weapon_supershotgun")
					|| streq(p->classname, "weapon_rocketlauncher")
					|| streq(p->classname, "weapon_grenadelauncher")
					|| streq(p->classname, "weapon_lightning"))
			{ // no weapons for any of this deathmatches (4 or 5)
				soft_ent_remove(p);
				continue;
			}

			if (deathmatch == 4)
			{
				if (streq(p->classname, "item_shells") || streq(p->classname, "item_spikes")
						|| streq(p->classname, "item_rockets") || streq(p->classname, "item_cells")
						|| (streq(p->classname, "item_health") && ((int)p->s.v.spawnflags & H_MEGA)))
				{ // no weapon ammo and megahealth for dmm4
					soft_ent_remove(p);
					continue;
				}
			}
		}

		if (k_killquad && streq(p->classname, "item_artifact_super_damage"))
		{	// no normal quad in killquad mode.
			soft_ent_remove(p);
			continue;
		}
	}

	ClearBodyQue(); // hide corpses
}

// put clients in server and reset some params
static void SM_PrepareClients(void)
{
	int hdc, i, j, player_count = 0;
	char *pl_team;
	gedict_t *p, *temp;
	gedict_t *players[MAX_CLIENTS];
	qbool hoonymode_reset = isHoonyModeAny() && HM_current_point() > 0;

	k_teamid = 666;
	localcmd("localinfo 666 \"\"\n");
	trap_executecmd(); // <- this really needed

	initial_match_spawns = true;

	for (p = world; (p = find_plr(p));)
	{
		p->socdDetectionCount = 0;
		p->socdValidationCount = 0;
		p->fStrafeChangeCount = 0;
		p->fFramePerfectStrafeChangeCount = 0;
		p->fLastSideMoveSpeed = 0;
		p->matchStrafeChangeCount = 0;
		p->matchPerfectStrafeCount = 0;
		p->totalStrafeChangeCount = 0;
		p->totalPerfectStrafeCount = 0;
		p->nullStrafeCount = 0;
	}

	for (p = world; (p = find_plr(p));)
	{
		players[player_count++] = p;
		p->leavemealone = false;		// can't have this enabled during match
	}

	for (i = player_count - 1; i > 0; i--)
	{
		j = rand() % (i + 1);
		temp = players[i];
		players[i] = players[j];
		players[j] = temp;
	}

	for (j = 0; j < player_count; j++)
	{
		p = players[j];
		if (!k_matchLess)
		{
			// skip setup k_teamnum in matchLess mode
			pl_team = getteam(p);
			p->k_teamnum = 0;

			if (!strnull(pl_team))
			{
				i = 665;

				while (k_teamid > i && !p->k_teamnum)
				{
					i++;

					if (streq(pl_team, ezinfokey(world, va("%d", i))))
					{
						p->k_teamnum = i;
					}
				}

				if (!p->k_teamnum)
				{
					// team not found in localinfo, so put it in
					i++;
					p->k_teamnum = k_teamid = i;
					localcmd("localinfo %d \"%s\"\n", i, pl_team);
					trap_executecmd(); // <- this really needed
				}
			}
			else
				p->k_teamnum = 666;
		}

		if (!hoonymode_reset)
		{
			p->kills = p->suicides = p->friendly = p->deaths = p->s.v.frags = 0;
		}

		hdc = p->ps.handicap; // save player handicap

		if (!hoonymode_reset)
		{
			memset((void*) &(p->ps), 0, sizeof(p->ps)); // clear player stats

			WS_Reset(p); // force reset "new weapon stats"
		}

		p->ps.handicap = hdc; // restore player handicap

		if (isRA())
		{
			if (isWinner(p) || isLoser(p))
			{
				setfullwep(p);
			}

			continue;
		}

		// ignore k_respawn() in case of coop unless bloodfest
		if (!deathmatch && !k_bloodfest)
		{
			continue;
		}

		// ignore k_respawn() in case of CA
		if (isCA())
		{
			continue;
		}

		// ignore k_respawn() in case of race mode
		if (isRACE())
		{
			continue;
		}

		k_respawn(p, false);

		p->k_pauseRequests = MAX_PAUSE_REQUESTS;
	}

	initial_match_spawns = false;

	// now that every player is spawned, do sounds & fog effect
	SM_ExecuteQueuedSpawnEffects();
}

// This creates visual effect & sound effects for players at start of match
// See bug #115... if 2 players spawn next to each other, the first player hears the second, but the second only hears the first based on final location during pre-war
static void SM_ExecuteQueuedSpawnEffects(void)
{
	gedict_t *p;

	for (p = world; (p = find_plr(p));)
	{
		if (p->spawn_effect_queued)
		{
			vec3_t fog_org;

			trap_makevectors(p->s.v.angles);
			VectorMA(p->s.v.origin, 20, g_globalvars.v_forward, fog_org);
			spawn_tfog(fog_org);

			play_teleport(p);

			p->spawn_effect_queued = false;
		}
	}
}

void SM_PrepareShowscores(void)
{
	gedict_t *p;
	char *team1 = "";
	char *team2 = "";
	char *team3 = "";

	if (k_matchLess && !isCTF()) // skip this in matchLess mode, unless CTF matchless (otherwise causes unlimited overtime because "sc" is always = 0 in CheckOvertime())
	{
		return;
	}

	if (((!isTeam() && !isCTF())
			|| ((CountRTeams() != 2) && (CountRTeams() != 3)))
			&& !(isCTF() && k_matchLess)) // we need 2 ready teams, unless CTF matchless
	{
		return;
	}

	if ((p = find_plr(world)))
	{
		team1 = getteam(p);
	}

	if (strnull(team1))
	{
		return;
	}

	while ((p = find_plr(p)))
	{
		team2 = getteam(p);

		if (strneq(team1, team2))
		{
			break;
		}
	}

	if (strnull(team2) || streq(team1, team2))
	{
		return;
	}

	k_showscores = 1;

	cvar_set("_k_team1", team1);
	cvar_set("_k_team2", team2);
	if ((current_umode >= um2on2on2) && (current_umode <= um4on4on4))
	{
		// let's see if there is a player with a 3rd team
		while ((p = find_plr(p)))
		{
			team3 = getteam(p);

			if (strneq(team1, team3) && strneq(team2, team3))
			{
				break;
			}
		}

		if (strnull(team3) || streq(team1, team3) || streq(team2, team3))
		{
			return;
		}

		cvar_set("_k_team3", team3);
	}
}

void SM_PrepareHostname(void)
{
	char *team1 = cvar_string("_k_team1");
	char *team2 = cvar_string("_k_team2");

	cvar_set("_k_host", cvar_string("hostname")); // save host name at match start

	if (k_showscores && !strnull(team1) && !strnull(team2))
	{
		if ((current_umode < um2on2on2) || (current_umode > um4on4on4))
		{
			cvar_set("hostname", va("%s (%.4s vs. %.4s)\207", cvar_string("hostname"), team1, team2));
		}
		else
		{
			char *team3 = cvar_string("_k_team3");

			cvar_set("hostname", va("%s (%.4s vs. %.4s vs. %.4s)\207", cvar_string("hostname"), team1, team2, team3));
		}
	}
	else
	{
		cvar_set("hostname", va("%s\207", cvar_string("hostname")));
	}
}

void SM_on_MatchStart(void)
{
	gedict_t *p;

	for (p = world; (p = find_client(p));)
	{
		on_match_start(p);
	}
}

void SM_on_Countdown(void)
{
	gedict_t *p;

	for (p = world; (p = find_client(p));)
	{
		on_countdown(p);
	}
}

// Reset player frags and start the timer.
void HideSpawnPoints(void);

void StartMatch(void)
{
	char date[64];

	// reset bloodfest vars.
	bloodfest_reset();

	k_nochange = 0;
	k_showscores = 0;
	k_standby = 0;
	k_checkx = 0;

	k_userid = 1;
	localcmd("localinfo 1 \"\"\n");
	trap_executecmd(); // <- this really needed

	first_rl_taken = false; // no one took rl yet

	SM_PrepareMap(); // remove/add some items from map regardind with dmm and game mode

	HideSpawnPoints();

	match_start_time = g_globalvars.time;
	g_matchstarttime = (int)(g_globalvars.time * 1000);
	match_in_progress = 2;

	// Disable berzerk at start
	k_berzerk = 0;
	// Get berzerk mode time
	if (cvar("k_bzk"))
	{
		k_berzerktime = cvar("k_btime");
	}
	else
	{
		k_berzerktime = 0;
	}

	lastTeamLocationTime = -TEAM_LOCATION_UPDATE_TIME; // update on next frame

	remove_specs_wizards(); // remove wizards

	SM_PrepareClients(); // put clients in server and reset some params

	if (!QVMstrftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S %Z", 0))
	{
		date[0] = 0;
	}

	if (deathmatch && (!isHoonyModeAny() || (HM_current_point() == 0)))
	{
		if (date[0])
		{
			G_bprint(2, "matchdate: %s\n", date);
		}

		if (!k_matchLess || cvar("k_matchless_countdown"))
		{
			G_bprint(2, "%s\n", redtext("The match has begun!"));
		}
	}

// spec silence
	{
		int fpd = iKey(world, "fpd");
		int k_spectalk = (coop ? 1 : bound(0, cvar("k_spectalk"), 1));
		cvar_fset("sv_spectalk", k_spectalk);

		fpd = (k_spectalk) ? (fpd & ~64) : (fpd | 64);

		localcmd("serverinfo fpd %d\n", fpd);
	}

	self->k_teamnum = g_globalvars.time + 3; //dirty i know, but why waste space?
											 // FIXME: waste space, but be clean

	if (isHoonyModeAny() && HM_timelimit() > 0)
	{
		int minutes = bound(0, HM_timelimit() / 60, 9999);
		int seconds = HM_timelimit() % 60;

		if (seconds)
		{
			++minutes;
		}
		else
		{
			seconds = 60;
		}

		self->cnt = minutes;
		self->cnt2 = seconds;
		localcmd("serverinfo status \"%d min left\"\n", minutes);
		match_end_time = match_start_time + (minutes - 1) * 60 + seconds;
	}
	else
	{
		self->cnt = bound(0, timelimit, 9999);
		self->cnt2 = 60;
		localcmd("serverinfo status \"%d min left\"\n", (int)timelimit);
		match_end_time = match_start_time + self->cnt * 60;
	}

	self->think = (func_t) TimerThink;
	self->s.v.nextthink = g_globalvars.time + 1;

	SM_PrepareShowscores();

	SM_PrepareHostname();

	SM_PrepareTeamsStats();

	SM_PrepareCA();

	SM_on_MatchStart();

	ClearDemoMarkers();

	StartLogs();

#ifdef BOT_SUPPORT
	BotsMatchStart();
#endif

	if (!self->cnt)
	{
		ent_remove(self); // timelimit == 0, so match will end no due to timelimit but due to fraglimit or something
	}

	if (isRACE())
	{
		race_match_start();
	}

	stuffcmd_flags(world, STUFFCMD_DEMOONLY, "//ktx matchstart\n");
	cvar_fset("qtv_sayenabled", 0);
}

// just check if someone using handicap
static qbool handicap_in_use(void)
{
	gedict_t *p;
	int from;

	for (from = 0, p = world; (p = find_plrghst(p, &from));)
	{
		if (GetHandicap(p) != 100)
		{
			return true;
		}
	}

	return false;
}

void PersonalisedCountdown(char *baseText)
{
	char text[1024];
	gedict_t *p;

	for (p = world; (p = find_plr(p)); /**/)
	{
		strlcpy(text, baseText, sizeof(text));

		if (HM_current_point_type() == HM_PT_SET)
		{
			strlcat(text, redtext("* Set Point *"), sizeof(text));
			strlcat(text, "\n\n", sizeof(text));
		}

		if (p->k_hoony_new_spawn && !strnull(p->k_hoony_new_spawn->targetname))
		{
			strlcat(text, va("Next %8.8s\n", redtext(p->k_hoony_new_spawn->targetname)),
					sizeof(text));

			if (timelimit)
			{
				strlcat(text, va("%s %3s\n", "Timelimit", dig3(timelimit)), sizeof(text));
			}
			else if (isHoonyModeDuel() && world->hoony_timelimit)
			{
				int minutes = world->hoony_timelimit / 60;
				int seconds = world->hoony_timelimit % 60;

				if (minutes == 0)
				{
					strlcat(text, va("%s %3ss\n", "Duration", dig3(seconds)), sizeof(text));
				}
				else if (seconds == 0)
				{
					strlcat(text, va("%s %3sm\n", "Duration", dig3(minutes)), sizeof(text));
				}
				else
				{
					strlcat(text, va("%s %1s:%2s\n", "Duration", dig3(minutes), dig3(seconds)),
							sizeof(text));
				}
			}

			if (!strnull(world->hoony_defaultwinner))
			{
				if (streq(p->k_hoony_new_spawn->targetname, world->hoony_defaultwinner))
				{
					strlcat(text, va("Draw %s\n", redtext(" you win")), sizeof(text));
				}
				else
				{
					strlcat(text, va("Draw %s\n", redtext("you lose")), sizeof(text));
				}
			}
		}

		G_centerprint(p, "%s", text);
	}
}

void PrintCountdown(int seconds)
{
// Countdown: seconds
//
//
// EQL semifinal
//
// Deathmatch	x
// Mode			D u e l | T e a m | F F A | C O O P | BLOODFST | C T F | RA | CA
// Spawnmodel	KTX | bla bla bla // optional
// Antilag		On|Off
// NoItems		On // optional
// Midair		On // optional
// Instagib		On // optional
// Yawnmode		On // optional
// Airstep		On // optional
// TmOverlay	On // optional
// Teamplay		x
// Timelimit	xx
// Fraglimit	xxx
// Overtime		xx		Overtime printout, supports sudden death display
// Powerups		On|Off|QPRS
// Dmgfrags		On // optional
// Noweapon

// Handicap in use // optional

	char text[1024] =
		{ 0 };
	char *mode = "";
	char *ot = "";
	char *nowp = "";
	char *matchtag = redtext(ezinfokey(world, "matchtag"));

	strlcat(text, va("%s: %2s\n\n\n", redtext("Countdown"), dig3(seconds)), sizeof(text));

//	if (matchtag[0]) {
//		strlcat(text, va("matchtag %s\n\n\n", matchtag), sizeof(text));
//	}
//	else {
//		strlcat(text, "no matchtag\n\n\n", sizeof(text));
//	}

	if (isHoonyModeDuel() && seconds <= 3)
	{
		PersonalisedCountdown(text);

		return;
	}

	// useless in RA
	if (!isRA() && !coop && !isRACE())
	{
		strlcat(text, va("%s %2s\n", "Deathmatch", dig3(deathmatch)), sizeof(text));
	}

	if (k_bloodfest)
	{
		mode = redtext("BLOODFST");
	}
	else if (coop)
	{
		mode = redtext("C O O P");
	}
	else if (isRA())
	{
		mode = redtext("RA");
	}
	else if (isCA())
	{
		if (cvar("k_clan_arena") == 2)
		{
			mode = redtext("Wipeout");	
		}
		else 
		{
			mode = redtext("CA");
		}
	}
	else if (isHoonyModeDuel())
	{
		mode = redtext("Hoony");
	}
	else if (lgc_enabled())
	{
		mode = redtext("LGC");
	}
	else if (isHoonyModeTDM())
	{
		mode = redtext("BlitzTDM");
	}
	else if (isRACE())
	{
		mode = redtext("R A C E");
	}
	else if (isDuel())
	{
		mode = redtext("D u e l");
	}
	else if (isTeam())
	{
		mode = redtext("T e a m");
	}
	else if (isFFA())
	{
		mode = redtext("F F A");
	}
	else if (isCTF())
	{
		mode = redtext("C T F");
	}
	else
	{
		mode = redtext("Unknown");
	}

	strlcat(text, va("%s %8s\n", "Mode", mode), sizeof(text));

	if (isCA())
	{
		strlcat(text, va("%s %3s\n", "RoundWins", dig3(CA_wins_required())), sizeof(text));
	}
	else if (isHoonyModeTDM() && HM_current_point())
	{
		strlcat(text, "\n", sizeof(text));
		strlcat(text, (char*) HM_round_explanation(), sizeof(text));
		strlcat(text, (char*) HM_series_explanation(), sizeof(text));
		strlcat(text, "\n", sizeof(text));
	}
	else if (isRACE())
	{
		if (race.round_number >= race.rounds)
		{
			strlcat(text, va("Round %7s\n", redtext("final")), sizeof(text));
		}

		strlcat(text, va("%s %9s\n", "Pts", race_scoring_system_name()), sizeof(text));
	}

	if (!(isHoonyModeTDM() && HM_current_point()))
	{
		//	if ( cvar( "k_spw" ) != 3 )
		if (!isRACE())
		{
			strlcat(text, va("%s %4s\n", "Respawns", respawn_model_name_short(cvar("k_spw"))),
					sizeof(text));
		}

		if (cvar("sv_antilag"))
		{
			strlcat(text, va("%s %5s\n", "Antilag", dig3((int)cvar("sv_antilag"))), sizeof(text));
		}

		if (cvar("k_noitems") && !isRACE())
		{
			strlcat(text, va("%s %5s\n", "NoItems", redtext("on")), sizeof(text));
		}

		if (cvar("k_midair"))
		{
			strlcat(text, va("%s %6s\n", "Midair", redtext("on")), sizeof(text));
		}

		if (cvar("k_instagib"))
		{
			strlcat(text, va("%s %4s\n", "Instagib", redtext("on")), sizeof(text));
		}

		if (k_yawnmode)
		{
			strlcat(text, va("%s %4s\n", "Yawnmode", redtext("on")), sizeof(text));
		}

		if (cvar("pm_airstep"))
		{
			strlcat(text, va("%s %5s\n", "Airstep", redtext("on")), sizeof(text));
		}

		vw_enabled = vw_available && cvar("k_allow_vwep") && cvar("k_vwep");
		if (vw_enabled && !isRACE())
		{
			strlcat(text, va("%s %8s\n", "VWep", redtext("on")), sizeof(text));
		}

		if (cvar("k_teamoverlay") && tp_num() && !isDuel() && !isRACE())
		{
			strlcat(text, va("%s %3s\n", "TmOverlay", redtext("on")), sizeof(text));
		}

		if (!isRA() && (isTeam() || isCTF()))
		{
			strlcat(text, va("%s %4s\n", "Teamplay", dig3(teamplay)), sizeof(text));
		}
	}

	if (isHoonyModeAny())
	{
		int hm_timelimit = HM_timelimit();

		if (hm_timelimit)
		{
			int minutes = hm_timelimit / 60;
			int seconds = hm_timelimit % 60;

			if (minutes == 0)
			{
				strlcat(text, va("%s %3ss\n", "Duration", dig3(seconds)), sizeof(text));
			}
			else if (seconds == 0)
			{
				strlcat(text, va("%s %3sm\n", "Duration", dig3(minutes)), sizeof(text));
			}
			else
			{
				strlcat(text, va("%s %1s:%2s\n", "Duration", dig3(minutes), dig3(seconds)),
						sizeof(text));
			}
		}

		if (HM_rounds())
		{
			strlcat(text,
					va(" %s     %2s/%2s\n", "Round", dig3(HM_current_point() + 1),
						dig3(HM_rounds())),
					sizeof(text));
		}
	}
	else if (timelimit)
	{
		strlcat(text, va("%s %3s\n", "Timelimit", dig3(timelimit)), sizeof(text));
	}

	if (!isHoonyModeAny() && fraglimit)
	{
		strlcat(text, va("%s %3s\n", "Fraglimit", dig3(fraglimit)), sizeof(text));
	}

	switch ((int)cvar("k_overtime"))
	{
		case 0:
			ot = redtext("off");
			break;

		case 1:
			ot = dig3(cvar("k_exttime"));
			break;

		case 2:
			ot = redtext("sd");
			break;

		case 3:
			ot = va("%s %s", dig3(tiecount()), redtext("tb"));
			break;

		default:
			ot = redtext("Unkn");
			break;
	}

	if (!isHoonyModeAny())
	{
		if (timelimit && cvar("k_overtime"))
		{
			strlcat(text, va("%s %4s\n", "Overtime", ot), sizeof(text));
		}
	}

	if (!isRA() && Get_Powerups() && strneq("off", Get_PowerupsStr()))
	{
		strlcat(text, va("%s %4s\n", "Powerups", redtext(Get_PowerupsStr())), sizeof(text));
	}

	if (cvar("k_dmgfrags"))
	{
		strlcat(text, va("%s %4s\n", "Dmgfrags", redtext("on")), sizeof(text));
	}

	if (cvar("k_freshteams"))
	{
		strlcat(text, va("%s %2s\n", "&c07fFreshTeams&r", redtext("on")), sizeof(text));
	}

	if (cvar("k_nosweep"))
	{
		strlcat(text, va("%s %5s\n", "NoSweep", redtext("on")), sizeof(text));
	}

	if ((deathmatch == 4) && !cvar("k_midair") && !cvar("k_instagib")
			&& !strnull(nowp = str_noweapon((int)cvar("k_disallow_weapons") & DA_WPNS)))
	{
		strlcat(text, va("\n%s %4s\n", "Noweapon", redtext(nowp[0] == 32 ? (nowp + 1) : nowp)),
				sizeof(text));
	}

	if (handicap_in_use())
	{
		strlcat(text, "\n"
				"Handicap in use\n",
				sizeof(text));
	}

	if (isHoonyModeAny())
	{
		if (((HM_current_point() % 2) == 0))
		{
			strlcat(text, va("\n%-13s\n", redtext("New spawns")), sizeof(text));
		}
		else
		{
			strlcat(text, va("\n%-13s\n", redtext("Switch spawns")), sizeof(text));
		}
	}

	if (tot_mode_enabled())
	{
		int weapon = FrogbotWeapon();
		strlcat(text, va("\nTribe of Tjernobyl mode %2s\n", redtext("on")), sizeof(text));
		strlcat(text, va("Break on death %11s\n",
			(int)cvar(FB_CVAR_BREAK_ON_DEATH) ? redtext("on") : redtext("off")),
			sizeof(text));
		strlcat(text, va("Bot weapon %15s\n", redtext(weapon ? WpName(weapon) : "random")), sizeof(text));
		strlcat(text, va("Bot health %15s\n", dig3(FrogbotHealth())), sizeof(text));
		strlcat(text, va("Bot skill %16s\n", dig3(FrogbotSkillLevel())), sizeof(text));
		strlcat(text, va("Quad damage multiplier %3s\n", dig3(FrogbotQuadMultiplier())), sizeof(text));
	}

	if (matchtag[0])
	{
		strlcat(text, va("\nmatchtag %s\n\n\n", matchtag), sizeof(text));
	}
	else
	{
		strlcat(text, "\nno matchtag\n\n\n", sizeof(text));
	}

	G_cp2all("%s", text);
}

qbool isCanStart(gedict_t *s, qbool forceMembersWarn)
{
	int k_lockmin = (isCA() || isRACE()) ? 2 : cvar("k_lockmin");
	int k_lockmax = (isCA() || isRACE()) ? 2 : cvar("k_lockmax");
	int k_membercount = cvar("k_membercount");
	int i = CountRTeams();
	int sub;
	int nready;
	char *txt = "";
	gedict_t *p;

	// no limits in RA
	if (isRA())
	{
		return true;
	}

	// some limits in duel...
	if (isDuel())
	{
		sub = CountPlayers() - 2;

		if (sub > 0) // we need two players in duel...
		{
			txt = va("Get rid of %d player%s!\n", sub, count_s(sub));

			if (s)
			{
				G_sprint(s, 2, "%s", txt);
			}
			else
			{
				G_bprint(2, "%s", txt);
			}

			return false;
		}
	}

	// no team/members rules limitation in non team game
	if (!isTeam() && !isCTF())
	{
		return true;
	}

	// below Team or CTF game modes and limits...

	for (p = world; (p = find_plr(p));)
	{
		if (strnull(getteam(p)))
		{
			txt = "Get rid of players with empty team\n";

			if (s)
			{
				G_sprint(s, 2, "%s", txt);
			}
			else
			{
				G_bprint(2, "%s", txt);
			}

			return false;
		}
	}

	if (i < k_lockmin)
	{
		sub = k_lockmin - i;
		txt = va("%d more team%s required!\n", sub, count_s(sub));

		if (s)
		{
			G_sprint(s, 2, "%s", txt);
		}
		else
		{
			G_bprint(2, "%s", txt);
		}

		return false;
	}

	if (i > k_lockmax)
	{
		sub = i - k_lockmax;
		txt = va("Get rid of %d team%s!\n", sub, count_s(sub));

		if (s)
		{
			G_sprint(s, 2, "%s", txt);
		}
		else
		{
			G_bprint(2, "%s", txt);
		}

		return false;
	}

	nready = 0;
	for (p = world; (p = find_plr(p));)
	{
		if (p->ready)
		{
			nready++;
		}
	}

	if (!CheckMembers(k_membercount))
	{
		if (!forceMembersWarn) // warn anyway if we want
		{
			if (nready != k_attendees && !s)
			{
				return false; // inform not in all cases, less annoying
			}
		}

		txt = va("%s %d %s\n"
					"%s\n",
					redtext("Server wants at least"), k_membercount,
					redtext("players in each team"), redtext("Waiting..."));

		if (s)
		{
			G_sprint(s, 2, "%s", txt);
		}
		else
		{
			G_bprint(2, "%s", txt);
		}

		return false;
	}

	if (isCTF() && !k_matchLess) // In matchless CTF, the "this map does not support CTF mode" would get spammed constantly in an unsupported map. So, don't bother returning false.
	{
		// can't really play ctf if map doesn't have flags
		gedict_t *rflag = find(world, FOFCLSN, "item_flag_team1");
		gedict_t *bflag = find(world, FOFCLSN, "item_flag_team2");

		if (!rflag || !bflag)
		{
			txt = "This map does not support CTF mode\n";

			if (s)
			{
				G_sprint(s, 2, "%s", txt);
			}
			else
			{
				G_bprint(2, "%s", txt);
			}

			return false;
		}
	}

	return true;
}

void standby_think(void)
{
	gedict_t *p;

	if (match_in_progress == 1 && !isRA())
	{

		k_standby = 1;

		for (p = world; (p = find_plr(p));)
		{
			if (!strnull(p->netname))
			{
				//set to ghost, 0.2 second before matchstart
				if (isHoonyModeDuel() && p->k_hoony_new_spawn)
				{
					// move viewpoint to selected spawn
					VectorCopy(p->k_hoony_new_spawn->s.v.origin, p->s.v.origin);
					p->s.v.origin[2] += 1;
					VectorCopy(p->k_hoony_new_spawn->s.v.angles, p->s.v.angles);
					p->s.v.fixangle = true;

					setnowep(p);
				}

				p->s.v.takedamage = 0;
				p->s.v.solid = 0;
				p->s.v.movetype = 0;
				p->s.v.modelindex = 0;
				p->model = "";

				// Relink after solid change to keep area lists consistent
				setorigin(p, PASSVEC3(p->s.v.origin));
			}
		}
	}

	ent_remove(self);
}

// Called every second during the countdown.
void TimerStartThink(void)
{
	gedict_t *p;

	k_attendees = CountPlayers();

	if (!isCanStart( NULL, true))
	{
		G_bprint(2, "Aborting...\n");

		StopTimer(1);

		return;
	}

	self->cnt2 -= 1;

	if (self->cnt2 == 1)
	{
		p = spawn();
		p->s.v.owner = EDICT_TO_PROG(world);
		p->classname = "standby_th";
		p->s.v.nextthink = g_globalvars.time + 0.8;
		p->think = (func_t) standby_think;

		if (isHoonyModeAny())
		{
			HM_reset_map();
		}
	}
	else if (self->cnt2 <= 0)
	{
		G_cp2all("%s", "");

		StartMatch();

		return;
	}

	PrintCountdown(self->cnt2);

	if (self->cnt2 < 6)
	{
		char *gr = redtext("Get ready");

		for (p = world; (p = find_client(p));)
		{
			if ((p->ct == ctPlayer) && !p->ready)
			{
				G_sprint(p, 2, "%s!\n", gr);
			}

			stuffcmd(p, "play buttons/switch04.wav\n");
		}
	}

	self->s.v.nextthink = g_globalvars.time + 1;
}

void ShowMatchSettings(void)
{
	int i;
	char *txt = "";

//	G_bprint(2, "Spawnmodel: %s\n", redtext( respawn_model_name( cvar( "k_spw" ) ) ));

// changed to print only if other than default

	if ((i = get_fair_pack()))
	{
		// Output the Fairpack setting here
		switch (i)
		{
			case 0:
				txt = "off";
				break;

			case 1:
				txt = "best weapon";
				break;

			case 2:
				txt = "last weapon fired";
				break;

			default:
				txt = "!Unknown!";
				break;
		}

		G_bprint(2, "Fairpacks setting: %s\n", redtext(txt));
	}

// print qizmo ( FPD ) settings
	if (!isHoonyModeAny() || HM_current_point() == 0)
	{
		i = iKey(world, "fpd");
		if (i & 170)
		{
			char buf[256] =
				{ 0 };

			if (i & 2)
			{
				strlcat(buf, " timer", sizeof(buf));
			}

			if (i & 8)
			{
				strlcat(buf, " lag", sizeof(buf));
			}

			if (i & 32)
			{
				strlcat(buf, " enemy", sizeof(buf));
			}

			if (i & 128)
			{
				strlcat(buf, " point", sizeof(buf));
			}

			G_bprint(2, "QiZmo:%s disabled\n", redtext(buf));
		}
	}
}

// duel_dag_vs_zu-zu[dm3]
// team_no!_vs_fom[dm3]
// ctf_no!_vs_fom[dm3]
// ffa_10[dm3] // where 10 is count of players
// ra_10[dm3] // where 10 is count of players
// unknown_10[dm3] // where 10 is count of players

char* CompilateDemoName(void)
{
	static char demoname[60];
	char date[128], *fmt;
	char teams[MAX_CLIENTS][MAX_TEAM_NAME];

	int i;
	int players;
	gedict_t *p;
	char *name, *vs;

	demoname[0] = 0;
	if (isRA())
	{
		strlcat(demoname, va("ra_%d", (int)CountPlayers()), sizeof(demoname));
	}
	else if (isCA())
	{
		if (cvar("k_clan_arena") == 1)
		{
			strlcat(demoname, "ca", sizeof(demoname));
		}
		else
		{
			strlcat(demoname, "wipeout", sizeof(demoname));
		}

		getteams(teams);
		
		for (vs = "_", i = 0; i < MAX_CLIENTS; i++)
		{
			if (strnull(teams[i]))
			{
				break;
			}

			strlcat(demoname, vs, sizeof(demoname));
			strlcat(demoname, teams[i], sizeof(demoname));
			vs = "_vs_";
		}
	}
	else if (isRACE() && !race_match_mode())
	{
		players = 0;

		strlcat(demoname, "race", sizeof(demoname));
		for (vs = "_", p = world; (p = find_plr(p));)
		{
			if (strnull(name = getname(p)) || !(p->racer))
			{
				continue;
			}

			if (players < 2)
			{
				strlcat(demoname, vs, sizeof(demoname));
				strlcat(demoname, name, sizeof(demoname));
			}
			else if (players == 2)
			{
				strlcat(demoname, vs, sizeof(demoname));
				strlcat(demoname, "others", sizeof(demoname));
			}

			++players;
		}
	}
	else if (isDuel())
	{
		strlcat(demoname, "duel", sizeof(demoname));
		if (isRACE())
		{
			strlcat(demoname, "_race", sizeof(demoname));
		}

		if (cvar("k_midair"))
		{
			strlcat(demoname, "_midair", sizeof(demoname));
		}

		if (cvar("k_instagib"))
		{
			strlcat(demoname, "_instagib", sizeof(demoname));
		}

		for (vs = "_", p = world; (p = find_plr(p));)
		{
			if (strnull(name = getname(p)))
			{
				continue;
			}
			if (isRACE() && !(p->race_participant))
			{
				continue;
			}

			strlcat(demoname, vs, sizeof(demoname));
			strlcat(demoname, name, sizeof(demoname));
			vs = "_vs_";
		}
	}
	else if (isTeam() || isCTF())
	{
		char teams[MAX_CLIENTS][MAX_TEAM_NAME];
		int cnt = getteams(teams);
		int clt = cvar("maxclients"); //CountPlayers();

		// guess is this XonX
		if ((clt > 1) && (cnt > 1) && !(clt % cnt))
		{
			clt /= cnt; // yes
		}
		else
		{
			clt = 0; // no
		}

		if (isTeam())
		{
			const char *strCurrentUmode;

			// The parameter is (current_umode-1), because the UserModes_t enum has `umUnknown` as first element,
			// but the um_list[] array doesn't have an 'empty' first row
			strCurrentUmode = um_name_byidx(current_umode-1);
			if (strCurrentUmode != NULL)
			{
				strlcat(demoname, (char *)strCurrentUmode, sizeof(demoname));
			}
			else
			{
				// Something is wrong with current_umode. Let's use the legacy format
				strlcat(demoname, (clt ? va("%don%d", clt, clt) : "team"), sizeof(demoname));
			}
		}
		else
		{
			strlcat(demoname, "ctf", sizeof(demoname));
		}

		if (isRACE())
		{
			strlcat(demoname, "_race", sizeof(demoname));
		}

		for (vs = "_", i = 0; i < MAX_CLIENTS; i++)
		{
			if (strnull(teams[i]))
			{
				break;
			}

			strlcat(demoname, vs, sizeof(demoname));
			strlcat(demoname, teams[i], sizeof(demoname));
			vs = "_vs_";
		}
	}
	else if (isFFA())
	{
		if (isRACE())
		{
			strlcat(demoname, "race_", sizeof(demoname));
		}

		strlcat(demoname, va("ffa_%d", (int)CountPlayers()), sizeof(demoname));
	}
	else
	{
		if (isRACE())
		{
			strlcat(demoname, "race_", sizeof(demoname));
		}

		strlcat(demoname, va("unknown_%d", (int)CountPlayers()), sizeof(demoname));
	}

	if (isRACE())
	{
		strlcat(demoname, va("[%s_r%02d]", mapname, race.active_route),
				sizeof(demoname));
	}
	else
	{
		strlcat(demoname, va("[%s]", mapname), sizeof(demoname));
	}

	fmt = cvar_string("k_demoname_date");

	if (!strnull(fmt) && QVMstrftime(date, sizeof(date), fmt, 0))
	{
		strlcat(demoname, date, sizeof(demoname));
	}

	return demoname;
}

void StartDemoRecord(void)
{
	char *demoname;

	// extralog should be set by easyrecord and if we skip recording we will have it set to WRONG value.
	// So this set it at least to something reasonable ffs.
	cvar_set("extralogname", "");

	if (cvar("demo_tmp_record"))
	{ // FIXME: TODO: make this more like ktpro
		qbool record = false;

		if (isRACE())
		{
			record = true;
		}
		else if (!deathmatch)
		{
			record = false;
		}
		else if (isFFA() && cvar("demo_skip_ktffa_record"))
		{
			record = false;
		}
		else if (isHoonyModeAny() && HM_current_point() > 0)
		{
			record = false; // don't try to record (segfault) when already recording
		}
		else
		{
			record = true;
		}

		if (record)
		{
			if (!strnull(cvar_string("serverdemo")))
			{
				localcmd("sv_democancel\n"); // demo is recording, cancel before new one
			}

			demoname = CompilateDemoName();
			localcmd("sv_demoeasyrecord \"%s\"\n", demoname);
		}
	}
}

// Spawns the timer and starts the countdown.
void StartTimer(void)
{
	gedict_t *timer;

	if (match_in_progress || intermission_running || match_over)
	{
		return;
	}

	if (k_matchLess && !CountPlayers())
	{
		return; // can't start countdown in matchless mode due to no players,
	}

	k_force = 0;

	for (timer = world; (timer = find(timer, FOFCLSN, "idlebot"));)
	{
		ent_remove(timer);
	}

	for (timer = world; (timer = find(timer, FOFCLSN, "timer"));)
	{
		ent_remove(timer);
	}

	for (timer = world; (timer = find(timer, FOFCLSN, "standby_th"));)
	{
		ent_remove(timer);
	}

	if (!k_matchLess)
	{
		ShowMatchSettings();
	}

	if (!k_matchLess || k_bloodfest)
	{
		for (timer = world; (timer = find_client(timer));)
		{
			stuffcmd(timer, "play items/protect2.wav\n");
		}
	}

	timer = spawn();
	timer->s.v.owner = EDICT_TO_PROG(world);
	timer->classname = "timer";
	timer->cnt = 0;

	timer->cnt2 = max(3, (int)cvar("k_count")); // at the least we want a 3 second countdown

	if (isHoonyModeDuel() && (HM_current_point() > 0))
	{
		timer->cnt2 = 3; // first point gets usual 10 seconds, next points gets less
	}

	if (k_bloodfest)
	{
		// at the least 5 second countdown in bloodfest mode.
		timer->cnt2 = max(5, (int)cvar("k_count"));
	}
	else if (!deathmatch)
	{
		// no countdown in coop or similar modes.
		timer->cnt2 = 0;
	}
	else if (k_matchLess)
	{
		if (!cvar("k_matchless_countdown"))
		{
			timer->cnt2 = 0; // no countdown if variable is not specified.
		}
	}

	(timer->cnt2)++;

	timer->s.v.nextthink = g_globalvars.time + 0.001;
	timer->think = (func_t) TimerStartThink;

	match_in_progress = 1;

	localcmd("serverinfo status Countdown\n");

	SM_on_Countdown();
	StartDemoRecord(); // if allowed
}

static qbool match_can_cancel_demo(void)
{
	char matchtag[64] =
		{ 0 };
	int k_demo_mintime = bound(0, cvar("k_demo_mintime"), 3600);

	if (!match_start_time)
	{
		return true; // match not started
	}

#ifdef BOT_SUPPORT
	// if any bots involved, /break is a standard cancel, regardless of matchtag
	if (CountBots() == 0)
	{
#endif
	// don't cancel demo for match with matchtag
	infokey(world, "matchtag", matchtag, sizeof(matchtag));
	if (!strnull(matchtag))
	{
		return false;
	}
#ifdef BOT_SUPPORT
	}
#endif

	// only cancel if /break before set time elapsed
	if (k_demo_mintime <= 0)
	{
		k_demo_mintime = 120; // 120 seconds is default
	}

	return (g_globalvars.time - match_start_time) < k_demo_mintime;
}

// Whenever a countdown or match stops, remove the timer and reset everything.
// also stop/cancel demo recording
void StopTimer(int removeDemo)
{
	gedict_t *timer, *p;

	if (match_in_progress == 1)
		G_cp2all("%s", ""); // clear center print

	k_force = 0;
	match_in_progress = 0;

	if (k_standby)
	{
		// Stops the bug where players are set to ghosts 0.2 second to go and countdown aborts.
		// standby flag needs clearing (sturm)
		k_standby = 0;

		for (p = world; (p = find_plr(p));)
		{
			setfullwep(p);

			p->s.v.takedamage = DAMAGE_AIM;
			p->s.v.solid = SOLID_SLIDEBOX;
			p->s.v.movetype = MOVETYPE_WALK;
			setmodel(p, "progs/player.mdl");

			// Relink after solid change so players are returned to the correct list
			setorigin(p, PASSVEC3(p->s.v.origin));
		}
	}

	for (timer = world; (timer = find(timer, FOFCLSN, "timer"));)
	{
		ent_remove(timer);
	}

	for (timer = world; (timer = find(timer, FOFCLSN, "standby_th"));)
	{
		ent_remove(timer);
	}

	if (removeDemo && (match_can_cancel_demo()) && (race_can_cancel_demo())
			&& !strnull(cvar_string("serverdemo")))
	{
		localcmd("sv_democancel\n"); // demo is recording and must be removed, do it
	}

	match_start_time = 0;

	// do not set to Standby during points, (unless its the final point of course)
	if (!isHoonyModeAny() || (HM_current_point_type() == HM_PT_FINAL))
	{
		localcmd("serverinfo status Standby\n");
	}
}

void IdlebotForceStart(void)
{
	gedict_t *p;
	int i;

	G_bprint(2, "server is tired of waiting\n"
				"match WILL commence!\n");

	i = 0;
	for (p = world; (p = find_plr(p));)
	{
		if (p->ready)
		{
			i++;
		}
		else
		{
			G_bprint(2, "%s was kicked by IDLE BOT\n", p->netname);
			G_sprint(p, 2, "Bye bye! Pay attention next time.\n");

			stuffcmd(p, "disconnect\n"); // FIXME: stupid way
		}
	}

	k_attendees = i;

	if (k_attendees > 1)
	{
		StartTimer();
	}
	else
	{
		G_bprint(2, "Can't start! More players needed.\n");
		EndMatch(1);
	}
}

void IdlebotThink(void)
{
	gedict_t *p;
	int i;

	if (cvar("k_idletime") <= 0)
	{
		ent_remove(self);
		return;
	}

	self->attack_finished -= 1;

	i = CountPlayers();

	if (((0.5f * i) > CountRPlayers()) || (i < 2))
	{
		G_bprint(2, "console: bah! chickening out?\n"
					"server disables the %s\n",
					redtext("idle bot"));

		ent_remove(self);

		return;
	}

	k_attendees = CountPlayers();

	if (!isCanStart(NULL, true))
	{
		G_bprint(2, "%s removed\n", redtext("idle bot"));

		ent_remove(self);

		return;
	}

	if (self->attack_finished < 1)
	{

		IdlebotForceStart();

		ent_remove(self);

		return;

	}
	else
	{
		i = self->attack_finished;

		if ((i < 5) || !(i % 5))
		{
			for (p = world; (p = find_plr(p));)
			{
				if (!p->ready)
				{
					G_sprint(p, 2, "console: %d second%s to go ready\n", i, (i == 1 ? "" : "s"));
				}
			}
		}
	}

	self->s.v.nextthink = g_globalvars.time + 1;
}

void IdlebotCheck(void)
{
	gedict_t *p;
	int i;
	int bots = CountBots();

	if ((cvar("k_idletime") <= 0) || bots)
	{
		if ((p = find(world, FOFCLSN, "idlebot")))
		{
			ent_remove(p);
		}

		return;
	}

	i = CountPlayers();

	if (((0.5f * i) > CountRPlayers()) || (i < 2))
	{
		p = find(world, FOFCLSN, "idlebot");

		if (p)
		{
			G_bprint(2, "console: bah! chickening out?\n"
						"server disables the %s\n",
						redtext("idle bot"));

			ent_remove(p);
		}

		return;
	}

	if (match_in_progress || intermission_running || k_force)
	{
		return;
	}

	// no idle bot in practice mode
	if (k_practice) // #practice mode#
	{
		return;
	}

	if ((p = find(world, FOFCLSN, "idlebot"))) // already have idlebot
	{
		return;
	}

	//50% or more of the players are ready! go-go-go

	k_attendees = CountPlayers();

	if (!isCanStart( NULL, true))
	{
		G_sprint(self, 2, "Can't issue %s!\n", redtext("idle bot"));

		return;
	}

	p = spawn();
	p->classname = "idlebot";
	p->think = (func_t) IdlebotThink;
	p->s.v.nextthink = g_globalvars.time + 0.001;

	p->attack_finished = max(3, cvar("k_idletime"));

	G_bprint(2, "\n"
				"server activates the %s\n",
				redtext("idle bot"));
}

void CheckAutoXonX(qbool use_time);
void r_changestatus(float t);

// Called by a player to inform that (s)he is ready for a match.
void PlayerReady(qbool startIdlebot)
{
	gedict_t *p;
	float nready;
	char *matchtag = ezinfokey(world, "matchtag");
	qbool has_matchtag = matchtag != NULL && matchtag[0];

	if (isRACE() && !race_match_mode())
	{
		r_changestatus(1); // race_ready

		return;
	}

	if (self->ct == ctSpec && !isRACE())
	{

		if (!cvar("k_auto_xonx") || k_matchLess)
		{
			G_sprint(self, 2, "Command not allowed\n");

			return;
		}

		if (self->ready)
		{
			G_sprint(self, 2, "Type break to unready yourself\n");

			return;
		}

		self->ready = 1;

		for (p = world; (p = (match_in_progress ? find_spc(p) : find_client(p)));)
		{
			G_sprint(p, 2, "%s %s to play\n", self->netname, redtext("desire"));
		}

		CheckAutoXonX(g_globalvars.time < 10 ? true : false); // force switch mode asap if possible after some time spent

		return;
	}

	if (intermission_running || (match_in_progress == 2) || match_over)
	{
		return;
	}

	if (k_practice && !isRACE())
	{ // #practice mode#
		G_sprint(self, 2, "%s\n", redtext("Server in practice mode"));

		return;
	}

	if (self->ready)
	{
		G_sprint(self, 2, "Type break to unready yourself\n");

		return;
	}

	if (is_private_game() && !is_logged_in(self))
	{
		G_sprint(self, 2, "You must login first\n");

		return;
	}

	if (isCTF() || isHoonyModeTDM())
	{
		if (!streq(getteam(self), "red") && !streq(getteam(self), "blue"))
		{
			G_sprint(self, 2, "You must be on team red or blue\n");

			return;
		}
	}

	if (k_force && (isTeam() || isCTF()))
	{
		nready = 0;
		for (p = world; (p = find_plr(p));)
		{
			if (p->ready)
			{
				if (streq(getteam(self), getteam(p)) && !strnull(getteam(self)))
				{
					nready = 1;
					break;
				}
			}
		}

		if (!nready)
		{
			G_sprint(self, 2, "Join an existing team!\n");

			return;
		}
	}

	// do not allow empty team in team mode, because it cause problems
	if ((isTeam() || isCTF() || isCA()) && strnull(getteam(self)))
	{
		G_sprint(self, 2, "Set your %s before ready!\n", redtext("team"));

		return;
	}

	if (GetHandicap(self) != 100)
	{
		G_sprint(self, 2, "\207%s you are using handicap!\n", redtext("WARNING:"));
	}

	self->ready = 1;
	self->v.brk = 0;
	self->k_teamnum = 0;

	// force red or blue color if ctf
	if (isCTF())
	{
		if (streq(getteam(self), "blue"))
		{
			stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "color 13\n");
		}
		else if (streq(getteam(self), "red"))
		{
			stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "color 4\n");
		}
	}

	if (!isHoonyModeAny() || (HM_current_point() == 0))
	{
		G_bprint(2, "%s %s%s\n", self->netname, redtext("is ready"),
					((isTeam() || isCTF()) ? va(" \220%s\221", getteam(self)) : ""));
	}

	nready = CountRPlayers();
	k_attendees = CountPlayers();

	if (match_in_progress)
	{
		return; // possible in bloodfest.
	}

	if (!isCanStart( NULL, false))
	{
		return; // rules does't allow us to start match, idlebot ignored because of same reason
	}

	if (k_force)
	{
		return; // admin forces match - timer will started somewhere else
	}

	// we ignore "all players ready" and "at least two players ready" checks in bloodfest mode.
	if (!k_bloodfest)
	{
		if (nready != k_attendees)
		{
			// not all players ready, check idlebot and return
			if (startIdlebot)
			{
				IdlebotCheck();
			}

			return;
		}

		// ok all players ready.
		// only one or less players ready, match is pointless.
		if (nready < 2)
		{
			return;
		}
	}

	if (isHoonyModeAny() && k_attendees && nready == k_attendees)
	{
		HM_all_ready();
	}
	else
	{
		if (k_attendees && (nready == k_attendees))
		{
			G_bprint(2, "All players ready\n");
		}

		if (has_matchtag && cvar("k_on_start_f_modified"))
		{
			stuffcmd(self, "say f_modified\n");
		}

		if (has_matchtag && cvar("k_on_start_f_ruleset"))
		{
			stuffcmd(self, "say f_ruleset\n");
		}

		if (has_matchtag && cvar("k_on_start_f_version"))
		{
			stuffcmd(self, "say f_version\n");
		}

		G_bprint(2, "Timer started\n");
	}

	StartTimer();
}

void PlayerSlowReady(void)
{
	PlayerReady(false);
}

void PlayerFastReady(void)
{
	PlayerReady(true);
}

void PlayerBreak(void)
{
	int votes;
	gedict_t *p;

	if (isRACE() && !race_match_mode())
	{
		r_changestatus(2); // race_break

		return;
	}

	if ((self->ct == ctSpec) && !isRACE())
	{
		if (!cvar("k_auto_xonx") || k_matchLess)
		{
			G_sprint(self, 2, "Command not allowed\n");

			return;
		}

		if (!self->ready)
		{
			return;
		}

		self->ready = 0;

		for (p = world; (p = (match_in_progress ? find_spc(p) : find_client(p)));)
		{
			G_sprint(p, 2, "%s %s to play\n", self->netname, redtext("lost desire"));
		}

		return;
	}

	if (!self->ready || intermission_running || match_over)
	{
		return;
	}

	if (isCA() && (match_in_progress == 2) && !self->ca_ready)
	{
		G_sprint(self, 2, "You must be in the game to vote\n");

		return;
	}

	if (k_matchLess && !k_bloodfest)
	{
		// do not allow break/next_map commands in some cases.
		if (cvar("k_no_vote_map"))
		{
			G_sprint(self, 2, "Voting next map is %s allowed\n", redtext("not"));

			return;
		}
	}

	if (!match_in_progress)
	{
		self->ready = 0;

		G_bprint(2, "%s %s\n", self->netname, redtext("is not ready"));

		return;
	}

	if (!k_matchLess || k_bloodfest)
	{
		// try stop countdown. (countdown between hoony-mode points can't be stopped, treat as standard break request).
		qbool can_stop_hoonymode = (!isHoonyModeAny() || HM_current_point() == 0);

		if (match_in_progress == 1 && can_stop_hoonymode)
		{
			p = find(world, FOFCLSN, "timer");

			if (p && p->cnt2 > 1)
			{
				self->ready = 0;

				if (!k_matchLess || (k_bloodfest && (CountRPlayers() < 1)))
				{
					G_bprint(2, "%s %s\n", self->netname, redtext("stops the countdown"));
					StopTimer(1);
				}
				else
				{
					G_bprint(2, "%s %s\n", self->netname, redtext("is not ready"));
				}
			}

			return;
		}
	}

	if (self->v.brk)
	{
		self->v.brk = 0;

		G_bprint(2, "%s %s %s vote%s\n", self->netname, redtext("withdraws"), redtext(g_his(self)),
					((votes = get_votes_req(OV_BREAK, true)) ? va(" (%d)", votes) : ""));

		return;
	}

	self->v.brk = 1;

	G_bprint(2, "%s %s%s\n", self->netname,
				redtext(k_matchLess ? "votes for next map" : "votes for stopping the match"),
				((votes = get_votes_req( OV_BREAK, true)) ? va(" (%d)", votes) : ""));

	// show warning to player - that he can't stop countdown alone in matchless mode.
	if (k_matchLess && (match_in_progress == 1) && (CountPlayers() == 1))
	{
		G_sprint(self, 2, "You can't stop countdown alone\n");
	}

	vote_check_break();
}
