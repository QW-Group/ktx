
// g_userinfo.c

#include "g_local.h"

extern void     trap_CmdArgv( int arg, char *valbuff, int sizebuff );

qboolean FixPlayerTeam ( char *newteam );

qboolean 	ClientUserInfoChanged ()
{
	char            arg_0[1024], arg_1[1024], arg_2[1024];

	if ( trap_CmdArgc() != 3 )
		return false; // something wrong

	trap_CmdArgv( 0, arg_0, sizeof( arg_0 ) );
	trap_CmdArgv( 1, arg_1, sizeof( arg_1 ) );
	trap_CmdArgv( 2, arg_2, sizeof( arg_2 ) );

//	G_sprint(self, 2, "'%s' '%s' '%s'\n", arg_0, arg_1, arg_2 );

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


