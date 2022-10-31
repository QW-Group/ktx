/*
 * This file contains the logic to print the overhauled Endgame Statistics, invoked with the /laststats command
 */
#include "g_local.h"
#include "stats.h"

static void onePlayerMidairStats(gedict_t *p);
static void onePlayerMidairKillStats(gedict_t *p);
static void onePlayerInstagibStats(gedict_t *p);
static void onePlayerInstagibKillStats(gedict_t *p);
static void onePlayerLGCStats(gedict_t *p);
static void onePlayerKillStats(gedict_t *p, int tp);
static void onePlayerItemStats(gedict_t *p, int tp);
static void onePlayerWeaponEffiStats(gedict_t *p);
static void onePlayerWeaponDmgStats(gedict_t *p);
static void onePlayerWeaponTakenStats(gedict_t *p);
static void onePlayerWeaponDroppedStats(gedict_t *p);
static void onePlayerWeaponKillStats(gedict_t *p);
static void onePlayerEnemyWeaponKillStats(gedict_t *p);
static void onePlayerDamageStats(gedict_t *p);
static void onePlayerItemTimeStats(gedict_t *p, int tp);
static void onePlayerWeaponTimeStats(gedict_t *p);
static void onePlayerCTFStats(gedict_t *p);
static void calculateEfficiency(gedict_t *player);
static void playerMidairStats(void);
static void playerMidairKillStats(void);
static void topMidairStats(void);
static void playerInstagibStats(void);
static void playerInstagibKillStats(void);
static void playerLGCStats(void);
static void playersKillStats(void);
static void playersItemStats(void);
static void playersWeaponEffiStats(void);
static void playersWeaponDmgStats(void);
static void playersWeaponTakenStats(void);
static void playersWeaponDroppedStats(void);
static void playersWeaponKillStats(void);
static void playersEnemyWeaponKillStats(void);
static void playersDamageStats(void);
static void playersItemTimeStats(void);
static void playersWeaponTimeStats(void);
static void playersCTFStats(void);

static void collectTpStats(void);
static void summaryTPStats(void);
static void topStats(void);

static teamStats_t tmStatsTables[MAX_TM_STATS];
static int tmStatsTables_cnt = 0;
qbool lastStatsData = false;

void MatchEndStatsTables(void)
{
	gedict_t *p;

	if (isHoonyModeAny() && !HM_is_game_over())
	{
		return;
	}

	if (!lastStatsData)
	{
		G_sprint(self, 2, "Laststats data empty\n");

		return;
	}

	if (cvar("k_midair"))
	{
		playerMidairStats();
		playerMidairKillStats();
		topMidairStats();
	}
	else if (cvar("k_instagib"))
	{
		playerInstagibStats();
		playerInstagibKillStats();
	}
	else if (lgc_enabled())
	{
		playerLGCStats();
	}
	else
	{
		playersKillStats();
		playersItemStats();
		playersWeaponEffiStats();
		playersWeaponDmgStats();
		playersWeaponTakenStats();
		playersWeaponDroppedStats();
		playersWeaponKillStats();
		playersEnemyWeaponKillStats();
		playersDamageStats();
		playersItemTimeStats();
		playersWeaponTimeStats();

		if (isCTF())
		{
			playersCTFStats();
		}

		if (isTeam() || isCTF())
		{
			collectTpStats();
			summaryTPStats(); // print summary stats like armos powerups weapons etc..
		}

		if (!isDuel()) // top stats only in non duel modes
		{
			topStats(); // print top frags tkills deaths...
		}

		if ((p = find(world, FOFCLSN, "ghost"))) // show legend :)
		{
			G_sprint(self, 2, "\n\203 - %s player\n\n", redtext("disconnected"));
		}
	}

	StatsToFile();
}

static void onePlayerMidairStats(gedict_t *p)
{
	G_sprint(self, 2, "%-20s|%3d|%3d|%3d|%3d|%3d|%3d|%3d|%5s|%5s|\n",
			va("%s%s", isghost(p) ? "\203" : "", getname(p)),
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

static void onePlayerMidairKillStats(gedict_t *p)
{
	float vh_rl;
	float a_rl;
	float e_rl;

	vh_rl = p->ps.wpn[wpRL].vhits;
	a_rl = p->ps.wpn[wpRL].attacks;
	e_rl = 100.0 * vh_rl / max(1, a_rl);

	G_sprint(self, 2, "%-20s|%5d|%5d|%5d|%5s|\n",
			va("%s%s", isghost(p) ? "\203" : "", getname(p)),
			(int)p->s.v.frags,
			p->ps.spawn_frags,
			p->ps.spree_max,
			(e_rl == 100 ? va("%.0f%%", e_rl) : va("%.1f%%", e_rl)));
}

static void onePlayerInstagibStats(gedict_t *p)
{
	G_sprint(self, 2, "%-20s|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|\n",
			va("%s%s", isghost(p) ? "\203" : "", getname(p)),
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

static void onePlayerInstagibKillStats(gedict_t *p)
{
	float h_sg;
	float a_sg;

	h_sg = p->ps.wpn[wpSG].hits;
	a_sg = p->ps.wpn[wpSG].attacks;

	G_sprint(self, 2, "%-20s|%5d|%5d|%5d|%5d|%5d|%5s|\n",
			va("%s%s", isghost(p) ? "\203" : "", getname(p)),
			(int)p->s.v.frags,
			p->ps.i_cggibs + p->ps.i_axegibs + p->ps.i_stompgibs,
			(int)p->deaths,
			p->ps.spawn_frags,
			p->ps.spree_max,
			(a_sg ? va("%.1f%%", h_sg) : "-"));
}

static void onePlayerLGCStats(gedict_t *p)
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

	G_sprint(self, 2, "%-20s|%5d|%5s|%5s|%5s|\n",
			va("%s%s", isghost(p) ? "\203" : "", getname(p)),
			(int)(e_lg * p->s.v.frags),
			(a_lg != 0 ? va("%.1f%%", ((over * 100.0f) / a_lg)) : "0.0%"),
			(a_lg != 0 ? va("%.1f%%", ((under * 100.0f) / a_lg)) : "0.0%"),
			(e_lg == 100 ? va("%.0f%%", e_lg) : va("%.1f%%", e_lg)));
}

static void onePlayerKillStats(gedict_t *p, int tp)
{
	G_sprint(self, 2, "%-20s|%5d|%5d|%5d|%5d|%5s|%5d|%5s|\n",
			va("%s%s", isghost(p) ? "\203" : "", getname(p)),
			(isCTF() ? (int)(p->s.v.frags - p->ps.ctf_points) : (int)p->s.v.frags),
			(int)(p->kills),
			(isCTF() ? (int)(p->ps.ctf_points - p->deaths) : (int)(p->deaths)),
			(int)(p->suicides),
			(tp ? va("%d", (int)p->friendly) : "-"),
			p->ps.spawn_frags,
			((p->efficiency >= 100) ? va("%.0f%%", p->efficiency) :
			((p->efficiency == 0)? va(" %.1f%%", p->efficiency) : va("%.1f%%", p->efficiency))));
}

static void onePlayerItemStats(gedict_t *p, int tp)
{
	G_sprint(self, 2, "%-20s|%5d|%5d|%5d|%5d|%5s|%5s|%5s|\n",
			va("%s%s", isghost(p) ? "\203" : "", getname(p)),
			p->ps.itm[itGA].tooks,
			p->ps.itm[itYA].tooks,
			p->ps.itm[itRA].tooks,
			p->ps.itm[itHEALTH_100].tooks,
			(tp ? va("%d", p->ps.itm[itQUAD].tooks) : "-"),
			(tp ? va("%d", p->ps.itm[itPENT].tooks) : "-"),
			(tp ? va("%d", p->ps.itm[itRING].tooks) : "-"));
}

static void onePlayerWeaponEffiStats(gedict_t *p)
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
	float a_sng;
	float e_sng;
	float h_sng;
	float a_ng;
	float e_ng;
	float h_ng;
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
	h_sng = p->ps.wpn[wpSNG].hits;
	a_sng = p->ps.wpn[wpSNG].attacks;
	h_ng = p->ps.wpn[wpNG].hits;
	a_ng = p->ps.wpn[wpNG].attacks;
	h_ssg = p->ps.wpn[wpSSG].hits;
	a_ssg = p->ps.wpn[wpSSG].attacks;
	h_sg = p->ps.wpn[wpSG].hits;
	a_sg = p->ps.wpn[wpSG].attacks;

	e_lg = 100.0 * h_lg / max(1, a_lg);
	e_rl = 100.0 * vh_rl / max(1, a_rl);
	e_gl = 100.0 * vh_gl / max(1, a_gl);
	e_sng = 100.0 * h_sng / max(1, a_sng);
	e_ng = 100.0 * h_ng / max(1, a_ng);
	e_ssg = 100.0 * h_ssg / max(1, a_ssg);
	e_sg = 100.0 * h_sg / max(1, a_sg);

	G_sprint(self, 2, "%-20s|%5s|%5s|%5s|%5s|%5s|%5s|%5s|\n",
			va("%s%s", isghost(p) ? "\203" : "", getname(p)),
			(a_lg ? ((e_lg >= 100)? va("%.0f%%", e_lg) : va("%.1f%%", e_lg)) : "-"),
			(a_rl ? ((e_rl >= 100) ? va("%.0f%%", e_rl) : va("%.1f%%", e_rl)) : "-"),
			(a_gl ? ((e_gl >= 100) ? va("%.0f%%", e_gl) : va("%.1f%%", e_gl)) : "-"),
			(a_sng ? ((e_sng >= 100) ? va("%.0f%%", e_sng) : va("%.1f%%", e_sng)) : "-"),
			(a_ng ? ((e_ng >= 100) ? va("%.0f%%", e_ng) : va("%.1f%%", e_ng)) : "-"),
			(a_ssg ? ((e_ssg >= 100) ? va("%.0f%%", e_ssg) : va("%.1f%%", e_ssg)) : "-"),
			(a_sg ? ((e_sg >= 100) ? va("%.0f%%", e_sg) : va("%.1f%%", e_sg)) : "-"));
}

static void onePlayerWeaponDmgStats(gedict_t *p)
{
	G_sprint(self, 2, "%-20s|%5d|%5d|%5d|%5d|%5d|%5d|%5d|\n",
			va("%s%s", isghost(p) ? "\203" : "", getname(p)),
			p->ps.wpn[wpLG].edamage,
			p->ps.wpn[wpRL].edamage,
			p->ps.wpn[wpGL].edamage,
			p->ps.wpn[wpSNG].edamage,
			p->ps.wpn[wpNG].edamage,
			p->ps.wpn[wpSSG].edamage,
			p->ps.wpn[wpSG].edamage);
}

static void onePlayerWeaponTakenStats(gedict_t *p)
{
	G_sprint(self, 2, "%-20s|%5d|%5d|%5d|%5d|%5d|%5d|\n",
			va("%s%s", isghost(p) ? "\203" : "", getname(p)),
			p->ps.wpn[wpLG].tooks,
			p->ps.wpn[wpRL].tooks,
			p->ps.wpn[wpGL].tooks,
			p->ps.wpn[wpSNG].tooks,
			p->ps.wpn[wpNG].tooks,
			p->ps.wpn[wpSSG].tooks);
}

static void onePlayerWeaponDroppedStats(gedict_t *p)
{
	G_sprint(self, 2, "%-20s|%5d|%5d|%5d|%5d|%5d|%5d|\n",
			va("%s%s", isghost(p) ? "\203" : "", getname(p)),
			p->ps.wpn[wpLG].drops,
			p->ps.wpn[wpRL].drops,
			p->ps.wpn[wpGL].drops,
			p->ps.wpn[wpSNG].drops,
			p->ps.wpn[wpNG].drops,
			p->ps.wpn[wpSSG].drops);
}

static void onePlayerWeaponKillStats(gedict_t *p)
{
	G_sprint(self, 2, "%-20s|%5d|%5d|%5d|%5d|%5d|%5d|\n",
			va("%s%s", isghost(p) ? "\203" : "", getname(p)),
			p->ps.wpn[wpLG].kills,
			p->ps.wpn[wpRL].kills,
			p->ps.wpn[wpGL].kills,
			p->ps.wpn[wpSNG].kills,
			p->ps.wpn[wpNG].kills,
			p->ps.wpn[wpSSG].kills);
}

static void onePlayerEnemyWeaponKillStats(gedict_t *p)
{
	G_sprint(self, 2, "%-20s|%5d|%5d|%5d|%5d|%5d|%5d|\n",
			va("%s%s", isghost(p) ? "\203" : "", getname(p)),
			p->ps.wpn[wpLG].ekills,
			p->ps.wpn[wpRL].ekills,
			p->ps.wpn[wpGL].ekills,
			p->ps.wpn[wpSNG].ekills,
			p->ps.wpn[wpNG].ekills,
			p->ps.wpn[wpSSG].ekills);
}

static void onePlayerDamageStats(gedict_t *p)
{
	G_sprint(self, 2, "%-20s|%5d|%5d|%5d|%5d|%5d|%5d|\n",
			va("%s%s", isghost(p) ? "\203" : "", getname(p)),
			(int)p->ps.dmg_t,
			(int)p->ps.dmg_g,
			(int)p->ps.dmg_eweapon,
			(int)p->ps.dmg_team,
			(int)p->ps.dmg_self,
			(p->deaths <= 0 ? 99999 : (int)(p->ps.dmg_t / p->deaths)));
}

static void onePlayerItemTimeStats(gedict_t *p, int tp)
{
	G_sprint(self, 2, "%-20s|%.2d:%.2d|%.2d:%.2d|%.2d:%.2d|%5s|%5s|%5s|\n",
			va("%s%s", isghost(p) ? "\203" : "", getname(p)),
			(int)(p->ps.itm[itRA].time / 60), ((int)(p->ps.itm[itRA].time)) % 60,
			(int)(p->ps.itm[itYA].time / 60), ((int)(p->ps.itm[itYA].time)) % 60,
			(int)(p->ps.itm[itGA].time / 60), ((int)(p->ps.itm[itGA].time)) % 60,
			tp ? va("%.2d:%.2d", (int)(p->ps.itm[itQUAD].time / 60),
					((int)(p->ps.itm[itQUAD].time)) % 60) :
					"    -",
			tp ? va("%.2d:%.2d", (int)(p->ps.itm[itPENT].time / 60),
					((int)(p->ps.itm[itPENT].time)) % 60) :
					"    -",
			tp ? va("%.2d:%.2d", (int)(p->ps.itm[itRING].time / 60),
					((int)(p->ps.itm[itRING].time)) % 60) :
					"    -");
}

static void onePlayerWeaponTimeStats(gedict_t *p)
{
	G_sprint(self, 2, "%-20s|%.2d:%.2d|%.2d:%.2d|%.2d:%.2d|%.2d:%.2d|%.2d:%.2d|%.2d:%.2d|\n",
			va("%s%s", isghost(p) ? "\203" : "", getname(p)),
			(int)(p->ps.wpn[wpLG].time / 60), ((int)(p->ps.wpn[wpLG].time)) % 60,
			(int)(p->ps.wpn[wpRL].time / 60), ((int)(p->ps.wpn[wpRL].time)) % 60,
			(int)(p->ps.wpn[wpGL].time / 60), ((int)(p->ps.wpn[wpGL].time)) % 60,
			(int)(p->ps.wpn[wpSNG].time / 60), ((int)(p->ps.wpn[wpSNG].time)) % 60,
			(int)(p->ps.wpn[wpNG].time / 60), ((int)(p->ps.wpn[wpNG].time)) % 60,
			(int)(p->ps.wpn[wpSSG].time / 60), ((int)(p->ps.wpn[wpSSG].time)) % 60);
}

static void onePlayerCTFStats(gedict_t *p)
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

	G_sprint(self, 2, "%-20s|%3d|%3d|%3d|%3d|%3d|%3s|%3s|%3s|%3s|\n",
			va("%s%s", isghost(p) ? "\203" : "", getname(p)),
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

static void calculateEfficiency(gedict_t *player)
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

static void playerMidairStats(void)
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

	G_sprint(self, 2, "\n%s   |%s|%s|%s|%s|%s|%s|%s|%s|%s|\n"
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
						onePlayerMidairStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

static void playerMidairKillStats(void)
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

	G_sprint(self, 2, "\n%s     |%s|%s|%s|%s|\n"
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
						onePlayerMidairKillStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

static void topMidairStats()
{
	gedict_t *p;
	float f1;
	float vh_rl;
	float a_rl;
	float ph_rl;
	float maxtopheight = 0;
	float maxtopavgheight = 0;
	float maxrlefficiency = 0;
	int from;
	int maxscore = -99999;
	int maxkills = 0;
	int maxmidairs = 0;
	int maxstomps = 0;
	int	maxstreak = 0;
	int maxspawnfrags = 0;
	int maxbonus = 0;

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

	G_sprint(self, 2, "\n%s:\n\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Top performers"));

	from = f1 = 0;
	p = find_plrghst(world, &from);
	while (p)
	{
		if (p->s.v.frags == maxscore)
		{
			G_sprint(self, 2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("score")),
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
				G_sprint(self, 2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("kills")),
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
				G_sprint(self, 2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("midairs")),
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
				G_sprint(self, 2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("head stomps")),
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
				G_sprint(self, 2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("streak")),
							(isghost(p) ? "\203" : ""), getname(p), maxstreak);
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
				G_sprint(self, 2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("spawn frags")),
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
				G_sprint(self, 2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("bonus fiend")),
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
				G_sprint(self, 2, "   %-13s: %s%s (%.1f)\n", (f1 ? "" : redtext("highest kill")),
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
				G_sprint(self, 2, "   %-13s: %s%s (%.1f)\n", (f1 ? "" : redtext("avg height")),
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
			G_sprint(self, 2, "   %-13s: %s%s (%.1f%%)\n", (f1 ? "" : redtext("rl efficiency")),
						(isghost(p) ? "\203" : ""), getname(p), maxrlefficiency);
			f1 = 1;
		}

		p = find_plrghst(p, &from);
	}

	G_sprint(self, 2, "\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n\n");
}

static void playerInstagibStats()
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

	G_sprint(self, 2, "\n%s |%s|%s|%s|%s|%s|%s|%s|%s|%s|\n"
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
						onePlayerInstagibStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

static void playerInstagibKillStats()
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

	G_sprint(self, 2, "\n%s     |%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Kill statistics"), redtext("Point"), redtext(" Frag"), redtext("Death"),
			redtext("SpwnF"), redtext(" Strk"), redtext(" CG %"));

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
						onePlayerInstagibKillStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

static void playerLGCStats()
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


	G_sprint(self, 2, "\n%s      |%s|%s|%s|%s|\n"
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
						onePlayerLGCStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

static void playersKillStats(void)
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

	G_sprint(self, 2, "\n%s     |%s|%s|%s|%s|%s|%s|%s|\n"
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
						calculateEfficiency(p2);
						onePlayerKillStats(p2, tp);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

static void playersItemStats(void)
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

	G_sprint(self, 2, "\n%s          |%s|%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Item Taken"), redtext("   GA"), redtext("   YA"), redtext("   RA"),
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
						onePlayerItemStats(p2, tp);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

static void playersWeaponEffiStats(void)
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

	G_sprint(self, 2, "\n%s   |%s|%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\237\n",
			redtext("Weapon Efficiency"), redtext("  LG%"), redtext("  RL%"), redtext("  GL%"),
			redtext(" SNG%"), redtext("  NG%"), redtext(" SSG%"), redtext("  SG%"));

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
						onePlayerWeaponEffiStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

static void playersWeaponDmgStats(void)
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

	G_sprint(self, 2, "\n%s       |%s|%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\237\n",
			redtext("Weapon Damage"), redtext("   LG"), redtext("   RL"), redtext("   GL"),
			redtext("  SNG"), redtext("   NG"), redtext("  SSG"), redtext("   SG"));

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
						onePlayerWeaponDmgStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

static void playersWeaponTakenStats(void)
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

	G_sprint(self, 2, "\n%s        |%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Weapon Taken"), redtext("   LG"), redtext("   RL"), redtext("   GL"),
			redtext("  SNG"), redtext("   NG"), redtext("  SSG"));

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
						onePlayerWeaponTakenStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

static void playersWeaponDroppedStats(void)
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

	G_sprint(self, 2, "\n%s      |%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Weapon Dropped"), redtext("   LG"), redtext("   RL"), redtext("   GL"),
			redtext("  SNG"), redtext("   NG"), redtext("  SSG"));

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
						onePlayerWeaponDroppedStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

static void playersWeaponKillStats(void)
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

	G_sprint(self, 2, "\n%s        |%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Weapon Kills"), redtext("   LG"), redtext("   RL"), redtext("   GL"),
			redtext("  SNG"), redtext("   NG"), redtext("  SSG"));

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
						onePlayerWeaponKillStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

static void playersEnemyWeaponKillStats(void)
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

	G_sprint(self, 2, "\n%s |%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Enemy Weapon Killed"), redtext("   LG"), redtext("   RL"), redtext("   GL"),
			redtext("  SNG"), redtext("   NG"), redtext("  SSG"));

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
						onePlayerEnemyWeaponKillStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

static void playersDamageStats(void)
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

	G_sprint(self, 2, "\n%s   |%s|%s|%s|%s|%s|%s|\n"
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
						onePlayerDamageStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

static void playersItemTimeStats(void)
{
	gedict_t *p;
	gedict_t *p2;
	char *tmp;
	char *tmp2;
	int from1;
	int from2;
	int tp;

	from1 = 0;
	p = find_plrghst(world, &from1);
	while (p)
	{
		p->ready = 0; // clear mark
		p = find_plrghst(p, &from1);
	}

	G_sprint(self, 2, "\n%s          |%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\237\n",
			redtext("Item times"), redtext("   RA"), redtext("   YA"), redtext("   GA"),
			redtext(" Quad"), redtext(" Pent"), redtext(" Ring"));

	tp = isTeam() || isCTF();

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
						onePlayerItemTimeStats(p2, tp);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

static void playersWeaponTimeStats(void)
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

	G_sprint(self, 2, "\n%s        |%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Weapon times"), redtext("   LG"), redtext("   RL"), redtext("   GL"),redtext("  SNG"),
			redtext("   NG"), redtext("  SSG"));

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
						onePlayerWeaponTimeStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

static void playersCTFStats(void)
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

	G_sprint(self, 2, "\n%s      |%s|%s|%s|%s|%s|%s|%s|%s|%s|\n"
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
						onePlayerCTFStats(p2);
					}
				}

				p2 = find_plrghst(p2, &from2);
			}
		}

		p = find_plrghst(p, &from1);
	}
}

static void collectTpStats(void)
{
	gedict_t *p;
	gedict_t *p2;
	int from1;
	int from2;
	int i;
	int *frags;
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

		if ((tmStatsTables_cnt < 0) || (tmStatsTables_cnt >= MAX_TM_STATS))
		{
			return; // all slots busy
		}

		if (tmStatsTables[tmStatsTables_cnt].name == NULL)
		{
			return;
		}

		for (from2 = 0, p2 = world; (p2 = find_plrghst(p2, &from2));)
		{
			if (p2->ready || strneq(tmp, getteam(p2)))
			{
				continue; // served or on different team
			}

			if (strnull(tmStatsTables[tmStatsTables_cnt].name)) // we not yet done that, do that once
			{
				strlcpy(tmStatsTables[tmStatsTables_cnt].name, tmp, MAX_TEAM_NAME);
			}

			frags = (p2->ct == ctPlayer ? &tmStatsTables[tmStatsTables_cnt].frags : &tmStatsTables[tmStatsTables_cnt].gfrags);
			frags[0] += p2->s.v.frags;
			tmStatsTables[tmStatsTables_cnt].deaths += p2->deaths;
			tmStatsTables[tmStatsTables_cnt].tkills += p2->friendly;

			tmStatsTables[tmStatsTables_cnt].dmg_t += p2->ps.dmg_t;
			tmStatsTables[tmStatsTables_cnt].dmg_g += p2->ps.dmg_g;
			tmStatsTables[tmStatsTables_cnt].dmg_team += p2->ps.dmg_team;
			tmStatsTables[tmStatsTables_cnt].dmg_eweapon += p2->ps.dmg_eweapon;

			for (i = itNONE; i < itMAX; i++)
			{
				// summ each field of items
				tmStatsTables[tmStatsTables_cnt].itm[i].tooks += p2->ps.itm[i].tooks;
				tmStatsTables[tmStatsTables_cnt].itm[i].time += p2->ps.itm[i].time;
			}

			for (i = wpNONE; i < wpMAX; i++)
			{
				// summ each field of weapons
				tmStatsTables[tmStatsTables_cnt].wpn[i].hits += p2->ps.wpn[i].hits;
				tmStatsTables[tmStatsTables_cnt].wpn[i].attacks += p2->ps.wpn[i].attacks;

				tmStatsTables[tmStatsTables_cnt].wpn[i].kills += p2->ps.wpn[i].kills;
				tmStatsTables[tmStatsTables_cnt].wpn[i].deaths += p2->ps.wpn[i].deaths;
				tmStatsTables[tmStatsTables_cnt].wpn[i].tkills += p2->ps.wpn[i].tkills;
				tmStatsTables[tmStatsTables_cnt].wpn[i].ekills += p2->ps.wpn[i].ekills;
				tmStatsTables[tmStatsTables_cnt].wpn[i].drops += p2->ps.wpn[i].drops;
				tmStatsTables[tmStatsTables_cnt].wpn[i].tooks += p2->ps.wpn[i].tooks;
				tmStatsTables[tmStatsTables_cnt].wpn[i].ttooks += p2->ps.wpn[i].ttooks;
			}

			tmStatsTables[tmStatsTables_cnt].transferred_RLpacks += p2->ps.transferred_RLpacks;
			tmStatsTables[tmStatsTables_cnt].transferred_LGpacks += p2->ps.transferred_LGpacks;

			// ctf related
			tmStatsTables[tmStatsTables_cnt].res += p2->ps.res_time;
			tmStatsTables[tmStatsTables_cnt].str += p2->ps.str_time;
			tmStatsTables[tmStatsTables_cnt].hst += p2->ps.hst_time;
			tmStatsTables[tmStatsTables_cnt].rgn += p2->ps.rgn_time;

			tmStatsTables[tmStatsTables_cnt].caps += p2->ps.caps;
			tmStatsTables[tmStatsTables_cnt].pickups += p2->ps.pickups;
			tmStatsTables[tmStatsTables_cnt].returns += p2->ps.returns;
			tmStatsTables[tmStatsTables_cnt].f_defends += p2->ps.f_defends;
			tmStatsTables[tmStatsTables_cnt].c_defends += p2->ps.c_defends;

			p2->ready = 1; // set mark
		}

		if (strnull(tmStatsTables[tmStatsTables_cnt].name))
		{
			continue; // wtf, empty team?
		}

		if (isCTF() && g_globalvars.time - match_start_time > 0)
		{
			tmStatsTables[tmStatsTables_cnt].res = (tmStatsTables[tmStatsTables_cnt].res
					/ (g_globalvars.time - match_start_time)) * 100;
			tmStatsTables[tmStatsTables_cnt].str = (tmStatsTables[tmStatsTables_cnt].str
					/ (g_globalvars.time - match_start_time)) * 100;
			tmStatsTables[tmStatsTables_cnt].hst = (tmStatsTables[tmStatsTables_cnt].hst
					/ (g_globalvars.time - match_start_time)) * 100;
			tmStatsTables[tmStatsTables_cnt].rgn = (tmStatsTables[tmStatsTables_cnt].rgn
					/ (g_globalvars.time - match_start_time)) * 100;
		}

		tmStatsTables_cnt++;
	}
}

static void summaryTPStats(void)
{
	int i;

	G_sprint(self, 2, "\n%s    |%s|%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\237\n",
			redtext("Team Items Taken"), redtext("RA"), redtext("YA"), redtext("GA"),
			redtext("MH"), redtext(" Q"), redtext(" P"), redtext(" R"));
	for (i = 0; i < min(tmStatsTables_cnt, MAX_TM_STATS); i++)
	{
		G_sprint(self, 2, "%-20s|%2d|%2d|%2d|%2d|%2d|%2d|%2d|\n",
				va("\220%s\221", tmStatsTables[i].name),
				tmStatsTables[i].itm[itRA].tooks,
				tmStatsTables[i].itm[itYA].tooks,
				tmStatsTables[i].itm[itGA].tooks,
				tmStatsTables[i].itm[itHEALTH_100].tooks,
				tmStatsTables[i].itm[itQUAD].tooks,
				tmStatsTables[i].itm[itPENT].tooks,
				tmStatsTables[i].itm[itRING].tooks);
	}

	G_sprint(self, 2, "\n%s        |%s|%s|%s|%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\237\n",
			redtext("Team Weapons"), redtext("LGT"), redtext("LGK"), redtext("LGD"),
			redtext("LGX"), redtext("RLT"), redtext("RLK"), redtext("RLD"), redtext("RLX"));
	for (i = 0; i < min(tmStatsTables_cnt, MAX_TM_STATS); i++)
	{
		G_sprint(self, 2, "%-20s|%3d|%3d|%3d|%3d|%3d|%3d|%3d|%3d|\n",
				va("\220%s\221", tmStatsTables[i].name),
				tmStatsTables[i].wpn[wpLG].tooks,
				tmStatsTables[i].wpn[wpLG].ekills,
				tmStatsTables[i].wpn[wpLG].drops,
				tmStatsTables[i].transferred_LGpacks,
				tmStatsTables[i].wpn[wpRL].tooks,
				tmStatsTables[i].wpn[wpRL].ekills,
				tmStatsTables[i].wpn[wpRL].drops,
				tmStatsTables[i].transferred_RLpacks);
	}

	G_sprint(self, 2, "\n%s   |%s|%s|%s|%s|%s|\n"
			"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\237\n",
			redtext("Team Frags+Damage"), redtext("Frags"), redtext("Given"), redtext("Taken"),
			redtext(" EWep"), redtext(" Team"));
	for (i = 0; i < min(tmStatsTables_cnt, MAX_TM_STATS); i++)
	{
		G_sprint(self, 2, "%-20s|%5d|%5d|%5d|%5d|%5d|\n",
				va("\220%s\221", tmStatsTables[i].name),
				tmStatsTables[i].frags,
				(int)tmStatsTables[i].dmg_g,
				(int)tmStatsTables[i].dmg_t,
				(int)tmStatsTables[i].dmg_eweapon,
				(int)tmStatsTables[i].dmg_team);
	}

	if (isCTF())
	{
		G_sprint(self, 2, "\n%s |%s|%s|%s|%s|%s|%s|%s|%s|%s|\n"
				"\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
				"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
				redtext("Team CTF statistics"), redtext("Fla"), redtext("Cap"), redtext("Ret"),
				redtext("DFl"), redtext("DCa"), redtext("Res"), redtext("Str"), redtext("Hst"),
				redtext("Rgn"));
		for (i = 0; i < min(tmStatsTables_cnt, MAX_TM_STATS); i++)
		{
			G_sprint(self, 2, "%-20s|%3d|%3d|%3d|%3d|%3d|%3s|%3s|%3s|%3s|\n",
					va("\220%s\221", tmStatsTables[i].name),
					tmStatsTables[i].pickups,
					tmStatsTables[i].caps,
					tmStatsTables[i].returns,
					tmStatsTables[i].f_defends,
					tmStatsTables[i].c_defends,
					((cvar("k_ctf_runes") ? va("%d%%", (int)tmStatsTables[i].res) : " - ")),
					((cvar("k_ctf_runes") ? va("%d%%", (int)tmStatsTables[i].str) : " - ")),
					((cvar("k_ctf_runes") ? va("%d%%", (int)tmStatsTables[i].hst) : " - ")),
					((cvar("k_ctf_runes") ? va("%d%%", (int)tmStatsTables[i].rgn) : " - ")));
		}
	}
}
static void topStats(void)
{
	gedict_t *p;
	float f1;
	int from;
	float maxfrags = -99999;
	float maxdeaths = -1;
	float maxfriend = -1;
	float maxeffi = -1;
	float maxspree = -1;
	float maxspree_q = -1;
	float maxdmgg = -1;
	float maxdmgtd = -1;
	int maxspawnfrags = 0;
	int maxcaps = 0;
	int maxdefends = 0;

	from = f1 = 0;
	p = find_plrghst(world, &from);
	while (p)
	{
		maxfrags = max((isCTF() ? p->s.v.frags - p->ps.ctf_points : p->s.v.frags), maxfrags);
		maxdeaths = max(p->deaths, maxdeaths);
		maxfriend = max(p->friendly, maxfriend);
		maxeffi = max(p->efficiency, maxeffi);
		maxspree = max(p->ps.spree_max, maxspree);
		maxspree_q = max(p->ps.spree_max_q, maxspree_q);
		maxdmgg = max(p->ps.dmg_g, maxdmgg);
		maxdmgtd = max((int)(p->ps.dmg_t / p->deaths), maxdmgtd);
		maxspawnfrags = max(p->ps.spawn_frags, maxspawnfrags);
		maxcaps = max(p->ps.caps, maxcaps);
		maxdefends = max(p->ps.f_defends, maxdefends);

		p = find_plrghst(p, &from);
	}

	G_sprint(self, 2, "\n%s:\n\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
			"\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n",
				redtext("Top performers"));

	from = f1 = 0;
	p = find_plrghst(world, &from);
	while (p)
	{
		if ((!isCTF() && (p->s.v.frags == maxfrags))
				|| (isCTF() && ((p->s.v.frags - p->ps.ctf_points) == maxfrags)))
		{
			G_sprint(self, 2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("Frags")),
						(isghost(p) ? "\203" : ""), getname(p), (int)maxfrags);
			f1 = 1;
		}

		p = find_plrghst(p, &from);
	}

	if (maxdeaths != -1)
	{
		from = f1 = 0;
		p = find_plrghst(world, &from);
		while (p)
		{
			if (p->deaths == maxdeaths)
			{
				G_sprint(self, 2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("Deaths")),
							(isghost(p) ? "\203" : ""), getname(p), (int)maxdeaths);
				f1 = 1;
			}

			p = find_plrghst(p, &from);
		}
	}

	if (maxfriend > 0)
	{
		from = f1 = 0;
		p = find_plrghst(world, &from);
		while (p)
		{
			if (p->friendly == maxfriend)
			{
				G_sprint(self, 2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("Friendkills")),
							(isghost(p) ? "\203" : ""), getname(p), (int)maxfriend);
				f1 = 1;
			}

			p = find_plrghst(p, &from);
		}
	}

	if (maxeffi != -1)
	{
		from = f1 = 0;
		p = find_plrghst(world, &from);
		while (p)
		{
			if (p->efficiency == maxeffi)
			{
				G_sprint(self, 2, "   %-13s: %s%s (%d%%)\n", (f1 ? "" : redtext("Efficiency")),
							(isghost(p) ? "\203" : ""), getname(p), (int)maxeffi);
				f1 = 1;
			}

			p = find_plrghst(p, &from);
		}
	}

	if (maxspree != -1)
	{
		from = f1 = 0;
		p = find_plrghst(world, &from);
		while (p)
		{
			if (p->ps.spree_max == maxspree)
			{
				G_sprint(self, 2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("Fragstreak")),
							(isghost(p) ? "\203" : ""), getname(p), (int)maxspree);
				f1 = 1;
			}

			p = find_plrghst(p, &from);
		}
	}

	if (maxspree_q > 0)
	{
		from = f1 = 0;
		p = find_plrghst(world, &from);
		while (p)
		{
			if (p->ps.spree_max_q == maxspree_q)
			{
				G_sprint(self, 2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("Quadrun")),
							(isghost(p) ? "\203" : ""), getname(p), (int)maxspree_q);
				f1 = 1;
			}

			p = find_plrghst(p, &from);
		}
	}

	if (maxdmgg != -1)
	{
		from = f1 = 0;
		p = find_plrghst(world, &from);
		while (p)
		{
			if (p->ps.dmg_g == maxdmgg)
			{
				G_sprint(self, 2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("Annihilator")),
							(isghost(p) ? "\203" : ""), getname(p), (int)maxdmgg);
				f1 = 1;
			}

			p = find_plrghst(p, &from);
		}
	}

	if (maxdmgtd != -1)
	{
		from = f1 = 0;
		p = find_plrghst(world, &from);
		while (p)
		{
			if ((p->deaths <= 0 ? 99999 : p->ps.dmg_t / p->deaths) == maxdmgtd)
			{
				G_sprint(self, 2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("Survivor")),
							(isghost(p) ? "\203" : ""), getname(p), (int)maxdmgtd);
				f1 = 1;
			}

			p = find_plrghst(p, &from);
		}
	}

	if (maxspawnfrags > 0)
	{
		from = f1 = 0;
		p = find_plrghst(world, &from);
		while (p)
		{
			if (p->ps.spawn_frags == maxspawnfrags)
			{
				G_sprint(self, 2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("Spawnfrags")),
							(isghost(p) ? "\203" : ""), getname(p), maxspawnfrags);
				f1 = 1;
			}

			p = find_plrghst(p, &from);
		}
	}

	if (isCTF())
	{
		if (maxcaps != -1)
		{
			from = f1 = 0;
			p = find_plrghst(world, &from);
			while (p)
			{
				if (p->ps.caps == maxcaps)
				{
					G_sprint(self, 2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("Captures")),
								(isghost(p) ? "\203" : ""), getname(p), maxcaps);
					f1 = 1;
				}

				p = find_plrghst(p, &from);
			}
		}

		if (maxdefends != -1)
		{
			from = f1 = 0;
			p = find_plrghst(world, &from);
			while (p)
			{
				if (p->ps.f_defends == maxdefends)
				{
					G_sprint(self, 2, "   %-13s: %s%s (%d)\n", (f1 ? "" : redtext("FlagDefends")),
								(isghost(p) ? "\203" : ""), getname(p), maxdefends);
					f1 = 1;
				}

				p = find_plrghst(p, &from);
			}
		}
	}
}
