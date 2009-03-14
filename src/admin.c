/*
 * $Id$
 */

// admin.c

#include "g_local.h"

void AdminMatchStart();
void PlayerReady();
void NextClient();
qboolean DoKick(gedict_t *victim, gedict_t *kicker);

// is real admin
qboolean
is_real_adm(gedict_t *p)
{
	return (p->k_admin & AF_REAL_ADMIN);
}

// is elected admin
qboolean
is_adm(gedict_t *p)
{
	return (is_real_adm(p) || (p->k_admin & AF_ADMIN));
}

void
KickThink()
{
	if (!self->k_kicking)
		return;

	// Check the 1 minute timeout for kick mode
	if (self->k_kicking + 60 < g_globalvars.time) {
		G_sprint(self, 2, "Your %s mode has timed out\n", redtext("kick"));
		ExitKick(self);
		return;
	}

	if (!is_adm(self)) {
		// not admin now, so cancel kick mode, just for sanity
		ExitKick(self);
		return;
	}
}

void
ExitKick(gedict_t *kicker)
{
	if (!kicker->k_kicking)
		return;

	kicker->k_playertokick = world;
	kicker->k_kicking = 0;

	if (!strnull(kicker->s.v.classname))
		G_sprint(kicker, 2, "Kicking process terminated\n");
}

// assuming kicker is admin
qboolean
is_can_kick(gedict_t *victim, gedict_t *kicker)
{
	if (VIP_IsFlags(victim, VIP_NOTKICKABLE) && !is_real_adm(kicker)) {
		G_sprint(kicker, 2, "You can't kick VIP \x8D %s as elected admin\n",
			(strnull(victim->s.v.netname) ? "!noname!" : victim->s.v.netname));
		return (false);
	}

	if (is_real_adm(victim) && !is_real_adm(kicker)) {
		G_sprint(kicker, 2, "You can't kick real admin \x8D %s as elected admin\n",
			(strnull(victim->s.v.netname) ? "!noname!" : victim->s.v.netname));
		return (false);
	}

	return (true);
}

qboolean
DoKick(gedict_t *victim, gedict_t *kicker)
{
	if (!victim || !kicker)
		return (false);

	if (victim == kicker) {
		G_bprint(2, "%s kicked %s\n", getname(kicker), g_himself(kicker));
		G_sprint(kicker, 2, "Say \"bye\" and then type \"disconnect\" next time\n");
		stuffcmd(kicker, "disconnect\n"); // FIXME: stupid way

		if (!FTE_sv)
			localcmd("addip %s ban +30\n", cl_ip(victim)); // BAN for 30 seconds
	}
	else
	{
		if (!is_can_kick(victim, kicker))
			return (false);

		G_bprint(2, "%s was kicked by %s\n", getname(victim), getname(kicker));
		G_sprint(victim, 2, "You were kicked from the server\n");
		stuffcmd(victim, "disconnect\n"); // FIXME: stupid way

		if (!FTE_sv)
			localcmd("addip %s ban +30\n", cl_ip(victim)); // BAN for 30 seconds
	}

	return (true);
}

void
AdminKick()
{
	int argc = trap_CmdArgc();

	if (!is_adm(self)) {
		G_sprint(self, 2, "You are not an admin\n");
		return;
	}

	if (self->k_kicking) {
		ExitKick(self);
		return;
	}

	if (argc >= 2) {
		gedict_t *p;
		char arg_2[1024], *str;

		trap_CmdArgv(1, arg_2, sizeof (arg_2));

		if (!(p = SpecPlayer_by_IDorName(arg_2)) && !(p = not_connected_by_IDorName(arg_2))) {
			G_sprint(self, 2, "kick: client %s not found\n", arg_2);
			return;
		}

		if (DoKick(p, self) && !strnull(str = params_str(2, -1))) // show reason
			G_bprint(2, "\x90%s\x91\n", str);

		return;
	}

	G_sprint(self, 2, "Kicking process started\n"
		"žžžžžžžžžžžžžžžžžžžžžŸ\n"
		"Type \371 to kick, \356 for next, %s to leave\n", redtext("kick"));
	self->k_kicking = g_globalvars.time;
	self->k_playertokick = world;
	NextClient();
}

// multi kick
void
m_kick()
{
	int i, k;
	gedict_t *p;
	char arg_x[1024], *str;
	int argc = trap_CmdArgc();

	if (!is_adm(self)) {
		G_sprint(self, 2, "You are not an admin\n");
		return;
	}

	trap_CmdArgv(1, arg_x, sizeof (arg_x));

	if (argc < 2 || !only_digits(arg_x)) {
		G_sprint(self, 2, "mkick <id1 [id2 [id3 ...]] [reason]>\n");
		return;
	}

	for (k = 0, i = 1; i < argc; i++) {
		trap_CmdArgv(i, arg_x, sizeof (arg_x));

		if (!only_digits(arg_x))
			break;

		if (!(p = SpecPlayer_by_id(atoi(arg_x))) && !(p = not_connected_by_id(atoi(arg_x)))) {
			G_sprint(self, 2, "mkick: client %s not found\n", arg_x);
			continue;
		}

		if (!DoKick(p, self))
			continue;

		k++;
	}

	if (!k)
		return;

	if (!strnull(str = params_str(i, -1))) // show reason
		G_bprint(2, "\x90%s\x91\n", str);
}

void
NextClient()
{
	int from = 0;

	if (!self->k_kicking)
		return;

	self->k_playertokick = (self->k_playertokick ? self->k_playertokick : world);
	from = (self->k_playertokick != world && self->k_playertokick->ct == ctSpec);
	self->k_playertokick = find_plrspc(self->k_playertokick, &from);

	if (!self->k_playertokick) {  // try find anyone at least
		from = 0;
		self->k_playertokick = find_plrspc(world, &from);
	}

	if (!self->k_playertokick) {
		G_sprint(self, 2, "Can't find anybody to kick\n");
		ExitKick(self);
		return;
	}

	G_sprint(self, 2, "Kick %s %s?\n",
		redtext(self->k_playertokick->ct == ctPlayer ? "player" : "spectator"),
		getname(self->k_playertokick));
}

void
YesKick()
{
	if (!self->k_kicking)
		return;

	if (!self->k_playertokick || strnull(self->k_playertokick->s.v.classname)) {
		NextClient();
		return;
	}

	if (DoKick(self->k_playertokick, self) && self->k_playertokick == self)
		return; // selfkick success ;)

	NextClient();
}

void
DontKick()
{
	if (!self->k_kicking)
		return;

	NextClient();
}

void
BecomeAdmin(gedict_t *p, int adm_flags)
{
	G_bprint(2, "%s %s!\n", p->s.v.netname, redtext("gains admins status"));
	G_sprint(p, 2, "Please give up admin rights when you're done\n"
		"Type %s for info\n", redtext("commands"));

	p->k_admin |= adm_flags;

	on_admin(p);
}

// "admin" command

void
ReqAdmin()
{
    //  check for election
    if (is_elected(self, etAdmin))
    {
        G_sprint(self, 2, "Abort %sion first\n", redtext("elect"));
        return;
    }

    if( is_adm( self ) )
    {
        G_bprint(2, "%s is no longer an %s\n", self->s.v.netname, redtext("admin"));

        if( self->k_kicking )
            ExitKick( self );

        self->k_admin = 0; // ok, remove all admin flags

		on_unadmin( self );

        return;
    }

    if( self->k_adminc )
	{
        G_sprint(self, 2, "%s code canceled\n", redtext("admin"));
    	self->k_adminc = 0;
		return;
	}

	if( !cvar( "k_admins" ) ) {
		G_sprint(self, 2, "%s on this server!\n", redtext("NO admins"));
		return;
	}

	if ( VIP_IsFlags( self, VIP_ADMIN ) ) // this VIP does't required pass
    {
		BecomeAdmin(self, AF_REAL_ADMIN);
		return;
    }
	
	// parse /admin <pass>
	if ( trap_CmdArgc() == 2 ) {
		char arg_2[1024];
		char *pass = cvar_string( "k_admincode" );
		int till = Q_rint(self->k_adm_lasttime + 5 - g_globalvars.time);

		if( self->k_adm_lasttime && till > 0  ) { // probably must help against brute force
			G_sprint(self, 2, "Wait %d second%s!\n", till, count_s(till) );
			return;
		}

		trap_CmdArgv( 1, arg_2, sizeof( arg_2 ) );

		if ( !strnull(pass) && strneq(pass, "none") && streq(arg_2, pass) )
			BecomeAdmin(self, AF_REAL_ADMIN);
		else {
            G_sprint(self, 2, "%s...\n", redtext("Access denied"));
			self->k_adm_lasttime = g_globalvars.time;
		}

		return;
	}

    self->k_adminc = 6;
    self->k_added  = 0;

    // You can now use numbers to enter code
    G_sprint(self, 2, "Use %s or %s to enter code\n", redtext("numbers"), redtext("impulses") );
}

void AdminImpBot ()
{
    float coef, i1;

    if( self->k_adminc < 1 )
    {
        self->k_adminc = 0;
        return;
    }

    i1   = (int)(self->k_adminc -= 1);
    coef = self->s.v.impulse;

    while( i1 > 0 )
    {
        coef *= 10;
        i1--;
    }

    self->k_added += coef;

    if( self->k_adminc < 1 )
    {
		int iPass = cvar( "k_admincode" );
		int till = Q_rint(self->k_adm_lasttime + 5 - g_globalvars.time);

        self->k_adminc = 0;

		if( self->k_adm_lasttime && till > 0  ) { // probably must help against brute force
			G_sprint(self, 2, "Wait %d second%s!\n", till, count_s(till) );
			return;
		}

        if( iPass && self->k_added == iPass )
        {
			BecomeAdmin(self, AF_REAL_ADMIN);
			return;
        }
        else
        {
            G_sprint(self, 2, "%s...\n", redtext("Access denied"));
			self->k_adm_lasttime = g_globalvars.time;
        }
    }
    else
        G_sprint(self, 2, "%d %s\n", (int)self->k_adminc, redtext("more to go"));
}

// "ellect" command

void VoteAdmin()
{
	gedict_t *p;
	int   till;

	gedict_t *electguard;

// Can't allow election and code entering for the same person at the same time
	if( self->k_adminc ) {
		G_sprint(self, 2, "Finish entering the code first\n");
		return;
	}

	if( is_adm( self ) ) {
		G_sprint(self, 2, "You are already an admin\n");
		return;
	}

	if( is_elected( self, etAdmin ) ) {
		G_bprint(2, "%s %s!\n", self->s.v.netname, redtext("aborts election"));
		AbortElect();
		return;
	}

// Only one election per server because otherwise we wouldn't know how to count
// "yes"s or "no"s
	if( get_votes( OV_ELECT ) ) {
		G_sprint(self, 2, "An election is already in progress\n");
		return;
	}

	if( !cvar( "k_admins" ) ) {
		G_sprint(self, 2, "%s on this server!\n", redtext("NO admins"));
		return;
	}

// Check if voteadmin is allowed
	if( !cvar( "k_allowvoteadmin" ) ) {
		G_sprint(self, 2, "Admin election is not allowed on this server\n");
		return;
	}

	if( (till = Q_rint( self->v.elect_block_till - g_globalvars.time)) > 0  ) {
		G_sprint(self, 2, "Wait %d second%s!\n", till, count_s(till) );
		return;
	}

	if( self->ct == ctSpec && match_in_progress )
		return;

	G_bprint(2, "%s has %s rights!\n", self->s.v.netname, redtext("requested admin"));

	for( p = world; (p = find_client( p )); )
		if ( p != self && p->ct == ctPlayer )
			G_sprint(p, 2, "Type %s in console to approve\n", redtext("yes"));

	G_sprint(self, 2, "Type %s to abort election\n", redtext("elect"));

    // announce the election
	self->v.elect = 1;
	self->v.elect_type = etAdmin;

	electguard = spawn(); // Check the 1 minute timeout for election
	electguard->s.v.owner = EDICT_TO_PROG( world );
	electguard->s.v.classname = "electguard";
	electguard->s.v.think = ( func_t ) ElectThink;
	electguard->s.v.nextthink = g_globalvars.time + 60;
}

void AdminMatchStart ()
{
    gedict_t *p;
    int i = 0;

    for( p = world; (p = find_plr( p )); )
    {
		if( p->ready ) {
			i++;
		}
		else
		{
			G_bprint(2, "%s was kicked by admin forcestart\n", p->s.v.netname);
			G_sprint(p, 2, "Bye bye! Pay attention next time.\n");

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
	
    if(    ( p2->ct == ctPlayer && !( p2->ready ) ) // forcestart breaked via break command
		|| ( p2->ct == ctSpec && !k_force )	// forcestart breaked via forcebreak command (spectator admin)
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

	txt = va( "%s second%s left before game starts", dig3( i1 ), ( i1 == 1 ? "" : "s") );
	gr  = va( "\n%s!", redtext("Get ready") );

    for( p = world; (p = find_client( p )); )
		if ( p->ct == ctPlayer )
			G_centerprint(p, "%s%s", txt, (p->ready ? "" : gr));
		else
			G_centerprint(p, "%s", txt);

    self->s.v.nextthink = g_globalvars.time + 1;
}

void AdminForceStart ()
{
    gedict_t *mess;

    if( match_in_progress || match_over || !is_adm( self ) )
        return;

	// no forcestart in practice mode
	if ( k_practice ) {
		G_sprint(self, 2, "%s\n", redtext("Server in practice mode"));
		return;
	}

    if( self->ct == ctPlayer && !self->ready )
    {
		PlayerReady();

		if ( !self->ready ) {
        	G_sprint(self, 2, "Ready yourself first\n");
        	return;
		}
    }

	if( find(world, FOFCLSN, "mess") ) {
        G_sprint(self, 2, "forcestart already in progress!\n");
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
    if( is_adm( self ) && self->ct != ctPlayer && !match_in_progress )
    {
        k_force = 0;
        return;
    }

    if( !is_adm( self ) || !match_in_progress )
        return;

    if( self->ct != ctPlayer && match_in_progress == 1 )
    {
        k_force = 0;
        StopTimer( 1 );

        return;
    }

    if( k_oldmaxspeed ) // huh???
        cvar_fset( "sv_maxspeed", k_oldmaxspeed );

    G_bprint(2, "%s forces a break!\n", self->s.v.netname);

    EndMatch( 0 );
}

void PlayerStopFire(gedict_t *p)
{
	stuffcmd_flags(p, STUFFCMD_IGNOREINDEMO, "-attack\n");
	p->wreg_attack = 0;
}

void PlayersStopFire()
{
    gedict_t *p;

	for( p = world; (p = find_plr( p )); )
		PlayerStopFire( p );
}

void TogglePreWar ()
{
    int k_prewar = bound(0, cvar( "k_prewar" ), 2);

    if( !is_adm( self ) )
        return;

	if ( ++k_prewar > 2 )
		k_prewar = 0;

	switch ( k_prewar ) {
		case  1: if( !match_in_progress )
					G_bprint(2, "Players may fire before match\n");
				 else
					G_sprint(self, 2, "Players may fire before match\n");

				 break;
		
		case  2: if ( !match_in_progress ) {
            		G_bprint(2, "Players may fire and jump when %s\n", redtext("ready"));
					PlayersStopFire();
				 }
				 else
					G_sprint(self, 2, "Players may fire and jump when %s\n", redtext("ready"));

				 break;
		case  0:
		default: if ( !match_in_progress ) {
            		G_bprint(2, "Players may %s fire before match\n", redtext("not"));
					PlayersStopFire();
				 }
				 else
					G_sprint(self, 2, "Players may %s fire before match\n", redtext("not"));

				 break;
	}

	cvar_fset( "k_prewar", k_prewar );
}

void ToggleMapLock ()
{
    float tmp;

    if( !is_adm( self ) )
        return;

    tmp = cvar( "k_lockmap" );

    if( tmp )
    {
		cvar_fset( "k_lockmap", 0 );

        if( !match_in_progress )
            G_bprint(2, "%s unlocks map\n", self->s.v.netname);
        else
            G_sprint(self, 2, "Map unlocked\n");

        return;
    }

	cvar_fset( "k_lockmap", 1 );

    if( !match_in_progress )
        G_bprint(2, "%s locks map\n", self->s.v.netname);
    else
        G_sprint(self, 2, "Map is locked\n");
}

void ToggleMaster ()
{
    float f1;

    if( !is_adm( self ) )
        return;

    f1 = !cvar( "k_master" );

    if( f1 )
        G_bprint(2, "%s sets mastermode!\n"
					"Players may %s alter settings\n", self->s.v.netname, redtext("not"));
    else
        G_bprint(2, "Mastermode disabled by %s\n"
					"Players %s now alter settings\n", self->s.v.netname, redtext("can"));

	cvar_fset( "k_master", f1 );
}

void ToggleFallBunny ()
{
    if( match_in_progress )
        return;

	if( check_master() )
		return;

	if ( k_yawnmode ) {
		G_sprint(self, 2, "Command blocked because yawnmode is active\n");
		return;
	}

	cvar_toggle_msg( self, "k_fallbunny", redtext("fallbunny") );
}

void sv_lock ()
{
	int lock_time = 15;

	if ( !k_sv_locktime ) {
		G_bprint(2, "%s %s for %s seconds\n", 
				getname(self), redtext("locked server"), dig3(lock_time));
		k_sv_locktime = g_globalvars.time + lock_time;
	}
	else {
		G_bprint(2, "%s %s\n", getname(self), redtext("unlocked server"));
		k_sv_locktime = 0;
	}
}

// convienence command for ctf admins
// often times you play a game on non-symmetrical map as one color then swap teams and play again to be fair
void AdminSwapAll()
{
	gedict_t *p;

	if ( !is_adm( self ) )
		return;

	if ( match_in_progress )
		return;

	if ( !isCTF() )
		return;

	for( p = world; (p = find_plr( p )); ) {
		if ( streq( getteam(p), "blue" ) )
        	stuffcmd_flags(p, STUFFCMD_IGNOREINDEMO, "team \"red\"\ncolor 4\n");
		else if ( streq( getteam(p), "red" ) )
			stuffcmd_flags(p, STUFFCMD_IGNOREINDEMO, "team \"blue\"\ncolor 13\n");
    }

	G_bprint(2, "%s swapped the teams\n", getname( self ) );
}

// assuming kicker is admin
qboolean is_can_forcespec(gedict_t *victim, gedict_t *kicker)
{
	if ( VIP_IsFlags(victim, VIP_NOTKICKABLE) && !is_real_adm(kicker) ) {
		G_sprint(kicker, 2, "You can't force_spec VIP \x8D %s as elected admin\n", 
					(strnull( victim->s.v.netname ) ? "!noname!" : victim->s.v.netname));
		return false;
	}
	if ( is_real_adm(victim) && !is_real_adm(kicker) ) {
		G_sprint(kicker, 2, "You can't force_spec real admin \x8D %s as elected admin\n", 
					(strnull( victim->s.v.netname ) ? "!noname!" : victim->s.v.netname));
		return false;
	}

	return true;
}

void do_force_spec(gedict_t *p, gedict_t *admin, qboolean spec)
{
	if (!is_can_forcespec(p, admin))
		return;

	G_sprint(p, 2, "You were forced to reconnect as %s by the admin\n", spec ? "spectator" : "player");
	stuffcmd_flags(p, STUFFCMD_IGNOREINDEMO, spec ? "spectator 1\n" : "spectator \"\"\n");

	if ( !strnull( ezinfokey(p, "Qizmo") ) ) 
		stuffcmd_flags(p, STUFFCMD_IGNOREINDEMO, "say ,:dis\nwait;wait;wait; say ,:reconnect\n");
	else 
		stuffcmd_flags(p, STUFFCMD_IGNOREINDEMO, "disconnect\nwait;wait;reconnect\n");
}

// ktpro (c)
void force_spec()
{
	qboolean found = false;
	gedict_t *p = NULL;
	char *c_fs, arg_2[1024];
	int i_fs, argc = trap_CmdArgc();

	if ( !is_adm( self ) )
		return;

	trap_CmdArgv( 1, arg_2, sizeof( arg_2 ) );
	c_fs = (argc >= 2 ? arg_2 : ezinfokey(self, "fs"));

	if ( strnull( c_fs ) ) {
		G_sprint(self, 2, "set setinfo \"fs\" properly\n");
		G_sprint(self, 2, "to force spec all not ready players\n");
		G_sprint(self, 2, "type: %s\n", redtext("setinfo fs \"*\""));
		G_sprint(self, 2, "or: %s to force spec specified player\n", redtext("setinfo fs \"playername\""));
		G_sprint(self, 2, "or just: %s\n", redtext("/force_spec \"playername\""));
		return;
	}

	if ( streq( c_fs, "*") || streq( c_fs, "* ") ) {
		//ok move all not ready players to specs
		for( p = world; (p = find_plr( p )); ) {
			if ( p->ready || p == self )
				continue;

			found = true;
			do_force_spec(p, self, true);
		}
	}
	else {
		p = ( (i_fs = atoi( c_fs ) ) < 0 ? spec_by_id( -i_fs ) : SpecPlayer_by_IDorName( c_fs ));
		if ( p ) {
			found = true;
			do_force_spec(p, self, p->ct != ctSpec);
		}
	}

	if ( !found ) 
		G_sprint(self, 2, "can't find specified players\n");
}

