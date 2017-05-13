// Converted from .qc on 05/02/2016

#ifdef BOT_SUPPORT

#include "g_local.h"
#include "fb_globals.h"

qbool BotDoorIsClosed (gedict_t* door);

// FIXME: saved_goal_desire set to 0 for both yellow armour if door is closed
//        should know that the path is blocked, rather than thinking "bot doesn't want yellow armour"
qbool POVDMM4DontWalkThroughDoor (gedict_t* goal_entity)
{
	if (!streq(g_globalvars.mapname, "povdmm4")) {
		return false;
	}

	if (cvar("k_midair") || cvar("k_instagib")) {
		goal_entity->fb.saved_goal_desire = 0;
		return true;
	}

	if (streq(goal_entity->s.v.classname, "item_armor2")) {
		// Find linked door entity - if closed, set desire to 0
		int i = 0;
		for (i = 0; i < sizeof (goal_entity->fb.paths) / sizeof (goal_entity->fb.paths[0]); ++i) {
			gedict_t* next = goal_entity->fb.paths[i].next_marker;
			if (next && BotDoorIsClosed (next)) {
				goal_entity->fb.saved_goal_desire = 0;
				return true;
			}
		}
	}

	return false;
}

#endif
