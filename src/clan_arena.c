//
// Clan Arena related
//

#include "g_local.h"

static int round_num;
static int team1_score;
static int team2_score;

qbool is_rules_change_allowed( void );

void SM_PrepareCA(void)
{
	if( !isCA() )
		return;

	team1_score = team2_score = 0;
	round_num = 1;
}

int CA_wins_required(void)
{
	int k_clan_arena_rounds = bound(3, cvar("k_clan_arena_rounds"), 101);
	k_clan_arena_rounds += (k_clan_arena_rounds % 2) ? 0 : 1;
	return (k_clan_arena_rounds + 1)/2;
}

qbool isCA( )
{
	return ( isTeam() && cvar("k_clan_arena") );
}

// hard coded default settings for CA
static char ca_settings[] =
	"k_clan_arena_rounds 9\n"
	"dp 0\n"
	"teamplay 4\n"
	"deathmatch 5\n"
	"k_overtime 0\n"
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
	if ( !is_rules_change_allowed() )
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

	// set CA self params
	if ( match_in_progress == 2 )
	{
		int items;

		self->s.v.ammo_nails   = 120;
		self->s.v.ammo_shells  = 40;
		self->s.v.ammo_rockets = 20;
		self->s.v.ammo_cells   = 35;

		self->s.v.armorvalue   = 200;
		self->s.v.armortype    = 0.8;
		self->s.v.health       = 100;

		items = 0;
		items |= IT_AXE;
		items |= IT_SHOTGUN;
		items |= IT_NAILGUN;
		items |= IT_SUPER_NAILGUN;
		items |= IT_SUPER_SHOTGUN;
		items |= IT_ROCKET_LAUNCHER;
		items |= IT_GRENADE_LAUNCHER;
		items |= IT_LIGHTNING;
		items |= IT_ARMOR3; // add red armor

		self->s.v.items = items;

		// { remove invincibility/quad if any
		self->invincible_time = 0;
		self->invincible_finished = 0;
		self->super_time = 0;
		self->super_damage_finished = 0;
		// }

		// default to spawning with rl
		self->s.v.weapon = IT_ROCKET_LAUNCHER;
	}

	// set to ghost if dead
	if ( ISDEAD( self ) )
	{
		self->s.v.solid		 = SOLID_NOT;
		self->s.v.movetype	 = MOVETYPE_NOCLIP;
		self->vw_index		 = 0;
		setmodel( self, "" );

		setorigin (self, PASSVEC3( self->s.v.origin ) );
	}
}

qbool CA_can_fire( gedict_t *p )
{
	if ( !p )
		return false;

	if ( !isCA() )
		return true;

	return ( ISLIVE( p ) && ra_match_fight == 2 && time_to_start && g_globalvars.time >= time_to_start );
}

// return 0 if there no alive teams
// return 1 if there one alive team and alive_team point to 1 or 2 wich refering to _k_team1 or _k_team2 cvars
// return 2 if there at least two alive teams
static int CA_check_alive_teams( int *alive_team )
{
	gedict_t *p;
	qbool few_alive_teams = false;
	char *first_team = NULL;

	if ( alive_team )
		*alive_team = 0;

	for( p = world; (p = find_plr( p )); )
	{
		if ( !first_team )
		{
			if ( ISLIVE( p ) )
				first_team = getteam( p ); // ok, we found first team with alive players

			continue;
		}

		if ( strneq( first_team, getteam( p ) ) ) 
		{
			if ( ISLIVE( p ) )
			{
				few_alive_teams = true; // we found at least two teams with alive players
				break;
			}
		}
	}

	if ( few_alive_teams )
		return 2;

	if ( first_team )
	{
		if ( alive_team )
		{
			*alive_team = streq( first_team, cvar_string("_k_team1") ) ? 1 : 2;
		}
		return 1;
	}

	return 0;
}

void CA_PrintScores(void)
{
	int s1 = team1_score;
	int s2 = team2_score;
	char *t1 = cvar_string( "_k_team1" );
	char *t2 = cvar_string( "_k_team2" );

	G_sprint(self, 2, "%s \x90%s\x91 = %s\n", redtext("Team"),
							 (s1 > s2 ? t1 : t2), dig3(s1 > s2 ? s1 : s2));
	G_sprint(self, 2, "%s \x90%s\x91 = %s\n", redtext("Team"),
							 (s1 > s2 ? t2 : t1), dig3(s1 > s2 ? s2 : s1));
}

void CA_TeamsStats(void)
{
	if (team1_score != team2_score)
	{
		G_bprint( 2, "%s \x90%s\x91 wins %d to %d\n", redtext("Team"),
			cvar_string(va("_k_team%d", team1_score > team2_score ? 1 : 2)),
			team1_score > team2_score ? team1_score : team2_score,
			team1_score > team2_score ? team2_score : team1_score);
	}
	else
	{
		G_bprint( 2, "%s have equal scores %d\n", redtext("Teams"), team1_score );
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
		int alive_team = 0;

		switch ( CA_check_alive_teams( &alive_team ) )
		{
		case 0: // DRAW, both teams are dead
			{
				G_bprint(2, "%s\n", redtext("draw"));
				round_num++;
				ra_match_fight = 0;

				break;
			}
		case 1: // Only one team alive
			{
				G_bprint(2, "%s \x90%s\x91 wins round\n", redtext("Team"), cvar_string(va("_k_team%d", alive_team)));
				if ( alive_team == 1 )
				{
					team1_score++;
				}
				else
				{
					team2_score++;				
				}
				round_num++;
				ra_match_fight = 0;

				break;
			}
		default: break; // both teams alive
		}

		if ( ra_match_fight == 2 )
			return;
	}

	if ( team1_score >= CA_wins_required() || team2_score >= CA_wins_required() )
	{
		EndMatch( 0 );
		return;
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
			G_cp2all("%d\n\n"
				"\x90%s\x91:%s \x90%s\x91:%s", 
				r, cvar_string("_k_team1"), dig3(team1_score), cvar_string("_k_team2"), dig3(team2_score)); // CA_wins_required
		}
	}
}
