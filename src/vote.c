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

// vote.c: election functions by rc\sturm

#include "g_local.h"

void	BeginPicking();
void	BecomeCaptain(gedict_t *p);

// AbortElect is used to terminate the voting
// Important if player to be elected disconnects or levelchange happens
void AbortElect()
{
	gedict_t *p;

	for( p = world; (p = find_client( p )); ) {
		if ( p->v.elect_type != etNone ) {
			if ( is_elected(p, etCaptain) )
				k_captains = floor( k_captains );

			p->v.elect_type = etNone;
			p->v.elect_block_till = g_globalvars.time + 30; // block election for some time
		}
	}

	vote_clear( OV_ELECT ); // clear vote

// Kill timeout checker entity
	for( p = world; (p = find(p, FOFCLSN, "electguard")); ) 
		ent_remove( p );
}

void ElectThink()
{
	G_bprint(2, "The voting has timed out.\n"
				"Election aborted\n");
	self->s.v.nextthink = -1;

	AbortElect();
}

void VoteYes()
{
	int votes;

	if( !get_votes( OV_ELECT ) )
		return;

	if( self->v.elect_type != etNone ) {
		G_sprint(self, 2, "You cannot vote for yourself\n");
		return;
	}

	if( self->v.elect ) {
		G_sprint(self, 2, "--- your vote is still good ---\n");
		return;
	}

// register the vote
	self->v.elect = 1;

	G_bprint(2, "%s gives %s vote\n", self->netname, g_his( self ));

// calculate how many more votes are needed
	if ( (votes = get_votes_req( OV_ELECT, true )) )
		G_bprint(2, "\x90%d\x91 more vote%s needed\n", votes, count_s( votes ));

	vote_check_elect ();
}

void VoteNo()
{
	int votes;

// withdraw one's vote
	if( !get_votes( OV_ELECT ) || self->v.elect_type != etNone || !self->v.elect )
		return;

// unregister the vote
	self->v.elect = 0;

	G_bprint(2, "%s withdraws %s vote\n", self->netname, g_his( self ));

// calculate how many more votes are needed
	if ( (votes = get_votes_req( OV_ELECT, true )) )
		G_bprint(2, "\x90%d\x91 more vote%s needed\n", votes, count_s( votes ));

	vote_check_elect ();
}

// get count of particular votes

int get_votes( int fofs )
{
	int votes = 0;
	gedict_t *p;

	for ( p = world; (p = find_client( p )); )
		if ( *(int*)((byte*)(&p->v)+fofs) )
			votes++;

	return votes;
}

// get count of particular votes and filter by value

int get_votes_by_value( int fofs, int value )
{
	int votes = 0;
	gedict_t *p;

	for ( p = world; (p = find_client( p )); )
		if ( *((int*)(&(p->v)+fofs)) == value )
			votes++;

	return votes;
}

int get_votes_req( int fofs, qbool diff )
{
	float percent = 51;
	int   votes, vt_req, idx, el_type;

	votes   = get_votes( fofs );

	switch ( fofs ) {
		case OV_BREAK:   percent = cvar(k_matchLess ? "k_vp_map" : "k_vp_break"); break; // in matchless mode there is no /break but /next_map so using "k_vp_map"
		case OV_PICKUP:  percent = cvar("k_vp_pickup"); break;
		case OV_RPICKUP: percent = cvar("k_vp_rpickup"); break;
		case OV_MAP:
					    percent = cvar("k_vp_map");
						idx = vote_get_maps ();
						if ( idx >= 0 && !strnull( GetMapName(maps_voted[idx].map_id) ) )
							votes = maps_voted[idx].map_votes;
						else
							votes = 0;
						break;
		case OV_ELECT:
						if ( (el_type = get_elect_type ()) == etAdmin ) {
							percent = cvar("k_vp_admin");
							break;
						}
						else if ( el_type == etCaptain ) {
							percent = cvar("k_vp_captain");
							break;
						}
						else {
							percent = 100; break; // unknown/none election
							break;
						}

						break;

		case OV_NOSPECS: percent = cvar("k_vp_nospecs"); break;
		case OV_TEAMOVERLAY: percent = cvar("k_vp_teamoverlay"); break;
		case OV_COOP:    percent = cvar("k_vp_coop"); break;
		case OV_ANTILAG: percent = cvar("k_vp_antilag"); break;
	}

	percent = bound(0.51, bound(51, percent, 100)/100, 1); // calc and bound percentage between 50% to 100%

	if (isRACE() && fofs == OV_MAP) {
		vt_req = race_count_votes_req(percent);
	}
	else {
		vt_req = ceil(percent * (CountPlayers() - CountBots()));
	}

	if ( fofs == OV_ELECT )
		vt_req = max(2, vt_req); // if election, at least 2 votes needed
	else if ( fofs == OV_BREAK && k_matchLess && match_in_progress == 1 )
		vt_req = max(2, vt_req); // at least 2 votes in this case
	else if ( fofs == OV_BREAK )
		vt_req = max(1, vt_req); // at least 1 vote in any case
	else if ( fofs == OV_RPICKUP )
		vt_req = max(3, vt_req); // at least 3 votes in this case
	else if ( fofs == OV_NOSPECS && cvar("_k_nospecs") )
		vt_req = max(1, vt_req); // at least 1 vote in this case
	else if ( fofs == OV_NOSPECS )
		vt_req = max(2, vt_req); // at least 2 votes in this case
	else if ( fofs == OV_TEAMOVERLAY )
		vt_req = max(2, vt_req); // at least 2 votes in this case
	else if ( fofs == OV_COOP )
		vt_req = max(1, vt_req); // at least 1 votes in this case
	else if ( fofs == OV_ANTILAG )
		vt_req = max(2, vt_req); // at least 2 votes in this case

	if (CountBots () > 0 && CountPlayers () - CountBots () == 1)
		vt_req = 1;

	if ( diff )
		return max(0, vt_req - votes );

	return max(0, vt_req - CountBots());
}

int is_admins_vote( int fofs )
{
	int votes = 0;
	gedict_t *p;

	for ( p = world; (p = find_client( p )); )
		if ( *(int*)((byte*)(&p->v)+fofs) && is_adm( p ) )
			votes++;

	return votes;
}

void vote_clear( int fofs )
{
	gedict_t *p;

	for ( p = world; (p = find_client( p )); )
		*(int*)((byte*)(&p->v)+fofs) = 0;
}

// return true if player invoke one of particular election
qbool is_elected(gedict_t *p, electType_t et)
{
	return (p->v.elect_type == et);
}

int get_elect_type ()
{
	gedict_t *p;

	for( p = world; (p = find_client( p )); ) {
		if( is_elected(p, etAdmin) ) // elected admin
			return etAdmin;

		if( is_elected(p, etCaptain) ) // elected captain
			return etCaptain;
	}

	return etNone;
}

char *get_elect_type_str ()
{

	switch ( get_elect_type () ) {
		case etNone: 	return "None";
		case etCaptain:	return "Captain";
		case etAdmin: 	return "Admin";
	}

	return "Unknown";
}


int     maps_voted_idx;

votemap_t maps_voted[MAX_CLIENTS];

// fill maps_voted[] with data,
// return the index in maps_voted[] of most voted map
// return -1 inf no votes at all or some failures
// if admin votes for map - map will be treated as most voted
int vote_get_maps ()
{
	int best_idx = -1, i;
	gedict_t *p;

	memset(maps_voted, 0, sizeof(maps_voted));
	maps_voted_idx = -1;

	if ( !get_votes( OV_MAP ) )
		return -1; // no one votes at all

	for( p = world; (p = find_client( p )); ) {

		if ( !p->v.map )
			continue; // player is not voted

		if (!race_allow_map_vote(p))
			continue;

		for ( i = 0; i < MAX_CLIENTS; i++ ) {
			if ( !maps_voted[i].map_id )
				break; // empty

			if ( maps_voted[i].map_id == p->v.map )
				break; // already count votes for this map
		}

		if ( i >= MAX_CLIENTS )
			continue; // heh, all slots is full, just for sanity

		maps_voted[i].map_id     = p->v.map;
		maps_voted[i].map_votes += 1;
		maps_voted[i].admins    += (is_adm( p ) ? 1 : 0);

		// find the most voted map
		if ( best_idx < 0 || maps_voted[i].map_votes > maps_voted[best_idx].map_votes )
			best_idx   = i;

		// admin voted maps have priority
		if ( maps_voted[i].admins > maps_voted[best_idx].admins )
			best_idx   = i;
	}

	return (maps_voted_idx = best_idx);
}

void vote_check_map ()
{
	int   vt_req = get_votes_req( OV_MAP, true );
	char  *m  = "";

	if ( maps_voted_idx < 0 || strnull( m = GetMapName(maps_voted[maps_voted_idx].map_id) ) )
		return;

	if ( !k_matchLess )
	if ( match_in_progress )
		return;

	if ( maps_voted[maps_voted_idx].admins )
		G_bprint(2, "%s\n", redtext("Admin veto"));
	else if( !vt_req  )
		G_bprint(2, "%s votes for mapchange.\n", redtext("Majority"));
	else
		return;

	vote_clear( OV_MAP );

	changelevel( m );
}

void vote_check_break ()
{
	if ( !match_in_progress || intermission_running || match_over )
		return;

	if( !get_votes_req( OV_BREAK, true ) ) {
		vote_clear( OV_BREAK );

		if (isHoonyModeAny()) {
			HM_match_break();
		}

		G_bprint(2, "%s\n", redtext("Match stopped by majority vote"));

		EndMatch( 0 );

		return;
	}
}

void vote_check_elect ()
{
	gedict_t *p;

	if( !get_votes_req( OV_ELECT, true ) ) {

		for( p = world; (p = find_client( p )); )
			if ( p->v.elect_type != etNone )
				break;

		if ( !p ) { // nor admin nor captain found - probably bug
			AbortElect();
			return;
		}

		if( !(p->ct == ctSpec && match_in_progress) )
		if( is_elected(p, etAdmin) ) // s: election was admin election
			BecomeAdmin(p, AF_ADMIN);

		if( !match_in_progress )
		if( is_elected(p, etCaptain) ) // s: election was captain election
			BecomeCaptain(p);

		AbortElect();
		return;
	}
}

// !!! do not confuse rpickup and pickup
void vote_check_pickup ()
{
	gedict_t *p;
	int veto;

	if ( match_in_progress || k_captains )
		return;

	if ( !get_votes( OV_PICKUP ) )
		return;

	veto = is_admins_vote( OV_PICKUP );

	if( veto || !get_votes_req( OV_PICKUP, true ) ) {
		vote_clear( OV_PICKUP );

		if ( veto )
			G_bprint(2, "console: admin veto for pickup\n");
		else
			G_bprint(2, "console: a pickup game it is then\n");

		for( p = world;	(p = find_plr( p )); ) {

			stuffcmd_flags(p, STUFFCMD_IGNOREINDEMO,
						"break\n"
						"color 0\n"
						"team \"\"\n"
						"skin base\n");
		}

		return;
	}
}

// !!! do not confuse rpickup and pickup
void vote_check_rpickup ()
{
	float frnd;
    int i, tn, pl_cnt, pl_idx;
	gedict_t *p;
	int veto;

	if ( match_in_progress || k_captains )
		return;

	if ( !get_votes( OV_RPICKUP ) )
		return;

   	// Firstly obtain the number of players we have in total on server
   	pl_cnt = CountPlayers();

	if ( pl_cnt < 4 )
		return;

	veto = is_admins_vote( OV_RPICKUP );

	if( veto || !get_votes_req( OV_RPICKUP, true ) ) {
		vote_clear( OV_RPICKUP );

		for( p = world; (p = find_plr( p )); )
			p->k_teamnumber = 0;

		for( tn = 1; pl_cnt > 0; pl_cnt-- ) {
			frnd = g_random(); // bound is macros - so u _can't_ put g_random inside bound
			pl_idx = bound(0, (int)( frnd * pl_cnt ), pl_cnt-1 ); // select random player between 0 and pl_cnt

			for( i = 0, p = world; (p = find_plr( p )); ) {
				if ( p->k_teamnumber )
					continue;

				if ( i == pl_idx ) {
					p->k_teamnumber = tn;
					tn = (tn == 1 ? 2 : 1); // next random player will be in other team

					if (p->k_teamnumber == 1) {
						if (p->isBot) {
							trap_SetBotUserInfo(NUM_FOR_EDICT(p), "team", "red", 0);
							trap_SetBotUserInfo(NUM_FOR_EDICT(p), "topcolor", "4", 0);
							trap_SetBotUserInfo(NUM_FOR_EDICT(p), "bottomcolor", "4", 0);
						}
						else {
							stuffcmd_flags(p, STUFFCMD_IGNOREINDEMO, "break\ncolor  4\nskin \"\"\nteam red\n");
						}
					}
					else {
						if (p->isBot) {
							trap_SetBotUserInfo(NUM_FOR_EDICT(p), "team", "blue", 0);
							trap_SetBotUserInfo(NUM_FOR_EDICT(p), "topcolor", "13", 0);
							trap_SetBotUserInfo(NUM_FOR_EDICT(p), "bottomcolor", "13", 0);
						}
						else {
							stuffcmd_flags(p, STUFFCMD_IGNOREINDEMO, "break\ncolor 13\nskin \"\"\nteam blue\n");
						}
					}

					break;
				}

				i++;
			}
		}

		if ( veto )
			G_bprint(2, "console: admin veto for %s\n", redtext("random pickup"));
		else
    		G_bprint(2, "console: %s game it is then\n", redtext("random pickup"));

		return;
	}
}

// { no specs feature

void FixNoSpecs( void )
{
	// turn off "no specs" mode if there no players left
	if ( g_globalvars.time > 10 && !match_in_progress && !CountPlayers() && cvar("_k_nospecs") )
	{
		G_bprint(2,	"%s mode turned off\n", redtext("No spectators"));
		cvar_set("_k_nospecs", "0");
	}
}

#define ALLOWED_NOSPECS_VIPS ( VIP_NOTKICKABLE | VIP_ADMIN | VIP_RCON )

qbool nospecs_canconnect( gedict_t *spec )
{
	if ( cvar("_k_nospecs") )
	{
		// some VIPS able to connect anyway
		if ( !( VIP( spec ) & ALLOWED_NOSPECS_VIPS ) )
		{
			return false;
		}
	}

	return true;
}

void vote_check_nospecs ()
{
	int veto;

	if ( match_in_progress || intermission_running || match_over )
		return;

	if ( !get_votes( OV_NOSPECS ) )
		return;

	veto = is_admins_vote( OV_NOSPECS );

	if( veto || !get_votes_req( OV_NOSPECS, true ) )
	{
		vote_clear( OV_NOSPECS );

		// set no specs mode
		cvar_fset("_k_nospecs", !cvar("_k_nospecs"));

		if ( veto )
			G_bprint(2, "%s\n", redtext(va("No spectators mode %s by admin veto", OnOff(cvar("_k_nospecs")))));
		else
			G_bprint(2, "%s\n", redtext(va("No spectators mode %s by majority vote", OnOff(cvar("_k_nospecs")))));

		// kick specs
		if ( cvar("_k_nospecs") )
		{
			gedict_t *spec;

			for ( spec = world; (spec = find_spc( spec )); )
			{
				if ( VIP( spec ) & ALLOWED_NOSPECS_VIPS )
					continue; // don't kick this VIP
    
				if ( is_real_adm(spec) )
					continue; // don't kick real admin
    
				stuffcmd(spec, "disconnect\n");  // FIXME: stupid way
			}
		}

		return;
	}
}

void nospecs( )
{
    int votes;
	
	if ( match_in_progress )
	{
        G_sprint(self, 2, "%s mode %s\n", redtext("No spectators"), OnOff(cvar("_k_nospecs")));
        return;
	}

	// admin may turn this status alone on server...
	if ( !is_adm( self ) )
	{
		// Dont need to bother if less than 2 players
		if ( CountPlayers() < 2 && !cvar("_k_nospecs") )
		{
			G_sprint(self, 2, "You need at least 2 players to do this.\n");
			return;
		}
	}

	self->v.nospecs = !self->v.nospecs;

	G_bprint(2, "%s %s!%s\n", self->netname, 
			(self->v.nospecs ? redtext(va("votes for nospecs %s", OnOff(!cvar("_k_nospecs")))) : 
							   redtext(va("withdraws %s nospecs vote", g_his(self)))),
			((votes = get_votes_req( OV_NOSPECS, true )) ? va(" (%d)", votes) : ""));

	vote_check_nospecs ();
}

void vote_check_teamoverlay ()
{
	int veto;

	if ( match_in_progress || intermission_running || match_over )
		return;

	if ( !get_votes( OV_TEAMOVERLAY ) )
		return;

	veto = is_admins_vote( OV_TEAMOVERLAY );

	if( veto || !get_votes_req( OV_TEAMOVERLAY, true ) )
	{
		vote_clear( OV_TEAMOVERLAY );

		// Toggle teamoverlay.
		cvar_fset("k_teamoverlay", !cvar("k_teamoverlay"));

		if ( veto )
			G_bprint(2, "%s\n", redtext(va("Teamoverlay %s by admin veto", OnOff(cvar("k_teamoverlay")))));
		else
			G_bprint(2, "%s\n", redtext(va("Teamoverlay %s by majority vote", OnOff(cvar("k_teamoverlay")))));

		return;
	}
}

void teamoverlay( )
{
    int votes;

    if ( match_in_progress )
    {
        G_sprint(self, 2, "%s %s\n", redtext("Teamoverlay"), OnOff(cvar("k_teamoverlay")));
        return;
    }

	// admin may turn this status alone on server...
	if ( !is_adm( self ) )
	{
		// Dont need to bother if less than 2 players
		if ( CountPlayers() < 2 )
		{
			G_sprint(self, 2, "You need at least 2 players to do this.\n");
			return;
		}
	}

	self->v.teamoverlay = !self->v.teamoverlay;

	G_bprint(2, "%s %s!%s\n", self->netname, 
			(self->v.teamoverlay ? redtext(va("votes for teamoverlay %s", OnOff(!cvar("k_teamoverlay")))) : 
							       redtext(va("withdraws %s teamoverlay vote", g_his(self)))),
			((votes = get_votes_req( OV_TEAMOVERLAY, true )) ? va(" (%d)", votes) : ""));

    vote_check_teamoverlay ();
}

// }

// { votecoop
void vote_check_coop ()
{
	int veto;

	if ( ( deathmatch && match_in_progress ) || intermission_running || match_over )
		return;

	if ( !get_votes( OV_COOP ) )
		return;

	veto = is_admins_vote( OV_COOP );

	if( veto || !get_votes_req( OV_COOP, true ) )
	{
		vote_clear( OV_COOP );

		// toggle coop mode
		cvar_fset( "coop", coop = !cvar("coop") );
		// set appropriate deathmatch
		cvar_fset( "deathmatch", deathmatch = !coop );

		if ( veto )
			G_bprint( 2, "%s\n", redtext(va("Coop mode %s by admin veto", OnOff(cvar("coop")))) );
		else
			G_bprint( 2, "%s\n", redtext(va("Coop mode %s by majority vote", OnOff(cvar("coop")))) );

		// and reload map
		if ( cvar("k_bloodfest") )
			changelevel( coop ? g_globalvars.mapname : cvar_string( "k_defmap" ) );
		else
			changelevel( coop ? "start" : g_globalvars.mapname );
			
		return;
	}
}

void votecoop( )
{
    int votes;

	if ( deathmatch && match_in_progress )
	{
        G_sprint(self, 2, "Match in progress and deathmatch is non zero, you can't vote for coop\n");
        return;
	}

	self->v.coop = !self->v.coop;

	G_bprint(2, "%s %s!%s\n", self->netname, 
			(self->v.coop ? redtext(va("votes for coop %s", OnOff(!cvar("coop")))) : 
							redtext(va("withdraws %s coop vote", g_his(self)))),
			((votes = get_votes_req( OV_COOP, true )) ? va(" (%d)", votes) : ""));

	vote_check_coop ();
}

// }

// { antilag vote feature

void vote_check_antilag ()
{
	int veto;

	if ( match_in_progress || intermission_running || match_over )
		return;

	if ( !get_votes( OV_ANTILAG ) )
		return;

	veto = is_admins_vote( OV_ANTILAG );

	if( veto || !get_votes_req( OV_ANTILAG, true ) )
	{
		vote_clear( OV_ANTILAG );

		// toggle antilag mode.
		trap_cvar_set_float( "sv_antilag", (float)(cvar( "sv_antilag" ) ? 0 : 2));

		if ( veto )
			G_bprint(2, "%s\n", redtext(va("Antilag mode %s by admin veto", OnOff(2 == cvar("sv_antilag")))));
		else
			G_bprint(2, "%s\n", redtext(va("Antilag mode %s by majority vote", OnOff(2 == cvar("sv_antilag")))));

		return;
	}
}

void antilag( )
{
    int votes;
	
	if ( match_in_progress )
	{
        G_sprint(self, 2, "%s mode %s\n", redtext("Antilag"), OnOff(2 == cvar("sv_antilag")));
        return;
	}

	// admin may turn this status alone on server...
	if ( !is_adm( self ) )
	{
		// Dont need to bother if less than 2 players
		if ( CountPlayers() < 2 )
		{
			G_sprint(self, 2, "You need at least 2 players to do this.\n");
			return;
		}
	}

	self->v.antilag = !self->v.antilag;

	G_bprint(2, "%s %s!%s\n", self->netname, 
			(self->v.antilag ? redtext(va("votes for antilag %s", OnOff(!(2 == cvar("sv_antilag"))))) : 
							   redtext(va("withdraws %s antilag vote", g_his(self)))),
			((votes = get_votes_req( OV_ANTILAG, true )) ? va(" (%d)", votes) : ""));

	vote_check_antilag ();
}

// }

void vote_check_all ()
{
	vote_check_map ();
	vote_check_break ();
	vote_check_elect ();
	vote_check_pickup ();
	vote_check_rpickup ();
	vote_check_nospecs ();
	vote_check_teamoverlay ();
	vote_check_coop ();
	vote_check_antilag ();
}

