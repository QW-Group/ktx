// Converted from .qc on 05/02/2016

#ifdef BOT_SUPPORT

#include "g_local.h"
#include "fb_globals.h"

gedict_t *dm6_door = 0;

qbool CheckNewWeapon(int weapon);

// FIXME: Globals
extern gedict_t *markers[];

void DM6CampLogic()
{
	qbool has_weapon = (int)self->s.v.items & (IT_ROCKET_LAUNCHER | IT_LIGHTNING);

	// Camp the red armor...
	if (NumberOfClients() > 1)
	{
		if (has_weapon && !self->fb.bot_evade)
		{
			if ((self->s.v.health > 80) && (self->s.v.armorvalue > 100))
			{
				if ((self->s.v.ammo_cells > 15) || (self->s.v.ammo_rockets > 3))
				{
					gedict_t *search_entity = ez_find(world, "item_armorInv");

					if (search_entity)
					{
						if (search_entity->s.v.origin[2] <= (self->s.v.origin[2] + 18))
						{
							if (VectorDistance(search_entity->s.v.origin, self->s.v.origin) < 200)
							{
								if (g_random() < 0.9)
								{
									self->fb.camp_state |= CAMPBOT;
									SetLinkedMarker(self, self->fb.touch_marker, "dm6-camp");
								}
							}
							else
							{
								self->fb.camp_state &= ~CAMPBOT;
							}
						}
					}
				}
			}
		}
	}
}

void DM6SelectWeaponToOpenDoor(gedict_t *self)
{
	if (dm6_door && (self->fb.path_state & DM6_DOOR))
	{
		int items_ = (int)self->s.v.items;
		int desired_weapon = 0;

		if (self->s.v.ammo_shells)
		{
			desired_weapon = IT_SHOTGUN;
		}
		else if ((items_ & IT_NAILGUN) && (self->s.v.ammo_nails))
		{
			desired_weapon = IT_NAILGUN;
		}
		else if ((items_ & IT_SUPER_NAILGUN) && (self->s.v.ammo_nails))
		{
			desired_weapon = IT_SUPER_NAILGUN;
		}
		else if ((items_ & IT_LIGHTNING) && (self->s.v.ammo_cells))
		{
			desired_weapon = IT_LIGHTNING;
		}

		self->fb.firing |= CheckNewWeapon(desired_weapon);
	}
}

qbool DM6DoorLogic(gedict_t *self, gedict_t *goal_entity)
{
	if (dm6_door && (goal_entity->fb.Z_ == 1))
	{
		if (self->fb.touch_marker->fb.zones[0].task & DM6_DOOR)
		{
			if (dm6_door->fb.door_entity && dm6_door->fb.door_entity->s.v.takedamage)
			{
				gedict_t *enemy = &g_edicts[self->s.v.enemy];

				if (enemy == self->fb.look_object)
				{
					if (!self->invincible_time)
					{
						// on dm6?...
						if (enemy->fb.firepower >= 50)
						{
							goal_entity->fb.saved_goal_desire = 0;

							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

qbool DM6DoorClosed(fb_path_eval_t *eval)
{
	if (!dm6_door)
	{
		return false;
	}

	return ((eval->test_marker == dm6_door) && !dm6_door->fb.door_entity->s.v.takedamage)
			|| ((eval->description & DM6_DOOR) && (dm6_door->fb.door_entity->s.v.origin[0] > -64));
}

void DM6MarkerTouchLogic(gedict_t *self, gedict_t *goalentity_marker)
{
	if (dm6_door && goalentity_marker && (goalentity_marker->fb.Z_ == 1))
	{
		if (self->fb.touch_marker->fb.zones[0].task & DM6_DOOR)
		{
			if (dm6_door->fb.door_entity->s.v.takedamage)
			{
				vec3_t temp, src, direction;

				VectorAdd(dm6_door->fb.door_entity->s.v.absmin,
							dm6_door->fb.door_entity->s.v.view_ofs, temp);
				VectorSubtract(temp, self->s.v.origin, temp);
				temp[2] -= 40;
				normalize(temp, direction);
				VectorCopy(self->s.v.origin, src);
				src[2] += 16;
				traceline(src[0], src[1], src[2], src[0] + direction[0] * 2048,
							src[1] + direction[1] * 2048, src[2] + direction[2] * 2048, false,
							self);
				if (PROG_TO_EDICT(g_globalvars.trace_ent) == dm6_door->fb.door_entity)
				{
					if (self->fb.debug_path)
					{
						G_bprint(PRINT_HIGH, "**** TRACED TO DOOR ****\n");
					}

					self->fb.path_state |= DM6_DOOR;
				}
			}
			else
			{
				self->fb.path_state &= ~DM6_DOOR;
			}
		}
	}
}

qbool DM6LookAtDoor(gedict_t *self)
{
	if (dm6_door && (self->fb.path_state & DM6_DOOR))
	{
		self->fb.state |= NOTARGET_ENEMY;
		self->fb.look_object = dm6_door->fb.door_entity;

		if (self->fb.debug_path)
		{
			G_bprint(PRINT_HIGH, "looking at door\n");
		}

		return true;
	}

	return false;
}

qbool DM6FireAtDoor(gedict_t *self, vec3_t rel_pos)
{
	if (dm6_door && (self->fb.path_state & DM6_DOOR))
	{
		if (dm6_door->fb.door_entity->s.v.takedamage)
		{
			rel_pos[2] -= 38;
		}
		else
		{
			self->fb.path_state &= ~DM6_DOOR;
			self->fb.state &= ~NOTARGET_ENEMY;
		}

		if (self->fb.debug_path)
		{
			G_bprint(PRINT_HIGH, "Shooting at door\n");
		}

		return true;
	}

	return false;
}

void DM6Debug(gedict_t *self)
{
	int i, j;

	if (dm6_door)
	{
		G_sprint(self, PRINT_HIGH, "DM6 Door is marker #%3d\n", dm6_door->fb.index + 1);
	}
	else
	{
		G_sprint(self, PRINT_HIGH, "DM6 Door not set...\n");
	}

	G_sprint(self, PRINT_HIGH, "Paths with DM6_DOOR set:\n");
	for (i = 0; i < NUMBER_MARKERS; ++i)
	{
		gedict_t *s = markers[i];

		if (s)
		{
			for (j = 0; j < NUMBER_PATHS; ++j)
			{
				gedict_t *t = markers[i]->fb.paths[j].next_marker;

				if (t && (s->fb.paths[j].flags & DM6_DOOR))
				{
					G_sprint(self, PRINT_HIGH, "  > %3d (%s) to %3d (%s)\n", s->fb.index + 1,
								s->classname, t->fb.index + 1, t->classname);
				}
			}
		}
	}

	G_sprint(self, PRINT_HIGH, "Markers with DM6_DOOR task set:\n");
	for (i = 0; i < NUMBER_MARKERS; ++i)
	{
		gedict_t *s = markers[i];

		if (s && (s->fb.zones[0].task & DM6_DOOR))
		{
			G_sprint(self, PRINT_HIGH, "  > %3d (%s)\n", s->fb.index + 1, s->classname);
		}
	}

	if (dm6_door)
	{
		G_sprint(self, PRINT_HIGH, "dm6_door->takedamage = %s\n",
					dm6_door->s.v.takedamage ? "true" : "false");
		if (dm6_door->fb.door_entity)
		{
			G_sprint(self, PRINT_HIGH, "dm6_door->door->takedamage = %s\n",
						dm6_door->fb.door_entity->s.v.takedamage ? "true" : "false");
		}
	}
}

#endif
