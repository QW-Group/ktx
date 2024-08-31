/*
 *  $Id$
 */

#include "g_local.h"

// CTF todo:
// . option to disable pents, but leave quads on?
// . ability to drop backpacks of ammo to teammates

// Changes from purectf:
// . Close range grapple exploit removed
// . Grapple speed stays consistent regardless of sv_maxspeed
// . Status bar removed use +scores instead
// . Physics are the same as those found on dm servers (you can accel jump, etc)
// . Spawns are now random (except initial spawn in your base) with default spawnsafety mode
// . Untouched runes respawn at 90 seconds instead of 120 (happens if in lava or falls through level, etc)
// . Defending the flag is now two bonus points rather than one

// Server config changes:
// . add 64 to k_allowed_free_modes to enable ctf, 127 now enables all modes
// . set k_mode 4 if you want server to default to ctf mode

#define FLAG_RETURN_TIME     30
#define CARRIER_ASSIST_TIME  6
#define RETURN_ASSIST_TIME   4
#define RETURN_BONUS         1
#define CAPTURE_BONUS        15
#define TEAM_BONUS           10
#define CARRIER_ASSIST_BONUS 2
#define RETURN_ASSIST_BONUS  1
#define CARRIER_DEFEND_TIME  4

void DropFlag(gedict_t *flag, qbool tossed);
void PlaceFlag(void);
void FlagThink(void);
void FlagTouch(void);
void SP_item_flag_team1(void);
void SP_item_flag_team2(void);

// Allows us to add flags (or other items) to dm maps when ctfing without actually changing bsp
void G_CallSpawn(gedict_t *ent);
void SpawnCTFItem(char *classname, float x, float y, float z, float angle)
{
	gedict_t *item = spawn();

	item->classname = classname;
	setorigin(item, x, y, z);
	item->s.v.angles[0] = 0;
	item->s.v.angles[1] = angle;

	G_CallSpawn(item);
}

void spawn_item_flag(void)
{
	if (k_ctf_custom_models)
	{
		setmodel(self, "progs/flag.mdl");
	}

	self->noise = "misc/flagtk.wav";
	self->noise1 = "doors/runetry.wav";
	setsize(self, -16, -16, 0, 16, 16, 74);
	self->mdl = self->model;
	self->s.v.flags = FL_ITEM;
	self->s.v.solid = SOLID_TRIGGER;
	self->s.v.movetype = MOVETYPE_TOSS;
	SetVector(self->s.v.velocity, 0, 0, 0);
	self->s.v.origin[2] += 6;
	self->think = (func_t) FlagThink;
	self->touch = (func_t) FlagTouch;
	self->s.v.nextthink = g_globalvars.time + 0.1;
	self->cnt = FLAG_AT_BASE;
	self->cnt2 = 0.0;
	VectorCopy(self->s.v.angles, self->mangle);
	self->s.v.effects = (int)self->s.v.effects | EF_DIMLIGHT;

	if (!droptofloor(self))
	{
		ent_remove(self);
	}
	else
	{
		VectorCopy(self->s.v.origin, self->s.v.oldorigin);
	}

	if (!isCTF())
	{
		setmodel(self, "");
		self->touch = (func_t) SUB_Null;
	}
}

void SP_item_flag_team1(void)
{
	self->k_teamnumber = 1;
	self->s.v.items = IT_KEY2;
	self->s.v.skin = 0;
	self->s.v.effects = (int)self->s.v.effects | EF_RED;

	if (!k_ctf_custom_models)
	{
		setmodel(self, "progs/w_g_key.mdl");
	}

	spawn_item_flag();
}

void SP_item_flag_team2(void)
{
	self->k_teamnumber = 2;
	self->s.v.items = IT_KEY1;
	self->s.v.skin = 1;
	self->s.v.effects = (int)self->s.v.effects | EF_BLUE;

	if (!k_ctf_custom_models)
	{
		setmodel(self, "progs/w_s_key.mdl");
	}

	spawn_item_flag();
}

// would love to know what a ctf wall is :O!
void SP_func_ctf_wall(void)
{
	SetVector(self->s.v.angles, 0, 0, 0);
	self->s.v.movetype = MOVETYPE_PUSH;
	self->s.v.solid = SOLID_BSP;
	setmodel(self, self->model);
}

#define TF_TEAM_BLUE 1
#define TF_TEAM_RED 2

void SP_item_tfgoal(void)
{
	// NOTE: team_no inverted for flags!!!

	if (self->team_no == TF_TEAM_RED)
	{
		self->classname = "item_flag_team2";
		SP_item_flag_team2();
	}
	else if (self->team_no == TF_TEAM_BLUE)
	{
		self->classname = "item_flag_team1";
		SP_item_flag_team1();
	}
	else
	{
		G_Printf("SP_item_tfgoal: team_no %d unsupported\n", self->team_no);
		ent_remove(self);
		return;
	}
}

void SP_info_player_teamspawn(void)
{
	if (self->team_no == TF_TEAM_RED)
	{
		self->classname = "info_player_team1";
	}
	else if (self->team_no == TF_TEAM_BLUE)
	{
		self->classname = "info_player_team2";
	}
	else
	{
		G_Printf("SP_info_player_teamspawn: team_no %d unsupported\n", self->team_no);
		ent_remove(self);
		return;
	}
}

void SP_i_p_t(void)
{
	self->classname = "info_player_teamspawn";
	SP_info_player_teamspawn();
}

// add/remove hook item to/from player
void AddHook(qbool yes)
{
	gedict_t *e, *oself;

	oself = self;

	for (e = world; (e = find_plr(e));)
	{
		e->s.v.items = (yes ? ((int)e->s.v.items | IT_HOOK) : ((int)e->s.v.items & ~IT_HOOK));

		self = e; // warning

		if (self->hook_out)
		{
			GrappleReset(self->hook);
		}

		self->hook_out = false;
		self->on_hook = false;

		if (!yes && (self->s.v.weapon == IT_HOOK))
		{ // actually remove hook from hands if hold, not just from items
			self->s.v.weapon = 0;
			W_SetCurrentAmmo();
		}
	}

	self = oself;
}

void RegenFlag(gedict_t *flag)
{
	flag->s.v.movetype = MOVETYPE_TOSS;
	flag->s.v.solid = SOLID_TRIGGER;
	setmodel(flag, flag->mdl);
	VectorCopy(flag->mangle, flag->s.v.angles);
	flag->cnt = FLAG_RETURNED;
	flag->cnt2 = 0.0;
	flag->s.v.owner = EDICT_TO_PROG(world);
	SetVector(flag->s.v.velocity, 0, 0, 0);
	sound(flag, CHAN_FLAG, "items/itembk2.wav", 1, ATTN_NORM);
	flag->s.v.nextthink = g_globalvars.time + 0.2;
	flag->s.v.groundentity = EDICT_TO_PROG(world);
	flag->touch = (func_t) FlagTouch;
}

// show/hide flag
void RegenFlags(qbool yes)
{
	gedict_t *flag;

	flag = find(world, FOFCLSN, "item_flag_team1");

	if (flag)
	{
		if (!yes)
		{
			flag->touch = (func_t) SUB_Null;
			setmodel(flag, "");
		}
		else
		{
			RegenFlag(flag);
		}
	}

	flag = find(world, FOFCLSN, "item_flag_team2");

	if (flag)
	{
		if (!yes)
		{
			flag->touch = (func_t) SUB_Null;
			setmodel(flag, "");
		}
		else
		{
			RegenFlag(flag);
		}
	}
}

void FlagThink(void)
{
	if (!isCTF())
	{
		return;
	}

	self->s.v.nextthink = g_globalvars.time + 0.1;

	if (self->cnt == FLAG_AT_BASE)
	{
		return;
	}

	if (self->cnt == FLAG_DROPPED)
	{
		self->cnt2 += 0.1;
		if (g_globalvars.time > self->super_time)
		{
			RegenFlag(self);
			G_bprint(2, "The %s flag has been returned\n",
						redtext(((int)self->s.v.items & IT_KEY1) ? "BLUE" : "RED"));
		}

		return;
	}

	if (self->cnt == FLAG_RETURNED)
	{
		setorigin(self, PASSVEC3(self->s.v.oldorigin));
		self->cnt = FLAG_AT_BASE;

		return;
	}

	self->cnt2 += 0.1;
}

void FlagTouch(void)
{
	gedict_t *p, *owner;

	if (!k_practice)
	{
		if (match_in_progress != 2)
		{
			return;
		}
	}

	if (other->ct != ctPlayer)
	{
		return;
	}

	if (other->s.v.health < 1)
	{
		return;
	}

	if (self->cnt == FLAG_RETURNED)
	{
		return;
	}

	// if owner of the flag, do nothing (probably toss in progress)
	if (self->s.v.owner == EDICT_TO_PROG(other))
	{
		return;
	}

	// touching their own flag
	if (((self->k_teamnumber == 1) && streq(getteam(other), "red"))
			|| ((self->k_teamnumber == 2) && streq(getteam(other), "blue")))
	{
		if (self->cnt == FLAG_AT_BASE)
		{
			if (other->ctf_flag & CTF_FLAG)
			{
				gedict_t *cflag = NULL;

				// capture
				other->ctf_flag -= ((int)other->ctf_flag & CTF_FLAG);
				other->s.v.effects -= ((int)other->s.v.effects & (EF_FLAG1 | EF_FLAG2));

				sound(other, CHAN_FLAG, "misc/flagcap.wav", 1, ATTN_NONE);

				G_bprint(2, "%s", other->netname);
				if (self->k_teamnumber == 1)
				{
					cflag = find(world, FOFCLSN, "item_flag_team2");
					G_bprint(2, " %s the %s flag!\n", redtext("captured"), redtext("BLUE"));
				}
				else
				{
					cflag = find(world, FOFCLSN, "item_flag_team1");
					G_bprint(2, " %s the %s flag!\n", redtext("captured"), redtext("RED"));
				}

				if (cflag)
				{
					G_bprint(2, "The capture took %.1f seconds\n", cflag->cnt2);
				}

				other->s.v.frags += CAPTURE_BONUS;
				other->ps.ctf_points += CAPTURE_BONUS;
				other->ps.caps++;

				// loop through all players on team to give bonus
				for (p = world; (p = find_plr(p));)
				{
					p->s.v.items -= ((int)p->s.v.items & (IT_KEY1 | IT_KEY2));
					if (streq(getteam(p), getteam(other)))
					{
						if (p->return_flag_time + RETURN_ASSIST_TIME > g_globalvars.time)
						{
							p->return_flag_time = -1;
							p->s.v.frags += RETURN_ASSIST_BONUS;
							p->ps.ctf_points += RETURN_ASSIST_BONUS;
							G_bprint(2, "%s gets an assist for returning his flag!\n", p->netname);
						}

						if (p->carrier_frag_time + CARRIER_ASSIST_TIME > g_globalvars.time)
						{
							p->carrier_frag_time = -1;
							p->s.v.frags += CARRIER_ASSIST_BONUS;
							p->ps.ctf_points += CARRIER_ASSIST_BONUS;
							G_bprint(2, "%s gets an assist for fragging the flag carrier!\n",
										p->netname);
						}

						if (p != other)
						{
							p->s.v.frags += TEAM_BONUS;
							p->ps.ctf_points += TEAM_BONUS;
						}
					}
					else
					{
						p->carrier_hurt_time = -1;
					}
				}

				RegenFlags(true);

				k_nochange = 0;	// Set it so it should update scores at next attempt.
				refresh_plus_scores();

				return;
			}

			return;
		}
		else if (self->cnt == FLAG_DROPPED)
		{
			other->s.v.frags += RETURN_BONUS;
			other->ps.ctf_points += RETURN_BONUS;
			other->ps.returns++;
			other->return_flag_time = g_globalvars.time;
			sound(other, CHAN_FLAG, self->noise1, 1, ATTN_NONE);
			RegenFlag(self);

			G_bprint(2, "%s", other->netname);

			if (self->k_teamnumber == 1)
			{
				G_bprint(2, " %s the %s flag!\n", redtext("returned"), redtext("RED"));
			}
			else
			{
				G_bprint(2, " %s the %s flag!\n", redtext("returned"), redtext("BLUE"));
			}

			k_nochange = 0;	// Set it so it should update scores at next attempt.
			refresh_plus_scores();

			return;
		}
	}

	if (strneq(getteam(other), "red") && strneq(getteam(other), "blue"))
	{
		return;
	}

	refresh_plus_scores(); // update players status bar faster

	// Pick up the flag
	sound(other, CHAN_FLAG, self->noise, 1, ATTN_NONE);
	other->ctf_flag |= CTF_FLAG;

	other->s.v.items = (int)other->s.v.items | (int)self->s.v.items;

	self->cnt = FLAG_CARRIED;
	self->s.v.solid = SOLID_NOT;
	self->s.v.owner = EDICT_TO_PROG(other);

	owner = PROG_TO_EDICT(self->s.v.owner);
	owner->ps.pickups++;

	G_bprint(2, "%s", other->netname);
	if (streq(getteam(other), "red"))
	{
		G_bprint(2, " %s the %s flag!\n", redtext("got"), redtext("BLUE"));
		owner->s.v.effects = (int)owner->s.v.effects | EF_FLAG2;
	}
	else
	{
		G_bprint(2, " %s the %s flag!\n", redtext("got"), redtext("RED"));
		owner->s.v.effects = (int)owner->s.v.effects | EF_FLAG1;
	}
	setmodel(self, "");
}

void FlagResetOwner(void)
{
	self->think = (func_t) FlagThink;
	self->touch = (func_t) FlagTouch;
	self->s.v.nextthink = g_globalvars.time + 0.1;
	self->s.v.owner = EDICT_TO_PROG(self);
}

void TossFlag(void)
{
	PlayerDropFlag(self, true);
}

void PlayerDropFlag(gedict_t *player, qbool tossed)
{
	gedict_t *flag;
	char *cn;

	if (!(player->ctf_flag & CTF_FLAG))
	{
		return;
	}

	if (streq(getteam(player), "red"))
	{
		cn = "item_flag_team2";
	}
	else
	{
		cn = "item_flag_team1";
	}

	flag = find(world, FOFCLSN, cn);
	if (flag)
	{
		DropFlag(flag, tossed);
	}
}

void DropFlag(gedict_t *flag, qbool tossed)
{
	gedict_t *p = PROG_TO_EDICT(flag->s.v.owner);
	gedict_t *p1;

	p->ctf_flag -= (p->ctf_flag & CTF_FLAG);
	p->s.v.effects -= ((int)p->s.v.effects & ( EF_FLAG1 | EF_FLAG2));
	p->s.v.items -= ((int)p->s.v.items & (int)flag->s.v.items);

	setorigin(flag, PASSVEC3(p->s.v.origin));
	flag->s.v.origin[2] -= 24;
	flag->cnt = FLAG_DROPPED;
	if (tossed)
	{
		trap_makevectors(p->s.v.v_angle);
		if (p->s.v.v_angle[0])
		{
			flag->s.v.velocity[0] = g_globalvars.v_forward[0] * 300 + g_globalvars.v_up[0] * 200;
			flag->s.v.velocity[1] = g_globalvars.v_forward[1] * 300 + g_globalvars.v_up[1] * 200;
			flag->s.v.velocity[2] = g_globalvars.v_forward[2] * 300 + g_globalvars.v_up[2] * 200;
		}
		else
		{
			aim(flag->s.v.velocity);
			VectorScale(flag->s.v.velocity, 300, flag->s.v.velocity);
			flag->s.v.velocity[2] = 200;
		}
	}
	else
	{
		SetVector(flag->s.v.velocity, 0, 0, 300);
	}

	flag->s.v.flags = FL_ITEM;
	flag->s.v.solid = SOLID_TRIGGER;
	flag->s.v.movetype = MOVETYPE_TOSS;
	setmodel(flag, flag->mdl);
	setsize(flag, -16, -16, 0, 16, 16, 74);
	flag->super_time = g_globalvars.time + FLAG_RETURN_TIME;
	if (tossed)
	{
		flag->s.v.nextthink = g_globalvars.time + 0.75;
		flag->think = (func_t) FlagResetOwner;
	}
	else
	{
		flag->s.v.owner = EDICT_TO_PROG(flag);
	}

	G_bprint(2, "%s", p->netname);
	if (streq(getteam(p), "red"))
	{
		G_bprint(2, " %s the %s flag!\n", tossed ? redtext("tossed") : redtext("lost"),
					redtext("BLUE"));
	}
	else
	{
		G_bprint(2, " %s the %s flag!\n", tossed ? redtext("tossed") : redtext("lost"),
					redtext("RED"));
	}

	for (p1 = world; (p1 = find_plr(p1));)
	{
		if (strneq(getteam(p), getteam(p1)))
		{
			p1->carrier_hurt_time = -1;
		}
	}

	refresh_plus_scores(); // update players status bar faster
}

void FlagStatus(void)
{
	gedict_t *flag1, *flag2;

	if (!isCTF())
	{
		return;
	}

	flag1 = find(world, FOFCLSN, "item_flag_team1");
	flag2 = find(world, FOFCLSN, "item_flag_team2");

	if (!flag1 || !flag2)
	{
		return;
	}

	if (self->ct == ctSpec)
	{
		switch ((int)flag1->cnt)
		{
			case FLAG_AT_BASE:
				G_sprint(self, 2, "The %s flag is in base.\n", redtext("RED"));
				break;

			case FLAG_CARRIED:
				G_sprint(self, 2, "%s has the %s flag.\n",
							PROG_TO_EDICT(flag1->s.v.owner)->netname, redtext("RED"));
				break;

			case FLAG_DROPPED:
				G_sprint(self, 2, "The %s flag is lying about.\n", redtext("RED"));
				break;
		}

		switch ((int)flag2->cnt)
		{
			case FLAG_AT_BASE:
				G_sprint(self, 2, "The %s flag is in base. ", redtext("BLUE"));
				break;

			case FLAG_CARRIED:
				G_sprint(self, 2, "%s has the %s flag. ",
							PROG_TO_EDICT(flag1->s.v.owner)->netname, redtext("BLUE"));
				break;

			case FLAG_DROPPED:
				G_sprint(self, 2, "The %s flag is lying about. ", redtext("BLUE"));
				break;
		}

		return;
	}

	// Swap flags so that flag1 is "your" flag
	if (streq(getteam(self), "blue"))
	{
		gedict_t *swap = flag1;
		flag1 = flag2;
		flag2 = swap;
	}

	switch ((int)flag1->cnt)
	{
		case FLAG_AT_BASE:
			G_sprint(self, 2, "Your flag is in base. ");
			break;

		case FLAG_CARRIED:
			G_sprint(self, 2, "%s has your flag. ", PROG_TO_EDICT(flag1->s.v.owner)->netname);
			break;

		case FLAG_DROPPED:
			G_sprint(self, 2, "Your flag is lying about. ");
			break;
	}

	switch ((int)flag2->cnt)
	{
		case FLAG_AT_BASE:
			G_sprint(self, 2, "The enemy flag is in their base.\n");
			break;

		case FLAG_CARRIED:
			if (self == PROG_TO_EDICT(flag2->s.v.owner))
				G_sprint(self, 2, "You have the enemy flag.\n");
			else
				G_sprint(self, 2, "%s has the enemy flag.\n",
							PROG_TO_EDICT(flag2->s.v.owner)->netname);
			break;

		case FLAG_DROPPED:
			G_sprint(self, 2, "The enemy flag is lying about.\n");
			break;

		default:
			G_sprint(self, 2, "\n");
	}
}

void CTF_CheckFlagsAsKeys(void)
{
	gedict_t *flag1, *flag2;

	if (!isCTF())
	{
		return;
	}

	flag1 = find(world, FOFCLSN, "item_flag_team1"); // RED
	flag2 = find(world, FOFCLSN, "item_flag_team2"); // BLUE

	if (!flag1 || !flag2)
	{
		return;
	}

	// remove keys/flags.
	self->s.v.items = (int)self->s.v.items & ~(IT_KEY1 | IT_KEY2);

	// add gold/RED.
	if (flag1->cnt != FLAG_AT_BASE)
	{
		self->s.v.items = (int)self->s.v.items | IT_KEY2;
	}

	// add silver/BLUE.
	if (flag2->cnt != FLAG_AT_BASE)
	{
		self->s.v.items = (int)self->s.v.items | IT_KEY1;
	}
}

void norunes(void)
{
	if (match_in_progress && !k_matchLess)
	{
		return;
	}

	if (!isCTF())
	{
		G_sprint(self, 2, "Can't do this in non CTF mode\n");

		return;
	}

	cvar_toggle_msg(self, "k_ctf_runes", redtext("runes"));

	// In matchless mode, toggling runes normally won't do anything since match is already in progress. Call this to handle this scenario.
	if (k_matchLess)
	{
		// If a player is carrying a rune when runes are disabled, get rid of it
		if (!cvar("k_ctf_runes"))
		{
			gedict_t *p;
			for (p = world; (p = find_plr(p));)
			{
				p->ctf_flag -= (p->ctf_flag & (CTF_RUNE_MASK));
				p->maxspeed = cvar("sv_maxspeed"); // Reset speed, in case was carrying haste
			}
		}

		SpawnRunes(cvar("k_ctf_runes")); // Toggle runes
	}
}

void nohook(void)
{
	if (match_in_progress && !k_matchLess)
	{
		return;
	}

	if (!isCTF())
	{
		G_sprint(self, 2, "Can't do this in non CTF mode\n");

		return;
	}

	cvar_toggle_msg(self, "k_ctf_hook", redtext("hook"));

	// In matchless mode, toggling hook normally won't do anything since match is already in progress. Call this to handle this scenario.
	if (k_matchLess)
	{
		if (cvar("k_ctf_hook"))
		{
			AddHook(true);
		}
		else
		{
			AddHook(false);
		}
	}
}

void noga(void)
{
	if (match_in_progress && !k_matchLess)
	{
		return;
	}

	if (!isCTF())
	{
		G_sprint(self, 2, "Can't do this in non CTF mode\n");

		return;
	}

	cvar_toggle_msg(self, "k_ctf_ga", redtext("green armor"));
}

void mctf(void)
{
	if (match_in_progress && !k_matchLess)
	{
		return;
	}

	if (!isCTF())
	{
		G_sprint(self, 2, "Can't do this in non CTF mode\n");

		return;
	}

	if (!cvar("k_ctf_hook") && !cvar("k_ctf_runes"))
	{
		G_sprint(self, 2, "Already done\n");
		return;
	}

	cvar_fset("k_ctf_hook", 0);
	cvar_fset("k_ctf_runes", 0);

	G_sprint(self, 2, "%s turn off: %s\n", getname(self), redtext("hook & runes"));

	// In matchless mode, toggling runes and hook normally won't do anything since match is already in progress. Call this to handle this scenario.
	if (k_matchLess)
	{
		// If a player is carrying a rune when runes are disabled, get rid of it
		if (!cvar("k_ctf_runes"))
		{
			gedict_t *p;
			for (p = world; (p = find_plr(p));)
			{
				p->ctf_flag -= (p->ctf_flag & (CTF_RUNE_MASK));
				p->maxspeed = cvar("sv_maxspeed"); // Reset speed, in case was carrying haste
			}
		}

		SpawnRunes(0);
		AddHook(false);
	}
}

void CTFBasedSpawn(void)
{
	if (match_in_progress && !k_matchLess)
	{
		return;
	}

	if (!isCTF())
	{
		G_sprint(self, 2, "Can't do this in non CTF mode\n");

		return;
	}

	if (cvar("k_ctf_based_spawn") && (find_cnt(FOFCLSN, "info_player_deathmatch") <= 1))
	{
		G_sprint(self, 2, "Spawn on base enforced due to map limitation\n");

		return;
	}

	cvar_toggle_msg(self, "k_ctf_based_spawn", redtext("spawn on base"));
}

void CTF_Obituary(gedict_t *targ, gedict_t *attacker)
{
	qbool carrier_bonus = false;
	qbool flagdefended = false;
	gedict_t *head;
	char *attackerteam;

	if (!isCTF())
	{
		return;
	}

	attackerteam = getteam(attacker);

	// 2 point bonus for killing enemy flag carrier
	if (targ->ctf_flag & CTF_FLAG)
	{
		attacker->ps.c_frags++;
		attacker->s.v.frags += 2;
		attacker->ps.ctf_points += 2;
		attacker->carrier_frag_time = g_globalvars.time;
		//G_sprint( attacker, 1, "Enemy flag carrier killed: 2 bonus frags\n" );
	}

	// defending carrier from aggressive player
	if ((targ->carrier_hurt_time + CARRIER_DEFEND_TIME > g_globalvars.time)
			&& !(attacker->ctf_flag & CTF_FLAG))
	{
		carrier_bonus = true;
		attacker->ps.c_defends++;
		attacker->s.v.frags += 2;
		attacker->ps.ctf_points += 2;
		// Yes, aggressive is spelled wrong.. but dont want to fix now and break stat parsers
		G_bprint(2, "%s defends %s's flag carrier against an aggressive enemy\n", attacker->netname,
					streq(getteam(attacker), "red") ? redtext("RED") : redtext("BLUE"));
	}

	head = trap_findradius(world, targ->s.v.origin, 400);
	while (head)
	{
		if (head->ct == ctPlayer)
		{
			if ((head->ctf_flag & CTF_FLAG) && (head != attacker)
					&& streq(getteam(head), getteam(attacker)) && !carrier_bonus)
			{
				attacker->ps.c_defends++;
				attacker->s.v.frags++;
				attacker->ps.ctf_points++;
				G_bprint(2, "%s defends %s's flag carrier\n", attacker->netname,
							streq(getteam(attacker), "red") ? redtext("RED") : redtext("BLUE"));
			}
		}

		if ((streq(getteam(attacker), "red") && streq(head->classname, "item_flag_team1"))
				|| (streq(getteam(attacker), "blue") && streq(head->classname, "item_flag_team2")))
		{
			flagdefended = true;
			attacker->ps.f_defends++;
			attacker->s.v.frags += 2;
			attacker->ps.ctf_points += 2;
			G_bprint(2, "%s defends the %s flag\n", attacker->netname,
						streq(getteam(attacker), "red") ? redtext("RED") : redtext("BLUE"));
		}

		head = trap_findradius(head, targ->s.v.origin, 400);
	}

	// Defend bonus if attacker is close to flag even if target is not
	head = trap_findradius(world, attacker->s.v.origin, 400);
	while (head)
	{
		if ((streq(head->classname, "item_flag_team1") && streq(attackerteam, "red"))
				|| (streq(head->classname, "item_flag_team2") && streq(attackerteam, "blue")))
		{
			if (!flagdefended)
			{
				attacker->ps.f_defends++;
				attacker->s.v.frags += 2;
				attacker->ps.ctf_points += 2;
				G_bprint(2, "%s defends the %s flag\n", attacker->netname,
							streq(attackerteam, "red") ? redtext("RED") : redtext("BLUE"));
			}
		}
		head = trap_findradius(head, attacker->s.v.origin, 400);
	}
}
