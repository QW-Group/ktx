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

// captain.c

#include "g_local.h"

// Captain functions introduced by rc\sturm
// here we use k_captains for storing the number of captains.
// one elected captain is worth 1, player being elected is worth 0.5 :)

// Check if picking should be finished

// pick the player

float CountPlayers();

void SetPlayerParams (gedict_t *p, gedict_t *cap)
{
    char *infoteam, *infocolor;

	infoteam  = cvar_string( va("_k_captteam%d", (int)k_captainturn) );
	infocolor = cvar_string( va("_k_captcolor%d", (int)k_captainturn) );
	k_captainturn = (k_captainturn == 1 ? 2 : 1);

    G_bprint(2, "%s set to team \x90%s\x91\n", p->s.v.netname, infoteam);

    G_sprint(p, 2, "You were picked by %s\n"
				   "Time to go ready\n", cap->s.v.netname );

    stuffcmd(p, "break\n"
				"team \"%s\"\n"
				"color \"%s\"\n", infoteam, infocolor);

    p->s.v.frags = 0;
    p->k_picked = cap->k_captain;
}

void PrintCaptainInTurn ()
{
    gedict_t *p;

    for( p = world; (p = find(p, FOFCLSN, "player")) && p->k_captain != k_captainturn; )
        ; // empty

	if ( p )
		G_bprint(2, "%s is picking\n", p->s.v.netname);
	else
		G_Error ("PrintCaptainInTurn null");
}

void CancelCaptains ()
{
    gedict_t *p;

	for( p = world; p = find(p, FOFCLSN, "player"); )
	{
		if( p->k_captain )
		{
			G_sprint(p, 2, "You are no longer a %s\n", redtext("captain"));

			p->k_captain = p->k_picked = 0;
		}
	}

	k_captains = 0;
}


void CheckFinishCaptain ()
{
    int pl_free = 0;
    gedict_t *p, *lastone = NULL;

    // s: calculate how many players are free
   	for( p = world; p = find(p, FOFCLSN, "player"); )
    {
        if( p->s.v.frags )
        {
            lastone = p;
            pl_free++;
        }
    }

    if( pl_free == 1 ) // one free player left
    {
    	for( p = world; (p = find(p, FOFCLSN, "player")) && p->k_captain != k_captainturn; )
        	; // empty

		if ( p )
        	SetPlayerParams( lastone, p );

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

    if( self->k_captain != k_captainturn )
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

    if( self->k_captain == 1 || self->k_captain == 2 )
    {
        self->k_captain = 0;

        if( k_captains == 2 )
        {
            G_bprint(2, "Player picking aborted\n");

            for( p = world; p = find(p, FOFCLSN, "player"); )
                if( p->s.v.frags )
                    p->s.v.frags = 0;
        }

        k_captains--;

        G_bprint(2, "%s captain present\n", (floor( k_captains ) ? "\x90\x31\x91" : redtext("No")));
    }
}

void BecomeCaptain ()
{
	int from, till;
    gedict_t *p, *electguard;

    // s: check if we are being elected or we are a captain already
    if( self->k_captain )
    {
        if( self->k_captain > 10 )
        {
            G_bprint(2,  "%s %s!\n", self->s.v.netname, redtext("aborts election"));

            AbortElect();
        }
        else
        {
            G_bprint(2, "%s is no longer a %s.\n", self->s.v.netname, redtext("captain"));

            ExitCaptain();
        }

        return;
    }

    if( match_in_progress || intermission_running )
        return;

    if( !isTeam() )
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

    // search if a captain already has the same team
    for( p = world; (p = find(p, FOFCLSN, "player")) && !p->k_captain; )
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

    k_captains += 0.5;

	G_bprint(2, "%s has %s status!\n", self->s.v.netname, redtext("requested captain"));

	for( from = 0, p = world; p = find_plrspc(p, &from); )
		if ( p != self && p->k_player )
			G_sprint(p, 2, "Type %s in console to approve\n", redtext("yes"));

    G_sprint(self, 2, "Type %s to abort election\n", redtext("captain"));

    // s: give a number for the captain (1 or 2); a number > 10 means election
    if(p && p->k_captain == 1)
        self->k_captain = 12;
    else
        self->k_captain = 11;

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

	for( p = world; p = find(p, FOFCLSN, "player"); )
    {
		if( p->k_captain )
		{
			G_sprint(p, 2, "\nUse %s or %s to choose\n", redtext("numbers"), redtext("impulses"));

			cvar_set( va("_k_captteam%d", (int)p->k_captain), getteam(p) );
			cvar_set( va("_k_captcolor%d", (int)p->k_captain), va("%s %s", 
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

	k_captainturn = g_random() < 0.5 ? 1 : 2;

    PrintCaptainInTurn();
}

