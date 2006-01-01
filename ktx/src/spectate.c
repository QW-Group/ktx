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
 *  $Id: spectate.c,v 1.6 2006/01/01 15:33:51 qqshka Exp $
 */

// spectate.c

#include "g_local.h"


void AdminImpBot();

void SMakeMOTD();
void ExitKick(gedict_t *kicker);

float CountALLPlayers ();

int GetSpecWizard ()
{
	int k_asw = bound(0, cvar("allow_spec_wizard"), 2);

	if ( match_in_progress || intermission_running )
		return 0;

	switch ( k_asw ) {
		case 0:
				return 0; // wizards not allowed
		case 1:
				return (CountALLPlayers () ? 0 : 1); // allowed without players
		case 2:
				return 2; // allowed with players in prematch
	}

	return 0;
}


void ShowCamHelp()
{
	G_sprint(self, 2, "use %s %s to jump between spawn points\n"
					  "use [attack] to change cam mode\n"
					  "use [jump] to change target\n", redtext("impulse"), dig3(1));
}

void wizard_think()
{
	if ( !cvar("k_no_wizard_animation") ) // animate if allowed
		( self->s.v.frame )++;

	if ( self->s.v.frame > 14 || self->s.v.frame < 0 )
		self->s.v.frame = 0;

	self->s.v.nextthink = g_globalvars.time + 0.1;
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
		G_bprint( PRINT_HIGH, "Spectator %s entered the game\n", self->s.v.netname );

	self->s.v.goalentity = EDICT_TO_PROG( world );	// used for impulse 1 below

	// Added this in for kick code.
	self->s.v.classname = "spectator";

	if ( match_in_progress != 2 ) {
		self->wizard = spawn();
		self->wizard->s.v.classname = "spectator_wizard";
		self->wizard->s.v.think = ( func_t ) wizard_think;
		self->wizard->s.v.nextthink = g_globalvars.time + 0.1;
	}

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
		G_bprint( PRINT_HIGH, "Spectator %s left the game\n", self->s.v.netname );

	if ( self->wizard ) {
		ent_remove( self->wizard );
		self->wizard = NULL;
	}

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
	gedict_t *wizard = self->wizard;

	if ( self->s.v.impulse )
		SpectatorImpulseCommand();

	if ( wizard ) {
		// set model angles
		wizard->s.v.angles[0] = -self->s.v.v_angle[0] / 2;
		wizard->s.v.angles[1] = self->s.v.v_angle[1];
        // wizard model blinking at spectator screen - so move model behind spec camera a bit		
		makevectors( self->s.v.v_angle );
		VectorMA (self->s.v.origin, -32, g_globalvars.v_forward, wizard->s.v.origin);
		// model bobbing
		wizard->s.v.origin[2] += sin( g_globalvars.time * 2.5 );
		setorigin( wizard, PASSVEC3( wizard->s.v.origin ) );

		if ( GetSpecWizard () ) {
			gedict_t *goal = PROG_TO_EDICT( self->s.v.goalentity );

			if ( goal && goal->k_player ) // tracking player, so turn model off
				wizard->s.v.model = "";
			else // turn model on
				setmodel( wizard, "progs/wizard.mdl" );
		}
		else {
			wizard->s.v.model = ""; // turn model off
		}
	}
}

void remove_specs_wizards ()
{
	gedict_t *p;

	for( p = world; p = find( p, FOFCLSN, "spectator" ); )
		if ( p->wizard ) {
			ent_remove( p->wizard );
			p->wizard = NULL;
		}
}

void hide_specs_wizards ()
{
	gedict_t *p;

	for( p = world; p = find( p, FOFCLSN, "spectator_wizard" ); )
		p->s.v.model = "";
}

void show_specs_wizards ()
{
	gedict_t *p;

	for( p = world; p = find( p, FOFCLSN, "spectator_wizard" ); )
		setmodel (p, "progs/wizard.mdl");
}

void FixSpecWizards ()
{
	static int k_asw = -1; // static

	qboolean changed   = false;
	int 	 k_asw_new = GetSpecWizard ();

	if( k_asw != k_asw_new || framecount == 1 ) { // force on first frame
		changed = true;
		k_asw = k_asw_new;
	}

	if ( changed ) {
		if ( k_asw )
			show_specs_wizards ();
		else
			hide_specs_wizards ();
	}
}

