// Converted from .qc on 05/02/2016

#ifdef BOT_SUPPORT

#include "g_local.h"
#include "fb_globals.h"

void DM4CampLogic() {
	gedict_t* enemy_ = &g_edicts[self->s.v.enemy];

	if (isDuel()) {
		if ((int)self->s.v.items & (IT_ROCKET_LAUNCHER | IT_LIGHTNING) && !self->fb.bot_evade) {
			if ((self->s.v.health > 50) && (self->s.v.armorvalue > 30)) {
				if ((self->s.v.ammo_cells > 15) || (self->s.v.ammo_rockets > 3)) {
					if (g_random() < 0.985) {
						vec3_t above_lg = { 448, -176, 60 };
						vec3_t on_quad_stairs = { 280, -330, 60 };

						if ((enemy_->s.v.origin[0] < 700) && (VectorDistance(above_lg, self->s.v.origin) < 200)) {
							self->fb.camp_state |= CAMPBOT;
							SetLinkedMarker(self, self->fb.touch_marker, "dm4-camp1");
						}
						else if ((enemy_->s.v.origin[0] >= 700) && (VectorDistance(on_quad_stairs, self->s.v.origin) < 200)) {
							self->fb.camp_state |= CAMPBOT;
							SetLinkedMarker(self, self->fb.touch_marker, "dm4-camp2");
						}
						else {
							self->fb.camp_state &= ~CAMPBOT;
						}
					}
				}
			}
		}
	}
}

#endif
