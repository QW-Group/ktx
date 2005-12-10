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
 *  $Id: spectate.c,v 1.4 2005/12/10 19:51:02 qqshka Exp $
 */

// spectate.c

#include "g_local.h"


void AdminImpBot();

void SMakeMOTD();
void ExitKick(gedict_t *kicker);


void ShowCamHelp()
{
	G_sprint(self, 2, "use %s %s to jump between spawn points\n"
					  "use [attack] to change cam mode\n"
					  "use [jump] to change target\n", redtext("impulse"), dig3(1));
}


////////////////
// GlobalParams:
// time
// self
// params
///////////////
void SpectatorConnect()
{
	Vip_ShowRights( self );

	if( match_in_progress != 2 || iKey(world, "k_ann") )
		G_bprint( PRINT_MEDIUM, "Spectator %s entered the game\n", self->s.v.netname );

	self->s.v.goalentity = EDICT_TO_PROG( world );	// used for impulse 1 below

	// Added this in for kick code.
	self->s.v.classname = "spectator";

	// Wait until you do stuffing
	SMakeMOTD();
}

////////////////
// GlobalParams:
// self
///////////////
void SpectatorDisconnect()
{
	if( match_in_progress != 2 || iKey(world, "k_ann") )
		G_bprint( PRINT_MEDIUM, "Spectator %s left the game\n", self->s.v.netname );

	self->s.v.classname = ""; // Cenobite, so we clear out any specs as they leave
	self->k_spectator = 0;

	if( self->k_kicking )
		ExitKick( self );
}

/*
================
SpectatorImpulseCommand

Called by SpectatorThink if the spectator entered an impulse
================
*/
void SpectatorImpulseCommand()
{
	gedict_t       *goal;

	goal = PROG_TO_EDICT( self->s.v.goalentity );

	if( self->k_admin == 1 && self->s.v.impulse >= 1 && self->s.v.impulse <=9 ) 
	{
		AdminImpBot();

	} else if( self->s.v.impulse == 1 ) {
		// teleport the spectator to the next spawn point
		// note that if the spectator is tracking, this doesn't do much
		goal = PROG_TO_EDICT( self->s.v.goalentity );
		goal = find( goal, FOFS( s.v.classname ), "info_player_deathmatch" );

		if ( !goal )
			goal = find( world, FOFS( s.v.classname ), "info_player_deathmatch" );

		if ( goal )
		{
			setorigin( self, PASSVEC3( goal->s.v.origin ) );
			VectorCopy( goal->s.v.angles, self->s.v.angles );
			self->s.v.fixangle = true;	// turn this way immediately
		} else
			goal = world;

		self->s.v.goalentity = EDICT_TO_PROG( goal );
	}

	self->s.v.impulse = 0;
}

////////////////
// GlobalParams:
// time
// self
///////////////
void SpectatorThink()
{
	if ( self->s.v.impulse )
		SpectatorImpulseCommand();
}
