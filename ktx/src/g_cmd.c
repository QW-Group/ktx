/*
 *  QWProgs-DM
 *  Copyright (C) 2004  [sd] angel
 *
 *  This code is based on Q3 VM code by Id Software, Inc.
 *
 *
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
 *  $Id: g_cmd.c,v 1.14 2006/05/15 00:08:58 qqshka Exp $
 */

#include "g_local.h"

extern void     trap_CmdArgv( int arg, char *valbuff, int sizebuff );

void			cmdinfo ();
void			cmduinfo ();
void			cmd_wreg_do( byte c );
void			cmd_ack();

qboolean 	ClientCommand()
{
	char	cmd_command[1024], arg_1[1024];

	self = PROG_TO_EDICT( g_globalvars.self );

	trap_CmdArgv( 0, cmd_command, sizeof( cmd_command ) );

/*
	{
		char arg_x[1024];
		trap_CmdArgv( 1, arg_x, sizeof( arg_x ) );
		G_bprint(2, "ClientCommand '%s' '%s'\n", cmd_command, arg_x);
	}
*/

	// ack allowed when even classname is not set
	if ( !strcmp( cmd_command, "ack" ) ) {
		cmd_ack();
		return true;
	}

	if ( strneq(self->s.v.classname, "player") && strneq(self->s.v.classname, "spectator") )
		return true; // not connected yet or something

	if ( cmd_command[0] && !cmd_command[1] ) { // one character command, this is wreg command
		cmd_wreg_do( cmd_command[0] );
		return true;
	}
	else if ( !strcmp( cmd_command, "cm" ) ) {
		if ( trap_CmdArgc() == 2 ) {
			trap_CmdArgv( 1, arg_1, sizeof( arg_1 ) );
			self->cmd_selectMap = atoi( arg_1 );
			SelectMap ();
			self->cmd_selectMap = 0;
		}
		return true;
	}
// { saved for ktpro compatibility
	else if ( !strcmp( cmd_command, "info" ) ) {
		cmdinfo ();
		return true;
	}
	else if ( !strcmp( cmd_command, "uinfo" ) ) {
		cmduinfo ();
		return true;
	}
// }
	else if (    only_digits( cmd_command ) // strlen(cmd_command) == 3 for now, but I omit this
		 	  && DoCommand( atoi( cmd_command ) ) != DO_OUT_OF_RANGE_CMDS
		    )
	{
		return true; // command may fail in some way, but at least command exist, so return true
	}
	else if ( DoCommand_Name( cmd_command ) != DO_OUT_OF_RANGE_CMDS )
	{
		return true; // command may fail in some way, but at least command exist, so return true
	}

	return false;
}

void cmd_ack()
{
	int		argc = trap_CmdArgc();
	char	arg_1[64];

	if ( argc != 2 )
		return; // something wrong

	trap_CmdArgv( 1, arg_1, sizeof( arg_1 ) );

	if ( streq(arg_1, "infoset") ) {
		on_connect(); // qqshka: on_connect is useless IMO, but ktpro compatible
		on_enter();
	}
	else if ( streq(arg_1, "noinfoset") ) {
		on_enter();
	}
}

// i dunno where put this. so put here.
// SAY / SAY_TEAM mod handler
// {

qboolean ClientSay( qboolean isTeamSay )
{
	int i;
	char arg_x[1024], buf[1024];
	int argc = trap_CmdArgc();

	self = PROG_TO_EDICT( g_globalvars.self );

	for ( buf[0] = 0, i = 1; i < argc; i++ ) {
		trap_CmdArgv( i, arg_x, sizeof( arg_x ) );

		if ( i != 1 )
			strlcat( buf,   " ", sizeof( buf ) );
		strlcat( buf, arg_x, sizeof( buf ) );
	}

	if ( self->k_spectator )
		G_bprint(PRINT_CHAT, "[SPEC] %s: %s\n", 
					(strnull( self->s.v.netname ) ? "!noname!" : self->s.v.netname), buf);
	else
		G_bprint(PRINT_CHAT, "%s: %s\n",
					(strnull( self->s.v.netname ) ? "!noname!" : self->s.v.netname), buf);

	return true;
}

// }

