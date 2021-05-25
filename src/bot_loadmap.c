/*
 loadmap.qc

 Copyright (C) 1997-1999 Robert 'Frog' Field
 Copyright (C) 2000-2007 ParboiL
 */

// Converted from .qc on 05/02/2016
#ifdef BOT_SUPPORT

#include "g_local.h"
#include "fb_globals.h"

static qbool map_supported = false;

void check_marker(gedict_t *self, gedict_t *other);
qbool LoadBotRoutingFromFile(void);
qbool pickup_true(void);
int AddPath(gedict_t *marker, gedict_t *next_marker);
void AssignGoalNumbers(void);

// fixme: also in doors.c
#define SECRET_OPEN_ONCE 1	// stays open

typedef struct fb_mapping_s
{
	char *name;
	fb_void_funcref_t func;
} fb_mapping_t;

fb_spawn_t* ItemSpawnFunction(int i);

void InvalidMap()
{
	G_sprint(self, 2, "This map does not have bot-support.\n");
}

static void fb_spawn_button(gedict_t *ent)
{
	AddToQue(ent);

	if (ent->s.v.health)
	{
		//Add_takedamage(ent);
	}
	else
	{
		BecomeMarker(ent);
		adjust_view_ofs_z(ent);
	}
}

static void fb_spawn_spawnpoint(gedict_t *ent)
{
	AddToQue(ent);
	ent->s.v.solid = SOLID_TRIGGER;
	ent->touch = (func_t) marker_touch;
	ent->s.v.flags = FL_ITEM;
	BecomeMarker(ent);
	setsize(ent, -65, -65, -24, 65, 65, 32);
	SetVector(ent->s.v.view_ofs, 80, 80, 24);
	ent->fb.pickup = pickup_true;
	adjust_view_ofs_z(ent);
}

static void fb_spawn_door(gedict_t *ent)
{
	gedict_t *original = ent;
	vec3_t position;

	VectorScale(original->s.v.mins, 0.5, position);
	VectorMA(position, 0.5, original->s.v.maxs, position);
	position[2] = min(original->s.v.mins[2], original->s.v.maxs[2]) + 24;
	ent = CreateMarker(PASSVEC3(position));
	ent->classname = "door_marker";
	ent->fb.door_entity = original;
	ent->s.v.solid = SOLID_NOT; // this will be set to SOLID_TRIGGER if MARKER_DOOR_TOUCHABLE flag set

	if (ent->fb.wait < 0)
	{
		// TODO: ?
		return;
	}
	else if ((int) ent->s.v.spawnflags & SECRET_OPEN_ONCE)
	{
		// TODO: ?
		return;
	}
	else
	{
		if (ent->s.v.health)
		{
			//Add_takedamage(ent);
		}

		adjust_view_ofs_z(ent);
		//BecomeMarker(ent);
	}
}

static void fb_spawn_plat(gedict_t *ent)
{
	AddToQue(ent);

	VectorSubtract(ent->s.v.absmax, ent->s.v.absmin, ent->s.v.view_ofs);
	VectorScale(ent->s.v.view_ofs, 0.5f, ent->s.v.view_ofs);
	ent->s.v.view_ofs[2] = ent->s.v.absmax[2] - ent->s.v.absmin[2] + 23;
	adjust_view_ofs_z(ent);
	BecomeMarker(ent);
}

static void fb_spawn_simple(gedict_t *ent)
{
	AddToQue(ent);

	if (ent->fb.wait < 0)
	{
		// Remove... so ignore?
		return;
	}
	else
	{
		if (ent->s.v.health)
		{
			//Add_takedamage(ent);
		}
	}
}

static void fb_spawn_trigger_teleport(gedict_t *ent)
{
	AddToQue(ent);

	//Com_Printf ("fb_trigger_tele([%f %f %f] > [%f %f %f])\n", PASSVEC3 (ent->s.v.mins), PASSVEC3 (ent->s.v.maxs));
	VectorSet(ent->fb.virtual_mins, ent->s.v.absmin[0] - 18, ent->s.v.absmin[1] - 18,
				ent->s.v.absmin[2] - 34);
	VectorSet(ent->fb.virtual_maxs, ent->s.v.absmax[0] + 18, ent->s.v.absmax[1] + 18,
				ent->s.v.absmax[2] + 26);
	//Com_Printf("> virtual [%f %f %f] [%f %f %f]\n", PASSVEC3(ent->fb.virtual_mins), PASSVEC3(ent->fb.virtual_maxs));
	setsize(ent, ent->s.v.mins[0] - 32, ent->s.v.mins[1] - 32, ent->s.v.mins[2],
			ent->s.v.maxs[0] + 32, ent->s.v.maxs[1] + 32, ent->s.v.maxs[2]);
	VectorSet(ent->s.v.view_ofs, 0.5 * (ent->s.v.absmax[0] - ent->s.v.absmin[0]),
				0.5 * (ent->s.v.absmax[1] - ent->s.v.absmin[1]),
				0.5 * (ent->s.v.absmax[2] - ent->s.v.absmin[2]));
	adjust_view_ofs_z(ent);
	BecomeMarker(ent);
}

static void fb_spawn_teleport_destination_touch(void)
{
	if (IsMarkerFrame() && (other->ct == ctPlayer))
	{
		check_marker(self, other);
	}
}

static void fb_spawn_teleport_destination(gedict_t *ent)
{
	AddToQue(ent);

	ent->s.v.solid = SOLID_TRIGGER;
	ent->touch = (func_t) fb_spawn_teleport_destination_touch;
	ent->s.v.flags = FL_ITEM;
	BecomeMarker(ent);
	setsize(ent, -65, -65, -24 - 27, 65, 65, 32);  // -27 extra to get back to floor
	VectorSet(ent->s.v.view_ofs, 80, 80, 24);
	ent->fb.pickup = pickup_true;
	adjust_view_ofs_z(ent);
}

static fb_spawn_t stdSpawnFunctions[] =
{
	{ "door", fb_spawn_door },  // covers func_door and func_door_secret
	{ "func_button", fb_spawn_button },
	{ "info_player_deathmatch", fb_spawn_spawnpoint },
	{ "info_teleport_destination", fb_spawn_teleport_destination },
	{ "plat", fb_spawn_plat },
	{ "train", fb_spawn_simple },
	{ "trigger_changelevel", fb_spawn_simple },
	{ "trigger_counter", fb_spawn_simple },
	{ "trigger_hurt", fb_spawn_simple },
	{ "trigger_multiple", fb_spawn_simple },
	{ "trigger_once", fb_spawn_simple },
	{ "trigger_onlyregistered", fb_spawn_simple },
	{ "trigger_push", fb_spawn_simple },
	{ "trigger_secret", fb_spawn_simple },
	{ "trigger_setskill", fb_spawn_simple },
	{ "trigger_teleport", fb_spawn_trigger_teleport }
};

void SetMarkerIndicatorPosition(gedict_t *item, gedict_t *indicator)
{
	vec3_t pos;

	VectorAdd(item->s.v.absmin, item->s.v.absmax, pos);
	VectorScale(pos, 0.5f, pos);
	if (streq(item->classname, "plat"))
	{
		VectorAdd(item->s.v.mins, item->s.v.maxs, pos);
		VectorScale(pos, 0.5f, pos);
	}

	setorigin(indicator, PASSVEC3(pos));
}

static void SpawnMarkerIndicator(gedict_t *item)
{
	gedict_t *p;

	if (FrogbotShowMarkerIndicators())
	{
		p = spawn();
		p->s.v.flags = FL_ITEM;
		p->s.v.solid = SOLID_NOT;
		p->s.v.movetype = MOVETYPE_NONE;
		setmodel(p, "progs/w_g_key.mdl");
		p->netname = "Marker";
		p->classname = "marker_indicator";
		p->fb.index = item->fb.index;

		SetMarkerIndicatorPosition(item, p);
	}
}

static qbool ProcessedItem(gedict_t *item)
{
	return (item->fb.fl_marker || item->fb.index);
}

static void CreateItemMarkers()
{
	// Old frogbot method was to call during item spawns, we just 
	//    catch up afterwards once we know the map is valid
	gedict_t *item;

	for (item = world; (item = nextent(item));)
	{
		int i = 0;
		qbool found = false;

		// Don't bother with search if it's already processed
		if (ProcessedItem(item))
		{
			continue;
		}

		// check for item spawn
		for (i = 0; i < ItemSpawnFunctionCount(); ++i)
		{
			fb_spawn_t *spawn = ItemSpawnFunction(i);
			if (streq(spawn->name, item->classname))
			{
				BecomeMarker(item);
				spawn->func(item);
				found = true;
				break;
			}
		}

		// check for std spawn (world items like buttons etc)
		if (!found)
		{
			for (i = 0; i < sizeof(stdSpawnFunctions) / sizeof(stdSpawnFunctions[0]); ++i)
			{
				if (streq(stdSpawnFunctions[i].name, item->classname))
				{
					stdSpawnFunctions[i].func(item);
					found = true;
					break;
				}
			}
		}

		if (found && ProcessedItem(item))
		{
			SpawnMarkerIndicator(item);
		}
	}
}

// After all markers have been created, re-process items
static void AssignVirtualGoals(void)
{
	gedict_t *item;

	for (item = world; (item = nextent(item));)
	{
		int i = 0;

		for (i = 0; i < ItemSpawnFunctionCount(); ++i)
		{
			fb_spawn_t *spawn = ItemSpawnFunction(i);
			if (streq(spawn->name, item->classname))
			{
				AssignVirtualGoal_apply(item);
				break;
			}
		}
	}
}

// Item creation functions 
static void PlaceItemFB(gedict_t *ent)
{
	vec3_t new_size =
		{ 49, 49, 0 };

	if (ent->fb.fixed_size[0])
	{
		new_size[0] = ent->fb.fixed_size[0] / 2 - 15;
	}

	if (ent->fb.fixed_size[1])
	{
		new_size[1] = ent->fb.fixed_size[1] / 2 - 15;
	}

	if (ent->fb.fixed_size[2])
	{
		new_size[2] = ent->fb.fixed_size[2] / 2 - 0;
	}

	ent->s.v.movetype = MOVETYPE_NONE;
	setsize(ent, ent->s.v.mins[0] - new_size[0], ent->s.v.mins[1] - new_size[1], ent->s.v.mins[2],
			ent->s.v.maxs[0] + new_size[0], ent->s.v.maxs[1] + new_size[1], ent->s.v.maxs[2]);
	adjust_view_ofs_z(ent);
	VectorSet(ent->fb.virtual_mins, ent->s.v.absmin[0] + (new_size[0] - 49 + 32),
				ent->s.v.absmin[1] + (new_size[1] - 49 + 32),
				ent->s.v.absmin[2] - (new_size[2] - 0 + 33));
	VectorSet(ent->fb.virtual_maxs, ent->fb.virtual_mins[0] + 96, ent->fb.virtual_mins[1] + 96,
				ent->fb.virtual_mins[2] + 114);
}

// KTX has in-built modifications to several maps - frogbot routing relies on entity order so we have
//   to customise again here.  Called after all markers created, but before traveltime calculations
static void CustomiseFrogbotMap(void)
{
	gedict_t *ent = NULL;

	// KTX may have added a quad, so to keep routes compatible with PR1-version, we add it as a marker after others
	if (streq(g_globalvars.mapname, "aerowalk") && !FrogbotOptionEnabled(FB_OPTION_EDITOR_MODE))
	{
		gedict_t *quad = ez_find(world, "item_artifact_super_damage");
		if (quad)
		{
			gedict_t *nearest_marker;

			quad->fb.fl_marker = false;
			nearest_marker = LocateMarker(quad->s.v.origin);
			quad->fb.fl_marker = true;
			StartItemFB(quad);
			quad->fb.T |= MARKER_DYNAMICALLY_ADDED;

			// Quad is in same zone as nearest marker, and linked by the first path that's valid
			SetZone(nearest_marker->fb.Z_, quad->fb.index + 1);
			SetGoalForMarker(18, quad);
			AddPath(nearest_marker, quad);
			AddPath(quad, nearest_marker);

			SpawnMarkerIndicator(quad);
		}
	}

	// We stopped it from removing the telespawn earlier on...
	if (!cvar("k_end_tele_spawn") && streq("end", g_globalvars.mapname))
	{
		vec3_t TS_ORIGIN =
			{ -392, 608, 40 }; // tele spawn
		gedict_t *p = NULL;
		gedict_t *m = NULL;

		for (p = world; (p = find(p, FOFCLSN, "info_player_deathmatch"));)
		{
			if (VectorCompare(p->s.v.origin, TS_ORIGIN))
			{
				p->classname = "info_player_deathmatch_removed";

				// Remove any spawn marker
				for (m = world; (m = find(m, FOFCLSN, "spawnpoint"));)
				{
					if (m->k_lastspawn == p)
					{
						ent_remove(m);
						break;
					}
				}

				break;
			}
		}
	}

	// Expand bounding box of all items
	if (!isRACE())
	{
		for (ent = world; (ent = nextent(ent));)
		{
			if (streq(ent->classname, "info_teleport_destination")
					|| streq(ent->classname, "info_player_deathmatch"))
			{
				continue;
			}

			if (streq(ent->classname, "marker"))
			{
				vec3_t mins =
					{ -65, -65, -24 };
				vec3_t maxs =
					{ 65, 65, 32 };
				vec3_t viewoffset =
					{ 80, 80, 24 };
				int i;

				for (i = 0; i < 3; ++i)
				{
					if (ent->fb.fixed_size[i])
					{
						mins[i] = -ent->fb.fixed_size[i] / 2 - (i < 2 ? 15 : 0);
						maxs[i] = ent->fb.fixed_size[i] / 2 - (i < 2 ? 15 : 0);
						viewoffset[i] = (maxs[i] - mins[i]) / 2;
					}
				}

				VectorCopy(viewoffset, ent->s.v.view_ofs);
				setsize(ent, PASSVEC3(mins), PASSVEC3(maxs));
			}
			else if ((int) ent->s.v.flags & FL_ITEM)
			{
				PlaceItemFB(ent);
			}
		}
	}

	// Link all teleporters
	if (FrogbotOptionEnabled(FB_OPTION_EDITOR_MODE))
	{
		for (ent = world; (ent = ez_find(ent, "trigger_teleport"));)
		{
			// If this teleport takes us to the marker close to the grenade, set arrow_time
			if (!strnull(ent->target))
			{
				gedict_t *target = find(world, FOFS(targetname), ent->target);

				AddPath(ent, target);
			}
		}
	}
}

void LoadMap(void)
{
	// Need to do this anyway, otherwise teleporters will be broken
	CreateItemMarkers();

	if (!(isRACE() || isCTF()) && deathmatch)
	{
		// If we have a .bot file, use that
		if (LoadBotRoutingFromFile())
		{
			map_supported = true;
			CustomiseFrogbotMap();
			AssignVirtualGoals();
			AllMarkersLoaded();

			return;
		}
	}

	// At this point it's an unsupported map
	CustomiseFrogbotMap();
	if (FrogbotOptionEnabled(FB_OPTION_EDITOR_MODE))
	{
		gedict_t *e;

		// We don't want spawnpoint markers or powerups to mess with colours
		for (e = world; (e = nextent(e));)
		{
			e->s.v.effects = (int) e->s.v.effects & ~(EF_BLUE | EF_GREEN | EF_RED);
		}

		AssignGoalNumbers();
	}
}

qbool FrogbotsCheckMapSupport(void)
{
	if (!map_supported && self)
	{
		G_sprint(self, 2, "Map %s not supported for bots\n", g_globalvars.mapname);
	}

	return map_supported;
}

void BecomeMarker(gedict_t *marker)
{
	marker->fb.fl_marker = true;
}

#endif // BOT_SUPPORT
