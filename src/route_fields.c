// Converted from .qc on 05/02/2016

#include "g_local.h"
#include "fb_globals.h"

static int subzone_indexes[NUMBER_ZONES] = { 0 };
static gedict_t* zone_tail[NUMBER_ZONES] = { 0 };

// FIXME: Map-specific hack for existing map-specific logic...
extern gedict_t* dm6_door;

// FIXME: globals
extern gedict_t* markers[];

void SetGoalForMarker(int goal, gedict_t* marker) {
	if (goal <= 0 || goal > NUMBER_GOALS)
		return;

	marker->fb.goals[goal - 1].next_marker = marker;
	marker->fb.G_ = goal;
}

void SetGoal(int goal, int marker_number) {
	gedict_t* marker = NULL;

	--marker_number;

	if (marker_number < 0 || marker_number >= NUMBER_MARKERS)
		return;

	marker = markers[marker_number];
	SetGoalForMarker (goal, marker);
}

void SetZone(int zone, int marker_number) {
	gedict_t* marker;
	fb_zone_t* z;

	// old frogbot code was 1-based
	--zone;
	--marker_number;

	if (zone < 0 || zone >= NUMBER_ZONES)
		return;
	if (marker_number < 0 || marker_number >= NUMBER_MARKERS)
		return;

	marker = markers[marker_number];
	z = &marker->fb.zones[zone];

	marker->fb.S_ = subzone_indexes[zone]++;
	z->marker = z->reverse_marker = z->next_zone = z->next = z->reverse_next = marker;
	marker->fb.Z_ = zone;

	AddZoneMarker (marker);
}

void SetMarkerFlag(int marker_number, int flags) {
	--marker_number;
	
	if (marker_number < 0 || marker_number >= NUMBER_MARKERS)
		return;

	markers[marker_number]->fb.T |= flags;

	if (flags & MARKER_IS_DM6_DOOR)
		dm6_door = markers[marker_number];
	if (flags & MARKER_DOOR_TOUCHABLE)
		markers[marker_number]->s.v.solid = SOLID_TRIGGER;
}

void SetMarkerFixedSize (int marker_number, int size_x, int size_y, int size_z)
{
	--marker_number;
	if (marker_number < 0 || marker_number >= NUMBER_MARKERS)
		return;

	if (!markers[marker_number])
		return;

	VectorSet (markers[marker_number]->fb.fixed_size, size_x, size_y, size_z);
}

