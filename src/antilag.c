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
#include "fb_globals.h"

int ANTILAG_MEMPOOL_WORLDSEEK;
antilag_t ANTILAG_MEMPOOL[ANTILAG_MAXEDICTS];
antilag_t *antilag_list_players;
antilag_t *antilag_list_world;
float antilag_nextthink_world;
vec3_t antilag_origin;
vec3_t antilag_retvec;
float time_corrected;

void Physics_PushEntityTrace(float push_x, float push_y, float push_z)
{
	vec3_t push;
	vec3_t end;

	push[0] = push_x;
	push[1] = push_y;
	push[2] = push_z;

	VectorAdd(self->s.v.origin, push, end);

	traceline(PASSVEC3(self->s.v.origin), PASSVEC3(end), false, self);
}

float Physics_PushEntity(float push_x, float push_y, float push_z, int failonstartsolid) // SV_PushEntity
{
	vec3_t push;

	push[0] = push_x;
	push[1] = push_y;
	push[2] = push_z;

	Physics_PushEntityTrace(PASSVEC3(push));

	if (g_globalvars.trace_startsolid && failonstartsolid)
		return g_globalvars.trace_fraction;

	trap_setorigin(NUM_FOR_EDICT(self), PASSVEC3(g_globalvars.trace_endpos));

	if (g_globalvars.trace_fraction < 1 || g_globalvars.trace_startsolid)
	{
		if (self->s.v.solid >= SOLID_TRIGGER && (!((int)self->s.v.flags & FL_ONGROUND) || (self->s.v.groundentity != g_globalvars.trace_ent)))
		{
			other = PROG_TO_EDICT(g_globalvars.trace_ent);
			((void(*)(void))(self->touch))();
		}
	}

	return g_globalvars.trace_fraction;
}

#define MAX_CLIP_PLANES 5
void Physics_ClipVelocity(float vel_x, float vel_y, float vel_z, float norm_x, float norm_y, float norm_z, float f) // SV_ClipVelocity
{
	vec3_t vel;
	vec3_t norm;
	vec3_t vel2;

	vel[0] = vel_x;
	vel[1] = vel_y;
	vel[2] = vel_z;

	norm[0] = norm_x;
	norm[1] = norm_y;
	norm[2] = norm_z;


	VectorScale(norm, DotProduct(vel, norm), vel2);
	VectorScale(vel2, f, vel2);
	VectorSubtract(vel, vel2, vel);

	if (vel[0] > -0.1 && vel[0] < 0.1) vel[0] = 0;
	if (vel[1] > -0.1 && vel[1] < 0.1) vel[1] = 0;
	if (vel[2] > -0.1 && vel[2] < 0.1) vel[2] = 0;

	VectorCopy(vel, antilag_retvec);
}

void Physics_Bounce(float dt)
{
	float gravity_value = cvar("sv_gravity");
	float movetime;
	float bump;

	if ((int)self->s.v.flags & FL_ONGROUND)
	{
		if (self->s.v.velocity[2] >= 1 / 32)
			self->s.v.flags = (int)self->s.v.flags &~ FL_ONGROUND;
		else if (!self->s.v.groundentity)
			return;
	}


	if (self->gravity)
		self->s.v.velocity[2] -= 0.5 * dt * self->gravity * gravity_value;
	else
		self->s.v.velocity[2] -= 0.5 * dt * gravity_value;

	VectorMA(self->s.v.angles, dt, self->s.v.avelocity, self->s.v.angles);

	movetime = dt;

	for (bump = 0; bump < MAX_CLIP_PLANES && movetime > 0; ++bump)
	{
		vec3_t move;
		float d, bouncefac = 0.5, bouncestp = 60 / 100;

		VectorScale(self->s.v.velocity, movetime, move);
		Physics_PushEntity(PASSVEC3(move), false);

		if (g_globalvars.trace_fraction == 1 && !g_globalvars.trace_startsolid)
			break;

		movetime *= 1 - ((g_globalvars.trace_fraction < 1) ? g_globalvars.trace_fraction : 1);

		if (self->gravity)
			bouncestp *= self->gravity * gravity_value;
		else
			bouncestp *= gravity_value;

		Physics_ClipVelocity(PASSVEC3(self->s.v.velocity), PASSVEC3(g_globalvars.trace_plane_normal), 1 + bouncefac);
		VectorCopy(antilag_retvec, self->s.v.velocity);

		d = DotProduct(g_globalvars.trace_plane_normal, self->s.v.velocity);
		if (g_globalvars.trace_plane_normal[2] > 0.7 && d < bouncestp && d > -bouncestp)
		{
			self->s.v.flags = (int)self->s.v.flags | FL_ONGROUND;
			self->s.v.groundentity = g_globalvars.trace_ent;
			VectorClear(self->s.v.velocity);
			VectorClear(self->s.v.avelocity);
		}
		else
		{
			self->s.v.flags = (int)self->s.v.flags &~ FL_ONGROUND;
		}
	}

	if (!((int)self->s.v.flags & FL_ONGROUND))
	{
		if (self->gravity)
			self->s.v.velocity[2] -= 0.5 * dt * self->gravity * gravity_value;
		else
			self->s.v.velocity[2] -= 0.5 * dt * gravity_value;
	}
}

void antilag_log(gedict_t *e, antilag_t *antilag)
{
	// stop extremely fast logging
	if (g_globalvars.time - antilag->rewind_time[antilag->rewind_seek] < 0.01)
		return;

	antilag->rewind_seek++;

	if (antilag->rewind_seek >= ANTILAG_MAXSTATES)
		antilag->rewind_seek = 0;

	VectorCopy(e->s.v.origin, antilag->rewind_origin[antilag->rewind_seek]);
	VectorCopy(e->s.v.velocity, antilag->rewind_velocity[antilag->rewind_seek]);
	antilag->rewind_time[antilag->rewind_seek] = g_globalvars.time;

	if ((int) e->s.v.flags & FL_ONGROUND)
		antilag->rewind_platform_edict[antilag->rewind_seek] = e->s.v.groundentity;
	else
		antilag->rewind_platform_edict[antilag->rewind_seek] = 0;
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
		data->next->prev = data->prev;
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
		data->next->prev = data->prev;
}

void antilag_updateworld(void)
{
	antilag_t *list;

	if (g_globalvars.time < antilag_nextthink_world)
		return;

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

	//don't rewind past spawns
	goal_time = max(goal_time, data->owner->spawn_time);

	if (data->owner->client_lastupdated > 0)
	{
		if (goal_time > data->owner->client_lastupdated)
			no_xerp = false;
	}

	if (no_xerp) // this should be true unless client is stuttering a lot
	{
		old_seek = data->rewind_seek;
		seek = data->rewind_seek - 1;

		if (seek < 0)
			seek = ANTILAG_MAXSTATES - 1;

		seek_time = data->rewind_time[seek];
		while (seek != data->rewind_seek && seek_time > goal_time)
		{
			old_seek = seek;
			seek--;
			if (seek < 0)
				seek = ANTILAG_MAXSTATES - 1;
			seek_time = data->rewind_time[seek];
		}

		under_time = data->rewind_time[old_seek];
		over_time = data->rewind_time[seek];
		frac = (goal_time - over_time) / (under_time - over_time);

		if (frac <= 1)
		{
			VectorSubtract(data->rewind_origin[old_seek], data->rewind_origin[seek], diff);

			if (VectorLength(diff) > 48) // whoops, maybe we teleported?
				frac = 1;

			VectorScale(diff, frac, diff);
			VectorAdd(data->rewind_origin[seek], diff, lerp_origin);
		}
		else
		{
			frac = (goal_time - over_time) / (g_globalvars.time - over_time);
			frac = min(frac, 1);

			VectorSubtract(owner->s.v.origin, data->rewind_origin[data->rewind_seek], diff);

			if (VectorLength(diff) > 48) // whoops, maybe we teleported?
				frac = 1;

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
		seek = ANTILAG_MAXSTATES - 1;

	seek_time = data->rewind_time[seek];
	while (seek != data->rewind_seek && seek_time > goal_time)
	{
		old_seek = seek;
		seek--;
		if (seek < 0)
			seek = ANTILAG_MAXSTATES - 1;
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
		seek = ANTILAG_MAXSTATES - 1;

	seek_time = data->rewind_time[seek];
	while (seek != data->rewind_seek && seek_time > goal_time)
	{
		seek--;
		if (seek < 0)
			seek = ANTILAG_MAXSTATES - 1;
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
			continue;

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

void antilag_lagmove_all_playeronly(gedict_t *e, float ms)
{
	float rewind_time = g_globalvars.time - ms;
	antilag_t *list;

	time_corrected = rewind_time;

	for (list = antilag_list_players; list != NULL; list = list->next)
	{
		if (list->owner->s.v.health <= 0)
			continue;

		if (list->owner == e)
			continue;

		antilag_lagmove(list, rewind_time);
	}
}

void antilag_lagmove_all_nohold(gedict_t *e, float ms, int plat_rewind)
{
	float rewind_time = g_globalvars.time - ms;
	int lag_platform;
	antilag_t *list;
	gedict_t *plat;
	vec3_t diff, org;

	time_corrected = rewind_time;

	for (list = antilag_list_players; list != NULL; list = list->next)
	{
		if (list->owner->s.v.health <= 0)
			continue;

		if (list->owner == e)
		{
			if (plat_rewind)
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
			}
			continue;
		}

		antilag_lagmove(list, rewind_time);
	}

	for (list = antilag_list_world; list != NULL; list = list->next)
	{
		antilag_lagmove(list, rewind_time);
	}
}

void antilag_unmove_specific(gedict_t *ent)
{
	if (cvar("sv_antilag") != 1)
		return;

	if (time_corrected >= g_globalvars.time)
		return;

	trap_setorigin(NUM_FOR_EDICT(ent), PASSVEC3(ent->antilag_data->held_origin));
}

void antilag_unmove_all(void)
{
	antilag_t *list;

	if (cvar("sv_antilag") != 1)
		return;

	time_corrected = g_globalvars.time;

	for (list = antilag_list_players; list != NULL; list = list->next)
	{
		if (list->owner->s.v.health <= 0)
			continue;

		trap_setorigin(NUM_FOR_EDICT(list->owner), PASSVEC3(list->held_origin));
	}

	for (list = antilag_list_world; list != NULL; list = list->next)
	{
		trap_setorigin(NUM_FOR_EDICT(list->owner), PASSVEC3(list->held_origin));
	}
}

void antilag_lagmove_all_hitscan(gedict_t *e)
{
	float ms = (atof(ezinfokey(e, "ping")) / 1000);

	if (cvar("sv_antilag") != 1)
		return;

	ms -= (ms < ANTILAG_MAX_PREDICTION ? (1 / 77.0) : ANTILAG_MAX_PREDICTION);

	if (ms > ANTILAG_REWIND_MAXHITSCAN)
		ms = ANTILAG_REWIND_MAXHITSCAN;
	else if (ms < 0)
		ms = 0;

	antilag_lagmove_all(e, ms);
}

void antilag_lagmove_all_proj(gedict_t *owner, gedict_t *e)
{
	float ms = (atof(ezinfokey(owner, "ping")) / 1000);
	float step_time, current_time;
	antilag_t *list;
	vec3_t old_org;
	gedict_t *oself;

	if (cvar("sv_antilag") != 1)
		return;

	ms -= (ms < ANTILAG_MAX_PREDICTION ? (1 / 77.0) : ANTILAG_MAX_PREDICTION);

	if (ms > ANTILAG_REWIND_MAXPROJECTILE)
		ms = ANTILAG_REWIND_MAXPROJECTILE;
	else if (ms < 0)
		ms = 0;

	e->client_time = ms;

	// log hold stats, because we use nohold antilag moving

	for (list = antilag_list_players; list != NULL; list = list->next)
	{
		if (list->owner->s.v.health <= 0)
			continue;

		VectorCopy(list->owner->s.v.origin, list->held_origin);
		VectorCopy(list->owner->s.v.velocity, list->held_velocity);
	}

	for (list = antilag_list_world; list != NULL; list = list->next)
	{
		VectorCopy(list->owner->s.v.origin, list->held_origin);
		VectorCopy(list->owner->s.v.velocity, list->held_velocity);
	}

	///*
	VectorCopy(owner->s.v.origin, old_org);
	antilag_lagmove_all_nohold(owner, ms, true);
	VectorSubtract(owner->s.v.origin, old_org, old_org);
	VectorAdd(e->s.v.origin, old_org, old_org);
	trap_setorigin(NUM_FOR_EDICT(e), PASSVEC3(old_org));
	VectorCopy(e->s.v.origin, e->oldangles); // store for later maybe
	VectorCopy(owner->antilag_data->held_origin, owner->s.v.origin);
	//*/

	VectorCopy(e->s.v.origin, e->oldangles); // store for later maybe
	e->s.v.armorvalue = ms;

	oself = self;

	step_time = min(cvar("sv_mintic"), ms);
	if (step_time * VectorLength(e->s.v.velocity) > 3)
	{
		// step size * velocity can't be more than player hitbox width, we don't want any shenanigans
		step_time = 8 / VectorLength(e->s.v.velocity);
	}

	current_time = g_globalvars.time - ms;
	// newmis reimplementation
	if (newmis == e)
	{
		antilag_lagmove_all_playeronly(owner, (g_globalvars.time - current_time));
		traceline(PASSVEC3(e->s.v.origin), e->s.v.origin[0] + e->s.v.velocity[0] * 0.05, e->s.v.origin[1] + e->s.v.velocity[1] * 0.05, e->s.v.origin[2] + e->s.v.velocity[2] * 0.05, false, e);
		trap_setorigin(NUM_FOR_EDICT(e), PASSVEC3(g_globalvars.trace_endpos));

		if (g_globalvars.trace_fraction < 1 || g_globalvars.trace_startsolid)
		{
			other = PROG_TO_EDICT(g_globalvars.trace_ent);
			self = e;
			self->s.v.flags = ((int)self->s.v.flags) | FL_GODMODE;
			((void(*)(void))(self->touch))();

			self = oself;
			antilag_unmove_all(); // emergency antilag cleanup
			if (HAVEEXTENSION(G_SETLASTRUNTIME))
				trap_SetLastRuntime(NUM_FOR_EDICT(e));
			return;
		}
	}
	//

	// actual stepping through
	while (current_time < g_globalvars.time)
	{
		float remaining = g_globalvars.time - current_time;
		time_corrected = current_time;
		step_time = remaining < 0.01f ? remaining : bound(0.01, min(step_time, remaining - 0.01), 0.05);
		if (e->s.v.nextthink) { e->s.v.nextthink -= step_time; }

		//antilag_lagmove_all_nohold(owner, (g_globalvars.time - current_time), false);
		antilag_lagmove_all_playeronly(owner, (g_globalvars.time - current_time));
		traceline(PASSVEC3(e->s.v.origin), e->s.v.origin[0] + e->s.v.velocity[0] * step_time,
			e->s.v.origin[1] + e->s.v.velocity[1] * step_time, e->s.v.origin[2] + e->s.v.velocity[2] * step_time,
			false, e);

		trap_setorigin(NUM_FOR_EDICT(e), PASSVEC3(g_globalvars.trace_endpos));

		if (g_globalvars.trace_fraction < 1 || g_globalvars.trace_startsolid)
		{
			//if (g_globalvars.trace_ent)
			//{
			other = PROG_TO_EDICT(g_globalvars.trace_ent);
			self = e;
			self->s.v.flags = ((int)self->s.v.flags) | FL_GODMODE;
			((void(*)(void))(self->touch))();
			break;
			//}
		}

		current_time += step_time;
	}
	//

	self = oself;

	// restore origins to held values
	antilag_unmove_all();
	if (HAVEEXTENSION(G_SETLASTRUNTIME))
		trap_SetLastRuntime(NUM_FOR_EDICT(e));
	time_corrected = g_globalvars.time;
}


void antilag_lagmove_all_proj_bounce(gedict_t *owner, gedict_t *e)
{
	float ms = (atof(ezinfokey(owner, "ping")) / 1000);
	float step_time, current_time;
	antilag_t *list;
	vec3_t old_org;
	gedict_t *oself;

	if (cvar("sv_antilag") != 1)
		return;

	ms -= (ms < ANTILAG_MAX_PREDICTION ? (1 / 77.0) : ANTILAG_MAX_PREDICTION);

	if (ms > ANTILAG_REWIND_MAXPROJECTILE)
		ms = ANTILAG_REWIND_MAXPROJECTILE;
	else if (ms < 0)
		ms = 0;

	e->client_time = ms;

	// log hold stats, because we use nohold antilag moving
	for (list = antilag_list_players; list != NULL; list = list->next)
	{
		if (list->owner->s.v.health <= 0)
			continue;

		VectorCopy(list->owner->s.v.origin, list->held_origin);
		VectorCopy(list->owner->s.v.velocity, list->held_velocity);
	}

	for (list = antilag_list_world; list != NULL; list = list->next)
	{
		VectorCopy(list->owner->s.v.origin, list->held_origin);
		VectorCopy(list->owner->s.v.velocity, list->held_velocity);
	}

	///*
	VectorCopy(owner->s.v.origin, old_org);
	antilag_lagmove_all_nohold(owner, ms, true);
	VectorSubtract(owner->s.v.origin, old_org, old_org);
	VectorAdd(e->s.v.origin, old_org, old_org);
	trap_setorigin(NUM_FOR_EDICT(e), PASSVEC3(old_org));
	VectorCopy(e->s.v.origin, e->oldangles); // store for later maybe
	VectorCopy(owner->antilag_data->held_origin, owner->s.v.origin);
	//*/

	e->s.v.armorvalue = ms;

	oself = self;
	self = e;

	step_time = min(cvar("sv_mintic"), ms);
	if (step_time * VectorLength(e->s.v.velocity) > 32)
	{
		// step size * velocity can't be more than player hitbox width, we don't want any shenanigans
		step_time = 32 / VectorLength(e->s.v.velocity);
	}

	current_time = g_globalvars.time - ms;
	// newmis reimplementation
	if (newmis == e)
	{
		antilag_lagmove_all_playeronly(owner, (g_globalvars.time - current_time));
		Physics_Bounce(0.05);
	}
	//

	// actual step through
	while (current_time < g_globalvars.time)
	{
		float remaining = g_globalvars.time - current_time;
		step_time = remaining < 0.01f ? remaining : bound(0.01f, min(step_time, remaining - 0.01f), 0.05f);

		antilag_lagmove_all_playeronly(owner, (g_globalvars.time - current_time));
		Physics_Bounce(step_time);
		if (self->s.v.nextthink) { self->s.v.nextthink -= step_time; }
		current_time += step_time;
	}
	//

	self = oself;

	// restore origins to held values
	antilag_unmove_all();
	if (HAVEEXTENSION(G_SETLASTRUNTIME))
		trap_SetLastRuntime(NUM_FOR_EDICT(e));
}
