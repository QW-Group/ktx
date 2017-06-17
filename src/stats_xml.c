
// stats_xml.c

#include "g_local.h"
#include "stats.h"

char* xml_string(const char* original);

static void xml_weap_header(fileHandle_t handle)
{
	s2di(handle, "\t\t\t<weapons>\n");
}

static void xml_weap_footer(fileHandle_t handle)
{
	s2di(handle, "\t\t\t</weapons>\n");
}

static void xml_weap_stats(fileHandle_t handle, int weapon, wpType_t* stats)
{
	s2di(handle, "\t\t\t\t<weapon name=\"%s\" hits=\"%d\" attacks=\"%d\""
		" kills=\"%d\" deaths=\"%d\" tkills=\"%d\" ekills=\"%d\""
		" drops=\"%d\" tooks=\"%d\" ttooks=\"%d\"/>\n",
		WpName(weapon), stats->hits, stats->attacks, stats->kills, stats->deaths, stats->tkills, stats->ekills,
		stats->drops, stats->tooks, stats->ttooks);
}

void xml_teams_header(fileHandle_t handle)
{
	char tmp[1024] = { 0 }, buf[1024] = { 0 };
	int i = 0;

	for (tmp[0] = i = 0; i < min(tmStats_cnt, MAX_TM_STATS); i++) {
		snprintf(buf, sizeof(buf), " team%d=\"%s\"", i + 1, xml_string(tmStats[i].name));
		strlcat(tmp, buf, sizeof(tmp));
	}

	if (i) {
		s2di(handle, "\t<teams%s>\n", tmp);
	}
}

void xml_teams_footer(fileHandle_t handle, int num)
{
	if (num) {
		s2di(handle, "\t</teams>\n");
	}
}

static void xml_team_header(fileHandle_t handle, int num, teamStats_t* stats)
{
	s2di(handle, "\t\t<team name=\"%s\" frags=\"%d\" deaths=\"%d\" tkills=\"%d\" dmg_tkn=\"%d\" dmg_gvn=\"%d\" dmg_tm=\"%d\">\n",
		xml_string(stats->name), stats->frags + stats->gfrags, stats->deaths, stats->tkills,
		(int)stats->dmg_t, (int)stats->dmg_g, (int)stats->dmg_team);
}

static void xml_team_footer(fileHandle_t handle)
{
	s2di(handle, "\t\t</team>\n");
}

static void xml_items_header(fileHandle_t handle)
{
	s2di(handle, "\t\t\t<items>\n");
}

static void xml_item_stats(fileHandle_t handle, int j, itType_t* stats)
{
	char buf[1024] = { 0 };

	if (itPowerup(j)) {
		snprintf(buf, sizeof(buf), " time=\"%d\"", (int)stats->time);
	}
	else {
		buf[0] = 0;
	}
	s2di(handle, "\t\t\t\t<item name=\"%s\" tooks=\"%d\"%s/>\n", ItName(j), stats->tooks, buf);
}

static void xml_items_footer(fileHandle_t handle)
{
	s2di(handle, "\t\t\t</items>\n");
}

void xml_players_header(fileHandle_t handle)
{
	s2di(handle, "\t<players>\n");
}

void xml_players_footer(fileHandle_t handle)
{
	s2di(handle, "\t</players>\n");
}

static void xml_player_header(fileHandle_t handle, gedict_t* player, const char* team)
{
	s2di(handle, "\t\t<player name=\"%s\" team=\"%s\" frags=\"%d\" deaths=\"%d\" tkills=\"%d\""
		" dmg_tkn=\"%d\" dmg_gvn=\"%d\" dmg_tm=\"%d\" spawnfrags=\"%d\" xfer_packs=\"%d\""
		" spree=\"%d\" qspree=\"%d\" control_time=\"%f\">\n",
		xml_string(getname(player)), xml_string(team), (int)player->s.v.frags, (int)player->deaths, (int)player->friendly,
		(int)player->ps.dmg_t, (int)player->ps.dmg_g, (int)player->ps.dmg_team, player->ps.spawn_frags, player->ps.transferred_packs,
		player->ps.spree_max, player->ps.spree_max_q, player->ps.control_time);
}

static void xml_player_footer(fileHandle_t handle)
{
	s2di(handle, "\t\t</player>\n");
}

char* xml_string(const char* original)
{
	static char string[MAX_STRINGS][1024];
	static int  index = 0;
	int length = strlen(original);
	int newlength = 0;
	int i = 0;

	index %= MAX_STRINGS;

	memset(string[index], 0, sizeof(string[0]));

	for (i = 0; i < length; ++i) {
		unsigned char ch = (unsigned char)original[i];

		if (ch == '<') {
			if (newlength < sizeof(string[0]) - 4) {
				string[index][newlength++] = '&';
				string[index][newlength++] = 'l';
				string[index][newlength++] = 't';
				string[index][newlength++] = ';';
			}
		}
		else if (ch == '>') {
			if (newlength < sizeof(string[0]) - 4) {
				string[index][newlength++] = '&';
				string[index][newlength++] = 'g';
				string[index][newlength++] = 't';
				string[index][newlength++] = ';';
			}
		}
		else if (ch == '"') {
			if (newlength < sizeof(string[0]) - 5) {
				string[index][newlength++] = '&';
				string[index][newlength++] = '#';
				string[index][newlength++] = '3';
				string[index][newlength++] = '4';
				string[index][newlength++] = ';';
			}
		}
		else if (ch == '&') {
			if (newlength < sizeof(string[0]) - 5) {
				string[index][newlength++] = '&';
				string[index][newlength++] = 'a';
				string[index][newlength++] = 'm';
				string[index][newlength++] = 'p';
				string[index][newlength++] = ';';
			}
		}
		else if (ch == '\'') {
			if (newlength < sizeof(string[0]) - 5) {
				string[index][newlength++] = '&';
				string[index][newlength++] = '#';
				string[index][newlength++] = '3';
				string[index][newlength++] = '9';
				string[index][newlength++] = ';';
			}
		}
		else {
			string[index][newlength++] = ch;
		}
	}

	return string[index++];
}

void xml_match_header(fileHandle_t handle, char* ip, int port)
{
	char date[64] = { 0 };
	char matchtag[64] = { 0 };
	const char* mode = cvar("k_instagib") ? "instagib" : (isRACE() ? "race" : GetMode());

	infokey(world, "matchtag", matchtag, sizeof(matchtag));

	if (!QVMstrftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S %Z", 0))
		date[0] = 0; // bad date

	s2di(handle, "%s", "<?xml version=\"1.0\"?>\n");
	s2di(handle, "<match version=\"3\" date=\"%s\" map=\"%s\" hostname=\"%s\" ip=\"%s\" port=\"%d\" mode=\"%s\" tl=\"%d\" fl=\"%d\" dmm=\"%d\" tp=\"%d\">\n",
		date, g_globalvars.mapname, xml_string(cvar_string("hostname")), ip, port, mode, timelimit, fraglimit, deathmatch, teamplay);
	if (!strnull(cvar_string("serverdemo"))) {
		s2di(handle, "\t<demo>%s</demo>\n", xml_string(cvar_string("serverdemo")));
	}
}

void xml_match_footer(fileHandle_t handle)
{
	s2di(handle, "</match>\n");
}

static void xml_player_ctf_stats(fileHandle_t handle, player_stats_t* stats)
{
	s2di(handle, "\t\t\t<ctf points=\"%d\" caps=\"%d\" flag-defends=\"%d\" cap-defends=\"%d\" "
		"cap-frags=\"%d\" pickups=\"%d\" returns=\"%d\" "
		"rune-res-time=\"%f\" rune-str-time=\"%f\" rune-hst-time=\"%f\" rune-rgn-time=\"%f\" />\n",
		stats->ctf_points, stats->caps, stats->f_defends, stats->c_defends,
		stats->c_frags, stats->pickups, stats->returns,
		stats->res_time, stats->str_time, stats->hst_time, stats->rgn_time);
}

static void xml_player_instagib_stats(fileHandle_t handle, player_stats_t* stats)
{
	s2di(handle, "\t\t\t<instagib height=\"%d\" maxheight=\"%d\" cggibs=\"%d\""
		" axegibs=\"%d\" stompgibs=\"%d\" multigibs=\"%d\" airgibs=\"%d\" "
		" maxmultigibs=\"%d\" rings=\"%d\" />\n",
		stats->i_height, stats->i_maxheight, stats->i_cggibs,
		stats->i_axegibs, stats->i_stompgibs, stats->i_multigibs, stats->i_airgibs,
		stats->i_maxmultigibs, stats->i_rings);
}

static void xml_player_midair_stats(fileHandle_t handle, player_stats_t* stats)
{
	s2di(handle, "\t\t\t<midair stomps=\"%d\" bronze=\"%d\" silver=\"%d\" gold=\"%d\" platinum=\"%d\" "
		" total=\"%d\" bonus=\"%d\" totalheight=\"%f\" maxheight=\"%f\" avgheight=\"%f\" />\n",
		stats->mid_stomps, stats->mid_bronze, stats->mid_silver, stats->mid_gold, stats->mid_platinum,
		stats->mid_total, stats->mid_bonus, stats->mid_totalheight, stats->mid_maxheight, stats->mid_avgheight);
}

static void xml_player_ra_stats(fileHandle_t handle, player_stats_t* stats)
{
	s2di(handle, "\t\t\t<rocket-arena wins=\"%d\" losses=\"%d\" />\n", stats->wins, stats->loses);
}

void xml_race_detail(fileHandle_t handle)
{
	extern gedict_t* race_find_racer(gedict_t* p);
	gedict_t* p;

	s2di(handle, "\t<race route=\"%d\" weaponmode=\"%d\" startmode=\"%d\">", race.active_route - 1, race.weapon, race.falsestart);
	if (!strnull(race.pacemaker_nick)) {
		s2di(handle, "\t\t<pacemaker time=\"%f\">%s</pacemaker>\n", race.pacemaker_time * 1.0f, xml_string(race.pacemaker_nick));
	}
	for (p = world; (p = race_find_racer(p)); /**/) {
		int player_number = NUM_FOR_EDICT(p) - 1;
		raceRecord_t* record = NULL;
		if (player_number < 0 || player_number >= sizeof(race.currentrace) / sizeof(race.currentrace[0])) {
			continue;
		}
		record = &race.currentrace[player_number];

		s2di(handle, "\t<racer avgspeed=\"%f\" distance=\"%f\" time=\"%f\" "
			"racer=\"%s\" weaponmode=\"%d\" startmode=\"%d\" maxspeed=\"%f\">\n",
			record->avgspeed / record->avgcount, record->distance, record->time,
			xml_string(p->netname), record->maxspeed
		);
		s2di(handle, "\t</race>\n");
	}
}

void xml_team_detail(fileHandle_t handle, int num, teamStats_t* stats)
{
	int j;

	xml_team_header(handle, num, stats);

	xml_weap_header(handle);
	for (j = 1; j < wpMAX; j++) {
		xml_weap_stats(handle, j, &stats->wpn[j]);
	}
	xml_weap_footer(handle);

	xml_items_header(handle);
	for (j = 1; j < itMAX; j++) {
		xml_item_stats(handle, j, &stats->itm[j]);
	}
	xml_items_footer(handle);

	xml_team_footer(handle);
}

#ifdef BOT_SUPPORT
void xml_player_bot_info(fileHandle_t handle, fb_entvars_t* vars)
{
}
#endif

void xml_player_detail(fileHandle_t handle, int num, gedict_t* player, const char* team)
{
	int j;

	xml_player_header(handle, player, team);

	xml_weap_header(handle);
	for (j = 1; j < wpMAX; j++ ) {
		xml_weap_stats(handle, j, &player->ps.wpn[j]);
	}
	xml_weap_footer(handle);

	xml_items_header(handle);
	for ( j = 1; j < itMAX; j++) {
		xml_item_stats(handle, j, &player->ps.itm[j]);
	}
	xml_items_footer(handle);

	if (cvar("k_midair")) {
		xml_player_midair_stats(handle, &player->ps);
	}
	if (cvar("k_instagib")) {
		xml_player_instagib_stats(handle, &player->ps);
	}
	if (isCTF()) {
		xml_player_ctf_stats(handle, &player->ps);
	}
	if (isRA()) {
		xml_player_ra_stats(handle, &player->ps);
	}
#ifdef BOT_SUPPORT
	if (player->isBot) {
		xml_player_bot_info(handle, &player->fb);
	}
#endif

	xml_player_footer(handle);
}
