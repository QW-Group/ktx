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
 *  $Id$
 */

// coach.c

#include "g_local.h"

int coach_num(gedict_t *p)
{
    if( p->k_coach == 1 || p->k_coach == 2 )
        return p->k_coach;

    return 0;
}

qbool is_coach(gedict_t *p)
{
    if (p->k_coach)
        return true;

    return false;
}

void CancelCoaches()
{
    gedict_t *p;

    k_coaches = 0;

    for( p = world; (p = find_plr( p )); )
    {
        if( coach_num( p ) )
            G_sprint(p, 2, "You are no longer a %s\n", redtext("coach"));

        p->k_coach = 0;

        if ( is_elected(p, etCoach) ) // just for sanity
            AbortElect();
    }
}

void ExitCoach()
{
    if( !coach_num( self ) )
        return;

    self->k_coach = 0;

    k_coaches--;

    G_bprint(2, "%s coach present\n", (floor( k_coaches ) ? "\x90\x31\x91" : redtext("No")));

    // if 'nospecs' is active, a demoted coach must be kicked
    if ( cvar("_k_nospecs") )
        stuffcmd(self, "disconnect\n");  // FIXME: stupid way
}

void VoteCoach()
{
    int till;
    gedict_t *p, *electguard;

    // s: check if we are being elected or we are a coach already
    if( is_elected(self, etCoach) )
    {
        G_bprint(2,  "%s %s!\n", self->netname, redtext("aborts election"));

        AbortElect();
        return;
    }

    if ( coach_num( self ) )
    {
        G_bprint(2, "%s is no longer a %s\n", self->netname, redtext("coach"));

        ExitCoach();
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

    if( k_coaches == 2 )
    {
        G_sprint(self, 2, "Only 2 coaches are allowed\n");
        return;
    }

    // s: no coach election if any election in progress
    if( get_votes( OV_ELECT ) )
    {
        G_sprint(self, 2, "An election is already in progress\n");
        return;
    }

    if( (till = Q_rint( self->v.elect_block_till - g_globalvars.time)) > 0 ) {
        G_sprint(self, 2, "Wait %d second%s!\n", till, count_s(till) );
        return;
    }

    // s: no coaches with team ""
    if( strnull( getteam( self ) ) )
    {
        G_sprint(self, 2, "Set your team name first\n");
        return;
    }

    // search if a coach already has the same team
    for( p = world; (p = find_spc( p )) && !coach_num( p ); )
        ; // empty

    if ( p ) {
        if( streq( getteam( self ), getteam( p ) ) )
        {
        	G_sprint(self, 2, "A %s with team \x90%s\x91 already exists\n"
                              "Choose a new team name\n", redtext("coach"), getteam( p ));
            return;
    	}
    }

    // announce the election
    self->v.elect = 1;
    self->v.elect_type = etCoach;

    k_coaches += 0.5;

    G_bprint(2, "%s has %s status!\n", self->netname, redtext("requested coach"));

    for( p = world; (p = find_client( p )); )
        if ( p != self && p->ct == ctPlayer )
            G_sprint(p, 2, "Type %s in console to approve\n", redtext("yes"));

    G_sprint(self, 2, "Type %s to abort election\n", redtext("coach"));

    // s: spawn the cool dude
    electguard = spawn(); // Check the 1 minute timeout for election
    electguard->s.v.owner = EDICT_TO_PROG( world );
    electguard->classname = "electguard";
    electguard->think = ( func_t) ElectThink;
    electguard->s.v.nextthink = g_globalvars.time + 60;
}

void BecomeCoach(gedict_t *p)
{
    gedict_t *coach = p; // warning, below 'p' is overwritten

    for( p = world; (p = find_plr( p )) && !coach_num( p ); )
        ; // empty

    coach->k_coach = ( p && coach_num( p ) == 1 ) ? 2 : 1;

    // a coach cannot autotrack
//    coach->autotrack = atNone;

    k_coaches = floor( k_coaches ) + 1;

    G_bprint(2, "%s becomes a %s\n", coach->netname, redtext("coach"));

    // s: if both coaches are already elected, start choosing players
    if( k_coaches < 2 )
        G_bprint(2, "One more %s should be elected\n", redtext("coach"));
}

// A coach might only track players from his/her own team.
// This function takes care of not allowing a coach to track enemy players.
// It is called whenever a track change is detected for a coach, but unfortunately
// it is called only after the track change.
// The function can have multiple outcomes:
// 1) the coach is jumped to a team mate -> nothing to do
// 2) the coach is jumped to an enemy player
// 2a) we are able to located a team mate -> start tracking that player instead
// 2b) we are not able to find a team mate -> stop tracking completely
//
// The function returns true if there was a forced track change; false other wise
qbool TrackChangeCoach(gedict_t *p)
{
    gedict_t *target;
    int id;

    target = PROG_TO_EDICT(p->s.v.goalentity);
    // we only going to update the tracked player, if we are tracking at all
    // probably a paranoid check, as the function should not be called if the 'target' of the
    // spectator is not changed.
    if ( (int)(target - world) >= 1 && (int)(target - world) <= MAX_CLIENTS )
    {
        // we are tracking someone. check team of the tracked player
        if (strneq(getteam(p), getteam(target)))
        {
            // team is not matching, so let's trying to locate a player within the coach's team
            // start from the currently tracked player, and go through the player list
            target = find_plr_same_team(target, getteam(p));
            if (!target)
            {
                // we haven't found a team mate. lets restart the search from the 1st player
                target = find_plr_same_team(world, getteam(p));
            }

            if (NULL != target)
            {
                // team mate found, lets track this play
                if ( ( id = GetUserID( target ) ) > 0 )
                {
                    stuffcmd_flags( p, STUFFCMD_IGNOREINDEMO, "track %d\n", id );
                }
            }
            else
            {
                // no suitable player to track (all of them in enemy team) -> just stop tracking
                G_centerprint(p, "Found no suitable player to track");
                // this is kind of ugly way to disable tracking, but I have no other idea :/
                stuffcmd_flags(p, STUFFCMD_IGNOREINDEMO, "-attack;wait;+attack;wait;-attack\n");
            }

            return true;
        }
    }

    return false;
}
