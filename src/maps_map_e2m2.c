#ifdef BOT_SUPPORT

#include "g_local.h"

// FIXME: Globals
extern gedict_t *markers[];

static float goal_discharge_marker(gedict_t* self, gedict_t* marker)
{
	// No need to check if we have the lg, as the bot will always pick it up when going to
	// discharge. Otherwise the bot will start going for quad / flag then go back after changing goal.
	if (self->deaths == 0 && self->team_no == 0)
	{
		return 99999;
	}

	return 0;
}

static qbool fb_discharge_marker_touch(gedict_t* ent, gedict_t* player)
{
	self->fb.look_object = ent;
	AssignVirtualGoal(ent);
	return false;
}

void BotsSetUpE2M2DischargeMarker(gedict_t* marker)
{
	marker->fb.desire = goal_discharge_marker;
	marker->fb.item_touch = fb_discharge_marker_touch;
}


qbool E2M2DischargeLogic(gedict_t *self)
{
	if (self->fb.touch_marker && self->fb.touch_marker->fb.T & MARKER_E2M2_DISCHARGE && self->deaths == 0 && self->team_no == 0)
	{
		return true;
	}

	return false;
}

#endif
