/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// vote.c: election functions by rc\sturm

#include "g_local.h"

float CountPlayers();
float CountRTeams();
void AdminForcePause();
void BeginPicking();

// AbortElect is used to terminate the voting
// Important if player to be elected disconnects or levelchange happens
void AbortElect()
{
	gedict_t *p;

// k_velect for election vote counter
	k_velect = 0;
	p = find(world, FOFCLSN, "player");
	while( p ) {
		if( !strnull( p->s.v.netname ) ) {
			p->k_vote2 = 0;

			if(p->k_admin == 1.5)
				p->k_admin = 0;
			if( p->k_captain > 10 ) {
				p->k_captain = 0;
				k_captains = floor( k_captains );
			}
		}

		p = find(p, FOFCLSN, "player");
	}

	p = find(world, FOFCLSN, "spectator");
	while( p )
	{
		if( !strnull( p->s.v.netname ) && p->k_admin == 1.5 )
			p->k_admin = 0;

		p = find(p, FOFCLSN, "spectator");
	}

	p = find(world, FOFCLSN, "electguard");
// Kill timeout checker entity
	if( p && streq( p->s.v.classname, "electguard" ) ) 
		ent_remove( p );
}

void ElectThink()
{
	G_bprint(2, "The voting has timed out.\n"
				"Election aborted\n");
	self->s.v.nextthink = -1;

	AbortElect();
}

void VoteAdmin()
{
	gedict_t *electguard;

// Check if voteadmin is allowed
	if( !atoi( ezinfokey( world, "k_allowvoteadmin" ) ) ) {
		G_sprint(self, 2, "Admin election is not allowed on this server.\n");
		return;
	}

// Can't allow election and code entering for the same person at the same time
	if( self->k_admin == 1 ) {
		G_sprint(self, 2, "Finish entering the code first\n");
		return;
	}

	if( self->k_admin == 2 ) {
		G_sprint(self, 2, "You are already an admin\n");
		return;
	}

	if( self->k_admin == 1.5 ) {
		G_bprint(2, "%s aborts election!\n", self->s.v.netname);
		AbortElect();
		return;
	}

// Only one election per server because otherwise we wouldn't know how to count
// "yes"s or "no"s
	if( k_velect ) {
		G_sprint(self, 2, "An election is already in progress\n");
		return;
	}

	if( !atoi( ezinfokey( world, "k_admins" ) ) ) {
		G_sprint(self, 2, "NO ADMINS ON THIS SERVER!\n");
		return;
	}

	if( streq( self->s.v.classname, "spectator" ) && match_in_progress )
		return;

	G_bprint(2, "%s has requested admin rights!\n"
				"Type yes in console to approve\n", self->s.v.netname);

	self->k_admin = 1.5;
	k_velect = 1;
	G_sprint(self, 2, "Type εμεγτ to abort election\n");

	electguard = spawn(); // Check the 1 minute timeout for election
	electguard->s.v.owner = EDICT_TO_PROG( world );
	electguard->s.v.classname = "electguard";
	electguard->s.v.think = ( func_t ) ElectThink;
	electguard->s.v.nextthink = g_globalvars.time + 60;
}

void VoteYes()
{
	float f1, f2;
	gedict_t *p;

	if( !k_velect )
		return;

	if( self->k_admin == 1.5 || self->k_captain > 10 ) {
		G_sprint(self, 2, "You cannot vote for yourself\n");
		return;
	}

	if( self->k_vote2 ) {
		G_sprint(self, 2, "--- your vote is still good ---\n");
		return;
	}

// register the vote
	k_velect++;

	G_bprint(2, "%s gives %s vote\n", self->s.v.netname,
				( streq( ezinfokey(self, "gender"), "f" ) ? "her" : "his" ) );

	f1 = CountPlayers();
	f2 = ( floor( f1 / 2 ) ) + 1;

	if( k_velect >= f2 ) {
		p = find(world, FOFCLSN, "player");
		while( p->k_admin != 1.5 && p->k_captain < 10 )
			p = find(p, FOFCLSN, "player");

		if(p->k_admin == 1.5) {
// s: election was admin election
			G_bprint(2, "%s ηαιξσ αδνιξ στατυσ!\n", p->s.v.netname);
			G_sprint(p, 2, "Please give up admin rights when you're done.\n"
						   "Type γονναξδσ for info\n");
			p->k_admin = 2;
		}
		if( p->k_captain > 10 ) {
// s: election was captain election
			p->k_captain -= 10;
			k_captains = floor(k_captains) + 1;
			G_bprint(2, "%s becomes a captain\n", p->s.v.netname);

// s: if both captains are already elected, start choosing players
			if( k_captains == 2 )
				BeginPicking();
			else
				G_bprint(2, "One more γαπταιξ should be elected\n");
		}

		AbortElect();
		return;
	}

// calculate how many more votes are needed
	self->k_vote2 = 1;
	f1 = f2 - k_velect;

	G_bprint(2, "%d‘ more vote%s needed\n", (int)f1, ( f1 > 1 ? "s" : "" ));
}

void VoteNo()
{
	float f1, f2;

// withdraw one's vote
	if( !k_velect || self->k_admin == 1.5 || !self->k_vote2 )
		return;

	G_bprint(2, "%s withdraws %s vote\n", self->s.v.netname,
					( streq( ezinfokey(self, "gender"), "f" ) ? "her" : "his" ) );

	self->k_vote2 = 0;
	k_velect--;

	f1 = CountPlayers();
	f2 = ( floor( f1 / 2 ) ) + 1;
	f1 = f2 - k_velect;

	G_bprint(2, "%d‘ more vote%s needed\n", (int)f1, ( f1 > 1 ? "s" : "" ));
}

