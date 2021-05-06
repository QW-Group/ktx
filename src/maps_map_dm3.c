// Converted from .qc on 05/02/2016

#ifdef BOT_SUPPORT

#include "g_local.h"
#include "fb_globals.h"

void DM3CampLogic()
{
	if (NumberOfClients() > 1)
	{
		if (teamplay && deathmatch <= 3)
		{
			if (((int)self->s.v.items & (IT_ROCKET_LAUNCHER | IT_LIGHTNING)) && !self->fb.bot_evade)
			{
				if ((self->s.v.health > 60) && (self->s.v.armorvalue > 80))
				{
					if ((self->s.v.ammo_cells > 15) || (self->s.v.ammo_rockets > 3))
					{
						gedict_t *search_entity = ez_find(world, "item_artifact_super_damage");

						if (search_entity)
						{
							if (g_random() < 0.5)
							{
								if (search_entity->s.v.origin[2] <= (self->s.v.origin[2] + 18))
								{
									vec3_t diff;

									VectorSubtract(search_entity->s.v.origin, self->s.v.origin,
													diff);
									if (vlen(diff) < 200)
									{
										if (g_random() < 0.9)
										{
											self->fb.camp_state |= CAMPBOT;
											SetLinkedMarker(self, self->fb.touch_marker,
															"dm3-camp");
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
	}
}

#endif
