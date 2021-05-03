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

extern qbool isSupport_Params(gedict_t *p);

extern void info_sys_mm_update(gedict_t *p, char *from, char *to);

//========================================================
//
// ktpro like 'cmd info' compatibility
//
//========================================================

#define isSysKey( key ) ( !strnull(key) && *(key) == '*' )

typedef struct cmdinfo_s
{
	char *key;
	void (*f)(gedict_t *p, char *from, char *to);
} cmdinfo_t;

cmdinfo_t cinfos[] =
{
//	{ "*is", 0 },		// keys with * is for internal use ONLY
	{ "*mm", info_sys_mm_update },	// for mmode
//	{ "*ml", 0 },					// for mmode last
//	{ "*mp", 0 },					// for mmode player
//	{ "*mt", 0 },					// for mmode team
//	{ "*mu", 0 },					// for mmode multi
//	{ "*at", 0 },					// autotrack

	{ "mi", info_mi_update },		// spec moreinfo
//    { "b_switch", 0 },

//    { "e-mail", 0 },
	{ "ev", info_ev_update },

//    { "fs", 0 }, // for force_spec 

//	{ "gender", 0 },

//    { "k", 0 },
//    { "k_nick", 0 },

	{ "kf", info_kf_update },

//	{ "ln", 0 },
//    { "ls", 0 },
//	{ "lw", 0 },
//	{ "lw_x", 0 },
//	{ "ktpl", 0 }, // zzz, so "ln" "ls" "lw" keys will work like in ktpro
//	{ "postmsg", 0 },
//	{ "premsg", 0 },

//	{ "ti", 0 },	// specifie use team info or not
//  { "w_switch", 0 },
//	{ "k_sdir", 0 },
//	{ "wps", 0 },
//	{ "lra", 0 },		// ra status bar modificator
//	{ "pbspeed", 0 },	// for /trx_play
//	{ "runes", 0 }	// for stuffing "set rune xxx" in CTF
};

int cinfos_cnt = sizeof(cinfos) / sizeof(cinfos[0]);

void cmdinfo()
{
	int argc = trap_CmdArgc();
	char arg_1[128], arg_2[128];

	if (trap_CmdArgc() < 1)
	{
		return; // something wrong
	}

	trap_CmdArgv(1, arg_1, sizeof(arg_1));
	trap_CmdArgv(2, arg_2, sizeof(arg_2));

	if ((argc == 1) || (argc > 3))
	{
		// just show info about all keys
		stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "cmd setinfo\n");

		return;
	}

	if (argc == 2)
	{
		// show info about particular key
		G_sprint(self, 2, "key %s = \"%s\"\n", arg_1, ezinfokey(self, arg_1));

		return;
	}

	if (argc == 3)
	{
		// set/remove particular key
		SetUserInfo(self, arg_1, arg_2, 0);

		return;
	}
}

void cmduinfo()
{
	gedict_t *p;
	int argc = trap_CmdArgc();
	char *v;
	char arg_1[128], arg_2[128];

	if (trap_CmdArgc() < 1)
	{
		return; // something wrong
	}

	if ((argc == 1) || (argc > 3))
	{
		// just show info about usage
		if (isSupport_Params(self))
		{
			G_sprint(self, 2, "usage: kuinfo <id/name> [key]\n");
		}
		else
		{
			G_sprint(self, 2, "usage: cmd uinfo <id/name> [key]\n");
		}

		return;
	}

	if (argc == 2)
	{
		// show info about keys of someone
		int i;

		trap_CmdArgv(1, arg_1, sizeof(arg_1));

		if (!(p = SpecPlayer_by_IDorName(arg_1)))
		{
			G_sprint(self, 2, "client \"%s\" not found\n", arg_1);

			return;
		}

		G_sprint(self, 2, "%s's personal keys:\n", p->netname);

		for (i = 0; i < cinfos_cnt; i++)
		{
			if (isSysKey(cinfos[i].key))
			{
				continue; // sys keys is not showed for mortals %)
			}

			v = ezinfokey(p, cinfos[i].key);

			if (!strnull(v)) // show not empty keys only
			{
				G_sprint(self, 2, "key %s = \"%s\"\n", cinfos[i].key, v);
			}
		}

		return;
	}

	if (argc == 3)
	{
		// show someone particular key
		trap_CmdArgv(1, arg_1, sizeof(arg_1));
		trap_CmdArgv(2, arg_2, sizeof(arg_2));

		if (!(p = SpecPlayer_by_IDorName(arg_1)))
		{
			G_sprint(self, 2, "client \"%s\" not found\n", arg_1);

			return;
		}

		if (isSysKey(arg_2))
		{
			v = NULL; // sys keys is not showed for mortals %)
		}
		else
		{
			v = ezinfokey(p, arg_2);
		}

		if (!v)
		{
			G_sprint(self, 2, "key \"%s\" is hidden\n", arg_2);
		}
		else
		{
			G_sprint(self, 2, "%s's %s = \"%s\"\n", p->netname, arg_2, v);
		}

		return;
	}
}

// this issued on each client connect.
// stuff/call "infoset" alias for player and "sinfoset" for spec
void cmdinfo_infoset(gedict_t *p)
{
	if (strnull(ezinfokey(p, "*is")))
	{
		SetUserInfo(p, "*is", "1", SETUSERINFO_STAR); // mark we are call infoset already
		stuffcmd_flags(p, STUFFCMD_IGNOREINDEMO, "%sinfoset\n", p->ct == ctSpec ? "s" : ""); // and call infoset
		stuffcmd_flags(p, STUFFCMD_IGNOREINDEMO, "ktx_%sinfoset\n", p->ct == ctSpec ? "s" : ""); // and call ktx_infoset
		// kick cmd back to server, so we know client get infoset,
		// so we can invoke on_connect and on_enter
		stuffcmd_flags(p, STUFFCMD_IGNOREINDEMO, "wait;wait;wait;cmd ack infoset\n");
	}
	else
	{
		// kick cmd back to server, so we know client already get infoset,
		// so we can invoke on_enter
		stuffcmd_flags(p, STUFFCMD_IGNOREINDEMO, "wait;wait;wait;cmd ack noinfoset\n");
	}
}

//===================================================
//
// native setinfo
//
//===================================================

qbool FixPlayerTeam(char *newteam);
qbool FixPlayerColor(char *newcolor);

qbool ClientUserInfoChanged()
{
	char arg_0[1024], arg_1[1024], arg_2[1024], *old;
	int i;

	if (trap_CmdArgc() != 3)
	{
		return false; // something wrong
	}

	trap_CmdArgv(0, arg_0, sizeof(arg_0));
	trap_CmdArgv(1, arg_1, sizeof(arg_1));
	trap_CmdArgv(2, arg_2, sizeof(arg_2));

//	G_bprint(2, "'%s' '%s' '%s'\n", arg_0, arg_1, arg_2 );

	if (streq("team", arg_1))
	{
		return FixPlayerTeam(arg_2);
	}

	if (streq("rate", arg_1))
	{
		return CheckRate(self, arg_2);
	}

	if (streq("bottomcolor", arg_1))
	{
		return FixPlayerColor(arg_2);
	}

	// call assigned function if any for this key

	old = ezinfokey(self, arg_1);

	for (i = 0; i < cinfos_cnt; i++)
	{
		if (streq(cinfos[i].key, arg_1))
		{
			if (cinfos[i].f)  // call some handler, if set
			{
				(cinfos[i].f)(self, old, arg_2);
			}

			break;
		}
	}

	return false;
}

//===================================================
//
// different handlers
//
//===================================================

// in ctf we dont want red team players to be blue, etc
qbool FixPlayerColor(char *newcolor)
{
	if (self->ct == ctSpec)
	{
		return false;
	}

	if (isCTF())
	{
		if (streq(getteam(self), "red"))
		{
			stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "color %d %d\n",
							iKey(self, "topcolor") == 13 ? 4 : iKey(self, "topcolor"), 4);
		}
		else if (streq(getteam(self), "blue"))
		{
			stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "color %d %d\n",
							iKey(self, "topcolor") == 4 ? 13 : iKey(self, "topcolor"), 13);
		}
	}

	return false;
}

// check if player tried to change team and this is allowed
qbool FixPlayerTeam(char *newteam)
{
	char *s1, *s2;

	if (self->ct == ctSpec)
	{
		// coach or potential coach may not change team
		if (coach_num(self) || is_elected(self, etCoach))
		{
			if (strneq(getteam(self), newteam))
			{
				G_sprint(self, 2, "You may %s change team\n", redtext("not"));
				stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "team \"%s\"\n", getteam(self)); // sends this to client - so he get right team too
			}

			return true;
		}

		// do we need this at all?
		// it is here because the coach stuff was copied from captain
		if (k_coaches == 2)
		{
			// get the strings to compare
			s1 = newteam;

			if (self->k_picked == 1)
			{
				s2 = cvar_string("_k_coachteam1");
			}
			else if (self->k_picked == 2)
			{
				s2 = cvar_string("_k_coachteam2");
			}
			else
			{
				s2 = "";
			}

			if (strneq(s1, s2))
			{
				G_sprint(self, 2, "You may %s change team\n", redtext("not"));
				stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "team \"%s\"\n", s2); // sends this to client - so he get right team too

				return true;
			}
		}

		return false;
	}

	// do not allow change team in game / countdown
	if (match_in_progress || coop)
	{
		s1 = newteam;
		s2 = getteam(self);

		if (strneq(s1, s2))
		{
			G_sprint(self, 2, "You may %s change team during game\n", redtext("not"));
			stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "team \"%s\"\n", s2); // sends this to client - so he get right team too
		}

		return true;
	}

	// captain or potential captain may not change team
	if (capt_num(self) || is_elected(self, etCaptain))
	{
		if (strneq(getteam(self), newteam))
		{
			G_sprint(self, 2, "You may %s change team\n", redtext("not"));
			stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "team \"%s\"\n", getteam(self)); // sends this to client - so he get right team too
		}

		return true;
	}

	if (k_captains == 2)
	{
		// get the strings to compare
		s1 = newteam;

		if (self->k_picked == 1)
		{
			s2 = cvar_string("_k_captteam1");
		}
		else if (self->k_picked == 2)
		{
			s2 = cvar_string("_k_captteam2");
		}
		else
		{
			s2 = "";
		}

		if (strneq(s1, s2))
		{
			G_sprint(self, 2, "You may %s change team\n", redtext("not"));
			stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "team \"%s\"\n", s2); // sends this to client - so he get right team too

			return true;
		}
	}

	if (!match_in_progress && isCTF() && self->ready)
	{
		// if CTF and player ready allow change team to red or blue
		s1 = newteam;
		s2 = getteam(self);

		if (strneq(s1, "red") && strneq(s1, "blue"))
		{
			G_sprint(self, 2, "You must be on team red or blue for CTF\n");
			stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "team \"%s\"\n", s2); // sends this to client - so he get right team too

			return true;
		}

		stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "color %d\n", streq(s1, "red") ? 4 : 13);
	}

	if (!match_in_progress && (isTeam() || isCTF()) && self->ready && strnull(newteam))
	{
		// do not allow empty team, because it cause problems
		G_sprint(self, 2, "Empty %s not allowed\n", redtext("team"));
		stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "team \"%s\"\n", getteam(self)); // sends this to client - so he get right team too

		return true;
	}

	if (isCTF() && (streq(newteam, "red") || streq(newteam, "blue")))
	{
		stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "auto%s\n", newteam);
	}

	return false;
}
