// HoonyMode implementation (thanks Richard 'Hoony' Sandlant for idea from cpma)

// Thanks for help with code and testing:
// #qw-dev, deurk, johnny_cz, vvd, timon, eternal, dr4ko, rusty, twitch, mushi, 23, m@tr!}{, helltiger, leopold, quark

// 2012-03-02 - Initial code by phil
// 2012-03-05 - Got stuck on cvar_set/fset not working, so using static int instead :D
// 2012-03-06 - Got point-by-point play working (without map reload)
// 2012-03-08 - Got spawn rigging to work
// 2012-03-09 - Cleanup client and server console spamming
// 2012-03-13 - Complete rewrite (now that I know how ktx works...) ready for testing
// 2012-03-14 - Nicer looking lastscores
// 2012-03-20 - Per-point player stats (was thinking about how to do this and had a sudden idea)
// 2012-03-26 - Bugs fixed - hmstats health incorrectly computed as health+armor,
//              lava deaths (and others) not scored properly, and also ring/quad... (thx dr4k0 for testing)
// 2016       - timelimits, scenarios based on .ent files, spawn nominations (meag)
// 2017-05-13 - team-based hoonymode - multiple rounds

// todo: fix lg door/dm2 floating/etc
// todo: test drop/rejoin, and how it affects spawn memory
// todo: test name change during game and how it affects lastscores
// todo: reset more items on map (e.g. lg door on dm6)

#include "g_local.h"

static void HM_store_spawns(void);

#define HM_RESULT_NOTPLAYED    0
#define HM_RESULT_WONROUND     1
#define HM_RESULT_LOSTROUND    2
#define HM_RESULT_SUICIDELOSS  3
#define HM_RESULT_SUICIDEWIN   4
#define HM_RESULT_DRAWWIN      5

const char hm_result_indicator[] = "\0WLswD";

#define HM_WINNING_DIFF        2

static qbool match_break = false;
static int round_number = 0;        // valid array index
static int true_round_number = 0;   // keeps increasing, for insane case of > HM_MAX_ROUNDS game
static char round_explanation[128] =
	{ 0 };
static char series_explanation[128] =
	{ 0 };

static void EndRound(void)
{
	++true_round_number;
	if (round_number < HM_MAX_ROUNDS - 1)
	{
		++round_number;
	}
	else
	{
		// Start over-writing last round to stop buffer over-run
		--round_number;
	}

	EndMatch(0);
}

const char* HM_round_results(gedict_t *player)
{
	static char buffer[HM_MAX_ROUNDS * 4 + 1];
	int i;

	if (isDuel())
	{
		for (i = 0; i < round_number; ++i)
		{
			if ((player->hoony_results[i] >= 0)
					&& (player->hoony_results[i] < sizeof(hm_result_indicator)))
			{
				buffer[i] = hm_result_indicator[player->hoony_results[i]];
			}
		}

		buffer[i] = '\0';

		return buffer;
	}

	return "";
}

qbool isHoonyModeDuel(void)
{
	return (isDuel() && cvar("k_hoonymode"));
}

qbool isHoonyModeAny(void)
{
	return cvar("k_hoonymode");
}

qbool isHoonyModeTDM(void)
{
	return (isTeam() && cvar("k_hoonymode"));
}

int HM_rounds(void)
{
	int rounds = cvar("k_hoonyrounds");

	return (rounds ? rounds : 6);
}

void HM_initialise_rounds(void)
{
	round_number = 0;
}

int HM_timelimit(void)
{
	if (timelimit)
	{
		return (timelimit * 60);
	}

	return world->hoony_timelimit;
}

// duel: suicide, etc, count as a point for other players
// in teamplay, dock the frag as normal
void HM_suicide(gedict_t *player)
{
	gedict_t *p;

	if (isDuel())
	{
		player->hoony_results[round_number] = HM_RESULT_SUICIDELOSS;

		for (p = world; (p = find_plr(p));)
		{
			if (p != player)
			{
				p->s.v.frags += 1;
				p->hoony_results[round_number] = HM_RESULT_SUICIDEWIN;
			}
		}

		EndRound();
	}
}

// Timelimit hit
void HM_draw(void)
{
	if (isHoonyModeDuel())
	{
		gedict_t *p;
		int maxfrags = -9999, minfrags = 9999;

		for (p = world; (p = find_plr(p));)
		{
			// .ent file can dictate that one player wins by default
			if (!strnull(world->hoony_defaultwinner) && p->k_hoony_new_spawn
					&& streq(p->k_hoony_new_spawn->targetname, world->hoony_defaultwinner))
			{
				p->s.v.frags++;
				G_bprint(2, "%s wins the round on time.\n", p->netname);
				EndRound();

				return;
			}

			maxfrags = max(p->s.v.frags, maxfrags);
			minfrags = min(p->s.v.frags, minfrags);
		}

		if (maxfrags < HM_rounds())
		{
			// If in normal rounds, everyone gets a point, so we get closer to finishing.  
			gedict_t *p;

			for (p = world; (p = find_plr(p));)
			{
				p->hoony_results[round_number] = HM_RESULT_DRAWWIN;
				p->s.v.frags++;
				for (p = world; (p = find_plr(p));)
				{
					p->hoony_results[round_number] = HM_RESULT_DRAWWIN;
					p->s.v.frags++;
				}
			}
		}

		// We go again...
		G_bprint(PRINT_HIGH, "This round ends in a draw\n");
	}

	EndRound();
}

qbool HM_is_game_over(void)
{
	// If the round ended due to /break then game is over
	if (match_break)
	{
		return true;
	}

	// Game is over if we've hit frag limit and one player is in lead (HM_PT_FINAL), or
	//   one player is one frag ahead at the start of a round of spawns and we're past the fraglimit
	if (HM_current_point_type() == HM_PT_FINAL)
	{
		return true;
	}

	// 
	if (isHoonyModeDuel() && HM_current_point_type() == HM_PT_SET && true_round_number % 2 == 0)
	{
		gedict_t *p;
		int maxfrags = -999;
		int minfrags = 999;

		for (p = world; (p = find_plr(p));)
		{
			maxfrags = max(p->s.v.frags, maxfrags);
			minfrags = min(p->s.v.frags, minfrags);
		}

		return ((maxfrags != minfrags) && (maxfrags > (HM_rounds() / 2)));
	}

	return false;
}

void HM_next_point(gedict_t *winner, gedict_t *loser)
{
	winner->hoony_results[round_number] = HM_RESULT_WONROUND;
	loser->hoony_results[round_number] = HM_RESULT_LOSTROUND;

	EndRound();
}

int HM_current_point(void)
{
	return round_number;
}

int HM_current_point_type(void)
{
	if (!isHoonyModeDuel())
	{
		int score1 = get_scores1();
		int score2 = get_scores2();
		int maxfrags = max(score1, score2), minfrags = min(score1, score2);
		int fragdiff = maxfrags - minfrags;

		// Must have even number of rounds
		if ((round_number >= (HM_rounds() - 1)) && ((round_number % 2) == 1))
		{
			return HM_PT_SET;
		}

		// Finish as soon as possible, as long as there's a winner
		if ((round_number >= HM_rounds()) && fragdiff)
		{
			return HM_PT_FINAL;
		}
	}
	else
	{
		gedict_t *p;
		int maxfrags = -999, minfrags = 999;
		int fragdiff;

		for (p = world; (p = find_plr(p));)
		{
			maxfrags = max(maxfrags, p->s.v.frags);
			minfrags = min(minfrags, p->s.v.frags);
		}

		fragdiff = maxfrags - minfrags;

		if ((round_number >= HM_rounds()) && (fragdiff >= HM_WINNING_DIFF))
		{
			return HM_PT_FINAL;
		}

		if ((maxfrags >= (HM_rounds() / 2)) && (fragdiff >= (HM_WINNING_DIFF - 1)))
		{
			return HM_PT_SET;
		}
	}

	return 0;
}

void SUB_regen(void);

void remove_items(char *classname)
{
	gedict_t *p;

	for (p = world; (p = find(p, FOFCLSN, classname)); /**/)
	{
		ent_remove(p);
	}
}

void respawn_items(char *classname, qbool enabled)
{
	gedict_t *p;

	if (strnull(classname))
	{
		G_Error("respawn_items");
	}

	for (p = world; (p = find(p, FOFCLSN, classname)); /**/)
	{
		if (enabled)
		{
			if (p->initial_spawn_delay > 0)
			{
				// hide, but respawn at future point
				setmodel(p, "");
				p->s.v.solid = (bots_enabled() ? SOLID_TRIGGER : SOLID_NOT);
				p->s.v.nextthink = g_globalvars.time + p->initial_spawn_delay;
				p->think = (func_t) SUB_regen;
			}
			else
			{
				// respawn now
				p->s.v.nextthink = g_globalvars.time;
				p->think = (func_t) SUB_regen;
			}

#ifdef BOT_SUPPORT
			p->fb.goal_respawn_time = p->s.v.nextthink;
#endif
		}
		else
		{
			// hide item
			setmodel(p, "");
			p->s.v.solid = (bots_enabled() ? SOLID_TRIGGER : SOLID_NOT);
			p->s.v.nextthink = 0;
#ifdef BOT_SUPPORT
			p->fb.goal_respawn_time = 0;
#endif
		}
	}
}

void HM_reset_map(void)
{
	// re-initialise items for every point
	respawn_items("item_shells", true);
	respawn_items("item_spikes", true);
	respawn_items("item_rockets", true);
	respawn_items("item_cells", true);

	respawn_items("item_health", true);
	respawn_items("item_armor1", true);
	respawn_items("item_armor2", true);
	respawn_items("item_armorInv", true);

	respawn_items("weapon_supershotgun", true);
	respawn_items("weapon_nailgun", true);
	respawn_items("weapon_supernailgun", true);
	respawn_items("weapon_grenadelauncher", true);
	respawn_items("weapon_rocketlauncher", true);
	respawn_items("weapon_lightning", true);

	respawn_items("item_artifact_invulnerability", Get_Powerups());
	respawn_items("item_artifact_super_damage", Get_Powerups());
	respawn_items("item_artifact_envirosuit", Get_Powerups());
	respawn_items("item_artifact_invisibility", Get_Powerups());

	// remove temporary objects
	remove_projectiles();
	remove_items("backpack");
}

static int HM_spawn_comparison(const void *lhs_, const void *rhs_)
{
	const gedict_t *lhs = *(const gedict_t**) lhs_;
	const gedict_t *rhs = *(const gedict_t**) rhs_;

	if (lhs->hoony_spawn_order < rhs->hoony_spawn_order)
	{
		return -1;
	}
	else if (lhs->hoony_spawn_order > rhs->hoony_spawn_order)
	{
		return 1;
	}

	return 0;
}

static void HM_sort_spawns(gedict_t **spawns, int count)
{
#ifdef Q3_VM
	qbool any_changes = true;

	// bubble-sort, lovely
	while (any_changes)
	{
		int i;

		any_changes = false;
		for (i = 0; i < count - 1; ++i)
		{
			int comp = HM_spawn_comparison(&spawns[i], &spawns[i + 1]);

			if (comp > 0)
			{
				gedict_t* temp = spawns[i];
				spawns[i] = spawns[i + 1];
				spawns[i + 1] = temp;
				any_changes = true;
			}
		}
	}
#else
	qsort(spawns, count, sizeof(spawns[0]), HM_spawn_comparison);
#endif
}

static void HM_shuffle_spawns(gedict_t **spawns, int count)
{
	int i = 0;
	int j = 0;

	for (i = 0; i < 6; ++i)
	{
		for (j = 0; j < count; ++j)
		{
			spawns[j]->hoony_spawn_order = (spawns[j]->hoony_nomination ? -1 : 1) * i_rnd(1, 100);
		}

		HM_sort_spawns(spawns, count);
	}
}

static gedict_t *red_spawns[MAX_CLIENTS] =
	{ 0 };
static gedict_t *blue_spawns[MAX_CLIENTS] =
	{ 0 };
static int red_spawncount = 0;
static int blue_spawncount = 0;

// This is called at the start of each round
void HM_all_ready(void)
{
	gedict_t *p;
	qbool debug = cvar("developer");

	for (p = world; (p = find_plr(p)); /**/)
	{
		// Clear allocated spawns for the current round
		p->k_hoony_new_spawn = NULL;
	}

	// Shuffle spawn order, then assign to players
	if (HM_current_point() % 2 == 0)
	{
		// randomise order
		if (isTeam())
		{
			int red_assigned_spawn = 0;
			int blue_assigned_spawn = 0;
			int red_players = 0;
			int blue_players = 0;
			qbool red_wrapped = false;
			qbool blue_wrapped = false;
			qbool alternate = i_rnd(1, 10) <= 5;

			// Allocate specific red/blue spawns accordingly
			red_spawncount = blue_spawncount = 0;

			// Allocate assigned deathmatch spawns
			for (p = world; (p = ez_find(p, "info_player_deathmatch"));)
			{
				if ((red_spawncount < MAX_CLIENTS) && (p->hoony_nomination == (alternate ? 2 : 1)))
				{
					red_spawns[red_spawncount++] = p;
				}
				else if ((blue_spawncount < MAX_CLIENTS)
						&& (p->hoony_nomination == (alternate ? 1 : 2)))
				{
					blue_spawns[blue_spawncount++] = p;
				}
			}

			// Allocate standard deathmatch spawns
			for (p = world;
					(p = ez_find(p, "info_player_deathmatch"))
							&& (red_spawncount < MAX_CLIENTS || blue_spawncount < MAX_CLIENTS);)
			{
				if (p->hoony_nomination)
				{
					continue;
				}

				if ((red_spawncount < MAX_CLIENTS) && (red_spawncount < blue_spawncount))
				{
					red_spawns[red_spawncount++] = p;
				}
				else if (blue_spawncount < MAX_CLIENTS)
				{
					blue_spawns[blue_spawncount++] = p;
				}
			}

			// Shuffle
			HM_shuffle_spawns(red_spawns, red_spawncount);
			HM_shuffle_spawns(blue_spawns, blue_spawncount);

			// Assign based on team
			for (p = world; (p = find_plr(p)); /**/)
			{
				if (red_assigned_spawn == (int)min(red_spawncount, MAX_CLIENTS))
				{
					red_assigned_spawn = 0;
					red_wrapped = true;
				}
				if (blue_assigned_spawn == (int)min(blue_spawncount, MAX_CLIENTS))
				{
					blue_assigned_spawn = 0;
					blue_wrapped = true;
				}

				if (streq(getteam(p), "red"))
				{
					p->k_hoony_new_spawn = p->k_hoonyspawn = red_spawns[red_assigned_spawn++];
					++red_players;
				}
				else
				{
					p->k_hoony_new_spawn = p->k_hoonyspawn = blue_spawns[blue_assigned_spawn++];
					++blue_players;
				}

				if (debug)
				{
					G_bprint(PRINT_HIGH, "Assigned '%s' to %s (%s)\n",
								p->k_hoony_new_spawn->targetname, p->netname, getteam(p));
				}
			}

			if (!red_wrapped)
			{
				red_spawncount = min(red_spawncount, max(red_assigned_spawn, blue_players));
			}

			if (!blue_wrapped)
			{
				blue_spawncount = min(blue_spawncount, max(blue_assigned_spawn, red_players));
			}
		}
		else
		{
			gedict_t *spawns[MAX_CLIENTS] =
				{ 0 };
			int spawncount = 0;
			int assigned_spawn = 0;

			for (p = world; (p = ez_find(p, "info_player_deathmatch")) && spawncount < MAX_CLIENTS;)
			{
				spawns[spawncount++] = p;
			}

			HM_shuffle_spawns(spawns, spawncount);
			if (debug)
			{
				G_bprint(PRINT_HIGH, "Shuffling spawn points...\n");
			}

			// assign as standard
			for (p = world; (p = find_plr(p)); /**/)
			{
				if (assigned_spawn == (int)min(spawncount, MAX_CLIENTS))
				{
					assigned_spawn = 0;
				}

				p->k_hoony_new_spawn = p->k_hoonyspawn = spawns[assigned_spawn++];
				if (debug)
				{
					G_bprint(PRINT_HIGH, "Assigned '%s' to %s\n", p->k_hoony_new_spawn->targetname,
								p->netname);
				}
			}
		}
	}
	else
	{
		// on odd-numbered rounds, swap spawn points with opponents
		if (isTeam())
		{
			int red_assigned_spawn = 0;
			int blue_assigned_spawn = 0;

			// Shuffle again
			HM_shuffle_spawns(red_spawns, red_spawncount);
			HM_shuffle_spawns(blue_spawns, blue_spawncount);

			// Assign based on team
			for (p = world; (p = find_plr(p)); /**/)
			{
				if (red_assigned_spawn == (int)min(red_spawncount, MAX_CLIENTS))
				{
					red_assigned_spawn = 0;
				}
				if (blue_assigned_spawn == (int)min(blue_spawncount, MAX_CLIENTS))
				{
					blue_assigned_spawn = 0;
				}

				if (!streq(getteam(p), "red"))
				{
					p->k_hoony_new_spawn = p->k_hoonyspawn = red_spawns[red_assigned_spawn++];
				}
				else
				{
					p->k_hoony_new_spawn = p->k_hoonyspawn = blue_spawns[blue_assigned_spawn++];
				}

				if (debug)
				{
					G_bprint(PRINT_HIGH, "Assigned '%s' to %s (%s)\n",
								p->k_hoony_new_spawn->targetname, p->netname, getteam(p));
				}
			}
		}
		else
		{
			for (p = world; (p = find_plr(p)); /**/)
			{
				gedict_t *next = find_plr(p);

				if (next == NULL)
				{
					next = find_plr(world);
				}

				p->k_hoony_new_spawn = next->k_hoonyspawn;
				if (debug)
				{
					G_bprint(PRINT_HIGH, "Assigned '%s' to %s\n", p->k_hoony_new_spawn->targetname,
								p->netname);
				}
			}
		}
	}
}

char* HM_lastscores_extra(void)
{
	//extra[0] = 0;
	//strlcat(extra, va("\n%s\n%s", extra1, extra2), sizeof(extra));
	//return extra;
	return "";
}

// Naming spawns
// - Spawns can have name set in map or .ent file
//   Hardcoded names for TB3/TB5 - could go away in future

typedef struct hm_spawn_name_t
{
	vec3_t origin;
	char *name;
} hm_spawn_name;

static void HM_name_spawn(gedict_t *spawn, hm_spawn_name *spawns, int spawncount)
{
	if (strnull(spawn->targetname))
	{
		int i = 0;
		for (i = 0; i < spawncount; ++i)
		{
			if (VectorCompare(spawn->s.v.origin, spawns[i].origin))
			{
				spawn->targetname = spawns[i].name;
				break;
			}
		}
	}
}

void HM_name_map_spawn(gedict_t *spawn)
{
	char *mapname = g_globalvars.mapname;

	if (streq(mapname, "dm2"))
	{
		hm_spawn_name spawns[] =
			{
				{
					{ 2560, -192, 32 }, "nailgun" },
				{
					{ 2544, -32, -64 }, "low tele" },
				{
					{ 2624, -488, 32 }, "water" },
				{
					{ 2176, -2176, 88 }, "low-btn" },
				{
					{ 2576, -1328, 24 }, "low-rl" },
				{
					{ 2704, -2048, 128 }, "ra-mega" },
				{
					{ 1712, -504, 24 }, "quad" },
				{
					{ 2048, -1352, 136 }, "big" },
				{
					{ 2248, -16, -136 }, "tele-btn" } };

		HM_name_spawn(spawn, spawns, sizeof(spawns) / sizeof(spawns[0]));
	}
	else if (streq(mapname, "dm3"))
	{
		hm_spawn_name spawns[] =
			{
				{
					{ -880, -232, -16 }, "tele/sng" },
				{
					{ 192, -208, -176 }, "big>ra" },
				{
					{ 1472, -928, -24 }, "ya box" },
				{
					{ 1520, 432, -88 }, "rl" },
				{
					{ -632, -680, -16 }, "tele/ra" },
				{
					{ 512, 768, 216 }, "lifts" } };

		HM_name_spawn(spawn, spawns, sizeof(spawns) / sizeof(spawns[0]));
	}
	else if (streq(mapname, "dm4"))
	{
		hm_spawn_name spawns[] =
			{
				{
					{ -64, 512, -296 }, "mh room" },
				{
					{ -64, -232, -72 }, "quad" },
				{
					{ 272, -952, 24 }, "high" },
				{
					{ 112, -1136, -104 }, "ssg" },
				{
					{ 776, -808, -232 }, "ra" },
				{
					{ 784, -176, 24 }, "ya-entry" } };

		HM_name_spawn(spawn, spawns, sizeof(spawns) / sizeof(spawns[0]));
	}
	else if (streq(mapname, "dm6"))
	{
		hm_spawn_name spawns[] =
			{
				{
					{ 232, -1512, 40 }, "ga tele" },
				{
					{ 1016, -416, 40 }, "low rl" },
				{
					{ 0, -1088, 264 }, "high rl" },
				{
					{ 456, -1504, 256 }, "sng" },
				{
					{ 408, -1088, 256 }, "gl>sng" },
				{
					{ 896, -1464, 256 }, "gl room" },
				{
					{ 1892, -160, 168 }, "ra" } };

		HM_name_spawn(spawn, spawns, sizeof(spawns) / sizeof(spawns[0]));
	}
	else if (streq(mapname, "e1m2"))
	{
		hm_spawn_name spawns[] =
			{
				{
					{ -416, -144, 320 }, "mh room" },
				{
					{ 168, -480, 320 }, "mh door" },
				{
					{ 1496, 1328, 200 }, "start" },
				{
					{ 1936, -136, 312 }, "traps>ya" },
				{
					{ 936, -1216, 432 }, "quad>gl" },
				{
					{ 792, -992, 440 }, "gl>quad" },
				{
					{ 1080, -720, 312 }, "ya" },
				{
					{ 408, -752, 432 }, "quad" },
				{
					{ 792, -208, 320 }, "doors" },
				{
					{ 784, 808, 206 }, "rl" } };

		HM_name_spawn(spawn, spawns, sizeof(spawns) / sizeof(spawns[0]));
	}
	else if (streq(mapname, "ztndm3"))
	{
		hm_spawn_name spawns[] =
			{
				{
					{ -432, 128, 32 }, "ra/nails" },
				{
					{ -1056, -544, 224 }, "grenade" },
				{
					{ -208, -400, 224 }, "ya room" },
				{
					{ -320, 576, 224 }, "quad" },
				{
					{ -176, -112, 288 }, "ra/ssg" },
				{
					{ -96, 224, 32 }, "lg" },
				{
					{ -800, 0, -32 }, "ra/mega" } };

		HM_name_spawn(spawn, spawns, sizeof(spawns) / sizeof(spawns[0]));
	}
	else if (streq(mapname, "aerowalk"))
	{
		hm_spawn_name spawns[] =
			{
				{
					{ -224, -720, 256 }, "grenade" },
				{
					{ -488, 624, 264 }, "low rl" },
				{
					{ -224, -704, 456 }, "high rl" },
				{
					{ -272, 24, 40 }, "floor" },
				{
					{ -320, 480, 456 }, "ra" },
				{
					{ 160, 128, 256 }, "under lg" } };

		HM_name_spawn(spawn, spawns, sizeof(spawns) / sizeof(spawns[0]));
	}
}

// Picking spawns
// - In pregame, allow players to pick spawn points to use during the game
//   If player doesn't nominate a spawn point, 'their' spawn will be randomly chosen each round
gedict_t* Spawn_OnePoint(gedict_t *spawn_point, vec3_t org, int effects);

static void HM_deselect_spawn(gedict_t *spawn)
{
	int effects = (EF_GREEN | EF_RED | EF_BLUE);

	if (!spawn->wizard)
	{
		return;
	}

	// If showing all spawns, just remove the glow.  otherwise remove the marker.
	if (cvar("k_spm_show"))
	{
		spawn->wizard->s.v.effects = (int) spawn->wizard->s.v.effects & ~effects;
	}
	else
	{
		ent_remove(spawn->wizard);
		spawn->wizard = 0;
	}

	if (spawn->hoony_nomination && isHoonyModeDuel())
	{
		g_edicts[spawn->hoony_nomination].hoony_nomination = 0;
	}
	spawn->hoony_nomination = 0;
}

static void HM_select_spawn(gedict_t *spawn, gedict_t *player, int effects)
{
	if (spawn->wizard)
	{
		spawn->wizard->s.v.effects = (int) spawn->wizard->s.v.effects | effects;
	}
	else
	{
		Spawn_OnePoint(spawn, spawn->s.v.origin, effects);
	}

	if (isHoonyModeDuel())
	{
		if (player)
		{
			spawn->hoony_nomination = NUM_FOR_EDICT(player);
			player->hoony_nomination = NUM_FOR_EDICT(spawn);
		}
	}
	else
	{
		spawn->hoony_nomination = (effects == EF_RED ? 1 : 2);
	}
}

void HM_pick_spawn(void)
{
	gedict_t *spawn = world;
	gedict_t *closest = world;
	gedict_t *old_nomination = world;
	int spawn_count = 0;
	int closest_spawn_num = 0;                  // count spawns and use when spawns are unnamed
	int self_num = NUM_FOR_EDICT(self);
	float closest_distance = 9999999.9f;
	int teamflag = 0;
	int red_spawns = 0, blue_spawns = 0;

	if (!isHoonyModeDuel())
	{
		char *team = getteam(self);
		if (streq(team, "red"))
		{
			teamflag = 1;
		}
		else if (streq(team, "blue"))
		{
			teamflag = 2;
		}
		else
		{
			G_sprint(self, 2, "Command only available in %s duel mode.\n", redtext("hoonymode"));

			return;
		}
	}

	if (match_in_progress || intermission_running)
	{
		G_sprint(self, 2, "Command not available during game.\n");

		return;
	}

	for (spawn = world; (spawn = ez_find(spawn, "info_player_deathmatch"));)
	{
		vec3_t difference;
		float distance = 0.0f;

		++spawn_count;

		VectorSubtract(spawn->s.v.origin, self->s.v.origin, difference);
		distance = VectorLength(difference);
		if ((closest == world) || (distance < closest_distance))
		{
			closest = spawn;
			closest_distance = distance;
			closest_spawn_num = spawn_count;
		}

		if (isHoonyModeDuel())
		{
			// Picking spawn for this individual
			if (spawn->hoony_nomination == self_num)
			{
				old_nomination = spawn;
			}
		}
		else
		{
			// Picking spawn for this team - don't allow too many
			if (spawn->hoony_nomination == 1)
			{
				++red_spawns;
			}
			else if (spawn->hoony_nomination == 2)
			{
				++blue_spawns;
			}
		}
	}

	if (closest == world)
	{
		G_sprint(self, 2, "No closest spawn found\n");

		return;
	}

	if ((closest == old_nomination) || (isHoonyModeTDM() && (closest->hoony_nomination == teamflag)))
	{
		// Duel only: we're picking for ourselves, so this unpicks the current option
		if (isHoonyModeDuel())
		{
			G_bprint(2, "%s opts for %s spawns\n", self->netname, redtext("random"));
		}
		else if (!strnull(closest->targetname))
		{
			G_bprint(2, "%s unpicks spawn %s for team \20%s\21\n", self->netname,
						redtext(closest->targetname), teamflag == 1 ? "red" : "blue");
		}
		else
		{
			G_bprint(2, "%s unpicks spawn #%d for team \20%s\21\n", self->netname,
						closest_spawn_num, teamflag == 1 ? "red" : "blue");
		}

		HM_deselect_spawn(closest);
	}
	else if (closest->hoony_nomination)
	{
		char *current = isHoonyModeDuel() ? g_edicts[closest->hoony_nomination].netname :
						closest->hoony_nomination == 1 ? "red" : "blue";

		if (!strnull(closest->targetname))
		{
			G_sprint(self, 2, "%s has already been picked by team \20%s\21\n",
						redtext(closest->targetname), current);
		}
		else
		{
			G_sprint(self, 2, "spawn #%d has already been picked by \20%s\21\n", closest_spawn_num,
						current);
		}
	}
	else
	{
		if ((teamflag == 1 ? red_spawns : blue_spawns) >= (cvar("maxclients") / 2))
		{
			G_sprint(self, PRINT_HIGH, "Team already has %d spawns allocated\n",
						(teamflag == 1 ? red_spawns : blue_spawns));

			return;
		}
		else
		{
			if (old_nomination != 0)
			{
				HM_deselect_spawn(old_nomination);
			}

			if (isHoonyModeDuel())
			{
				if (!strnull(closest->targetname))
				{
					G_bprint(2, "%s picks spawn %s\n", self->netname, redtext(closest->targetname));
				}
				else
				{
					G_bprint(2, "%s picks spawn #%d\n", self->netname, closest_spawn_num);
				}
			}
			else if (!strnull(closest->targetname))
			{
				G_bprint(2, "%s picks spawn %s for team \20%s\21\n", self->netname,
							redtext(closest->targetname), teamflag == 1 ? "red" : "blue");
			}
			else
			{
				G_bprint(2, "%s picks spawn #%d for team \20%s\21\n", self->netname,
							closest_spawn_num, teamflag == 1 ? "red" : "blue");
			}

			HM_select_spawn(closest, self,
							isHoonyModeDuel() ? EF_GREEN : teamflag == 1 ? EF_RED : EF_BLUE);
		}
	}

	HM_store_spawns();
}

void HM_unpick_all_spawns(void)
{
	gedict_t *spawn;

	for (spawn = world; (spawn = ez_find(spawn, "info_player_deathmatch"));)
	{
		HM_deselect_spawn(spawn);
	}

	HM_store_spawns();
}

// If this returns NULL, standard ktx spawn-picking rules will be applied
gedict_t* HM_choose_spawn_point(gedict_t *self)
{
	if (self->k_hoony_new_spawn)
	{
		if ((match_in_progress == 2) && isHoonyModeDuel())
		{
			return self->k_hoony_new_spawn;
		}
		else if (match_in_progress == 2)
		{
			// tdm: return selected spawn on initial spawn, then default behaviour
			if (match_start_time == g_globalvars.time)
			{
				return self->k_hoony_new_spawn;
			}

			return NULL;
		}
		else
		{
			// Go through spawns sequentially when pre-game
			gedict_t *spot = ez_find(self->k_hoony_new_spawn, "info_player_deathmatch");
			if (spot == NULL)
			{
				spot = ez_find(world, "info_player_deathmatch");
			}

			return spot;
		}
	}

	// We haven't allocated a spawnpoint, so just act normal
	return NULL;
}

// The player was spawned at a particular point, update if necessary
void HM_log_spawn_point(gedict_t *player, gedict_t *spawn)
{
	if (match_in_progress == 0)
	{
		player->k_hoony_new_spawn = spawn;
	}
}

void HM_rounds_adjust(int change)
{
	int rounds = HM_rounds();
	int new_rounds = bound(2, HM_rounds() + change * 2, 20);

	cvar_fset("k_hoonyrounds", new_rounds);

	if (new_rounds == rounds)
	{
		G_sprint(self, 2, "%s still %s\n", redtext("roundlimit"), dig3(new_rounds));
	}
	else
	{
		G_bprint(2, "%s %s\n", redtext("Roundlimit set to"), dig3(new_rounds));
	}
}

void HM_point_stats(void)
{
	gedict_t *p;
	int red_frags = 0;
	int blue_frags = 0;
	int prev_red_frags = 0;
	int prev_blue_frags = 0;
	int red_last_round = 0;
	int blue_last_round = 0;

	if (!isTeam())
	{
		// In duels, HM_next_point stores point data as death occurs
		return;
	}

	for (p = world; (p = find_plr(p));)
	{
		char *team = getteam(p);
		if (streq(team, "red"))
		{
			prev_red_frags += p->hoony_prevfrags;
			red_frags += p->s.v.frags;
		}
		else if (streq(team, "blue"))
		{
			prev_blue_frags += p->hoony_prevfrags;
			blue_frags += p->s.v.frags;
		}

		if (round_number)
		{
			p->hoony_results[round_number - 1] = p->s.v.frags - p->hoony_prevfrags;
		}
		p->hoony_prevfrags = p->s.v.frags;
	}

	red_last_round = red_frags - prev_red_frags;
	blue_last_round = blue_frags - prev_blue_frags;

	if (red_last_round == blue_last_round)
	{
		strlcpy(round_explanation, "Round was a draw!\n", sizeof(round_explanation));
	}
	else if (red_last_round > blue_last_round)
	{
		snprintf(round_explanation, sizeof(round_explanation), "\20%s\21 won round by %d frag%s\n",
					redtext("red"), red_last_round - blue_last_round,
					red_last_round - blue_last_round > 1 ? "s" : "");
	}
	else
	{
		snprintf(round_explanation, sizeof(round_explanation), "\20%s\21 won round by %d frag%s\n",
					redtext("blue"), blue_last_round - red_last_round,
					blue_last_round - red_last_round > 1 ? "s" : "");
	}

	if (red_frags == blue_frags)
	{
		strlcpy(series_explanation, "Series is currently tied.\n", sizeof(series_explanation));
	}
	else if (red_frags > blue_frags)
	{
		snprintf(series_explanation, sizeof(series_explanation),
					"\20%s\21 leads series by %d frag%s\n", redtext("red"), red_frags - blue_frags,
					red_frags - blue_frags > 1 ? "s" : "");
	}
	else
	{
		snprintf(series_explanation, sizeof(series_explanation),
					"\20%s\21 leads series by %d frag%s\n", redtext("blue"), blue_frags - red_frags,
					blue_frags - red_frags > 1 ? "s" : "");
	}

	G_bprint(PRINT_HIGH, round_explanation);
	G_bprint(PRINT_HIGH, series_explanation);
}

const char* HM_round_explanation(void)
{
	return round_explanation;
}

const char* HM_series_explanation(void)
{
	return series_explanation;
}

void HM_roundsup(void)
{
	if (!isHoonyModeAny())
	{
		G_sprint(self, PRINT_HIGH, "Command only available in hoonymode\n");
	}
	else if (!match_in_progress)
	{
		HM_rounds_adjust(1);
	}
}

void HM_roundsdown(void)
{
	if (!isHoonyModeAny())
	{
		G_sprint(self, PRINT_HIGH, "Command only available in hoonymode\n");
	}
	else if (!match_in_progress)
	{
		HM_rounds_adjust(-1);
	}
}

void HM_restore_spawns(void)
{
	char *entityFile = cvar_string("k_entityfile");

	// Leave remembering spawn for individual players for now...
	//   re-establish as the player re-joins server
	if (!isHoonyModeTDM())
	{
		return;
	}

	if (streq(cvar_string("k_hoonymode_prevmap"),
				strnull(entityFile) ? g_globalvars.mapname : entityFile))
	{
		char *spawns = cvar_string("k_hoonymode_prevspawns");
		int spawn_count = 0;
		gedict_t *spawn;

		for (spawn = world; (spawn = ez_find(spawn, "info_player_deathmatch"));)
		{
			++spawn_count;
		}

		if (strlen(spawns) == spawn_count)
		{
			spawn_count = 0;
			for (spawn = world; (spawn = ez_find(spawn, "info_player_deathmatch")); spawn_count++)
			{
				if (spawns[spawn_count] == '1')
				{
					HM_select_spawn(spawn, NULL, EF_RED);
				}
				else if (spawns[spawn_count] == '2')
				{
					HM_select_spawn(spawn, NULL, EF_BLUE);
				}
				else
				{
					HM_deselect_spawn(spawn);
				}
			}
		}
		else
		{
			cvar_set("k_hoonymode_prevspawns", "");
		}
	}
	else
	{
		cvar_set("k_hoonymode_prevspawns", "");
	}
}

static void HM_store_spawns(void)
{
	char *entityFile = cvar_string("k_entityfile");
	char buffer[128];
	gedict_t *spawn;
	char *next;

	next = buffer;
	for (spawn = world; (spawn = ez_find(spawn, "info_player_deathmatch"));)
	{
		*next++ = '0' + spawn->hoony_nomination;
	}

	*next = '\0';

	cvar_set("k_hoonymode_prevmap", strnull(entityFile) ? g_globalvars.mapname : entityFile);
	cvar_set("k_hoonymode_prevspawns", buffer);
}

void HM_match_break(void)
{
	match_break = true;
}
