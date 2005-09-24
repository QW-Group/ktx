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
 *  $Id: g_cmd.c,v 1.1.1.1 2005/09/24 12:45:02 disconn3ct Exp $
 */

#include "g_local.h"

extern void     trap_CmdArgv( int arg, char *valbuff, int sizebuff );

void            ClientKill();
void			SelectMap();

qboolean 	ClientCommand()
{
	char            cmd_command[1024], arg_1[1024];

	self = PROG_TO_EDICT( g_globalvars.self );
	trap_CmdArgv( 0, cmd_command, sizeof( cmd_command ) );

	if ( !strcmp( cmd_command, "kill" ) ) // TODO: put this in 'cc' commands, is this possible?
	{
		ClientKill();
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
	else if ( !strcmp( cmd_command, "cc" ) ) {
		if ( trap_CmdArgc() == 2 ) {
			trap_CmdArgv( 1, arg_1, sizeof( arg_1 ) );
			DoCommand ( atoi( arg_1 ) );
		}
		return true;
	}

//	if ( trap_CmdArgc() > 1 ) {
//		char arg_x[1024]
//		trap_CmdArgv( 1, arg_x, sizeof( arg_x ) );
//		G_Printf("ClientCommand %s %s\n",cmd_command, arg_x);
//	}

	return false;
}
