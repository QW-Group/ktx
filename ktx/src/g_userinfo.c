
// g_userinfo.c

#include "g_local.h"


extern void     trap_CmdArgv( int arg, char *valbuff, int sizebuff );

//===================================================
// native setinfo

qboolean FixPlayerTeam ( char *newteam );

qboolean 	ClientUserInfoChanged ()
{
	char            arg_0[1024], arg_1[1024], arg_2[1024];

	if ( trap_CmdArgc() != 3 )
		return false; // something wrong

	trap_CmdArgv( 0, arg_0, sizeof( arg_0 ) );
	trap_CmdArgv( 1, arg_1, sizeof( arg_1 ) );
	trap_CmdArgv( 2, arg_2, sizeof( arg_2 ) );

//	G_bprint(2, "'%s' '%s' '%s'\n", arg_0, arg_1, arg_2 );

	if ( streq("team", arg_1) )
		return FixPlayerTeam ( arg_2 );

	return false;
}

// check if player tried to change team and this is allowed
qboolean FixPlayerTeam ( char *newteam )
{
	char *s1, *s2;

	if( k_captains == 2 ) {
		// get the strings to compare
//		s1 = ezinfokey(self, "team");
		s1 = newteam;

		if     (self->k_captain == 1 || self->k_picked == 1)
			s2 = ezinfokey(world, "captteam1");
		else if(self->k_captain == 2 || self->k_picked == 2)
			s2 = ezinfokey(world, "captteam2");
		else
			s2 = "";

		if( strneq( s1, s2 ) ) {
			G_sprint(self, 2, "You may %s change team\n", redtext("not"));
			stuffcmd(self, "team \"%s\"\n", s2); // sends this to client - so he get right team too
			return true;
		}
	}

	if( match_in_progress == 2 && self->k_accepted == 2 && self->k_teamnum )
	{
//		s1 = ezinfokey(self, "team");
		s1 = newteam;
		s2 = ezinfokey(world, va("%d", (int)self->k_teamnum));

		if( strneq( s1, s2 ) )
		{
			self->k_accepted = 0;
			G_bprint(2,  "%s gets %s for changing team\n", self->s.v.netname, redtext("kicked"));
			GhostFlag(self);
			self->s.v.classname = "";
			stuffcmd(self, "disconnect\n"); // FIXME: stupid way
			return true;
		}
	}

	return false;
}

//========================================================
// ktpro like 'cmd info' work around, which partially replace setinfo, which is too short for us

#define isSysKey( key ) ( !strnull(key) && *(key) == '*' )


cmdinfo_t cinfos[] = {
	{ "*is" },		// keys with * is for internal use ONLY
    { "b_switch" },
//    { "bw" },
//      b1, b2, b3, b4
    { "e-mail" },
//      ev, +ev, -ev
//      fs
//      ft
	{ "gender" },
    { "icq" },
    { "k" },
    { "k_nick" },
//      ke
	{ "kf" },
//		+kf, -kf
	{ "ln" },
//    { "ls" },
	{ "lw" },
	{ "postmsg" },
	{ "premsg" },
//    	quote 	// wtf?
//   	ma		// wtf?
	{ "ss" },
//		rb
    { "w_switch" },
	{ "k_sdir" }
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

	if ( !p )
		G_Error("cmdinfo_getkey: null");

	if ( strnull( key ) )
		return NULL;

// this is generally GetUserID but we can't use it here, because of infinite recursion
//{
	if ( !p->k_player && !p->k_spectator ) // not a player or spectator
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

		G_sprint(self, 2, "usage: cmd uinfo <id/name> [key]\n");
		return;
	}

	if ( argc == 2 ) { // show info about keys of someone
		int i;

		trap_CmdArgv( 1, arg_1, sizeof( arg_1 ) );

		if ( !(p = player_by_IDorName( arg_1 )) ) {
			G_sprint(self, 2, "player \"%s\" not found\n", arg_1 );
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

		if ( !(p = player_by_IDorName( arg_1 )) ) {
			G_sprint(self, 2, "player \"%s\" not found\n", arg_1 );
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
		stuffcmd(p, "%sinfoset\n", p->k_spectator ? "s" : ""); // and call infoset
	}
}

