// Converted from .qc on 05/02/2016

#include "g_local.h"
#include "fb_globals.h"

#define BOT_DROWN_SAFETY_TIME           2  // Time before air running out that the bot starts searching for air

static qbool BotCanReachMarker(gedict_t* self) {
	vec3_t spot1,
	       spot2;
	VectorCopy(self->fb.linked_marker->s.v.origin, spot2);
	VectorAdd(self->s.v.origin, self->s.v.view_ofs, spot1);
	traceline(spot1[0], spot1[1], spot1[2], spot2[0], spot2[1], spot2[2], true, self);
	return (g_globalvars.trace_fraction == 1);
}

static qbool BotSwimDown(gedict_t* self) {
	return (self->fb.linked_marker->s.v.origin[2] < self->s.v.origin[2]);
}

static qbool BotSwimUp(gedict_t* self) {
	return (self->fb.linked_marker->s.v.origin[2] >= self->s.v.origin[2]);
}

static qbool BotGoUpForAir(gedict_t* self, vec3_t dir_move) {
	vec3_t temp;

	if (g_globalvars.time > (self->air_finished - BOT_DROWN_SAFETY_TIME)) {
		vec3_t new_velocity;

		traceline(self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2], self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] + 64, true, self);
		if (g_globalvars.trace_fraction == 1) {
			return (self->fb.swim_arrow = UP);
		}

		VectorCopy(self->s.v.velocity, new_velocity);
		VectorNormalize(dir_move);
		VectorCopy(new_velocity, temp);
		VectorNormalize(temp);
		VectorAdd(dir_move, temp, dir_move);
		dir_move[2] = 0;
		NewVelocityForArrow(self, dir_move, "UpForAir");

		// Drowning...
		if (g_globalvars.time > self->air_finished) {
			traceline(self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2], self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] + 32, true, self);
			if (g_globalvars.trace_fraction != 1) {
				return (self->fb.swim_arrow = UP);
			}
		}
	}
	return false;
}

static void SwimAwayFromWall(gedict_t* self, vec3_t dir_move) {
	if (DotProduct(self->fb.obstruction_normal, self->fb.obstruction_direction) > 0.5) {
		VectorScale(dir_move, -1, dir_move);
	}
	traceline(self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2], self->s.v.origin[0] + g_globalvars.v_right[0] * 20, self->s.v.origin[1] + g_globalvars.v_right[1] * 20, self->s.v.origin[2] + g_globalvars.v_right[2] * 20, true, self);
	if (g_globalvars.trace_fraction != 1) {
		vec3_t temp;

		VectorNormalize(dir_move);
		VectorScale(g_globalvars.v_right, g_random() * -32, temp);
		VectorAdd(temp, dir_move, temp);
		normalize(temp, dir_move);
	}
	traceline(self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2], self->s.v.origin[0] + g_globalvars.v_right[0] * (-20), self->s.v.origin[1] + g_globalvars.v_right[1] * (-20), self->s.v.origin[2] + g_globalvars.v_right[2] * (-20), true, self);
	if (g_globalvars.trace_fraction != 1) {
		vec3_t temp;

		VectorNormalize(dir_move);
		VectorScale(g_globalvars.v_right, g_random() * 32, temp);
		VectorAdd(temp, dir_move, temp);
		normalize(temp, dir_move);
	}
	NewVelocityForArrow(self, dir_move, "SwimAway");
}

void BotWaterMove(gedict_t* self) {
	vec3_t dir_move;

	//if ((self->s.v.watertype == CONTENT_LAVA || self->s.v.watertype == CONTENT_SLIME) && escape_marker_count > 0) {

	//}

	self->fb.swim_arrow = 0;
	if (self->s.v.waterlevel <= 2 || FUTURE(frogwatermove_time)) {
		return;
	}

	VectorCopy (self->fb.dir_move_, dir_move);
	self->fb.frogwatermove_time = self->fb.frogbot_nextthink + 0.1;
	if (self->fb.obstruction_normal[0] || self->fb.obstruction_normal[1] || self->fb.obstruction_normal[2]) {
		SwimAwayFromWall(self, dir_move);
	}

	if (BotGoUpForAir (self, dir_move)) {
		return;
	}

	if (BotCanReachMarker(self)) {
		if (BotSwimDown(self)) {
			self->fb.swim_arrow = DOWN;
		}
		else if (BotSwimUp(self)) {
			self->fb.swim_arrow = UP;
		}
	}
	else  {
		traceline(self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2], self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] + 32, true, self);
		if (g_globalvars.trace_fraction == 1) {
			self->fb.swim_arrow = UP;
		}
		else  {
			traceline(self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2], self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] + -32, true, self);
			if (g_globalvars.trace_fraction == 1) {
				self->fb.swim_arrow = DOWN;
			}
			else {
				self->fb.swim_arrow = UP;
			}
		}
	}
}

// client.qc
void BotWaterJumpFix() {
	if (self->isBot) {
		++self->fb.tread_water_count;
		if (self->fb.tread_water_count > 60) {
			self->fb.tread_water_count = 0;
			self->fb.old_linked_marker = NULL;
			SetLinkedMarker(self, LocateMarker(self->s.v.origin), "BotWaterJumpFix");
			self->fb.path_state = 0;
			self->fb.linked_marker_time = g_globalvars.time + 5;
		}
	}
}

