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

void info_sys_mm_update( gedict_t *p, char *from, char *to );

qboolean isSupport_Params(gedict_t *p);

#define isSysKey( key ) ( !strnull(key) && *(key) == '*' )
extern cmdinfo_t cinfos[];
extern int cinfos_cnt;

qboolean fromSetInfo; // hack, so i know is key come to me from user setinfo
					  // or server localinfo, used by cmdinfo_getkey

extern void     trap_CmdArgv( int arg, char *valbuff, int sizebuff );

//===================================================
// native setinfo

qboolean FixPlayerTeam ( char *newteam );
qboolean FixPlayerColor ( char *newcolor );

qboolean 	ClientUserInfoChanged ()
{
	char            arg_0[1024], arg_1[1024], arg_2[1024], *old;

	if ( trap_CmdArgc() != 3 )
		return false; // something wrong

	trap_CmdArgv( 0, arg_0, sizeof( arg_0 ) );
	trap_CmdArgv( 1, arg_1, sizeof( arg_1 ) );
	trap_CmdArgv( 2, arg_2, sizeof( arg_2 ) );

//	G_bprint(2, "'%s' '%s' '%s'\n", arg_0, arg_1, arg_2 );

	if ( streq("team", arg_1) )
		return FixPlayerTeam ( arg_2 );
	if ( streq("rate", arg_1) )
		return CheckRate ( self, arg_2 );
	if ( streq("bottomcolor", arg_1) )
		return FixPlayerColor ( arg_2 );

	// here a hack for support calling handler if some "cmd info" stored in user setinfo
	if (    (old = cmdinfo_getkey( self, arg_1 )) != NULL // key is supported by "cmd info"
		 && fromSetInfo // come to me from user setinfo
	   ) {
		int i;

		for ( i = 0; i < cinfos_cnt; i++ )
			if ( streq(cinfos[i].key, arg_1) ) {
				if ( cinfos[i].f )  // call some handler, if set
					(cinfos[i].f)( self, (old ? old : ""), arg_2 );

				break;
			}

		return false;
	}

	return false;
}

// in ctf we dont want red team players to be blue, etc
qboolean FixPlayerColor ( char *newcolor )
{
	if ( self->ct == ctSpec )
		return false;

	if ( isCTF() )
	{
		if ( streq(getteam(self), "red") )
			stuffcmd( self, "color %d %d\n", iKey(self, "topcolor") == 13 ? 4 : iKey(self, "topcolor"), 4 );
		else if ( streq(getteam(self), "blue") )
			stuffcmd( self, "color %d %d\n", iKey(self, "topcolor") == 4 ? 13 : iKey(self, "topcolor"), 13 );
	}
	return false;
}

// check if player tried to change team and this is allowed
qboolean FixPlayerTeam ( char *newteam )
{
	char *s1, *s2;

	// specs does't have limits
	if ( self->ct == ctSpec )
		return false;

	// do not allow change team in game / countdown
	if( match_in_progress )
	{
		s1 = newteam;
		s2 = getteam(self);

		if( strneq( s1, s2 ) )
		{
			G_sprint(self, 2, "You may %s change team during game\n", redtext("not"));
			stuffcmd(self, "team \"%s\"\n", s2); // sends this to client - so he get right team too
		}

		return true;
	}

	// captain or potential captain may not change team
	if ( capt_num( self ) || is_elected(self, etCaptain) ) {
		if( strneq( getteam( self ), newteam ) ) {
			G_sprint(self, 2, "You may %s change team\n", redtext("not"));
			stuffcmd(self, "team \"%s\"\n", getteam(self)); // sends this to client - so he get right team too
		}

		return true;
	}

	if( k_captains == 2 ) {
		// get the strings to compare
		s1 = newteam;

		if     ( self->k_picked == 1 )
			s2 = cvar_string( "_k_captteam1" );
		else if( self->k_picked == 2 )
			s2 = cvar_string( "_k_captteam2" );
		else
			s2 = "";

		if( strneq( s1, s2 ) ) {
			G_sprint(self, 2, "You may %s change team\n", redtext("not"));
			stuffcmd(self, "team \"%s\"\n", s2); // sends this to client - so he get right team too
			return true;
		}
	}

	if ( !match_in_progress && isCTF() && self->ready ) {
		// if CTF and player ready allow change team to red or blue
		s1 = newteam;
		s2 = getteam(self);

		if( strneq(s1, "red") && strneq(s1, "blue") )
		{
			G_sprint(self, 2, "You must be on team red or blue for CTF\n" );
			stuffcmd(self, "team \"%s\"\n", s2); // sends this to client - so he get right team too
			return true;
		}
		stuffcmd(self, "color %d\n", streq(s1, "red") ? 4 : 13); 
	}

	if ( !match_in_progress && ( isTeam() || isCTF() ) && self->ready && strnull( newteam ) ) {
		// do not allow empty team, because it cause problems
		G_sprint(self, 2, "Empty %s not allowed\n", redtext("team"));
		stuffcmd(self, "team \"%s\"\n", getteam(self)); // sends this to client - so he get right team too
		return true;
	}

	if ( isCTF() && ( streq(newteam, "red") || streq(newteam, "blue") ) )
		stuffcmd(self, "auto%s\n", newteam);

	return false;
}

//========================================================
// ktpro like 'cmd info' work around, which partially replace setinfo, which is too short for us


cmdinfo_t cinfos[] = {
	{ "*is", 0 },		// keys with * is for internal use ONLY
	{ "*mm", info_sys_mm_update },	// for mmode
	{ "*ml", 0 },					// for mmode last
	{ "*mp", 0 },					// for mmode player
	{ "*mt", 0 },					// for mmode team
	{ "*mu", 0 },					// for mmode multi
	{ "*at", 0 },					// autotrack
    { "mi", info_mi_update },		// spec moreinfo
    { "b_switch", 0 },
//    { "bw" },
//      b1, b2, b3, b4
    { "e-mail", 0 },
    { "ev", info_ev_update },
//      +ev, -ev
    { "fs", 0 }, // for force_spec 
//      ft
	{ "gender", 0 },
    { "icq", 0 },
    { "k", 0 },
    { "k_nick", 0 },
//      ke
	{ "kf", info_kf_update },
//		+kf, -kf
	{ "ln", 0 },
    { "ls", 0 },
	{ "lw", 0 },
	{ "lw_x", 0 },
	{ "ktpl", 0 }, // zzz, so "ln" "ls" "lw" keys will work like in ktpro
	{ "postmsg", 0 },
	{ "premsg", 0 },
//    	quote 	// wtf?
//   	ma		// wtf?
//	{ "ss", 0 }, // not used anymore
	{ "ti", 0 },	// specifie use team info or not
//		rb
    { "w_switch", 0 },
	{ "k_sdir", 0 },
	{ "wps", 0 },
	{ "lra", 0 },		// ra status bar modificator
	{ "pbspeed", 0 },	// for /trx_play
	{ "runes", 0 }	// for stuffing "set rune xxx" in CTF
};

int cinfos_cnt = sizeof( cinfos ) / sizeof( cinfos[0] );

// do not use ezinfokey here, or will be infinite recursion
// if key is allowed, search key in servers localinfo, if not found try found in userinfo too
char *cmdinfo_getkey( gedict_t *p, char *key )
{
		   char		cid[20];
	static char		string[MAX_STRINGS][1024];
	static int		index = 0;

	int id, i;

	fromSetInfo = false;

	if ( !p )
		G_Error("cmdinfo_getkey: null");

	if ( strnull( key ) )
		return NULL;

// this is generally GetUserID but we can't use it here, because of infinite recursion
//{
	if ( p->ct != ctPlayer && p->ct != ctSpec ) // not a player or spectator
		return NULL;

	if ( !( id = atoi(infokey(p, "*userid", cid, sizeof(cid))) ) ) // get user id
		return NULL;
//}

	index %= MAX_STRINGS;

	infokey( world, va("key_%d_%s", id, key), string[index], sizeof( string[0] ) );

	if ( !strnull( string[index] ) ) {
		return string[index++]; // luck - key is found
	}
	else {
		for ( i = 0; i < cinfos_cnt; i++ ) {
			if ( streq(cinfos[i].key, key) ) { // key is allowed - return "" or value
				if ( isSysKey( key ) )
					return "";  // no sys key in native userinfo

				fromSetInfo = true;

				return infokey( p, key, string[index++], sizeof( string ) );
			}
		}
	}

	return NULL; // key is not supported / allowed
}

// check is this key is in cinfos list, in this case set this key for this player or spec
// return 0 is all ok
// return 2 is key not supported / allowed
int cmdinfo_setkey( gedict_t *p, char *key, char *value )
{
	int id, i;

	if ( !p || !value )
		G_Error("cmdinfo_setkey: null");

	if ( strnull( key ) )
		return 2;

	if ( !( id = GetUserID( p ) ) ) // not a player or spectator
		return 1;

	for ( i = 0; i < cinfos_cnt; i++ ) {
		if ( streq(cinfos[i].key, key) ) {
			if ( cinfos[i].f ) { // call some handler, if set
				char *old = cmdinfo_getkey( p, key ); // get old value

				(cinfos[i].f)( p, (old ? old : ""), value );
			}

			localcmd("localinfo key_%d_%s \"%s\"\n", id, key, (streq(value, "\\") ? "" : value));
			return 0;
		}
	}

	return 2;
}

void cmdinfo ()
{
	int		argc = trap_CmdArgc();
	char	*v;
	char	arg_1[128], arg_2[128];

	if ( trap_CmdArgc() < 1 )
		return; // something wrong

	if ( argc == 1 || argc > 3 ) { // just show info about all keys and usage
		int i;

		if ( isSupport_Params( self ) )
			G_sprint(self, 2, "usage: kinfo [<key> [value] (\"\\\" for removing key)]\n\n");
		else
			G_sprint(self, 2, "usage: cmd info [<key> [value] (\"\\\" for removing key)]\n\n");

		for ( i = 0; i < cinfos_cnt; i++ ) {
			
			if ( isSysKey( cinfos[i].key ) )
				continue; // sys keys is not showed for mortals %)

			v = cmdinfo_getkey( self, cinfos[i].key );

			if ( !strnull( v ) ) // check if key is allowed and not empty ""
				G_sprint(self, 2, "key %s = \"%s\"\n", cinfos[i].key, v);
		}

		return;
	}

	if ( argc == 2 ) { // show info about particular key, even _SYS_ key can be showed

		trap_CmdArgv( 1, arg_1, sizeof( arg_1 ) );
		v = cmdinfo_getkey( self, arg_1 );

		if ( !v )
			G_sprint(self, 2, "key \"%s\" is not supported\n", arg_1);
		else
			G_sprint(self, 2, "key %s = \"%s\"\n", arg_1, v);

		return;
	}

	if ( argc == 3 ) { // set/remove particular key

		trap_CmdArgv( 1, arg_1, sizeof( arg_1 ) );
		trap_CmdArgv( 2, arg_2, sizeof( arg_2 ) );

		if ( isSysKey( arg_1 ) ) {
			G_sprint(self, 2, "key \"%s\" is can't be changed\n", arg_1);
			return;
		}

		if ( cmdinfo_setkey( self, arg_1, arg_2 ) == 2 )
			G_sprint(self, 2, "key \"%s\" is not supported\n", arg_1);

		return;
	}
}

void cmduinfo ()
{
	gedict_t *p;
	int		argc = trap_CmdArgc();
	char	*v;
	char	arg_1[128], arg_2[128];

	if ( trap_CmdArgc() < 1 )
		return; // something wrong

	if ( argc == 1 || argc > 3 ) { // just show info about usage

		if ( isSupport_Params( self ) )
			G_sprint(self, 2, "usage: kuinfo <id/name> [key]\n");
		else
			G_sprint(self, 2, "usage: cmd uinfo <id/name> [key]\n");

		return;
	}

	if ( argc == 2 ) { // show info about keys of someone
		int i;

		trap_CmdArgv( 1, arg_1, sizeof( arg_1 ) );

		if ( !(p = SpecPlayer_by_IDorName( arg_1 )) ) {
			G_sprint(self, 2, "client \"%s\" not found\n", arg_1 );
			return;
		}

		G_sprint(self, 2, "%s's personal keys:\n", p->s.v.netname);

		for ( i = 0; i < cinfos_cnt; i++ ) {
			
			if ( isSysKey( cinfos[i].key ) )
				continue; // sys keys is not showed for mortals %)

			v = cmdinfo_getkey( p, cinfos[i].key );

			if ( !strnull( v ) ) // check if key is allowed and not empty ""
				G_sprint(self, 2, "key %s = \"%s\"\n", cinfos[i].key, v);
		}

		return;
	}

	if ( argc == 3 ) { // show someone particular key
		trap_CmdArgv( 1, arg_1, sizeof( arg_1 ) );
		trap_CmdArgv( 2, arg_2, sizeof( arg_2 ) );

		if ( !(p = SpecPlayer_by_IDorName( arg_1 )) ) {
			G_sprint(self, 2, "client \"%s\" not found\n", arg_1 );
			return;
		}

		v = cmdinfo_getkey( p, arg_2 );

		if ( !v )
			G_sprint(self, 2, "key \"%s\" is not supported\n", arg_2);
		else
			G_sprint(self, 2, "%s's %s = \"%s\"\n", p->s.v.netname, arg_2, v);

		return;
	}
}

void cmdinfo_check_obsolete ( gedict_t *p )
{
	qboolean warn = false;
	char v[128];
	int i;

	for ( i = 0; i < cinfos_cnt; i++ )
		if ( !strnull( infokey( p, cinfos[i].key, v, sizeof( v ) ) ) ) {
			G_sprint(p, 2, "setinfo %s found\n", cinfos[i].key);
			warn = true;
		}

	if ( warn )
		G_sprint(p, 2, "please use cmd info [<key> [value]]\n");
}

// remove all keys for specified client
void cmdinfo_clear ( gedict_t *p )
{
	int i;

	for ( i = 0; i < cinfos_cnt; i++ )
		cmdinfo_setkey( p, cinfos[i].key, "" );
}

// call infoset alias at client for player and sinfo for spec
void cmdinfo_infoset ( gedict_t *p )
{
	if ( strnull( cmdinfo_getkey( p, "*is" ) ) ) {
		cmdinfo_check_obsolete ( p ); // check keys wich must be moved from userinfo to cmd info
		cmdinfo_clear ( p ); // remove all keys
		cmdinfo_setkey( p, "*is", "1" ); // mark we are call infoset already
		stuffcmd(p, "%sinfoset\n", p->ct == ctSpec ? "s" : ""); // and call infoset
		stuffcmd(p, "ktx_%sinfoset\n", p->ct == ctSpec ? "s" : ""); // and call ktx_infoset
		// kick cmd back to server, so we know client get infoset,
	  	// so we can invoke on_connect and on_enter
		stuffcmd(p, "wait;wait;wait;cmd ack infoset\n");
	}
	else {
		// kick cmd back to server, so we know client already get infoset,
	  	// so we can invoke on_enter
		stuffcmd(p, "wait;wait;wait;cmd ack noinfoset\n");
	}
}

