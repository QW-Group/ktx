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

qbool isHoonyMode()
	{
	return (isDuel() && cvar("k_hoonymode"));
	}

char extra1[99] = {0};
char extra2[99] = {0};
char extra[199] = {0};

void HM_next_point(gedict_t *won, gedict_t *lost) // won is optional, lost is not
	{
	// note: i need won/lost because depending on s.v.frags is unreliable here...
	gedict_t *p;
	char *p_extra;
	int i = 0;
	static int id1 = -99, id2 = -99, max_len = 0;
	int f = 0;

	if (won == 0) for (p = world; (p = find_plr(p));) if (p != lost) won = p;

	if (id1 == -99)
		id1 = GetUserID(won);
	else if (id2 == -99)
		id2 = GetUserID(lost);

	for (p = world; (p = find_plr(p));)
		{
		if (id1 == GetUserID(p))
			p_extra = extra1;
		else
			p_extra = extra2;

		if (HM_current_point() == 1)
			{
			max_len = max(strlen(won->s.v.netname), strlen(lost->s.v.netname));
			for (i = 0; i < max_len + 2; ++i) // I forgot how to do this using std functions lol --phil
				{
				p_extra[i] = (i < strlen(p->s.v.netname) ? p->s.v.netname[i] : (i == strlen(p->s.v.netname) ? ':' : ' '));
				}
			p_extra[i] = 0;
			}

		for (i = max_len + 2; i < strlen(p_extra); i += 2)
			f += (p_extra[i] == '0' ? 0 : 1);

		p_extra[i] = (p == won ? '1' : '0');
		p_extra[i+1] = ' ';
		p_extra[i+2] = 0;
		}
	EndMatch(0);
	}

#define HM_MIN_POINTS 6
#define HM_WINNING_DIFF 2

int HM_current_point()
	{
	gedict_t *p;
	int i = 0;
	for (p = world; (p = find_plr(p));) i += p->s.v.frags;
	return i;
	}

/*
static struct
	{
	float lg;
	int dh;
	int hp;
	}
hm_stat[2][99]; */ // save myself some code

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
		if (previous_point % HM_PTS_PER_STAT_LINE == 0) strlcat(vs_blurb, va("%s%s", p->s.v.netname, i == 0 ? " - " : "\n"), sizeof(vs_blurb));
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
		if (previous_point == 0) hm_stat_lines[0][0] = hm_stat_lines[1][0] = hm_stat_lines[2][0] = 0;
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
	int s1 = -999, s2 = -999;

	for (p = world; (p = find_plr(p));)
		if (s1 == -999) s1 = p->s.v.frags; else s2 = p->s.v.frags;

	if ((s1 >= HM_MIN_POINTS || s2 >= HM_MIN_POINTS) && abs(s1-s2) >= HM_WINNING_DIFF)
		return HM_PT_FINAL; // as used in the code (match.c) HM_PT_FINAL == "last point was final"

	if ((s1 >= HM_MIN_POINTS - 1 || s2 >= HM_MIN_POINTS - 1) && abs(s1-s2) >= HM_WINNING_DIFF - 1)
		return HM_PT_SET; // as used in the code, HM_PT_SET == "this point is set point"

	return 0;
	}

void show_powerups(char *);
void hide_powerups(char *);

void HM_all_ready()
	{
	gedict_t *p;

	// make sure stuff is spawned for every point
	show_powerups("item_shells");
	show_powerups("item_spikes");
	show_powerups("item_rockets");
	show_powerups("item_cells");
	show_powerups("item_health");
	show_powerups("item_armor1");
	show_powerups("item_armor2");
	show_powerups("item_armorInv");
	hide_powerups("item_artifact_invulnerability");
	hide_powerups("item_artifact_super_damage");
	hide_powerups("item_artifact_envirosuit");
	hide_powerups("item_artifact_invisibility");

	// get rid of any backpacks too ... is there a better find than nextent() here?
	for (p = world; (p = nextent(p));) if (streq(p->s.v.classname, "backpack"))
		ent_remove(p);

	if (HM_current_point_type() == HM_PT_SET)
		G_bprint(2, "Set point!\n");
	}

// note: this will probably break if a player drops and rejoins..
void HM_rig_the_spawns(int mode, gedict_t *spot)
	{
	static int enabled = 0;
	static gedict_t *p;

	static gedict_t spot1, spot2;
	static int id1 = -99, id2 = -99;

	if (mode < 2) // we don't want to rig every spawn, so first call it with (1, null) to turn on spawn rigging (don't forget (0, null) to turn off)
		{
		enabled = mode;
		return;
		}

	// then we call it with (2, spot) to spawn on the other guy's last spawn (only if it is enabled, of course)
	if (enabled)
		{
		if (HM_current_point() % 2 == 1)
			{
			for (p = world; (p = find_plr(p));) if (p == self)
				{
				if (id1 == GetUserID(p))
					*spot = spot2;
				else
					*spot = spot1;
				}
			// this should be all you need
			/*for (p = world; (p = find_plr(p));)
				{
				if (GetUserID(p) != GetUserID(self))
					{
					spot = p->wizard;
					}
				}*/
			}
		else
			{
			for (p = world; (p = find_plr(p));) if (p == self)
				{
				if (id1 == -99)
					id1 = GetUserID(p);
				else if (id2 == -99)
					id2 = GetUserID(p);

				if (id1 == GetUserID(p))
					spot1 = *spot;
				else
					spot2 = *spot;
				}
			// this should be all you need
			//self->wizard = spot; // ->wizard should really be ->k_hoonyspawn, but when I add it in progs.h, stuff breaks randomly? :X
			}
		}
	}


char *HM_lastscores_extra ()
	{
	extra[0] = 0;
	strlcat(extra, va("\n%s\n%s", extra1, extra2), sizeof(extra));
	return extra;
	}
