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

// motd.c

#include "g_local.h"

void StuffMainMaps();

void PMOTDThink()
{
	int i;
	char buf[2048] = {0};
	char *s;

	// remove MOTD in some cases
	if( self->attack_finished < g_globalvars.time // expired
    	|| ( !k_matchLess && match_in_progress )  // non matchless and (match has began or countdown)
	   	|| ( k_matchLess && match_in_progress == 1 ) // matchless and countdown
		|| PROG_TO_EDICT( self->s.v.owner )->attack_finished > g_globalvars.time // player fire something, so he wanna play, not reading motd
	  ) {
		if ( self->attack_finished < g_globalvars.time )
			G_centerprint ( PROG_TO_EDICT( self->s.v.owner ), "");

		ent_remove( self );
		return;
	}

	if (    PROG_TO_EDICT( self->s.v.owner )->wp_stats || PROG_TO_EDICT( self->s.v.owner )->sc_stats
		 || PROG_TO_EDICT( self->s.v.owner )->shownick_time
	   ) {
		self->s.v.nextthink = g_globalvars.time + 1;  // do not interference with +wp_stats or +scores and shownick
		return;
	}

	for( i = 1; i <= MOTD_LINES; i++) {
		if ( strnull( s = cvar_string(va("k_motd%d", i)) ) )
			continue;
		
		strlcat(buf, s, sizeof(buf));
		strlcat(buf, "\n", sizeof(buf));
	}

 	// no "welcome" - if k_motd keys is present - because admin may wanna customize this
	if ( strnull( buf ) )
		strlcat(buf, "Welcome\n\n", sizeof(buf));

	strlcat(buf, "\n€‚\n\n", sizeof(buf));
	strlcat(buf, va("Running %s %s (build %s)\nby %s\n\n", redtext(MOD_NAME),
					dig3s(MOD_VERSION), dig3s("%d", build_number()), redtext("KTX development team")), sizeof(buf));
	strlcat(buf, va("Website: %s\n", redtext(MOD_URL)), sizeof(buf));
	//strlcat(buf, va("Based on %s\n", redtext("Kombat teams 2.21")), sizeof(buf));
// qqshka - this info can be found in /about command
//	strlcat(buf, "by kemiKal, Cenobite, Sturm and Fang\n\n", sizeof(buf));
	strlcat(buf, va("Type \"%s\" for help", redtext("commands")), sizeof(buf));

	G_centerprint ( PROG_TO_EDICT( self->s.v.owner ), "%s",  buf);

	self->s.v.nextthink = g_globalvars.time + 0.7;
}

void SMOTDThink()
{
	PMOTDThink(); // equal motd for player and spectator now
}

void MOTDThinkX()
{
	gedict_t *owner = PROG_TO_EDICT( self->s.v.owner );

	// FIXME: server work around, frags are not restored, ie showed as 0, force update frags manually
	if ( owner->s.v.frags && (int)( owner - world - 1 ) >= 0 && (int)( owner - world - 1 ) < MAX_CLIENTS )
	{
		int to = MSG_ALL;

		WriteByte(to, SVC_UPDATEFRAGS); // update frags
		WriteByte(to, (int)( owner - world - 1 ));
		WriteShort(to, owner->s.v.frags);
	}

// check if we are need to stuff aliases, or already done this
	if( !(PROG_TO_EDICT( self->s.v.owner )->k_stuff) )
	{
		gedict_t *p = spawn();

		p->s.v.classname = "motdX";
		p->s.v.owner = self->s.v.owner;
		p->cnt = -1;
    	p->s.v.think = ( func_t ) StuffMainMaps;
    	p->s.v.nextthink = g_globalvars.time + 0.1;
	}

	self->s.v.think = // select MOTD for spectator or player
		  ( func_t ) ( PROG_TO_EDICT( self->s.v.owner )->ct == ctSpec ? SMOTDThink : PMOTDThink );
	self->s.v.nextthink = g_globalvars.time + 0.3;

// remove motd if player already stuffed, because them probably sow motd already one time
	if( k_matchLess && PROG_TO_EDICT( self->s.v.owner )->k_stuff )
		ent_remove( self );
}

void MakeMOTD()
{
	gedict_t *motd;
	int i = bound(0, cvar("k_motd_time"), 30);

	motd = spawn();
	motd->s.v.classname = "motd";
	motd->s.v.owner = EDICT_TO_PROG( self );
	motd->s.v.think = ( func_t ) MOTDThinkX;
	motd->s.v.nextthink = g_globalvars.time + 0.1;
	motd->attack_finished = g_globalvars.time + (i ? i : ( k_matchLess ? 3 : 7 ));
}

void RemoveMOTD()
{
	gedict_t *motd;
	int owner = EDICT_TO_PROG( self );

	for( motd = world; (motd = find(motd, FOFCLSN, "motd")); ) // self MOTD
		if ( owner == motd->s.v.owner )
			ent_remove( motd );

	for( motd = world; (motd = find(motd, FOFCLSN, "motdX")); ) // staffing aliases/maps
		if ( owner == motd->s.v.owner )
			ent_remove( motd );
}
