// Converted from .qc on 05/02/2016

#ifdef BOT_SUPPORT

#include "g_local.h"
#include "fb_globals.h"

void AMPHI2BotInLava(void) {
	// TODO: rewrite... essentially if in lava and enemy isn't shafting, jump to target instead of walk
	//       this is probably true of all lava-burn situations, as long as we're not lava-rjing to escape?
	if ( self->isBot && streq(g_globalvars.mapname, "amphi2") ) {
		if (g_globalvars.time > self->fb.arrow_time) {
			if (self->s.v.waterlevel == 1) {
				vec3_t point = { self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] - 24 };
				if (trap_pointcontents(point[0], point[1], point[2]) == CONTENT_LAVA) {
					if ((int)self->s.v.flags & FL_ONGROUND) {
						if (!enemy_shaft_attack()) {
							if (!self->fb.rocketJumping) {
								NewVelocityForArrow (self, self->fb.dir_move_, "amphi2");
								SetJumpFlag (self, true, "AMPHI2 logic");
							}
						}
					}
				}
			}
		}
	}
}

#endif
