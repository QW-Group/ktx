//========================================================================
//
//  Copyright (C)  2020 - 2021	Samuel Piper
//
//  This code is free software; you can redistribute it and/or modify
//  it under the terms of the GNU GPL (General Public License); either
//  version 2 of the License, or (at your option) any later version.
//
//========================================================================
#include "g_local.h"

int ANTILAG_MEMPOOL_WORLDSEEK;
antilag_t ANTILAG_MEMPOOL[ANTILAG_MAXEDICTS];
antilag_t *antilag_list_players;
antilag_t *antilag_list_world;
float antilag_nextthink_world;
vec3_t antilag_origin;
float time_corrected;

static qbool antilag_current_rewind(gedict_t *e, float *ms)
{
	char *rewind = ezinfokey(e, "antilag_rewind");

	if (strnull(rewind))
	{
		return false;
	}

	*ms = atof(rewind) / 1000;
	return true;
}

static float antilag_current_ping(gedict_t *e)
{
	char *ping = ezinfokey(e, "ping_current");

	if (!strnull(ping))
	{
		return atof(ping) / 1000;
	}

	return atof(ezinfokey(e, "ping")) / 1000;
}

void antilag_log(gedict_t *e, antilag_t *antilag)
{
	// stop extremely fast logging
	if (g_globalvars.time - antilag->rewind_time[antilag->rewind_seek] < 0.01)
	{
		return;
	}

	antilag->rewind_seek++;

	if (antilag->rewind_seek >= ANTILAG_MAXSTATES)
	{
		antilag->rewind_seek = 0;
	}

	VectorCopy(e->s.v.origin, antilag->rewind_origin[antilag->rewind_seek]);
	VectorCopy(e->s.v.velocity, antilag->rewind_velocity[antilag->rewind_seek]);
	antilag->rewind_time[antilag->rewind_seek] = g_globalvars.time;

	if ((int)e->s.v.flags & FL_ONGROUND)
	{
		antilag->rewind_platform_edict[antilag->rewind_seek] = e->s.v.groundentity;
	}
	else
	{
		antilag->rewind_platform_edict[antilag->rewind_seek] = 0;
	}
}

antilag_t *antilag_create_player(gedict_t *e)
{
	antilag_t *new_datastruct = &ANTILAG_MEMPOOL[NUM_FOR_EDICT(e)];
	memset(new_datastruct, 0, sizeof(antilag_t));
	new_datastruct->prev = NULL;
	new_datastruct->next = NULL;
	new_datastruct->owner = e;

	if (antilag_list_players != NULL)
	{
		new_datastruct->next = antilag_list_players;
		antilag_list_players->prev = new_datastruct;
	}

	antilag_list_players = new_datastruct;

	return new_datastruct;
}

void antilag_delete_player(gedict_t *e)
{
	antilag_t *data = e->antilag_data;
	if (data->prev != NULL)
	{
		data->prev->next = data->next;
	}
	else if (antilag_list_players == data)
	{
		antilag_list_players = data->next;
	}

	if (data->next != NULL)
	{
		data->next->prev = data->prev;
	}
}

antilag_t *antilag_create_world(gedict_t *e)
{
	antilag_t *new_datastruct = &ANTILAG_MEMPOOL[64 + ANTILAG_MEMPOOL_WORLDSEEK];
	memset(new_datastruct, 0, sizeof(antilag_t));
	ANTILAG_MEMPOOL_WORLDSEEK++;

	new_datastruct->prev = NULL;
	new_datastruct->next = NULL;
	new_datastruct->owner = e;

	if (antilag_list_world != NULL)
	{
		new_datastruct->next = antilag_list_world;
		antilag_list_world->prev = new_datastruct;
	}

	antilag_list_world = new_datastruct;

	return new_datastruct;
}

void antilag_delete_world(gedict_t *e)
{
	antilag_t *data = e->antilag_data;
	if (data->prev != NULL)
	{
		data->prev->next = data->next;
	}
	else if (antilag_list_world == data)
	{
		antilag_list_world = data->next;
	}

	if (data->next != NULL)
	{
		data->next->prev = data->prev;
	}
}

void antilag_updateworld(void)
{
	antilag_t *list;

	if (g_globalvars.time < antilag_nextthink_world)
	{
		return;
	}

	antilag_nextthink_world = g_globalvars.time + cvar("sv_mintic");

	for (list = antilag_list_world; list != NULL; list = list->next)
	{
		antilag_log(list->owner, list);
	}
}

void antilag_lagmove(antilag_t *data, float goal_time)
{
	float seek_time, under_time, over_time, frac;
	int seek, old_seek, no_xerp = true;
	gedict_t *owner = data->owner;
	vec3_t lerp_origin, diff;

	// don't rewind past spawns
	goal_time = max(goal_time, data->owner->spawn_time);

	if (data->owner->client_lastupdated > 0)
	{
		if (goal_time > data->owner->client_lastupdated)
		{
			no_xerp = false;
		}
	}

	if (no_xerp) // this should be true unless client is stuttering a lot
	{
		old_seek = data->rewind_seek;
		seek = data->rewind_seek - 1;

		if (seek < 0)
		{
			seek = ANTILAG_MAXSTATES - 1;
		}

		seek_time = data->rewind_time[seek];
		while (seek != data->rewind_seek && seek_time > goal_time)
		{
			old_seek = seek;
			seek--;
			if (seek < 0)
			{
				seek = ANTILAG_MAXSTATES - 1;
			}
			seek_time = data->rewind_time[seek];
		}

		under_time = data->rewind_time[old_seek];
		over_time = data->rewind_time[seek];
		frac = (goal_time - over_time) / (under_time - over_time);

		if (frac <= 1)
		{
			VectorSubtract(data->rewind_origin[old_seek], data->rewind_origin[seek], diff);

			if (VectorLength(diff) > 48) // whoops, maybe we teleported?
			{
				frac = 1;
			}

			VectorScale(diff, frac, diff);
			VectorAdd(data->rewind_origin[seek], diff, lerp_origin);
		}
		else
		{
			frac = (goal_time - over_time) / (g_globalvars.time - over_time);
			frac = min(frac, 1);

			VectorSubtract(owner->s.v.origin, data->rewind_origin[data->rewind_seek], diff);

			if (VectorLength(diff) > 48) // whoops, maybe we teleported?
			{
				frac = 1;
			}

			VectorScale(diff, frac, diff);
			VectorAdd(data->rewind_origin[data->rewind_seek], diff, lerp_origin);
			seek = data->rewind_seek;
		}
	}
	else // we need to extrapolate to make up for bad connection
	{
		VectorMA(data->owner->s.v.origin, min(goal_time - data->owner->client_lastupdated, ANTILAG_MAX_XERP), data->owner->s.v.velocity, lerp_origin);
	}

	trap_setorigin(NUM_FOR_EDICT(owner), PASSVEC3(lerp_origin));
}

void antilag_getorigin(antilag_t *data, float goal_time)
{
	int old_seek = data->rewind_seek;
	int seek = data->rewind_seek - 1;
	float seek_time, under_time, over_time, frac;
	gedict_t *owner;
	vec3_t lerp_origin, diff;

	if (seek < 0)
	{
		seek = ANTILAG_MAXSTATES - 1;
	}

	seek_time = data->rewind_time[seek];
	while (seek != data->rewind_seek && seek_time > goal_time)
	{
		old_seek = seek;
		seek--;
		if (seek < 0)
		{
			seek = ANTILAG_MAXSTATES - 1;
		}
		seek_time = data->rewind_time[seek];
	}

	under_time = data->rewind_time[old_seek];
	over_time = data->rewind_time[seek];
	frac = (goal_time - over_time) / (under_time - over_time);
	owner = data->owner;

	if (frac <= 1)
	{
		VectorSubtract(data->rewind_origin[old_seek], data->rewind_origin[seek], diff);
		VectorScale(diff, frac, diff);
		VectorAdd(data->rewind_origin[seek], diff, lerp_origin);
	}
	else
	{
		frac = (goal_time - over_time) / (g_globalvars.time - over_time);
		VectorSubtract(owner->s.v.origin, data->rewind_origin[data->rewind_seek], diff);
		VectorScale(diff, frac, diff);
		VectorAdd(data->rewind_origin[data->rewind_seek], diff, lerp_origin);
		seek = data->rewind_seek;
	}

	VectorCopy(lerp_origin, antilag_origin);
}

int antilag_getseek(antilag_t *data, float ms)
{
	float seek_time, goal_time = g_globalvars.time - ms;
	int seek = data->rewind_seek - 1;

	if (seek < 0)
	{
		seek = ANTILAG_MAXSTATES - 1;
	}

	seek_time = data->rewind_time[seek];
	while (seek != data->rewind_seek && seek_time > goal_time)
	{
		seek--;
		if (seek < 0)
		{
			seek = ANTILAG_MAXSTATES - 1;
		}
		seek_time = data->rewind_time[seek];
	}

	return seek;
}

void antilag_lagmove_all(gedict_t *e, float ms)
{
	float rewind_time = g_globalvars.time - ms;
	int lag_platform;
	gedict_t *plat;
	antilag_t *list;
	vec3_t diff, org;

	time_corrected = rewind_time;

	for (list = antilag_list_players; list != NULL; list = list->next)
	{
		if (list->owner->s.v.health <= 0)
		{
			continue;
		}

		VectorCopy(list->owner->s.v.origin, list->held_origin);
		VectorCopy(list->owner->s.v.velocity, list->held_velocity);

		if (list->owner == e)
		{
			lag_platform = list->rewind_platform_edict[antilag_getseek(list, ms)];
			if (lag_platform)
			{
				plat = PROG_TO_EDICT(lag_platform);
				if (plat->antilag_data != NULL)
				{
					VectorClear(diff);
					antilag_getorigin(plat->antilag_data, rewind_time);
					VectorSubtract(antilag_origin, plat->s.v.origin, diff);

					VectorAdd(e->s.v.origin, diff, org);

					trap_setorigin(NUM_FOR_EDICT(e), PASSVEC3(org));
				}
			}
			continue;
		}

		antilag_lagmove(list, rewind_time);
	}

	for (list = antilag_list_world; list != NULL; list = list->next)
	{
		VectorCopy(list->owner->s.v.origin, list->held_origin);
		VectorCopy(list->owner->s.v.velocity, list->held_velocity);
		antilag_lagmove(list, rewind_time);
	}
}

void antilag_unmove_specific(gedict_t *ent)
{
	if (cvar("sv_antilag") != 1)
	{
		return;
	}

	if (time_corrected >= g_globalvars.time)
	{
		return;
	}

	trap_setorigin(NUM_FOR_EDICT(ent), PASSVEC3(ent->antilag_data->held_origin));
}

void antilag_unmove_all(void)
{
	antilag_t *list;

	if (cvar("sv_antilag") != 1)
	{
		return;
	}

	time_corrected = g_globalvars.time;

	for (list = antilag_list_players; list != NULL; list = list->next)
	{
		if (list->owner->s.v.health <= 0)
		{
			continue;
		}

		trap_setorigin(NUM_FOR_EDICT(list->owner), PASSVEC3(list->held_origin));
	}

	for (list = antilag_list_world; list != NULL; list = list->next)
	{
		trap_setorigin(NUM_FOR_EDICT(list->owner), PASSVEC3(list->held_origin));
	}
}

void antilag_lagmove_all_hitscan(gedict_t *e)
{
	float ms;

	if (cvar("sv_antilag") != 1)
	{
		return;
	}

	if (!antilag_current_rewind(e, &ms))
	{
		ms = antilag_current_ping(e);
		ms -= (ms < ANTILAG_MAX_PREDICTION ? (1 / 77.0) : ANTILAG_MAX_PREDICTION);
	}

	if (ms > ANTILAG_REWIND_MAXHITSCAN)
	{
		ms = ANTILAG_REWIND_MAXHITSCAN;
	}
	else if (ms < 0)
	{
		ms = 0;
	}

	antilag_lagmove_all(e, ms);
}
