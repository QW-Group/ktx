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

#include "g_local.h"

static char *fixed_maps_list[] =
{
	// episode 1
	"e1m1",
	"e1m2",
	"e1m3",
	"e1m4",
	"e1m5",
	"e1m6",
	"e1m7",
	"e1m8",

	// episode 2
	"e2m1",
	"e2m2",
	"e2m3",
	"e2m4",
	"e2m5",
	"e2m6",
	"e2m7",

	// episode 3
	"e3m1",
	"e3m2",
	"e3m3",
	"e3m4",
	"e3m5",
	"e3m6",
	"e3m7",

	// episode 4
	"e4m1",
	"e4m2",
	"e4m3",
	"e4m4",
	"e4m5",
	"e4m6",
	"e4m7",
	"e4m8",

	// 
	"start",
	"end",

	// DM maps
	"dm1",
	"dm2",
	"dm3",
	"dm4",
	"dm5",
	"dm6"
};

static int fixed_maps_cnt = sizeof ( fixed_maps_list ) / sizeof ( fixed_maps_list[0] );

//===============================================

#define MAX_MAPS 4096

static char		*mapslist[MAX_MAPS] = {0};
static int		maps_cnt = 0;

static char ml_buf[MAX_MAPS * 32] = {0}; // OUCH OUCH!!! btw, 32 is some average len of map name here, with path

// NOTE: we did not check is this map alredy in list or not...
static void Map_AddMapToList( char *name )
{
	int l;

	if ( strnull( name ) )
		return; // fu!

	if ( maps_cnt < 0 || maps_cnt >= MAX_MAPS )
		return; // too many

	l = strlen( name ) + 1;  // + nul
	mapslist[maps_cnt] = G_Alloc( l );		// alloc mem
	strlcpy( mapslist[maps_cnt], name, l );	// copy

	maps_cnt++;
}

// Extension to allow multiple .ent files for individual maps.
// We check all *.ent files we can find, and look for format mapName#description.ent, where mapName is a 
//    standard map we already have in list.
void GetCustomEntityMapsForDirectory(char* directory)
{
	char *s, name[32], *sep;
	int i, cnt, l, m;

	ml_buf[0] = 0;

	// Find all entity files in the maps directory
	cnt = trap_FS_GetFileList( directory, ( FTE_sv ? ".ent" : "\\.ent$" ), ml_buf, sizeof(ml_buf), 0 );
	ml_buf[sizeof(ml_buf)-1] = 0;

	for ( i = 0, s = ml_buf; i < cnt && s < ml_buf + sizeof(ml_buf); ++i )
	{
		l = strlen( s );

		if ( FTE_sv )
			l -= 4; // skip extension

		if ( l <= 0 )
			break;

		l++; // + nul

		sep = strchr(s, K_ENTITYFILE_SEPARATOR);
		if (sep)
		{
			int baseMapFound = 0, duplicateFound = 0;
			int mapNameLength;

			// copy
			strlcpy( name, s, min( sizeof(name), l ) );

			// The server could have mapName#entName.ent in more than one directory, so check here for duplicates
			mapNameLength = sep - s;
			if (mapNameLength > 0)
			{
				for (m = 0; m < maps_cnt; ++m)
				{
					baseMapFound |= (strlen(mapslist[m]) == mapNameLength && !strncmp(mapslist[m], s, mapNameLength));
					duplicateFound |= !strcmp(mapslist[m], s);
				}

				if (baseMapFound && ! duplicateFound)
					Map_AddMapToList( name );
			}
		}

		// find next map name
		s = strchr(s, 0);
		if ( !s )
			G_Error( "GetMapList: strchr returns NULL" );
		s++;
	}

	return;
}

void GetCustomEntityMaps(void)
{
	char path[1024] = { 0 };

	char* entityDir = cvar_string("sv_loadentfiles_dir");
	if (entityDir && *entityDir)
	{
		snprintf(path, sizeof(path) - 1, "maps/%s", entityDir);
		GetCustomEntityMapsForDirectory(path);
	}

	GetCustomEntityMapsForDirectory("maps");
}

void AddFixedMaps(void)
{
	int i;

	if ( mapslist[0] || maps_cnt )
		G_Error( "AddFixedMaps: can't do it twice" );

	for ( i = 0; i < MAX_MAPS && i < fixed_maps_cnt ; i++ )
		Map_AddMapToList( fixed_maps_list[i] );
}

void GetMapList(void)
{
	char *s, name[32];
	int i, cnt, l;

	ml_buf[0] = 0;

	if ( mapslist[0] || maps_cnt )
		G_Error( "GetMapList: can't do it twice" );

	if ( !FTE_sv )
		AddFixedMaps(); // add maps like dm3 dm2 e1m2 etc from paks, FTE doesn't need it.

	// this is reg exp search, so we escape . with \. in extension, however \ must be escaped in C string too so its \\.
	cnt = trap_FS_GetFileList( "maps", ( FTE_sv ? ".bsp" : "\\.bsp$" ), ml_buf, sizeof(ml_buf), 0 );

	cnt = bound(0, cnt, MAX_MAPS);

	ml_buf[sizeof(ml_buf)-1] = 0; // well, this is optional, just sanity

	for ( i = 0, s = ml_buf; i < cnt && s < ml_buf + sizeof(ml_buf); i++ )
	{
		l = strlen( s );

		if ( FTE_sv )
			l -= 4; // skip extension

		if ( l <= 0 )
			break;

		l++; // + nul

		// because of FTE we can't use 's' as is, we need skip extension with this weird strlcpy()
		strlcpy( name, s, min( sizeof(name), l ) );	// copy
		Map_AddMapToList( name );

		// find next map name
		s = strchr(s, 0);
		if ( !s )
			G_Error( "GetMapList: strchr returns NULL" );

		s++;
	}

	GetCustomEntityMaps();

#if 0 // debug
	G_cprint( "Maps list\n" );

	for ( i = 0; i < maps_cnt; i++ )
		G_cprint( "%4d: %s\n", i, mapslist[i] );
#endif
}

void mapslist_dl()
{
	char arg_2[32];
	int i, from, to;

	// seems we alredy done that
	if ( self->k_stuff & STUFF_MAPS )
	{
		G_sprint( self, 2, "mapslist alredy stuffed\n" );
		return;
	}

	// no arguments
	if ( trap_CmdArgc() == 1 )
	{
		G_sprint( self, 2, "mapslist without arguments\n" );
		return;
	}

	trap_CmdArgv( 1, arg_2, sizeof( arg_2 ) );

	from = bound( 0, atoi( arg_2 ), maps_cnt );
	to   = bound( from, from + MAX_STUFFED_ALIASES_PER_FRAME, maps_cnt );

	// stuff portion of aliases
	for ( i = from; i < to; i++ )
	{
		if ( i == 0 )
		G_sprint( self, 2, "Loading maps list...\n" );

		stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "alias %s cmd cm %d\n", mapslist[i], i + 1);
	}

	if ( i < maps_cnt )
	{
		// request next stuffing
		stuffcmd_flags( self, STUFFCMD_IGNOREINDEMO, "cmd mapslist_dl %d\n", i );
		return;
	}

	// we done
	self->k_stuff = self->k_stuff | STUFF_MAPS; // add flag
	G_sprint( self, 2, "Maps loaded\n" );

	// request commands
	if ( !( self->k_stuff & STUFF_COMMANDS ) )
		StuffModCommands( self );
}

void StuffMaps( gedict_t *p )
{
	p->k_stuff = p->k_stuff & ~STUFF_MAPS; // remove flag

	stuffcmd_flags( p, STUFFCMD_IGNOREINDEMO, "cmd mapslist_dl %d\n", 0 );
}

char *GetMapName(int imp)
{
	int i;

	if ( imp > 0 )
	{
		i = imp - 1;

	 	if ( i >= 0 && i < maps_cnt )
			return mapslist[i];
	}

	return "";
}

int GetMapNum(char *map)
{
	int i;

	if ( strnull( map ) )
		return 0;

	for( i = 0; i < maps_cnt; i++ )
	{
		if ( streq( mapslist[i], map ) )
		{
			return i + 1;
		}
	}

	return 0;
}

void DoSelectMap(int iMap)
{
	char     *m;
	gedict_t *p;
	int 	 till;
	qbool isVoted = false;

	if( (till = Q_rint( ( k_matchLess ? 15: 7 ) - g_globalvars.time)) > 0  ) {
		G_sprint(self, 2, "Wait %d second%s!\n", till, count_s(till) );
		return;
	}

	if ( k_matchLess && !k_bloodfest )
	{
		if ( cvar("k_no_vote_map") ) {
			G_sprint(self, 2, "Voting map is %s allowed\n", redtext("not"));
			return;
		}

		// you can select map in matchLess mode, but not in countdown.
		if ( match_in_progress != 2 )
			return;
	}
	else if ( match_in_progress )
		return;

	if ( self->ct == ctSpec && !is_adm(self ) ) // only admined specs can select map
		return;

	if ( strnull( m = GetMapName( iMap ) ) )
		return;

	if( cvar( "k_lockmap" ) && !is_adm( self ) )
	{
		G_sprint(self, 2, "MAP IS LOCKED!\n"
						  "You are NOT allowed to change!\n");
		return;
	}

	if ( self->v.map == iMap ) {
		G_sprint(self, 2, "--- your vote is still good ---\n");
		return;
	}

	for ( p = world; (p = find_plr( p )); )
		if ( p->v.map == iMap ) {
			isVoted = true;
			break;
		}

	if( !get_votes( OV_MAP ) ) {
		G_bprint(2, "%s %s %s\n", self->s.v.netname, redtext("suggests map"),m);
	}
	else if ( isVoted ) {
		G_bprint(2, "%s %s %s %s %s\n", self->s.v.netname, redtext("agrees"),
			(CountPlayers() < 3 ? redtext("to") : redtext("on") ), redtext("map"), m);
	}
	else
		G_bprint(2, "%s %s %s\n", self->s.v.netname, redtext("would rather play on"), m);

	self->v.map = k_lastvotedmap = iMap;

	vote_check_map ();
}

void SelectMap()
{
	char	arg_1[1024];

	trap_CmdArgv( 1, arg_1, sizeof( arg_1 ) );

	DoSelectMap ( atoi( arg_1 ) );
}


void VoteMap()
{
	char	arg_1[1024];

	trap_CmdArgv( 1, arg_1, sizeof( arg_1 ) );

	DoSelectMap ( GetMapNum( arg_1 ) );
}

void ShowMaps()
{
	int i, cnt;
	char	arg_1[1024];

	trap_CmdArgv( 1, arg_1, sizeof( arg_1 ) );

	G_sprint( self, 2, "Vote for maps by typing the mapname,\n"
					   "for example \"%s\" or use \"%s\".\n", redtext("dm6"), redtext("votemap dm6"));

	for( cnt = i = 0; i < maps_cnt; i++ )
	{
		if ( arg_1[0] && !strstr(mapslist[i], arg_1) )
			continue;

		if ( !cnt )
			G_sprint( self, 2, "\n---List of maps\n" );

		G_sprint( self, 2, ((cnt & 1) ? "%s\n" : "%-17s "), mapslist[i]);
		cnt++;
	}
	
	if ( cnt )
		G_sprint( self, 2, "%s---End of list (%d/%d maps)\n", (cnt & 1) ? "\n" : "", cnt, maps_cnt);
}

int IsMapInCycle(char *map)
{
	char newmap[128] = {0}, mapid[128] = {0};
	int i;

	if ( strnull( map ) )
		return 0;

	for ( i = 0; i < 1000; i++ ) {
		snprintf(mapid, sizeof(mapid), "k_ml_%d", i);
		// huge for(), so using trap version instead of lazy but unsafe cvar_string()
		trap_cvar_string( mapid, newmap, sizeof(newmap) );

		if ( strnull( newmap ) ) // end of list
			return 0;

		if ( streq( map, newmap ) ) // ok map found in map list
			return i + 1; // i may be 0, so returning i + 1
	}

	return 0;
}

char *SelectRandomMap(char *buf, int buf_size)
{
	char newmap[128] = {0}, mapid[128] = {0};
	int cnt, c;

	buf[0] = 0;

	// find how much maps in pool.
	for ( cnt = 0; cnt < 1000; cnt++ )
	{
		snprintf( mapid, sizeof(mapid), "k_ml_%d", cnt );
		// huge for(), so using trap version instead of lazy but unsafe cvar_string()
		trap_cvar_string( mapid, newmap, sizeof(newmap) );

		if ( strnull( newmap ) ) // end of list
			break;
	}

	// few attempts, to minimize selecting current map.
	for ( c = 0; c < 5; c++ )
	{
		int id = i_rnd( 0, cnt - 1 ); // generate random id

		// get map.
		snprintf( mapid, sizeof(mapid), "k_ml_%d", id );
		trap_cvar_string( mapid, newmap, sizeof(newmap) );

		if ( streq( g_globalvars.mapname, newmap) )
			continue; // same map, lets try again then.

		// ok, we found it.
		strlcpy(buf, newmap, buf_size);
		break;
	}

	return buf;
}

// map list have now next syntax:
// set k_ml_0 dm6
// set k_ml_1 dm4
// set k_ml_2 dm2
// so this mean we have rotation of maps dm6 dm4 dm2 dm6 dm4 dm2 ...

char *SelectMapInCycle(char *buf, int buf_size)
{
	char newmap[128] = {0}, mapid[128] = {0};
	int i;

	buf[0] = 0;

	if ( cvar( "k_random_maplist" ) )
	{
		if ( *SelectRandomMap( buf, buf_size ) )
			return buf;
	}

	if ( (i = IsMapInCycle( g_globalvars.mapname )) ) { // ok map found in map list, select next map

		snprintf( mapid, sizeof(mapid), "k_ml_%d", i >= 1000 ? 0 : i );
		trap_cvar_string( mapid, newmap, sizeof(newmap) );

	}
	else { // map not found in map cycle
		if ( (i = cvar( "_k_last_cycle_map" )) ) { // try to return to map list at proper point
    
			snprintf( mapid, sizeof(mapid), "k_ml_%d", i );
			trap_cvar_string( mapid, newmap, sizeof(newmap) );
    
			if( !IsMapInCycle( newmap ) )
				newmap[0] = 0; // seems map not in current cycle
		}
	}

	if ( strnull( newmap ) ) // last resort, trying get first entry in map list
		trap_cvar_string( "k_ml_0", newmap, sizeof(newmap) );

	strlcpy(buf, newmap, buf_size);

	return buf;
}
