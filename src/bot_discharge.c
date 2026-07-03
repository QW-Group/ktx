#ifdef BOT_SUPPORT

#include "g_local.h"

// FIXME: Globals
extern gedict_t *markers[];

// True if the marker is a discharge-at-start marker for the player's team.
static qbool DischargeMarkerForTeam(gedict_t *marker, gedict_t *player)
{
	if ((marker->fb.T & MARKER_DISCHARGE_AT_START_RED) && streq(getteam(player), "red"))
	{
		return true;
	}
	if ((marker->fb.T & MARKER_DISCHARGE_AT_START_BLUE) && streq(getteam(player), "blue"))
	{
		return true;
	}

	return false;
}

static float goal_discharge_marker(gedict_t *self, gedict_t *marker)
{
	// No need to check if we have the lg, as the bot will always pick it up when going to
	// discharge. Otherwise the bot will start going for quad / flag then go back after changing goal.
	if (self->deaths == 0 && DischargeMarkerForTeam(marker, self))
	{
		return 99999;
	}

	return 0;
}

static qbool fb_discharge_marker_touch(gedict_t *ent, gedict_t *player)
{
	self->fb.look_object = ent;
	AssignVirtualGoal(ent);
	return false;
}

void BotsSetUpDischargeMarker(gedict_t *marker)
{
	marker->fb.desire = goal_discharge_marker;
	marker->fb.item_touch = fb_discharge_marker_touch;
}


qbool DischargeAtStartLogic(gedict_t *self)
{
	if (!self->fb.touch_marker || self->deaths != 0)
	{
		return false;
	}

	return DischargeMarkerForTeam(self->fb.touch_marker, self);
}

#endif
