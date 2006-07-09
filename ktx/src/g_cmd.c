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
 *  $Id: g_cmd.c,v 1.22 2006/07/09 22:53:25 qqshka Exp $
 */

#include "g_local.h"

extern void     trap_CmdArgv( int arg, char *valbuff, int sizebuff );

void			cmdinfo ();
void			cmduinfo ();
void			cmd_wreg_do( byte c );
void			cmd_ack();

void			s_common( gedict_t *from, gedict_t *to, char *msg );
void			s_p();
void			s_lr( float l );
void			s_t_do (char *str, char *tname);
void			s_t();
void			s_m_do (char *str, int m); // do multi print
void			s_m();
void			multi_do(int from_arg, qboolean from_mmode); // set up multi set

qboolean 	ClientCommand()
{
	char	cmd_command[1024], arg_1[1024];

	self = PROG_TO_EDICT( g_globalvars.self );

	if ( !self->k_accepted )
		return true; // cmon, u r zombie or etc...

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
	int j, l, mmode;
	char text[1024] = {0}, arg_2[10], *str, *name, *team;
	int sv_spectalk = cvar("sv_spectalk");
	int sv_sayteam_to_spec = cvar("sv_sayteam_to_spec");
	gedict_t *client, *goal;
	qboolean fake = false;

	self = PROG_TO_EDICT( g_globalvars.self );

	if ( !self->k_accepted )
		return true; // cmon, u r zombie or etc...

	if ( isSayFlood( self ) )
		return true; // flooder

	trap_CmdArgv( 1, arg_2, sizeof( arg_2 ) );

	if ( streq(arg_2, "s-p") ) {
		s_p();
		return true;
	}

	if ( (l = (streq(arg_2, "s-l") ? 1 : 0)) || (l = (streq(arg_2, "s-r") ? 2 : 0)) ) {
		s_lr( l );
		return true;
	}

	if ( streq(arg_2, "s-t") ) {
		s_t();
		return true;
	}

	if ( streq(arg_2, "s-m") ) {
		s_m();
		return true;
	}

	str = params_str(1, -1);

	mmode = iKey(self, "*mm");
	switch ( mmode ) {
		case MMODE_PLAYER:
			if ( (goal = SpecPlayer_by_id( iKey(self, "*mp") )) )
				s_common(self, goal, str);
			else
				G_sprint(self, 2, "mmode(player): can't find player\n");

			return true;

		case MMODE_TEAM:

			s_t_do(str, ezinfokey(self, "*mt"));

			return true;

		case MMODE_MULTI:

			s_m_do(str, iKey(self, "*mu"));

			return true;

		case MMODE_NAME:

			stuffcmd(self, "name \"%s\"\n", str);

			return true;

		case MMODE_RCON:
/*
			if ( !VIP_IsFlags(self, VIP_RCON) ) { // just sanity check
				G_sprint(self, 2, "mmode(rcon): access denied...\n");
			}
			else
*/
			{
				G_cprint("RCON from: %s: %s: %s\n", getname(self), ezinfokey(self, "ip"), str);
				trap_redirectcmd(self, va("%s\n", str)); // !!! WARNING: FULL ACCESS TO SERVER CONSOLE !!!
			}

			return true;
	}

// { MVDSV

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
						 && (!isTeam() || strneq(team, ezinfokey(client, "team")))
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

// { additional communication commands

void s_common( gedict_t *from, gedict_t *to, char *msg )
{
	char *ch;

	if ( from == to )
		return; // ignore sending text to self

	if ( match_in_progress && (from->k_player != to->k_player || from->k_spectator != to->k_spectator) )
		return; // spec to player or player to spec, disallowed in match

	from->s_last_to = to;
	to->s_last_from = from;

	for ( ch = msg; *ch; ch++ )
		if ( *ch == 13 ) // kickfake to unfake ->
			*ch = ' ';

	G_sprint(to,   PRINT_CHAT, "[%s->]: %s\n", getname(from), msg);
	G_sprint(from, PRINT_CHAT, "[->%s]: %s\n", getname(to),   msg);
}

void s_p()
{
	int argc = trap_CmdArgc();
	gedict_t *p;
	char arg_3[1024], *str;

	if ( argc < 4 ) {
		G_sprint(self, 2, "usage: s-p id/name txt\n");
		return;
	}

	trap_CmdArgv( 2, arg_3, sizeof( arg_3 ) );

	if ( (p = SpecPlayer_by_IDorName( arg_3 )) ) {
		str = params_str(3, -1);
		s_common( self, p, str );
	}
	else {
		G_sprint(self, 2, "s-p: client %s not found\n", arg_3);
		return;
	}
}

/*
void s_p_cmd() // just redirect /s-p command to /say
{
	stuffcmd(self, "say s-p %s\n", params_str(1, -1));
}
*/

void s_lr( float l )
{
	int argc = trap_CmdArgc();
	gedict_t *p;
	char *str;

	if ( argc < 3 ) {
		G_sprint(self, 2, "usage: %s txt\n", ( l == 1 ? "s-l" : "s-r" ));
		return;
	}

	p = ( l == 1 ? self->s_last_to : self->s_last_from );

	if ( p && GetUserID( p ) ) {
		str = params_str(2, -1);
		s_common( self, p, str );
	}
	else {
		G_sprint(self, 2, "%s: client not found\n", ( l == 1 ? "s-l" : "s-r" ));
		return;
	}
}

/*
void s_lr_cmd( float l ) // just redirect /s-l or /s-r command to /say
{
	stuffcmd(self, "say %s %s\n", ( l == 1 ? "s-l" : "s-r" ), params_str(1, -1));
}
*/

void s_lr_clear( gedict_t *dsc )
{
	int from;
	gedict_t *p;

	for ( from = 0, p = world; (p = find_plrspc(p, &from)); ) {

		if ( p->s_last_to == dsc )
			p->s_last_to = NULL;

		if ( p->s_last_from == dsc )
			p->s_last_from = NULL;
	}
}

void s_t_do (char *str, char *tname)
{
	int from, i;
	gedict_t *p;
	char *name, *ch;

	name = getname(self);

	for ( ch = str; *ch; ch++ )
		if ( *ch == 13 ) // kickfake to unfake ->
			*ch = ' ';

	for ( i = 0, from = 0, p = world; (p = find_plrspc(p, &from)); ) {
		if ( self == p ) // ignore sending text to self
			continue;

		if ( match_in_progress && (self->k_player != p->k_player || self->k_spectator != p->k_spectator) )
			continue; // spec to player or player to spec, disallowed in match

		if ( !(
			      ( streq(tname, "player") && p->k_player )
			   || ( streq(tname, "spectator") && p->k_spectator )
			   || ( streq(tname, "admin") && is_adm( p ) )
			   || ( streq(tname, getteam( p )) )
		   	  )
		   )
			continue;

		//[sender <t:teamname>]: txt
		G_sprint(p, PRINT_CHAT, "[%s <t:%s>]: %s\n", name, tname, str);
		i++;
	}

	if ( !i ) {
		G_sprint(self, 2, "s-t: no clients found for team %s\n", tname);
		return;
	}

	G_sprint(self, PRINT_CHAT, "[<t:%s>]: %s\n", tname, str);
}

void s_t()
{
	int argc = trap_CmdArgc();
	char arg_3[1024], *str;

	if ( argc < 4 ) {
		G_sprint(self, 2, "usage: s-t team txt\n");
		return;
	}

	str = params_str(3, -1);

	trap_CmdArgv( 2, arg_3, sizeof( arg_3 ) );

	s_t_do (str, arg_3);
}

/*
void s_t_cmd() // just redirect /s-t command to /say
{
	stuffcmd(self, "say s-t %s\n", params_str(1, -1));
}
*/

void s_m_do (char *str, int m)
{
	int from, i, bit;
	gedict_t *p;
	char *name, *ch;

	name = getname(self);

	for ( ch = str; *ch; ch++ )
		if ( *ch == 13 ) // kickfake to unfake ->
			*ch = ' ';

	for ( i = 0, from = 0, p = world; (p = find_plrspc(p, &from)); ) {
		if ( self == p ) // ignore sending text to self
			continue;

		if ( match_in_progress && (self->k_player != p->k_player || self->k_spectator != p->k_spectator) )
			continue; // spec to player or player to spec, disallowed in match

		bit = 1 << (int)(p - g_edicts - 1);

		if ( !( m & bit ) )
			continue; // not in multi set

		//[sender <m:num>]: txt
		G_sprint(p, PRINT_CHAT, "[%s <m:%d>]: %s\n", name, m, str);
		i++;
	}

	if ( !i ) {
		G_sprint(self, 2, "s-m: no clients found\n");
		return;
	}

	G_sprint(self, PRINT_CHAT, "[<m:%d>]: %s\n", m, str);
}

void s_m()
{
	if ( trap_CmdArgc() < 3 ) {
		G_sprint(self, 2, "usage: s-m txt\n");
		return;
	}

	s_m_do (params_str(2, -1), iKey(self, "*mu"));
}

// "/multi"
void multi()
{
	multi_do(1, false);
}

void multi_usage()
{
	G_sprint(self, 2, "Usage: multi <=/+/-/\?/\?\?> id1/name1 id2/name2 ...\n");
}


#define MMOP_N  (0) // none
#define MMOP_S  (1) // =
#define MMOP_P  (2) // +
#define MMOP_M  (3) // -
#define MMOP_Q  (4) // ?
#define MMOP_QQ (5) // ??


void multi_do(int from_arg, qboolean from_mmode)
{
	gedict_t *p;
	char arg_1[1024], arg_2[1024], arg_x[1024], multi[1024];
	int argc = trap_CmdArgc(), mmop, k, i, m, bit;

	trap_CmdArgv( from_arg,     arg_1, sizeof( arg_1 ) );
	trap_CmdArgv( from_arg + 1, arg_2, sizeof( arg_2 ) );

	// argh, this case/hack for "/mmode multi"
	if ( from_mmode && strnull(arg_1) ) {

		m = iKey(self, "*mu");

		for ( multi[0] = 0, k = 0, i = 0; i < MAX_CLIENTS; i++ ) {

			p = &(g_edicts[i+1]);

			if ( !(p->k_player || p->k_spectator) ) 
				continue; // not valid

			bit = 1 << i;

			if ( !( m & bit ) )
				continue; // not in multi set

			if ( k )
				strlcat(multi, ", ", sizeof(multi));
			strlcat(multi, getname( p ), sizeof(multi));

			k++;
		}
		
		if ( k ) {
			cmdinfo_setkey(self, "*mm", va("%d", MMODE_MULTI));
			cmdinfo_setkey(self, "*mu", va("%d", m));
			G_sprint(self, 2, "mmode(multi): %s\n", multi);
		}
		else
			G_sprint(self, 2, "mmode(multi): 0 players found\n");

		return;
	}

	if ( strneq(arg_1, "?") && strnull(arg_2) ) { // something like: "/mmode multi =" or /multi =" - too few params
		multi_usage(); 
		return;
	}

	if      ( streq(arg_1, "=") )  mmop = MMOP_S;
	else if ( streq(arg_1, "+") )  mmop = MMOP_P;
	else if ( streq(arg_1, "-") )  mmop = MMOP_M;
	else if ( streq(arg_1, "?") )  mmop = MMOP_Q;
	else if ( streq(arg_1, "??") ) mmop = MMOP_QQ;
	else mmop = MMOP_N;

	m = 0; // set this for MMOP_S

	switch ( mmop ) {

		case MMOP_P:
		case MMOP_M:

		m = iKey(self, "*mu");

		case MMOP_S:

		for ( multi[0] = 0, k = 0, i = from_arg + 1; i < argc; i++ ) {
			trap_CmdArgv( i, arg_x, sizeof( arg_x ) );

			if ( !(p = SpecPlayer_by_IDorName( arg_x )) || p == self ) 
				continue;

			bit = 1 << (int)(p - g_edicts - 1);

			if ( mmop == MMOP_M ) {
				if ( !( m & bit ) )
					continue; // nothing remove

				m &= ~bit; // minus bit
			}
			else {
				if ( ( m & bit ) )
					continue; // already set

				m |=  bit; // plus bit
			}

			if ( k )
				strlcat(multi, ", ", sizeof(multi));
			strlcat(multi, getname( p ), sizeof(multi));

			k++;
		}
		
		if ( k ) {
			if ( from_mmode ) // allow set up mmode only from /mmode cmd, not from /multi cmd
				cmdinfo_setkey(self, "*mm", va("%d", MMODE_MULTI));

			cmdinfo_setkey(self, "*mu", va("%d", m)); // this must be set in both cases

			G_sprint(self, 2, "multi %s: %s\n", arg_1, multi);
		}
		else
			G_sprint(self, 2, "multi %s: 0 players found\n", arg_1);

		return;

		case MMOP_Q:
		case MMOP_QQ:

		m = ( mmop == MMOP_Q ? iKey(self, "*mu") : atoi(arg_2) );

		for ( multi[0] = 0, k = 0, i = 0; i < MAX_CLIENTS; i++ ) {

			p = &(g_edicts[i+1]);

			if ( !(p->k_player || p->k_spectator) ) 
				continue; // not valid

			bit = 1 << i;

			if ( !( m & bit ) )
				continue; // not in multi set

			if ( k )
				strlcat(multi, ", ", sizeof(multi));
			strlcat(multi, getname( p ), sizeof(multi));

			k++;
		}
		
		if ( k )
			G_sprint(self, 2, "multi %s: %s\n", arg_1, multi);
		else
			G_sprint(self, 2, "multi %s: 0 players found\n", arg_1);

		return;

		default:

		multi_usage(); 

		return;
	}
}

void mmode_usage()
{
	G_sprint(self, 2, "Usage: mmode <player . , team multi name last off rcon> [params]\n");
}

void info_sys_mm_update( gedict_t *p, char *from, char *to )
{
	int mm  = atoi(to);
	int omm = atoi(from);

	if ( mm == omm )
		return;

	if ( mm == MMODE_NONE )
		return;

	cmdinfo_setkey(p, "*ml", va("%d", mm));
}

char *mmode_str(int mmode)
{
	switch ( mmode ) {
		case MMODE_NONE:	return "none";
		case MMODE_PLAYER:	return "player";
		case MMODE_TEAM:	return "team";
		case MMODE_MULTI:	return "multi";
		case MMODE_RCON:	return "rcon";
		case MMODE_NAME:	return "name";
	}

	return "unknown";
}

void mmode ()
{
	qboolean set;
	gedict_t *p = NULL;
	int argc = trap_CmdArgc(), id, mmode;
	char arg_2[1024], arg_3[1024], *tname, *rcpass;

	mmode = (argc < 2 ? iKey(self, "*mm") : MMODE_NONE); // try serve "/mmode" cmd without params

	trap_CmdArgv( 1, arg_2, sizeof( arg_2 ) ); // if argc >= 2
	trap_CmdArgv( 2, arg_3, sizeof( arg_3 ) ); // if argc >= 3

	if ( argc >= 2 ) { // try serve cmd "/mmode" with at least one param
		if ( streq(arg_2, "off") )	        mmode = MMODE_NONE;
		else if ( streq(arg_2, "player") )	mmode = MMODE_PLAYER;
		else if ( streq(arg_2, "team") ) 	mmode = MMODE_TEAM;
		else if ( streq(arg_2, "multi") )	mmode = MMODE_MULTI;
		else if ( streq(arg_2, "name") )	mmode = MMODE_NAME;
		else if ( streq(arg_2, "rcon") )	mmode = MMODE_RCON;
		else if ( streq(arg_2, ".") || streq(arg_2, ",") ) {
			p = ( streq(arg_2, ".") ? self->s_last_to : self->s_last_from );
			argc  = -1; // hint for code below
			mmode = MMODE_PLAYER;
		}
		else if ( streq(arg_2, "last") ) {
			int last = iKey(self, "*ml");

/*
			if ( last == MMODE_RCON && !VIP_IsFlags(self, VIP_RCON) ) { // just sanity check
				G_sprint(self, 2, "mmode(rcon): access denied...\n");
				return;
			}
*/

			cmdinfo_setkey(self, "*mm", va("%d", last));
			G_sprint(self, 2, "last mmode(%s)\n", mmode_str(last));
			return;
		}
		else {
			mmode_usage();
			return;
		}
	}

	switch ( mmode ) {
		case MMODE_PLAYER:

			if ( argc != -1 ) // ok apply hint here
				p = (argc < 3 ? SpecPlayer_by_id( iKey(self, "*mp") ) : SpecPlayer_by_IDorName( arg_3 ));

			if ( !p || !(id = GetUserID( p )) || p == self ) {
				G_sprint(self, 2, "mmode(player): can't find player\n");
				return;
			}

			// check do we need set this values or not
			if ( ( set = ( mmode != iKey(self, "*mm") || id != iKey(self, "*mp") ) ) ) {
				cmdinfo_setkey(self, "*mm", va("%d", mmode));
				cmdinfo_setkey(self, "*mp", va("%d", id));
			}

			G_sprint(self, 2, "mmode(player)%s: %s\n", (set ? " set": ""), getname( p ));

			return;

		case MMODE_TEAM:

			tname = (argc < 3 ? ezinfokey(self, "*mt" ) : arg_3);

			// check do we need set this values or not
			if ( ( set = ( mmode != iKey(self, "*mm") || strneq(tname, ezinfokey(self, "*mt")) ) ) ) {
				cmdinfo_setkey(self, "*mm", va("%d", mmode));
				cmdinfo_setkey(self, "*mt", va("%s", tname));
			}

			G_sprint(self, 2, "mmode(team)%s: %s\n", (set ? " set": ""), tname);

			return;

		case MMODE_MULTI:

			multi_do(2, true);

			return;

		case MMODE_NAME:

			// check do we need set this values or not
			if ( ( set = ( mmode != iKey(self, "*mm") ) ) )
				cmdinfo_setkey(self, "*mm", va("%d", mmode));

			G_sprint(self, 2, "mmode(name)%s\n", (set ? " set": ""));

			return;

		case MMODE_RCON:
			rcpass = cvar_string("rcon_password");

			if ( !(   ( !strnull(arg_3) && strneq("none", rcpass) && streq(arg_3, rcpass) ) // rcon via pass
				    || VIP_IsFlags(self, VIP_RCON) // rcon via VIP rights
				  )
			   ) { // ok, here we really need check access
				G_cprint("RCON failed from: %s: %s\n", getname(self), ezinfokey(self, "ip"));
				G_sprint(self, 2, "mmode(rcon): access denied...\n");
				return;
			}

			// check do we need set this values or not
			if ( ( set = ( mmode != iKey(self, "*mm") ) ) )
				cmdinfo_setkey(self, "*mm", va("%d", mmode));

			G_sprint(self, 2, "mmode(rcon)%s\n", (set ? " set": ""));

			return;

		case MMODE_NONE:
		default:

			G_sprint(self, 2, "mmode: off\n");
			cmdinfo_setkey(self, "*mm", va("%d", MMODE_NONE));

			return;
	}
}


// }

