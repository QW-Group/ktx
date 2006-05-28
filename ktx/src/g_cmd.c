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
 *  $Id: g_cmd.c,v 1.18 2006/05/28 03:44:28 qqshka Exp $
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

int k_say_fp_per = 1, k_say_fp_for = 1, k_say_fp_count = 9;

typedef struct say_fp_level_s {

	int fp_count;
	int fp_per;
	int fp_for;
	char *name;

} say_fp_level_t;

say_fp_level_t say_fp_levels[] = {
	{ 9, 1, 1, "Low"},
	{ 4, 1, 5, "Medium" },
	{ 5, 3, 7, "High" }
};

int say_fp_levels_cnt = sizeof(say_fp_levels) / sizeof(say_fp_levels[0]);

void FixSayFloodProtect()
{
	static int k_fp_last = -1;

	char buf[1024*4];

	int k_fp = bound(1, cvar("k_fp"), say_fp_levels_cnt);

	k_say_fp_count = say_fp_levels[k_fp-1].fp_count;
	k_say_fp_per   = say_fp_levels[k_fp-1].fp_per;
	k_say_fp_for   = say_fp_levels[k_fp-1].fp_for;

	if ( k_fp != k_fp_last ) {
		// qqshka: generally mod ignore server floodprot, but if server not support
		//         redirect say/say_team to mod, then try use server floodprot, so
		//		   in most cases this code useless.

		trap_readcmd( va("floodprot %d %d %d\n", 
						k_say_fp_count, k_say_fp_per, k_say_fp_for), buf, sizeof(buf) );
		G_cprint("%s", buf);
	}

	k_fp_last = k_fp;
}

void fp_toggle ()
{
	int k_fp = bound(1, cvar("k_fp"), say_fp_levels_cnt);

	if( check_master() )
		return;

	if ( !is_adm( self ) ) {
		G_sprint(self, 2, "You are not an admin\n");
		return;
	}

	if ( ++k_fp > say_fp_levels_cnt )
		k_fp = 1;

	cvar_fset("k_fp", k_fp);

	FixSayFloodProtect();

	G_bprint( 2, "Floodprot level %s \x90%s %s %s\x91 %6s\n",
					dig3(k_fp), 
					dig3(say_fp_levels[k_fp-1].fp_count),
					dig3(say_fp_levels[k_fp-1].fp_per),
					dig3(say_fp_levels[k_fp-1].fp_for),
					say_fp_levels[k_fp-1].name );
}

qboolean isSayFlood(gedict_t *p)
{
	int idx;

	float say_time;

	idx = bound(0, p->fp_s.last_cmd, MAX_FP_CMDS-1);
	say_time = p->fp_s.cmd_time[idx];

	if ( g_globalvars.time < p->fp_s.locked )
	{
		G_sprint(p, PRINT_CHAT, "You can't talk for %d more seconds\n",
										(int)(p->fp_s.locked - g_globalvars.time + 1));
		return true; // flooder
	}

	if ( say_time && ( g_globalvars.time - say_time < k_say_fp_per ) )
	{
		G_sprint(p, PRINT_CHAT, "FloodProt: You can't talk for %d seconds.\n", (int)k_say_fp_for);

		p->fp_s.locked = g_globalvars.time + k_say_fp_for;

		p->fp_s.warnings += 1; // collected but unused stat

		return true; // flooder
	}

	p->fp_s.cmd_time[idx] = g_globalvars.time;

	if ( ++idx >= k_say_fp_count )
		idx = 0;

	p->fp_s.last_cmd = idx;

	return false;
}

qboolean ClientSay( qboolean isTeamSay )
{
	int j;
	char text[1024] = {0}, *str, *name, *team;
	int sv_spectalk = cvar("sv_spectalk");
	int sv_sayteam_to_spec = cvar("sv_sayteam_to_spec");
	gedict_t *client, *goal;
	qboolean fake = false;

	self = PROG_TO_EDICT( g_globalvars.self );

	if ( isSayFlood( self ) )
		return true; // flooder

// { MVDSV

	str = params_str(1, -1);

	name = (strnull( self->s.v.netname ) ? "!noname!" : self->s.v.netname);

	if ( self->k_spectator ) {

		if ( !sv_spectalk || isTeamSay )
			strlcpy(text, va("[SPEC] %s: %s", name, str), sizeof(text));
		else
			strlcpy(text, va("%s: %s", name, str), sizeof(text));
	}
	else {

		if ( isTeamSay )
			strlcpy(text, va("(%s): %s", name, str), sizeof(text));
		else
			strlcpy(text, va("%s: %s", name, str), sizeof(text));
	}

	//bliP: kick fake ->
	if ( !isTeamSay && cvar("sv_unfake") ) {
		char *ch;

		for ( ch = text; *ch; ch++ )
			if ( *ch == 13 ) //bliP: 24/9 kickfake to unfake ->
				*ch = '#';
	}
	//<-

	fake = ( strchr(text, 13) ? true : false ); // check if string contain "$\"

	G_cprint("%s\n", text);
//	SV_Write_Log(CONSOLE_LOG, 1, text);

	team = ezinfokey(self, "team");

	for ( j = 1, client = &(g_edicts[j]); j <= MAX_CLIENTS; j++, client++ )
	{
		//bliP: everyone sees all ->
		//if (client->state != cs_spawned)
		//	continue;
		//<-
		if ( self->k_spectator && !sv_spectalk )
			if ( !client->k_spectator )
				continue;

		if ( isTeamSay )
		{
			// the spectator team
			if ( self->k_spectator )
			{
				if ( !client->k_spectator )
					continue;
			}
			else
			{
				if ( client->k_spectator )
				{
					goal = PROG_TO_EDICT( client->s.v.goalentity );

					if(   !sv_sayteam_to_spec // player can't say_team to spec in this case
					   || !fake // self say_team does't contain $\ so this is treat as private message
					   || (    (goal != world && goal->k_player) // spec track player
						   && strneq(team, ezinfokey(goal, "team"))
						  ) // spec track player on different team
					   || (   !(goal != world && goal->k_player) // spec _not_ track player
						   && strneq(team, ezinfokey(client, "team"))
						  ) // spec do not track player and on different team
					  )
					continue;	// on different teams
				}
				else if (   self != client // send msg to self anyway
						 && (!teamplay || strneq(team, ezinfokey(client, "team")))
						)
					continue;	// on different teams
			}
		}

		G_sprint(client, PRINT_CHAT, "%s\n", text);
	}

// } MVDSV

	return true;
}

// }

