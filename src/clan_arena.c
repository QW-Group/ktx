//
// Clan Arena related
//

#include "g_local.h"

qboolean isCA( )
{
	return ( isTeam() && cvar("k_clan_arena") );
}

// hard coded default settings for CA
static char ca_settings[] =
	"k_spw 1\n"
	"k_dmgfrags 1\n"
	"k_noitems 1\n";

void apply_CA_settings(void)
{
    char buf[1024*4];
	char *cfg_name;

	if ( !isCA() )
		return;

	trap_readcmd( ca_settings, buf, sizeof(buf) );
	G_cprint("%s", buf);

	cfg_name = va("configs/usermodes/ca/default.cfg");
	if ( can_exec( cfg_name ) )
	{
		trap_readcmd( va("exec %s\n", cfg_name), buf, sizeof(buf) );
		G_cprint("%s", buf);
	}

	cfg_name = va("configs/usermodes/ca/%s.cfg", g_globalvars.mapname);
	if ( can_exec( cfg_name ) )
	{
		trap_readcmd( va("exec %s\n", cfg_name), buf, sizeof(buf) );
		G_cprint("%s", buf);
	}

	G_cprint("\n");
}

void ToggleCArena()
{
	if ( match_in_progress )
		return;

	if( check_master() )
		return;

	if ( !isCA() )
	{
		// seems we trying turn CA on.
		if ( !isTeam() )
		{
			G_sprint(self, 2, "Set %s mode first\n", redtext("team"));
			return;
		}
	}

	cvar_toggle_msg( self, "k_clan_arena", redtext("Clan Arena") );

	apply_CA_settings();
}

void CA_PutClientInServer(void)
{
	if ( !isCA() )
		return;

	self->ca_alive = (ra_match_fight != 2);

	if ( !self->ca_alive )
	{
		self->s.v.solid		 = SOLID_NOT;
		self->s.v.movetype	 = MOVETYPE_NOCLIP;
		self->vw_index		 = 0;
		setmodel( self, "" );

		setorigin (self, PASSVEC3( self->s.v.origin ) );
	}
}

void CA_Frame(void)
{
	static int last_r;

	int     r;
	gedict_t *p;

	if ( !isCA() || match_over )
		return;

	if ( match_in_progress != 2 )
		return;

	// check if there exist only one team with alive players and others are eluminated, if so then its time to start ca countdown
	if ( ra_match_fight == 2 )
	{
		qboolean few_alive_teams = false;
		char *last_team = NULL;
		
		for( p = world; (p = find_plr( p )); )
		{
			if ( !last_team )
			{
				if ( p->ca_alive )
					last_team = getteam( p ); // ok, we found first team with alive players

				continue;
			}

			if ( strneq( last_team, getteam( p ) ) ) 
			{
				if ( p->ca_alive )
				{
					few_alive_teams = true; // we found at least two teams with alive players
					break;
				}
			}
		}

		if ( few_alive_teams )
			return;

		ra_match_fight = 0;
	}

	if ( !ra_match_fight )
	{
		// ok start ra timer
		ra_match_fight = 1; // ra countdown
		last_r = 999999999;
		time_to_start  = g_globalvars.time + 6;

		for( p = world; (p = find_plr( p )); )
		{
			k_respawn( p, false );
		}
	}

	r = Q_rint( time_to_start - g_globalvars.time );

	if ( r <= 0 )
	{
		char *fight = redtext("FIGHT!");

		// TODO replace RA sounds (but world precached)
		//sound (world, CHAN_AUTO + CHAN_NO_PHS_ADD, "ra/fight.wav", 1, ATTN_NONE);
		G_cp2all("%s", fight);

		ra_match_fight = 2;

		// rounding suck, so this force readytostart() return true right after FIGHT! is printed
		time_to_start = g_globalvars.time;
	}
	else if ( r != last_r )
	{
		last_r = r;

		if ( r < 6 )
		{
			// TODO replace RA sounds (but world precached)
			//sound (world, CHAN_AUTO + CHAN_NO_PHS_ADD, va("ra/%d.wav", r), 1, ATTN_NONE);
			for( p = world; (p = find_client( p )); )
				stuffcmd (p, "play buttons/switch04.wav\n");
		}

		if ( r < 11 )
		{
			G_cp2all("%d", r);
		}
	}
}
