#include "g_local.h"
#include "stats.h"

// TODO
// - Hoonymode stats: would be interesting to see spawn A vs B result... could always get from demo tho
// - 

//#define USER_FRIENDLY_JSON

#ifdef USER_FRIENDLY_JSON
#define INDENT2  "  "
#define INDENT4  "    "
#define INDENT6  "      "
#define INDENT8  "        "
#define INDENT10 "          "
#define INDENT12 "            "
#define JSON_CR  "\n"
#else
#define INDENT2
#define INDENT4
#define INDENT6
#define INDENT8
#define INDENT10
#define INDENT12
#define JSON_CR  ""
#endif

static void json_player_ctf_stats(fileHandle_t handle, player_stats_t *stats);
static void json_player_instagib_stats(fileHandle_t handle, player_stats_t *stats);
static void json_player_midair_stats(fileHandle_t handle, player_stats_t *stats);
static void json_player_ra_stats(fileHandle_t handle, player_stats_t *stats);
static void json_player_hoonymode_stats(fileHandle_t handle, gedict_t *player);
static void json_player_lgc_stats(fileHandle_t handle, gedict_t *player);

#define STATS_VERSION_NUMBER 3

#define SIMPLE_CHECK(handle, any, string) {\
	if (any) { \
		s2di(handle, string); \
	} \
	any = true; \
}

#define NEWLINE_CHECK(handle, any) SIMPLE_CHECK(handle, any, JSON_CR)
#define COMMA_CHECK(handle, any) SIMPLE_CHECK(handle, any, "," JSON_CR)

static char* json_string(const char *input)
{
	// >>>> like va(...) ... eugh
	static char string[MAX_STRINGS][1024];
	static int index = 0;
	char *ch, *start;

	index %= MAX_STRINGS;
	// <<<<

	start = ch = string[index++];
	while (*input)
	{
		unsigned char current = *input;

		if ((ch - start) >= 1000)
		{
			break;
		}

		if ((current == '\\') || (current == '"'))
		{
			*ch++ = '\\';
			*ch++ = current;
		}
		else if (current == '\n')
		{
			*ch++ = '\\';
			*ch++ = 'n';
		}
		else if (current == '\r')
		{
			*ch++ = '\\';
			*ch++ = 'r';
		}
		else if (current == '\b')
		{
			*ch++ = '\\';
			*ch++ = 'b';
		}
		else if (current == '\t')
		{
			*ch++ = '\\';
			*ch++ = 't';
		}
		else if (current == '\f')
		{
			*ch++ = '\\';
			*ch++ = 'f';
		}
		else if ((current < ' ') || (current >= 128))
		{
			*ch++ = '\\';
			*ch++ = 'u';
			*ch++ = '0';
			*ch++ = '0';
			if (current < 16)
			{
				*ch++ = '0';
				*ch++ = "0123456789ABCDEF"[(int)current];
			}
			else
			{
				*ch++ = "0123456789ABCDEF"[((int)(current)) >> 4];
				*ch++ = "0123456789ABCDEF"[((int)(current)) & 15];
			}
		}
		else
		{
			*ch++ = current;
		}

		++input;
	}

	*ch = '\0';

	return start;
}

static void json_weap_header(fileHandle_t handle)
{
	s2di(handle, INDENT6 "\"weapons\": {" JSON_CR);
}

static void json_weap_detail(fileHandle_t handle, const char *name, int weapon_num, wpType_t *stats)
{
	qbool any = false;

	if (weapon_num)
	{
		s2di(handle, "," JSON_CR);
	}

	s2di(handle, INDENT8 "\"%s\": {" JSON_CR, json_string(name));
	if (stats->attacks || stats->hits || stats->rhits || stats->vhits)
	{
		s2di(handle, INDENT10 "\"acc\": { \"attacks\": %d, \"hits\": %d", stats->attacks,
				stats->hits);
		if (stats->rhits || stats->vhits)
		{
			s2di(handle, ", \"real\": %d, \"virtual\": %d", stats->rhits, stats->vhits);
		}

		s2di(handle, " }");
		any = true;
	}
	if (stats->kills || stats->tkills || stats->ekills || stats->suicides)
	{
		COMMA_CHECK(handle, any);
		s2di(handle,
				INDENT10 "\"kills\": { \"total\": %d, \"team\": %d, \"enemy\": %d, \"self\": %d }",
				stats->kills, stats->tkills, stats->ekills, stats->suicides);
	}

	if (deathmatch < 4)
	{
		COMMA_CHECK(handle, any);
		s2di(handle, INDENT10 "\"deaths\": %d", stats->deaths);
	}

	if ((deathmatch < 4)
			&& (stats->drops || stats->tooks || stats->ttooks || stats->stooks || stats->sttooks))
	{
		qbool inner_any = false;
		COMMA_CHECK(handle, any);
		s2di(handle, INDENT10 "\"pickups\": {" JSON_CR);
		if (stats->drops)
		{
			COMMA_CHECK(handle, inner_any);
			s2di(handle, INDENT12 "\"dropped\": %d", stats->drops);
		}

		if (stats->tooks)
		{
			COMMA_CHECK(handle, inner_any);
			s2di(handle, INDENT12 "\"taken\": %d", stats->tooks);
		}

		if (stats->ttooks)
		{
			COMMA_CHECK(handle, inner_any);
			s2di(handle, INDENT12 "\"total-taken\": %d", stats->ttooks);
		}

		if (stats->stooks)
		{
			COMMA_CHECK(handle, inner_any);
			s2di(handle, INDENT12 "\"spawn-taken\": %d", stats->stooks);
		}

		if (stats->sttooks)
		{
			COMMA_CHECK(handle, inner_any);
			s2di(handle, INDENT12 "\"spawn-total-taken\": %d", stats->sttooks);
		}

		NEWLINE_CHECK(handle, inner_any);
		s2di(handle, INDENT10 "}");
	}

	if (stats->edamage || stats->tdamage)
	{
		COMMA_CHECK(handle, any);
		s2di(handle, INDENT10 "\"damage\": { \"enemy\": %d, \"team\": %d }", stats->edamage,
				stats->tdamage);
	}

	NEWLINE_CHECK(handle, any);
	s2di(handle, INDENT8 "}");
}

static void json_weap_footer(fileHandle_t handle, int num)
{
	if (num)
	{
		s2di(handle, JSON_CR);
	}

	s2di(handle, INDENT6 "}," JSON_CR);
}

void json_teams_header(fileHandle_t handle)
{
	char tmp[1024] =
		{ 0 };
	int i = 0;

	for (tmp[0] = i = 0; i < min(tmStats_cnt, MAX_TM_STATS); i++)
	{
		if (i)
		{
			strlcat(tmp, ", ", sizeof(tmp));
		}

		strlcat(tmp, "\"", sizeof(tmp));
		strlcat(tmp, json_string(tmStats[i].name), sizeof(tmp));
		strlcat(tmp, "\"", sizeof(tmp));
	}

	if (i)
	{
		s2di(handle, INDENT2 "\"teams\": [%s]," JSON_CR, tmp);
	}
}

void json_teams_footer(fileHandle_t handle, int teams)
{
}

void json_team_detail(fileHandle_t handle, int num, teamStats_t *stats)
{
}

void json_team_footer(fileHandle_t handle)
{
}

static void json_items_header(fileHandle_t handle)
{
	s2di(handle, INDENT6 "\"items\": {" JSON_CR);
}

static void json_item_detail(fileHandle_t handle, const char *name, int item_num, itType_t *stats)
{
	qbool any = false;

	if (item_num)
	{
		s2di(handle, "," JSON_CR);
	}

	s2di(handle, INDENT8 "\"%s\": { ", json_string(name));
	if (stats->tooks)
	{
		COMMA_CHECK(handle, any);
		s2di(handle, "\"took\": %d", stats->tooks);
	}

	if ((int)stats->time)
	{
		COMMA_CHECK(handle, any);
		s2di(handle, "\"time\": %d", (int)stats->time);
	}

	s2di(handle, " }");
}

static void json_items_footer(fileHandle_t handle, int item_num)
{
	if (item_num)
	{
		s2di(handle, "" JSON_CR);
	}

	s2di(handle, INDENT6 "}");
}

void json_players_header(fileHandle_t handle)
{
	s2di(handle, INDENT2 "\"players\": [" JSON_CR);
}

void json_players_footer(fileHandle_t handle, int player_count)
{
	if (player_count)
	{
		s2di(handle, JSON_CR);
	}

	s2di(handle, INDENT2 "]" JSON_CR);
}

#ifdef BOT_SUPPORT
void json_player_bot_info(fileHandle_t handle, fb_botskill_t* skill)
{
	s2di(handle, "," JSON_CR);
	s2di(handle, INDENT6 "\"bot\": {" JSON_CR);
	s2di(handle, INDENT8 "\"skill\": %d," JSON_CR, skill->skill_level);
	s2di(handle, INDENT8 "\"customised\": %s" JSON_CR, skill->customised ? "true" : "false");
	s2di(handle, INDENT6 "}");
}
#endif

void json_player_detail(fileHandle_t handle, int player_num, gedict_t *player, const char *team)
{
	int j;
	player_stats_t *stats = &player->ps;
	int count = 0;

	if (player_num)
	{
		s2di(handle, "," JSON_CR);
	}

	s2di(handle, INDENT4 "{" JSON_CR);
	s2di(handle, INDENT6 "\"top-color\": %d," JSON_CR, iKey(player, "topcolor"));
	s2di(handle, INDENT6 "\"bottom-color\": %d," JSON_CR, iKey(player, "bottomcolor"));
	s2di(handle, INDENT6 "\"ping\": %d," JSON_CR, iKey(player, "ping"));
	s2di(handle, INDENT6 "\"login\": \"%s\"," JSON_CR, ezinfokey(player, "login"));
	s2di(handle, INDENT6 "\"name\": \"%s\"," JSON_CR, json_string(getname(player)));
	s2di(handle, INDENT6 "\"team\": \"%s\"," JSON_CR, json_string(team));
	s2di(handle,
			INDENT6 "\"stats\": { \"frags\": %d, \"deaths\": %d, \"tk\": %d, \"spawn-frags\": %d, \"kills\": %d, \"suicides\": %d }," JSON_CR,
			(int)player->s.v.frags, (int)player->deaths, (int)player->friendly,
			player->ps.spawn_frags, (int)player->kills, (int)player->suicides);
	s2di(handle,
			INDENT6 "\"dmg\": { \"taken\": %d, \"given\": %d, \"team\": %d, \"self\": %d, \"team-weapons\": %d, \"enemy-weapons\": %d, \"taken-to-die\": %d }," JSON_CR,
			(int)player->ps.dmg_t, (int)player->ps.dmg_g, (int)player->ps.dmg_team,
			(int)player->ps.dmg_self, (int)player->ps.dmg_tweapon, (int)player->ps.dmg_eweapon,
			(int)player->deaths == 0 ? 99999 : (int)player->ps.dmg_t / (int)player->deaths);
	s2di(handle, INDENT6 "\"xfer\": %d," JSON_CR, player->ps.transferred_packs);
	s2di(handle, INDENT6 "\"spree\": { \"max\": %d, \"quad\": %d }," JSON_CR, player->ps.spree_max,
			player->ps.spree_max_q);
	s2di(handle, INDENT6 "\"control\": %f," JSON_CR, player->ps.control_time);
	s2di(handle, INDENT6 "\"speed\": { \"max\": %f, \"avg\": %f }," JSON_CR,
			player->ps.velocity_max,
			player->ps.vel_frames > 0 ? player->ps.velocity_sum / player->ps.vel_frames : 0.);
	if (GetHandicap(player) != 100)
	{
		s2di(handle, INDENT6 "\"handicap\": %d," JSON_CR, GetHandicap(player));
	}

	json_weap_header(handle);
	for (j = 1; j < wpMAX; j++)
	{
		wpType_t *weap = &stats->wpn[j];
		int old_ekills = weap->ekills;

		if ((deathmatch >= 4) || (j == wpAXE) || (j == wpSG) || cvar("k_instagib"))
		{
			weap->ekills = 0;
		}

		if (!((weap->attacks == 0) && (weap->deaths == 0) && (weap->drops == 0) && (weap->sttooks == 0)
				&& (weap->ttooks == 0)))
		{
			json_weap_detail(handle, WpName(j), count, weap);
			++count;
		}

		weap->ekills = old_ekills;
	}

	json_weap_footer(handle, count);

	count = 0;
	json_items_header(handle);
	for (j = 1; j < itMAX; j++)
	{
		itType_t *item = &stats->itm[j];

		if ((item->tooks != 0) || ((deathmatch <= 3) && (item->time != 0)))
		{
			json_item_detail(handle, ItName(j), count, item);
			++count;
		}
	}

	json_items_footer(handle, count);

	if (isCTF())
	{
		json_player_ctf_stats(handle, stats);
	}

	if (cvar("k_instagib"))
	{
		json_player_instagib_stats(handle, stats);
	}

	if (cvar("k_midair"))
	{
		json_player_midair_stats(handle, stats);
	}

	if (isRA())
	{
		json_player_ra_stats(handle, stats);
	}

	if (isHoonyModeAny())
	{
		json_player_hoonymode_stats(handle, player);
	}

	if (lgc_enabled())
	{
		json_player_lgc_stats(handle, player);
	}

#ifdef BOT_SUPPORT
	if (player->isBot)
	{
		json_player_bot_info(handle, &player->fb.skill);
	}
#endif

	s2di(handle, JSON_CR);
	s2di(handle, INDENT4 "}");
}

void json_match_header(fileHandle_t handle, char *ip, int port)
{
	char date[64] =
		{ 0 };
	char matchtag[64] =
		{ 0 };
	const char *mode = GetMode();
	int duration = (g_globalvars.time - match_start_time);

	infokey(world, "matchtag", matchtag, sizeof(matchtag));

	if (!QVMstrftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S %z", 0))
	{
		date[0] = 0; // bad date
	}

	s2di(handle, "{" JSON_CR);
	s2di(handle, INDENT2 "\"version\": %d," JSON_CR, STATS_VERSION_NUMBER);
	s2di(handle, INDENT2 "\"date\": \"%s\"," JSON_CR, date);
	s2di(handle, INDENT2 "\"map\": \"%s\"," JSON_CR, json_string(mapname));
	s2di(handle, INDENT2 "\"hostname\": \"%s\"," JSON_CR,
			json_string(striphigh(cvar_string("hostname"))));
	s2di(handle, INDENT2 "\"ip\": \"%s\"," JSON_CR, json_string(ip));
	s2di(handle, INDENT2 "\"port\": %d," JSON_CR, port);
	if (!strnull(matchtag))
	{
		s2di(handle, INDENT2 "\"matchtag\": \"%s\"," JSON_CR, json_string(matchtag));
	}

	if (!strnull(mode))
	{
		s2di(handle, INDENT2 "\"mode\": \"%s\"," JSON_CR, json_string(mode));
	}

	if (timelimit)
	{
		s2di(handle, INDENT2 "\"tl\": %d," JSON_CR, timelimit);
	}

	if (fraglimit)
	{
		s2di(handle, INDENT2 "\"fl\": %d," JSON_CR, timelimit);
	}

	if (deathmatch)
	{
		s2di(handle, INDENT2 "\"dm\": %d," JSON_CR, deathmatch);
	}

	if (teamplay)
	{
		s2di(handle, INDENT2 "\"tp\": %d," JSON_CR, teamplay);
	}

	s2di(handle, INDENT2 "\"duration\": %d," JSON_CR, duration);

	if (!strnull(cvar_string("serverdemo")))
	{
		const char *server_demo = cvar_string("serverdemo");

		s2di(handle, INDENT2 "\"demo\": \"%s\"," JSON_CR, json_string(server_demo));
	}
}

void json_match_footer(fileHandle_t handle)
{
	s2di(handle, "}" JSON_CR);
}

static void json_player_ctf_stats(fileHandle_t handle, player_stats_t *stats)
{
	qbool any = false;

	s2di(handle, "," JSON_CR);
	s2di(handle, INDENT6 "\"ctf\": {" JSON_CR);
	if (stats->ctf_points)
	{
		s2di(handle, INDENT8 "\"points\": %d", stats->ctf_points);
		any = true;
	}

	if (stats->caps)
	{
		COMMA_CHECK(handle, any);
		s2di(handle, INDENT8 "\"caps\": %d", stats->caps);
	}

	if (stats->f_defends)
	{
		COMMA_CHECK(handle, any);
		s2di(handle, INDENT8 "\"defends\": %d", stats->f_defends);
	}

	if (stats->c_defends)
	{
		COMMA_CHECK(handle, any);
		s2di(handle, INDENT8 "\"carrier-defends\": %d", stats->c_defends);
	}

	if (stats->c_frags)
	{
		COMMA_CHECK(handle, any);
		s2di(handle, INDENT8 "\"carrier-frags\": %d", stats->c_frags);
	}

	if (stats->pickups)
	{
		COMMA_CHECK(handle, any);
		s2di(handle, INDENT8 "\"pickups\": %d", stats->pickups);
	}

	if (stats->returns)
	{
		COMMA_CHECK(handle, any);
		s2di(handle, INDENT8 "\"returns\": %d", stats->returns);
	}

	COMMA_CHECK(handle, any);
	s2di(handle, INDENT8 "\"runes\": [%d, %d, %d, %d]" JSON_CR, (int)(stats->res_time + 0.5f),
			(int)(stats->str_time + 0.5f), (int)(stats->hst_time + 0.5f),
			(int)(stats->rgn_time + 0.5f));
	s2di(handle, INDENT6 "}");
}

static void json_player_instagib_stats(fileHandle_t handle, player_stats_t *stats)
{
	qbool any = false;

	s2di(handle, "," JSON_CR);
	s2di(handle, INDENT6 "\"instagib\": {" JSON_CR);
	if (stats->i_height || stats->i_maxheight)
	{
		s2di(handle, INDENT8 "\"height\": [%d, %d]", stats->i_height, stats->i_maxheight);
		COMMA_CHECK(handle, any);
	}

	if (stats->i_rings)
	{
		COMMA_CHECK(handle, any);
		s2di(handle, INDENT8 "\"rings\": %d", stats->i_rings);
	}

	COMMA_CHECK(handle, any);
	s2di(handle,
			INDENT8 "\"gibs\": { \"coil\": %d, \"axe\": %d, \"stomp\": %d, \"multi\": %d, \"air\": %d, \"best-multi\": %d }" JSON_CR,
			stats->i_cggibs, stats->i_axegibs, stats->i_stompgibs, stats->i_multigibs,
			stats->i_airgibs, stats->i_maxmultigibs);
	s2di(handle, INDENT6 "}");
}

static void json_player_midair_stats(fileHandle_t handle, player_stats_t *stats)
{
	qbool any = false;

	s2di(handle, "," JSON_CR);
	s2di(handle, INDENT6 "\"midair\": {" JSON_CR);
	if (stats->mid_stomps)
	{
		s2di(handle, INDENT8 "\"stomps\": %d", stats->mid_stomps);
		any = true;
	}

	if (stats->mid_bronze || stats->mid_silver || stats->mid_gold || stats->mid_platinum)
	{
		COMMA_CHECK(handle, any);
		s2di(handle,
				INDENT8 "\"midairs\": { \"bronze\": %d, \"silver\": %d, \"gold\": %d, \"platinum\": %d }",
				stats->mid_bronze, stats->mid_silver, stats->mid_gold, stats->mid_platinum);
	}

	if (stats->mid_total)
	{
		COMMA_CHECK(handle, any);
		s2di(handle, INDENT8 "\"total\": %d", stats->mid_total);
	}

	if (stats->mid_bonus)
	{
		COMMA_CHECK(handle, any);
		s2di(handle, INDENT8 "\"bonus\": %d", stats->mid_bonus);
	}

	if (stats->mid_totalheight || stats->mid_maxheight || stats->mid_avgheight)
	{
		COMMA_CHECK(handle, any);
		s2di(handle, INDENT8 "\"heights\": { \"total\": %f, \"max\": %f, \"avg\": %f }",
				stats->mid_totalheight, stats->mid_maxheight, stats->mid_avgheight);
	}

	NEWLINE_CHECK(handle, any);
	s2di(handle, INDENT6 "}");
}

static void json_player_ra_stats(fileHandle_t handle, player_stats_t *stats)
{
	s2di(handle, "," JSON_CR);
	s2di(handle, INDENT6 "\"ra\": { \"wins\": %d, \"losses\": %d }", stats->wins, stats->loses);
}

static void json_player_lgc_stats(fileHandle_t handle, gedict_t *player)
{
	int i;

	s2di(handle, "," JSON_CR);
	s2di(handle, INDENT6 "\"lgc\": {" JSON_CR);
	s2di(handle, INDENT8 "\"under\": %d," JSON_CR, player->ps.lgc_undershaft);
	s2di(handle, INDENT8 "\"over\": %d," JSON_CR, player->ps.lgc_overshaft);
	s2di(handle, INDENT8 "\"hits\": [");
	for (i = 0; i < LGCMODE_DISTANCE_BUCKETS; ++i)
	{
		s2di(handle, "%s%d", i ? ", " : "", player->lgc_distance_hits[i]);
	}

	s2di(handle, "]," JSON_CR);
	s2di(handle, INDENT8 "\"misses\": [");
	for (i = 0; i < LGCMODE_DISTANCE_BUCKETS; ++i)
	{
		s2di(handle, "%s%d", i ? ", " : "", player->lgc_distance_misses[i]);
	}

	s2di(handle, "]" JSON_CR);
	s2di(handle, "}" JSON_CR);
}

static void json_player_hoonymode_stats(fileHandle_t handle, gedict_t *player)
{
	s2di(handle, "," JSON_CR);
	if (isHoonyModeDuel())
	{
		s2di(handle, INDENT6 "\"hm-rounds\": \"%s\"" JSON_CR,
				json_string(HM_round_results(player)));
	}
	else
	{
		int i;

		s2di(handle, INDENT6 "\"hm-frags\": [");
		for (i = 0; i < HM_current_point(); ++i)
		{
			s2di(handle, "%s%d", i ? ", " : "", player->hoony_results[i]);
		}

		s2di(handle, "]" JSON_CR);
	}
}

void json_race_detail(fileHandle_t handle)
{
	extern gedict_t* race_find_racer(gedict_t *p);
	qbool any = false;
	race_stats_score_t *stats;

	s2di(handle, INDENT2 "\"race\": {" JSON_CR);
	s2di(handle, INDENT4 "\"route\": %d," JSON_CR, race.active_route - 1);
	s2di(handle,
			INDENT4 "\"weapon-mode\": \"%s\"," JSON_CR,
			race.weapon == raceWeaponAllowed ? "allowed" :
			race.weapon == raceWeapon2s ? "delayed" : "none");
	s2di(handle, INDENT4 "\"can-false-start\": %s," JSON_CR,
			race.falsestart == raceFalseStartYes ? "true" : "false");
	s2di(handle, INDENT4 "\"match\": %s," JSON_CR, race_match_mode() ? "true" : "false");
	if (!strnull(race.pacemaker_nick))
	{
		s2di(handle, INDENT4 "\"pacemaker\": { \"time\": %.3f, \"name\": \"%s\" }," JSON_CR,
				race.pacemaker_time * 0.001f, json_string(race.pacemaker_nick));
	}

	if (race_match_mode())
	{
		int player_count;
		int i;

		s2di(handle, INDENT4 "\"scoring\": \"%s\"," JSON_CR, race_scoring_system_name());
		stats = race_get_player_stats(&player_count);
		s2di(handle, INDENT4 "\"racers\": [" JSON_CR);
		for (i = 0; i < player_count; ++i)
		{
			COMMA_CHECK(handle, any);
			s2di(handle, INDENT6 "{ \"bestTime\": %.3f, \"completions\": %d, \"score\": %d, "
					"\"racer\": \"%s\", \"distance\": %f, \"time\": %.3f, \"wins\": %d }",
					stats[i].best_time / 1000.0f, stats[i].completions, stats[i].score,
					json_string(stats[i].name), stats[i].total_distance,
					stats[i].total_time / 1000.0f, stats[i].wins);
		}

		NEWLINE_CHECK(handle, any);
		s2di(handle, INDENT4 "]" JSON_CR);
	}
	else
	{
		gedict_t *p;
		s2di(handle, INDENT4 "\"racers\": [" JSON_CR);
		for (p = world; (p = race_find_race_participants(p)); /**/)
		{
			int player_number = NUM_FOR_EDICT(p) - 1;
			raceRecord_t *record = NULL;

			if ((player_number < 0)
					|| (player_number >= (sizeof(race.currentrace) / sizeof(race.currentrace[0]))))
			{
				continue;
			}

			record = &race.currentrace[player_number];

			if (!record->time)
			{
				continue;
			}

			COMMA_CHECK(handle, any);
			s2di(handle, INDENT6 "{ \"avgspeed\": %f, \"distance\": %f, \"time\": %f, "
					"\"racer\": \"%s\", \"maxspeed\": %f }",
					record->avgspeed / record->avgcount, record->distance, record->time / 1000.0f,
					json_string(p->netname), record->maxspeed);
		}

		NEWLINE_CHECK(handle, any);
		s2di(handle, INDENT4 "]" JSON_CR);
	}

	s2di(handle, INDENT2 "}" JSON_CR);
}
