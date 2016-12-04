// Converted from .qc on 05/02/2016

#include "g_local.h"
#include "fb_globals.h"

static float unstick_time = 0;
static qbool no_bots_stuck = 0;

int NumberOfClients (void)
{
	int count = 0;
	gedict_t* plr = NULL;

	for (plr = world; plr = find_plr (plr); ) {
		if (plr->ct == ctPlayer)
			++count;
	}

	return count;
}

void FrogbotPrePhysics1(void) {
	// Set all players to non-solid so we can avoid hazards
	if (IsHazardFrame()) {
		for (self = world; self = find_plr(self); ) {
			self->fb.oldsolid = self->s.v.solid;
			self->s.v.solid = SOLID_NOT;
		}
	}

	// 
	for (self = world; self = find_plr (self); ) {
		if (self->isBot && self->s.v.takedamage) {
			VectorCopy(self->s.v.velocity, self->fb.oldvelocity);
			if (IsHazardFrame()) {
				AvoidHazards(self);
			}
		}
	}

	// Re-instate client entity types
	if (IsHazardFrame()) {
		for (self = world; self = find_plr(self); ) {
			self->s.v.solid = self->fb.oldsolid;
		}
	}
}

void BotDetectTrapped(gedict_t* self) {
	// This tries to detect stuck bots, and fixes the situation by either jumping or committing suicide
	vec3_t point = { self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] - 24 };
	int content1 = trap_pointcontents(point[0], point[1], point[2]);
	if (content1 == CONTENT_EMPTY) {
		self->fb.oldwaterlevel = 0;
		self->fb.oldwatertype = CONTENT_EMPTY;
	}
	else if (content1 == CONTENT_SOLID) {
		unstick_time = unstick_time + g_globalvars.frametime;
		if (unstick_time <= NumberOfClients()) {
			no_bots_stuck = false;
			self->fb.jumping = true;
		}
		else  {
			self->fb.botchose = true;
			self->fb.next_impulse = CLIENTKILL;
		}
	}
	else {
		int content2 = trap_pointcontents(self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] + 4);
		if (content2 == CONTENT_EMPTY) {
			self->fb.oldwaterlevel = 1;
			self->fb.oldwatertype = content1;
		}
		else  {
			int content3 = trap_pointcontents(self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] + 22);
			if (content3 == CONTENT_EMPTY) {
				self->fb.oldwaterlevel = 2;
				self->fb.oldwatertype = content2;
			}
			else  {
				self->fb.oldwaterlevel = 3;
				self->fb.oldwatertype = content3;
			}
		}
	}
}

void FrogbotPrePhysics2() {
	no_bots_stuck = true;

	for (self = world; self = find_plr (self); ) {
		if (self->isBot) {
			BotDetectTrapped(self);

			if (self->s.v.takedamage) {
				VectorCopy(self->s.v.origin, self->s.v.oldorigin);
			}
		}
	}
	if (no_bots_stuck) {
		unstick_time = 0;
	}
}
