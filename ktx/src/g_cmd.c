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
 *  $Id: g_cmd.c,v 1.10 2006/04/16 15:25:27 qqshka Exp $
 */

#include "g_local.h"

extern void     trap_CmdArgv( int arg, char *valbuff, int sizebuff );

void            ClientKill();
void			cmdinfo ();
void			cmduinfo ();
void			cmd_ack();
void			cmd_wreg();
void			cmd_wreg_do( byte c );

// VVD: Need for executing commands by 'cmd cc <CMD_NAME> <ARG1> ... <ARGn>',
// because '<CMD_NAME> <ARG1> ... <ARGn>' work only with last ezQuake qw client.
// Function only_digits was copied from bind (DNS server) sources.
int only_digits(const char *s)
{
	if (*s == '\0')
		return (0);
	while (*s != '\0')
	{
		if (!isdigit(*s))
			return (0);
		s++;
	}
	return (1);
}

qboolean 	ClientCommand()
{
	char            cmd_command[1024], arg_1[1024];

	self = PROG_TO_EDICT( g_globalvars.self );

	trap_CmdArgv( 0, cmd_command, sizeof( cmd_command ) );

/*
	{
		char arg_x[1024];
		trap_CmdArgv( 1, arg_x, sizeof( arg_x ) );
		G_bprint(2, "ClientCommand '%s' '%s'\n", cmd_command, arg_x);
	}
*/

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
	else if ( !strcmp( cmd_command, "kill" ) ) // TODO: put this in 'cc' commands, is this possible?
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
		if ( trap_CmdArgc() >= 2 ) {
			trap_CmdArgv( 1, arg_1, sizeof( arg_1 ) );
// VVD: Need for executing commands by 'cmd cc <CMD_NAME> <ARG1> ... <ARGn>',
// because '<CMD_NAME> <ARG1> ... <ARGn>' work only with last ezQuake qw client.
			if (only_digits(arg_1))
				DoCommand ( atoi( arg_1 ) );
			else
				DoCommand_Name ( arg_1 );
		}
		return true;
	}
	else if ( !strcmp( cmd_command, "info" ) ) {
		cmdinfo ();
		return true;
	}
	else if ( !strcmp( cmd_command, "uinfo" ) ) {
		cmduinfo ();
		return true;
	}
	else if ( !strcmp( cmd_command, "wreg" ) ) {
		cmd_wreg();
		return true;
	}


//	if ( trap_CmdArgc() > 1 ) {
//		char arg_x[1024];
//		trap_CmdArgv( 1, arg_x, sizeof( arg_x ) );
//		G_Printf("ClientCommand %s %s\n",cmd_command, arg_x);
//	}

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

void wreg_usage()
{
	G_sprint(self, 2, "usage: cmd wreg [[char] [[+/-]weapon order]]\n");
}

void wreg_showslot(	wreg_t *w, int slot )
{
	int i;
	char *sign, order[MAX_WREG_IMP+1];

	if ( !w->init ) {
		G_sprint(self, 2, "slot \"%c\" - unregistered\n", (char)slot);
		return;
	}

	sign = "";
	if (w->attack > 0 )
		sign = "+";
	else if (w->attack < 0 )
		sign = "-";

	for ( order[0] = i = 0; i < MAX_WREG_IMP && w->impulse[i]; i++ )
		order[i] = '0' + w->impulse[i];
	order[i] = 0;

	G_sprint(self, 2, "slot \"%c\" - \"%s%s\"\n", (char)slot, sign, order);
}

void cmd_wreg()
{
	int		argc = trap_CmdArgc(), attack = 0, imp[MAX_WREG_IMP], i, cnt;
	char	arg_1[64], arg_2[64], *tmp = arg_2;
	byte    c;
	wreg_t  *w;

	if ( !self->wreg )
		return;

	if ( argc == 1 ) {
		qboolean found = false;

		G_sprint(self, 2, "list of registered weapons:\n");

		for ( i = 0; i < MAX_WREGS; i++ ) {
			w = &(self->wreg[i]);
			if ( !w->init )
				continue;

			found = true;
			wreg_showslot( w, i );
		}

		if ( !found )
			G_sprint(self, 2, "none\n");

		return;
	}

	trap_CmdArgv( 1, arg_1, sizeof( arg_1 ) );

	if ( strnull(arg_1) ) {
		wreg_usage();
		G_sprint(self, 2, "empty char\n");
		return;
	}

	if ( strlen(arg_1) > 1 ) {
		wreg_usage();
		G_sprint(self, 2, "char can be only one byte\n");
		return;
	}

	c = arg_1[0];

	if ( c <= 0 || c > 175 || c >= MAX_WREGS ) {
		wreg_usage();
		G_sprint(self, 2, "\"%c\" - illegal char!\n", (char)c);
		return;
	}

	w = &(self->wreg[c]);

	if ( argc == 2 ) {
		wreg_showslot( w, c );
		return;
	}

	if ( argc != 3 ) {
		wreg_usage();
		return; // something wrong
	}

	trap_CmdArgv( 2, arg_2, sizeof( arg_2 ) );

	if ( strnull(arg_2) ) {
		if ( w->init ) {
			w->init = false;
			G_sprint(self, 2, "slot \"%c\" - unregistered\n", (char)c);
		}
		else {
			wreg_usage();
			G_sprint(self, 2, "empty weapon order\n");
		}
		return;
	}

	for ( cnt = i = 0; i < MAX_WREGS; i++ ) {
		if ( !(self->wreg[i].init) )
			continue;

		if ( ++cnt >= 20 ) {
			G_sprint(self, 2, "too many wregs, discard registration\n");
			return;
		}
	}

	if ( strlen(arg_2) > 10 ) { // 10 == strlen("+987654321")
		wreg_usage();
		G_sprint(self, 2, "too long weapon order\n");
		return;
	}

	if ( tmp[0] == '+' ) {
		tmp++;
		attack = 1;
	}
	else if ( tmp[0] == '-' ) {
		tmp++;
		attack = -1;
	}

	if ( !strnull( tmp ) && !only_digits( tmp ) ) {
		wreg_usage();
		G_sprint(self, 2, "illegal character in weapon order\n");
		return;
	}

	for ( i = 0; i < MAX_WREG_IMP && !strnull(tmp); tmp++ ) {
		if ( tmp[0] == '0' ) // do not confuse with '\0'
			continue;

		imp[i] = tmp[0] - '0';
		i++;
	}

	// ok we parse wreg command, and all ok, init it
	memset( w, 0, sizeof( wreg_t ) ); // clear

	w->init   = true;
	w->attack = attack;

	for ( i--; i >= 0 && i < MAX_WREG_IMP; i-- )
		w->impulse[i] = imp[i];

	G_sprint(self, 2, "slot \"%c\" - registered\n", (char)c);
}

void cmd_wreg_do( byte c )
{
	qboolean warn;
	int j;
	wreg_t *w;

	if ( !self->wreg || c >= MAX_WREGS )
		return;

	w = &(self->wreg[c]);

	if ( !w->init ) {
		G_sprint(self, 2, "unregistered wreg char - \"%c\"\n", (char)c);
		return;
	}

//	G_sprint(self, 2, "wreg char - %c, i - %d %d %d\n", (char)c, w->impulse[0], w->impulse[1], w->impulse[2]);

	if ( w->attack > 0 ) {
		self->wreg_attack = 1;

		if( self->k_spectator )
			stuffcmd(self, "+attack\n");
	}
	else if ( w->attack < 0 ) {
		self->wreg_attack = 0;

		if( self->k_spectator )
			stuffcmd(self, "-attack\n");
	}

	if( self->k_spectator )
		return;

	for ( j = 0; j < MAX_WREG_IMP && w->impulse[j]; j++ ) {

		if ( j + 1 >= MAX_WREG_IMP || !w->impulse[j + 1] )
			warn = true; // warn about no weapon or ammo if this last impulse in array
		else
			warn = false;

		if ( W_CanSwitch( w->impulse[j], warn ) ) {
			self->s.v.impulse = w->impulse[j];
			return;
		}
	}
}

