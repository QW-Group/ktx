#include "g_local.h"
#include "stats.h"

FILE_FORMAT_DECL(xml)
FILE_FORMAT_DECL(json)

static stats_format_t file_formats[] =
{
	FILE_FORMAT_DEF(xml),
	FILE_FORMAT_DEF(json)
};

char tmStats_names[MAX_TM_STATS][MAX_TEAM_NAME]; // u can't put this in struct in QVM
teamStats_t tmStats[MAX_TM_STATS];
int tmStats_cnt = 0;

static void CollectTpStats(void)
{
	gedict_t *p, *p2;
	int from1, from2;
	int i, *frags;
	char *tmp = "";

	for (from1 = 0, p = world; (p = find_plrghst(p, &from1));)
	{
		p->ready = 0; // clear mark
	}

	//	get one player and search all his mates, mark served players via ->ready field
	//  ghosts is served too

	for (from1 = 0, p = world; (p = find_plrghst(p, &from1));)
	{
		if (p->ready || strnull(tmp = getteam(p)))
		{
			continue; // served or wrong team
		}

		if ((tmStats_cnt < 0) || (tmStats_cnt >= MAX_TM_STATS))
		{
			return; // all slots busy
		}

		if (tmStats[tmStats_cnt].name == NULL)
		{
			return;
		}

		for (from2 = 0, p2 = world; (p2 = find_plrghst(p2, &from2));)
		{
			if (p2->ready || strneq(tmp, getteam(p2)))
			{
				continue; // served or on different team
			}

			if (strnull(tmStats[tmStats_cnt].name)) // we not yet done that, do that once
			{
				strlcpy(tmStats[tmStats_cnt].name, tmp, MAX_TEAM_NAME);
			}

			frags = (p2->ct == ctPlayer ? &tmStats[tmStats_cnt].frags : &tmStats[tmStats_cnt].gfrags);
			frags[0] += p2->s.v.frags;
			tmStats[tmStats_cnt].deaths += p2->deaths;
			tmStats[tmStats_cnt].tkills += p2->friendly;

			tmStats[tmStats_cnt].dmg_t += p2->ps.dmg_t;
			tmStats[tmStats_cnt].dmg_g += p2->ps.dmg_g;
			tmStats[tmStats_cnt].dmg_team += p2->ps.dmg_team;
			tmStats[tmStats_cnt].dmg_eweapon += p2->ps.dmg_eweapon;

			for (i = itNONE; i < itMAX; i++)
			{ // summ each field of items
				tmStats[tmStats_cnt].itm[i].tooks += p2->ps.itm[i].tooks;
				tmStats[tmStats_cnt].itm[i].time += p2->ps.itm[i].time;
			}

			for (i = wpNONE; i < wpMAX; i++)
			{ // summ each field of weapons
				tmStats[tmStats_cnt].wpn[i].hits += p2->ps.wpn[i].hits;
				tmStats[tmStats_cnt].wpn[i].attacks += p2->ps.wpn[i].attacks;

				tmStats[tmStats_cnt].wpn[i].kills += p2->ps.wpn[i].kills;
				tmStats[tmStats_cnt].wpn[i].deaths += p2->ps.wpn[i].deaths;
				tmStats[tmStats_cnt].wpn[i].tkills += p2->ps.wpn[i].tkills;
				tmStats[tmStats_cnt].wpn[i].ekills += p2->ps.wpn[i].ekills;
				tmStats[tmStats_cnt].wpn[i].drops += p2->ps.wpn[i].drops;
				tmStats[tmStats_cnt].wpn[i].tooks += p2->ps.wpn[i].tooks;
				tmStats[tmStats_cnt].wpn[i].ttooks += p2->ps.wpn[i].ttooks;
			}

			tmStats[tmStats_cnt].transferred_RLpacks += p2->ps.transferred_RLpacks;
			tmStats[tmStats_cnt].transferred_LGpacks += p2->ps.transferred_LGpacks;

			// { ctf related
			tmStats[tmStats_cnt].res += p2->ps.res_time;
			tmStats[tmStats_cnt].str += p2->ps.str_time;
			tmStats[tmStats_cnt].hst += p2->ps.hst_time;
			tmStats[tmStats_cnt].rgn += p2->ps.rgn_time;

			tmStats[tmStats_cnt].caps += p2->ps.caps;
			tmStats[tmStats_cnt].pickups += p2->ps.pickups;
			tmStats[tmStats_cnt].returns += p2->ps.returns;
			tmStats[tmStats_cnt].f_defends += p2->ps.f_defends;
			tmStats[tmStats_cnt].c_defends += p2->ps.c_defends;
			// }

			p2->ready = 1; // set mark
		}

		if (strnull(tmStats[tmStats_cnt].name))
		{
			continue; // wtf, empty team?
		}

		if (isCTF() && g_globalvars.time - match_start_time > 0)
		{
			tmStats[tmStats_cnt].res = (tmStats[tmStats_cnt].res
					/ (g_globalvars.time - match_start_time)) * 100;
			tmStats[tmStats_cnt].str = (tmStats[tmStats_cnt].str
					/ (g_globalvars.time - match_start_time)) * 100;
			tmStats[tmStats_cnt].hst = (tmStats[tmStats_cnt].hst
					/ (g_globalvars.time - match_start_time)) * 100;
			tmStats[tmStats_cnt].rgn = (tmStats[tmStats_cnt].rgn
					/ (g_globalvars.time - match_start_time)) * 100;
		}

		tmStats_cnt++;
	}
}

static void ShowTeamsBanner(void)
{
	int i;

	G_bprint(2, "\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n");

	//	for( i = 666 + 1; i <= k_teamid ; i++ )
	//		G_bprint(2, "%s\220%s\221", (i != (666+1) ? " vs " : ""), ezinfokey(world, va("%d", i)));
	for (i = 0; i < min(tmStats_cnt, MAX_TM_STATS); i++)
	{
		G_bprint(2, "%s\220%s\221", (i ? " vs " : ""), tmStats[i].name);
	}

	G_bprint(2, " %s:\n", redtext("match statistics"));

	G_bprint(2, "\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n");
}

static void SummaryTPStats(void)
{
	int i;
	float h_sg, h_ssg, h_gl, h_rl, h_lg;

	ShowTeamsBanner();

	G_bprint(2, "\n%s, %s, %s, %s\n", redtext("weapons"), redtext("powerups"),
				redtext("armors&mhs"), redtext("damage"));
	G_bprint(2, "\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n");

	for (i = 0; i < min(tmStats_cnt, MAX_TM_STATS); i++)
	{
		h_sg = 100.0 * tmStats[i].wpn[wpSG].hits / max(1, tmStats[i].wpn[wpSG].attacks);
		h_ssg = 100.0 * tmStats[i].wpn[wpSSG].hits / max(1, tmStats[i].wpn[wpSSG].attacks);
		h_gl = tmStats[i].wpn[wpGL].hits;
		h_rl = tmStats[i].wpn[wpRL].hits;
		h_lg = 100.0 * tmStats[i].wpn[wpLG].hits / max(1, tmStats[i].wpn[wpLG].attacks);

		// weapons
		if (!cvar("k_instagib"))
		{
			G_bprint(2, "\220%s\221: %s:%s%s%s%s%s\n", tmStats[i].name, redtext("Wp"),
						(h_lg ? va(" %s%.0f%%", redtext("lg"), h_lg) : ""),
						(h_rl ? va(" %s%.0f", redtext("rl"), h_rl) : ""),
						(h_gl ? va(" %s%.0f", redtext("gl"), h_gl) : ""),
						(h_sg ? va(" %s%.0f%%", redtext("sg"), h_sg) : ""),
						(h_ssg ? va(" %s%.0f%%", redtext("ssg"), h_ssg) : ""));

			// powerups
			G_bprint(2, "%s: %s:%d %s:%d %s:%d\n", redtext("Powerups"), redtext("Q"),
						tmStats[i].itm[itQUAD].tooks, redtext("P"), tmStats[i].itm[itPENT].tooks,
						redtext("R"), tmStats[i].itm[itRING].tooks);

			// armors + megahealths
			G_bprint(2, "%s: %s:%d %s:%d %s:%d %s:%d\n", redtext("Armr&mhs"), redtext("ga"),
						tmStats[i].itm[itGA].tooks, redtext("ya"), tmStats[i].itm[itYA].tooks,
						redtext("ra"), tmStats[i].itm[itRA].tooks, redtext("mh"),
						tmStats[i].itm[itHEALTH_100].tooks);
		}
		else
		{
			G_bprint(2, "\220%s\221: %s:%s\n", tmStats[i].name, redtext("Wp"),
						(h_sg ? va(" %s%.0f%%", redtext("cg"), h_sg) : ""));
		}

		if (isCTF())
		{
			if (cvar("k_ctf_runes"))
			{
				G_bprint(2, "%s: %s:%.0f%% %s:%.0f%% %s:%.0f%% %s:%.0f%%\n", redtext("RuneTime"),
							redtext("res"), tmStats[i].res, redtext("str"), tmStats[i].str,
							redtext("hst"), tmStats[i].hst, redtext("rgn"), tmStats[i].rgn);
			}

			G_bprint(2, "%s: %s:%d %s:%d %s:%d\n", redtext("     CTF"), redtext("pickups"),
						tmStats[i].pickups, redtext("caps"), tmStats[i].caps, redtext("returns"),
						tmStats[i].returns);
			G_bprint(2, "%s: %s:%d %s:%d\n", redtext(" Defends"), redtext("flag"),
						tmStats[i].f_defends, redtext("carrier"), tmStats[i].c_defends);
		}

		// rl
		G_bprint(2, "%s: %s:%d %s:%d %s:%d %s:%d\n", redtext("      RL"), redtext("Took"),
					tmStats[i].wpn[wpRL].tooks, redtext("Killed"), tmStats[i].wpn[wpRL].ekills,
					redtext("Dropped"), tmStats[i].wpn[wpRL].drops, redtext("Xfer"),
					tmStats[i].transferred_RLpacks);

		// lg
		G_bprint(2, "%s: %s:%d %s:%d %s:%d %s:%d\n", redtext("      LG"), redtext("Took"),
					tmStats[i].wpn[wpLG].tooks, redtext("Killed"), tmStats[i].wpn[wpLG].ekills,
					redtext("Dropped"), tmStats[i].wpn[wpLG].drops, redtext("Xfer"),
					tmStats[i].transferred_LGpacks);

		// damage
		if (deathmatch == 1)
		{
			G_bprint(2, "%s: %s:%.0f %s:%.0f %s:%.0f %s:%.0f\n", redtext("  Damage"),
						redtext("Tkn"), tmStats[i].dmg_t, redtext("Gvn"), tmStats[i].dmg_g,
						redtext("EWep"), tmStats[i].dmg_eweapon, redtext("Tm"),
						tmStats[i].dmg_team);
		}
		else
		{
			G_bprint(2, "%s: %s:%.0f %s:%.0f %s:%.0f\n", redtext("  Damage"), redtext("Tkn"),
						tmStats[i].dmg_t, redtext("Gvn"), tmStats[i].dmg_g, redtext("Tm"),
						tmStats[i].dmg_team);
		}

		// times
		G_bprint(2, "%s: %s:%d\n", redtext("    Time"), redtext("Quad"),
					(int)tmStats[i].itm[itQUAD].time);
	}

	G_bprint(2, "\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n");
}

static void TeamsStats(void)
{
	int i, sumfrags = 0, wasPrint = 0;

	if (isCA())
	{
		CA_TeamsStats();

		return;
	}

	// Summing up the frags to calculate team percentages
	for (i = 0; i < min(tmStats_cnt, MAX_TM_STATS); i++)
	{
		sumfrags += max(0, tmStats[i].frags + tmStats[i].gfrags);
	}
	// End of summing

	G_bprint(
			2,
			"\n%s: %s\n\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Team scores"), redtext("frags \217 percentage"));

	for (i = 0; i < min(tmStats_cnt, MAX_TM_STATS); i++)
	{
		G_bprint(2, "\220%s\221: %d", tmStats[i].name, tmStats[i].frags);

		if (tmStats[i].gfrags)
		{
			G_bprint(2, " + (%d) = %d", tmStats[i].gfrags, tmStats[i].frags + tmStats[i].gfrags);
		}

		// effi
		G_bprint(
				2,
				" \217 %.1f%%\n",
				(sumfrags > 0 ?
						((float)(tmStats[i].frags + tmStats[i].gfrags)) / sumfrags * 100 : 0.0));

		wasPrint = 1;
	}

	if (wasPrint)
	{
		G_bprint(
				2, "\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n");
	}
}

// Statistic file generation
const char* GetMode(void)
{
	if (cvar("k_instagib"))
	{
		return "instagib";
	}
	else if (cvar("k_midair"))
	{
		return "midair";
	}
	else if (isHoonyModeAny())
	{
		return "hoonymode";
	}
	else if (isRACE())
	{
		return "race";
	}
	else if (isCA())
	{
		return "clan-arena";
	}
	else if (isRA())
	{
		return "rocket-arena";
	}
	else if (isDuel())
	{
		return "duel";
	}
	else if (isTeam())
	{
		return "team";
	}
	else if (isCTF())
	{
		return "ctf";
	}
	else if (isFFA())
	{
		return "ffa";
	}
	else
	{
		return "unknown";
	}
}

// Also used in //wps transmission
char* WpName(weaponName_t wp)
{
	switch (wp)
	{
		case wpAXE:
			return "axe";

		case wpSG:
			return "sg";

		case wpSSG:
			return "ssg";

		case wpNG:
			return "ng";

		case wpSNG:
			return "sng";

		case wpGL:
			return "gl";

		case wpRL:
			return "rl";

		case wpLG:
			return "lg";

			// shut up gcc
		case wpNONE:
		case wpMAX:
			return "unknown";
	}

	return "unknown";
}

char* ItName(itemName_t it)
{
	switch (it)
	{
		case itHEALTH_15:
			return "health_15";

		case itHEALTH_25:
			return "health_25";

		case itHEALTH_100:
			return "health_100";

		case itGA:
			return "ga";

		case itYA:
			return "ya";

		case itRA:
			return "ra";

		case itQUAD:
			return "q";

		case itPENT:
			return "p";

		case itRING:
			return "r";

			// shut up gcc
		case itNONE:
		case itMAX:
			return "unknown";
	}

	return "unknown";
}

qbool itPowerup(itemName_t it)
{
	return ((it == itQUAD) || (it == itPENT) || (it == itRING));
}

void s2di(fileHandle_t file_handle, const char *fmt, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	text[sizeof(text) - 1] = 0;

	trap_FS_WriteFile(text, strlen(text), file_handle);
}

static stats_format_t* FindStatsFormat(const char* requested_format)
{
	int i;

	for (i = 0; i < sizeof(file_formats) / sizeof(file_formats[0]); ++i)
	{
		if (streq(requested_format, file_formats[i].name))
		{
			return &file_formats[i];
		}
	}

	// default to xml
	return &file_formats[0];
}

static qbool CreateStatsFile(char* filename, char* ip, int port, stats_format_t* format)
{
	gedict_t *p, *p2;
	fileHandle_t di_handle;
	int from1, from2;
	char *team = "";
	int player_num = 0;
	int i = 0;

	if (format == NULL)
	{
		return false;
	}

	if (trap_FS_OpenFile(va("%s.%s", filename, format->name), &di_handle, FS_WRITE_BIN) < 0)
	{
		return false;
	}

	format->match_header(di_handle, ip, port);
	if (isRACE())
	{
		format->race_detail(di_handle);
	}
	else
	{
		format->teams_header(di_handle);
		for (i = 0; i < min(tmStats_cnt, MAX_TM_STATS); i++)
		{
			format->team_detail(di_handle, i, &tmStats[i]);
		}

		format->teams_footer(di_handle, i);

		// } TEAMS

		// { PLAYERS

		for (from1 = 0, p = world; (p = find_plrghst(p, &from1));)
		{
			p->ready = 0; // clear mark
		}

		//	get one player and search all his mates, mark served players via ->ready field
		//  ghosts is served too

		format->players_header(di_handle);
		for (from1 = 0, p = world; (p = find_plrghst(p, &from1));)
		{
			team = getteam(p);
			if (p->ready /* || strnull( team ) */)
			{
				continue; // served or wrong team
			}

			for (from2 = 0, p2 = world; (p2 = find_plrghst(p2, &from2));)
			{
				if (p2->ready || strneq(team, getteam(p2)))
				{
					continue; // served or on different team
				}

				format->player_detail(di_handle, player_num++, p2, team);

				p2->ready = 1; // set mark
			}
		}

		format->players_footer(di_handle, player_num);
		// } PLAYERS
	}

	format->match_footer(di_handle);

	trap_FS_CloseFile(di_handle);
	return true;
}

void StatsToFile(void)
{
	char name[256] =
		{ 0 }, *ip = "", *port = "";
	int i = 0;
	qbool written = false;
	qbool send_to_website = !strnull(cvar_string("sv_www_address"));
	qbool embed_in_mvd = (sv_extensions & SV_EXTENSIONS_MVDHIDDEN);
	stats_format_t* json_format = NULL;
	stats_format_t* format = NULL;

	if (strnull(ip = cvar_string("sv_local_addr")) || strnull(port = strchr(ip, ':'))
			|| !(i = atoi(port + 1)))
	{
		return;
	}

	port[0] = 0;
	port++;

	if (strnull(cvar_string("serverdemo")) || (cvar("sv_demotxt") != 2))
	{
		return; // doesn't record demo or doesn't want stats to be put in file
	}

	format = FindStatsFormat(cvar_string("k_demotxt_format"));
	json_format = FindStatsFormat("json");

	// This file over-written every time
	snprintf(name, sizeof(name), "demoinfo_%s_%d", ip, i);

	// Always write json, so it can be embedded in demo
	if (json_format != NULL)
	{
		written = CreateStatsFile(name, ip, i, json_format);

		if (written)
		{
			// submit to central website
			if (send_to_website)
			{
				localcmd(
						"\nsv_web_postfile ServerApi/UploadGameStats \"\" \"%s.%s\" *internal authinfo\n",
						name, json_format->name);
				trap_executecmd();
			}

			// if server supports embedding in .mvd/qtv stream, do that
			if (embed_in_mvd)
			{
				localcmd("\nsv_demoembedinfo \"%s.%s\"\n", name, json_format->name);
				trap_executecmd();
			}
		}

		written = true;
	}

	// If non-json version required, generate now
	if (!streq(format->name, "json"))
	{
		written = CreateStatsFile(name, ip, i, format);
	}

	// add info
	if (written)
	{
		localcmd("\n" // why new line?
			"sv_demoinfoadd ** %s.%s\n", name, format->name);
		trap_executecmd();
	}
}

float maxfrags, maxdeaths, maxfriend, maxeffi, maxcaps, maxdefends, maxsgeffi;
int maxspree, maxspree_q, maxdmgg, maxdmgtd, maxrlkills;

// This is the old Endgame stats, which has been split into individual tables.
void OnePlayerStats(gedict_t *p, int tp)
{
	float dmg_g;
	float dmg_t;
	float dmg_team;
	float dmg_self;
	float dmg_eweapon;
	float dmg_g_rl;
	float dmg_td;
	int ra;
	int ya;
	int ga;
	int mh;
	int d_rl;
	int k_rl;
	int t_rl;
	int d_lg;
	int k_lg;
	int t_lg;
	int quad;
	int pent;
	int ring;
	float ph_rl;
	float vh_rl;
	float h_rl;
	float a_rl;
	float ph_gl;
	float vh_gl;
	float a_gl;
	float h_lg;
	float a_lg;
	float h_sg;
	float a_sg;
	float h_ssg;
	float a_ssg;
	float e_sg;
	float e_ssg;
	float e_lg;
	int res;
	int str;
	int hst;
	int rgn;

	dmg_g = p->ps.dmg_g;
	dmg_g_rl = p->ps.dmg_g_rl;
	dmg_t = p->ps.dmg_t;
	dmg_team = p->ps.dmg_team;
	dmg_self = p->ps.dmg_self;
	dmg_eweapon = p->ps.dmg_eweapon;
	dmg_td = p->deaths <= 0 ? 99999 : (int)(p->ps.dmg_t / p->deaths);
	ra = p->ps.itm[itRA].tooks;
	ya = p->ps.itm[itYA].tooks;
	ga = p->ps.itm[itGA].tooks;
	mh = p->ps.itm[itHEALTH_100].tooks;
	quad = p->ps.itm[itQUAD].tooks;
	pent = p->ps.itm[itPENT].tooks;
	ring = p->ps.itm[itRING].tooks;

	h_rl = p->ps.wpn[wpRL].hits;
	vh_rl = p->ps.wpn[wpRL].vhits;
	a_rl = p->ps.wpn[wpRL].attacks;
	vh_gl = p->ps.wpn[wpGL].vhits;
	a_gl = p->ps.wpn[wpGL].attacks;
	h_lg = p->ps.wpn[wpLG].hits;
	a_lg = p->ps.wpn[wpLG].attacks;
	h_sg = p->ps.wpn[wpSG].hits;
	a_sg = p->ps.wpn[wpSG].attacks;
	h_ssg = p->ps.wpn[wpSSG].hits;
	a_ssg = p->ps.wpn[wpSSG].attacks;

	e_sg = 100.0 * h_sg / max(1, a_sg);
	e_ssg = 100.0 * h_ssg / max(1, a_ssg);
	ph_gl = 100.0 * vh_gl / max(1, a_gl);
	ph_rl = 100.0 * vh_rl / max(1, a_rl);
	e_lg = 100.0 * h_lg / max(1, a_lg);

	d_rl = p->ps.wpn[wpRL].drops;
	k_rl = p->ps.wpn[wpRL].ekills;
	t_rl = p->ps.wpn[wpRL].tooks;

	d_lg = p->ps.wpn[wpLG].drops;
	k_lg = p->ps.wpn[wpLG].ekills;
	t_lg = p->ps.wpn[wpLG].tooks;

	if (isCTF() && ((g_globalvars.time - match_start_time) > 0))
	{
		res = (p->ps.res_time / (g_globalvars.time - match_start_time)) * 100;
		str = (p->ps.str_time / (g_globalvars.time - match_start_time)) * 100;
		hst = (p->ps.hst_time / (g_globalvars.time - match_start_time)) * 100;
		rgn = (p->ps.rgn_time / (g_globalvars.time - match_start_time)) * 100;
	}
	else
	{
		res = str = hst = rgn = 0;
	}

	if (tp)
	{
		G_bprint(2, "\235\236\236\236\236\236\236\236\236\237\n");
	}

	G_bprint(
			2,
			"\207 %s%s:\n"
			"  %d (%d) %s%.1f%%\n",
			(isghost(p) ? "\203" : ""),
			getname(p),
			(isCTF() ? (int)(p->s.v.frags - p->ps.ctf_points) : (int)p->s.v.frags),
			(isCTF() ?
					(int)(p->s.v.frags - p->ps.ctf_points - p->deaths) :
					(int)(p->s.v.frags - p->deaths)),
			(tp ? va("%d ", (int)p->friendly) : ""), p->efficiency);

	// qqshka - force show this always
	//	if ( !tp || cvar( "tp_players_stats" ) ) {
	// weapons
	G_bprint(2, "%s:%s%s%s%s%s\n", redtext("Wp"),
				(a_lg ? va(" %s%.1f%% (%d/%d)", redtext("lg"), e_lg, (int)h_lg, (int)a_lg) : ""),
				(ph_rl ? va(" %s%.1f%%", redtext("rl"), ph_rl) : ""),
				(ph_gl ? va(" %s%.1f%%", redtext("gl"), ph_gl) : ""),
				(e_sg ? va(" %s%.1f%%", redtext("sg"), e_sg) : ""),
				(e_ssg ? va(" %s%.1f%%", redtext("ssg"), e_ssg) : ""));

	// rockets detail
	if (!lgc_enabled())
	{
		G_bprint(2, "%s: %s:%.1f %s:%.0f\n", redtext("RL skill"), redtext("ad"),
					vh_rl ? (dmg_g_rl / vh_rl) : 0., redtext("dh"), h_rl);
	}

	// velocity
	if (isDuel())
	{
		G_bprint(2, "%s: %s:%.1f %s:%.1f\n", redtext("   Speed"), redtext("max"),
					p->ps.velocity_max, redtext("average"),
					p->ps.vel_frames > 0 ? p->ps.velocity_sum / p->ps.vel_frames : 0.);
	}

	// armors + megahealths
	if (!lgc_enabled())
	{
		G_bprint(2, "%s: %s:%d %s:%d %s:%d %s:%d\n", redtext("Armr&mhs"), redtext("ga"), ga,
					redtext("ya"), ya, redtext("ra"), ra, redtext("mh"), mh);
	}

	// powerups
	if (isTeam() || isCTF())
	{
		G_bprint(2, "%s: %s:%d %s:%d %s:%d\n", redtext("Powerups"), redtext("Q"), quad,
					redtext("P"), pent, redtext("R"), ring);
	}

	if (isCTF())
	{
		if (cvar("k_ctf_runes"))
		{
			G_bprint(2, "%s: %s:%d%% %s:%d%% %s:%d%% %s:%d%%\n", redtext("RuneTime"),
						redtext("res"), res, redtext("str"), str, redtext("hst"), hst,
						redtext("rgn"), rgn);
		}

		G_bprint(2, "%s: %s:%d %s:%d %s:%d\n", redtext("     CTF"), redtext("pickups"),
					p->ps.pickups, redtext("caps"), p->ps.caps, redtext("returns"), p->ps.returns);
		G_bprint(2, "%s: %s:%d %s:%d\n", redtext(" Defends"), redtext("flag"), p->ps.f_defends,
					redtext("carrier"), p->ps.c_defends);
	}

	// rl
	if (isTeam())
	{
		G_bprint(
				2,
				"%s: %s:%d %s:%d %s:%d%s\n",
				redtext("      RL"),
				redtext("Took"),
				t_rl,
				redtext("Killed"),
				k_rl,
				redtext("Dropped"),
				d_rl,
				(p->ps.transferred_RLpacks ?
						va(" %s:%d", redtext("Xfer"), p->ps.transferred_RLpacks) : ""));
	}

	// lg
	if (isTeam())
	{
		G_bprint(
				2,
				"%s: %s:%d %s:%d %s:%d%s\n",
				redtext("      LG"),
				redtext("Took"),
				t_lg,
				redtext("Killed"),
				k_lg,
				redtext("Dropped"),
				d_lg,
				(p->ps.transferred_LGpacks ?
						va(" %s:%d", redtext("Xfer"), p->ps.transferred_LGpacks) : ""));

	}

	// damage
	if (isTeam() && (deathmatch == 1))
	{
		G_bprint(2, "%s: %s:%.0f %s:%.0f %s:%.0f %s:%.0f %s:%.0f %s:%.0f\n", redtext("  Damage"),
					redtext("Tkn"), dmg_t, redtext("Gvn"), dmg_g, redtext("EWep"), dmg_eweapon,
					redtext("Tm"), dmg_team, redtext("Self"), dmg_self, redtext("ToDie"),
					dmg_td == -1 ? 99999 : dmg_td);
	}
	else
	{
		G_bprint(2, "%s: %s:%.0f %s:%.0f %s:%.0f %s:%.0f %s:%.0f\n", redtext("  Damage"),
					redtext("Tkn"), dmg_t, redtext("Gvn"), dmg_g, redtext("Tm"), dmg_team,
					redtext("Self"), dmg_self, redtext("ToDie"), dmg_td == -1 ? 99999 : dmg_td);
	}

	// times
	if ((g_globalvars.time - match_start_time) > 0)
	{
		if (isDuel())
		{
			G_bprint(
					2, "%s: %s:%d (%d%%)\n", redtext("    Time"), redtext("Control"),
					(int)p->ps.control_time,
					(int)((p->ps.control_time / (g_globalvars.time - match_start_time)) * 100));
		}
		else
		{
			G_bprint(2, "%s: %s:%d\n", redtext("    Time"), redtext("Quad"),
						(int)p->ps.itm[itQUAD].time);
		}
	}

	if (isDuel())
	{
		//  endgame h & a
		G_bprint(2, " %s: %s:%d %s:", redtext("EndGame"), redtext("H"), (int)p->s.v.health,
					redtext("A"));
		if ((int)p->s.v.armorvalue)
		{
			G_bprint(2, "%s%d\n", armor_type(p->s.v.items), (int)p->s.v.armorvalue);
		}
		else
		{
			G_bprint(2, "0\n");
		}

		// overtime h & a
		if (k_overtime)
		{
			G_bprint(2, " %s: %s:%d %s:", redtext("OverTime"), redtext("H"), (int)p->ps.ot_h,
						redtext("A"));
			if ((int)p->ps.ot_a)
			{
				G_bprint(2, "%s%d\n", armor_type(p->ps.ot_items), (int)p->ps.ot_a);
			}
			else
			{
				G_bprint(2, "0\n");
			}
		}
	}
	else
	{
		G_bprint(2, "%s: %s:%d %s:%d\n", redtext(" Streaks"), redtext("Frags"), p->ps.spree_max,
					redtext("QuadRun"), p->ps.spree_max_q);
	}

	// spawnfrags
	if (!isCTF() && !lgc_enabled())
	{
		G_bprint(2, "  %s: %d\n", redtext("SpawnFrags"), p->ps.spawn_frags);
	}

	if (lgc_enabled() && a_lg)
	{
		int over = p->ps.lgc_overshaft;
		int under = p->ps.lgc_undershaft;

		G_bprint(PRINT_HIGH, "  %s : \20%d\21\n", redtext("LGC Score"),
					(int)(e_lg * p->s.v.frags));
		G_bprint(PRINT_HIGH, "  %s : %.1f%% (%d/%d)\n", redtext("Overshaft"), over * 100.0f / a_lg,
					(int)over, (int)a_lg);
		G_bprint(PRINT_HIGH, "  %s: %.1f%% (%d/%d)\n", redtext("Undershaft"), under * 100.0f / a_lg,
					(int)under, (int)a_lg);
	}

	//	}
	if (!tp)
	{
		G_bprint(2, "\235\236\236\236\236\236\236\236\236\237\n");
	}

	maxfrags = max((isCTF() ? p->s.v.frags - p->ps.ctf_points : p->s.v.frags), maxfrags);
	maxdeaths = max(p->deaths, maxdeaths);
	maxfriend = max(p->friendly, maxfriend);
	maxeffi = max(p->efficiency, maxeffi);
	maxcaps = max(p->ps.caps, maxcaps);
	maxdefends = max(p->ps.f_defends, maxdefends);
	maxspree = max(p->ps.spree_max, maxspree);
	maxspree_q = max(p->ps.spree_max_q, maxspree_q);
	maxdmgg = max(p->ps.dmg_g, maxdmgg);
	maxdmgtd = max((int)(p->ps.dmg_t / p->deaths), maxdmgtd);
	maxrlkills = max(p->ps.wpn[wpRL].ekills, maxrlkills);
	maxsgeffi = max(e_sg, maxsgeffi);
}

void OnePlayerMidairStats(gedict_t *p)
{
	G_bprint(2,
			 "%s%-20s|%3d|%3d|%3d|%3d|%3d|%3d|%3d|%5s|%5s|\n",
			(isghost(p) ? "\203" : ""), getname(p),
			p->ps.mid_total,
			p->ps.mid_bronze,
			p->ps.mid_silver,
			p->ps.mid_gold,
			p->ps.mid_platinum,
			p->ps.mid_stomps,
			p->ps.mid_bonus,
			va("%.1f", p->ps.mid_maxheight),
			va("%.1f", (p->ps.mid_maxheight ? p->ps.mid_avgheight : 0)));
}

void OnePlayerMidairKillStats(gedict_t *p)
{
	float vh_rl;
	float a_rl;
	float e_rl;

	vh_rl = p->ps.wpn[wpRL].vhits;
	a_rl = p->ps.wpn[wpRL].attacks;
	e_rl = 100.0 * vh_rl / max(1, a_rl);

	G_bprint(2,
			 "%s%-20s|%5d|%5d|%5d|%5s|\n",
			(isghost(p) ? "\203" : ""), getname(p),
			(int)p->s.v.frags,
			p->ps.spawn_frags,
			p->ps.spree_max,
			(e_rl == 100 ? va("%.0f%%", e_rl) : va("%.1f%%", e_rl)));
}

void OnePlayerInstagibStats(gedict_t *p)
{
//	float h_ax;
//	float a_ax;
//	float h_sg;
//	float a_sg;
//	float h_ssg;
//	float a_ssg;
//	char *stats_text;

//	h_sg = p->ps.wpn[wpSG].hits;
//	a_sg = p->ps.wpn[wpSG].attacks;
//	h_ssg = p->ps.wpn[wpSSG].hits;
//	a_ssg = p->ps.wpn[wpSSG].attacks;
//	h_ax = p->ps.wpn[wpAXE].hits;
//	a_ax = p->ps.wpn[wpAXE].attacks;
//
//	h_ax = 100.0 * h_ax / max(1, a_ax);
//	h_sg = 100.0 * h_sg / max(1, a_sg);
//	h_ssg = 100.0 * h_ssg / max(1, a_ssg);

//	stats_text = va("\n\207 %s: %s%s \207\n", "PLAYER", (isghost(p) ? "\203" : ""), getname(p));
//
//	stats_text = va("%s \220%s\221\n", stats_text, "SCORES");
//	stats_text = va("%s  %s: %.1f\n", stats_text, redtext("Efficiency"), p->efficiency);
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Points"), (int)p->s.v.frags);
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Frags"), p->ps.i_cggibs + p->ps.i_axegibs + p->ps.i_stompgibs);
//	if (tp)
//	{
//		stats_text = va("%s  %s: %d\n", stats_text, redtext("Teamkills"), (int)p->friendly);
//	}
//
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Deaths"), (int)p->deaths);
//
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Streaks"), p->ps.spree_max);
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Spawns"), p->ps.spawn_frags);
//
//	stats_text = va("%s \220%s\221\n", stats_text, "SPEED");
//	stats_text = va("%s  %s: %.1f\n", stats_text, redtext("Maximum"), p->ps.velocity_max);
//	stats_text = va("%s  %s: %.1f\n", stats_text, redtext("Average"), p->ps.vel_frames > 0 ? p->ps.velocity_sum / p->ps.vel_frames : 0.);
//
//	stats_text = va("%s \220%s\221\n", stats_text, "WEAPONS");
//	stats_text = va("%s  %s: %s", stats_text, redtext("Coilgun"), (a_sg ? va("%.1f%% (%d)", h_sg, p->ps.i_cggibs) : ""));
//	stats_text = va("%s%s", stats_text, (a_sg ? "" : "n/u"));
//
//	stats_text = va("%s\n", stats_text);
//	stats_text = va("%s  %s: %s", stats_text, redtext("Axe"), (a_ax ? va("%.1f%% (%d)", h_ax, p->ps.i_axegibs) : ""));
//	stats_text = va("%s%s", stats_text, (a_ax ? "" : "n/u"));
//	stats_text = va("%s\n", stats_text);
//
//	stats_text = va("%s \220%s\221\n", stats_text, "GIBS");
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Coilgun"), p->ps.i_cggibs);
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Axe"), p->ps.i_axegibs);
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Stomps"), p->ps.i_stompgibs);
//
//	stats_text = va("%s \220%s\221\n", stats_text, "MULTIGIBS");
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Total Multigibs"), p->ps.i_multigibs);
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Maximum Victims"), p->ps.i_maxmultigibs);
//
//	stats_text = va("%s \220%s\221\n", stats_text, "AIRGIBS");
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Total"), p->ps.i_airgibs);
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Total Height"), p->ps.i_height);
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Maximum Height"), p->ps.i_maxheight);
//	stats_text = va("%s  %s: %.1f\n", stats_text, redtext("Average Height"), p->ps.i_airgibs ? p->ps.i_height / p->ps.i_airgibs : 0.);
//	G_bprint(2, "%s", stats_text);

	// Coilgun Axe Stomps Multigibs MaxMG Victims Airgibs TotalH. MaxH. Ring
	G_bprint(2,
			 "%s%-20s|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|\n",
			(isghost(p) ? "\203" : ""), getname(p),
			p->ps.i_cggibs,
			p->ps.i_axegibs,
			p->ps.i_stompgibs,
			p->ps.i_multigibs,
			p->ps.i_maxmultigibs,
			p->ps.i_airgibs,
			p->ps.i_height,
			p->ps.i_maxheight,
			p->ps.i_rings);
}

void OnePlayerInstagibKillStats(gedict_t *p)
{
//	float h_ax;
//	float a_ax;
	float h_sg;
	float a_sg;
//	float h_ssg;
//	float a_ssg;
//	char *stats_text;

	h_sg = p->ps.wpn[wpSG].hits;
	a_sg = p->ps.wpn[wpSG].attacks;
//	h_ssg = p->ps.wpn[wpSSG].hits;
//	a_ssg = p->ps.wpn[wpSSG].attacks;
//	h_ax = p->ps.wpn[wpAXE].hits;
//	a_ax = p->ps.wpn[wpAXE].attacks;
//
//	h_ax = 100.0 * h_ax / max(1, a_ax);
//	h_sg = 100.0 * h_sg / max(1, a_sg);
//	h_ssg = 100.0 * h_ssg / max(1, a_ssg);

//	stats_text = va("\n\207 %s: %s%s \207\n", "PLAYER", (isghost(p) ? "\203" : ""), getname(p));

//	stats_text = va("%s \220%s\221\n", stats_text, "SCORES");
//	stats_text = va("%s  %s: %.1f\n", stats_text, redtext("Efficiency"), p->efficiency);
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Points"), (int)p->s.v.frags);
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Frags"), p->ps.i_cggibs + p->ps.i_axegibs + p->ps.i_stompgibs);
//	if (tp)
//	{
//		stats_text = va("%s  %s: %d\n", stats_text, redtext("Teamkills"), (int)p->friendly);
//	}
//
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Deaths"), (int)p->deaths);
//
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Streaks"), p->ps.spree_max);
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Spawns"), p->ps.spawn_frags);
//
//	stats_text = va("%s \220%s\221\n", stats_text, "SPEED");
//	stats_text = va("%s  %s: %.1f\n", stats_text, redtext("Maximum"), p->ps.velocity_max);
//	stats_text = va("%s  %s: %.1f\n", stats_text, redtext("Average"), p->ps.vel_frames > 0 ? p->ps.velocity_sum / p->ps.vel_frames : 0.);
//
//	stats_text = va("%s \220%s\221\n", stats_text, "WEAPONS");
//	stats_text = va("%s  %s: %s", stats_text, redtext("Coilgun"), (a_sg ? va("%.1f%% (%d)", h_sg, p->ps.i_cggibs) : ""));
//	stats_text = va("%s%s", stats_text, (a_sg ? "" : "n/u"));
//
//	stats_text = va("%s\n", stats_text);
//	stats_text = va("%s  %s: %s", stats_text, redtext("Axe"), (a_ax ? va("%.1f%% (%d)", h_ax, p->ps.i_axegibs) : ""));
//	stats_text = va("%s%s", stats_text, (a_ax ? "" : "n/u"));
//	stats_text = va("%s\n", stats_text);
//
//	stats_text = va("%s \220%s\221\n", stats_text, "GIBS");
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Coilgun"), p->ps.i_cggibs);
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Axe"), p->ps.i_axegibs);
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Stomps"), p->ps.i_stompgibs);
//
//	stats_text = va("%s \220%s\221\n", stats_text, "MULTIGIBS");
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Total Multigibs"), p->ps.i_multigibs);
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Maximum Victims"), p->ps.i_maxmultigibs);
//
//	stats_text = va("%s \220%s\221\n", stats_text, "AIRGIBS");
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Total"), p->ps.i_airgibs);
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Total Height"), p->ps.i_height);
//	stats_text = va("%s  %s: %d\n", stats_text, redtext("Maximum Height"), p->ps.i_maxheight);
//	stats_text = va("%s  %s: %.1f\n", stats_text, redtext("Average Height"), p->ps.i_airgibs ? p->ps.i_height / p->ps.i_airgibs : 0.);
//	G_bprint(2, "%s", stats_text);

	// Points Frags Death Streaks Spawns CG %
	G_bprint(2,
			 "%s%-20s|%5d|%5d|%5d|%5d|%5d|%5s|\n",
			(isghost(p) ? "\203" : ""), getname(p),
			(int)p->s.v.frags,
			p->ps.i_cggibs + p->ps.i_axegibs + p->ps.i_stompgibs,
			(int)p->deaths,
			p->ps.spawn_frags,
			p->ps.spree_max,
			(a_sg ? va("%.1f%%", h_sg) : "-"));

//	maxfrags = max((isCTF() ? p->s.v.frags - p->ps.ctf_points : p->s.v.frags), maxfrags);
//	maxdeaths = max(p->deaths, maxdeaths);
//	maxfriend = max(p->friendly, maxfriend);
//	maxeffi = max(p->efficiency, maxeffi);
//	maxcaps = max(p->ps.caps, maxcaps);
//	maxdefends = max(p->ps.f_defends, maxdefends);
//	maxspree = max(p->ps.spree_max, maxspree);
//	maxspree_q = max(p->ps.spree_max_q, maxspree_q);
//	maxdmgg = max(p->ps.dmg_g, maxdmgg);
//	maxrlkills = max(p->ps.wpn[wpRL].ekills, maxrlkills);
//	maxsgeffi = max(h_sg, maxsgeffi);
}

void OnePlayerLGCStats(gedict_t *p)
{
	int over;
	int under;
	float a_lg;
	float h_lg;
	float e_lg;

	over = p->ps.lgc_overshaft;
	under = p->ps.lgc_undershaft;
	a_lg = p->ps.wpn[wpLG].attacks;
	h_lg = p->ps.wpn[wpLG].hits;
	e_lg = 100.0 * h_lg / max(1, a_lg);

	G_bprint(2,
			 "%s%-20s|%5d|%5s|%5s|%5s|\n",
			(isghost(p) ? "\203" : ""), getname(p),
			(int)(e_lg * p->s.v.frags),
			(a_lg != 0 ? va("%.1f%%", ((over * 100.0f) / a_lg)) : "0.0%"),
			(a_lg != 0 ? va("%.1f%%", ((under * 100.0f) / a_lg)) : "0.0%"),
			(e_lg == 100 ? va("%.0f%%", e_lg) : va("%.1f%%", e_lg)));
}

void OnePlayerKillStats(gedict_t *p, int tp)
{
	G_bprint(
			2,
			"%s%-20s|%5d|%5d|%5d|%5d|%5s|%5d|%5s|\n",
			(isghost(p) ? "\203" : ""), getname(p),
			(isCTF() ? (int)(p->s.v.frags - p->ps.ctf_points) : (int)p->s.v.frags),
			(int)(p->kills),
			(isCTF() ? (int)(p->ps.ctf_points - p->deaths) : (int)(p->deaths)),
			(int)(p->suicides),
			(tp ? va("%d", (int)p->friendly) : "-"),
			p->ps.spawn_frags,
			((p->efficiency >= 100) ? va("%.0f%%", p->efficiency) :
			((p->efficiency == 0)? va(" %.1f%%", p->efficiency) : va("%.1f%%", p->efficiency))));
}

void OnePlayerItemStats(gedict_t *p, int tp)
{
	G_bprint(2,
			"%s%-20s|%5d|%5d|%5d|%5d|%5s|%5s|%5s|\n",
			(isghost(p) ? "\203" : ""), getname(p),
			p->ps.itm[itGA].tooks,
			p->ps.itm[itYA].tooks,
			p->ps.itm[itRA].tooks,
			p->ps.itm[itHEALTH_100].tooks,
			(tp ? va("%d", p->ps.itm[itQUAD].tooks) : "-"),
			(tp ? va("%d", p->ps.itm[itPENT].tooks) : "-"),
			(tp ? va("%d", p->ps.itm[itRING].tooks) : "-"));
}

void OnePlayerWeaponEffiStats(gedict_t *p)
{
	float h_lg;
	float a_lg;
	float e_lg;
	float vh_rl;
	float a_rl;
	float e_rl;
	float vh_gl;
	float a_gl;
	float e_gl;
	float a_ssg;
	float e_ssg;
	float h_ssg;
	float h_sg;
	float a_sg;
	float e_sg;

	h_lg = p->ps.wpn[wpLG].hits;
	a_lg = p->ps.wpn[wpLG].attacks;
	vh_rl = p->ps.wpn[wpRL].vhits;
	a_rl = p->ps.wpn[wpRL].attacks;
	vh_gl = p->ps.wpn[wpGL].vhits;
	a_gl = p->ps.wpn[wpGL].attacks;
	h_ssg = p->ps.wpn[wpSSG].hits;
	a_ssg = p->ps.wpn[wpSSG].attacks;
	h_sg = p->ps.wpn[wpSG].hits;
	a_sg = p->ps.wpn[wpSG].attacks;

	e_lg = 100.0 * h_lg / max(1, a_lg);
	e_rl = 100.0 * vh_rl / max(1, a_rl);
	e_gl = 100.0 * vh_gl / max(1, a_gl);
	e_ssg = 100.0 * h_ssg / max(1, a_ssg);
	e_sg = 100.0 * h_sg / max(1, a_sg);

	G_bprint(2,
			"%s%-20s|%5s|%5s|%5s|%5s|%5s|\n",
			(isghost(p) ? "\203" : ""), getname(p),
			(a_lg ? ((e_lg >= 100)? va("%.0f%%", e_lg) : va("%.1f%%", e_lg)) : "-"),
			(a_rl ? ((e_rl >= 100) ? va("%.0f%%", e_rl) : va("%.1f%%", e_rl)) : "-"),
			(a_gl ? ((e_gl >= 100) ? va("%.0f%%", e_gl) : va("%.1f%%", e_gl)) : "-"),
			(a_ssg ? ((e_ssg >= 100) ? va("%.0f%%", e_ssg) : va("%.1f%%", e_ssg)) : "-"),
			(a_sg ? ((e_sg >= 100) ? va("%.0f%%", e_sg) : va("%.1f%%", e_sg)) : "-"));
}

void OnePlayerWeaponTakenStats(gedict_t *p)
{
	G_bprint(2,
			"%s%-20s|%5d|%5d|%5d|%5d|\n",
			(isghost(p) ? "\203" : ""), getname(p),
			p->ps.wpn[wpLG].tooks,
			p->ps.wpn[wpRL].tooks,
			p->ps.wpn[wpGL].tooks,
			p->ps.wpn[wpSSG].tooks);
}

void OnePlayerWeaponDroppedStats(gedict_t *p)
{
	G_bprint(2,
			"%s%-20s|%5d|%5d|%5d|%5d|\n",
			(isghost(p) ? "\203" : ""), getname(p),
			p->ps.wpn[wpLG].drops,
			p->ps.wpn[wpRL].drops,
			p->ps.wpn[wpGL].drops,
			p->ps.wpn[wpSSG].drops);
}

void OnePlayerWeaponKillStats(gedict_t *p)
{
	G_bprint(2,
			"%s%-20s|%5d|%5d|%5d|%5d|%5d|\n",
			(isghost(p) ? "\203" : ""), getname(p),
			p->ps.wpn[wpLG].kills,
			p->ps.wpn[wpRL].kills,
			p->ps.wpn[wpGL].kills,
			p->ps.wpn[wpSSG].kills,
			p->ps.wpn[wpSG].kills);
}

void OnePlayerEnemyWeaponKillStats(gedict_t *p)
{
	G_bprint(2,
			"%s%-20s|%5d|%5d|%5d|%5d|\n",
			(isghost(p) ? "\203" : ""), getname(p),
			p->ps.wpn[wpLG].ekills,
			p->ps.wpn[wpRL].ekills,
			p->ps.wpn[wpGL].ekills,
			p->ps.wpn[wpSSG].ekills);
}

void OnePlayerDamageStats(gedict_t *p)
{
	G_bprint(2,
			 "%s%-20s|%5d|%5d|%5d|%5d|%5d|%5d|\n",
			(isghost(p) ? "\203" : ""), getname(p),
			(int)p->ps.dmg_t,
			(int)p->ps.dmg_g,
			(int)p->ps.dmg_eweapon,
			(int)p->ps.dmg_team,
			(int)p->ps.dmg_self,
			(p->deaths <= 0 ? 99999 : (int)(p->ps.dmg_t / p->deaths)));
}

void OnePlayerCTFStats(gedict_t *p)
{
	int res = 0;
	int str = 0;
	int hst = 0;
	int rgn = 0;

	if ((g_globalvars.time - match_start_time) > 0)
	{
		res = (p->ps.res_time / (g_globalvars.time - match_start_time)) * 100;
		str = (p->ps.str_time / (g_globalvars.time - match_start_time)) * 100;
		hst = (p->ps.hst_time / (g_globalvars.time - match_start_time)) * 100;
		rgn = (p->ps.rgn_time / (g_globalvars.time - match_start_time)) * 100;
	}

	G_bprint(2,
			 "%s%-20s|%3d|%3d|%3d|%3d|%3d|%3s|%3s|%3s|%3s|\n",
			(isghost(p) ? "\203" : ""), getname(p),
			p->ps.pickups,
			p->ps.caps,
			p->ps.returns,
			p->ps.f_defends,
			p->ps.c_defends,
			((cvar("k_ctf_runes") ? va("%d%%", res) : " - ")),
			((cvar("k_ctf_runes") ? va("%d%%", str) : " - ")),
			((cvar("k_ctf_runes") ? va("%d%%", hst) : " - ")),
			((cvar("k_ctf_runes") ? va("%d%%", rgn) : " - ")));
}

void CalculateEfficiency(gedict_t *player)
{
	if (isCTF())
	{
		if ((player->s.v.frags - player->ps.ctf_points) < 1)
		{
			player->efficiency = 0;
		}
		else
		{
			player->efficiency = (player->s.v.frags - player->ps.ctf_points)
					/ (player->s.v.frags - player->ps.ctf_points + player->deaths) * 100;
		}
	}
	else if (isRA())
	{
		player->efficiency = (
				(player->ps.loses + player->ps.wins) ?
						(player->ps.wins * 100.0f) / (player->ps.loses + player->ps.wins) :
						0);
	}
	else
	{
		if (player->s.v.frags < 1)
		{
			player->efficiency = 0;
		}
		else
		{
			player->efficiency = player->s.v.frags / (player->s.v.frags + player->deaths) * 100;
		}
	}
}

void PlayerMidairStats(void)
{
	gedict_t *p;
	gedict_t *p2;
	char *tmp;
	char *tmp2;
	int from1;
	int from2;

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		p->ready = 0; // clear mark
		p = find_plrghst(p, &from1);
	}

	G_bprint(
			2, "\n%s   |%s|%s|%s|%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Midair statistics"), redtext("Tot"), redtext("Bro"),
			redtext("Sil"), redtext("Gol"), redtext("Pla"), redtext("Sto"),
			redtext("Bon"), redtext("Max H"), redtext("Avg H"));

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		if (!p->ready)
		{
			from2 = 0;
			p2 = find_plrghst(world, &from2);
			while (p2)
			{
				if (!p2->ready)
				{
					// sort by team
					tmp = getteam(p);
					tmp2 = getteam(p2);

					if (streq(tmp, tmp2))
					{
						p2->ready = 1; // set mark
						OnePlayerMidairStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

void PlayerMidairKillStats(void)
{
	gedict_t *p;
	gedict_t *p2;
	char *tmp;
	char *tmp2;
	int from1;
	int from2;

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		p->ready = 0; // clear mark
		p = find_plrghst(p, &from1);
	}

	G_bprint(
			2, "\n%s     |%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\237\n",
			redtext("Kill statistics"), redtext(" Frag"), redtext("SpwnF"), redtext(" Strk"), redtext(" RL %"));

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		if (!p->ready)
		{
			from2 = 0;
			p2 = find_plrghst(world, &from2);
			while (p2)
			{
				if (!p2->ready)
				{
					// sort by team
					tmp = getteam(p);
					tmp2 = getteam(p2);

					if (streq(tmp, tmp2))
					{
						p2->ready = 1; // set mark
						OnePlayerMidairKillStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

void PlayerInstagibStats()
{
	gedict_t *p;
	gedict_t *p2;
	char *tmp;
	char *tmp2;
	int from1;
	int from2;

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		p->ready = 0; // clear mark
		p = find_plrghst(p, &from1);
	}

	G_bprint(
			2, "\n%s |%s|%s|%s|%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\237\n",
			redtext("Instagib statistics"), redtext("Coil"), redtext(" Axe"), redtext("Stmp"),
			redtext("Mult"), redtext("MMax"), redtext(" Air"), redtext("TotH"), redtext("MaxH"), redtext("Ring"));
	// Coilgun Axe Stomps Multigibs MaxMG Airgibs TotalH. MaxH. Ring

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		if (!p->ready)
		{
			from2 = 0;
			p2 = find_plrghst(world, &from2);
			while (p2)
			{
				if (!p2->ready)
				{
					// sort by team
					tmp = getteam(p);
					tmp2 = getteam(p2);

					if (streq(tmp, tmp2))
					{
						p2->ready = 1; // set mark
						OnePlayerInstagibStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

void PlayerInstagibKillStats()
{
	gedict_t *p;
	gedict_t *p2;
	char *tmp;
	char *tmp2;
	int from1;
	int from2;

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		p->ready = 0; // clear mark
		p = find_plrghst(p, &from1);
	}

	G_bprint(
			2, "\n%s     |%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Kill statistics"), redtext("Point"), redtext(" Frag"), redtext("Death"),
			redtext("SpwnF"), redtext(" Strk"), redtext(" CG %"));

	// Points Frags Death Streaks Spawns CG %

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		if (!p->ready)
		{
			from2 = 0;
			p2 = find_plrghst(world, &from2);
			while (p2)
			{
				if (!p2->ready)
				{
					// sort by team
					tmp = getteam(p);
					tmp2 = getteam(p2);

					if (streq(tmp, tmp2))
					{
						p2->ready = 1; // set mark
						OnePlayerInstagibKillStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

void PlayerLGCStats()
{
	gedict_t *p;
	gedict_t *p2;
	char *tmp;
	char *tmp2;
	int from1;
	int from2;

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		p->ready = 0; // clear mark
		p = find_plrghst(p, &from1);
	}


	G_bprint(
			2, "\n%s      |%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\237\n",
			redtext("LGC statistics"), redtext("Score"), redtext(" Over"), redtext("Under"), redtext(" LG %"));

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		if (!p->ready)
		{
			from2 = 0;
			p2 = find_plrghst(world, &from2);
			while (p2)
			{
				if (!p2->ready)
				{
					// sort by team
					tmp = getteam(p);
					tmp2 = getteam(p2);

					if (streq(tmp, tmp2))
					{
						p2->ready = 1; // set mark
						OnePlayerLGCStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

void PlayersKillStats(void)
{
	gedict_t *p;
	gedict_t *p2;
	char *tmp;
	char *tmp2;
	int tp;
	int from1;
	int from2;

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		p->ready = 0; // clear mark
		p = find_plrghst(p, &from1);
	}

	tp = isTeam() || isCTF();

	G_bprint(
			2, "\n%s     |%s|%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Kill Statistics"), redtext(" Frag"), redtext(" Kill"), redtext("Death"),
			redtext("Suici"), redtext("TKill"), redtext("SpwnF"), redtext("Effic"));

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		if (!p->ready)
		{
			from2 = 0;
			p2 = find_plrghst(world, &from2);
			while (p2)
			{
				if (!p2->ready)
				{
					// sort by team
					tmp = getteam(p);
					tmp2 = getteam(p2);

					if (streq(tmp, tmp2))
					{
						p2->ready = 1; // set mark
						CalculateEfficiency(p2);
						OnePlayerKillStats(p2, tp);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

void PlayersItemStats(void)
{
	gedict_t *p;
	gedict_t *p2;
	char *tmp;
	char *tmp2;
	int tp;
	int from1;
	int from2;

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		p->ready = 0; // clear mark
		p = find_plrghst(p, &from1);
	}

	tp = isTeam() || isCTF();

	G_bprint(
			2, "\n%s |%s|%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Armor+Powerup Taken"), redtext("   GA"), redtext("   YA"), redtext("   RA"),
			redtext("   MH"), redtext(" Quad"), redtext(" Pent"), redtext(" Ring"));

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		if (!p->ready)
		{
			from2 = 0;
			p2 = find_plrghst(world, &from2);
			while (p2)
			{
				if (!p2->ready)
				{
					// sort by team
					tmp = getteam(p);
					tmp2 = getteam(p2);

					if (streq(tmp, tmp2))
					{
						p2->ready = 1; // set mark
						OnePlayerItemStats(p2, tp);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

void PlayersWeaponEffiStats(void)
{
	gedict_t *p;
	gedict_t *p2;
	char *tmp;
	char *tmp2;
	int from1;
	int from2;

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		p->ready = 0; // clear mark
		p = find_plrghst(p, &from1);
	}

	G_bprint(
			2, "\n%s   |%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\237\n",
			redtext("Weapon Efficiency"), redtext("  LG%"), redtext("  RL%"), redtext("  GL%"),
			redtext(" SSG%"), redtext("  SG%"));

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		if (!p->ready)
		{
			from2 = 0;
			p2 = find_plrghst(world, &from2);
			while (p2)
			{
				if (!p2->ready)
				{
					// sort by team
					tmp = getteam(p);
					tmp2 = getteam(p2);

					if (streq(tmp, tmp2))
					{
						p2->ready = 1; // set mark
						OnePlayerWeaponEffiStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

void PlayersWeaponTakenStats(void)
{
	gedict_t *p;
	gedict_t *p2;
	char *tmp;
	char *tmp2;
	int from1;
	int from2;

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		p->ready = 0; // clear mark
		p = find_plrghst(p, &from1);
	}

	G_bprint(
			2, "\n%s        |%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\237\n",
			redtext("Weapon Taken"), redtext("   LG"), redtext("   RL"), redtext("   GL"),
			redtext("  SSG"));

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		if (!p->ready)
		{
			from2 = 0;
			p2 = find_plrghst(world, &from2);
			while (p2)
			{
				if (!p2->ready)
				{
					// sort by team
					tmp = getteam(p);
					tmp2 = getteam(p2);

					if (streq(tmp, tmp2))
					{
						p2->ready = 1; // set mark
						OnePlayerWeaponTakenStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

void PlayersWeaponDroppedStats(void)
{
	gedict_t *p;
	gedict_t *p2;
	char *tmp;
	char *tmp2;
	int from1;
	int from2;

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		p->ready = 0; // clear mark
		p = find_plrghst(p, &from1);
	}

	G_bprint(
			2, "\n%s      |%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\237\n",
			redtext("Weapon Dropped"), redtext("   LG"), redtext("   RL"), redtext("   GL"),
			redtext("  SSG"));

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		if (!p->ready)
		{
			from2 = 0;
			p2 = find_plrghst(world, &from2);
			while (p2)
			{
				if (!p2->ready)
				{
					// sort by team
					tmp = getteam(p);
					tmp2 = getteam(p2);

					if (streq(tmp, tmp2))
					{
						p2->ready = 1; // set mark
						OnePlayerWeaponDroppedStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

void PlayersWeaponKillStats(void)
{
	gedict_t *p;
	gedict_t *p2;
	char *tmp;
	char *tmp2;
	int from1;
	int from2;

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		p->ready = 0; // clear mark
		p = find_plrghst(p, &from1);
	}

	G_bprint(
			2, "\n%s        |%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\237\n",
			redtext("Weapon Kills"), redtext("   LG"), redtext("   RL"), redtext("   GL"),
			redtext("  SSG"), redtext("   SG"));

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		if (!p->ready)
		{
			from2 = 0;
			p2 = find_plrghst(world, &from2);
			while (p2)
			{
				if (!p2->ready)
				{
					// sort by team
					tmp = getteam(p);
					tmp2 = getteam(p2);

					if (streq(tmp, tmp2))
					{
						p2->ready = 1; // set mark
						OnePlayerWeaponKillStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

void PlayersEnemyWeaponKillStats(void)
{
	gedict_t *p;
	gedict_t *p2;
	char *tmp;
	char *tmp2;
	int from1;
	int from2;

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		p->ready = 0; // clear mark
		p = find_plrghst(p, &from1);
	}

	G_bprint(
			2, "\n%s |%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\237\n",
			redtext("Enemy Weapon Killed"), redtext("   LG"), redtext("   RL"), redtext("   GL"),
			redtext("  SSG"));

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		if (!p->ready)
		{
			from2 = 0;
			p2 = find_plrghst(world, &from2);
			while (p2)
			{
				if (!p2->ready)
				{
					// sort by team
					tmp = getteam(p);
					tmp2 = getteam(p2);

					if (streq(tmp, tmp2))
					{
						p2->ready = 1; // set mark
						OnePlayerEnemyWeaponKillStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

void PlayersDamageStats(void)
{
	gedict_t *p;
	gedict_t *p2;
	char *tmp;
	char *tmp2;
	int from1;
	int from2;

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		p->ready = 0; // clear mark
		p = find_plrghst(p, &from1);
	}

	G_bprint(
			2, "\n%s   |%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Damage statistics"), redtext("Taken"), redtext("Given"), redtext("EWeap"),
			redtext(" Team"), redtext(" Self"), redtext("ToDie"));

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		if (!p->ready)
		{
			from2 = 0;
			p2 = find_plrghst(world, &from2);
			while (p2)
			{
				if (!p2->ready)
				{
					// sort by team
					tmp = getteam(p);
					tmp2 = getteam(p2);

					if (streq(tmp, tmp2))
					{
						p2->ready = 1; // set mark
						OnePlayerDamageStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

void PlayersCTFStats(void)
{
	gedict_t *p;
	gedict_t *p2;
	char *tmp;
	char *tmp2;
	int from1;
	int from2;

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		p->ready = 0; // clear mark
		p = find_plrghst(p, &from1);
	}

	G_bprint(
			2, "\n%s      |%s|%s|%s|%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("CTF statistics"), redtext("Fla"), redtext("Cap"), redtext("Ret"),
			redtext("DFl"), redtext("DCa"), redtext("Res"), redtext("Str"), redtext("Hst"),
			redtext("Rgn"));

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		if (!p->ready)
		{
			from2 = 0;
			p2 = find_plrghst(world, &from2);
			while (p2)
			{
				if (!p2->ready)
				{
					// sort by team
					tmp = getteam(p);
					tmp2 = getteam(p2);

					if (streq(tmp, tmp2))
					{
						p2->ready = 1; // set mark
						OnePlayerCTFStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

// Print the high score table
void TopStats(void)
{
	gedict_t *p;
	float f1, h_sg, a_sg;
	int from;

	G_bprint(2, "\220%s\221 %s:\n"
				"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n"
				"      Frags: ",
				mapname, redtext("top scorers"));

	from = f1 = 0;
	p = find_plrghst(world, &from);
	while (p)
	{
		if ((!isCTF() && (p->s.v.frags == maxfrags))
				|| (isCTF() && ((p->s.v.frags - p->ps.ctf_points) == maxfrags)))
		{
			G_bprint(2, "%s%s%s \220%d\221\n", (f1 ? "             " : ""),
						(isghost(p) ? "\203" : ""), getname(p), (int)maxfrags);
			f1 = 1;
		}

		p = find_plrghst(p, &from);
	}

	G_bprint(2, "     Deaths: ");

	from = f1 = 0;
	p = find_plrghst(world, &from);
	while (p)
	{
		if (p->deaths == maxdeaths)
		{
			G_bprint(2, "%s%s%s \220%d\221\n", (f1 ? "             " : ""),
						(isghost(p) ? "\203" : ""), getname(p), (int)maxdeaths);
			f1 = 1;
		}

		p = find_plrghst(p, &from);
	}

	if (maxfriend)
	{
		G_bprint(2, "Friendkills: ");

		from = f1 = 0;
		p = find_plrghst(world, &from);
		while (p)
		{
			if (p->friendly == maxfriend)
			{
				G_bprint(2, "%s%s%s \220%d\221\n", (f1 ? "             " : ""),
							(isghost(p) ? "\203" : ""), getname(p), (int)maxfriend);
				f1 = 1;
			}

			p = find_plrghst(p, &from);
		}
	}

	G_bprint(2, " Efficiency: ");

	from = f1 = 0;
	p = find_plrghst(world, &from);
	while (p)
	{
		if (p->efficiency == maxeffi)
		{
			G_bprint(2, "%s%s%s \220%.1f%%\221\n", (f1 ? "             " : ""),
						(isghost(p) ? "\203" : ""), getname(p), maxeffi);
			f1 = 1;
		}

		p = find_plrghst(p, &from);
	}

	if (maxspree)
	{
		G_bprint(2, " FragStreak: ");
		from = f1 = 0;
		p = find_plrghst(world, &from);
		while (p)
		{
			if (p->ps.spree_max == maxspree)
			{
				G_bprint(2, "%s%s%s \220%d\221\n", (f1 ? "             " : ""),
							(isghost(p) ? "\203" : ""), getname(p), maxspree);
				f1 = 1;
			}

			p = find_plrghst(p, &from);
		}
	}

	if (maxspree_q)
	{
		G_bprint(2, "    QuadRun: ");
		from = f1 = 0;
		p = find_plrghst(world, &from);
		while (p)
		{
			if (p->ps.spree_max_q == maxspree_q)
			{
				G_bprint(2, "%s%s%s \220%d\221\n", (f1 ? "             " : ""),
							(isghost(p) ? "\203" : ""), getname(p), maxspree_q);
				f1 = 1;
			}

			p = find_plrghst(p, &from);
		}
	}

	if (maxrlkills && deathmatch == 1)
	{
		G_bprint(2, "  RL Killer: ");
		from = f1 = 0;
		p = find_plrghst(world, &from);
		while (p)
		{
			if (p->ps.wpn[wpRL].ekills == maxrlkills)
			{
				G_bprint(2, "%s%s%s \220%d\221\n", (f1 ? "             " : ""),
							(isghost(p) ? "\203" : ""), getname(p), maxrlkills);
				f1 = 1;
			}

			p = find_plrghst(p, &from);
		}
	}

	if (maxsgeffi && deathmatch == 1)
	{
		G_bprint(2, "Boomsticker: ");
		from = f1 = 0;
		p = find_plrghst(world, &from);
		while (p)
		{
			h_sg = p->ps.wpn[wpSG].hits;
			a_sg = p->ps.wpn[wpSG].attacks;
			h_sg = 100.0 * h_sg / max(1, a_sg);
			if (h_sg == maxsgeffi)
			{
				G_bprint(2, "%s%s%s \220%.1f%%\221\n", (f1 ? "             " : ""),
							(isghost(p) ? "\203" : ""), getname(p), maxsgeffi);
				f1 = 1;
			}

			p = find_plrghst(p, &from);
		}
	}

	if (maxdmgtd)
	{
		G_bprint(2, "   Survivor: ");
		from = f1 = 0;
		p = find_plrghst(world, &from);
		while (p)
		{
			if ((int)(p->deaths <= 0 ? 99999 : p->ps.dmg_t / p->deaths) == maxdmgtd)
			{
				G_bprint(2, "%s%s%s \220%d\221\n", (f1 ? "             " : ""),
							(isghost(p) ? "\203" : ""), getname(p), maxdmgtd);
				f1 = 1;
			}

			p = find_plrghst(p, &from);
		}
	}

	if (maxdmgg)
	{
		G_bprint(2, "Annihilator: ");
		from = f1 = 0;
		p = find_plrghst(world, &from);
		while (p)
		{
			if (p->ps.dmg_g == maxdmgg)
			{
				G_bprint(2, "%s%s%s \220%d\221\n", (f1 ? "             " : ""),
							(isghost(p) ? "\203" : ""), getname(p), maxdmgg);
				f1 = 1;
			}

			p = find_plrghst(p, &from);
		}
	}

	if (isCTF())
	{
		if (maxcaps > 0)
		{
			G_bprint(2, "   Captures: ");
			from = f1 = 0;
			p = find_plrghst(world, &from);
			while (p)
			{
				if (p->ps.caps == maxcaps)
				{
					G_bprint(2, "%s%s%s \220%d\221\n", (f1 ? "             " : ""),
								(isghost(p) ? "\203" : ""), getname(p), (int)maxcaps);
					f1 = 1;
				}

				p = find_plrghst(p, &from);
			}
		}

		if (maxdefends > 0)
		{
			G_bprint(2, "FlagDefends: ");
			from = f1 = 0;
			p = find_plrghst(world, &from);
			while (p)
			{
				if (p->ps.f_defends == maxdefends)
				{
					G_bprint(2, "%s%s%s \220%d\221\n", (f1 ? "             " : ""),
								(isghost(p) ? "\203" : ""), getname(p), (int)maxdefends);
					f1 = 1;
				}

				p = find_plrghst(p, &from);
			}
		}
	}

	G_bprint(2, "\n\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n");
}

void TopMidairStats()
{
	gedict_t *p;
	float f1, vh_rl, a_rl, ph_rl, maxtopheight = 0, maxtopavgheight = 0, maxrlefficiency = 0;
	int from, maxscore = -99999, maxkills = 0, maxmidairs = 0, maxstomps = 0, maxstreak = 0,
			maxspawnfrags = 0, maxbonus = 0;

	from = f1 = 0;
	p = find_plrghst(world, &from);
	while (p)
	{
		maxscore = max(p->s.v.frags, maxscore);
		maxkills = max(p->ps.mid_total + p->ps.mid_stomps, maxkills);
		maxmidairs = max(p->ps.mid_total, maxmidairs);
		maxstomps = max(p->ps.mid_stomps, maxstomps);
		maxstreak = max(p->ps.spree_max, maxstreak);
		maxspawnfrags = max(p->ps.spawn_frags, maxspawnfrags);
		maxbonus = max(p->ps.mid_bonus, maxbonus);
		maxtopheight = max(p->ps.mid_maxheight, maxtopheight);
		maxtopavgheight = max(p->ps.mid_avgheight, maxtopavgheight);
		vh_rl = p->ps.wpn[wpRL].vhits;
		a_rl = p->ps.wpn[wpRL].attacks;
		ph_rl = 100.0 * vh_rl / max(1, a_rl);
		maxrlefficiency = max(ph_rl, maxrlefficiency);

		p = find_plrghst(p, &from);
	}

	G_bprint(2, "%s:\n\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
				redtext("Top performers"));

	from = f1 = 0;
	p = find_plrghst(world, &from);
	while (p)
	{
		if (p->s.v.frags == maxscore)
		{
			G_bprint(2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("score")),
						(isghost(p) ? "\203" : ""), getname(p), maxscore);
			f1 = 1;
		}

		p = find_plrghst(p, &from);
	}

	if (maxkills)
	{
		from = f1 = 0;
		p = find_plrghst(world, &from);
		while (p)
		{
			if ((p->ps.mid_total + p->ps.mid_stomps) == maxkills)
			{
				G_bprint(2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("kills")),
							(isghost(p) ? "\203" : ""), getname(p), maxkills);
				f1 = 1;
			}

			p = find_plrghst(p, &from);
		}
	}

	if (maxmidairs)
	{
		from = f1 = 0;
		p = find_plrghst(world, &from);
		while (p)
		{
			if ((p->ps.mid_total) == maxmidairs)
			{
				G_bprint(2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("midairs")),
							(isghost(p) ? "\203" : ""), getname(p), maxmidairs);
				f1 = 1;
			}

			p = find_plrghst(p, &from);
		}
	}

	if (maxstomps)
	{
		from = f1 = 0;
		p = find_plrghst(world, &from);
		while (p)
		{
			if ((p->ps.mid_stomps) == maxstomps)
			{
				G_bprint(2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("head stomps")),
							(isghost(p) ? "\203" : ""), getname(p), maxstomps);
				f1 = 1;
			}
			p = find_plrghst(p, &from);
		}
	}

	if (maxstreak)
	{
		from = f1 = 0;
		p = find_plrghst(world, &from);
		while (p)
		{
			if ((p->ps.spree_max) == maxstreak)
			{
				G_bprint(2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("streak")),
							(isghost(p) ? "\203" : ""), getname(p), maxmidairs);
				f1 = 1;
			}

			p = find_plrghst(p, &from);
		}
	}

	if (maxspawnfrags)
	{
		from = f1 = 0;
		p = find_plrghst(world, &from);
		while (p)
		{
			if ((p->ps.spawn_frags) == maxspawnfrags)
			{
				G_bprint(2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("spawn frags")),
							(isghost(p) ? "\203" : ""), getname(p), maxspawnfrags);
				f1 = 1;
			}

			p = find_plrghst(p, &from);
		}
	}

	if (maxbonus)
	{
		from = f1 = 0;
		p = find_plrghst(world, &from);
		while (p)
		{
			if ((p->ps.mid_bonus) == maxbonus)
			{
				G_bprint(2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("bonus fiend")),
							(isghost(p) ? "\203" : ""), getname(p), maxbonus);
				f1 = 1;
			}

			p = find_plrghst(p, &from);
		}
	}

	if (maxtopheight)
	{
		from = f1 = 0;
		p = find_plrghst(world, &from);
		while (p)
		{
			if ((p->ps.mid_maxheight) == maxtopheight)
			{
				G_bprint(2, "   %-13s: %s%s (%.1f)\n", (f1 ? "" : redtext("highest kill")),
							(isghost(p) ? "\203" : ""), getname(p), maxtopheight);
				f1 = 1;
			}

			p = find_plrghst(p, &from);
		}
	}

	if (maxtopavgheight)
	{
		from = f1 = 0;
		p = find_plrghst(world, &from);
		while (p)
		{
			if ((p->ps.mid_avgheight) == maxtopavgheight)
			{
				G_bprint(2, "   %-13s: %s%s (%.1f)\n", (f1 ? "" : redtext("avg height")),
							(isghost(p) ? "\203" : ""), getname(p), maxtopavgheight);
				f1 = 1;
			}

			p = find_plrghst(p, &from);
		}
	}

	from = f1 = 0;
	p = find_plrghst(world, &from);
	while (p)
	{
		vh_rl = p->ps.wpn[wpRL].vhits;
		a_rl = p->ps.wpn[wpRL].attacks;
		ph_rl = 100.0 * vh_rl / max(1, a_rl);
		if ((ph_rl) == maxrlefficiency)
		{
			G_bprint(2, "   %-13s: %s%s (%.1f%%)\n", (f1 ? "" : redtext("rl efficiency")),
						(isghost(p) ? "\203" : ""), getname(p), maxrlefficiency);
			f1 = 1;
		}

		p = find_plrghst(p, &from);
	}

	G_bprint(2, "\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n");
}

void EM_CorrectStats(void)
{
	gedict_t *p;

	for (p = world; (p = find_plr(p));)
	{
		// take away powerups so scoreboard looks normal
		p->s.v.items = (int)p->s.v.items
				& ~(IT_INVISIBILITY | IT_INVULNERABILITY | IT_SUIT | IT_QUAD);
		p->s.v.effects = (int)p->s.v.effects
				& ~(EF_DIMLIGHT | EF_BRIGHTLIGHT | EF_BLUE | EF_RED | EF_GREEN);
		p->invisible_finished = 0;
		p->invincible_finished = 0;
		p->super_damage_finished = 0;
		p->radsuit_finished = 0;

		p->ps.spree_max = max(p->ps.spree_current, p->ps.spree_max);
		p->ps.spree_max_q = max(p->ps.spree_current_q, p->ps.spree_max_q);

		adjust_pickup_time(&p->q_pickup_time, &p->ps.itm[itQUAD].time);
		adjust_pickup_time(&p->p_pickup_time, &p->ps.itm[itPENT].time);
		adjust_pickup_time(&p->r_pickup_time, &p->ps.itm[itRING].time);

		if (p->control_start_time)
		{
			p->ps.control_time += g_globalvars.time - p->control_start_time;
			p->control_start_time = 0;
		}

		if (isCTF())
		{
			// if a player ends the game with a rune adjust their rune time
			if (p->ctf_flag & CTF_RUNE_RES)
			{
				p->ps.res_time += g_globalvars.time - p->rune_pickup_time;
			}
			else if (p->ctf_flag & CTF_RUNE_STR)
			{
				p->ps.str_time += g_globalvars.time - p->rune_pickup_time;
			}
			else if (p->ctf_flag & CTF_RUNE_HST)
			{
				p->ps.hst_time += g_globalvars.time - p->rune_pickup_time;
			}
			else if (p->ctf_flag & CTF_RUNE_RGN)
			{
				p->ps.rgn_time += g_globalvars.time - p->rune_pickup_time;
			}
		}
	}
}

void MatchEndStats(void)
{
	gedict_t *p;

	if (isHoonyModeAny() && !HM_is_game_over())
	{
		return;
	}

	if (isTeam() || isCTF())
	{
		CollectTpStats();
	}

	if (isRACE())
	{
		race_match_stats();
	}
	else if (cvar("k_midair"))
	{
		PlayerMidairStats();
		PlayerMidairKillStats();
		TopMidairStats();
	}
	else if (cvar("k_instagib"))
	{
		PlayerInstagibStats();
		PlayerInstagibKillStats();
	}
	else if (lgc_enabled())
	{
		PlayerLGCStats();
	}
	else
	{
		PlayersKillStats();
		PlayersItemStats();
		PlayersWeaponEffiStats();
		PlayersWeaponTakenStats();
		PlayersWeaponDroppedStats();
		PlayersWeaponKillStats();
		PlayersEnemyWeaponKillStats();
		PlayersDamageStats();
		// (quad)time
		if (isCTF())
		{
			PlayersCTFStats();
		}


		if (isTeam() || isCTF())
		{
			SummaryTPStats(); // print summary stats like armos powerups weapons etc..
		}

		if (!isDuel()) // top stats only in non duel modes
		{
			TopStats(); // print top frags tkills deaths...
		}

		if (isTeam() || isCTF())
		{
			TeamsStats(); // print basic info like frags for each team
		}

		if ((p = find(world, FOFCLSN, "ghost"))) // show legend :)
		{
			G_bprint(2, "\n\203 - %s player\n\n", redtext("disconnected"));
		}
	}

	StatsToFile();
}

void SM_PrepareTeamsStats(void)
{
	int i;

	tmStats_cnt = 0;
	memset(tmStats, 0, sizeof(tmStats));
	memset(tmStats_names, 0, sizeof(tmStats_names));

	for (i = 0; i < MAX_TM_STATS; i++)
	{
		tmStats[i].name = tmStats_names[i];
	}
}
