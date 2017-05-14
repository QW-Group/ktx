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

#define HM_WINNING_DIFF        2

static int round_number = 0;

static void EndRound()
{
	if (round_number < HM_MAX_ROUNDS - 1)
		++round_number;
	else
		--round_number;	// Start over-writing to stop buffer over-run

	EndMatch(0);
}

qbool isHoonyMode()
{
	return (isDuel() && cvar("k_hoonymode"));
}

void HM_initialise_rounds()
{
	round_number = 0;
}

int HM_timelimit()
{
	if (timelimit)
		return timelimit * 60;

	return world->hoony_timelimit;
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

	EndRound();
}

// Timelimit hit
void HM_draw()
{
	gedict_t* p;
	int maxfrags = -9999, minfrags = 9999;

	for (p = world; (p = find_plr(p));) {
		// .ent file can dictate that one player wins by default
		if (! strnull(world->hoony_defaultwinner) && p->k_hoony_new_spawn && streq(p->k_hoony_new_spawn->s.v.targetname, world->hoony_defaultwinner)) {
			p->s.v.frags++;
			G_bprint(2, "%s wins the round on time.\n", p->s.v.netname);
			EndRound();
			return;
		}

		maxfrags = max(p->s.v.frags, maxfrags);
		minfrags = min(p->s.v.frags, minfrags);
	}

	if (maxfrags < fraglimit)
	{
		// If in normal rounds, everyone gets a point, so we get closer to finishing.  
		gedict_t* p;

		for (p = world; (p = find_plr(p)); )
		{
			p->hoony_results[round_number] = HM_RESULT_DRAWWIN;
			p->s.v.frags++;
		}
	}

	// We go again...
	G_bprint(2, "This round ends in a draw\n");
	EndRound();
}

qbool HM_is_game_over()
{
	// Game is over if we've hit frag limit and one player is in lead (HM_PT_FINAL), or
	//                 one player is one frag ahead at the start of a round of spawns and we're past the fraglimit
	if (HM_current_point_type() == HM_PT_FINAL)
		return true;

	if (HM_current_point_type() == HM_PT_SET && HM_current_point() % 2 == 0)
	{
		gedict_t* p;
		int maxfrags = -999;
		int minfrags = 999;

		for (p = world; (p = find_plr(p));)
		{
			maxfrags = max(p->s.v.frags, maxfrags);
			minfrags = min(p->s.v.frags, minfrags);
		}

		return maxfrags != minfrags && maxfrags >= fraglimit;
	}

	return false;
}

void HM_next_point(gedict_t *winner, gedict_t *loser)
{
	winner->hoony_results[round_number] = HM_RESULT_WONROUND;
	loser->hoony_results[round_number]  = HM_RESULT_LOSTROUND;
	
	EndRound();
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

	if (maxfrags >= fraglimit && fragdiff >= HM_WINNING_DIFF)
		return HM_PT_FINAL; // as used in the code (match.c) HM_PT_FINAL == "last point was final"

	if (maxfrags >= fraglimit - 1 && fragdiff >= HM_WINNING_DIFF - 1)
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

static int HM_spawn_comparison(const void* lhs_, const void* rhs_)
{
	const gedict_t* lhs = *(gedict_t**) lhs_;
	const gedict_t* rhs = *(gedict_t**) rhs_;

	if (lhs->hoony_spawn_order < rhs->hoony_spawn_order)
		return -1;
	else if (lhs->hoony_spawn_order > rhs->hoony_spawn_order)
		return 1;

	return 0;
}

static void HM_shuffle_spawns(gedict_t** spawns, int count)
{
	int i = 0;
	int j = 0;

	for (i = 0; i < 6; ++i)
	{
		for (j = 0; j < count; ++j)
			spawns[j]->hoony_spawn_order = (spawns[j]->hoony_nomination ? -1 : 1) * i_rnd(1, 100);

		qsort(spawns, count, sizeof(gedict_t*), HM_spawn_comparison);
	}
}

// This is called at the start of each round
void HM_all_ready()
{
	gedict_t* p;

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
			gedict_t* red_spawns[MAX_CLIENTS] = { 0 };
			gedict_t* blue_spawns[MAX_CLIENTS] = { 0 };

			int red_spawncount = 0;
			int blue_spawncount = 0;
			int red_assigned_spawn = 0;
			int blue_assigned_spawn = 0;

			// Allocate specific red/blue spawns accordingly
			for (p = world; (p = ez_find(p, "info_player_team1")) && red_spawncount < MAX_CLIENTS; )
				red_spawns[red_spawncount++] = p;
			for (p = world; (p = ez_find(p, "info_player_team2")) && blue_spawncount < MAX_CLIENTS; )
				blue_spawns[blue_spawncount++] = p;

			// Allocate standard deathmatch spawns
			for (p = world; (p = ez_find(p, "info_player_deathmatch")) && (red_spawncount < MAX_CLIENTS || blue_spawncount < MAX_CLIENTS); )
			{
				if (red_spawncount < MAX_CLIENTS && red_spawncount < blue_spawncount)
					red_spawns[red_spawncount++] = p;
				else if (blue_spawncount < MAX_CLIENTS)
					blue_spawns[blue_spawncount++] = p;
			}

			// Shuffle
			HM_shuffle_spawns(red_spawns, red_spawncount);
			HM_shuffle_spawns(blue_spawns, blue_spawncount);

			// Assign based on team
			for (p = world; (p = find_plr(p)); /**/) 
			{
				if (red_assigned_spawn == (int) min(red_spawncount, MAX_CLIENTS))
					red_assigned_spawn = 0;
				if (blue_assigned_spawn == (int) min(blue_spawncount, MAX_CLIENTS))
					blue_assigned_spawn = 0;

				if (streq(getteam(self), "red"))
					p->k_hoony_new_spawn = p->k_hoonyspawn = red_spawns[red_assigned_spawn++];
				else
					p->k_hoony_new_spawn = p->k_hoonyspawn = blue_spawns[blue_assigned_spawn++];
			}
		}
		else 
		{
			gedict_t* spawns[MAX_CLIENTS] = { 0 };
			int spawncount = 0;
			int assigned_spawn = 0;

			for (p = world; (p = ez_find(p, "info_player_deathmatch")) && spawncount < MAX_CLIENTS; )
				spawns[spawncount++] = p;
			HM_shuffle_spawns(spawns, spawncount);

			// assign as standard
			for (p = world; (p = find_plr(p)); /**/) 
			{
				if (assigned_spawn == (int) min(spawncount, MAX_CLIENTS))
					assigned_spawn = 0;

				p->k_hoony_new_spawn = p->k_hoonyspawn = spawns[assigned_spawn++];
			}
		}
	}
	else 
	{
		// on odd-numbered rounds, spawn spawn points
		if (isTeam())
		{
			//gedict_t* red_players[MAX_CLIENTS] = { 0 };
			//gedict_t* blue_players[MAX_CLIENTS] = { 0 };
		}
		else 
		{
			for (p = world; (p = find_plr(p)); /**/)
			{
				gedict_t* next = find_plr(p);

				if (next == NULL)
					next = find_plr(world);

				p->k_hoony_new_spawn = next->k_hoonyspawn;
			}
		}
	}
}

char *HM_lastscores_extra (void)
{
	//extra[0] = 0;
	//strlcat(extra, va("\n%s\n%s", extra1, extra2), sizeof(extra));
	//return extra;
	return "";
}

// Naming spawns
// - Spawns can have name set in map or .ent file
//   Hardcoded names for TB3/TB5 - could go away in future

typedef struct hm_spawn_name_t {
	vec3_t origin;
	char* name;
} hm_spawn_name;

static void HM_name_spawn(gedict_t* spawn, hm_spawn_name* spawns, int spawncount)
{
	if (strnull(spawn->s.v.targetname))
	{
		int i = 0;
		for (i = 0; i < spawncount; ++i)
		{
			if (VectorCompare(spawn->s.v.origin, spawns[i].origin))
			{
				spawn->s.v.targetname = spawns[i].name;
				break;
			}
		}
	}
}

void HM_name_map_spawn(gedict_t* spawn)
{
	char* mapname = g_globalvars.mapname;

	if (streq(mapname, "dm2"))
	{
		hm_spawn_name spawns[] = {
			{ { 2560,  -192,   32 }, "nailgun" },
			{ { 2544,   -32,  -64 }, "low-tele" },
			{ { 2624,  -488,   32 }, "water" },
			{ { 2176, -2176,   88 }, "ra-mega btn" },
			{ { 2576, -1328,   24 }, "low-rl" },
			{ { 2704, -2048,  128 }, "ra-mega" },
			{ { 1712,  -504,   24 }, "under quad" },
			{ { 2048, -1352,  136 }, "big (stairs)" },
			{ { 2248,   -16, -136 }, "low-tele (btn)" }
		};

		HM_name_spawn(spawn, spawns, sizeof(spawns) / sizeof(spawns[0]));
	}
	else if (streq(mapname, "dm3"))
	{
		hm_spawn_name spawns[] = {
			{ { -880, -232,  -16 }, "tele (sng)" },
			{ {  192, -208, -176 }, "ra tunnel" },
			{ { 1472, -928,  -24 }, "ya box" },
			{ { 1520,  432,  -88 }, "rl room" },
			{ { -632, -680,  -16 }, "tele (ra)" },
			{ {  512,  768,  216 }, "lifts" }
		};

		HM_name_spawn(spawn, spawns, sizeof(spawns) / sizeof(spawns[0]));
	}
	else if (streq(mapname, "dm4"))
	{
		hm_spawn_name spawns[] = {
			{ { -64,   512, -296 }, "mega-room" },
			{ { -64,  -232,  -72 }, "quad-tele" },
			{ { 272,  -952,   24 }, "high-tele" },
			{ { 112, -1136, -104 }, "ammo-room" },
			{ { 776,  -808, -232 }, "ra / rl" },
			{ { 784,  -176,   24 }, "ya-entry" }
		};

		HM_name_spawn(spawn, spawns, sizeof(spawns) / sizeof(spawns[0]));
	}
	else if (streq(mapname, "dm6"))
	{
		hm_spawn_name spawns[] = {
			{ {  232, -1512,  40 }, "ga tele" },
			{ { 1016,  -416,  40 }, "low rl" },
			{ {    0, -1088, 264 }, "high rl" },
			{ {  456, -1504, 256 }, "above sng" },
			{ {  408, -1088, 256 }, "gl > sng" },
			{ {  896, -1464, 256 }, "gl room" },
			{ { 1892,  -160, 168 }, "behind ra" }
		};

		HM_name_spawn(spawn, spawns, sizeof(spawns) / sizeof(spawns[0]));
	}
	else if (streq(mapname, "e1m2"))
	{
		hm_spawn_name spawns[] = {
			{ { -416,  -144, 320 }, "mega room" },
			{ {  168,  -480, 320 }, "mega entrance" },
			{ { 1496,  1328, 200 }, "start" },
			{ { 1936,  -136, 312 }, "nail traps" },
			{ {  936, -1216, 432 }, "quad -> gl" },
			{ {  792,  -992, 440 }, "gl -> quad" },
			{ { 1080,  -720, 312 }, "ya area" },
			{ {  408,  -752, 432 }, "quad room" },
			{ {  792,  -208, 320 }, "doors" },
			{ {  784,   808, 206 }, "rl room" }
		};

		HM_name_spawn(spawn, spawns, sizeof(spawns) / sizeof(spawns[0]));
	}
	else if (streq(mapname, "ztndm3"))
	{
		hm_spawn_name spawns[] = {
			{ {  -432,  128,  32 }, "ra (nailgun)" },
			{ { -1056, -544, 224 }, "gl/corner" },
			{ {  -208, -400, 224 }, "ya room" },
			{ {  -320,  576, 224 }, "quad room" },
			{ {  -176, -112, 288 }, "ra (ssg)" },
			{ {   -96,  224,  32 }, "lg tunnel" },
			{ {  -800,    0, -32 }, "ra (rl/mega)" }
		};

		HM_name_spawn(spawn, spawns, sizeof(spawns) / sizeof(spawns[0]));
	}
	else if (streq(mapname, "aerowalk"))
	{
		hm_spawn_name spawns[] = {
			{ { -224, -720, 256 }, "ga (gl/quad)" },
			{ { -488,  624, 264 }, "ga (low rl)" },
			{ { -224, -704, 456 }, "high rl" },
			{ { -272,   24,  40 }, "big (ssg)" },
			{ { -320,  480, 456 }, "ra platform" },
			{ {  160,  128, 256 }, "air tunnel" }
		};

		HM_name_spawn(spawn, spawns, sizeof(spawns) / sizeof(spawns[0]));
	}
}

// Picking spawns
// - In pregame, allow players to pick spawn points to use during the game
//   If player doesn't nominate a spawn point, 'their' spawn will be randomly chosen each round
gedict_t *Spawn_OnePoint( gedict_t* spawn_point, vec3_t org, int effects );

static void HM_deselect_spawn(gedict_t* spawn)
{
	int effects = (EF_GREEN | EF_RED);

	if (! spawn->wizard)
		return;

	// If showing all spawns, just remove the glow.  otherwise remove the marker.
	if (cvar( "k_spm_show" ))
	{
		spawn->wizard->s.v.effects = (int) spawn->wizard->s.v.effects & ~effects;
	}
	else 
	{
		ent_remove(spawn->wizard);
		spawn->wizard = 0;
	}

	if (spawn->hoony_nomination)
		g_edicts[spawn->hoony_nomination].hoony_nomination = 0;
	spawn->hoony_nomination = 0;
}

static void HM_select_spawn(gedict_t* spawn, gedict_t* player)
{
	int effects = (EF_GREEN | EF_RED);

	if (spawn->wizard)
	{
		spawn->wizard->s.v.effects = (int) spawn->wizard->s.v.effects | effects;
	}
	else 
	{
		Spawn_OnePoint(spawn, spawn->s.v.origin, effects);
	}

	spawn->hoony_nomination = NUM_FOR_EDICT(player);
	player->hoony_nomination = NUM_FOR_EDICT(spawn);
}

void HM_pick_spawn(void)
{
	gedict_t* spawn = world;
	gedict_t* closest = world;
	gedict_t* old_nomination = world;
	int spawn_count = 0;
	int closest_spawn_num = 0;                  // count spawns and use when spawns are unnamed
	int self_num = NUM_FOR_EDICT(self);
	float closest_distance = 9999999.9f;

	if (! isHoonyMode())
	{
		G_sprint(self, 2, "Command only available in %s.\n", redtext("hoonymode"));
		return;
	}

	if (match_in_progress || intermission_running) 
	{
		G_sprint(self, 2, "Command not available during game.\n");
		return;
	}

	for (spawn = world; (spawn = ez_find(spawn, "info_player_deathmatch")); )
	{
		vec3_t difference;
		float distance = 0.0f;
		
		++spawn_count;
		if (spawn->hoony_nomination == self_num)
			old_nomination = spawn;

		VectorSubtract(spawn->s.v.origin, self->s.v.origin, difference);
		distance = VectorLength(difference);

		if (closest == world || distance < closest_distance)
		{
			closest = spawn;
			closest_distance = distance;
			closest_spawn_num = spawn_count;
		}
	}

	if (closest == world)
	{
		G_sprint(self, 2, "No closest spawn found\n");
		return;
	}

	if (closest == old_nomination) 
	{
		G_bprint(2, "%s opts for %s spawns\n", self->s.v.netname, redtext("random"));
		
		HM_deselect_spawn(closest);
	}
	else if (closest->hoony_nomination)
	{
		if (! strnull(closest->s.v.targetname))
			G_sprint(self, 2, "%s has already nominated %s\n", g_edicts[closest->hoony_nomination].s.v.netname, closest->s.v.targetname);
		else
			G_sprint(self, 2, "%s has already nominated spawn #%d\n", g_edicts[closest->hoony_nomination].s.v.netname, closest_spawn_num);
	}
	else
	{
		if (old_nomination != 0)
			HM_deselect_spawn(old_nomination);

		if (! strnull(closest->s.v.targetname))
			G_bprint(2, "%s picks spawn %s\n", self->s.v.netname, redtext(closest->s.v.targetname));
		else
			G_bprint(2, "%s picks spawn #%d\n", self->s.v.netname, closest_spawn_num);

		HM_select_spawn(closest, self);
	}
}
