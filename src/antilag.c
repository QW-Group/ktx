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
