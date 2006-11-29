/*
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
 *  $Id: captain.c,v 1.18 2006/11/29 06:47:17 qqshka Exp $
 */

// captain.c

#include "g_local.h"

// Captain functions introduced by rc\sturm
// here we use k_captains for storing the number of captains.
// one elected captain is worth 1, player being elected is worth 0.5 :)

// Check if picking should be finished

// pick the player

static int turn_number;

void CancelCaptains ();

int capt_num(gedict_t *p)
{
    if( p->k_captain == 1 || p->k_captain == 2 )
		return p->k_captain;

	return 0;
}

void SetPlayerParams (gedict_t *p, gedict_t *cap)
{
    char *infoteam, *infocolor;

	infoteam  = cvar_string( va("_k_captteam%d", (int)k_captainturn) );
	infocolor = cvar_string( va("_k_captcolor%d", (int)k_captainturn) );
	if ( turn_number != 1 ) // captains turn rules: c1 c2 c2 c1 c2 c1 c2 etc... i.e. second captain pick twice at beginning
		k_captainturn = (k_captainturn == 1 ? 2 : 1);
	turn_number++;

    G_bprint(2, "%s set to team \x90%s\x91\n", p->s.v.netname, infoteam);

    G_sprint(p, 2, "You were picked by %s\n"
				   "Time to go ready\n", cap->s.v.netname );

    stuffcmd(p, "break\n"
				"team \"%s\"\n"
				"color \"%s\"\n", infoteam, infocolor);

    p->s.v.frags = 0;
    p->k_picked = capt_num( cap );
}

void PrintCaptainInTurn ()
{
    gedict_t *p;

    for( p = world; (p = find(p, FOFCLSN, "player")) && capt_num( p ) != k_captainturn; )
        ; // empty

	if ( p )
		G_bprint(2, "%s is picking\n", p->s.v.netname);
	else {
		G_bprint(2, "PrintCaptainInTurn: captain not found\n");
		CancelCaptains ();
	}
}

void CancelCaptains ()
{
    gedict_t *p;

	k_captains = 0;

	for( p = world; (p = find(p, FOFCLSN, "player")); )
	{
		if( capt_num( p ) )
			G_sprint(p, 2, "You are no longer a %s\n", redtext("captain"));

		p->k_captain = p->k_picked = 0;

		if ( is_elected(p, etCaptain) ) // just for sanity
			AbortElect();
	}
}


void CheckFinishCaptain ()
{
    int pl_free = 0;
    gedict_t *p, *lastone = NULL;

    // s: calculate how many players are free
   	for( p = world; (p = find(p, FOFCLSN, "player")); )
    {
        if( p->s.v.frags )
        {
            lastone = p;
            pl_free++;
        }
    }

    if( pl_free == 1 ) // one free player left
    {
    	for( p = world; (p = find(p, FOFCLSN, "player")) && capt_num( p ) != k_captainturn; )
        	; // empty

		if ( p )
        	SetPlayerParams( lastone, p );
		else
        	G_bprint(2, "CheckFinishCaptain: captain not found\n");

        pl_free = 0;
    }

    if( !pl_free ) {
        G_bprint(2, "All players chosen. Captain modes exited\n");
		CancelCaptains ();
	}
}

void CaptainPickPlayer ()
{
    gedict_t *p;

    if( capt_num( self ) != k_captainturn )
    {
        G_sprint(self, 2, "It's %s your turn\n", redtext("not"));
        return;
    }

    for( p = world; (p = find(p, FOFCLSN, "player")) && p->s.v.frags != self->s.v.impulse; )
        ; // empty

    if( !p )
    {
        G_sprint(self, 2, "No such player. Pick another one\n");
        return;
    }

    SetPlayerParams( p, self );

    CheckFinishCaptain();

    if( k_captains )
        PrintCaptainInTurn();
}

void ExitCaptain ()
{
    gedict_t *p;

    if( !capt_num( self ) )
		return;

	self->k_captain = 0;

	if( k_captains == 2 )
	{
    	G_bprint(2, "Player picking aborted\n");

    	for( p = world; (p = find(p, FOFCLSN, "player")); )
        	if( p->s.v.frags )
            	p->s.v.frags = 0;
	}

	k_captains--;

	G_bprint(2, "%s captain present\n", (floor( k_captains ) ? "\x90\x31\x91" : redtext("No")));
}

void VoteCaptain ()
{
	int from, till;
    gedict_t *p, *electguard;

    // s: check if we are being elected or we are a captain already
	if( is_elected(self, etCaptain) )
	{
    	G_bprint(2,  "%s %s!\n", self->s.v.netname, redtext("aborts election"));

    	AbortElect();
		return;
	}

	if ( capt_num( self ) )
	{
    	G_bprint(2, "%s is no longer a %s\n", self->s.v.netname, redtext("captain"));

	    ExitCaptain();
		return;
	}

    if( match_in_progress || intermission_running )
        return;

    if( !isTeam() && !isCTF() )
    {
        G_sprint(self, 2, "No team picking in non team mode\n");
        return;
    }

    if( CountPlayers() < 3 )
    {
        G_sprint(self, 2, "Not enough players present\n");
        return;
    }

    if( k_captains == 2 )
    {
        G_sprint(self, 2, "Only 2 captains are allowed\n");
        return;
    }

    // s: no captain election if any election in progress
    if( get_votes( OV_ELECT ) )
    {
        G_sprint(self, 2, "An election is already in progress\n");
        return;
    }

	if( (till = Q_rint( self->v.elect_block_till - g_globalvars.time)) > 0  ) {
		G_sprint(self, 2, "Wait %d second%s!\n", till, count_s(till) );
		return;
	}

    // s: no captains with team ""
    if( strnull( getteam( self ) ) )
    {
        G_sprint(self, 2, "Set your team name first\n");
        return;
    }

    // must be red or blue in ctf
    if ( isCTF() )
    {
      if ( !streq(getteam(self), "blue") && !streq(getteam(self), "red") )
      {
        G_sprint( self, 2, "Must be team red or blue for ctf\n" );
        return;
      }
      streq(getteam(self), "blue") ? stuffcmd( self, "color 13\n" ) : stuffcmd( self, "color 4\n" );
    }

    // search if a captain already has the same team
    for( p = world; (p = find(p, FOFCLSN, "player")) && !capt_num( p ); )
		; // empty

	if ( p ) {

    	if( streq( getteam( self ), getteam( p ) ) )
    	{
        	G_sprint(self, 2, "A %s with team \x90%s\x91 already exists\n"
							  "Choose a new team name\n", redtext("captain"), getteam( p ));
        	return;
    	}

    	// check if a captain already has the same colors
    	if(    streq( ezinfokey(self, "topcolor"),    ezinfokey(p, "topcolor") ) 
			&& streq( ezinfokey(self, "bottomcolor"), ezinfokey(p, "bottomcolor"))
		  )
    	{
        	G_sprint(self, 2, "A %s with your color already exists\n"
							  "Change to new color\n", redtext("captain"));
        	return;
    	}
	}

    // announce the election
	self->v.elect = 1;
	self->v.elect_type = etCaptain;	

    k_captains += 0.5;

	G_bprint(2, "%s has %s status!\n", self->s.v.netname, redtext("requested captain"));

	for( from = 0, p = world; (p = find_plrspc(p, &from)); )
		if ( p != self && p->ct == ctPlayer )
			G_sprint(p, 2, "Type %s in console to approve\n", redtext("yes"));

    G_sprint(self, 2, "Type %s to abort election\n", redtext("captain"));

    // s: spawn the cool dude
    electguard = spawn(); // Check the 1 minute timeout for election
    electguard->s.v.owner = EDICT_TO_PROG( world );
    electguard->s.v.classname = "electguard";
    electguard->s.v.think = ( func_t) ElectThink;
    electguard->s.v.nextthink = g_globalvars.time + 60;
}

void BeginPicking ()
{
    gedict_t *p;
    int num;
 
    if( CountPlayers() < 3 )
    {
        G_bprint(2, "Not enough players present\n"
					"Captain modes exited\n");

		CancelCaptains ();

        return;
    }

    G_bprint(2, "Both %s elected\n"
				"Team picking begins\n", redtext("captains"));

	num = 1;

	for( p = world; (p = find(p, FOFCLSN, "player")); )
    {
		p->k_picked = 0;

		if( capt_num( p ) )
		{
			G_sprint(p, 2, "\nUse %s or %s to choose\n", redtext("numbers"), redtext("impulses"));

			cvar_set( va("_k_captteam%d",  capt_num( p )), getteam(p) );
			cvar_set( va("_k_captcolor%d", capt_num( p )), va("%s %s", 
							ezinfokey(p, "topcolor"), ezinfokey(p, "bottomcolor")));
		}
		else
		{
    		stuffcmd(p, "break\n"
						"color 0\n"
						"skin \"\"\n"
						"team \"\"\n");
			p->s.v.frags = num;
			num++;
		}
	}

	turn_number   = 0;
	k_captainturn = (g_random() < 0.5 ? 1 : 2);

    PrintCaptainInTurn();
}

void BecomeCaptain(gedict_t *p)
{
	gedict_t *cap = p; // warning, below 'p' is overwriten

	for( p = world; (p = find(p, FOFCLSN, "player")) && !capt_num( p ); )
		; // empty

	cap->k_captain = ( p && capt_num( p ) == 1 ) ? 2 : 1;

	k_captains = floor( k_captains ) + 1;

	G_bprint(2, "%s becomes a %s\n", cap->s.v.netname, redtext("captain"));

	// s: if both captains are already elected, start choosing players
	if( k_captains == 2 )
		BeginPicking();
	else
		G_bprint(2, "One more %s should be elected\n", redtext("captain"));
}

