#include "g_local.h"

#define START_OFF 1
#define LASER_SOLID 2

static void laser_helper_think(void)
{
	gedict_t *owner = PROG_TO_EDICT(self->s.v.owner);
	float alpha = trap_GetExtField_f(self, "alpha");

	if (!((int) owner->s.v.spawnflags & START_OFF))
	{
		trap_SetExtField_f(self, "alpha", alpha * 0.8f + alpha * g_random() * 0.4f);
	}

	self->s.v.nextthink = g_globalvars.time + 0.05f;
}

static void init_laser_noise(void)
{
	gedict_t *owner = PROG_TO_EDICT(self->s.v.owner);

	sound(owner, CHAN_VOICE, owner->noise, 1, ATTN_NORM);

	self->think = (func_t) laser_helper_think;
	self->s.v.nextthink = g_globalvars.time + 0.05f;
}

static void func_laser_touch(void)
{
	// from Copper -- dumptruck_ds
	if (other->s.v.movetype == MOVETYPE_NOCLIP)
	{
			return;
	}

	if (other->s.v.takedamage && self->attack_finished < g_globalvars.time)
	{
		T_Damage (other, self, self, self->dmg);
		// add "zap" sound when damage is dealt
		sound (self, CHAN_WEAPON, self->noise2, 1, ATTN_NORM);
		self->attack_finished = g_globalvars.time + 0.333f;
	}
}

static void func_laser_use(void)
{
	if ((int) self->s.v.spawnflags & START_OFF)
	{
		setorigin(self, 0, 0, 0);
		self->s.v.spawnflags = self->s.v.spawnflags - START_OFF;
		sound(self, CHAN_VOICE, self->noise, 1, ATTN_NORM);
	}
	else
	{
		sound (self, CHAN_VOICE, self->noise1, 1, ATTN_NORM);
		setorigin(self, 0, 0, 9000);
		self->s.v.spawnflags = self->s.v.spawnflags + START_OFF;
	}
};

/**
 * QUAKED func_laser (0 .5 .8) ? START_OFF LASER_SOLID X X X X X X NOT_ON_EASY NOT_ON_NORMAL NOT_ON_HARD_OR_NIGHTMARE NOT_IN_DEATHMATCH NOT_IN_COOP NOT_IN_SINGLEPLAYER X NOT_ON_HARD_ONLY NOT_ON_NIGHTMARE_ONLY
 * A toggleable laser, hurts to touch, can be used to block players
 * START_OFF: Laser starts off.
 * LASER_SOLID: Laser blocks movement while turned on.
 * Keys:
 * "dmg" damage to do on touch. default 1
 * "alpha" approximate alpha you want the laser drawn at. default 0.5. alpha will vary by 20% of this value.
 * "message" message to display when activated (not supported)
 * "message2" message to display when deactivated (not supported)
 */
void SP_func_laser(void)
{
	gedict_t *helper;
	float alpha;

	setmodel(self, self->model);

	trap_precache_sound ("buttons/switch02.wav");
	trap_precache_sound ("buttons/switch04.wav");
	trap_precache_sound ("misc/null.wav");

	if ((int) self->s.v.spawnflags & LASER_SOLID)
	{
		self->s.v.solid = SOLID_BSP; //so you can shoot between lasers in a single bmodel
		self->s.v.movetype = MOVETYPE_PUSH; //required becuase of SOLID_BSP
	}
	else
	{
		self->s.v.solid = SOLID_TRIGGER;
		self->s.v.movetype = MOVETYPE_NONE;
	}

	alpha = trap_GetExtField_f(self, "alpha");
	if (!alpha)
	{
		alpha = 0.5f;
	}
	trap_SetExtField_f(self, "alpha", alpha);

	if (!self->dmg)
	{
		self->dmg = 1;
	}

	self->use = (func_t) func_laser_use;
	self->touch = (func_t) func_laser_touch;

	if ((int) self->s.v.spawnflags & START_OFF)
	{
		setorigin(self, 0, 0, 9000);
	}
	else
	{
		setorigin(self, 0, 0, 0);
	}

	if (!strnull(self->noise))
	{
		trap_precache_sound(self->noise);
	}
	else
	{
		self->noise = "buttons/switch02.wav";
	}

	if (!strnull(self->noise1))
	{
		trap_precache_sound(self->noise1);
	}
	else
	{
		self->noise1 = "buttons/switch04.wav";
	}

	if (!strnull(self->noise2))
	{
		trap_precache_sound(self->noise2);
	}
	else
	{
		self->noise2 = "misc/null.wav";
	}

	//spawn a second entity to handle alpha changes, since MOVETYPE_PUSH doesn't support think functions
	helper = spawn();
	helper->s.v.owner = EDICT_TO_PROG(self);
	helper->s.v.nextthink = g_globalvars.time + 2;

	trap_SetExtField_f(helper, "alpha", alpha);

	helper->think = (func_t) init_laser_noise;
}
