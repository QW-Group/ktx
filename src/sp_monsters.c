/*  Copyright (C) 1996-1997  Id Software, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    See file, 'COPYING', for details.
*/

#include "g_local.h"

/* ALL MONSTERS SHOULD BE 1 0 0 IN COLOR */

// name =[framenum,	nexttime, nextthink] {code}
// expands to:
// name ()
// {
//		self.frame=framenum;
//		self.nextthink = time + nexttime;
//		self.think = nextthink
//		<code>
// }


/*
================
monster_use

Using a monster makes it angry at the current activator
================
*/
void monster_use ()
{
	if ( self->s.v.enemy )
		return;

	if ( ISDEAD( self ) )
		return;

	if ( activator->ct != ctPlayer )
		return;
		
	if ( (int)activator->s.v.items & IT_INVISIBILITY )
		return;

	if ( (int)activator->s.v.flags & FL_NOTARGET )
		return;

	// delay reaction so if the monster is teleported, its sound is still heard
	self->s.v.enemy = EDICT_TO_PROG( activator );
	self->s.v.nextthink = g_globalvars.time + 0.1;
	self->s.v.think = ( func_t ) FoundTarget;
}

/*
================
monster_death_use

When a mosnter dies, it fires all of its targets with the current
enemy as activator.
================
*/
void monster_death_use ()
{
	if ( !( ( int )self->s.v.flags & FL_MONSTER ) )
		return; // not a monster

	// fall to ground
	self->s.v.flags = (int)self->s.v.flags & ~( FL_FLY | FL_SWIM);

	if ( !self->s.v.target )
		return;

	activator = PROG_TO_EDICT( self->s.v.enemy );
	SUB_UseTargets ();
}


//============================================================================

typedef enum
{
	mtWalk,
	mtFly,
	mtSwim,
} monsterType_t;

void monster_start_go( monsterType_t mt )
{
	vec3_t tmpv;

	if ( mt == mtWalk )
	{
		self->s.v.origin[2] += 1;	// raise off floor a bit
		droptofloor( self );
	}

	VectorSet(tmpv, 0, 1, 0);
	self->s.v.ideal_yaw = DotProduct( self->s.v.angles, tmpv );
	if ( !self->s.v.yaw_speed )
	{
		self->s.v.yaw_speed = ( mt == mtWalk ? 20 : 10 );
	}

	VectorSet( self->s.v.view_ofs, 0, 0, mt == mtSwim ? 10 : 25 );
	self->s.v.use = ( func_t ) monster_use;
	self->s.v.takedamage = DAMAGE_AIM;

	if ( !walkmove( self, 0, 0 ) )
	{
		G_bprint( 2, "monster in wall at: %.1f %.1f %.1f\n", self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] );
	}

	if ( self->s.v.target )
	{
		self->movetarget = find( world, FOFS( s.v.targetname ), self->s.v.target );
		if ( !self->movetarget ) // NOTE: this is a damn difference with qc
			self->movetarget = world;
		self->s.v.goalentity = EDICT_TO_PROG( self->movetarget );

		VectorSubtract( PROG_TO_EDICT( self->s.v.goalentity )->s.v.origin, self->s.v.origin, tmpv );
		self->s.v.ideal_yaw = vectoyaw( tmpv );

		if ( !self->movetarget || self->movetarget == world )
		{
			G_bprint( 2, "monster can't find target at: %.1f %.1f %.1f\n", self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] );
		}
		// this used to be an objerror

		if ( self->movetarget && streq( self->movetarget->s.v.classname, "path_corner" ) )
		{
			if ( self->th_walk )
				self->th_walk();
		}
		else
		{
			self->pausetime = 99999999;
			if ( self->th_stand )
				self->th_stand();
		}
	}
	else
	{
		self->pausetime = 99999999;
		if ( self->th_stand )
			self->th_stand();
	}

	// spread think times so they don't all happen at same time
	self->s.v.nextthink = g_globalvars.time + g_random() * 0.5;
}

void walkmonster_start_go()
{
	monster_start_go( mtWalk );
}

void flymonster_start_go()
{
	monster_start_go( mtFly );
}

void swimmonster_start_go()
{
	monster_start_go( mtSwim );
}

void walkmonster_start()
{
	self->s.v.flags = (int)self->s.v.flags | FL_MONSTER;

	// delay drop to floor to make sure all doors have been spawned
	// spread think times so they don't all happen at same time
	self->s.v.nextthink = g_globalvars.time + 0.1 + g_random() * 0.5;
	self->s.v.think = ( func_t ) walkmonster_start_go;

	g_globalvars.total_monsters += 1;
}

void flymonster_start()
{
	// set FL_FLY early so that we're not affected by gravity
	self->s.v.flags = (int)self->s.v.flags | FL_MONSTER | FL_FLY;

	// spread think times so they don't all happen at same time
	self->s.v.nextthink = g_globalvars.time + 0.1 + g_random() * 0.5;
	self->s.v.think = ( func_t ) flymonster_start_go;
	g_globalvars.total_monsters += 1;
}

void swimmonster_start()
{
	// set FL_SWIM early so that we're not affected by gravity
	self->s.v.flags = (int)self->s.v.flags | FL_MONSTER | FL_SWIM;

	// spread think times so they don't all happen at same time
	self->s.v.nextthink = g_globalvars.time + 0.1 + g_random() * 0.5;
	self->s.v.think = (func_t) swimmonster_start_go;
	g_globalvars.total_monsters += 1;
}
