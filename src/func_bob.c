#include "g_local.h"

// Stripped down port of dumptruckDS's func_bob.qc

// progsdump / Arcane Dimensions spawnflags
#define BOB_COLLISION      2
#define BOB_NONSOLID       4

// Dimension of the Machine spawnflags
// Not yet supported, uses different easing function, might be possible to detect.
// https://github.com/id-Software/quake-rerelease-qc/blob/main/quakec_mg1/func_bob.qc
#define FUNC_BOB_NONSOLID  1
#define FUNC_BOB_START_ON  2

static void func_bob_timer(void)
{
	vec3_t delta;

	// Has the cycle completed?
	if (self->endtime < g_globalvars.time)
	{
		// Setup bob cycle and half way point for slowdown
		self->endtime = g_globalvars.time + self->count;
		self->distance = g_globalvars.time + (self->count * 0.5f);

		// Flip direction of bmodel bob
		self->lefty = 1 - self->lefty;
		if (self->lefty < 1)
		{
			self->t_length = self->height;
		}
		else
		{
			self->t_length = -self->height;
		}

		// Always reset velocity at pivot
		SetVector(self->s.v.velocity, 0, 0, 0);
	}

	if (self->distance < g_globalvars.time)
	{
		// Slow down velocity (gradually)
		VectorScale(self->s.v.velocity, self->waitmin2, self->s.v.velocity);
	}
	else
	{
		// Speed up velocity (linear/exponentially)
		self->t_length *= self->waitmin;
		VectorScale(self->s.v.movedir, self->t_length, delta);
		VectorAdd(self->s.v.velocity, delta, self->s.v.velocity);
	}

	self->s.v.nextthink = self->s.v.ltime + 0.1f;
}

void SP_func_bob(void)
{
	// Non-solid bobs not implemented yet, remove for now to not block player.
	if ((int) self->s.v.spawnflags & (BOB_NONSOLID | FUNC_BOB_NONSOLID)) {
		ent_remove(self);
		return;
	}

	self->s.v.movetype = MOVETYPE_PUSH;
	self->s.v.solid = SOLID_BSP;

	setmodel(self, self->model);
	setsize(self, PASSVEC3(self->s.v.mins), PASSVEC3(self->s.v.maxs));

	SetMovedir();
	VectorNormalize(self->s.v.movedir);

	if (self->height <= 0)
	{
		self->height = 8; // Direction intensity
	}
	if (self->count < 1)
	{
		self->count = 2; // Direction switch timer
	}
	if (self->waitmin <= 0)
	{
		self->waitmin = 1; // Speed up
	}
	if (self->waitmin2 <= 0)
	{
		self->waitmin2 = 0.75f; // Slow down
	}
	if (self->delay < 0)
	{
		self->delay = g_random() + g_random() + g_random();
	}

	self->think = (func_t) func_bob_timer;
	self->s.v.nextthink = g_globalvars.time + 0.1 + self->delay;
}
