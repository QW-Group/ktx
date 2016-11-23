
#include "g_local.h"
#include "fb_globals.h"

#define ARROW_TIME_INCREASE       0.15  // Seconds to advance after NewVelocityForArrow

void SetLinkedMarker (gedict_t* player, gedict_t* marker, char* explanation)
{
	gedict_t* touch = player->fb.touch_marker;

	//if (player->isBot && marker != player->fb.linked_marker)
	//	G_sprint (player, 2, "linked to %3d/%s, touching %3d/%s g %s (%s)\n", marker ? marker->fb.index : -1, marker ? marker->s.v.classname : "(null)", touch ? touch->fb.index : -1, touch ? touch->s.v.classname : "(null)", g_edicts[player->s.v.goalentity].s.v.classname, explanation ? explanation : "");
	if (player->isBot && player->fb.debug_path)
		G_bprint (2, "linked to %3d/%s, touching %3d/%s g %s (%s)\n", marker ? marker->fb.index : -1, marker ? marker->s.v.classname : "(null)", touch ? touch->fb.index : -1, touch ? touch->s.v.classname : "(null)", g_edicts[player->s.v.goalentity].s.v.classname, explanation ? explanation : "");

	// FIXME: This should be when they touch the marker, not when linking to it
	if (player->fb.fixed_goal == marker) {
		G_bprint (2, "at goal, path complete.\n");
		player->fb.fixed_goal = NULL;
		player->fb.debug_path = false;
	}

	player->fb.linked_marker = marker;
}

void SetDirectionMove (gedict_t* self, vec3_t dir_move, const char* explanation)
{
	normalize (dir_move, self->fb.dir_move_);

//	if (self->isBot && self->fb.debug_path)
//		G_bprint (2, "SetDirection(%2.2f %2.2f %2.2f): %s\n", PASSVEC3(self->fb.dir_move_), explanation);
}

void NewVelocityForArrow(gedict_t* self, vec3_t dir_move, const char* explanation) {
	SetDirectionMove (self, dir_move, explanation);
	self->fb.arrow_time = g_globalvars.time + ARROW_TIME_INCREASE;
}

