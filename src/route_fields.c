// Converted from .qc on 05/02/2016

#include "g_local.h"
#include "fb_globals.h"

static int subzone_indexes[NUMBER_ZONES] = { 0 };
static gedict_t* zone_tail[NUMBER_ZONES] = { 0 };

// FIXME: Map-specific hack for existing map-specific logic...
extern gedict_t* dm6_door;
gedict_t* spawn_marker (float x, float y, float z);

// FIXME: globals
extern gedict_t* markers[];

void SetGoalForMarker(int goal, gedict_t* marker) {
	if (goal <= 0 || goal > NUMBER_GOALS || marker == NULL)
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
	if (marker == NULL) {
		return;
	}

	z = &marker->fb.zones[zone];

	marker->fb.S_ = subzone_indexes[zone]++;
	z->marker = z->reverse_marker = z->next_zone = z->next = z->reverse_next = marker;
	marker->fb.Z_ = zone + 1;

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

void RemoveMarker (gedict_t* marker)
{
	int i, j;

	if (!streq (marker->s.v.classname, "marker")) {
		G_sprint (self, 2, "Cannot remove non-marker\n");
		return;
	}

	for (i = 0; i < NUMBER_MARKERS; ++i) {
		if (!markers[i])
			continue;

		// Remove from linked paths
		for (j = 0; j < NUMBER_PATHS; ++j) {
			if (markers[i]->fb.paths[j].next_marker == marker) {
				markers[i]->fb.paths[j].next_marker = NULL;
			}
		}

		// Remove marker
		if (markers[i] == marker) {
			ent_remove (markers[i]);
			markers[i] = NULL;
		}
	}
}

gedict_t* CreateNewMarker (vec3_t origin)
{
	gedict_t* new_marker = spawn_marker (PASSVEC3 (origin));
	int i;

	for (i = 0; i < NUMBER_MARKERS; ++i) {
		if (markers[i] == NULL) {
			markers[i] = new_marker;
			new_marker->fb.index = i;
			return new_marker;
		}
	}

	AddToQue (new_marker);

	return new_marker;
}

void MoveMarker (gedict_t* selected, vec3_t move_to)
{
	//selected->
}

qbool CreateNewPath (gedict_t* current, gedict_t* next)
{
	int i;

	for (i = 0; i < NUMBER_PATHS; ++i) {
		if (current->fb.paths[i].next_marker == NULL) {
			current->fb.paths[i].next_marker = next;
			current->fb.paths[i].flags = 0;
			current->fb.paths[i].time = 0;
			return true;
		}
	}

	G_sprint (self, 2, "Source marker already has %d paths, cannot add any more.", NUMBER_PATHS);
	return false;
}

void RemovePath (gedict_t* source, int path_number)
{
	if (path_number < 0 || path_number >= NUMBER_PATHS)
		return;

	source->fb.paths[path_number].flags = 0;
	source->fb.paths[path_number].next_marker = NULL;
	source->fb.paths[path_number].time = 0;
}

int AddPath (gedict_t* source, gedict_t* next)
{
	int i = 0;
	int place = -1;

	if (source == NULL || next == NULL || source == next) {
		return -1;
	}

	for (i = 0; i < NUMBER_PATHS; ++i) {
		if (source->fb.paths[i].next_marker == next) {
			return i;
		}
		if (place < 0 && source->fb.paths[i].next_marker == NULL) {
			place = i;
		}
	}

	if (place >= 0) {
		source->fb.paths[place].next_marker = next;
		source->fb.paths[place].flags = 0;
		source->fb.paths[place].time = 0;
	}
	return place;
}
