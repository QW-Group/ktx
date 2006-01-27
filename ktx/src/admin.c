
// admin.c

#include "g_local.h"

float CountPlayers();
float CountRTeams();
void AdminMatchStart();
void StartTimer();
void StopTimer ( int removeDemo );

void NextClient();
void ExitKick(gedict_t *kicker);
void Deathmsg();
void RandomPickup();

// This is designed for pickup games and creates totally random teams(ish)
// It creates teams thus :
// Team red  color 4  skin base
// team blue color 13 skin base
// Does it by finding out how many players should be in each team.  Then it goes through each
// player and uses a random function.  If the number returned less than .5 it puts it in k_teamnumber 1.
// It does this until alreadyset = ineachteam.  Once that is done it loops through players and checks if
// we have found enough players for the team.  If it has then it sets all others to blue.
// Else it adds next player.  Once we have found enough players then we loop through again setting teams.
void RandomPickup ()
{
    gedict_t *p;
    float f1=0, ineachteam=0, alreadyset=0;

    if( self->k_admin != 2 )
    {
        G_sprint(self, 2, "You are not an admin\n");
        return;
    }

    if( match_in_progress )
        return;

    // Firstly obtain the number of players we have in total on server
    f1 = CountPlayers();

    // Dont need to bother if less than 4 players
    if(f1 < 4)
    {
        G_sprint(self, 2, "You need at least 4 players to do this.\n");
        return;
    }

    // Now we have number in each team.
    ineachteam = f1/2;

    // loop through players and put in one team or other as you go through.
    p = find(world, FOFCLSN, "player");
    while( p )
    {
        if( !strnull ( p->s.v.netname ) )
        {
            // Make all players have no team/skin/color x
            stuffcmd(p, "break\ncolor 0\nteam \"\"\nskin base\n");

            // If we have not found enough players then add to first team.
            if( alreadyset < ineachteam )
            {
                if ( g_random() < 0.5 )
                {
                    // Make member of team one
                    p->k_teamnumber = 1;
                    // Increment number of players set to team 1.
                    alreadyset++;
                }
                else
                {
                    // Make member of team two
                    p->k_teamnumber = 2;
                }
            }
        }
        p = find(p, FOFCLSN, "player");
    }

    // We have looped through each players once and should hopefully have enough players.
    // If not we loop again and find enough players for the first team.
    if( alreadyset < ineachteam )
    {
        p = find(world, FOFCLSN, "player");
        while( p )
        {
            if( !strnull ( p->s.v.netname )  && p->k_teamnumber != 1 )
            {
                // If we have not found enough players then add to first team.
                if( alreadyset < ineachteam )
                {
                    // We make this player member of team 1.
                    p->k_teamnumber = 1;

                    // Increment number of players set to team 1.
                    alreadyset = alreadyset + 1;
                }
            }
            p = find(p, FOFCLSN, "player");
        }
    }

    // We have looped through the players twice so we are guaranteed to have enough players.
    // We now loop again for the last time and set the teams.

    p = find(world, FOFCLSN, "player");
    while( p )
    {
        if( !strnull ( p->s.v.netname ) )
        {
            if( p->k_teamnumber == 1 )
                stuffcmd(p, "break\ncolor 4\nskin \"\"\nteam red\n");
            else
                stuffcmd(p, "break\ncolor 13\nskin \"\"\nteam blue\n");

            // Then we reset what team they are in to avoid problems.
            p->k_teamnumber = 0;
        }
        p = find(p, FOFCLSN, "player");
    }

    G_bprint(3, "console: random pickup game it is then\n");
}

// This toggle's between different messages
void Deathmsg ()
{
    float tmp;

    if( self->k_admin != 2 )
    {
        G_sprint(self, 2, "You are not an admin\n");
        return;
    }

    if( match_in_progress )
        return;

    tmp = atoi( ezinfokey ( world, "k_deathmsg" ) );

    G_bprint(2, "Γυστον δεατθνεσσαηεσ ");

    if(tmp != 0)
    {
        G_bprint(2, "disabled\n");
        localcmd("localinfo k_deathmsg 0\n");
    }
    else
    {
        G_bprint(2, "enabled\n");
        localcmd("localinfo k_deathmsg 1\n");
    }
}


void KickThink ()
{
    G_sprint( PROG_TO_EDICT( self->s.v.owner ) , 2, "Your λιγλ mode has timed out.\n");
    self->s.v.nextthink = -1;
    ExitKick( PROG_TO_EDICT( self->s.v.owner ) );
}

void AdminKick ()
{
    gedict_t *kickguard;

    if( self->k_captain )
    {
        G_sprint(self, 2, "Exit %s mode first\n", redtext("captain"));
        return;
    }

    if( self->k_admin != 2 )
    {
        G_sprint(self, 2, "You are not an admin\n");
        return;
    }

    if( self->k_kicking )
        ExitKick( self );
    else
    {
        self->k_kicking = 1;
        G_sprint(self, 2, "Kicking process started\n"
						  "\n"
						  "Type ω to kick, ξ for next, λιγλ to leave.\n");

        self->kick_ctype = "player";
        self->k_playertokick = world;

        kickguard = spawn(); // Check the 1 minute timeout for kick mode
        kickguard->s.v.owner = EDICT_TO_PROG( self );
        kickguard->s.v.classname = "kickguard";
        kickguard->s.v.think = ( func_t ) KickThink;
        kickguard->s.v.nextthink = g_globalvars.time + 60;

        NextClient();
    }
}

void NextClient ()
{
    float loop = 1;

    while( loop )
    {
		self->k_playertokick = self->k_playertokick ? self->k_playertokick : world;
        self->k_playertokick = find(self->k_playertokick, FOFCLSN, self->kick_ctype);

        if( !(self->k_playertokick) ) {
            if( streq( self->kick_ctype, "player" ) ) {
                self->kick_ctype = "spectator";
				loop++;
			}
            else {
                self->kick_ctype = "player";
				loop++;
			}
		}
        else if( !strnull( self->k_playertokick->s.v.netname )
				&& ( streq ( self->kick_ctype, "spectator" )
						|| ( streq ( self->kick_ctype, "player" )
							 && self->k_playertokick->k_accepted == 2
						   )
				   )
			   )
            loop = 0;
		
		if ( loop > 3 ) {
			G_sprint(self, 2, "Can't find anybody to kick\n");
			ExitKick ( self );
			return;
		}
    }

    G_sprint(self, 2, "Kick %s %s?\n", redtext(self->kick_ctype),
								self->k_playertokick->s.v.netname);
}

void YesKick ()
{
    gedict_t *oldtokick;

    if( self->k_kicking )
    {
        if( !strnull(self->k_playertokick->s.v.classname) && self->k_playertokick != self )
        {
            G_bprint(2, "%s was kicked by %s\n",
								self->k_playertokick->s.v.netname, self->s.v.netname);

            G_sprint(self->k_playertokick, 2, "You were kicked from the server\n");
            oldtokick = self->k_playertokick;

            NextClient();
			
			GhostFlag(oldtokick);
            oldtokick->s.v.classname = "";
            stuffcmd(oldtokick, "disconnect\n"); // FIXME: stupid way
        }
        else if( self->k_playertokick == self )
        {
            G_bprint(2, "%s kicked %s\n", self->s.v.netname,
						 ( streq( ezinfokey( self, "gender"),  "f") ? "herself" : "himself" ) );

			// hehe %)
            G_sprint(self, 2, "Say \"bye\" and then type \"disconnect\" next time.\n");
			GhostFlag(self);
            self->s.v.classname = "";
            stuffcmd(self, "disconnect\n");  // FIXME: stupid way
        }
    }
}

void DontKick ()
{
    if ( self->k_kicking )
        NextClient();
}

void ExitKick (gedict_t *kicker)
{
    gedict_t *childkick;

    if ( kicker->k_kicking )
    {
        kicker->k_kicking = 0;

        childkick = find(world, FOFCLSN, "kickguard");

        while( childkick && PROG_TO_EDICT( childkick->s.v.owner ) != kicker )
            childkick = find(childkick, FOFCLSN, "kickguard");

		if ( childkick )
			ent_remove ( childkick );
		else
			G_Error ("ExitKick null");

        if( !strnull( kicker->s.v.classname ) )
            G_sprint(kicker, 2, "Kicking process terminated\n");
    }
}

void BecomeAdmin()
{
	G_bprint(2, "%s ηαιξσ αδνιξ στατυσ!\n", self->s.v.netname);
	G_sprint(self, 2, "Type γονναξδσ for info\n");
	self->k_admin = 2;
}

void ReqAdmin ()
{
    //  admin state=1.5 check for election
    if( self->k_admin == 1.5 )
    {
        G_sprint(self, 2, "Abort %sion first\n", redtext("election"));
        return;
    }

    if( self->k_admin == 2 )
    {
        G_bprint(2, "%s is no longer an %s.\n", self->s.v.netname, redtext("admin"));

        if( self->k_kicking )
            ExitKick( self );

        self->k_admin = 0;
        return;
    }

    if( self->k_admin )
        return;

	if ( Vip_IsFlags( self, VIP_ADMIN ) )
    {
		BecomeAdmin();
		return;
    }

    self->k_admin  = 1;
    self->k_adminc = 6;
    self->k_added  = 0;

    // You can now use numbers to enter code
    G_sprint(self, 2, "Use ξυνβεςσ or ινπυμσεσ to enter code\n");
}

void AdminImpBot ()
{
    float coef, i1;

    if( !self->k_adminc )
    {
        self->k_admin = 0;
        return;
    }

    i1 = (int)self->k_adminc - 1;
    coef = self->s.v.impulse;

    while( i1 )
    {
        coef = coef * 10;
        i1--;
    }

    self->k_added  += coef;
    self->k_adminc -= 1;

    if( !self->k_adminc )
    {
        if( self->k_added == atoi( ezinfokey( world, "k_admincode" ) ) )
        {
			BecomeAdmin();
			return;
        }
        else
        {
            self->k_admin = 0;
            G_sprint(self, 2, "Access denied...\n");
        }
    }
    else
        G_sprint(self, 2, "%d νοςε το ηο\n", (int)self->k_adminc);
}

void AdminMatchStart ()
{
    gedict_t *p;
    int i = 0;

    for( p = world; p = find(p, FOFCLSN, "player"); )
    {
		if( p->ready && p->k_accepted == 2 ) {
			i++;
		}
		else
		{
			G_bprint(2, "%s was kicked by admin forcestart\n", p->s.v.netname);
			G_sprint(p, 2, "Bye bye! Pay attention next time.\n");

			p->k_accepted = 0;
			p->s.v.classname = "";
			stuffcmd(p, "disconnect\n"); // FIXME: stupid way
		}
	}

    k_attendees = i;

    if( k_attendees ) {
        StartTimer();
	}
    else
    {
        G_bprint(2, "Can't start! More players needed.\n");
		EndMatch( 1 );
    }
}

void ReadyThink ()
{
    float i1;
	char *txt, *gr;
    gedict_t *p=NULL, *p2=NULL;

    p2 = PROG_TO_EDICT( self->s.v.owner );
	
    if(    ( p2->k_player && !( p2->ready ) ) // forcestart breaked via break command
		|| ( p2->k_spectator && !k_force )	// forcestart breaked via forcebreak command (spectator admin)
	  )
    {
        k_force = 0;

        G_bprint(2, "%s interrupts countdown\n", p2->s.v.netname );

        ent_remove ( self );

        return;
    }

	k_attendees = CountPlayers();

	if ( !isCanStart(NULL, true) ) {
        k_force = 0;

        G_bprint(2, "Forcestart canceled\n");

        ent_remove ( self );

        return;
	}


    self->attack_finished--;

    i1 = self->attack_finished;

    if( i1 <= 0 )
    {
        k_force = 0;

        AdminMatchStart();

        ent_remove ( self );

        return;
    }

	txt = va( "%s second%s to gamestart", dig3( i1 ), ( i1 == 1 ? "" : "s") );
	gr  = va( "\n%s!", redtext("Go ready") );

    for( p = world; p = find(p, FOFCLSN, "player"); )
		G_centerprint(p, "%s%s", txt, (p->ready ? "" : gr));

    for( p = world; p = find(p, FOFCLSN, "spectator"); )
		G_centerprint(p, "%s", txt);

    self->s.v.nextthink = g_globalvars.time + 1;
}

void AdminForceStart ()
{
    gedict_t *mess;
//    float f1, k_lockmin, k_lockmax, f2;
//    char *tmp;

    if( match_in_progress || self->k_admin != 2 )
        return;

	// no forcestart in practice mode
	if ( k_practice ) {
		G_sprint(self, 2, "%s\n", redtext("Server in practice mode"));
		return;
	}

    if( streq( self->s.v.classname, "player" ) && !self->ready )
    {
        G_sprint(self, 2, "Ready yourself first\n");
        return;
    }

	k_attendees = CountPlayers();

	if ( !isCanStart( self, true ) ) {
        G_sprint(self, 2, "Can't issue!\n");
		return;
	}

    if( k_attendees )
    {
        G_bprint(2, "%s forces matchstart!\n", self->s.v.netname);

        k_force = 1;

        mess = spawn();
        mess->s.v.classname = "mess";
        mess->s.v.owner = EDICT_TO_PROG( self );
        mess->s.v.think = ( func_t ) ReadyThink;
        mess->s.v.nextthink = g_globalvars.time + 0.1;
        mess->attack_finished = 10 + 1;
    }
    else
        G_sprint(self, 2, "Can't issue! More players needed.\n");
}


void AdminForceBreak ()
{
//    char *tmp;

    if( self->k_admin == 2 && strneq( self->s.v.classname, "player" ) && !match_in_progress )
    {
        k_force = 0;
        return;
    }

    if( self->k_admin != 2 || !match_in_progress )
        return;

    if( strneq( self->s.v.classname, "player" ) && match_in_progress == 1 )
    {
        k_force = 0;
        StopTimer( 1 );

        return;
    }

    if( k_oldmaxspeed ) // huh???
        cvar_set("sv_maxspeed", va("%d", (int)k_oldmaxspeed));

    G_cprint("%%forcebreak%%%s\n", self->s.v.netname);
    G_bprint(2, "%s forces a break!\n"
				"MATCH OVER!!\n", self->s.v.netname);

    EndMatch( 0 );
}

void TogglePreWar ()
{
    gedict_t *p = world;
    float tmp;

    if( self->k_admin != 2 )
        return;

    tmp = atoi( ezinfokey( world, "k_prewar" ) );

    if( tmp )
    {
        localcmd("localinfo k_prewar 0\n");

        if( !match_in_progress )
        {
            G_bprint(2, "Players may ξοτ fire before match!\n");

            p = find(p, FOFCLSN, "player");
            while( p )
            {
                if( !strnull ( p->s.v.netname ) )
                    stuffcmd(p, "-attack\n");

                p = find(p, FOFCLSN, "player");
            }
        }
        else
            G_sprint(self, 2, "Players may ξοτ fire before match!\n");

        return;
    }

    localcmd("localinfo k_prewar 1\n");

    if( !match_in_progress )
        G_bprint(2, "Players may fire before match.\n");
    else
        G_sprint(self, 2, "Players may fire before match.\n");
}

void ToggleMapLock ()
{
    float tmp;

    if( self->k_admin != 2 )
        return;

    tmp = atoi( ezinfokey( world, "k_lockmap" ) );

    if( tmp )
    {
        localcmd("localinfo k_lockmap 0\n");

        if( !match_in_progress )
            G_bprint(2, "%s unlocks map.\n", self->s.v.netname);
        else
            G_sprint(self, 2, "Map unlocked.\n");

        return;
    }

    localcmd("localinfo k_lockmap 1\n");

    if( !match_in_progress )
        G_bprint(2, "%s locks map!\n", self->s.v.netname);
    else
        G_sprint(self, 2, "Map is locked!\n");
}

void ToggleMaster ()
{
    float f1;

    if( self->k_admin != 2 )
        return;

    f1 = atoi( ezinfokey( world, "k_master" ) );
    f1 = 1 - f1;

    if( f1 )
        G_bprint(2, "%s sets mastermode!\nPlayers may ξοτ alter settings\n", self->s.v.netname);
    else
        G_bprint(2,"Mastermode disabled\nPlayers can now alter settings\n");

    localcmd("localinfo k_master %d\n", (int)f1);
}


// FIXME: and why we can't use server pause command, instead of this UGLY HACK?
// may be because pause command dosn't exist? %)

int k_realPausable = 0;

void ModPause (int pause)
{
    gedict_t *e1;
    float f1;

	if ( k_pause == pause )
		G_Error ("ModPause: k_pause == pause");

    k_pause = pause;

    WriteByte(MSG_ALL, SVC_SETPAUSE);

    if( pause )
    {
        k_pausetime = g_globalvars.time;

		k_realPausable = (int) cvar( "pausable" ); // save
        cvar_set("pausable", "0"); // mod pause issued, so server native pause can't be issued

        k_oldmaxspeed = k_maxspeed;
        cvar_set("sv_maxspeed", "0");

        WriteByte(MSG_ALL, 1); // pause plaque on

        e1 = nextent( world );
        while ( e1 )
        {
            if( e1->s.v.nextthink > g_globalvars.time )
            {
                e1->k_ptime = e1->s.v.nextthink;
                e1->s.v.nextthink = -1;

                if( streq( e1->s.v.classname, "player" ) )
                    e1->maxspeed = 0;
            }

            e1 = nextent( e1 );
        }
    }
    else
    {
        WriteByte(MSG_ALL, 0); // pause plaque off

        cvar_set("pausable", va("%d", k_realPausable)); // restore
        cvar_set("sv_maxspeed", va("%d", (int)k_oldmaxspeed));

        f1 = g_globalvars.time - k_pausetime;

        e1 = nextent( world );
        while( e1 )
        {
            if(e1->s.v.nextthink == -1)
                e1->s.v.nextthink = f1 + e1->k_ptime;
            if( streq( e1->s.v.classname, "player" ) )
            {
                e1->maxspeed = k_oldmaxspeed;
                e1->pain_finished += f1;

                if(e1->invincible_finished)
// qqshka: imho wrong   e1->invincible_finished = f1 + e1->pain_finished;
// FIXME: check this
				e1->invincible_finished			+= f1;

                if(e1->invisible_finished)
                    e1->invisible_finished		+= f1;

                if(e1->super_damage_finished)
                    e1->super_damage_finished	+= f1;

                if(e1->super_damage_finished)
                    e1->radsuit_finished		+= f1;

                e1->air_finished += f1;
            }
            else if(   streq( e1->s.v.classname, "dropring" )
					|| streq( e1->s.v.classname, "dropquad" ) )
                e1->cnt += f1;

            e1 = nextent( e1 );
        }
    }
}

void AdminForcePause ()
{
    if( self->k_admin != 2 || match_in_progress != 2 )
        return;

    if( !k_pause )
        G_bprint(2, "%s issues a pause\n", self->s.v.netname);
    else
        G_bprint(2, "unpaused by %s!\n", self->s.v.netname);

	ModPause (!k_pause);
}

char *Enables( float f )
{
	return ( f ? "enables" : "disables" );
}

char *Allows( float f )
{
	return ( f ? "allows" : "disallows" );
}


// qqshka

void ToggleFallBunny ()
{
    float f1;

    if( self->k_admin != 2 )
        return;

    f1 = !atoi( ezinfokey( world, "k_fallbunny" ) );

    G_bprint(2, "%s %s fallbunny!\n", self->s.v.netname, Enables( f1 ));

    localcmd("localinfo k_fallbunny %d\n", (int)f1);
}

