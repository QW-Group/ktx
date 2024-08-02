/*
 *  QWProgs-DM
 *  Copyright (C) 2004  [sd] angel
 *
 *  This code is based on QuakeWorld DM mod code by Id Software, Inc.
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *  $Id$
 */

#include "g_local.h"

void plat_center_touch(void);
void plat_outside_touch(void);
void plat_trigger_use(void);
void plat_go_up(void);
void plat_go_down(void);
void plat_crush(void);

#define PLAT_LOW_TRIGGER 1

void plat_spawn_inside_trigger(void)
{
	gedict_t *trigger;
	vec3_t tmin, tmax;

//
// middle trigger
// 
	trigger = spawn();
	trigger->touch = (func_t) plat_center_touch;
	trigger->s.v.movetype = MOVETYPE_NONE;
	trigger->s.v.solid = SOLID_TRIGGER;
	trigger->s.v.enemy = EDICT_TO_PROG(self);

	tmin[0] = self->s.v.mins[0] + 25;
	tmin[1] = self->s.v.mins[1] + 25;
	tmin[2] = self->s.v.mins[2] + 0;

	tmax[0] = self->s.v.maxs[0] - 25;
	tmax[1] = self->s.v.maxs[1] - 25;
	tmax[2] = self->s.v.maxs[2] + 8;

	tmin[2] = tmax[2] - (self->pos1[2] - self->pos2[2] + 8);

	if ((int)(self->s.v.spawnflags) & PLAT_LOW_TRIGGER)
		tmax[2] = tmin[2] + 8;

	if (self->s.v.size[0] <= 50)
	{
		tmin[0] = (self->s.v.mins[0] + self->s.v.maxs[0]) / 2;
		tmax[0] = tmin[0] + 1;
	}

	if (self->s.v.size[1] <= 50)
	{
		tmin[1] = (self->s.v.mins[1] + self->s.v.maxs[1]) / 2;
		tmax[1] = tmin[1] + 1;
	}

	setsize(trigger, PASSVEC3(tmin), PASSVEC3(tmax));
}

void plat_hit_top(void)
{
	sound(self, CHAN_NO_PHS_ADD + CHAN_VOICE, self->noise1, 1, ATTN_NORM);
	self->state = STATE_TOP;
	self->think = (func_t) plat_go_down;
	self->s.v.nextthink = self->s.v.ltime + 3;

#ifdef BOT_SUPPORT
	if (bots_enabled())
	{
		BotEventPlatformHitTop(self);
	}
#endif
}

void plat_hit_bottom(void)
{
	sound(self, CHAN_NO_PHS_ADD + CHAN_VOICE, self->noise1, 1, ATTN_NORM);
	self->state = STATE_BOTTOM;

#ifdef BOT_SUPPORT
	if (bots_enabled())
	{
		BotEventPlatformHitBottom(self);
	}
#endif
}

void plat_go_down(void)
{
	sound(self, CHAN_VOICE, self->noise, 1, ATTN_NORM);
	self->state = STATE_DOWN;
	SUB_CalcMove(self->pos2, self->speed, plat_hit_bottom);
}

void plat_go_up(void)
{
	sound(self, CHAN_VOICE, self->noise, 1, ATTN_NORM);
	self->state = STATE_UP;
	SUB_CalcMove(self->pos1, self->speed, plat_hit_top);
}

void plat_center_touch(void)
{
	// return if countdown or map frozen
	if (!k_practice) // #practice mode#
	{
		if ((match_in_progress == 1) || (!match_in_progress && cvar("k_freeze")))
		{
			return;
		}
	}

	if (other->ct != ctPlayer)
	{
		return;
	}

	if (ISDEAD(other))
	{
		return;
	}

	self = PROG_TO_EDICT(self->s.v.enemy);
#ifdef BOT_SUPPORT
	BotPlatformTouched(self, other);
#endif

	if (self->state == STATE_BOTTOM)
	{
		plat_go_up();
	}
	else if (self->state == STATE_TOP)
	{
		self->s.v.nextthink = self->s.v.ltime + 1;	// delay going down
	}
}

void plat_outside_touch(void)
{
	// return if countdown or map frozen
	if (!k_practice) // #practice mode#
	{
		if ((match_in_progress == 1) || (!match_in_progress && cvar("k_freeze")))
		{
			return;
		}
	}

	if (other->ct != ctPlayer)
	{
		return;
	}

	if (ISDEAD(other))
	{
		return;
	}

//dprint ("plat_outside_touch\n");
	self = PROG_TO_EDICT(self->s.v.enemy);
	if (self->state == STATE_TOP)
	{
		plat_go_down();
	}
}

void plat_trigger_use(void)
{
	if (self->think)
	{
		return;		// already activated
	}

	plat_go_down();
}

void plat_crush(void)
{
//dprint ("plat_crush\n");

	other->deathtype = dtSQUISH;
	T_Damage(other, self, self, 1);

	if (self->state == STATE_UP)
	{
		plat_go_down();
	}
	else if (self->state == STATE_DOWN)
	{
		plat_go_up();
	}
	else
	{
		G_Error("plat_crush: bad self.state\n");
	}
}

void plat_use(void)
{
	self->use = (func_t) SUB_Null;
	if (self->state != STATE_UP)
	{
		G_Error("plat_use: not in up state");
	}

	plat_go_down();
}

/*QUAKED func_plat (0 .5 .8) ? PLAT_LOW_TRIGGER
 speed default 150

 Plats are always drawn in the extended position, so they will light correctly.

 If the plat is the target of another trigger or button, it will start out disabled in the extended position until it is trigger, when it will lower and become a normal plat.

 If the "height" key is set, that will determine the amount the plat moves, instead of being implicitly determined by the model's height.
 Set "sounds" to one of the following:
 1) base fast
 2) chain slow
 */

void SP_func_plat(void)
{
//gedict_t* t;

	if (!self->t_length)
	{
		self->t_length = 80;
	}

	if (!self->t_width)
	{
		self->t_width = 10;
	}

	if (self->s.v.sounds == 0)
	{
		self->s.v.sounds = 2;
	}

	// FIX THIS TO LOAD A GENERIC PLAT SOUND
	if (self->s.v.sounds == 1)
	{
		trap_precache_sound("plats/plat1.wav");
		trap_precache_sound("plats/plat2.wav");
		self->noise = "plats/plat1.wav";
		self->noise1 = "plats/plat2.wav";
	}

	if (self->s.v.sounds == 2)
	{
		trap_precache_sound("plats/medplat1.wav");
		trap_precache_sound("plats/medplat2.wav");
		self->noise = "plats/medplat1.wav";
		self->noise1 = "plats/medplat2.wav";
	}

	VectorCopy(self->s.v.angles, self->mangle);
	//self->mangle = self->s.v.angles;
	SetVector(self->s.v.angles, 0, 0, 0);

	self->classname = "plat";
	self->s.v.solid = SOLID_BSP;
	self->s.v.movetype = MOVETYPE_PUSH;
	setorigin(self, PASSVEC3(self->s.v.origin));
	setmodel(self, self->model);
	setsize(self, PASSVEC3(self->s.v.mins), PASSVEC3(self->s.v.maxs));

	self->blocked = (func_t) plat_crush;
	if (!self->speed)
	{
		self->speed = 150;
	}

// pos1 is the top position, pos2 is the bottom
	VectorCopy(self->s.v.origin, self->pos1);
	VectorCopy(self->s.v.origin, self->pos2);
	//self->pos1 = self->s.v.origin;
	//self->pos2 = self->s.v.origin;
	if (self->height)
	{
		self->pos2[2] = self->s.v.origin[2] - self->height;
	}
	else
	{
		self->pos2[2] = self->s.v.origin[2] - self->s.v.size[2] + 8;
	}

	self->use = (func_t) plat_trigger_use;

	plat_spawn_inside_trigger();	// the "start moving" trigger 

	if (self->targetname)
	{
		self->state = STATE_UP;
		self->use = (func_t) plat_use;
	}
	else
	{
		setorigin(self, PASSVEC3(self->pos2));
		self->state = STATE_BOTTOM;
	}
}

//============================================================================

void train_next(void);
void funcref_train_find(void);

void train_blocked(void)
{
	if (g_globalvars.time < self->attack_finished)
	{
		return;
	}

	self->attack_finished = g_globalvars.time + 0.5;
	other->deathtype = dtSQUISH;
	T_Damage(other, self, self, self->dmg);
}

void train_use(void)
{
	if (self->think != (func_t) funcref_train_find)
	{
		return;		// already activated
	}

	train_next();
}

void train_wait(void)
{
	if (self->wait)
	{
		self->s.v.nextthink = self->s.v.ltime + self->wait;
		sound(self, CHAN_NO_PHS_ADD + CHAN_VOICE, self->noise, 1, ATTN_NORM);
	}
	else
	{
		self->s.v.nextthink = self->s.v.ltime + 0.1;
	}

	// make trains stop if frozen
	if ((match_in_progress == 2) || (!cvar("k_freeze") && !match_in_progress) || k_practice) // #practice mode#
	{
		self->think = (func_t) train_next;
	}
}

void train_next(void)
{
	gedict_t *targ;
	vec3_t tmpv;

	targ = find(world, FOFS(targetname), self->target);
	if (!targ)
	{
		G_Error("train_next: no next target");
	}

	self->target = targ->target;
	if (!self->target)
	{
		G_Error("train_next: no next target");
	}

	if (targ->wait)
	{
		self->wait = targ->wait;
	}
	else
	{
		self->wait = 0;
	}

	sound(self, CHAN_VOICE, self->noise1, 1, ATTN_NORM);
	VectorSubtract(targ->s.v.origin, self->s.v.mins, tmpv);
	SUB_CalcMove(tmpv, self->speed, train_wait);
}

void funcref_train_find(void)
{
	gedict_t *targ;

	targ = find(world, FOFS(targetname), self->target);
	if (!targ)
	{
		G_Error("funcref_train_find: no next target");
	}

// qqshka: i comment below line, this let us freeze level _right_ after spawn,
// 			of course if k_freeze is set, if u uncomment this, train will move one time,
//			even k_freeze is set
//	self->target = targ->target;

	setorigin(self, targ->s.v.origin[0] - self->s.v.mins[0],
				targ->s.v.origin[1] - self->s.v.mins[1], targ->s.v.origin[2] - self->s.v.mins[2]);
	if (!self->targetname)
	{			// not triggered, so start immediately
		self->s.v.nextthink = self->s.v.ltime + 0.1;
		self->think = (func_t) train_next;
	}
}

/*QUAKED funcref_train (0 .5 .8) ?
 Trains are moving platforms that players can ride.
 The targets origin specifies the min point of the train at each corner.
 The train spawns at the first target it is pointing at.
 If the train is the target of a button or trigger, it will not begin moving until activated.
 speed default 100
 dmg  default 2
 sounds
 1) ratchet metal

 */
void SP_func_train(void)
{
	if (!self->speed)
	{
		self->speed = 100;
	}

	if (!self->target)
	{
		G_Error("funcref_train without a target");
	}

	if (!self->dmg)
	{
		self->dmg = 2;
	}

	if (self->s.v.sounds == 0)
	{
//		self->noise = ( "misc/null.wav" );
///		trap_precache_sound( "misc/null.wav" );
//		self->noise1 = ( "misc/null.wav" );
//		trap_precache_sound( "misc/null.wav" );
// qqshka: shut up null.wav, have some artefact, we can hear
		self->noise = "";
		self->noise1 = "";
	}

	if (self->s.v.sounds == 1)
	{
		self->noise = ("plats/train2.wav");
		trap_precache_sound("plats/train2.wav");
		self->noise1 = ("plats/train1.wav");
		trap_precache_sound("plats/train1.wav");
	}

	self->cnt = 1;
	self->s.v.solid = SOLID_BSP;
	self->s.v.movetype = MOVETYPE_PUSH;
	self->blocked = (func_t) train_blocked;
	self->use = (func_t) train_use;
	self->classname = "train";

	setmodel(self, self->model);
	setsize(self, PASSVEC3(self->s.v.mins), PASSVEC3(self->s.v.maxs));
	setorigin(self, PASSVEC3(self->s.v.origin));

// start trains on the second frame, to make sure their targets have had
// a chance to spawn
	self->s.v.nextthink = self->s.v.ltime + 0.1;
	self->think = (func_t) funcref_train_find;
}

/*QUAKED misc_teleporttrain (0 .5 .8) (-8 -8 -8) (8 8 8)
 This is used for the final bos
 */
void SP_misc_teleporttrain(void)
{
	if (!self->speed)
	{
		self->speed = 100;
	}

	if (!self->target)
	{
		G_Error("funcref_train without a target");
	}

	self->cnt = 1;
	self->s.v.solid = SOLID_NOT;
	self->s.v.movetype = MOVETYPE_PUSH;
	self->blocked = (func_t) train_blocked;
	self->use = (func_t) train_use;
	SetVector(self->s.v.avelocity, 100, 200, 300);
	//self->s.v.avelocity = '100 200 300';

//	self->noise = ( "misc/null.wav" );
//	trap_precache_sound( "misc/null.wav" );
//	self->noise1 = ( "misc/null.wav" );
//	trap_precache_sound( "misc/null.wav" );
// qqshka: shut up null.wav, have some artefact, we can hear
	self->noise = "";
	self->noise1 = "";

	trap_precache_model("progs/teleport.mdl");
	setmodel(self, "progs/teleport.mdl");
	setsize(self, PASSVEC3(self->s.v.mins), PASSVEC3(self->s.v.maxs));
	setorigin(self, PASSVEC3(self->s.v.origin));

// start trains on the second frame, to make sure their targets have had
// a chance to spawn
	self->s.v.nextthink = self->s.v.ltime + 0.1;
	self->think = (func_t) funcref_train_find;
}
