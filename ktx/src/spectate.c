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
 *  $Id: spectate.c,v 1.1.1.1 2005/09/24 12:45:05 disconn3ct Exp $
 */

// spectate.c

#include "g_local.h"

#ifdef KTEAMS
//team
//shame about the fecking mess in the spectator code, ill structure the thing later (hopefully)
// /kK
// oldman: YOU NEVER DID!
// qqshka: FIXME: clean it up, someday %)

void AdminImpBot();

void SMakeMOTD();
void ExitKick(gedict_t *kicker);

void TrackNext()
{
	gedict_t *p;

	if ( !PROG_TO_EDICT( self->s.v.goalentity ) )
		G_Error ("TrackNext null");

	p = find(PROG_TO_EDICT( self->s.v.goalentity ), FOFCLSN, "player");
	while( p ) {
		if( p != self && !strnull( p->s.v.netname ) ) {
			self->s.v.goalentity = EDICT_TO_PROG( p );
			return;
		}

		p = find(p, FOFCLSN, "player");
	}

	p = find(world, FOFCLSN, "player");
	while( p ) {
		if( p != self && !strnull( p->s.v.netname ) ) {
			self->s.v.goalentity = EDICT_TO_PROG( p );
			return;
		}

		p = find(p, FOFCLSN, "player");
	}

	if ( !p ) {
		G_sprint(self, 2, "No target found ...\n");
		self->k_track = 2;
	}
}

void TrackNext2()
{
	gedict_t *p;

	if ( !PROG_TO_EDICT( self->s.v.goalentity ) )
		G_Error ("TrackNext2 null");

	p = find(PROG_TO_EDICT( self->s.v.goalentity ), FOFCLSN, "info_intermission");
	if( p ) {
		self->s.v.goalentity = EDICT_TO_PROG( p );
		return;
	}

	p = find(world, FOFCLSN, "info_intermission");
	if( p ) {
		self->s.v.goalentity = EDICT_TO_PROG( p );
		return;
	}

	G_sprint(self, 2, "No target found ...\n");

	self->k_track = 0;
}

void ShowCamHelp()
{
	G_sprint(self, 2, "éíðõìóå ²µ next spawnpoint (floatcam)\n"
					  "éíðõìóå ²¶ change camera mode\n"
					  "éíðõìóå ²· zoom out (trackcam)\n"
					  "éíðõìóå ²¸ zoom in  (trackcam)\n"
					  "éíðõìóå ²¹ toggle statusbar (trackcam)\n"
					  "use [jump] to change target\n");
}
#endif

////////////////
// GlobalParams:
// time
// self
// params
///////////////
void SpectatorConnect()
{
	Vip_ShowRights( self );

	if( match_in_progress != 2 || atoi( ezinfokey(world, "k_ann") ) )
		G_bprint( PRINT_MEDIUM, "Spectator %s entered the game\n", self->s.v.netname );

	self->s.v.goalentity = EDICT_TO_PROG( world );	// used for impulse 1 below

	// Added this in for kick code.
	self->s.v.classname = "spectator";

	self->k_track = 0;
	self->ready = 0;		//used to check if admin stuffing has been performed
	self->k_666 = 40;		//zoom value default for trackcam
	self->suicide_time = 1;	//user for statusbar on/off checking

	// Wait until you do stuffing
	SMakeMOTD();
}

////////////////
// GlobalParams:
// self
///////////////
void SpectatorDisconnect()
{
	if( match_in_progress != 2 || atoi( ezinfokey(world, "k_ann") ) )
		G_bprint( PRINT_MEDIUM, "Spectator %s  left the game\n", self->s.v.netname );

	self->s.v.classname = ""; // Cenobite, so we clear out any specs as they leave
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

    // Check to see if you kicking first so Camera code doesnt take over

	goal = PROG_TO_EDICT( self->s.v.goalentity );

	if( self->k_track == 1 
			&& ( strneq ( goal->s.v.classname, "player") || strnull( goal->s.v.netname) ) )
            self->k_track = 0;

	if( self->k_admin == 1 && self->s.v.impulse >= 1 && self->s.v.impulse <=9 ) 
	{
		AdminImpBot();

		self->s.v.impulse = 0;

	} else if( self->s.v.impulse == 26 ) {
		self->s.v.goalentity = EDICT_TO_PROG( world );
		self->k_track += 1;

		if( self->k_track == 3 )
			self->k_track = 0;

		if( self->k_track == 1 ) {
			if( self->s.v.goalentity == EDICT_TO_PROG( world ) )
				TrackNext();

			if( self->k_track == 1 ) {
				G_sprint(self, 2, "--- Trackcam ---");
				if ( !(self->suicide_time) )
					G_sprint(self, 2, " : showing %s\n",
							PROG_TO_EDICT( self->s.v.goalentity )->s.v.netname);
				else
					G_sprint(self, 2, "\n");

				self->deaths = g_globalvars.time; 	//.deaths is used for spectators as a counter for centerprinted stuff
			}
		} else if( !self->k_track )
			G_sprint( self, 2, "--- Floatcam ---\n");

		if( self->k_track == 2 ) {
			if( self->s.v.goalentity == EDICT_TO_PROG( world ) )
				TrackNext2();

			if( self->k_track ) {
				G_sprint(self, 2, "--- Intermissioncam ---\n");
				self->deaths = g_globalvars.time;
			}
		}
	} else if( self->s.v.impulse == 27 && self->k_track == 1 ) {
		if( self->k_666 < 60 )
			self->k_666 += 10;

	} else if( self->s.v.impulse == 28 && self->k_track == 1 ) {
		if( self->k_666 > -10 )
			self->k_666 -= 10;

	} else if( self->s.v.impulse == 29 ) {
		self->suicide_time += 1;

		if( self->suicide_time == 3 )
			self->suicide_time = 0;

		if( !self->suicide_time ) {
			g_globalvars.msg_entity = EDICT_TO_PROG( self );
			WriteByte(1, 26);
			WriteString(1, "\n");
		}
	} else if( self->s.v.impulse == 25 && !(self->k_track) ) {
		// teleport the spectator to the next spawn point
		// note that if the spectator is tracking, this doesn't do
		// much
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
#ifdef KTEAMS
	if ( PROG_TO_EDICT( self->s.v.goalentity ) )
		self->s.v.effects = PROG_TO_EDICT( self->s.v.goalentity )->s.v.effects;
	else
		G_Error ("SpectatorThink null");
#endif

	if ( self->s.v.impulse )
		SpectatorImpulseCommand();
}
