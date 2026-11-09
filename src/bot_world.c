#ifdef BOT_SUPPORT

#include "g_local.h"

extern gedict_t *markers[];

void BotEventPlatformHitTop(gedict_t *self)
{
	int i = 0;

	for (i = 0; i < sizeof(self->fb.paths) / sizeof(self->fb.paths[0]); ++i)
	{
		gedict_t *next = self->fb.paths[i].next_marker;
		if (next)
		{
			int j = 0;
			for (j = 0; j < sizeof(self->fb.paths) / sizeof(self->fb.paths[0]); ++j)
			{
				if ((next->fb.paths[j].next_marker == self)
						&& (next->fb.paths[j].flags & VERTICAL_PLATFORM))
				{
					next->fb.paths[j].flags |= ROCKET_JUMP;
				}
			}
		}
	}
}

void BotEventPlatformHitBottom(gedict_t *self)
{
	int i = 0;

	for (i = 0; i < sizeof(self->fb.paths) / sizeof(self->fb.paths[0]); ++i)
	{
		gedict_t *next = self->fb.paths[i].next_marker;
		if (next)
		{
			int j = 0;
			for (j = 0; j < sizeof(self->fb.paths) / sizeof(self->fb.paths[0]); ++j)
			{
				if ((next->fb.paths[j].next_marker == self)
						&& (next->fb.paths[j].flags & VERTICAL_PLATFORM))
				{
					next->fb.paths[j].flags &= ~ROCKET_JUMP;
				}
			}
		}
	}
}

static qbool BotDoorCanHoldUpPath(gedict_t *from_marker, gedict_t *to_marker);

static qbool door_routes_stale = false;

// Route times through a door depend on whether it is shut, see BotDoorHoldsUpPath()
static void BotDoorRoutesChanged(gedict_t *door)
{
	int i, j;

	for (i = 0; i < NUMBER_MARKERS; ++i)
	{
		gedict_t *m = markers[i];

		if (!m)
		{
			continue;
		}

		for (j = 0; j < NUMBER_PATHS; ++j)
		{
			gedict_t *next = m->fb.paths[j].next_marker;

			if (next && (next->fb.door_entity == door) && BotDoorCanHoldUpPath(m, next))
			{
				door_routes_stale = true;

				return;
			}
		}
	}
}

void BotEventDoorHitTop(gedict_t *self)
{
	//G_bprint (2, "DoorHitTop... %s\n", (self->state & STATE_BOTTOM ? "bottom" : "non-bottom"));
	BotDoorRoutesChanged(self);
}

void BotEventDoorHitBottom(gedict_t *self)
{
	//G_bprint (2, "DoorHitBottom... %s\n", (self->state & STATE_BOTTOM ? "bottom" : "non-bottom"));
	BotDoorRoutesChanged(self);
}

// Recalculate route times once a frame at most, however many doors moved
void BotDoorRoutesFrame(void)
{
	if (door_routes_stale)
	{
		door_routes_stale = false;
		RecalculateMarkerRouteTimes();
	}
}

void BotPlatformTouched(gedict_t *platform, gedict_t *player)
{
	if (IsMarkerFrame() && platform->fb.fl_marker)
	{
		check_marker(platform, player);
	}
}

// TODO: the height offset is fixed here, leading to the bot not targetting the player when they are behind barrier on povdmm4
//       improve with VISIBILITY_LOW | NORMAL | HIGH?  
// Test if one player is visible to another.  takes into account other entities & ring
static qbool VisibilityTest(gedict_t *self, gedict_t *visible_object, float min_dot_product)
{
	vec3_t temp;

	if (visible_object->s.v.takedamage)
	{
		// Can only see invisible objects when they're attacking
		if ((g_globalvars.time < visible_object->invisible_finished)
				&& (g_globalvars.time >= visible_object->attack_finished))
		{
			return false;
		}

		// If we can draw straight line between the two...
		traceline(self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] + 32,
					visible_object->s.v.origin[0], visible_object->s.v.origin[1],
					visible_object->s.v.origin[2] + 32, true, self);
		if (g_globalvars.trace_fraction == 1)
		{
			if (min_dot_product == 0)
			{
				return true;
			}

			// Check it's sufficiently in front of the player
			trap_makevectors(self->s.v.v_angle);
			VectorSubtract(visible_object->s.v.origin, self->s.v.origin, temp);
			VectorNormalize(temp);

			return DotProduct (g_globalvars.v_forward, temp) >= min_dot_product;
		}
	}

	return false;
}

qbool Visible_360(gedict_t *self, gedict_t *visible_object)
{
	return (self->fb.enemy_visible = VisibilityTest(self, visible_object, 0.0f));
}

qbool Visible_infront(gedict_t *self, gedict_t *visible_object)
{
	return (self->fb.enemy_visible = VisibilityTest(self, visible_object, self->fb.skill.visibility));
}

qbool BotDoorIsClosed(gedict_t *door)
{
	// Not a door
	if (!door->fb.door_entity)
	{
		return false;
	}

	door = door->fb.door_entity;
	if (streq(door->classname, "door"))
	{
		if (door->fb.T & MARKER_BLOCKED_ON_STATE_TOP)
		{
			return (door->state == STATE_TOP);
		}
		else
		{
			return (door->state == STATE_BOTTOM);
		}
	}

	return false;
}

// A door that only a button or another entity opens can stay shut for a long time, and a bot can't
//   open it by walking up to it, so while it is shut a path into it takes as long as a trip to
//   the button, see BotDoorButton().  Doors that open as a player comes near, or that bots shoot
//   open, don't hold up the path.
static qbool BotDoorCanHoldUpPath(gedict_t *from_marker, gedict_t *to_marker)
{
	gedict_t *door = to_marker->fb.door_entity;
	gedict_t *master;
	gedict_t *opener;

	if (!door)
	{
		return false;
	}

	// linked doors share the targetname and health of the master door
	master = PROG_TO_EDICT(door->s.v.owner);
	if (strnull(master->targetname) || ISLIVE(master))
	{
		return false;
	}

	// from another part of the same door
	if (from_marker->fb.door_entity
			&& (PROG_TO_EDICT(from_marker->fb.door_entity->s.v.owner) == master))
	{
		return false;
	}

	for (opener = world; (opener = find(opener, FOFS(target), master->targetname));)
	{
		// from the button that opens it, or a trigger opens it as a player comes near
		if ((opener == from_marker) || !strncmp(opener->classname, "trigger_", 8))
		{
			return false;
		}
	}

	return true;
}

qbool BotDoorHoldsUpPath(gedict_t *from_marker, gedict_t *to_marker)
{
	return BotDoorCanHoldUpPath(from_marker, to_marker) && BotDoorIsClosed(to_marker);
}

// leaves the middle_marker and zone_time globals that ZoneMarker() sets alone
static float BotRouteTime(gedict_t *from_marker, gedict_t *to_marker)
{
	fb_zone_t *zone;

	if (!to_marker->fb.Z_)
	{
		return 1000000;
	}

	zone = &from_marker->fb.zones[to_marker->fb.Z_ - 1];

	return SubZoneArrivalTime(zone->time, zone->marker, to_marker, false);
}

// Whether a bot at marker can shoot button, a func_button with health
static qbool BotButtonInSight(gedict_t *marker, gedict_t *button)
{
	vec3_t from, to;

	VectorAdd(marker->s.v.absmin, marker->s.v.view_ofs, from);
	from[2] += 16;
	VectorAdd(button->s.v.absmin, button->s.v.view_ofs, to);
	traceline(PASSVEC3(from), PASSVEC3(to), true, world);

	return (g_globalvars.trace_fraction == 1) || (PROG_TO_EDICT(g_globalvars.trace_ent) == button);
}

// Looks at a button with health on the route to shoot it, see SetFireButton()
qbool BotLookAtButton(gedict_t *self, gedict_t *button)
{
	vec3_t to;

	if (!button || !button->s.v.takedamage)
	{
		return false;
	}

	VectorAdd(button->s.v.absmin, button->s.v.view_ofs, to);
	traceline(self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] + 16, PASSVEC3(to), true,
				self);
	if ((g_globalvars.trace_fraction != 1) && (PROG_TO_EDICT(g_globalvars.trace_ent) != button))
	{
		return false;
	}

	self->fb.look_object = button;

	return true;
}

// The button that opens to_marker's door with the shortest trip there and back from from_marker,
//   or NULL if they are all behind the door.  A button with health is only used if it can be shot
//   from from_marker.
gedict_t* BotDoorButton(gedict_t *from_marker, gedict_t *to_marker, float *detour_time)
{
	gedict_t *master = PROG_TO_EDICT(to_marker->fb.door_entity->s.v.owner);
	gedict_t *best_button = NULL;
	float best_time = CLOSED_DOOR_PATH_TIME;
	gedict_t *opener;

	for (opener = world; (opener = find(opener, FOFS(target), master->targetname));)
	{
		float opener_time;

		if (!streq(opener->classname, "func_button"))
		{
			continue;
		}

		if (ISLIVE(opener))
		{
			if (!BotButtonInSight(from_marker, opener))
			{
				continue;
			}

			// turn, shoot and wait for the door
			opener_time = 1;
		}
		else if (opener->fb.fl_marker)
		{
			opener_time = BotRouteTime(from_marker, opener) + BotRouteTime(opener, from_marker);
		}
		else
		{
			continue;
		}

		if (opener_time < best_time)
		{
			best_time = opener_time;
			best_button = opener;
		}
	}

	if (detour_time)
	{
		*detour_time = best_time;
	}

	return best_button;
}

// Follows the route from from_marker to to_marker and returns the button to press for the first
//   door on it that holds up the path, and in door_from the marker before that door
gedict_t* BotDoorButtonOnRoute(gedict_t *from_marker, gedict_t *to_marker, qbool rl_routes,
								gedict_t **door_from)
{
	gedict_t *m = from_marker;
	int i;

	for (i = 0; m && (m != to_marker) && (i < NUMBER_MARKERS); ++i)
	{
		gedict_t *next;

		if (m->fb.Z_ == to_marker->fb.Z_)
		{
			fb_subzone_t *sub = &m->fb.subzones[to_marker->fb.S_];

			next = (rl_routes ? sub->next_marker_rj : sub->next_marker);
		}
		else
		{
			next = ZonePathMarker(m, to_marker, true, rl_routes);
		}

		if (next && BotDoorHoldsUpPath(m, next))
		{
			*door_from = m;

			return BotDoorButton(m, next, NULL);
		}

		m = next;
	}

	return NULL;
}

#endif
