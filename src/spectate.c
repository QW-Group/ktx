/*
 *  QWProgs-DM
 *  Copyright (C) 2004  [sd] angel
 *
 *  This code is based on QuakeWorld DM mod code by Id Software, Inc.
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
 *  $Id$
 */

// spectate.c
#include "g_local.h"

void Wp_Stats(float on);
void Sc_Stats(float on);

void DoAutoTrack();
void AdminImpBot();

void MakeMOTD();
void AutoTrackRestore();

void Bot_Print_Thinking(void);

qbool TrackChangeCoach(gedict_t *p);

int GetSpecWizard()
{
	int k_asw = bound(0, cvar("allow_spec_wizard"), 2);

	if (match_in_progress || intermission_running || isRACE())
	{
		return 0;
	}

	switch (k_asw)
	{
		case 0:
			return 0; // wizards not allowed

		case 1:
			return (CountPlayers() ? 0 : 1); // allowed without players

		case 2:
			return 2; // allowed with players in prematch
	}

	return 0;
}

void ShowCamHelp()
{
	G_sprint(self, 2, "use %s %s to jump between spawn points\n"
				"use [attack] to change cam mode\n"
				"use [jump] to change target\n",
				redtext("impulse"), dig3(1));
}

void wizard_think()
{
	if (!cvar("k_no_wizard_animation")) // animate if allowed
	{
		(self->s.v.frame)++;
	}

	if ((self->s.v.frame > 14) || (self->s.v.frame < 0))
	{
		self->s.v.frame = 0;
	}

	self->s.v.nextthink = g_globalvars.time + 0.1;
}

void SpecDecodeLevelParms()
{
	/*
	 self->s.v.items			= g_globalvars.parm1;
	 self->s.v.health		= g_globalvars.parm2;
	 self->s.v.armorvalue 	= g_globalvars.parm3;
	 self->s.v.ammo_shells 	= g_globalvars.parm4;
	 self->s.v.ammo_nails 	= g_globalvars.parm5;
	 self->s.v.ammo_rockets 	= g_globalvars.parm6;
	 self->s.v.ammo_cells 	= g_globalvars.parm7;
	 self->s.v.weapon 		= g_globalvars.parm8;
	 self->s.v.armortype 	= g_globalvars.parm9 * 0.01;
	 */
	if (g_globalvars.parm11)
	{
		self->k_admin = g_globalvars.parm11;
	}

	if (g_globalvars.parm12)
	{
		self->k_coach = g_globalvars.parm12;
	}

	if (g_globalvars.parm13)
	{
		self->k_stuff = g_globalvars.parm13;
	}

//	if ( g_globalvars.parm14 )
//    	self->ps.handicap = g_globalvars.parm14;
}

qbool nospecs_canconnect(gedict_t *spec)
{
	if (cvar("_k_nospecs"))
	{
		// some VIPS able to connect anyway
		if (!(VIP(spec) & ALLOWED_NOSPECS_VIPS) && !is_coach(spec))
		{
			return false;
		}
	}

	return true;
}

////////////////////////////////////////////////////
// Function checks if a spectator can connect or not
//
// return true - if a spec can connect
//        false - otherwise
////////////////////////////////////////////////////
qbool SpecCanConnect(gedict_t *spec)
{
	if (!nospecs_canconnect(spec))
	{
		G_sprint(spec, 2, "%s mode, you can't connect\n", redtext("No spectators"));

		return false;
	}

	return true;
}

////////////////
// GlobalParams:
// time
// self
// params
///////////////
void SpectatorConnect()
{
	gedict_t *p;
	int diff = (int)(PROG_TO_EDICT(self->s.v.goalentity) - world);

	// we need this before the SpecCanConnect() call, as we need to restore admin and/or coach flags
	SpecDecodeLevelParms();

	if (!SpecCanConnect(self))
	{
		stuffcmd(self, "disconnect\n"); // FIXME: stupid way

		return;
	}

	self->ct = ctSpec;
	self->classname = "spectator"; // Added this in for kick code
	self->k_accepted = 1; // spectator has no restriction to connect

	for (p = world; (p = (match_in_progress == 2 && !cvar("k_ann")) ? find_spc(p) : find_client(p));)
	{
		if (p != self)  // does't show msg for self
		{
			G_sprint(p, PRINT_HIGH, "Spectator %s entered the game\n", self->netname);
		}
	}

	if ((diff < 0) || (diff >= MAX_EDICTS)) // something wrong happen - fixing
	{
		self->s.v.goalentity = EDICT_TO_PROG(world);
	}

	VIP_ShowRights(self);
	CheckRate(self, "");

	if (match_in_progress != 2)
	{
		self->wizard = spawn();
		self->wizard->classname = "spectator_wizard";
		self->wizard->think = (func_t) wizard_think;
		self->wizard->s.v.nextthink = g_globalvars.time + 0.1;
	}

	// Wait until you do stuffing
	MakeMOTD();
}

////////////////
// GlobalParams:
// time
// self
///////////////
extern int g_matchstarttime;
void PutSpectatorInServer()
{
//	G_sprint(self, 2, "Hellow %s\n", getname(self));

	g_globalvars.msg_entity = EDICT_TO_PROG(self);
	WriteByte(MSG_ONE, 38 /*svc_updatestatlong*/);
	WriteByte(MSG_ONE, 18 /*STAT_MATCHSTARTTIME*/);
	WriteLong(MSG_ONE, g_matchstarttime);

	AutoTrackRestore();
}

////////////////
// GlobalParams:
// self
///////////////
void SpectatorDisconnect()
{
	gedict_t *p;

	if (self->k_accepted)
	{
		for (p = world;
				(p = (match_in_progress == 2 && !cvar("k_ann")) ? find_spc(p) : find_client(p));)
		{
			G_sprint(p, PRINT_HIGH, "Spectator %s left the game\n", self->netname);
		}
	}

// s: added conditional function call here
	if (self->v.elect_type != etNone)
	{
		if (match_in_progress != 2)
		{
			G_bprint(2, "Election aborted\n");
		}

		AbortElect();
	}

	if (coach_num(self))
	{
		G_bprint(2, "A %s has left\n", redtext("coach"));

		ExitCoach();
	}

	if (self->wizard)
	{
		ent_remove(self->wizard);
		self->wizard = NULL;
	}

	if (self->k_kicking)
	{
		ExitKick(self);
	}

	self->classname = ""; // Cenobite, so we clear out any specs as they leave
	self->k_accepted = 0;
	self->ct = ctNone;
}

/*
 ================
 SpectatorImpulseCommand

 Called by SpectatorThink if the spectator entered an impulse
 ================
 */
void SpectatorImpulseCommand()
{
	gedict_t *goal;

	if (self->ct != ctSpec)
	{
		self->s.v.impulse = 0;

		return;
	}

	goal = PROG_TO_EDICT(self->s.v.goalentity);

	if (self->k_adminc && (self->s.v.impulse >= 1) && (self->s.v.impulse <= 9))
	{
		AdminImpBot();
	}
	else if (self->s.v.impulse == 1)
	{
		// teleport the spectator to the next spawn point
		// note that if the spectator is tracking, this doesn't do much
		goal = PROG_TO_EDICT(self->s.v.goalentity);

		// if track someone - return
		if (((int)(goal - world) >= 1) && ((int)(goal - world) <= MAX_CLIENTS))
		{
// qqshka - heh, not all guys like this warning
//			G_sprint(self, 2, "stop %s first\n", redtext("tracking"));
			self->s.v.impulse = 0;

			return;
		}

		goal = find(goal, FOFCLSN, "info_player_deathmatch");

		if (!goal)
		{
			goal = find(world, FOFCLSN, "info_player_deathmatch");
		}

		if (goal)
		{
			setorigin(self, PASSVEC3(goal->s.v.origin));
			VectorCopy(goal->s.v.angles, self->s.v.angles);
			self->s.v.fixangle = true;	// turn this way immediately
		}
		else
		{
			goal = world;
		}

		self->s.v.goalentity = EDICT_TO_PROG(goal);
	}

	self->s.v.impulse = 0;
}

void SpecGoalChanged()
{
	if (self->k_coach)
	{
		if (TrackChangeCoach(self))
		{
			return;
		}
	}

	if (self->wp_stats)
	{
		Wp_Stats(2); // force refresh
	}

	if (self->sc_stats)
	{
		Sc_Stats(2); // force refresh
	}

	WS_OnSpecPovChange(self); // refresh "new weapon stats"
}

////////////////
// GlobalParams:
// time
// self
///////////////
void SpectatorThink()
{
	gedict_t *wizard = self->wizard;

	if (self->last_goal != self->s.v.goalentity)
	{
		SpecGoalChanged();

		self->last_goal = self->s.v.goalentity;
	}

	if (self->autotrack)
	{
		DoAutoTrack();
	}

	if (self->s.v.impulse)
	{
		SpectatorImpulseCommand();
	}

	if (self->sc_stats && self->sc_stats_time && (self->sc_stats_time <= g_globalvars.time)
			&& (match_in_progress != 1))
	{
		Print_Scores();
	}

	if (self->wp_stats && self->wp_stats_time && (self->wp_stats_time <= g_globalvars.time)
			&& (match_in_progress != 1))
	{
		Print_Wp_Stats();
	}

#ifdef BOT_SUPPORT
	if (self->s.v.goalentity)
	{
		gedict_t *goal = PROG_TO_EDICT(self->s.v.goalentity);

		if (goal->isBot)
		{
			Bot_Print_Thinking();
		}
	}
#endif

	if (wizard)
	{
		// set model angles
		wizard->s.v.angles[0] = -self->s.v.v_angle[0] / 2;
		wizard->s.v.angles[1] = self->s.v.v_angle[1];
		// wizard model blinking at spectator screen - so move model behind spec camera a bit
		trap_makevectors(self->s.v.v_angle);
		VectorMA(self->s.v.origin, -32, g_globalvars.v_forward, wizard->s.v.origin);
		// model bobbing
		wizard->s.v.origin[2] += sin(g_globalvars.time * 2.5);
		setorigin(wizard, PASSVEC3(wizard->s.v.origin));

		if (GetSpecWizard())
		{
			gedict_t *goal = PROG_TO_EDICT(self->s.v.goalentity);

			if (goal && (goal->ct == ctPlayer)) // tracking player, so turn model off
			{
				wizard->model = "";
			}
			else
			{
				// turn model on
				setmodel(wizard, "progs/wizard.mdl");
			}
		}
		else
		{
			wizard->model = ""; // turn model off
		}
	}
}

void remove_specs_wizards()
{
	gedict_t *p;

	for (p = world; (p = find_spc(p));)
	{
		if (p->wizard)
		{
			ent_remove(p->wizard);
			p->wizard = NULL;
		}
	}
}

void hide_specs_wizards()
{
	gedict_t *p;

	for (p = world; (p = find(p, FOFCLSN, "spectator_wizard"));)
	{
		p->model = "";
	}
}

void show_specs_wizards()
{
	gedict_t *p;

	for (p = world; (p = find(p, FOFCLSN, "spectator_wizard"));)
	{
		setmodel(p, "progs/wizard.mdl");
	}
}

void FixSpecWizards()
{
	static int k_asw = -1; // static

	qbool changed = false;
	int k_asw_new = GetSpecWizard();

	if ((k_asw != k_asw_new) || (framecount == 1))
	{ // force on first frame
		changed = true;
		k_asw = k_asw_new;
	}

	if (changed)
	{
		if (k_asw)
		{
			show_specs_wizards();
		}
		else
		{
			hide_specs_wizards();
		}
	}
}
