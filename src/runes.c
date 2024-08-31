/*
 *  $Id$
 */

#include "g_local.h"

void RegenLostRot(void);
void RuneRespawn(void);
void RuneTouch(void);
void RuneResetOwner(void);
char* GetRuneSpawnName(void);

void DoDropRune(int rune, qbool on_respawn)
{
	gedict_t *item, *pos = NULL;
	float movetype = MOVETYPE_NONE;

	cl_refresh_plus_scores(self);

	if (on_respawn)
	{
		if (rune & CTF_RUNE_RES)
		{
			pos = ez_find(world, "item_rune_res");
		}
		else if (rune & CTF_RUNE_STR)
		{
			pos = ez_find(world, "item_rune_str");
		}
		else if (rune & CTF_RUNE_HST)
		{
			pos = ez_find(world, "item_rune_hst");
		}
		else if (rune & CTF_RUNE_RGN)
		{
			pos = ez_find(world, "item_rune_rgn");
		}
	}

	// No dedicated rune position found, just toss on self
	if (pos == NULL)
	{
		pos = self;
		if (on_respawn)
		{
			// Rune respawn, bounce adds randomization.
			movetype = (int) cvar("k_ctf_rune_bounce") & 1 ? MOVETYPE_BOUNCE : MOVETYPE_TOSS;
		}
		else
		{
			// Drop rune due to player died.
			movetype = MOVETYPE_TOSS;
		}
	}

	item = spawn();
	setorigin(item, pos->s.v.origin[0], pos->s.v.origin[1], pos->s.v.origin[2] - 24);
	item->classname = "rune";
	item->ctf_flag = rune;
	item->s.v.velocity[0] = i_rnd(-100, 100);
	item->s.v.velocity[1] = i_rnd(-100, 100);
	item->s.v.velocity[2] = 400;
	item->s.v.flags = FL_ITEM;
	item->s.v.solid = SOLID_TRIGGER;
	item->s.v.movetype = movetype;

	if (rune & CTF_RUNE_RES)
	{
		setmodel(item, "progs/end1.mdl");
	}
	else if (rune & CTF_RUNE_STR)
	{
		setmodel(item, "progs/end2.mdl");
	}
	else if (rune & CTF_RUNE_HST)
	{
		setmodel(item, "progs/end3.mdl");
	}
	else if (rune & CTF_RUNE_RGN)
	{
		setmodel(item, "progs/end4.mdl");
	}

	setsize(item, -16, -16, 0, 16, 16, 56);
	item->touch = (func_t) RuneTouch;
	item->s.v.nextthink = g_globalvars.time + 90;
	item->think = (func_t) RuneRespawn;

	// qqshka, add spawn sound to rune if rune respawned, not for player dropped from corpse rune
	if (on_respawn)
	{
		sound(item, CHAN_VOICE, "items/itembk2.wav", 1, ATTN_NORM);	// play respawn sound
	}
}

void DoTossRune(int rune)
{
	gedict_t *item;

	cl_refresh_plus_scores(self);

	item = spawn();
	item->ctf_flag = rune;
	item->classname = "rune";
	item->s.v.flags = FL_ITEM;
	item->s.v.solid = SOLID_TRIGGER;
	item->s.v.movetype = (int) cvar("k_ctf_rune_bounce") & 2 ? MOVETYPE_BOUNCE : MOVETYPE_TOSS;

	trap_makevectors(self->s.v.v_angle);

	if (self->s.v.v_angle[0])
	{
		item->s.v.velocity[0] = g_globalvars.v_forward[0] * 300 + g_globalvars.v_up[0] * 200;
		item->s.v.velocity[1] = g_globalvars.v_forward[1] * 300 + g_globalvars.v_up[1] * 200;
		item->s.v.velocity[2] = g_globalvars.v_forward[2] * 300 + g_globalvars.v_up[2] * 200;
	}
	else
	{
		aim(item->s.v.velocity);
		VectorScale(item->s.v.velocity, 300, item->s.v.velocity);
		item->s.v.velocity[2] = 200;
	}

	if (rune & CTF_RUNE_RES)
	{
		setmodel(item, "progs/end1.mdl");
	}
	else if (rune & CTF_RUNE_STR)
	{
		setmodel(item, "progs/end2.mdl");
	}
	else if (rune & CTF_RUNE_HST)
	{
		setmodel(item, "progs/end3.mdl");
	}
	else if (rune & CTF_RUNE_RGN)
	{
		setmodel(item, "progs/end4.mdl");
	}

	setorigin(item, self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] - 24);
	setsize(item, -16, -16, 0, 16, 16, 56);
	item->s.v.owner = EDICT_TO_PROG(self);
	item->touch = (func_t) RuneTouch;
	item->s.v.nextthink = g_globalvars.time + 0.75;
	item->think = (func_t) RuneResetOwner;
}

void DropRune(void)
{
	if (self->ctf_flag & CTF_RUNE_RES)
	{
		DoDropRune( CTF_RUNE_RES, false);
		self->ps.res_time += g_globalvars.time - self->rune_pickup_time;
	}

	if (self->ctf_flag & CTF_RUNE_STR)
	{
		DoDropRune( CTF_RUNE_STR, false);
		self->ps.str_time += g_globalvars.time - self->rune_pickup_time;
	}

	if (self->ctf_flag & CTF_RUNE_HST)
	{
		DoDropRune( CTF_RUNE_HST, false);
		self->ps.hst_time += g_globalvars.time - self->rune_pickup_time;
	}

	if (self->ctf_flag & CTF_RUNE_RGN)
	{
		DoDropRune( CTF_RUNE_RGN, false);
		self->ps.rgn_time += g_globalvars.time - self->rune_pickup_time;
	}

	self->ctf_flag -= (self->ctf_flag & (CTF_RUNE_MASK));
	// self->s.v.items -= ( (int)self->s.v.items & (CTF_RUNE_MASK) );
}

void TossRune(void)
{
	if (self->ctf_flag & CTF_RUNE_RES)
	{
		DoTossRune( CTF_RUNE_RES);
		self->ps.res_time += g_globalvars.time - self->rune_pickup_time;
	}

	if (self->ctf_flag & CTF_RUNE_STR)
	{
		DoTossRune( CTF_RUNE_STR);
		self->ps.str_time += g_globalvars.time - self->rune_pickup_time;
	}

	if (self->ctf_flag & CTF_RUNE_HST)
	{
		DoTossRune( CTF_RUNE_HST);
		self->ps.hst_time += g_globalvars.time - self->rune_pickup_time;
		self->maxspeed = cvar("sv_maxspeed");
	}

	if (self->ctf_flag & CTF_RUNE_RGN)
	{
		gedict_t *regenrot = spawn();
		DoTossRune( CTF_RUNE_RGN);
		self->ps.rgn_time += g_globalvars.time - self->rune_pickup_time;
		regenrot->s.v.nextthink = g_globalvars.time + 5;
		regenrot->think = (func_t) RegenLostRot;
		regenrot->s.v.owner = EDICT_TO_PROG(self);
	}

	self->ctf_flag -= (self->ctf_flag & (CTF_RUNE_MASK));
	//self->s.v.items -= ( (int)self->s.v.items & (CTF_RUNE_MASK) );
}

void RegenLostRot(void)
{
	other = PROG_TO_EDICT(self->s.v.owner);
	if ((other->s.v.health < 101) || (other->ctf_flag & CTF_RUNE_RGN)
			|| ((int)other->s.v.items & IT_SUPERHEALTH))
	{
		ent_remove(self);

		return;
	}

	other->s.v.health--;
	self->s.v.nextthink = g_globalvars.time + 1;
}

void RuneResetOwner(void)
{
	self->s.v.owner = EDICT_TO_PROG(self);
	self->think = (func_t) RuneRespawn;
	self->s.v.nextthink = g_globalvars.time + 90;
}

void RuneRespawn(void)
{
	int rune = self->ctf_flag;

	ent_remove(self);
	self = SelectSpawnPoint(GetRuneSpawnName());
	DoDropRune(rune, true);
}

void RuneTouch(void)
{
	if (other->ct != ctPlayer)
	{
		return;
	}

	if (ISDEAD(other))
	{
		return;
	}

	if (!k_practice)
	{
		if (match_in_progress != 2)
		{
			return;
		}
	}

	if (other == PROG_TO_EDICT(self->s.v.owner))
	{
		return;
	}

	if (self->think == (func_t)RuneRespawn)
	{
		self->s.v.nextthink = g_globalvars.time + 90;
	}

	if (other->ctf_flag & CTF_RUNE_MASK)
	{
		if (g_globalvars.time > other->rune_notify_time)
		{
			other->rune_notify_time = g_globalvars.time + 10;
			G_sprint(other, 1, "You already have a rune. Use \"%s\" to drop\n",
						redtext("tossrune"));
		}

		return;
	}

	cl_refresh_plus_scores(other);

	other->ctf_flag |= self->ctf_flag;
	other->rune_pickup_time = g_globalvars.time;

	if (other->ctf_flag & CTF_RUNE_RES)
	{
		// other->s.v.items = (int)other->s.v.items | IT_SIGIL1;
		G_sprint(other, 2, "You got the %s rune\n", redtext("resistance"));
	}

	if (other->ctf_flag & CTF_RUNE_STR)
	{
		// other->s.v.items = (int)other->s.v.items | IT_SIGIL2;
		G_sprint(other, 2, "You got the %s rune\n", redtext("strength"));
	}

	if (other->ctf_flag & CTF_RUNE_HST)
	{
		other->maxspeed *= (cvar("k_ctf_rune_power_hst") / 8) + 1;
		// other->s.v.items = (int)other->s.v.items | CTF_RUNE_HST;
		G_sprint(other, 2, "You got the %s rune\n", redtext("haste"));
	}

	if (other->ctf_flag & CTF_RUNE_RGN)
	{
		// other->s.v.items = (int)other->s.v.items | CTF_RUNE_RGN;
		G_sprint(other, 2, "You got the %s rune\n", redtext("regeneration"));
	}

	sound(other, CHAN_ITEM, "weapons/lock4.wav", 1, ATTN_NORM);
	stuffcmd(other, "bf\n");
	ent_remove(self);
}

char* GetRuneSpawnName(void)
{
	char *runespawn;

	if (cvar("k_ctf_based_spawn") == 1)
	{
		runespawn = g_random() < 0.5 ? "info_player_team1" : "info_player_team2";
	}
	else
	{
		// we'll just use the player spawn point selector for runes as well
		runespawn = "info_player_deathmatch";
	}

	return runespawn;
}

// try to find a unique spawn position for a rune given the NULL-terminated
// list of other runes
gedict_t* UniqueRuneSpawn(int rune_type, int nrunes, gedict_t **runes)
{
	char *spawnname;
	int i, nspawns;
	qbool unique;
	gedict_t *e;

	spawnname = GetRuneSpawnName();

	for (e = world, nspawns = 0; (e = ez_find(e, spawnname)); nspawns++);

	for (i = 0; i < nspawns; i++)
	{
		self = SelectSpawnPoint(spawnname);

		unique = true;

		for (i = 0; i < nrunes; i++)
		{
			if (runes && self == runes[i])
			{
				unique = false;
				break;
			}
		}

		if (unique)
		{
			DoDropRune(rune_type, true);
			return self;
		}
	}

	// Unable to find a unique spawn, drop anyway
	DoDropRune(rune_type, true);

	return self;
}

// spawn/remove runes
void SpawnRunes(qbool yes)
{
	gedict_t *oself, *e, *runes[4];
	int nrunes = 0;

	for (e = world; (e = find(e, FOFCLSN, "rune"));)
	{
		ent_remove(e);
	}

	if (!yes)
	{
		return;
	}

	oself = self;

	memset(runes, 0, sizeof(runes));

	if (cvar("k_ctf_rune_power_res") > 0)
	{
		runes[nrunes] = UniqueRuneSpawn(CTF_RUNE_RES, nrunes, runes);
		nrunes++;
	}

	if (cvar("k_ctf_rune_power_str") > 0)
	{
		runes[nrunes] = UniqueRuneSpawn(CTF_RUNE_STR, nrunes, runes);
		nrunes++;
	}

	if (cvar("k_ctf_rune_power_hst") > 0)
	{
		runes[nrunes] = UniqueRuneSpawn(CTF_RUNE_HST, nrunes, runes);
		nrunes++;
	}

	if (cvar("k_ctf_rune_power_rgn") > 0)
	{
		UniqueRuneSpawn(CTF_RUNE_RGN, nrunes, runes);
	}

	self = oself;
}

void ResistanceSound(gedict_t *player)
{
	if (player->ctf_flag & CTF_RUNE_RES)
	{
		if (player->rune_sound_time < g_globalvars.time)
		{
			player->rune_sound_time = g_globalvars.time + 1;
			sound(player, CHAN_ITEM, "rune/rune1.wav", 1, ATTN_NORM);
		}
	}
}

void HasteSound(gedict_t *player)
{
	if (player->ctf_flag & CTF_RUNE_HST)
	{
		if (player->rune_sound_time < g_globalvars.time)
		{
			player->rune_sound_time = g_globalvars.time + 1;
			sound(player, CHAN_ITEM, "rune/rune3.wav", 1, ATTN_NORM);
		}
	}
}

void RegenerationSound(gedict_t *player)
{
	if (player->ctf_flag & CTF_RUNE_RGN)
	{
		if (player->rune_sound_time < g_globalvars.time)
		{
			player->rune_sound_time = g_globalvars.time + 1;
			sound(player, CHAN_ITEM, "rune/rune4.wav", 1, ATTN_NORM);
		}
	}
}

void CheckStuffRune(void)
{
	char *rune = "";

	if (cvar("k_instagib"))
	{
		if (self->i_agmr)
		{
			self->items2 = (int)self->items2 | (CTF_RUNE_RES << 5);

			return;
		}
	}

	if (!isCTF())
	{
		self->items2 = 0; // no runes/sigils in HUD

		if (self->last_rune && iKey(self, "runes"))
		{
			self->last_rune = NULL;
			stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "set rune \"\"\n");
		}

		return;
	}

	self->items2 = (self->ctf_flag & CTF_RUNE_MASK) << 5;

	if (!iKey(self, "runes"))
	{
		return;
	}

	if (self->ctf_flag & CTF_RUNE_RES)
	{
		rune = "res";
	}
	else if (self->ctf_flag & CTF_RUNE_STR)
	{
		rune = "str";
	}
	else if (self->ctf_flag & CTF_RUNE_HST)
	{
		rune = "hst";
	}
	else if (self->ctf_flag & CTF_RUNE_RGN)
	{
		rune = "rgn";
	}
	else
	{
		rune = "";
	}

	if (!self->last_rune || strneq(rune, self->last_rune))
	{
		self->last_rune = rune;
		stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "set rune \"%s\"\n", rune);
	}
}
