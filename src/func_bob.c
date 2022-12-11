#include "g_local.h"

// Stripped down port of dumptruckDS's func_bob.qc

static void func_bob_timer()
{
	vec3_t delta;

	// Has the cycle completed?
	if (self->endtime < g_globalvars.time)
	{
		// Setup bob cycle and half way point for slowdown
		self->endtime = g_globalvars.time + self->count;
		self->distance = g_globalvars.time + (self->count * 0.5);

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

	self->s.v.nextthink = self->s.v.ltime + 0.1;
}

void SP_func_bob()
{
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
		self->waitmin2 = 0.75; // Slow down
	}
	if (self->delay < 0)
	{
		self->delay = g_random() + g_random() + g_random();
	}

	self->think = (func_t) func_bob_timer;
	self->s.v.nextthink = g_globalvars.time + 0.1 + self->delay;
}
