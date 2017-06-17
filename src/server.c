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

/*
==============================================================================

MOVETARGET CODE

The angle of the movetarget effects standing and bowing direction, but has no effect on movement, which allways heads to the next target.

targetname
must be present.  The name of this movetarget.

target
the next spot to move to.  If not present, stop here for good.

pausetime
The number of seconds to spend standing or bowing for path_stand or path_bow

==============================================================================
*/

/*
=============
t_movetarget

Something has bumped into a movetarget.  If it is a monster
moving towards it, change the next destination and continue.
==============
*/
void t_movetarget()
{
	gedict_t       *temp;
	vec3_t          tmpv;

	if ( other->movetarget != self )
		return;

	if ( other->s.v.enemy )
		return;		// fighting, not following a path

	temp = self;
	self = other;
	other = temp;

	if ( streq( self->classname, "monster_ogre" ) )
		sound( self, CHAN_VOICE, "ogre/ogdrag.wav", 1, ATTN_IDLE );	// play chainsaw drag sound

//dprint ("t_movetarget\n");
	if ( other->target )
		self->movetarget = find( world, FOFS( targetname ), other->target );
	else
		self->movetarget = NULL;

	if ( !self->movetarget ) // NOTE: this is a damn difference with qc
		self->movetarget = world;
	self->s.v.goalentity = EDICT_TO_PROG( self->movetarget );

	VectorSubtract( self->movetarget->s.v.origin, self->s.v.origin, tmpv );
	self->s.v.ideal_yaw = vectoyaw( tmpv );
	if ( !self->movetarget || self->movetarget == world )
	{
		self->pausetime = g_globalvars.time + 999999;
		if ( self->th_stand )
			self->th_stand(); //monster code
		return;
	}
}


void movetarget_f()
{
	if ( !self->targetname )
		G_Error( "monster_movetarget: no targetname" );

	self->s.v.solid = SOLID_TRIGGER;
	self->touch = ( func_t ) t_movetarget;
	setsize( self, -8, -8, -8, 8, 8, 8 );

}

/*QUAKED path_corner (0.5 0.3 0) (-8 -8 -8) (8 8 8)
Monsters will continue walking towards the next target corner.
*/
void SP_path_corner()
{
	movetarget_f();
}



//============================================================================
