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

// motd.c
#include "g_local.h"

void PMOTDThink(void)
{
	int i;
	char buf[2048] =
		{ 0 };
	char *s;

	// remove MOTD in some cases
	if ((self->attack_finished < g_globalvars.time) // expired
			|| (!k_matchLess && match_in_progress)  // non matchless and (match has began or countdown)
			|| (k_matchLess && match_in_progress == 1) // matchless and countdown
			|| (PROG_TO_EDICT(self->s.v.owner)->attack_finished > g_globalvars.time)) // player fire something, so he wanna play, not reading motd
	{
		if (self->attack_finished < g_globalvars.time)
		{
			G_centerprint(PROG_TO_EDICT(self->s.v.owner), "%s", "");
		}

		ent_remove(self);

		return;
	}

	if (PROG_TO_EDICT(self->s.v.owner)->wp_stats || PROG_TO_EDICT(self->s.v.owner)->sc_stats
			|| PROG_TO_EDICT(self->s.v.owner)->shownick_time)
	{
		self->s.v.nextthink = g_globalvars.time + 1; // do not interference with +wp_stats or +scores and shownick

		return;
	}

	for (i = 1; i <= MOTD_LINES; i++)
	{
		if (strnull(s = cvar_string(va("k_motd%d", i))))
		{
			continue;
		}

		strlcat(buf, s, sizeof(buf));
		strlcat(buf, "\n", sizeof(buf));
	}

	// no "welcome" - if k_motd keys is present - because admin may wanna customize this
	if (strnull(buf))
	{
		strlcat(buf, "Welcome\n\n", sizeof(buf));
	}

	strlcat(buf, "\n\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237\n\n",
			sizeof(buf));
	strlcat(buf,
			va("Running %s %s", redtext(cvar_string("qwm_name")),
				redtext(cvar_string("qwm_version"))),
			sizeof(buf));
	if (strlen(cvar_string("qws_name")) && strlen(cvar_string("qws_version")))
	{
		strlcat(buf,
				va(" on %s %s", redtext(cvar_string("qws_name")),
					redtext(cvar_string("qws_version"))),
				sizeof(buf));
	}

	strlcat(buf,
			va("\n\nType \"%s\" for available commands\nType \"%s\" for server details",
				redtext("commands"), redtext("about")),
			sizeof(buf));

	G_centerprint(PROG_TO_EDICT(self->s.v.owner), "%s", buf);

	self->s.v.nextthink = g_globalvars.time + 0.7;
}

void SMOTDThink(void)
{
	PMOTDThink(); // equal motd for player and spectator now
}

void MOTDThinkX(void)
{
	gedict_t *owner = PROG_TO_EDICT(self->s.v.owner);

	// FIXME: server work around, frags are not restored, ie showed as 0, force update frags manually
	if (owner->s.v.frags && ((int)(owner - world - 1) >= 0)
			&& ((int)(owner - world - 1) < MAX_CLIENTS))
	{
		WriteByte(MSG_ALL, SVC_UPDATEFRAGS); // update frags
		WriteByte(MSG_ALL, (int)(owner - world - 1));
		WriteShort(MSG_ALL, owner->s.v.frags);
	}

	// select MOTD for spectator or player
	self->think = (func_t)(owner->ct == ctSpec ? SMOTDThink : PMOTDThink);
	self->s.v.nextthink = g_globalvars.time + 0.3;

	if (owner->k_stuff)
	{
		if (k_matchLess) // remove motd if player already stuffed, because them probably sow motd already one time
		{
			ent_remove(self);
		}
	}

	// stuff or not to stuff, that the question!
	if (!(owner->k_stuff & STUFF_MAPS))
	{
		StuffMaps(owner);
	}
	else if (!(owner->k_stuff & STUFF_COMMANDS))
	{
		StuffModCommands(owner);
	}
}

void MakeMOTD(void)
{
	gedict_t *motd;
	int i = bound(0, cvar("k_motd_time"), 30);

	motd = spawn();
	motd->classname = "motd";
	motd->s.v.owner = EDICT_TO_PROG(self);
	motd->think = (func_t) MOTDThinkX;
	motd->s.v.nextthink = g_globalvars.time + 0.1;
	motd->attack_finished = g_globalvars.time + (i ? i : (k_matchLess ? 3 : 7));
}

void RemoveMOTD(void)
{
	gedict_t *motd;
	int owner = EDICT_TO_PROG(self);

	for (motd = world; (motd = find(motd, FOFCLSN, "motd"));) // self MOTD
	{
		if (owner == motd->s.v.owner)
		{
			ent_remove(motd);
		}
	}
}
