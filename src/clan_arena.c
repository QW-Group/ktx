//
// Clan Arena related
//

#include "g_local.h"

static int round_num;
static int team1_score;
static int team2_score;
static int pause_count;
static int pause_time;
static int round_pause;

gedict_t* ca_find_player(gedict_t *p)
{
	p = find_plr(p);
	while (p && !p->in_play)
	{
		p = find_plr(p);
	}

	return p;
}

gedict_t* ca_get_player(void)
{
	return ca_find_player(world);
}

qbool is_rules_change_allowed(void);

void SM_PrepareCA(void)
{
	if (!isCA())
	{
		return;
	}

	team1_score = team2_score = 0;
	round_num = 1;
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

// hard coded default settings for CA
static char ca_settings[] = "k_clan_arena_rounds 9\n"
		"k_clan_arena_max_respawns 0\n"
		"dp 0\n"
		"teamplay 4\n"
		"deathmatch 5\n"
		"timelimit 30"
		"k_pow 0"
		"k_overtime 0\n"
		"k_spw 1\n"
		"k_dmgfrags 1\n"
		"k_noitems 1\n";

void track_player(gedict_t *observer)
{
	gedict_t *player = ca_get_player();
	vec3_t delta;
	float vlen;
	int follow_distance;
	int upward_distance;

	if (!player) 
	{
		return;
	}

	if (!observer->in_play && observer->tracking_enabled)
	{
		if (observer->track_target && observer->track_target->in_play)
		{
			player = observer->track_target;
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
	}

	if (!observer->tracking_enabled)
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
	if (!(((int)(self->s.v.flags)) & FL_ATTACKRELEASED))
	{
		return;
	}

	self->s.v.flags = (int)self->s.v.flags & ~FL_ATTACKRELEASED;

	r_changetrackingstatus((float) 3);
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
	int max_respawns = 0;
	
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
		G_bprint(2, "%s %s %s\n", self->netname, "enables", redtext("Clan Arena: Wipeout"));
		max_respawns = 4;
	}
	else
	{
		G_bprint(2, "%s unknown\n", redtext("Clan Arena"));
	}

	apply_CA_settings();
	cvar_fset("k_clan_arena", k_clan_arena);
	cvar_fset("k_clan_arena_max_respawns", max_respawns);
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
		self->s.v.ammo_rockets = 60;
		self->s.v.ammo_cells = 150;

		self->s.v.armorvalue = 200;
		self->s.v.armortype = 0.8;
		self->s.v.health = 100;

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

		if (!self->teamcolor)
		{
			// if team is red or blue, set color to match
			if (streq(getteam(self), "red") || streq(getteam(self), "blue"))
			{
				self->teamcolor = streq(getteam(self), "red") ? "4" : "13";
			}
			else if (streq(cvar_string("_k_team1"), "red") || streq(cvar_string("_k_team2"), "red"))
			{
				self->teamcolor = "13";
			}
			else if (streq(cvar_string("_k_team1"), "blue") || streq(cvar_string("_k_team2"), "blue"))
			{
				self->teamcolor = "4";
			}
			else 
			{
				self->teamcolor = streq(cvar_string("_k_team1"), getteam(self)) ? "4" : "13";
			}
		}

		SetUserInfo(self, "topcolor", self->teamcolor, 0);
		SetUserInfo(self, "bottomcolor", self->teamcolor, 0);
		stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "color %s\n", self->teamcolor);
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

		if (self->round_deaths <= max_deaths)
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
			// Change color to white if dead
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

void CA_ClientObituary(gedict_t *targ, gedict_t *attacker)
{
	int ah, aa;

	if (!isCA())
	{
		return;
	}

	if (targ->ct != ctPlayer)
	{
		return; // so below targ is player
	}

	if (attacker->ct != ctPlayer)
	{
		attacker = targ; // seems killed self
	}

	ah = attacker->s.v.health;
	aa = attacker->s.v.armorvalue;

	if (attacker->ct == ctPlayer)
	{
		if (attacker != targ)
		{
			if ((ah == 100) && (aa == 200))
			{
				G_sprint(targ, PRINT_HIGH, "%s %s %d %s %d %s\n",
							attacker->netname, redtext("had"), aa,
							redtext("armor and"), ah, redtext("health"));
			}
		}
	}
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

void EndRound(int alive_team)
{
	gedict_t *p;
	static int last_count;
	static qbool do_endround_stuff = false;
	
	if(!round_pause)
	{
		round_pause = 1;
		last_count = 999999999;
		pause_time = g_globalvars.time + 8;

		if (alive_team == 1)
		{
			team1_score++;
		}
		else
		{
			team2_score++;
		}
	}

	pause_count = Q_rint(pause_time - g_globalvars.time);

	if (pause_count <= 0)
	{
		round_pause = 0;
		round_num++;
		ra_match_fight = 0;
		do_endround_stuff = false;
	}
	else if (pause_count != last_count)
	{
		last_count = pause_count;

		if (pause_count < 8)
		{
			if (alive_team == 0)
			{
				G_cp2all("Round draw!");

				for (p = world; (p = find_client(p));)
				{
					stuffcmd(p, "play ca/sfdraw.wav\n");
				}
			}
			
			else
			{
				G_cp2all("Team \x90%s\x91 wins the round!",
						cvar_string(va("_k_team%d", alive_team))); // CA_wins_required
			}
		}

		if (pause_count == 7)
		{
			if (!do_endround_stuff)
			{
				do_endround_stuff = true;

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

						if (strneq(ezinfokey(p, "bottomcolor"), p->teamcolor))
						{
							SetUserInfo(p, "topcolor", "0", 0);
							SetUserInfo(p, "bottomcolor", "0", 0);
						}
					}
				}
			}
			
		}

		if (pause_count < 4)
		{
			ra_match_fight = 1; // disable firing
		}
	}
}

static void track_player_next(gedict_t *observer)
{
	gedict_t *first_player = ca_get_player();
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
		player = ca_find_player(observer->track_target);
		if (!player)
		{
			player = first_player;
		}

		observer->track_target = player;
	}

	G_sprint(self, 2, "Tracking: %s\n", redtext(player->netname));
}

void CA_player_pre_think(void)
{
	if (isCA())
	{
		// Set this player to solid so we trigger checkpoints & teleports during move
		self->s.v.solid = (ISDEAD(self) ? SOLID_NOT : SOLID_SLIDEBOX);
		
		if ((self->s.v.mins[0] == 0) || (self->s.v.mins[1] == 0))
		{
			// This can happen if the world 'squashes' a SOLID_NOT entity, mvdsv will turn into corpse
			setsize(self, PASSVEC3(VEC_HULL_MIN), PASSVEC3(VEC_HULL_MAX));
		}

		setorigin(self, PASSVEC3(self->s.v.origin));

		if ((self->ct == ctPlayer) && ISDEAD(self))
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
	}
}

void CA_player_post_think(void)
{
	if (isCA())
	{
		if (ra_match_fight > 0 && !self->in_play)
		{
			track_player(self);
		}
	}
	
}

void CA_Frame(void)
{
	static int last_r;
	static int last_respawn_count;
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

	// if max_respawns are greater than 0, we're playing wipeout
	if (ra_match_fight == 2 && !round_pause && cvar("k_clan_arena_max_respawns"))
	{
		int max_deaths = cvar("k_clan_arena_max_respawns");

		for (p = world; (p = find_plr(p));)
		{
			if (!p->in_play && p->round_deaths <= max_deaths)
			{
				p->in_limbo = true;

				if (!p->spawn_delay)
				{
					int delay = p->round_deaths * 7;

					last_respawn_count = 999999999;
					p->spawn_delay = g_globalvars.time + delay;
				}

				p->seconds_to_respawn = Q_rint(p->spawn_delay - g_globalvars.time);

				if (p->seconds_to_respawn <= 0)
				{
					p->spawn_delay = 0;
					G_centerprint(p, "%s\n", "FIGHT!");
					k_respawn(p, true);

					p->seconds_to_respawn = g_globalvars.time;
				}
				
				else if (p->seconds_to_respawn != last_respawn_count)
				{
					last_respawn_count = p->seconds_to_respawn;

					if (p->seconds_to_respawn > 0)
					{
						G_centerprint(p, "%d\n\n\n seconds to respawn!\n", p->seconds_to_respawn);
					}
				}	
			}
		}
	}

	// check if there exist only one team with alive players and others are eluminated, if so then its time to start ca countdown
	if (ra_match_fight == 2 || (ra_match_fight == 1 && round_pause == 1))
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
		EndMatch(0);

		return;
	}

	if (!round_pause)
	{
		if (!ra_match_fight)
		{
			// ok start ra timer
			ra_match_fight = 1; // ra countdown
			last_r = 999999999;
			time_to_start = g_globalvars.time + 9;

			for (p = world; (p = find_plr(p));)
			{
				p->round_deaths = 0;
				k_respawn(p, false);
				G_sprint(p, 2, "round \x90%d\x91 starts in 5 seconds.\n", round_num);
			}
		}

		r = Q_rint(time_to_start - g_globalvars.time);

		if (r <= 0)
		{
			for (p = world; (p = find_client(p));)
			{
				stuffcmd(p, "play ca/sffight.wav\n");
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
				
				G_cp2all("%d\n\n"
							"\x90%s\x91:%s \x90%s\x91:%s",
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
