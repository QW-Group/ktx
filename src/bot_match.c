/*
 match.qc

 Copyright (C) 1997-1999 Robert 'Frog' Field
 Copyright (C) 2000-2007 ParboiL
 */

// bot_match.c
#ifdef BOT_SUPPORT

#include "g_local.h"

void button_use(void);
void fd_secret_use(gedict_t *attacked, float take);
void door_use(void);
void door_killed(void);
void multi_use(void);

// Can also be called via "botcmd debug startmap"
void BotsFireInitialTriggers(gedict_t *player)
{
	gedict_t *old_self = self;
	extern gedict_t *markers[];
	int i;

	other = activator = player;
	for (i = 0; i < NUMBER_MARKERS; ++i)
	{
		gedict_t *ent = markers[i];
		if (!ent)
		{
			break;
		}

		if (!(ent->fb.T & MARKER_FIRE_ON_MATCH_START))
		{
			continue;
		}

		if (ent && ent->fb.door_entity)
		{
			ent = ent->fb.door_entity;
		}

		if (streq(ent->classname, "func_button"))
		{
			self = ent;
			button_use();
		}
		else if (streq(ent->classname, "trigger_once"))
		{
			self = ent;
			multi_use();
		}
		else if (streq(ent->classname, "door"))
		{
			self = ent;
			if (ent->s.v.takedamage)
			{
				((void (*)(void))(ent->th_pain))();
			}
			else if (ent->use)
			{
				((void (*)(void))(ent->use))();
			}
		}
	}

	self = old_self;
}

// Assign bitmask to each player to indicate what team they are in
//   Called at match start and also when a client connects
void BotsAssignTeamFlags(void)
{
	gedict_t *p, *p2;
	int teamflag = 1;
	char *s = "";

	if (!teamplay)
	{
		return;
	}

	// Clear teamflag from all items
	for (p = world; (p = nextent(p));)
	{
		p->fb.teamflag = 0;
	}

	for (p = world; (p = find_plr(p));)
	{
		p->k_flag = 0;
	}

	for (p = world; (p = find_plr(p));)
	{
		if (p->k_flag || strnull(s = getteam(p)))
		{
			continue;
		}

		p->k_flag = 1;
		p->fb.teamflag = teamflag;
		for (p2 = p; (p2 = find_plr(p2));)
		{
			if (streq(s, getteam(p2)))
			{
				p2->k_flag = 1;
				p2->fb.teamflag = teamflag;
			}
		}

		teamflag <<= 1;
	}
}

gedict_t* BotsFirstBot(void)
{
	gedict_t *first_bot = NULL;
	gedict_t *ent;

	for (ent = world; (ent = find_plr(ent));)
	{
		if (ent->isBot)
		{
			first_bot = ent;
			break;
		}
	}

	return first_bot;
}

// Called by KTX as match begins
void BotsMatchStart(void)
{
	gedict_t *first_bot = BotsFirstBot();
	gedict_t *ent;

	for (ent = world; (ent = find(ent, FOFCLSN, "marker_indicator"));)
	{
		ent_remove(ent);
	}

	for (ent = world; (ent = find(ent, FOFCLSN, "marker"));)
	{
		setmodel(ent, "");
	}

	// No bots => perform no action, standard human match
	if (first_bot == NULL)
	{
		return;
	}

	BotsAssignTeamFlags();
	BotsFireInitialTriggers(first_bot);
}

#endif
