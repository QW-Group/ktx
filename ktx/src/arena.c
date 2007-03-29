/*
 * Copyright (C) 1997 David 'crt' Wright
 *
 * $Id: arena.c,v 1.12 2007/03/29 22:45:24 qqshka Exp $
 */

// arena.c - rocket arena stuff

#include "g_local.h"

void k_respawn( gedict_t *p );

void setnowep( gedict_t *anent );

void SetWinner( gedict_t *p );
void SetLoser( gedict_t *p );
void SetNone( gedict_t *p );

// { ra queue stuff

gedict_t	*ra_que[MAX_CLIENTS];

void ra_init_que()
{
	memset(ra_que, 0, sizeof(ra_que));
}

// return first element in ra queue, return NULL if queue empty
gedict_t *ra_que_first()
{
	return ra_que[0];
}

// add element to end of ra queue
void ra_in_que( gedict_t *p )
{
	int i;

	if ( !p )
		G_Error ("ra_in_que: null");

	for ( i = 0; i < MAX_CLIENTS; i++ )
		if ( !ra_que[i] ) { // ok, we found empty space, add
			ra_que[i] = p;
			p->ra_pt = raQue; // mark player - he in queue now

			if ( CountPlayers() > 2 ) // if only two players, no reason of such messages
				G_sprint(p, PRINT_HIGH, "You are %s in line\n", (!i ? "next": "last"));

			return;
		}

	G_Error ("ra_in_que: full");
}

// remove element from ra queue 
void ra_out_que( gedict_t *p )
{
	int i;

	if ( !p )
		G_Error ("ra_out_que: null");

	for ( i = 0; i < MAX_CLIENTS; i++ )
		if ( ra_que[i] == p ) { // ok, we found element
			SetNone( p ); // mark player - he NOT in queue now
			for ( ; i < MAX_CLIENTS && ra_que[i]; i++ ) // move along other elements
				ra_que[i] = ( (i + 1 < MAX_CLIENTS) ? ra_que[i + 1] : NULL );

			return;
		}

	G_Error ("ra_out_que: not found");
}

// check if element is in ra queue
qboolean ra_isin_que( gedict_t *p )
{
	int i;

	if ( !p )
		G_Error ("ra_isin_que: null");

	for ( i = 0; i < MAX_CLIENTS && ra_que[i]; i++ )
		if ( ra_que[i] == p ) // ok, we found
			return true;

	return false;
}

// return element postion in ra queue, -1 if not in que
int ra_pos_que( gedict_t *p )
{
	int i;

	if ( !p )
		G_Error ("ra_pos_que: null");

	for ( i = 0; i < MAX_CLIENTS && ra_que[i]; i++ )
		if ( ra_que[i] == p ) // ok, we found
			return i;

	return -1;
}

// }

// ra is just modificator of duel
qboolean isRA( )
{
	return ( isDuel() && k_rocketarena );
}

qboolean isWinner( gedict_t *p )
{
	return ( p->ra_pt == raWinner );
}

qboolean isLoser( gedict_t *p )
{
	return ( p->ra_pt == raLoser );
}

gedict_t *getWinner()
{
	gedict_t *p;

	for( p = g_edicts + 1; p <= g_edicts + MAX_CLIENTS; p++ )
		if ( p->ct == ctPlayer && isWinner( p ) )
			return p;

	return NULL;
}

gedict_t *getLoser()
{
	gedict_t *p;

	for( p = g_edicts + 1; p <= g_edicts + MAX_CLIENTS; p++ )
		if ( p->ct == ctPlayer && isLoser( p ) )
			return p;

	return NULL;
}

void SetWinner( gedict_t *p )
{
	p->ra_pt = raWinner;
}

void SetLoser( gedict_t *p )
{
	p->ra_pt = raLoser;
}

void SetNone( gedict_t *p )
{
	p->ra_pt = raNone;
}

void ra_Precache()
{
	if ( !isRA() )
		return;

	trap_precache_sound("ra/1.wav");
	trap_precache_sound("ra/2.wav");
	trap_precache_sound("ra/3.wav");
	trap_precache_sound("ra/excelent.wav");
	trap_precache_sound("ra/fight.wav");
	trap_precache_sound("ra/flawless.wav");
}

void ra_ClientDisconnect()
{
	gedict_t *p = NULL;

	if ( !isRA() )
		return;

	if ( isWinner( self ) )
	{
		G_bprint (PRINT_HIGH, "The %s has left\n", redtext("winner"));
		if ( (p = getLoser()) )
			p->s.v.takedamage = DAMAGE_NO; // no damage to loser since winner is left game

		ra_match_fight = 0;
	}
	else if ( isLoser( self ) )
	{
		G_bprint (PRINT_HIGH, "The %s has left\n", redtext("challenger"));
		if ( (p = getWinner()) )
			p->s.v.takedamage = DAMAGE_NO; // no damage to winner since loser is left game

		ra_match_fight = 0;
	}
	else if ( self == ra_que_first() )
		G_bprint (PRINT_HIGH, "The %s has left\n", redtext("line leader"));

	if ( ra_isin_que( self ) ) // in the queue
		ra_out_que( self );	// remove from queue

	SetNone( self );
}

void ra_ClientObituary( gedict_t *targ, gedict_t *attacker )
{
	int ah, aa;
	gedict_t *loser, *winner;

	if ( !isRA() )
		return;

	if ( targ->ct != ctPlayer )
		return; // so below targ is player

	ra_match_fight = 0;

	if ( attacker->ct != ctPlayer )
		attacker = targ; // seems killed self

	if ( (loser = getLoser()) ) // stop them from attacking during countdown
	{
		loser->s.v.takedamage = DAMAGE_NO;
		PlayerStopFire( loser );
	}

	if ( (winner = getWinner()) ) // stop them from attacking during countdown
	{
		winner->s.v.takedamage = DAMAGE_NO;
		PlayerStopFire( winner );
	}

	if ( !loser || !winner ) {
		// just wanna know is that possible somehow
		if ( !loser )
			G_bprint (PRINT_HIGH, "BUG: ra_ClientObituary without loser\n");
		if ( !winner )
			G_bprint (PRINT_HIGH, "BUG: ra_ClientObituary without winner\n");

		return; // require both
	}

	ah = attacker->s.v.health;
	aa = attacker->s.v.armorvalue;

	if ( targ == winner )
	{
		winner->ps.loses += 1;
		loser->ps.wins   += 1;
		
		G_bprint (PRINT_HIGH, "The %s %s has been defeated\n", redtext("winner"), getname(winner));

		if ( targ == attacker )
		{
			G_bprint (PRINT_HIGH, "by %s!\n", g_himself(winner));
			winner->s.v.frags -= 1;
		}

		ra_in_que( winner ); // move to que winner
		setfullwep( loser );
	}
	else if ( targ == loser )
	{
		loser->ps.loses += 1;
		winner->ps.wins += 1;

		G_bprint (PRINT_HIGH, "The %s %s has failed\n", redtext("challenger"), getname(loser));

		if ( targ == attacker )
		{
			G_bprint (PRINT_HIGH, "because %s became bored with life!\n", g_he(loser));
			loser->s.v.frags -= 1;
		}

		ra_in_que( loser ); // move to que loser
		setfullwep( winner );
	}
	else {
		// just wanna know is that possible somehow
		G_bprint (PRINT_HIGH, "BUG: ra_ClientObituary unknown targ\n");
	}

	if ( attacker->ct == ctPlayer )
	{
		if ( attacker != targ )
		{
			if ( ah == 100 && aa == 200 )
			{
				G_bprint (PRINT_HIGH, "%s\n", redtext("FLAWLESS Victory!"));
				sound (world, CHAN_AUTO + CHAN_NO_PHS_ADD, "ra/flawless.wav", 1, ATTN_NONE);
			}
			else
			{
				G_bprint (PRINT_HIGH, "%s %s \x90%s\x91 %s \x90%s\x91 %s\n",
					redtext(getname(attacker)), redtext("had"), dig3(ah), redtext("health and"), dig3(aa), 
								redtext("armor left"));

				if ( ah >= 75 && aa >= 100 )
					sound (world, CHAN_AUTO + CHAN_NO_PHS_ADD, "ra/excelent.wav", 1, ATTN_NONE);
			}

			attacker->s.v.frags += 1;
		}

		logfrag (attacker, targ);
	}
}   

void ra_PutClientInServer()
{
	if ( !isRA() )
		return;

	setnowep( self ); // basic shit, even for qued player

	if ( isWinner( self ) || isLoser( self ) ) {
		VectorScale( g_globalvars.v_forward, 300, self->s.v.velocity );
		setfullwep( self ); // shit for winner or loser
	}
}

void setnowep( gedict_t *anent )
{
	gedict_t *swap;

	anent->s.v.ammo_shells	= 10; // wtf 10 ?
	anent->s.v.ammo_nails	= 0;
	anent->s.v.ammo_shells	= 0;
	anent->s.v.ammo_rockets	= 0;
	anent->s.v.ammo_cells	= 0;
	anent->s.v.takedamage	= DAMAGE_NO;
	anent->s.v.items		= IT_AXE;

	anent->s.v.armorvalue	= 0;
	anent->s.v.armortype	= 0;
	anent->s.v.health		= 100;
	anent->s.v.weapon		= IT_AXE;
	anent->idletime			= 0;
	anent->lasttime			= 0;
	anent->laststattime		= 0;

	swap = self;
	self = anent;

	// drop down to best weapon actually hold
	if ( !( (int)self->s.v.weapon & (int)self->s.v.items ) )
		self->s.v.weapon = W_BestWeapon();

	W_SetCurrentAmmo ();
	self = swap;
}

void setfullwep( gedict_t *anent )
{
	gedict_t *swap;
	qboolean add = (match_start_time == g_globalvars.time ? false : true);

// ammo
	anent->s.v.ammo_nails   = min(200, (add ? anent->s.v.ammo_nails : 0)   + 80);
	anent->s.v.ammo_shells  = min(100, (add ? anent->s.v.ammo_shells : 0)  + 30);
	anent->s.v.ammo_rockets = min(100, (add ? anent->s.v.ammo_rockets : 0) + 30);
	anent->s.v.ammo_cells   = min(100, (add ? anent->s.v.ammo_cells : 0)   + 30);
// weapons
	anent->s.v.items = IT_AXE | IT_SHOTGUN | IT_SUPER_SHOTGUN | IT_NAILGUN | IT_SUPER_NAILGUN |
					   IT_GRENADE_LAUNCHER | IT_ROCKET_LAUNCHER | IT_LIGHTNING;
// armor
	anent->s.v.items = (int)anent->s.v.items | IT_ARMOR3;
	anent->s.v.armorvalue = 200;
	anent->s.v.armortype = 0.8;
// health
	anent->s.v.health = 100;
// powerups
	anent->super_damage_finished = 0;
	anent->super_time			 = 0;
	anent->radsuit_finished		 = 0;
	anent->rad_time				 = 0;
	anent->invisible_finished	 = 0;
	anent->invisible_time		 = 0;
	anent->invincible_finished	 = 0;
	anent->invincible_time		 = 0;
	anent->k_666	   = 0;	// team
	anent->s.v.effects = 0;
	anent->lastwepfired = 0;

	adjust_pickup_time( &anent->q_pickup_time, &anent->ps.itm[itQUAD].time );
	adjust_pickup_time( &anent->p_pickup_time, &anent->ps.itm[itPENT].time );
	adjust_pickup_time( &anent->r_pickup_time, &anent->ps.itm[itRING].time );

	swap = self;
	self = anent;

	// drop down to best weapon actually hold
	if ( !( (int)self->s.v.weapon & (int)self->s.v.items ) )
		self->s.v.weapon = W_BestWeapon();

	W_SetCurrentAmmo ();
	self = swap;
}

qboolean readytostart()
{
	if ( !isRA() || ( time_to_start && g_globalvars.time > time_to_start && getWinner() && getLoser() ) )
		return true;
	else
		return false;
}

void PrintStats( gedict_t *who )
{
	int i, owner = EDICT_TO_PROG( who );
	char buf[1024] = {0};
	gedict_t *winner = getWinner(), *loser = getLoser(), *motd;

	if ( !winner || !loser )
		return;

	for( motd = world; (motd = find(motd, FOFCLSN, "motd")); )
		if ( owner == motd->s.v.owner )
			break; // no centerprint stats while have motd

	if ( (i = iKey( who, "lra" )) > 0 ) {
		i = bound(0, i, sizeof(buf)-1 );
		memset( (void*)buf, (int)'\n', i);
		buf[i] = 0;
	}

	strlcat(buf, va("%s%.10s %3d:%3d       %.10s %3d:%3d           ", buf, 
				getname(winner), (int)winner->s.v.armorvalue, (int)winner->s.v.health,
				getname(loser),  (int)loser->s.v.armorvalue,  (int)winner->s.v.health), sizeof(buf));

	if ( (i = iKey( who, "lra" )) < 0 ) {
		int offset = strlen(buf);
		i = bound(0, -i, (int)sizeof(buf) - offset - 1);
		memset( (void*)(buf + offset), (int)'\n', i);
		buf[i+offset] = 0;
	}

	if ( !motd && match_in_progress != 1 )
		G_centerprint(who, "%s", buf);

	if ( winner->s.v.health > 0 )
		who->s.v.armorvalue = winner->s.v.health;
	if ( loser->s.v.health > 0 )
		who->s.v.health = loser->s.v.health;

	who->s.v.currentammo = ra_pos_que( who ) + 1;
	who->laststattime = g_globalvars.time + PLAYERSTATTIME;
}

void ra_Frame ()
{
	static int last_r;

	int	r;
	gedict_t *winner, *loser;

	if ( !isRA() || match_over )
		return;

 	winner = getWinner();
	loser  = getLoser();

	if ( !winner || !loser ) {
		ra_match_fight = 0;

		if ( !winner && loser ) { // promote loser since winner not present for some reason
			winner = loser;
			loser  = NULL;
			G_bprint (PRINT_HIGH, "The new %s is %s\n", redtext("winner"), getname(winner));
			SetWinner( winner );
		}
    
		if ( !winner && (winner = ra_que_first()) ) { // still lack of winner
			ra_out_que( winner );
			G_bprint (PRINT_HIGH, "The new %s is %s\n", redtext("winner"), getname(winner));
			SetWinner( winner );
			k_respawn( winner ); // respawn player
		}
    
		if ( !loser && (loser = ra_que_first()) ) { // lack of loser
			ra_out_que( loser );
			G_bprint (PRINT_HIGH, "The new %s is %s\n", redtext("challenger"), getname(loser));
			SetLoser( loser );
			k_respawn( loser ); // respawn player
		}

		if ( !winner || !loser )
			return;
	}

	if ( ra_match_fight == 2 || match_in_progress != 2 )
		return;

	if ( !ra_match_fight ) { // ok start ra timer
		ra_match_fight = 1; // ra countdown
		last_r = 99999;
		time_to_start  = g_globalvars.time + 10;
	}

	r = Q_rint(time_to_start - g_globalvars.time);

	if ( r <= 0 )
	{
		char *fight = redtext("FIGHT!");
		gedict_t *first = ra_que_first();

		sound (world, CHAN_AUTO + CHAN_NO_PHS_ADD, "ra/fight.wav", 1, ATTN_NONE);
		G_bprint (PRINT_HIGH, "%s vs. %s\n", getname(winner), getname(loser));

		if ( first )
			G_bprint (PRINT_HIGH, "%s is next in line\n", getname(first));

		winner->s.v.takedamage = DAMAGE_AIM;
		loser->s.v.takedamage  = DAMAGE_AIM;

		G_centerprint (winner, "%s", fight);
		G_centerprint (loser,  "%s", fight);

		ra_match_fight = 2;
	}
	else if ( r != last_r )
	{
		last_r = r;

		if ( r <= 3 )
			sound (world, CHAN_AUTO + CHAN_NO_PHS_ADD, va("ra/%d.wav", r), 1, ATTN_NONE);

		G_centerprint (winner, "%s\n\n%d", getname(loser),  r);
		G_centerprint (loser,  "%s\n\n%d", getname(winner), r);
	}
}

void RocketArenaPre()
{
	if ( !isRA() )
		return;

	if ( self->idletime != 0 )
	{
		int r = Q_rint(self->idletime - g_globalvars.time);

		if ( r != self->lasttime )
		{
			self->lasttime = r;

			if ( r == 60 )
			{
				G_sprint (self, PRINT_HIGH,"You have 1 minute left\n"
										   "%s to get back in line\n", redtext("ra_break"));
				stuffcmd(self, "play player/axhit1.wav\n");

			}
			else if ( r == 30 )
			{
				G_sprint (self, PRINT_HIGH,"You have 30 seconds left\n"
										   "%s to get back in line\n", redtext("ra_break"));
				stuffcmd(self, "play player/axhit1.wav\n");
			}
			else if ( r > 0 && r <= 10 )
			{
				G_sprint (self, PRINT_HIGH,"You have %d second%s left to get in line\n", r, count_s( r ));
				stuffcmd(self, "play player/axhit1.wav\n");
			}
			else if ( r <= 0 )
			{
				self->idletime = 0;
				G_sprint (self, PRINT_HIGH,"Sorry, your wait time has expired!\n");
				G_bprint (PRINT_HIGH, "%s stood around too long\n", getname(self));

				stuffcmd(self, "play player/death1.wav\n");

				stuffcmd(self, "disconnect\n"); // FIXME: stupid way

				return;
			}

		}
	}

	if ( !isWinner( self ) && !isLoser( self ) && self->laststattime < g_globalvars.time  )
		PrintStats( self );
}

// { ra commands

void ra_PlayerStats()
{
	gedict_t *p;
	int i, pL = 0;

	if ( !isRA() )
		return;

	if( match_in_progress != 2 ) {
		G_sprint(self, 2, "no game - no statistics\n");
		return;
	}

	for ( p = world; (p = find_plr( p )); )
		pL = max(pL, strlen(p->s.v.netname));

	pL = bound( strlen("Name"), pL, 10 );

	G_sprint(self, 2, "%s:\n"
					  "%.10s", redtext("Player statistics"), redtext("Name"));
	for ( i = strlen("Name"); i < pL; i++ )
		G_sprint(self, 2, " "); // dynamically pad name
	G_sprint(self, 2,  " %s %s %s \217   %s\245\n", redtext("Frags"), redtext("Wins"), redtext("Loses"), redtext("Effi"));

	G_sprint(self, 2, "\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236"
					  "\236\236\236\236");
	for ( i = 0; i < pL; i++ )
		G_sprint(self, 2, "\236"); // dynamically pad name
	G_sprint(self, 2, "\237\n");

	for ( p = world; (p = find_plr( p )); ) {
		G_sprint(self, 2, "%.10s", p->s.v.netname); // player name
		for ( i = strlen(p->s.v.netname); i < pL; i++ )
			G_sprint(self, 2, " "); // dynamically pad name

		G_sprint(self, 2, " %5d", (int)p->s.v.frags); // Frags
		G_sprint(self, 2, " %4d", p->ps.wins);        // Wins
		G_sprint(self, 2, " %5d", p->ps.loses);       // Loses

		p->efficiency = ( ( p->ps.loses + p->ps.wins ) ? ( p->ps.wins * 100.0f ) / ( p->ps.loses + p->ps.wins ) : 0 );
		G_sprint(self, 2, " \217  %6.1f\n", p->efficiency); // effi
	}
}

void ra_PrintPos()
{
	int pos;

	if ( !isRA() || isWinner( self ) || isLoser( self ) )
		return;

	if ( (pos = ra_pos_que( self )) < 0 ) {
		G_sprint (self, PRINT_HIGH,"You are out of line\n"
								   "%s to return\n", redtext("ra_break"));
		return;
	}

	if ( !pos )	{
		G_sprint (self, PRINT_HIGH,"You are next\n");
		return;
	}

	if ( pos == 1 ) {
		G_sprint (self, PRINT_HIGH,"There is 1 person ahead of you\n");
		return;
	}

	if ( pos > 1 ) {
		G_sprint (self, PRINT_HIGH,"There are %d people ahead of you\n", pos);
		return;
	}
}

void ra_break()
{
	if ( !isRA() || isWinner( self ) || isLoser( self ) )
		return;

	if ( ra_isin_que( self ) ) // take OUT of line
	{
		G_sprint (self, PRINT_HIGH,"You can have up to a 5 minute break\n"
								   "%s to get back in line\n", redtext("ra_break"));	

		self->idletime = g_globalvars.time + MAXIDLETIME;
		ra_out_que( self );
	}
	else // put INTO line
	{
		self->idletime = 0;
		ra_in_que( self );
	}
}

// }

