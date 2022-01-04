#ifdef BOT_SUPPORT

#include "g_local.h"

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

void BotEventDoorHitTop(gedict_t *self)
{
	//G_bprint (2, "DoorHitTop... %s\n", (self->state & STATE_BOTTOM ? "bottom" : "non-bottom"));
}

void BotEventDoorHitBottom(gedict_t *self)
{
	//G_bprint (2, "DoorHitBottom... %s\n", (self->state & STATE_BOTTOM ? "bottom" : "non-bottom"));
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

#endif
