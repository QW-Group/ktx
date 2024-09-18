/*
 route_lookup.qc

 Copyright (C) 1997-1999 Robert 'Frog' Field
 */

// Converted from .qc on 05/02/2016

#ifdef BOT_SUPPORT

#include "g_local.h"

float SubZoneArrivalTime(float zone_time, gedict_t *middle_marker, gedict_t *from_marker,
							qbool rl_routes, qbool hook_routes)
{
	if (hook_routes)
	{
		return (zone_time + middle_marker->fb.subzones[from_marker->fb.S_].hook_time);
	}
	else if (rl_routes)
	{
		return (zone_time + middle_marker->fb.subzones[from_marker->fb.S_].rj_time);
	}
	else
	{
		return (zone_time + middle_marker->fb.subzones[from_marker->fb.S_].time);
	}
}

gedict_t* SubZoneNextPathMarker(gedict_t *from_marker, gedict_t *to_marker)
{
	return (from_marker && to_marker ? from_marker->fb.subzones[to_marker->fb.S_].next_marker : NULL);
}

gedict_t* SightFromMarkerFunction(gedict_t *from_marker, gedict_t *to_marker)
{
	return (to_marker && from_marker && from_marker->fb.Z_ ?
			to_marker->fb.zones[from_marker->fb.Z_ - 1].sight_from : NULL);
}

gedict_t* HigherSightFromFunction(gedict_t *from_marker, gedict_t *to_marker)
{
	return (to_marker && from_marker && from_marker->fb.Z_ ?
			to_marker->fb.zones[from_marker->fb.Z_ - 1].higher_sight_from : NULL);
}

float SightFromTime(gedict_t *from_marker, gedict_t *to_marker)
{
	return (to_marker && from_marker && from_marker->fb.Z_ ?
			to_marker->fb.zones[from_marker->fb.Z_ - 1].sight_from_time : 0.0f);
}

void ZoneMarker(gedict_t *from_marker, gedict_t *to_marker, qbool path_normal, qbool rl_jump_routes, qbool hook_routes)
{
	fb_zone_t *zone;

	if ((from_marker == NULL) || (to_marker == NULL))
	{
		middle_marker = dropper;
		zone_time = 1000000;

		return;
	}

	if (!to_marker->fb.Z_)
	{
		return;
	}

	zone = &from_marker->fb.zones[to_marker->fb.Z_ - 1];
	if (path_normal)
	{
		middle_marker = rl_jump_routes ? zone->marker_rj : zone->marker;
		zone_time = rl_jump_routes ? zone->rj_time : zone->time;

		middle_marker = hook_routes ? zone->marker_hook : zone->marker;
		zone_time = hook_routes ? zone->hook_time : zone->time;
	}
	else
	{
		middle_marker = from_marker->fb.zones[to_marker->fb.Z_ - 1].reverse_marker;
		zone_time = from_marker->fb.zones[to_marker->fb.Z_ - 1].reverse_time;
	}
}

gedict_t* ZonePathMarker(gedict_t *from_marker, gedict_t *to_marker, qbool path_normal,
							qbool rl_jump_routes, qbool hook_routes)
{
	if ((from_marker == NULL) || (to_marker == NULL) || (to_marker->fb.Z_ == 0))
	{
		return NULL;
	}

	if (path_normal)
	{
		if (rl_jump_routes)
		{
			return from_marker->fb.zones[to_marker->fb.Z_ - 1].next_rj;
		}

		if (hook_routes)
		{
			return from_marker->fb.zones[to_marker->fb.Z_ - 1].next_hook;
		}

		return from_marker->fb.zones[to_marker->fb.Z_ - 1].next;
	}

	return from_marker->fb.zones[to_marker->fb.Z_ - 1].reverse_next;
}

// This is called when the standard highersight calculation has failed
// Difference in 'higher' is that they must be on different heights (>=40)
// Otherwise no difference?
// from_marker = zone with all markers to be checked, marker->to_marker must be visible, marker must be higher
gedict_t* SightMarker(gedict_t *from_marker, gedict_t *to_marker, float max_distance,
						float min_height_diff)
{
	gedict_t *marker_;
	vec3_t marker_pos;
	vec3_t to_marker_pos;
	gedict_t *look_marker = NULL;

	look_traveltime = 1000000;
	middle_marker = from_marker;
	zone_time = 0;

	VectorAdd(to_marker->s.v.absmin, to_marker->s.v.view_ofs, to_marker_pos);
	to_marker_pos[2] += 32;

	// Forall markers in this zone...
	marker_ = from_marker->fb.Z_head;
	while (marker_)
	{
		VectorAdd(marker_->s.v.absmin, marker_->s.v.view_ofs, marker_pos);
		marker_pos[2] += 32;

		// Only check markers where this marker is higher than target
		if ((min_height_diff == 0) || ((marker_pos[2] - to_marker_pos[2]) >= min_height_diff))
		{
			if ((max_distance == 0) || VectorDistance(marker_pos, to_marker_pos) <= max_distance)
			{
				// Must be able to draw straight line between the two
				traceline(PASSVEC3(to_marker_pos), PASSVEC3(marker_pos), true, world);
				if (g_globalvars.trace_fraction == 1)
				{
					// 
					traveltime = SubZoneArrivalTime(zone_time, middle_marker, marker_, false, false);
					if (look_traveltime > traveltime)
					{
						// Teleports don't count
						if (strneq(marker_->classname, "trigger_teleport"))
						{
							look_traveltime = traveltime;
							look_marker = marker_;
						}
					}
				}
			}
		}

		marker_ = marker_->fb.Z_next;
	}

	return look_marker;
}

#endif
