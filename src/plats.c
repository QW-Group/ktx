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
#include "fb_globals.h"

void            plat_center_touch();
void            plat_outside_touch();
void            plat_trigger_use();
void            plat_go_up();
void            plat_go_down();
void            plat_crush();

#define PLAT_LOW_TRIGGER 1

void plat_spawn_inside_trigger()
{
	gedict_t       *trigger;
	vec3_t          tmin, tmax;

//
// middle trigger
// 
	trigger = spawn();
	trigger->s.v.touch = ( func_t ) plat_center_touch;
	trigger->s.v.movetype = MOVETYPE_NONE;
	trigger->s.v.solid = SOLID_TRIGGER;
	trigger->s.v.enemy = EDICT_TO_PROG( self );


	tmin[0] = self->s.v.mins[0] + 25;
	tmin[1] = self->s.v.mins[1] + 25;
	tmin[2] = self->s.v.mins[2] + 0;

	tmax[0] = self->s.v.maxs[0] - 25;
	tmax[1] = self->s.v.maxs[1] - 25;
	tmax[2] = self->s.v.maxs[2] + 8;

	tmin[2] = tmax[2] - ( self->pos1[2] - self->pos2[2] + 8 );

	if ( ( int ) ( self->s.v.spawnflags ) & PLAT_LOW_TRIGGER )
		tmax[2] = tmin[2] + 8;

	if ( self->s.v.size[0] <= 50 )
	{
		tmin[0] = ( self->s.v.mins[0] + self->s.v.maxs[0] ) / 2;
		tmax[0] = tmin[0] + 1;
	}

	if ( self->s.v.size[1] <= 50 )
	{
		tmin[1] = ( self->s.v.mins[1] + self->s.v.maxs[1] ) / 2;
		tmax[1] = tmin[1] + 1;
	}

	setsize( trigger, PASSVEC3( tmin ), PASSVEC3( tmax ) );
}

void plat_hit_top()
{
	sound( self, CHAN_NO_PHS_ADD + CHAN_VOICE, self->s.v.noise1, 1, ATTN_NORM );
	self->state = STATE_TOP;
	self->s.v.think = ( func_t ) plat_go_down;
	self->s.v.nextthink = self->s.v.ltime + 3;

	if (bots_enabled ())
		BotEventPlatformHitTop (self);
}

void plat_hit_bottom()
{
	sound( self, CHAN_NO_PHS_ADD + CHAN_VOICE, self->s.v.noise1, 1, ATTN_NORM );
	self->state = STATE_BOTTOM;

	if (bots_enabled ())
		BotEventPlatformHitBottom (self);
}

void plat_go_down()
{
	sound( self, CHAN_VOICE, self->s.v.noise, 1, ATTN_NORM );
	self->state = STATE_DOWN;
	SUB_CalcMove( self->pos2, self->speed, plat_hit_bottom );
}

void plat_go_up()
{
	sound( self, CHAN_VOICE, self->s.v.noise, 1, ATTN_NORM );
	self->state = STATE_UP;
	SUB_CalcMove( self->pos1, self->speed, plat_hit_top );
}

void plat_center_touch()
{
    // return if countdown or map frozen
	if( !k_practice ) // #practice mode#
	if( match_in_progress == 1
		|| ( !match_in_progress && cvar( "k_freeze" ) )
	  )
		return;

	if ( other->ct != ctPlayer )
		return;

	if ( ISDEAD( other ) )
		return;

	self = PROG_TO_EDICT( self->s.v.enemy );
	BotPlatformTouched (self, other);

	if ( self->state == STATE_BOTTOM )
		plat_go_up();

	else if ( self->state == STATE_TOP )
		self->s.v.nextthink = self->s.v.ltime + 1;	// delay going down
}

void plat_outside_touch()
{
    // return if countdown or map frozen
	if( !k_practice ) // #practice mode#
	if( match_in_progress == 1
		|| ( !match_in_progress && cvar( "k_freeze" ) )
	  )
		return;

	if ( other->ct != ctPlayer )
		return;

	if ( ISDEAD( other ) )
		return;

//dprint ("plat_outside_touch\n");
	self = PROG_TO_EDICT( self->s.v.enemy );
	if ( self->state == STATE_TOP )
		plat_go_down();
}

void plat_trigger_use()
{
	if ( self->s.v.think )
		return;		// allready activated
	plat_go_down();
}


void plat_crush()
{
//dprint ("plat_crush\n");

	other->deathtype = dtSQUISH;
	T_Damage( other, self, self, 1 );

	if ( self->state == STATE_UP )
		plat_go_down();
	else if ( self->state == STATE_DOWN )
		plat_go_up();
	else
		G_Error( "plat_crush: bad self.state\n" );
}

void plat_use()
{
	self->s.v.use = ( func_t ) SUB_Null;
	if ( self->state != STATE_UP )
		G_Error( "plat_use: not in up state" );

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


void SP_func_plat()
{
//gedict_t* t;

	if ( !self->t_length )
		self->t_length = 80;

	if ( !self->t_width )
		self->t_width = 10;

	if ( self->s.v.sounds == 0 )
		self->s.v.sounds = 2;
// FIX THIS TO LOAD A GENERIC PLAT SOUND

	if ( self->s.v.sounds == 1 )
	{
		trap_precache_sound( "plats/plat1.wav" );
		trap_precache_sound( "plats/plat2.wav" );
		self->s.v.noise = "plats/plat1.wav";
		self->s.v.noise1 = "plats/plat2.wav";
	}

	if ( self->s.v.sounds == 2 )
	{
		trap_precache_sound( "plats/medplat1.wav" );
		trap_precache_sound( "plats/medplat2.wav" );
		self->s.v.noise = "plats/medplat1.wav";
		self->s.v.noise1 = "plats/medplat2.wav";
	}

	VectorCopy( self->s.v.angles, self->mangle );
	//self->mangle = self->s.v.angles;
	SetVector( self->s.v.angles, 0, 0, 0 );

	self->s.v.classname = "plat";
	self->s.v.solid = SOLID_BSP;
	self->s.v.movetype = MOVETYPE_PUSH;
	setorigin( self, PASSVEC3( self->s.v.origin ) );
	setmodel( self, self->s.v.model );
	setsize( self, PASSVEC3( self->s.v.mins ), PASSVEC3( self->s.v.maxs ) );

	self->s.v.blocked = ( func_t ) plat_crush;
	if ( !self->speed )
		self->speed = 150;

// pos1 is the top position, pos2 is the bottom
	VectorCopy( self->s.v.origin, self->pos1 );
	VectorCopy( self->s.v.origin, self->pos2 );
	//self->pos1 = self->s.v.origin;
	//self->pos2 = self->s.v.origin;
	if ( self->height )
		self->pos2[2] = self->s.v.origin[2] - self->height;
	else
		self->pos2[2] = self->s.v.origin[2] - self->s.v.size[2] + 8;

	self->s.v.use = ( func_t ) plat_trigger_use;

	plat_spawn_inside_trigger();	// the "start moving" trigger 

	if ( self->s.v.targetname )
	{
		self->state = STATE_UP;
		self->s.v.use = ( func_t ) plat_use;
	} else
	{
		setorigin( self, PASSVEC3( self->pos2 ) );
		self->state = STATE_BOTTOM;
	}
}

//============================================================================

void            train_next();
void            func_train_find();

void train_blocked()
{
	if ( g_globalvars.time < self->attack_finished )
		return;

	self->attack_finished = g_globalvars.time + 0.5;
	other->deathtype = dtSQUISH;
	T_Damage( other, self, self, self->dmg );
}

void train_use()
{
	if ( self->s.v.think != ( func_t ) func_train_find )
		return;		// already activated

	train_next();
}

void train_wait()
{
	if ( self->wait )
	{
		self->s.v.nextthink = self->s.v.ltime + self->wait;
		sound( self, CHAN_NO_PHS_ADD + CHAN_VOICE, self->s.v.noise, 1,
			    ATTN_NORM );
	} else
		self->s.v.nextthink = self->s.v.ltime + 0.1;

    // make trains stop if frozen
	if( match_in_progress == 2
		|| ( !cvar( "k_freeze" ) && !match_in_progress )
		|| k_practice  // #practice mode#
	  )
		self->s.v.think = ( func_t ) train_next;
}

void train_next()
{
	gedict_t       *targ;
	vec3_t          tmpv;

	targ = find( world, FOFS( s.v.targetname ), self->s.v.target );
	if ( !targ )
		G_Error( "train_next: no next target" );

	self->s.v.target = targ->s.v.target;
	if ( !self->s.v.target )
		G_Error( "train_next: no next target" );

	if ( targ->wait )
		self->wait = targ->wait;
	else
		self->wait = 0;

	sound( self, CHAN_VOICE, self->s.v.noise1, 1, ATTN_NORM );
	VectorSubtract( targ->s.v.origin, self->s.v.mins, tmpv );
	SUB_CalcMove( tmpv, self->speed, train_wait );
}

void func_train_find()
{
	gedict_t       *targ;

	targ = find( world, FOFS( s.v.targetname ), self->s.v.target );
	if ( !targ )
		G_Error( "func_train_find: no next target" );

// qqshka: i comment below line, this let us freeze level _right_ after spawn,
// 			of course if k_freeze is set, if u uncomment this, train will move one time,
//			even k_freeze is set
//	self->s.v.target = targ->s.v.target;

	setorigin( self, targ->s.v.origin[0] - self->s.v.mins[0],
			targ->s.v.origin[1] - self->s.v.mins[1],
			targ->s.v.origin[2] - self->s.v.mins[2] );
	if ( !self->s.v.targetname )
	{			// not triggered, so start immediately
		self->s.v.nextthink = self->s.v.ltime + 0.1;
		self->s.v.think = ( func_t ) train_next;
	}
}

/*QUAKED func_train (0 .5 .8) ?
Trains are moving platforms that players can ride.
The targets origin specifies the min point of the train at each corner.
The train spawns at the first target it is pointing at.
If the train is the target of a button or trigger, it will not begin moving until activated.
speed default 100
dmg  default 2
sounds
1) ratchet metal

*/
void SP_func_train()
{
	if ( !self->speed )
		self->speed = 100;
	if ( !self->s.v.target )
		G_Error( "func_train without a target" );
	if ( !self->dmg )
		self->dmg = 2;

	if ( self->s.v.sounds == 0 )
	{
//		self->s.v.noise = ( "misc/null.wav" );
///		trap_precache_sound( "misc/null.wav" );
//		self->s.v.noise1 = ( "misc/null.wav" );
//		trap_precache_sound( "misc/null.wav" );
// qqshka: shut up null.wav, have some artefact, we can hear
		self->s.v.noise  = "";
		self->s.v.noise1 = "";
	}

	if ( self->s.v.sounds == 1 )
	{
		self->s.v.noise = ( "plats/train2.wav" );
		trap_precache_sound( "plats/train2.wav" );
		self->s.v.noise1 = ( "plats/train1.wav" );
		trap_precache_sound( "plats/train1.wav" );
	}

	self->cnt = 1;
	self->s.v.solid = SOLID_BSP;
	self->s.v.movetype = MOVETYPE_PUSH;
	self->s.v.blocked = ( func_t ) train_blocked;
	self->s.v.use = ( func_t ) train_use;
	self->s.v.classname = "train";

	setmodel( self, self->s.v.model );
	setsize( self, PASSVEC3( self->s.v.mins ), PASSVEC3( self->s.v.maxs ) );
	setorigin( self, PASSVEC3( self->s.v.origin ) );

// start trains on the second frame, to make sure their targets have had
// a chance to spawn
	self->s.v.nextthink = self->s.v.ltime + 0.1;
	self->s.v.think = ( func_t ) func_train_find;
}

/*QUAKED misc_teleporttrain (0 .5 .8) (-8 -8 -8) (8 8 8)
This is used for the final bos
*/
void SP_misc_teleporttrain()
{
	if ( !self->speed )
		self->speed = 100;
	if ( !self->s.v.target )
		G_Error( "func_train without a target" );

	self->cnt = 1;
	self->s.v.solid = SOLID_NOT;
	self->s.v.movetype = MOVETYPE_PUSH;
	self->s.v.blocked = ( func_t ) train_blocked;
	self->s.v.use = ( func_t ) train_use;
	SetVector( self->s.v.avelocity, 100, 200, 300 );
	//self->s.v.avelocity = '100 200 300';

//	self->s.v.noise = ( "misc/null.wav" );
//	trap_precache_sound( "misc/null.wav" );
//	self->s.v.noise1 = ( "misc/null.wav" );
//	trap_precache_sound( "misc/null.wav" );
// qqshka: shut up null.wav, have some artefact, we can hear
	self->s.v.noise  = "";
	self->s.v.noise1 = "";


	trap_precache_model( "progs/teleport.mdl" );
	setmodel( self, "progs/teleport.mdl" );
	setsize( self, PASSVEC3( self->s.v.mins ), PASSVEC3( self->s.v.maxs ) );
	setorigin( self, PASSVEC3( self->s.v.origin ) );

// start trains on the second frame, to make sure their targets have had
// a chance to spawn
	self->s.v.nextthink = self->s.v.ltime + 0.1;
	self->s.v.think = ( func_t ) func_train_find;
}
