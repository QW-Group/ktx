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

void ElectThink();
void AbortElect();
float CountPlayers();

void SetPlayerParams (gedict_t *p)
{
    char *infoteam, *infocolor;

    if( k_captainturn == 1 )
    {
        k_captainturn = 2;
        infoteam  = ezinfokey(world, "captteam1");
        infocolor = ezinfokey(world, "captcolor1");
    }
    else
    {
        k_captainturn = 1;
        infoteam  = ezinfokey(world, "captteam2");
        infocolor = ezinfokey(world, "captcolor2");
    }

    G_bprint(2, "%s set to team %s‘\n", p->s.v.netname, infoteam);

    stuffcmd(p, "break\n"
				"team \"%s\"\n"
				"color \"%s\"\n", infoteam, infocolor);

    p->s.v.frags = 0;
    p->k_picked = self->k_captain;
}

void PrintCaptainInTurn ()
{
    gedict_t *p;

    p = find(world, FOFCLSN, "player");
    while( p && p->k_captain != k_captainturn )
        p = find(p, FOFCLSN, "player");

	if ( p )
		G_bprint(2, "%s is picking\n", p->s.v.netname);
	else
		G_Error ("PrintCaptainInTurn null");
}

void CheckFinishCaptain ()
{
    float f1 = 0;
    gedict_t *p=NULL, *lastone=NULL, *oldself=NULL;

    // s: calculate how many players are free
    p = find(world, FOFCLSN, "player");
    while( p )
    {
        if( !strnull( p->s.v.netname ) && p->s.v.frags )
        {
            lastone = p;
            f1++;
        }

        p = find(p, FOFCLSN, "player");
    }

    if( f1 == 1 )
    {
        p = find(world, FOFCLSN, "player");
        while( p && p->k_captain != k_captainturn )
            p = find(p, FOFCLSN, "player");

        oldself = self;
        self = p;

        SetPlayerParams( lastone );

        self = oldself;

        G_bprint(2, "All players chosen. Captain modes exited\n");

        f1 = 0;
    }

    if( !f1 )
    {
        p = find(world, FOFCLSN, "player");
        while( p )
        {
            if( p->k_captain )
            {
				if ( !oldself )
					G_Error ("CheckFinishCaptain null");

                if( strnull( oldself->s.v.netname ) )
                    G_sprint(p, 2, "You are no longer a captain\n");

                p->k_captain = p->k_picked = 0;
            }

            p = find(p, FOFCLSN, "player");
        }

        k_captains = 0;
    }
}

void CaptainPickPlayer ()
{
    gedict_t *p;

    if( self->k_captain != k_captainturn )
    {
        G_sprint(self, 2, "It's ξοτ your turn\n");
        return;
    }

    p = find(world, FOFCLSN, "player");
    while( p  && p->s.v.frags != self->s.v.impulse )
        p = find(p, FOFCLSN, "player");

    if( !p || p->s.v.frags != self->s.v.impulse )
    {
        G_sprint(self, 2, "No such player. Pick another one\n");
        return;
    }

    SetPlayerParams( p );
    G_sprint(p, 2, "You were picked by %s\n"
				   "Time to go ready\n", self->s.v.netname );

    CheckFinishCaptain();

    if( k_captains )
        PrintCaptainInTurn();
}

void ExitCaptain ()
{
    gedict_t *p;
    char *tmp;

    if( self->k_captain == 1 || self->k_captain == 2 )
    {
        self->k_captain = 0;

        if( k_captains == 2 )
        {
            G_bprint(2, "Player picking aborted\n");

            p = find(world, FOFCLSN, "player");
            while( p )
            {
                if( p->s.v.frags )
                    p->s.v.frags = 0;

                p = find(p, FOFCLSN, "player");
            }
        }

        k_captains--;

        if( floor( k_captains ) )
            tmp = "1‘";
        else
            tmp = "No";

        G_bprint(2, "%s captain present\n", tmp);
    }
}

void BecomeCaptain ()
{
    float f1;
    gedict_t *p, *electguard;
    char *tmp, *s2;

    // s: check if we are being elected or we are a captain already
    if( self->k_captain )
    {
        if( self->k_captain > 10 )
        {
            G_bprint(2,  "%s aborts election\n", self->s.v.netname);

            AbortElect();
        }
        else
        {
            G_bprint(2, "%s is no longer a captain.\n", self->s.v.netname);

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

    f1 = CountPlayers();
    if( f1 < 3 )
    {
        G_sprint(self, 2, "Not enough players present\n");
        return;
    }

    if( !teamplay )
    {
        G_sprint(self, 2, "Set proper teamplay mode first\n");
        return;
    }

    if( k_captains == 2 )
    {
        G_sprint(self, 2, "Only 2 captains are allowed\n");
        return;
    }

    // s: no captain election if any election in progress
    if( k_velect )
    {
        G_sprint(self, 2, "An election is already in progress\n");
        return;
    }

    // s: no captains with team ""
    if( strnull( ezinfokey(self, "team") ))
    {
        G_sprint(self, 2, "Set your team name first\n");
        return;
    }

    // search if a captain already has the same team
    p = find(world, FOFCLSN, "player");
    while( p  && !p->k_captain )
        p = find(p, FOFCLSN, "player");

	if ( p ) {

    	tmp = ezinfokey(p, "team");

    	if( streq( ezinfokey(self, "team"), tmp ) )
    	{
        	G_sprint(self, 2, "A γαπταιξ with team %s‘ already exists\n"
							  "Choose a new team name\n", tmp);
        	return;
    	}

    	// check if a captain already has the same colors
    	tmp = ezinfokey(self, "topcolor");
    	s2  = ezinfokey(self, "bottomcolor");

    	if( streq( tmp, ezinfokey(p, "topcolor") ) && streq( s2, ezinfokey(p, "bottomcolor")) )
    	{
        	G_sprint(self, 2, "A γαπταιξ with your color already exists\n"
							  "Change to new color\n");
        	return;
    	}
	}

    // announce the election
    k_velect = 1;
    k_captains = k_captains + 0.5;
    G_bprint(2, "%s has requested captain status\n"
				"Type ωεσ in console to approve\n",	self->s.v.netname);

    G_sprint(self, 2, "Type γαπταιξ to abort election\n");

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
    float num;
    char *tmp;

    if( CountPlayers() < 3 )
    {
        G_bprint(2, "Not enough players present\nCaptain modes exited\n");

        p = find(world, FOFCLSN, "player");
        while( p )
        {
            if( p->k_captain && !strnull( p->s.v.netname ) )
            {
                G_sprint(p, 2, "You are no longer a captain\n");
                p->k_captain = 0;
            }

            p = find(p, FOFCLSN, "player");
        }

        k_captains = 0;
        return;
    }

    num = 1;

    G_bprint(2, "Both γαπταιξσ elected\nTeam picking begins\n");

    p = find(world, FOFCLSN, "player");
    while( p )
    {
        if( !strnull( p->s.v.netname ) )
        {
            if( p->k_captain )
            {
                G_sprint(p, 2, "\nUse ξυνβεςσ or ινπυμσεσ to choose\n");

                tmp = ezinfokey(p, "team");

                if(p->k_captain == 1)
                    localcmd("localinfo captteam1 \"");
                else
                    localcmd("localinfo captteam2 \"");

                localcmd(tmp);

                if(p->k_captain == 1)
                    localcmd("\"\nlocalinfo captcolor1 \"");
                else
                    localcmd("\"\nlocalinfo captcolor2 \"");

                tmp = ezinfokey(p, "topcolor");
                localcmd("%s %s\"\n", tmp, ezinfokey(p, "bottomcolor"));
            }
            else
            {
                stuffcmd(p, "break\ncolor 0\nskin \"\"\nteam \"\"\n");
                p->s.v.frags = num;
                num++;
            }

//            p->k_msgcount = g_globalvars.time + 1; // shut up warning about team changing
        }

        p = find(p, FOFCLSN, "player");
    }

    num = g_random();

    if(num < 0.5)
        k_captainturn = 1;
    else
        k_captainturn = 2;

    PrintCaptainInTurn();
}

