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

// todo: fix lg door/dm2 floating/etc
// todo: test drop/rejoin, and how it affects spawn memory
// todo: test name change during game and how it affects lastscores
// todo: reset more items on map (e.g. lg door on dm6)
// todo: add newline + [0...max_line+2] every 8 points in HM_lastscores_extra
// todo: lastspawn and extra[] should really be part of gedict_t
// todo: tdm hoonymode (like clan arena except no dmm4!)

#include "g_local.h"

#define HM_RESULT_NOTPLAYED    0
#define HM_RESULT_WONROUND     1
#define HM_RESULT_LOSTROUND    2
#define HM_RESULT_SUICIDELOSS  3
#define HM_RESULT_SUICIDEWIN   4
#define HM_RESULT_DRAWWIN      5

#define HM_MIN_POINTS 6
#define HM_WINNING_DIFF 2

qbool isHoonyMode()
{
	return (isDuel() && cvar("k_hoonymode"));
}

static int round_number = 0;

void HM_initialise_rounds()
{
	round_number = 0;
}

// hoonymode: suicide, etc, count as a point for other players
void HM_suicide(gedict_t* player)
{
	gedict_t *p;

	player->hoony_results[round_number] = HM_RESULT_SUICIDELOSS;

	for (p = world; (p = find_plr(p));) {
		if (p != player) {
			p->s.v.frags += 1; 
			p->hoony_results[round_number] = HM_RESULT_SUICIDEWIN;
		}
	}

	EndMatch( 0 );
}

// Timelimit hit
void HM_draw()
{
	gedict_t* p;

	// .ent file can dictate that one player wins by default
	if (! strnull(world->hoony_defaultwinner)) {
		for (p = world; p = find_plr(p);) {
			if (p->k_hoony_new_spawn && streq(p->k_hoony_new_spawn->s.v.targetname, world->hoony_defaultwinner)) {
				p->s.v.frags++;
				G_bprint(2, "%s wins the round on time.\n", p->s.v.netname);
				EndMatch( 0 );
				return;
			}
		}
	}

	// If in normal rounds, everyone gets a point, so we get closer to finishing.  
	//   Otherwise ignore round and go on to next one
	if (round_number < HM_MIN_POINTS)
	{
		gedict_t* p;

		for (p = world; p = find_plr(p); )
		{
			p->hoony_results[round_number] = HM_RESULT_DRAWWIN;
			p->s.v.frags++;
		}

		++round_number;
	}
	G_bprint(2, "This round ends in a draw\n");

	EndMatch(0);
}

void HM_next_point(gedict_t *winner, gedict_t *loser)
{
	winner->hoony_results[round_number] = HM_RESULT_WONROUND;
	loser->hoony_results[round_number]  = HM_RESULT_LOSTROUND;
	
	if (round_number < HM_MAX_ROUNDS - 1)
		++round_number;

	EndMatch(0);
}

int HM_current_point()
{
	return round_number;
}

// [80] = 5 points per line (plus maybe a vs_blurb), times 30/3 = 50 points max
static char hm_stat_lines[30][80];
#define HM_PTS_PER_STAT_LINE 5

void HM_stats_show()
{
	int previous_point = HM_current_point() - 1;
	int i = 0;

	if (previous_point == -1)
	{
		G_sprint(self, 2, "no statistics until end of first point\n");
		return;
	}

	while (previous_point >= 0)
	{
		G_sprint(self, 2,va("%s\n%s\n%s\n"
			,hm_stat_lines[i]
			,hm_stat_lines[i+1]
			,hm_stat_lines[i+2]
		) );
		previous_point -= HM_PTS_PER_STAT_LINE;
		i += 3;
	}
	G_sprint(self,2,"\235\236\236\236\236\236\236\236\236\237\n");
}

void HM_stats()
{
	gedict_t *p;
	int previous_point = HM_current_point() - 1;

	struct {float lg; int dh; int hp;} hm_stat[2];

	int i = 0;
	int line_set = (previous_point / HM_PTS_PER_STAT_LINE) * 3;

	char vs_blurb[40] = {0};

	for (p = world; (p = find_plr(p));)
	{
		if (previous_point % HM_PTS_PER_STAT_LINE == 0) 
			strlcat(vs_blurb, va("%s%s", p->s.v.netname, i == 0 ? " - " : "\n"), sizeof(vs_blurb));
		hm_stat[i].lg = bound(0.0, 100.0 * p->ps.wpn[wpLG].hits / max(1, p->ps.wpn[wpLG].attacks), 99.0);
		hm_stat[i].dh = p->ps.wpn[wpRL].hits + p->ps.wpn[wpGL].hits;
		if (p->s.v.health <= 0) // avoid calculating something like (hp = -10) + (armor = 100) ...
			hm_stat[i].hp = 0;
		else // is armor ever negative? who knows...
			hm_stat[i].hp = bound(0, bound(0,p->s.v.armorvalue,99) + p->s.v.health,99);
		++i;
	}

	if (previous_point % HM_PTS_PER_STAT_LINE == 0)
	{
		if (previous_point == 0) 
			hm_stat_lines[0][0] = hm_stat_lines[1][0] = hm_stat_lines[2][0] = 0;
		strlcat(hm_stat_lines[0+line_set], va("%sLG: ", vs_blurb), sizeof(hm_stat_lines[0])); // don't need to add to sizeof()s since its static array
		strlcat(hm_stat_lines[1+line_set], "DH: ", sizeof(hm_stat_lines[1]));
		strlcat(hm_stat_lines[2+line_set], "HP: ", sizeof(hm_stat_lines[2]));
	}

	strlcat(hm_stat_lines[0+line_set], va("%02.0f-%02.0f ", hm_stat[0].lg, hm_stat[1].lg), sizeof(hm_stat_lines[0]));
	strlcat(hm_stat_lines[1+line_set], va("%02d-%02d ", hm_stat[0].dh, hm_stat[1].dh), sizeof(hm_stat_lines[1]));
	strlcat(hm_stat_lines[2+line_set], va("%02d-%02d ", hm_stat[0].hp, hm_stat[1].hp), sizeof(hm_stat_lines[2]));
}

int HM_current_point_type()
{
	gedict_t *p;
	int maxfrags = -999, minfrags = 999;
	int fragdiff = 0;

	for (p = world; (p = find_plr(p));) {
		maxfrags = max(maxfrags, p->s.v.frags);
		minfrags = min(minfrags, p->s.v.frags);
	}
	fragdiff = maxfrags - minfrags;

	if (maxfrags >= HM_MIN_POINTS && fragdiff >= HM_WINNING_DIFF)
		return HM_PT_FINAL; // as used in the code (match.c) HM_PT_FINAL == "last point was final"

	if (maxfrags >= HM_MIN_POINTS - 1 && fragdiff >= HM_WINNING_DIFF - 1)
		return HM_PT_SET; // as used in the code, HM_PT_SET == "this point is set point"

	return 0;
}

void SUB_regen();

void remove_items(char* classname)
{
	gedict_t *p;

	for (p = world; (p = find(p, FOFCLSN, classname)); /**/) 
		ent_remove(p);
}

void respawn_items(char* classname, qbool enabled)
{
	gedict_t *p;

	if ( strnull( classname ) )
		G_Error("respawn_items");

	for( p = world; (p = find(p, FOFCLSN, classname)); /**/ )
	{
		if (enabled)
		{
			if (p->initial_spawn_delay > 0)
			{
				// hide, but respawn at future point
				p->s.v.model = NULL;
				p->s.v.solid = SOLID_NOT;
				p->s.v.nextthink = g_globalvars.time + p->initial_spawn_delay;
				p->s.v.think = ( func_t ) SUB_regen;
			}
			else if (strnull( p->s.v.model ) || p->s.v.solid != SOLID_TRIGGER)
			{
				// respawn now
				p->s.v.nextthink = g_globalvars.time;
				p->s.v.think = ( func_t ) SUB_regen;
			}
		}
		else 
		{
			// hide item
			p->s.v.model = NULL;
			p->s.v.solid = SOLID_NOT;
			p->s.v.nextthink = 0; 
		}
	}
}

void HM_reset_map()
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

	respawn_items("item_artifact_invulnerability", false);
	respawn_items("item_artifact_super_damage", false);
	respawn_items("item_artifact_envirosuit", false);
	respawn_items("item_artifact_invisibility", false);

	// remove temporary objects
	remove_projectiles();
	remove_items("backpack");
}

static qbool SpawnAlreadyAllocated(gedict_t* spawn)
{
	gedict_t* p;

	for (p = world; p = find_plr(p); /**/) {
		if (p->k_hoony_new_spawn == spawn)
			return true;
	}

	return false;
}

static void HM_rig_spawn(gedict_t* player)
{
	if (HM_current_point() % 2 == 1)
	{
		// Swap spawn points with next player
		gedict_t* p = find_plr(player);

		if (p == NULL)
			p = find_plr(world);
		
		player->k_hoony_new_spawn = p->k_hoonyspawn;
	}
	else 
	{
		// SelectSpawnPoint() is used to us placing the player there immediately, which we are not doing,
		//   so try and avoid picking the same point for two people manually.
		int i;

		for (i = 0; i < 10; ++i)
		{
			gedict_t* attempt = SelectSpawnPoint("info_player_deathmatch");

			if (! SpawnAlreadyAllocated(attempt)) {
				player->k_hoony_new_spawn = player->k_hoonyspawn = attempt;
				break;
			}
		}

		if (! player->k_hoony_new_spawn)
		{
			// oh dear - go sequential...
			gedict_t* attempt = SelectSpawnPoint("info_player_deathmatch");

			if (SpawnAlreadyAllocated(attempt))
			{
				attempt = find(attempt, FOFCLSN, "info_player_deathmatch");

				// go back to the start
				if (attempt == NULL)
					attempt = find(world, FOFCLSN, "info_player_deathmatch");

				// should never happen...
				if (attempt == NULL)
					attempt = SelectSpawnPoint("info_player_deathmatch");	
			}
			player->k_hoony_new_spawn = player->k_hoonyspawn = attempt;	
		}
	}
}

void HM_all_ready()
{
	gedict_t* p;

	// Clear all spawns for the current round
	for (p = world; p = find_plr(p); /**/) {
		p->k_hoony_new_spawn = NULL;
	}

	// TODO: at start of match: 
	// - put all spawns in array, shuffle order
	// - shuffle order and then assign to players/teams?

	// Assign players to spawns
	for (p = world; p = find_plr(p); /**/) {
		HM_rig_spawn(p);
	}
}

char *HM_lastscores_extra ()
{
	//extra[0] = 0;
	//strlcat(extra, va("\n%s\n%s", extra1, extra2), sizeof(extra));
	//return extra;
	return "";
}
