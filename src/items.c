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

#include "g_local.h"

void SP_item_artifact_invisibility(void);
void SP_item_artifact_super_damage(void);
void SP_item_artifact_invulnerability(void);

void TookWeaponHandler(gedict_t *p, int new_wp, qbool from_backpack);
void BotsBackpackTouchedNonPlayer(gedict_t *backpack, gedict_t *entity);
void BotsBackpackDropped(gedict_t *self, gedict_t *pack);
void BotsPowerupTouchedNonPlayer(gedict_t *powerup, gedict_t *touch_ent);
void BotsPowerupDropped(gedict_t *player, gedict_t *powerup);

static qbool ItemTouched(gedict_t *item, gedict_t *player)
{
#ifdef BOT_SUPPORT
	return (self->fb.item_touch && self->fb.item_touch(item, player));
#else
	return false;
#endif
}

static void ItemTaken(gedict_t *item, gedict_t *player)
{
	TeamplayEventItemTaken(player, item);

#ifdef BOT_SUPPORT
	if (self->fb.item_taken)
	{
		self->fb.item_taken(item, player);
	}
#endif
}

void SUB_regen(void)
{
	if (!deathmatch && (skill < 3))
	{
		return; // do not respawn items in deathmatch 0 except on nightmare skill
	}

	self->model = self->mdl;	// restore original model
	self->s.v.solid = SOLID_TRIGGER;	// allow it to be touched again
	sound(self, CHAN_VOICE, "items/itembk2.wav", 1, ATTN_NORM);	// play respawn sound
	setorigin(self, PASSVEC3(self->s.v.origin));

#ifdef BOT_SUPPORT
	if (self->fb.item_respawned)
	{
		self->fb.item_respawned(self);
	}
#endif
}

void SUB_regen_powerups(void)
{
	extern void ktpro_autotrack_predict_powerup(void);

	// attempt to predict player which awating such powerup
	ktpro_autotrack_predict_powerup();

	self->think = (func_t) SUB_regen;
	self->s.v.nextthink = g_globalvars.time + AUTOTRACK_POWERUPS_PREDICT_TIME;
}

void PlaceItem(void)
{
	self->s.v.solid = SOLID_TRIGGER;
	self->s.v.movetype = MOVETYPE_TOSS;
	self->s.v.flags = FL_ITEM;
	self->mdl = strnull(self->model) ? self->mdl : self->model; // save .mdl if .model is not init
	self->mdl = strnull(self->mdl) ? "" : self->mdl; // init .mdl with empty string if not set

	SetVector(self->s.v.velocity, 0, 0, 0);
	self->s.v.origin[2] += 6;

	if (!droptofloor(self))
	{
		G_Printf("Bonus item fell out of level at  '%f %f %f'\n", self->s.v.origin[0],
					self->s.v.origin[1], self->s.v.origin[2]);
		ent_remove(self);
	}

	// if powerups disabled - hide
	if ((int)self->s.v.items & (IT_INVISIBILITY | IT_INVULNERABILITY | IT_SUIT | IT_QUAD))
	{
		if (!Get_Powerups() || (((int)self->s.v.items & IT_INVISIBILITY) && !cvar("k_pow_r"))
				|| (((int)self->s.v.items & IT_INVULNERABILITY) && !cvar("k_pow_p"))
				|| (((int)self->s.v.items & IT_SUIT) && !cvar("k_pow_s"))
				|| (((int)self->s.v.items & IT_QUAD) && !cvar("k_pow_q")))
		{
			self->model = "";
			self->s.v.solid = SOLID_NOT;
		}
	}
}

/*
 ============
 PlaceItemIngame

 It is StartItem + PlaceItem, set some required fields for an item and drop it with random velocity,
 used for dropable powerups.
 ============
 */
void PlaceItemIngame(void)
{
	self->s.v.solid = SOLID_TRIGGER;
	self->s.v.movetype = MOVETYPE_TOSS;
	self->s.v.flags = FL_ITEM;

	self->mdl = self->model; // qqshka - save model ASAP

	self->s.v.velocity[2] = 300;
	self->s.v.velocity[0] = -100 + (g_random() * 200);
	self->s.v.velocity[1] = -100 + (g_random() * 200);

	self->s.v.nextthink = self->cnt; // remove it with the time left on it
	self->think = (func_t) SUB_Remove;
}

/*
 ============
 StartItem

 Sets the clipping size and plants the object on the floor
 ============
 */
void StartItem(void)
{
//	G_bprint(2, "StartItem: %s\n", self->classname);

	self->mdl = self->model; // qqshka - save model ASAP

	self->s.v.nextthink = g_globalvars.time + 0.2;	// items start after other solids
	self->think = (func_t) PlaceItem;
}

/*
 =========================================================================

 HEALTH BOX

 =========================================================================
 */
//
// T_Heal: add health to an gedict_t*, limiting health to max_health
// "ignore" will ignore max_health limit
//
float T_Heal(gedict_t *e, float healamount, float ignore)
{
	float real_healamount;
	char *playername;

	if (ISDEAD(e))
	{
		return 0;
	}

	if ((!ignore) && (e->s.v.health >= other->s.v.max_health))
	{
		return 0;
	}

	real_healamount = e->s.v.health; // save health

	healamount = ceil(healamount);

	e->s.v.health = e->s.v.health + healamount;
	if ((!ignore) && (e->s.v.health >= other->s.v.max_health))
	{
		e->s.v.health = other->s.v.max_health;
	}

	if (e->s.v.health > 250)
	{
		e->s.v.health = 250;
	}

	real_healamount = e->s.v.health - real_healamount; // so heal amount is current - old health

	playername = e->netname;

	/*
	 log_printf( "\t\t\t<pickmi time=\"%f\" item=\"health_%d\" player=\"%s\" value=\"%d\"/>\n",
	 g_globalvars.time - match_start_time,
	 (int)healamount,
	 cleantext(playername),
	 (int)real_healamount );
	 */
	log_printf("\t\t<event>\n"
				"\t\t\t<pick_mapitem>\n"
				"\t\t\t\t<time>%f</time>\n"
				"\t\t\t\t<item>health_%d</item>\n"
				"\t\t\t\t<player>%s</player>\n"
				"\t\t\t\t<value>%d</value>\n"
				"\t\t\t</pick_mapitem>\n"
				"\t\t</event>\n",
				g_globalvars.time - match_start_time, (int)healamount, cleantext(playername),
				(int)real_healamount);

	return 1;
}

void health_touch(void);
void item_megahealth_rot(void);

/*QUAKED item_health (.3 .3 1) (0 0 0) (32 32 32) rotten megahealth
 Health box. Normally gives 25 points.
 Rotten box heals 15 points,
 megahealth will add 100 health, then
 rot you down to your maximum health limit,
 one point per second.
 */

void SP_item_health(void)
{
	self->touch = (func_t) health_touch;
	self->tp_flags = it_health;

	if ((int)self->s.v.spawnflags & H_ROTTEN)
	{
		setmodel(self, "maps/b_bh10.bsp");
		self->noise = "items/r_item1.wav";
		self->healamount = 15;
		self->healtype = 0;
	}
	else if ((int)self->s.v.spawnflags & H_MEGA)
	{
		setmodel(self, "maps/b_bh100.bsp");
		self->noise = "items/r_item2.wav";
		self->healamount = 100;
		self->healtype = 2;
		self->tp_flags = it_mh;
	}
	else
	{
		setmodel(self, "maps/b_bh25.bsp");
		self->noise = "items/health1.wav";
		self->healamount = 25;
		self->healtype = 1;
	}

	setsize(self, 0, 0, 0, 32, 32, 56);
	StartItem();
}

void health_touch(void)
{
	if (other->ct != ctPlayer)
	{
		return;
	}

	if (ISDEAD(other))
	{
		return;
	}

	if (ItemTouched(self, other))
	{
		return;
	}

	if (deathmatch == 4)
	{
		if (other->invincible_time > 0)
		{
			return;
		}
	}

	if ((match_in_progress != 2) || !readytostart())
	{
		return;
	}

	if (self->healtype == 2)	// Megahealth?  Ignore max_health...
	{
		if (other->s.v.health >= 250)
		{
			return;
		}

		if (!T_Heal(other, self->healamount, 1))
		{
			return;
		}

		other->ps.itm[itHEALTH_100].tooks++;

		mi_print(other, IT_SUPERHEALTH, va("%s got Megahealth", getname(other)));
	}
	else
	{
		if (!T_Heal(other, self->healamount, 0))
		{
			return;
		}

		switch ((int)self->healamount)
		{
			case 15:
				other->ps.itm[itHEALTH_15].tooks++;
				break;

			case 25:
				other->ps.itm[itHEALTH_25].tooks++;
				break;
		}
	}

	G_sprint(other, PRINT_LOW, "You receive %.0f health\n", self->healamount);

// health touch sound
	sound(other, CHAN_ITEM, self->noise, 1, ATTN_NORM);

	stuffcmd(other, "bf\n");

	self->model = "";
	self->s.v.solid = SOLID_NOT;

	// Megahealth = rot down the player's super health
	if (self->healtype == 2)
	{
		other->s.v.items = (int)other->s.v.items | IT_SUPERHEALTH;
		if (deathmatch != 4)
		{
			self->s.v.nextthink = g_globalvars.time + 5;
			self->think = (func_t) item_megahealth_rot;
			stuffcmd_flags(other, STUFFCMD_DEMOONLY, "//ktx took %d %d %d\n", NUM_FOR_EDICT(self),
							0, NUM_FOR_EDICT(other));
		}

		self->s.v.owner = EDICT_TO_PROG(other);
	}
	else
	{
		if (deathmatch != 2)
		{
			// deathmatch 2 is the silly old rules
			self->s.v.nextthink = g_globalvars.time + 20;
			self->think = (func_t) SUB_regen;
		}
	}

	ItemTaken(self, other);

	activator = other;
	SUB_UseTargets();	// fire all targets / killtargets
}

void item_megahealth_rot(void)
{
	other = PROG_TO_EDICT(self->s.v.owner);

	if (other->s.v.health > other->s.v.max_health)
	{
		if (!(other->ctf_flag & CTF_RUNE_RGN))
		{
			other->s.v.health -= 1;
		}

		self->s.v.nextthink = g_globalvars.time + 1;
#ifdef BOT_SUPPORT
		if (self->fb.item_affect)
		{
			self->fb.item_affect(self, other);
		}
#endif

		return;
	}

// it is possible for a player to die and respawn between rots, so don't
// just blindly subtract the flag off
	other->s.v.items -= (int)other->s.v.items & IT_SUPERHEALTH;

	if (deathmatch != 2)	// deathmatch 2 is silly old rules
	{
		self->s.v.nextthink = g_globalvars.time + 20;
		stuffcmd_flags(other, STUFFCMD_DEMOONLY, "//ktx timer %d %d\n", NUM_FOR_EDICT(self), 20);
		self->think = (func_t) SUB_regen;
	}

#ifdef BOT_SUPPORT
	if (self->fb.item_affect)
	{
		self->fb.item_affect(self, other);
	}
#endif
}

/*
 ===============================================================================

 ARMOR

 ===============================================================================
 */
void armor_touch(void)
{
	float type = 0;
	float value = 0;
	float real_value = 0;
	int bit = 0;
	char *playername;
	itemName_t armorType = itNONE;

	if (ISDEAD(other))
	{
		return;
	}

	if (lgc_enabled())
	{
		return;
	}

	if (other->ct != ctPlayer)
	{
		return;
	}

	if (ItemTouched(self, other))
	{
		return;
	}

	if ((match_in_progress != 2) || !readytostart())
	{
		return;
	}

	if (deathmatch == 4)
	{
		if (other->invincible_time > 0)
		{
			return;
		}
	}

	if (!strcmp(self->classname, "item_armor1"))
	{
		armorType = itGA;
		type = (k_yawnmode ? 0.4 : 0.3); // Yawnmode: changed armor protection
		value = 100;
		bit = IT_ARMOR1;
	}
	else if (!strcmp(self->classname, "item_armor2"))
	{
		armorType = itYA;
		type = (k_yawnmode ? 0.6 : 0.6); // Yawnmode: changed armor protection
		value = 150;
		bit = IT_ARMOR2;
	}
	else if (!strcmp(self->classname, "item_armorInv"))
	{
		armorType = itRA;
		type = (k_yawnmode ? 0.8 : 0.8); // Yawnmode: changed armor protection
		value = 200;
		bit = IT_ARMOR3;
	}
	else
	{
		return;
	}

	// check if we have more armor than we trying to pick up.
	// We add 1.0e-6 so floaing point comparision is happy,
	// not all systems require it but on some this bugs in your face.
	if ((other->s.v.armortype * other->s.v.armorvalue + 1.0e-6) >= (type * value))
	{
		return;
	}

	mi_print(other, bit, va("%s got %s", getname(other), self->netname));

	if (armorType != itNONE)
	{
		adjust_pickup_time(&other->it_pickup_time[armorType], &other->ps.itm[armorType].time);
		other->it_pickup_time[armorType] = g_globalvars.time;
		other->ps.itm[armorType].tooks++;
		switch (armorType)
		{
			case itGA:
				adjust_pickup_time(&other->it_pickup_time[itYA], &other->ps.itm[itYA].time);
				adjust_pickup_time(&other->it_pickup_time[itRA], &other->ps.itm[itRA].time);
				break;

			case itYA:
				adjust_pickup_time(&other->it_pickup_time[itGA], &other->ps.itm[itGA].time);
				adjust_pickup_time(&other->it_pickup_time[itRA], &other->ps.itm[itRA].time);
				break;

			case itRA:
				adjust_pickup_time(&other->it_pickup_time[itGA], &other->ps.itm[itGA].time);
				adjust_pickup_time(&other->it_pickup_time[itYA], &other->ps.itm[itYA].time);
				break;

			default:
				break;
		}
	}

	real_value = other->s.v.armorvalue;

	other->s.v.armortype = type;
	other->s.v.armorvalue = value;
	other->s.v.items += -((int)other->s.v.items & ( IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3)) + bit;

	self->s.v.solid = SOLID_NOT;
	self->model = "";
	if (deathmatch != 2)
	{
		self->s.v.nextthink = g_globalvars.time + 20;
		stuffcmd_flags(other, STUFFCMD_DEMOONLY, "//ktx took %d %d %d\n", NUM_FOR_EDICT(self), 20,
						NUM_FOR_EDICT(other));
	}
	self->think = (func_t) SUB_regen;

	real_value = value - real_value;

	playername = other->netname;

	/*
	 log_printf( "\t\t\t<pickmi time=\"%f\" item=\"%s\" player=\"%s\" value=\"%d\" />\n",
	 g_globalvars.time - match_start_time,
	 self->classname,
	 cleantext(playername),
	 (int)real_value );
	 */
	log_printf("\t\t<event>\n"
				"\t\t\t<pick_mapitem>\n"
				"\t\t\t\t<time>%f</time>\n"
				"\t\t\t\t<item>%s</item>\n"
				"\t\t\t\t<player>%s</player>\n"
				"\t\t\t\t<value>%d</value>\n"
				"\t\t\t</pick_mapitem>\n"
				"\t\t</event>\n",
				g_globalvars.time - match_start_time, self->classname, cleantext(playername),
				(int)real_value);

	G_sprint(other, PRINT_LOW, "You got the %s\n", self->netname);
// armor touch sound
	sound(other, CHAN_AUTO, "items/armor1.wav", 1, ATTN_NORM);
	stuffcmd(other, "bf\n");

	activator = other;
	SUB_UseTargets();	// fire all targets / killtargets

	ItemTaken(self, other);
}

/*QUAKED item_armor1 (0 .5 .8) (-16 -16 0) (16 16 32)
 */

void SP_item_armor1(void)
{
	self->touch = (func_t) armor_touch;
	setmodel(self, "progs/armor.mdl");
	self->netname = "Green Armor";
	self->s.v.skin = 0;
	self->tp_flags = it_ga;
	setsize(self, -16, -16, 0, 16, 16, 56);
	StartItem();
}

/*QUAKED item_armor2 (0 .5 .8) (-16 -16 0) (16 16 32)
 */

void SP_item_armor2(void)
{
	self->touch = (func_t) armor_touch;
	setmodel(self, "progs/armor.mdl");
	self->netname = "Yellow Armor";
	self->s.v.skin = 1;
	self->tp_flags = it_ya;
	setsize(self, -16, -16, 0, 16, 16, 56);
	StartItem();
}

/*QUAKED item_armorInv (0 .5 .8) (-16 -16 0) (16 16 32)
 */

void SP_item_armorInv(void)
{
	self->touch = (func_t) armor_touch;
	setmodel(self, "progs/armor.mdl");
	self->netname = "Red Armor";
	self->s.v.skin = 2;
	self->tp_flags = it_ra;
	setsize(self, -16, -16, 0, 16, 16, 56);
	StartItem();
}

/*
 ===============================================================================

 WEAPONS

 ===============================================================================
 */

void bound_other_ammo(void)
{
	if (other->s.v.ammo_shells > 100)
	{
		other->s.v.ammo_shells = 100;
	}

	if (other->s.v.ammo_nails > 200)
	{
		other->s.v.ammo_nails = 200;
	}

	if (other->s.v.ammo_rockets > 100)
	{
		other->s.v.ammo_rockets = 100;
	}

	if (other->s.v.ammo_cells > 100)
	{
		other->s.v.ammo_cells = 100;
	}
}
float RankForWeapon(float w)
{
	if (w == IT_LIGHTNING)
	{
		return 1;
	}

	if (w == IT_ROCKET_LAUNCHER)
	{
		return 2;
	}

	if (w == IT_SUPER_NAILGUN)
	{
		return 3;
	}

	if (w == IT_GRENADE_LAUNCHER)
	{
		return 4;
	}

	if (w == IT_SUPER_SHOTGUN)
	{
		return 5;
	}

	if (w == IT_NAILGUN)
	{
		return 6;
	}

	return 7;
}

float WeaponCode(float w)
{
	if (w == IT_SHOTGUN)
	{
		return 2;
	}

	if (w == IT_SUPER_SHOTGUN)
	{
		return 3;
	}

	if (w == IT_NAILGUN)
	{
		return 4;
	}

	if (w == IT_SUPER_NAILGUN)
	{
		return 5;
	}

	if (w == IT_GRENADE_LAUNCHER)
	{
		return 6;
	}

	if (w == IT_ROCKET_LAUNCHER)
	{
		return 7;
	}

	if (w == IT_LIGHTNING)
	{
		return 8;
	}

	return 1;
}

/*
 =============
 Deathmatch_Weapon

 Deathmatch weapon change rules for picking up a weapon

 .float          ammo_shells, ammo_nails, ammo_rockets, ammo_cells;
 =============
 */
void Deathmatch_Weapon(int new)
{
	int or, nr;

	if ((self->s.v.weapon == IT_HOOK) && self->s.v.button0)
	{
		return;
	}

// change self.weapon if desired
	or = RankForWeapon(self->s.v.weapon);
	nr = RankForWeapon(new);
	if (nr < or)
	{
		self->s.v.weapon = new;
	}
}

void DoWeaponChange(int new, qbool backpack)
{
	int w_switch = iKey(self, backpack ? "b_switch" : "w_switch");

	if (self->isBot)
	{
		return;
	}

	if (!w_switch)
	{
		if (iKey(self, "w_rank"))
		{
			w_switch = W_BestWeapon();
		}
		else
		{
			w_switch = 8;
		}
	}

	if (WeaponCode(new) <= w_switch)
	{
		if (((int)(self->s.v.flags)) & FL_INWATER)
		{
			if (new != IT_LIGHTNING)
			{
				Deathmatch_Weapon(new);
			}
		}
		else
		{
			Deathmatch_Weapon(new);
		}
	}

	W_SetCurrentAmmo();
}

/*
 =============
 weapon_touch
 =============
 */
float W_BestWeapon(void);

void weapon_touch(void)
{
	int hadammo = 0, new = 0;
	gedict_t *stemp;
	int leave;
	int real_ammo = 0;
	int k_freshteams = cvar("k_freshteams");
	int limit_sweep_ammo = cvar("k_freshteams_limit_sweep_ammo");
	int k_nosweep = cvar("k_nosweep");
	int weapon_time = k_freshteams ? cvar("k_freshteams_weapon_time") : 30;
	char *playername;

	if (ISDEAD(other))
	{
		return;
	}

	if (other->ct != ctPlayer)
	{
		return;
	}

	if (ItemTouched(self, other))
	{
		return;
	}

	if ((match_in_progress != 2) || !readytostart())
	{
		return;
	}

	if ((deathmatch == 2) || (deathmatch == 3) || (deathmatch == 5) || coop)
	{
		leave = 1;
	}
	else
	{
		leave = 0;
	}

	if (!strcmp(self->classname, "weapon_nailgun"))
	{
		if ((leave || k_nosweep) && ((int)other->s.v.items & IT_NAILGUN))
		{
			return;
		}

		hadammo = other->s.v.ammo_nails;
		new = IT_NAILGUN;

		if (k_freshteams && limit_sweep_ammo && ((int)other->s.v.items & IT_NAILGUN))
		{
			other->s.v.ammo_nails += cvar("k_freshteams_sweep_ng_ammo");
		}
		else
		{
			other->s.v.ammo_nails += 30;
		}
	}
	else if (!strcmp(self->classname, "weapon_supernailgun"))
	{
		if ((leave || k_nosweep) && ((int)other->s.v.items & IT_SUPER_NAILGUN))
		{
			return;
		}

		hadammo = other->s.v.ammo_nails;
		new = IT_SUPER_NAILGUN;

		if (k_freshteams && limit_sweep_ammo && ((int)other->s.v.items & IT_SUPER_NAILGUN))
		{
			other->s.v.ammo_nails += cvar("k_freshteams_sweep_sng_ammo");
		}
		else
		{
			other->s.v.ammo_nails += 30;
		}
	}
	else if (!strcmp(self->classname, "weapon_supershotgun"))
	{
		if ((leave || k_nosweep) && ((int)other->s.v.items & IT_SUPER_SHOTGUN))
		{
			return;
		}

		hadammo = other->s.v.ammo_shells;
		new = IT_SUPER_SHOTGUN;

		if (k_freshteams && limit_sweep_ammo && ((int)other->s.v.items & IT_SUPER_SHOTGUN))
		{
			other->s.v.ammo_shells += cvar("k_freshteams_sweep_ssg_ammo");
		}
		else
		{
			other->s.v.ammo_shells += 5;
		}
	}
	else if (!strcmp(self->classname, "weapon_rocketlauncher"))
	{
		if ((leave || k_nosweep) && ((int)other->s.v.items & IT_ROCKET_LAUNCHER))
		{
			return;
		}

		hadammo = other->s.v.ammo_rockets;
		new = IT_ROCKET_LAUNCHER;

		if (k_freshteams && limit_sweep_ammo && ((int)other->s.v.items & IT_ROCKET_LAUNCHER))
		{
			other->s.v.ammo_rockets += cvar("k_freshteams_sweep_rl_ammo");
		}
		else
		{
			other->s.v.ammo_rockets += 5;
		}

		if (!first_rl_taken)
		{
			extern void ktpro_autotrack_on_first_rl(gedict_t *dude);

			ktpro_autotrack_on_first_rl(other);
			first_rl_taken = true;
		}
	}
	else if (!strcmp(self->classname, "weapon_grenadelauncher"))
	{
		if ((leave || k_nosweep) && ((int)other->s.v.items & IT_GRENADE_LAUNCHER))
		{
			return;
		}

		hadammo = other->s.v.ammo_rockets;
		new = IT_GRENADE_LAUNCHER;

		if (k_freshteams && limit_sweep_ammo && ((int)other->s.v.items & IT_GRENADE_LAUNCHER))
		{
			other->s.v.ammo_rockets += cvar("k_freshteams_sweep_gl_ammo");
		}
		else
		{
			other->s.v.ammo_rockets += 5;
		}
	}
	else if (!strcmp(self->classname, "weapon_lightning"))
	{
		if ((leave || k_nosweep) && ((int)other->s.v.items & IT_LIGHTNING))
		{
			return;
		}

		hadammo = other->s.v.ammo_cells;
		new = IT_LIGHTNING;

		if (k_freshteams && limit_sweep_ammo && ((int)other->s.v.items & IT_LIGHTNING))
		{
			other->s.v.ammo_cells += cvar("k_freshteams_sweep_lg_ammo");
		}
		else
		{
			other->s.v.ammo_cells += 15;
		}
	}
	else
	{
		G_Error("weapon_touch: unknown classname");
	}

	TookWeaponHandler(other, new, false);
	mi_print(other, new, va("%s got %s", getname(other), self->netname));

	G_sprint(other, PRINT_LOW, "You got the %s\n", self->netname);
// weapon touch sound
	sound(other, CHAN_AUTO, "weapons/pkup.wav", 1, ATTN_NORM);
	stuffcmd(other, "bf\n");

	bound_other_ammo();

	if (!strcmp(self->classname, "weapon_nailgun"))
	{
		real_ammo = other->s.v.ammo_nails - hadammo;
	}
	else if (!strcmp(self->classname, "weapon_supernailgun"))
	{
		real_ammo = other->s.v.ammo_nails - hadammo;
	}
	else if (!strcmp(self->classname, "weapon_supershotgun"))
	{
		real_ammo = other->s.v.ammo_shells - hadammo;
	}
	else if (!strcmp(self->classname, "weapon_rocketlauncher"))
	{
		real_ammo = other->s.v.ammo_rockets - hadammo;
	}
	else if (!strcmp(self->classname, "weapon_grenadelauncher"))
	{
		real_ammo = other->s.v.ammo_rockets - hadammo;
	}
	else if (!strcmp(self->classname, "weapon_lightning"))
	{
		real_ammo = other->s.v.ammo_cells - hadammo;
	}

	playername = other->netname;

	/*
	 log_printf( "\t\t\t<pickmi time=\"%f\" item=\"%s\" player=\"%s\" value=\"%d\" />\n",
	 g_globalvars.time - match_start_time,
	 self->classname,
	 cleantext(playername),
	 real_ammo );
	 */
	log_printf("\t\t<event>\n"
				"\t\t\t<pick_mapitem>\n"
				"\t\t\t\t<time>%f</time>\n"
				"\t\t\t\t<item>%s</item>\n"
				"\t\t\t\t<player>%s</player>\n"
				"\t\t\t\t<value>%d</value>\n"
				"\t\t\t</pick_mapitem>\n"
				"\t\t</event>\n",
				g_globalvars.time - match_start_time, self->classname, cleantext(playername),
				real_ammo);

// change to the weapon
	other->s.v.items = (int)other->s.v.items | new;

	stemp = self;
	self = other;

	DoWeaponChange(new, false); // change to the weapon

	self = stemp;

	// If dmm2 or dmm3 or dmm5 or coop.
	if (leave)
	{
		ItemTaken(self, other);

		return;
	}

	// remove it in single player, or setup for respawning in deathmatch
	self->model = "";
	self->s.v.solid = SOLID_NOT;
	// At this point it is either dmm0 (singleplayer) or dmm1 or perhaps dmm4 (if for some reason weapon was not removed at match start),
	// we still try to use SUB_regen and do final decision there if we should regen item.
	if (deathmatch != 2)
	{
		self->s.v.nextthink = g_globalvars.time + weapon_time;
		stuffcmd_flags(other, STUFFCMD_DEMOONLY, "//ktx took %d %d %d\n", NUM_FOR_EDICT(self),
						weapon_time, NUM_FOR_EDICT(other));
	}

	self->think = (func_t) SUB_regen;

	activator = other;
	SUB_UseTargets();	// fire all targets / killtargets

	ItemTaken(self, other);
}

/*QUAKED weapon_supershotgun (0 .5 .8) (-16 -16 0) (16 16 32)
 */

void SP_weapon_supershotgun(void)
{
	setmodel(self, "progs/g_shot.mdl");

	self->s.v.weapon = IT_SUPER_SHOTGUN;
	self->netname = "Double-barrelled Shotgun";
	self->touch = (func_t) weapon_touch;
	self->tp_flags = it_ssg;

	setsize(self, -16, -16, 0, 16, 16, 56);

	StartItem();
}

/*QUAKED weapon_nailgun (0 .5 .8) (-16 -16 0) (16 16 32)
 */

void SP_weapon_nailgun(void)
{
	setmodel(self, "progs/g_nail.mdl");

	self->s.v.weapon = IT_NAILGUN;
	self->netname = "nailgun";
	self->touch = (func_t) weapon_touch;
	self->tp_flags = it_ng;

	setsize(self, -16, -16, 0, 16, 16, 56);

	StartItem();
}

/*QUAKED weapon_supernailgun (0 .5 .8) (-16 -16 0) (16 16 32)
 */

void SP_weapon_supernailgun(void)
{
	setmodel(self, "progs/g_nail2.mdl");

	self->s.v.weapon = IT_SUPER_NAILGUN;
	self->netname = "Super Nailgun";
	self->touch = (func_t) weapon_touch;
	self->tp_flags = it_sng;

	setsize(self, -16, -16, 0, 16, 16, 56);

	StartItem();

}

/*QUAKED weapon_grenadelauncher (0 .5 .8) (-16 -16 0) (16 16 32)
 */

void SP_weapon_grenadelauncher(void)
{
	setmodel(self, "progs/g_rock.mdl");

	self->s.v.weapon = 3;
	self->netname = "Grenade Launcher";
	self->touch = (func_t) weapon_touch;
	self->tp_flags = it_gl;

	setsize(self, -16, -16, 0, 16, 16, 56);

	StartItem();
}

/*QUAKED weapon_rocketlauncher (0 .5 .8) (-16 -16 0) (16 16 32)
 */

void SP_weapon_rocketlauncher(void)
{
	setmodel(self, "progs/g_rock2.mdl");

	self->s.v.weapon = 3;
	self->netname = "Rocket Launcher";
	self->touch = (func_t) weapon_touch;
	self->tp_flags = it_rl;

	setsize(self, -16, -16, 0, 16, 16, 56);

	StartItem();
}

/*QUAKED weapon_lightning (0 .5 .8) (-16 -16 0) (16 16 32)
 */

void SP_weapon_lightning(void)
{
	setmodel(self, "progs/g_light.mdl");

	self->s.v.weapon = 3;
	self->netname = "Thunderbolt";
	self->touch = (func_t) weapon_touch;
	self->tp_flags = it_lg;

	setsize(self, -16, -16, 0, 16, 16, 56);

	StartItem();
}

/*
 ===============================================================================

 AMMO

 ===============================================================================
 */

void ammo_touch(void)
{
	int ammo, weapon, best;
	int real_ammo = 0;
	qbool freshteams_fast_ammo = (cvar("k_freshteams") && cvar("k_freshteams_fast_ammo"));
	gedict_t *stemp;
	char *playername;

	if (ISDEAD(other))
	{
		return;
	}

	if (other->ct != ctPlayer)
	{
		return;
	}

	if (ItemTouched(self, other))
	{
		return;
	}

	if ((match_in_progress != 2) || !readytostart())
	{
		return;
	}

// if the player was using his best weapon, change up to the new one if better          
	stemp = self;
	self = other;
	best = W_BestWeapon(); // save best weapon before update ammo
	self = stemp;

	ammo = self->aflag;
	weapon = self->s.v.weapon;

	if (weapon == 1) // shotgun
	{
		if (other->s.v.ammo_shells >= 100)
		{
			return;
		}

		real_ammo = other->s.v.ammo_shells;
		other->s.v.ammo_shells += ammo;
	}
	else if (weapon == 2) // spikes
	{
		if (other->s.v.ammo_nails >= 200)
		{
			return;
		}

		real_ammo = other->s.v.ammo_nails;
		other->s.v.ammo_nails += ammo;
	}
	else if (weapon == 3) // rockets
	{
		if (other->s.v.ammo_rockets >= 100)
		{
			return;
		}

		real_ammo = other->s.v.ammo_rockets;
		other->s.v.ammo_rockets += ammo;
	}
	else if (weapon == 4) // cells
	{
		if (other->s.v.ammo_cells >= 100)
		{
			return;
		}

		real_ammo = other->s.v.ammo_cells;
		other->s.v.ammo_cells += ammo;
	}

	bound_other_ammo();

	if (weapon == 1)
	{
		real_ammo = other->s.v.ammo_shells - real_ammo;
	}
	else if (weapon == 2)
	{
		real_ammo = other->s.v.ammo_nails - real_ammo;
	}
	else if (weapon == 3)
	{
		real_ammo = other->s.v.ammo_rockets - real_ammo;
	}
	else if (weapon == 4)
	{
		real_ammo = other->s.v.ammo_cells - real_ammo;
	}

	playername = other->netname;

	/*
	 log_printf( "\t\t\t<pickmi time=\"%f\" item=\"%s\" player=\"%s\" value=\"%d\" />\n",
	 g_globalvars.time - match_start_time,
	 self->classname,
	 cleantext(playername),
	 real_ammo );
	 */
	log_printf("\t\t<event>\n"
				"\t\t\t<pick_mapitem>\n"
				"\t\t\t\t<time>%f</time>\n"
				"\t\t\t\t<item>%s</item>\n"
				"\t\t\t\t<player>%s</player>\n"
				"\t\t\t\t<value>%d</value>\n"
				"\t\t\t</pick_mapitem>\n"
				"\t\t</event>\n",
				g_globalvars.time - match_start_time, self->classname, cleantext(playername),
				real_ammo);

	G_sprint(other, PRINT_LOW, "You got the %s\n", self->netname);
// ammo touch sound
	sound(other, CHAN_ITEM, "weapons/lock4.wav", 1, ATTN_NORM);
	stuffcmd(other, "bf\n");

// change to a better weapon if appropriate
// before we got ammo we use best weapon - best weapon may change due to ammo, so check this
	if (other->s.v.weapon == best)
	{
		stemp = self;
		self = other;

		DoWeaponChange(W_BestWeapon(), false); // change to the weapon

		self = stemp;
	}

// if changed current ammo, update it
	stemp = self;
	self = other;

	W_SetCurrentAmmo();

	self = stemp;

// remove it in single player, or setup for respawning in deathmatch
	self->model = "";
	self->s.v.solid = SOLID_NOT;
	if (deathmatch != 2)
	{
		self->s.v.nextthink = g_globalvars.time + 30;
	}

// Xian -- If playing in DM 3.0 mode, halve the time ammo respawns        

	if ((deathmatch == 3) || (deathmatch == 5))
	{
		self->s.v.nextthink = g_globalvars.time + 15;
	}

	// If playing freshteams and fast_ammo is enabled, set ammo respawn time same as weapons
	if (freshteams_fast_ammo)
	{
		self->s.v.nextthink = g_globalvars.time + cvar("k_freshteams_weapon_time");
	}

	self->think = (func_t) SUB_regen;
	ItemTaken(self, other);

	activator = other;
	SUB_UseTargets();	// fire all targets / killtargets
}

#define  WEAPON_BIG2  1

/*QUAKED item_shells (0 .5 .8) (0 0 0) (32 32 32) big
 */

void SP_item_shells(void)
{
	self->touch = (func_t) ammo_touch;

	if ((int)(self->s.v.spawnflags) & WEAPON_BIG2)
	{
		setmodel(self, "maps/b_shell1.bsp");
		self->aflag = 40;
	}
	else
	{
		setmodel(self, "maps/b_shell0.bsp");
		self->aflag = 20;
	}

	self->s.v.weapon = 1;
	self->netname = "shells";
	self->classname = "item_shells";
	self->tp_flags = it_shells;

	setsize(self, 0, 0, 0, 32, 32, 56);
	StartItem();
}

/*QUAKED item_spikes (0 .5 .8) (0 0 0) (32 32 32) big
 */

void SP_item_spikes(void)
{
	qbool old_style = streq(self->classname, "item_weapon");

	self->touch = (func_t) ammo_touch;

	if ((int)(self->s.v.spawnflags) & WEAPON_BIG2)
	{
		setmodel(self, "maps/b_nail1.bsp");
		self->aflag = (old_style ? 40 : 50);
	}
	else
	{
		setmodel(self, "maps/b_nail0.bsp");
		self->aflag = (old_style ? 20 : 25);
	}

	self->s.v.weapon = 2;
	self->netname = (old_style ? "spikes" : "nails"); // hehe, different message when u pick different nails ammo
	self->classname = "item_spikes";
	self->tp_flags = it_nails;

	setsize(self, 0, 0, 0, 32, 32, 56);
	StartItem();
}

/*QUAKED item_rockets (0 .5 .8) (0 0 0) (32 32 32) big
 */

void SP_item_rockets(void)
{
	self->touch = (func_t) ammo_touch;

	if ((int)(self->s.v.spawnflags) & WEAPON_BIG2)
	{
		setmodel(self, "maps/b_rock1.bsp");
		self->aflag = 10;
	}
	else
	{
		setmodel(self, "maps/b_rock0.bsp");
		self->aflag = 5;
	}

	self->s.v.weapon = 3;
	self->netname = "rockets";
	self->classname = "item_rockets";
	self->tp_flags = it_rockets;

	setsize(self, 0, 0, 0, 32, 32, 56);
	StartItem();
}

/*QUAKED item_cells (0 .5 .8) (0 0 0) (32 32 32) big
 */

void SP_item_cells(void)
{
	self->touch = (func_t) ammo_touch;

	if ((int)(self->s.v.spawnflags) & WEAPON_BIG2)
	{
		setmodel(self, "maps/b_batt1.bsp");
		self->aflag = 12;
	}
	else
	{
		setmodel(self, "maps/b_batt0.bsp");
		self->aflag = 6;
	}

	self->s.v.weapon = 4;
	self->netname = "cells";
	self->classname = "item_cells";
	self->tp_flags = it_cells;

	setsize(self, 0, 0, 0, 32, 32, 56);
	StartItem();
}

/*QUAKED item_weapon (0 .5 .8) (0 0 0) (32 32 32) shotgun rocket spikes big
 DO NOT USE THIS!!!! IT WILL BE REMOVED!
 */

#define WEAPON_SHOTGUN  1
#define WEAPON_ROCKET  2
#define WEAPON_SPIKES  4
#define WEAPON_BIG  8
void SP_item_weapon(void)
{
	if ((int)(self->s.v.spawnflags) & WEAPON_SHOTGUN)
	{
		self->s.v.spawnflags = (((int)(self->s.v.spawnflags) & WEAPON_BIG) ? WEAPON_BIG2 : 0);
		SP_item_shells();

		return;
	}

	if ((int)(self->s.v.spawnflags) & WEAPON_SPIKES)
	{
		self->s.v.spawnflags = (((int)(self->s.v.spawnflags) & WEAPON_BIG) ? WEAPON_BIG2 : 0);
		SP_item_spikes();

		return;
	}

	if ((int)(self->s.v.spawnflags) & WEAPON_ROCKET)
	{
		self->s.v.spawnflags = (((int)(self->s.v.spawnflags) & WEAPON_BIG) ? WEAPON_BIG2 : 0);
		SP_item_rockets();

		return;
	}

	SUB_Remove(); // was unknown item, remove it
}

/*
 ===============================================================================

 KEYS

 ===============================================================================
 */
void key_touch(void)
{
//gedict_t*    stemp;
//float             best;
	char *playername;

	if (other->ct != ctPlayer)
	{
		return;
	}

	if (ISDEAD(other))
	{
		return;
	}

	if ((int)other->s.v.items & (int)self->s.v.items)
	{
		return;
	}

	if ((match_in_progress != 2) || !readytostart())
	{
		return;
	}

	playername = other->netname;

	/*
	 log_printf( "\t\t\t<pickmi time=\"%f\" item=\"%s\" player=\"%s\" value=\"%d\" />\n",
	 g_globalvars.time - match_start_time,
	 self->classname,
	 cleantext(playername),
	 0 );
	 */
	log_printf("\t\t<event>\n"
				"\t\t\t<pick_mapitem>\n"
				"\t\t\t\t<time>%f</time>\n"
				"\t\t\t\t<item>%s</item>\n"
				"\t\t\t\t<player>%s</player>\n"
				"\t\t\t\t<value>%d</value>\n"
				"\t\t\t</pick_mapitem>\n"
				"\t\t</event>\n",
				g_globalvars.time - match_start_time, self->classname, cleantext(playername), 0);

	G_sprint(other, PRINT_LOW, "You got the %s\n", self->netname);

	sound(other, CHAN_ITEM, self->noise, 1, ATTN_NORM);
	stuffcmd(other, "bf\n");
	other->s.v.items = (int)other->s.v.items | (int)self->s.v.items;

	if (!coop)
	{
		self->s.v.solid = SOLID_NOT;
		self->model = "";
	}

	activator = other;
	SUB_UseTargets();	// fire all targets / killtargets
}

void key_setsounds(void)
{
	if (world->worldtype == 0)
	{
		trap_precache_sound("misc/medkey.wav");
		self->noise = "misc/medkey.wav";
	}

	if (world->worldtype == 1)
	{
		trap_precache_sound("misc/runekey.wav");
		self->noise = "misc/runekey.wav";
	}

	if (world->worldtype == 2)
	{
		trap_precache_sound("misc/basekey.wav");
		self->noise = "misc/basekey.wav";
	}
}

/*QUAKED item_key1 (0 .5 .8) (-16 -16 -24) (16 16 32)
 SILVER key
 In order for keys to work
 you MUST set your maps
 worldtype to one of the
 following:
 0: medieval
 1: metal
 2: base
 */

void SP_item_key1(void)
{
	if (world->worldtype == 0)
	{
		trap_precache_model("progs/w_s_key.mdl");
		setmodel(self, "progs/w_s_key.mdl");
		self->netname = "silver key";
	}
	else if (world->worldtype == 1)
	{
		trap_precache_model("progs/m_s_key.mdl");
		setmodel(self, "progs/m_s_key.mdl");
		self->netname = "silver runekey";
	}
	else if (world->worldtype == 2)
	{
		trap_precache_model("progs/b_s_key.mdl");
		setmodel(self, "progs/b_s_key.mdl");
		self->netname = "silver keycard";
	}

	key_setsounds();
	self->touch = (func_t) key_touch;
	self->s.v.items = IT_KEY1;
	setsize(self, -16, -16, -24, 16, 16, 32);
	StartItem();
}

/*QUAKED item_key2 (0 .5 .8) (-16 -16 -24) (16 16 32)
 GOLD key
 In order for keys to work
 you MUST set your maps
 worldtype to one of the
 following:
 0: medieval
 1: metal
 2: base
 */

void SP_item_key2(void)
{
	if (world->worldtype == 0)
	{
		trap_precache_model("progs/w_g_key.mdl");
		setmodel(self, "progs/w_g_key.mdl");
		self->netname = "gold key";
	}

	if (world->worldtype == 1)
	{
		trap_precache_model("progs/m_g_key.mdl");
		setmodel(self, "progs/m_g_key.mdl");
		self->netname = "gold runekey";
	}

	if (world->worldtype == 2)
	{
		trap_precache_model("progs/b_g_key.mdl");
		setmodel(self, "progs/b_g_key.mdl");
		self->netname = "gold keycard";
	}

	key_setsounds();
	self->touch = (func_t) key_touch;
	self->s.v.items = IT_KEY2;
	setsize(self, -16, -16, -24, 16, 16, 32);
	StartItem();
}

/*
 ===============================================================================

 END OF LEVEL RUNES

 ===============================================================================
 */
void sigil_touch(void)
{
//gedict_t*    stemp;
//float             best;
	char *playername;

	if (other->ct != ctPlayer)
	{
		return;
	}

	if (ISDEAD(other))
	{
		return;
	}

	if ((match_in_progress != 2) || !readytostart())
	{
		return;
	}

	playername = other->netname;

	/*
	 log_printf( "\t\t\t<pickmi time=\"%f\" item=\"%s\" player=\"%s\" value=\"%d\" />\n",
	 g_globalvars.time - match_start_time,
	 self->classname,
	 cleantext(playername),
	 0 );
	 */
	log_printf("\t\t<event>\n"
				"\t\t\t<pick_mapitem>\n"
				"\t\t\t\t<time>%f</time>\n"
				"\t\t\t\t<item>%s</item>\n"
				"\t\t\t\t<player>%s</player>\n"
				"\t\t\t\t<value>%d</value>\n"
				"\t\t\t</pick_mapitem>\n"
				"\t\t</event>\n",
				g_globalvars.time - match_start_time, self->classname, cleantext(playername), 0);

	G_centerprint(other, "You got the rune!");

	sound(other, CHAN_ITEM, self->noise, 1, ATTN_NORM);
	stuffcmd(other, "bf\n");
	self->s.v.solid = SOLID_NOT;
	self->model = "";
	g_globalvars.serverflags = (int)(g_globalvars.serverflags)
			| ((int)(self->s.v.spawnflags) & 15);
	self->classname = "";	// so rune doors won't find it

	activator = other;
	SUB_UseTargets();	// fire all targets / killtargets
}

/*QUAKED item_sigil (0 .5 .8) (-16 -16 -24) (16 16 32) E1 E2 E3 E4
 End of level sigil, pick up to end episode and return to jrstart.
 */

void SP_item_sigil(void)
{
	if (!(int)(self->s.v.spawnflags))
	{
		G_Error("item_sigil no spawnflags");
	}

	trap_precache_sound("misc/runekey.wav");
	self->noise = "misc/runekey.wav";

	if ((int)(self->s.v.spawnflags) & 1)
	{
		trap_precache_model("progs/end1.mdl");
		setmodel(self, "progs/end1.mdl");
	}

	if ((int)(self->s.v.spawnflags) & 2)
	{
		trap_precache_model("progs/end2.mdl");
		setmodel(self, "progs/end2.mdl");
	}

	if ((int)(self->s.v.spawnflags) & 4)
	{
		trap_precache_model("progs/end3.mdl");
		setmodel(self, "progs/end3.mdl");
	}

	if ((int)(self->s.v.spawnflags) & 8)
	{
		trap_precache_model("progs/end4.mdl");
		setmodel(self, "progs/end4.mdl");
	}

	self->touch = (func_t) sigil_touch;
	setsize(self, -16, -16, -24, 16, 16, 32);
	StartItem();
	self->tp_flags = it_rune1;
}

/*
 ===============================================================================

 POWERUPS

 ===============================================================================
 */

void ktpro_autotrack_on_powerup_take(gedict_t *dude);

void adjust_pickup_time(float *current, float *total)
{
	if (!current || !*current || !total)
	{
		return;
	}

	*total += (g_globalvars.time - *current);
	*current = 0;
}

void hide_powerups(char *classname)
{
	gedict_t *p;

	if (strnull(classname))
	{
		G_Error("hide_items");
	}

	for (p = world; (p = find(p, FOFCLSN, classname));)
	{
		// simply remove dropable powerups
		if (p->cnt)
		{
			ent_remove(p);
			continue;
		}

		p->s.v.solid = SOLID_NOT;
		p->model = "";
		p->s.v.nextthink = 0;  // disable next think
	}
}

void show_powerups(char *classname)
{
	gedict_t *p;

	if (strnull(classname))
	{
		G_Error("show_items");
	}

	for (p = world; (p = find(p, FOFCLSN, classname));)
	{
		// spawn item if needed
		if (strnull(p->model) || (p->s.v.solid != SOLID_TRIGGER))
		{
			if (match_in_progress == 2)
			{
				// spawn item in 30 seconds if game already running
				p->s.v.nextthink = g_globalvars.time + 30;
				p->s.v.nextthink -= AUTOTRACK_POWERUPS_PREDICT_TIME;
				p->think = (func_t) SUB_regen_powerups;
			}
			else
			{
				// spawn item instantly if game is not running
				p->s.v.nextthink = g_globalvars.time;
				p->think = (func_t) SUB_regen;
			}
		}
	}
}

static void KillQuadThink(void)
{
	ent_remove(self);
}

void DropPowerup(float timeleft, int powerup)
{
	gedict_t *swp = self; // save self
	char *playername;

	if ((timeleft <= 0) || (match_in_progress != 2))
	{
		return;
	}

	if ((powerup != IT_QUAD) && (powerup != IT_INVISIBILITY) && (powerup != IT_INVULNERABILITY)) // only this supported
	{
		return;
	}

	self = spawn(); // WARNING!

	setorigin(self, PASSVEC3(swp->s.v.origin));
	self->cnt = g_globalvars.time + timeleft;

	if (powerup == IT_QUAD)
	{
		SP_item_artifact_super_damage();
		if (k_killquad)
		{
			self->s.v.nextthink = g_globalvars.time + 10;
			self->think = (func_t) KillQuadThink; // ATM just remove self.
		}
	}
	else if (powerup == IT_INVISIBILITY)
	{
		SP_item_artifact_invisibility();
	}
	else if (powerup == IT_INVULNERABILITY)
	{
		SP_item_artifact_invulnerability();
	}
	else
	{
		G_Error("DropPowerup");
	}

	if (k_bloodfest)
	{
		// limit amount of dropped powerups of particular class.
		if (find_cnt( FOFCLSN, self->classname) > 3)
		{
			ent_remove(self);

			self = swp; // restore self!!!

			return;
		}
	}

	playername = swp->netname;

	log_printf("\t\t<event>\n"
				"\t\t\t<drop_powerup>\n"
				"\t\t\t\t<time>%f</time>\n"
				"\t\t\t\t<item>%s</item>\n"
				"\t\t\t\t<player>%s</player>\n"
				"\t\t\t\t<timeleft>%f</timeleft>\n"
				"\t\t\t</drop_powerup>\n"
				"\t\t</event>\n",
				g_globalvars.time - match_start_time, self->classname, cleantext(playername),
				timeleft);

	if (swp->ct == ctPlayer)
	{
		mi_print(
				swp,
				powerup,
				va("%s dropped a %s with %.0f seconds left", swp->netname, self->netname,
					timeleft));
	}

#ifdef BOT_SUPPORT
	BotsPowerupDropped(swp, self);
#endif
	self = swp; // restore self!!!
}

static qbool NeedDropQuad(void)
{
	gedict_t *p;

	for (p = world; (p = find_plr(p));)
	{
		if (ISDEAD(p))
		{
			continue; // ignore dead
		}

		if (p->super_damage_finished > 0)
		{
			return false; // one have quad, so we do not need drop new.
		}
	}

	return !ez_find(world, "item_artifact_super_damage");
}

void DropPowerups(void)
{
	if ((k_killquad || (cvar("dq") && Get_Powerups() && cvar("k_pow_q"))) && !k_berzerk)
	{
		if (k_killquad)
		{
			if (NeedDropQuad())
			{
				DropPowerup(666, IT_QUAD);
			}
		}
		else if (self->super_damage_finished > 0)
		{
			DropPowerup(self->super_damage_finished - g_globalvars.time, IT_QUAD);
		}
	}

	if (cvar("dr") && Get_Powerups() && cvar("k_pow_r"))
	{
		if (self->invisible_finished > 0)
		{
			DropPowerup(self->invisible_finished - g_globalvars.time, IT_INVISIBILITY);
		}
	}
}

void powerup_touch(void)
{
	float *p_cnt = NULL;
	float real_time = 30;
	float old_pu_time = 0;
	char *playername;

	if (strnull(self->classname))
	{
		G_Error("powerup_touch: null classname");
	}

	if (other->ct != ctPlayer)
	{
#ifdef BOT_SUPPORT
		BotsPowerupTouchedNonPlayer(self, other);
#endif
		return;
	}

	if (ISDEAD(other))
	{
		return;
	}

	if (ItemTouched(self, other))
	{
		return;
	}

	if (!k_practice) // #practice mode#
	{
		if ((match_in_progress != 2) || !readytostart())
		{
			return;
		}
	}

	if (!Get_Powerups() || (((int)self->s.v.items & IT_INVISIBILITY) && !cvar("k_pow_r"))
			|| (((int)self->s.v.items & IT_INVULNERABILITY) && !cvar("k_pow_p"))
			|| (((int)self->s.v.items & IT_SUIT) && !cvar("k_pow_s"))
			|| (((int)self->s.v.items & IT_QUAD) && !cvar("k_pow_q")))
	{
		return;
	}

	// if "fair" powerups pickup is activated, don't allow one to pickup
	// powerup if he already has one of the same kind (ie 2 quads)
	if (cvar("k_pow_pickup"))
	{
		if (streq(self->classname, "item_artifact_envirosuit")
				&& other->radsuit_finished > g_globalvars.time)
		{
			return;
		}

		if (streq(self->classname, "item_artifact_invulnerability")
				&& other->invincible_finished > g_globalvars.time)
		{
			return;
		}

		if (streq(self->classname, "item_artifact_invisibility")
				&& other->invisible_finished > g_globalvars.time)
		{
			return;
		}

		if (streq(self->classname, "item_artifact_super_damage")
				&& other->super_damage_finished > g_globalvars.time)
		{
			return;
		}
	}

	G_sprint(other, PRINT_LOW, "You got the %s\n", self->netname);

	self->mdl = self->model;

	if (streq(self->classname, "item_artifact_invulnerability")
			|| streq(self->classname, "item_artifact_invisibility"))
	{
		self->s.v.nextthink = g_globalvars.time + 60 * 5;
		if (isHoonyModeTDM())
		{
			// 5 minute rounds => respawn at 3 minutes
			// 10 minute rounds => respawn at 4 minutes
			// otherwise => default
			if (HM_timelimit() <= 300)
			{
				self->s.v.nextthink = g_globalvars.time + 60 * 3;
			}
			else if (HM_timelimit() <= 600)
			{
				self->s.v.nextthink = g_globalvars.time + 60 * 4;
			}
		}

		if (!k_practice)
		{
			stuffcmd_flags(other, STUFFCMD_DEMOONLY, "//ktx took %d %d %d\n", NUM_FOR_EDICT(self),
							300, NUM_FOR_EDICT(other));
		}
	}
	else
	{
		self->s.v.nextthink = g_globalvars.time + 60;
		if (!k_practice)
		{
			stuffcmd_flags(other, STUFFCMD_DEMOONLY, "//ktx took %d %d %d\n", NUM_FOR_EDICT(self),
							60, NUM_FOR_EDICT(other));
		}
	}

	// all powerups respawn after 30 seconds in practice mode
	if (k_practice) // #practice mode#
	{
		self->s.v.nextthink = g_globalvars.time + 30;
	}

	self->s.v.nextthink -= AUTOTRACK_POWERUPS_PREDICT_TIME;

	self->think = (func_t) SUB_regen_powerups;

// like ktpro
//	sound( other, CHAN_VOICE, self->noise, 1, ATTN_NORM );
	sound(other, CHAN_ITEM, self->noise, 1, ATTN_NORM);
	stuffcmd(other, "bf\n");
	self->s.v.solid = SOLID_NOT;
	setorigin(self, PASSVEC3(self->s.v.origin));
	other->s.v.items = ((int)other->s.v.items) | ((int)self->s.v.items);
	self->model = "";

// do the apropriate action
	if (streq(self->classname, "item_artifact_envirosuit"))
	{
		old_pu_time = other->radsuit_finished;
		other->rad_time = 1;
		other->radsuit_finished = g_globalvars.time + 30;

		if (self->cnt > g_globalvars.time) // is this was a dropped powerup
		{
			p_cnt = &(other->radsuit_finished);
		}
	}
	else if (streq(self->classname, "item_artifact_invulnerability"))
	{
		old_pu_time = other->invincible_finished;
		adjust_pickup_time(&other->it_pickup_time[itPENT], &other->ps.itm[itPENT].time);
		other->it_pickup_time[itPENT] = g_globalvars.time;

		other->ps.itm[itPENT].tooks++;
		other->invincible_time = 1;
		other->invincible_finished = g_globalvars.time + 30;

		if (self->cnt > g_globalvars.time) // is this was a dropped powerup
		{
			p_cnt = &(other->invincible_finished);
		}
	}
	else if (streq(self->classname, "item_artifact_invisibility"))
	{
		old_pu_time = other->invisible_finished;
		adjust_pickup_time(&other->it_pickup_time[itRING], &other->ps.itm[itRING].time);
		other->it_pickup_time[itRING] = g_globalvars.time;

		other->ps.itm[itRING].tooks++;
		other->invisible_time = 1;
		other->invisible_finished = g_globalvars.time + 30;

		if (self->cnt > g_globalvars.time) // is this was a dropped powerup
		{
			p_cnt = &(other->invisible_finished);
		}
	}
	else if (streq(self->classname, "item_artifact_super_damage"))
	{
		old_pu_time = other->super_damage_finished;
		adjust_pickup_time(&other->it_pickup_time[itQUAD], &other->ps.itm[itQUAD].time);
		other->it_pickup_time[itQUAD] = g_globalvars.time;

		other->ps.itm[itQUAD].tooks++;
		other->ps.spree_max_q = max(other->ps.spree_current_q, other->ps.spree_max_q);
		other->ps.spree_current_q = 0;

		if (deathmatch == 4 && !tot_mode_enabled())
		{
			other->s.v.armortype = 0;
			other->s.v.armorvalue = 0;
			other->s.v.ammo_cells = 0;
		}

		other->super_time = 1;
		other->super_damage_finished = g_globalvars.time + 30;

		if (self->cnt > g_globalvars.time) // is this was a dropped powerup
		{
			p_cnt = &(other->super_damage_finished);
		}
	}
	else
	{
		return;
	}

	if (p_cnt)  // is this was a dropped powerup
	{
		float seconds_left = self->cnt - g_globalvars.time; // seconds left on quad.

		// sum up seconds player alredy have and seconds on quad left.
		p_cnt[0] = max(g_globalvars.time, old_pu_time) + seconds_left;
		// do not allow more than 30 seconds of quad anyway.
		p_cnt[0] = min(g_globalvars.time + 30, p_cnt[0]);

		seconds_left = p_cnt[0] - g_globalvars.time;
		real_time = seconds_left;

//		if ( k_bloodfest )
//			G_sprint( other, 2, "Your %s have %d seconds left\n", self->netname, ( int )seconds_left );

		mi_print(
				other,
				self->s.v.items,
				va("%s got a %s with %d seconds left", other->netname, self->netname,
					(int)seconds_left));

		SUB_RM_01(self); // remove later
	}
	else
	{
		mi_print(other, self->s.v.items, va("%s got %s", getname(other), self->netname));
	}

	playername = other->netname;

	log_printf("\t\t<event>\n"
				"\t\t\t<pick_powerup>\n"
				"\t\t\t\t<time>%f</time>\n"
				"\t\t\t\t<item>%s</item>\n"
				"\t\t\t\t<player>%s</player>\n"
				"\t\t\t\t<timeleft>%f</timeleft>\n"
				"\t\t\t</pick_powerup>\n"
				"\t\t</event>\n",
				g_globalvars.time - match_start_time, self->classname, cleantext(playername),
				real_time);

	ktpro_autotrack_on_powerup_take(other);

	ItemTaken(self, other);

	activator = other;
	SUB_UseTargets();	// fire all targets / killtargets
}

/*QUAKED item_artifact_invulnerability (0 .5 .8) (-16 -16 -24) (16 16 32)
 Player is invulnerable for 30 seconds
 */
void SP_item_artifact_invulnerability(void)
{
	qbool b_dp = self->cnt > g_globalvars.time; // dropped powerup by player, not normal spawn

	self->touch = (func_t) powerup_touch;

	self->noise = "items/protect.wav";
	setmodel(self, "progs/invulner.mdl");
	self->netname = "Pentagram of Protection";
	self->classname = "item_artifact_invulnerability";

	self->s.v.effects = (int)self->s.v.effects | EF_RED;

	self->s.v.items = IT_INVULNERABILITY;
	self->tp_flags = it_pent;
	setsize(self, -16, -16, -24, 16, 16, 32);

	if (b_dp)
	{
		PlaceItemIngame();
	}
	else
	{
		StartItem();
	}
}

/*QUAKED item_artifact_envirosuit (0 .5 .8) (-16 -16 -24) (16 16 32)
 Player takes no damage from water or slime for 30 seconds
 */
void SP_item_artifact_envirosuit(void)
{
	self->touch = (func_t) powerup_touch;

	self->noise = "items/suit.wav";
	setmodel(self, "progs/suit.mdl");
	self->netname = "Biosuit";
	self->classname = "item_artifact_envirosuit";

	self->s.v.effects = (int)self->s.v.effects | EF_GREEN;

	self->s.v.items = IT_SUIT;
	self->tp_flags = it_suit;
	setsize(self, -16, -16, -24, 16, 16, 32);
	StartItem();
}

/*QUAKED item_artifact_invisibility (0 .5 .8) (-16 -16 -24) (16 16 32)
 Player is invisible for 30 seconds
 */
void SP_item_artifact_invisibility(void)
{
	qbool b_dp = self->cnt > g_globalvars.time; // dropped powerup by player, not normal spawn

	self->touch = (func_t) powerup_touch;

	self->noise = "items/inv1.wav";
	setmodel(self, "progs/invisibl.mdl");
	self->netname = "Ring of Shadows";
	self->classname = "item_artifact_invisibility";
	self->s.v.items = IT_INVISIBILITY;
	self->tp_flags = it_ring;
	setsize(self, -16, -16, -24, 16, 16, 32);

	if (b_dp)
	{
		PlaceItemIngame();
	}
	else
	{
		StartItem();
	}
}

/*QUAKED item_artifact_super_damage (0 .5 .8) (-16 -16 -24) (16 16 32)
 The next attack from the player will do 4x damage
 */
void SP_item_artifact_super_damage(void)
{
	qbool b_dp = self->cnt > g_globalvars.time; // dropped powerup by player, not normal spawn

	self->touch = (func_t) powerup_touch;

	self->noise = "items/damage.wav";
	setmodel(self, "progs/quaddama.mdl");
	self->classname = "item_artifact_super_damage";
	self->netname = deathmatch == 4 ? "OctaPower" : "Quad Damage";
	self->s.v.items = IT_QUAD;
	self->tp_flags = it_quad;

	self->s.v.effects = (int)self->s.v.effects | EF_BLUE;

	setsize(self, -16, -16, -24, 16, 16, 32);

	if (b_dp)
	{
		PlaceItemIngame();
	}
	else
	{
		StartItem();
	}
}

/*
 ===============================================================================

 PLAYER BACKPACKS

 ===============================================================================
 */

void BackpackTouch(void)
{
	int new;
	gedict_t *stemp, *p;
	float acount, new_shells, new_nails, new_rockets, new_cells;
	char *new_wp = "";
	char *playername;

	if (other->ct != ctPlayer)
	{
#ifdef BOT_SUPPORT
		BotsBackpackTouchedNonPlayer(self, other);
#endif
		return;
	}

	if (ISDEAD(other))
	{
		return;
	}

	if (ItemTouched(self, other))
	{
		return;
	}

	if (match_in_progress != 2)
	{
		return;
	}

	if (deathmatch == 4)
	{
		if (other->invincible_finished)
		{
			return; // we have pent, ignore pack
		}
	}

	//crt -- no backpacks in waiting area
	if (isRA() && !isWinner(other) && !isLoser(other))
	{
		return;
	}

	if (cvar("k_midair") && other->super_damage_finished)
	{
		return; // we have quad, ignore pack
	}

	if (cvar("k_instagib") && other->invisible_finished)
	{
		return; // we have ring, ignore pack
	}

	if ((deathmatch == 4) && lgc_enabled() && (other->s.v.health >= 300))
	{
		return; // don't allow bonus powers, leave pack hanging around
	}

	acount = 0;
	G_sprint(other, PRINT_LOW, "You get ");

	if (deathmatch == 4)
	{
		other->s.v.health += 10;
		G_sprint(other, PRINT_LOW, "10 additional health\n");

		if ((other->s.v.health > 250) && (other->s.v.health < 300))
		{
			sound(other, CHAN_ITEM, "items/protect3.wav", 1, ATTN_NORM);
		}
		else
		{
			sound(other, CHAN_ITEM, "weapons/lock4.wav", 1, ATTN_NORM);
		}

		stuffcmd(other, "bf\n");

		if ((lgc_enabled() || tot_mode_enabled()) && (other->s.v.health > 299))
		{
			// cap & don't allow bonus powers
			other->s.v.health = 300;
		}
		else if (other->s.v.health > 299)
		{
			if (cvar("k_instagib"))
			{
				other->invisible_time = 1;
				other->invisible_finished = g_globalvars.time + 30;
				other->s.v.items = (int)other->s.v.items | IT_INVISIBILITY;
			}
			else
			{
				if (!cvar("k_midair"))
				{
					other->invincible_time = 1;
					other->invincible_finished = g_globalvars.time + 30;
					other->s.v.items = (int)other->s.v.items | IT_INVULNERABILITY;
				}

				other->super_time = 1;
				other->super_damage_finished = g_globalvars.time + 30;
				other->s.v.items = (int)other->s.v.items | IT_QUAD;
				other->ps.mid_bonus++;
			}

			other->s.v.ammo_cells = 0;

			sound(other, CHAN_AUTO, "boss1/sight1.wav", 1, ATTN_NORM);
			stuffcmd(other, "bf\n");

			ktpro_autotrack_on_powerup_take(other);

			G_bprint( PRINT_HIGH, "%s gained bonus powers!!!\n", other->netname);
			other->ps.i_rings++;
		}

		ent_remove(self);

		return;
	}

	new = self->s.v.items;
	if (new)
	{
		if ((new & IT_ROCKET_LAUNCHER) || (new & IT_LIGHTNING))
		{
			stuffcmd_flags(other, STUFFCMD_DEMOONLY, "//ktx bp %d %d\n", NUM_FOR_EDICT(self),
							NUM_FOR_EDICT(other));
		}

		TookWeaponHandler(other, new, true);

		if (!((int)other->s.v.items & new))
		{ // new weapon - so print u got it
			acount = 1;
			G_sprint(other, PRINT_LOW, "the %s", self->netname);

			new_wp = self->netname;

			// FIXME: so specs does't seen this message if player alredy have such weapon, is this BUG?
			mi_print(other, new, va("%s got backpack with %s", getname(other), self->netname));
		}
	}

// change weapons
	new_shells = other->s.v.ammo_shells;
	new_nails = other->s.v.ammo_nails;
	new_rockets = other->s.v.ammo_rockets;
	new_cells = other->s.v.ammo_cells;

	other->s.v.ammo_shells = other->s.v.ammo_shells + self->s.v.ammo_shells;
	other->s.v.ammo_nails = other->s.v.ammo_nails + self->s.v.ammo_nails;
	other->s.v.ammo_rockets = other->s.v.ammo_rockets + self->s.v.ammo_rockets;
	other->s.v.ammo_cells = other->s.v.ammo_cells + self->s.v.ammo_cells;

	other->s.v.items = (int)other->s.v.items | new;

	bound_other_ammo();

	new_shells = other->s.v.ammo_shells - new_shells;
	new_nails = other->s.v.ammo_nails - new_nails;
	new_rockets = other->s.v.ammo_rockets - new_rockets;
	new_cells = other->s.v.ammo_cells - new_cells;

	playername = other->netname;

	/*
	 log_printf( "\t\t\t<pickbp time=\"%f\" weapon=\"%s\" shells=\"%d\" nails=\"%d\" rockets=\"%d\" cells=\"%d\" player=\"%s\" />\n",
	 g_globalvars.time - match_start_time,
	 new_wp,
	 (int)self->s.v.ammo_shells,
	 (int)self->s.v.ammo_nails,
	 (int)self->s.v.ammo_rockets,
	 (int)self->s.v.ammo_cells,
	 cleantext(playername) );
	 */
	log_printf("\t\t<event>\n"
				"\t\t\t<pick_backpack>\n"
				"\t\t\t\t<time>%f</time>\n"
				"\t\t\t\t<weapon>%s</weapon>\n"
				"\t\t\t\t<shells>%d</shells>\n"
				"\t\t\t\t<nails>%d</nails>\n"
				"\t\t\t\t<rockets>%d</rockets>\n"
				"\t\t\t\t<cells>%d</cells>\n"
				"\t\t\t\t<player>%s</player>\n"
				"\t\t\t</pick_backpack>\n"
				"\t\t</event>\n",
				g_globalvars.time - match_start_time, new_wp, (int)self->s.v.ammo_shells,
				(int)self->s.v.ammo_nails, (int)self->s.v.ammo_rockets,
				(int)self->s.v.ammo_cells, cleantext(playername));

	if (self->s.v.ammo_shells)
	{
		if (acount)
		{
			G_sprint(other, PRINT_LOW, ", ");
		}

		acount = 1;
		G_sprint(other, PRINT_LOW, "%.0f shells", self->s.v.ammo_shells);
	}

	if (self->s.v.ammo_nails)
	{
		if (acount)
		{
			G_sprint(other, PRINT_LOW, ", ");
		}

		acount = 1;
		G_sprint(other, PRINT_LOW, "%.0f nails", self->s.v.ammo_nails);
	}

	if (self->s.v.ammo_rockets)
	{
		if (acount)
		{
			G_sprint(other, PRINT_LOW, ", ");
		}

		acount = 1;
		G_sprint(other, PRINT_LOW, "%.0f rockets", self->s.v.ammo_rockets);
	}

	if (self->s.v.ammo_cells)
	{
		if (acount)
		{
			G_sprint(other, PRINT_LOW, ", ");
		}

		acount = 1;
		G_sprint(other, PRINT_LOW, "%.0f cells", self->s.v.ammo_cells);
	}

	if (((deathmatch == 3) || (deathmatch == 5)) && ((WeaponCode(new) == 6) || (WeaponCode(new) == 7))
			&& (other->s.v.ammo_rockets < 5))
	{
		other->s.v.ammo_rockets = 5;
	}

	// detect transferred RL packs, credit the player who dropped the pack with the transfer
	if (isTeam() && (self->s.v.items == IT_ROCKET_LAUNCHER))
	{
		if (streq(self->backpack_team_name, getteam(other)))
		{
			for (p = world; (p = find_plr(p));)
			{
				if (streq(getname(p), self->backpack_player_name))
				{
					p->ps.transferred_RLpacks++;
					break;
				}
			}
		}
	}

	// detect transferred LG packs, credit the player who dropped the pack with the transfer
	if (isTeam() && (self->s.v.items == IT_LIGHTNING))
	{
		if (streq(self->backpack_team_name, getteam(other)))
		{
			for (p = world; (p = find_plr(p));)
			{
				if (streq(getname(p), self->backpack_player_name))
				{
					p->ps.transferred_LGpacks++;
					break;
				}
			}
		}
	}

	G_sprint(other, PRINT_LOW, "\n");
// backpack touch sound
	sound(other, CHAN_ITEM, "weapons/lock4.wav", 1, ATTN_NORM);
	stuffcmd(other, "bf\n");

	ItemTaken(self, other);

	ent_remove(self);

	stemp = self;
	self = other;

	DoWeaponChange(new, true); // change to the weapon

	self = stemp;
}

/*
 ===============
 DropBackpack
 ===============
 */

#define IT_DROPPABLE_WEAPONS (IT_SUPER_SHOTGUN|IT_NAILGUN|IT_SUPER_NAILGUN|IT_GRENADE_LAUNCHER|IT_ROCKET_LAUNCHER|IT_LIGHTNING)

void DropBackpack(void)
{
	gedict_t *item;
	float f1;
	char *playername;
	qbool fresh_packs = (cvar("k_freshteams") && cvar("k_freshteams_limit_packs"));

	if (k_bloodfest)
	{
		return;
	}

	f1 = get_fair_pack();

	if ((match_in_progress != 2) || !cvar("dp"))
	{
		return;
	}

	if (!k_yawnmode) // Yawnmode: pack dropped in yawn mode independantly from death type
	{
		if (dtSUICIDE == self->deathtype)
		{
			return;
		}
	}

	if (!(self->s.v.ammo_shells + self->s.v.ammo_nails + self->s.v.ammo_rockets
			+ self->s.v.ammo_cells) && !((int)self->s.v.weapon & IT_DROPPABLE_WEAPONS)
			&& !(f1 == 2 && ((int)self->lastwepfired & IT_DROPPABLE_WEAPONS)))
	{
		return; // nothing in it
	}

	item = spawn();

	VectorCopy(self->s.v.origin, item->s.v.origin);
	item->s.v.origin[2] -= 24;

	item->s.v.items = self->s.v.weapon;
	item->tp_flags = it_pack;

// drop best weapon in case of fairpacks 1 (KTEAMS)
	if (f1 == 1)
	{
		if (((int)self->s.v.items & IT_NAILGUN) && self->s.v.ammo_nails > 0)
		{
			item->s.v.items = IT_NAILGUN;
		}

		if (((int)self->s.v.items & IT_SUPER_SHOTGUN) && self->s.v.ammo_shells > 0)
		{
			item->s.v.items = IT_SUPER_SHOTGUN;
		}

		if (((int)self->s.v.items & IT_SUPER_NAILGUN) && self->s.v.ammo_nails > 0)
		{
			item->s.v.items = IT_SUPER_NAILGUN;
		}

		if (((int)self->s.v.items & IT_GRENADE_LAUNCHER) && self->s.v.ammo_rockets > 0)
		{
			item->s.v.items = IT_GRENADE_LAUNCHER;
		}

		if (((int)self->s.v.items & IT_LIGHTNING) && self->s.v.ammo_cells > 0)
		{
			item->s.v.items = IT_LIGHTNING;
		}

		if (((int)self->s.v.items & IT_ROCKET_LAUNCHER) && self->s.v.ammo_rockets > 0)
		{
			item->s.v.items = IT_ROCKET_LAUNCHER;
		}
	}

// drop lastfired even if no ammo in case of fairpacks 2 (KTEAMS)
	if (f1 == 2)
	{
		if ((int)self->lastwepfired & IT_DROPPABLE_WEAPONS)
		{
			item->s.v.items = self->lastwepfired;
		}
	}

	// Yawnmode: unfairpacks in DMM1, only drop current weapon if the player was shooting (idea from Tonik)
	// - Molgrum
	if (k_yawnmode && (deathmatch == 1) && (self->attack_finished < g_globalvars.time))
	{
		item->s.v.items = IT_SHOTGUN;
	}

	//item->mdl = "progs/backpack.mdl";
	setmodel(item, "progs/backpack.mdl");

	if ((item->s.v.items == IT_ROCKET_LAUNCHER) || (item->s.v.items == IT_LIGHTNING))
	{
		stuffcmd_flags(self, STUFFCMD_DEMOONLY, "//ktx drop %d %d %d\n", NUM_FOR_EDICT(item),
						(int)item->s.v.items, NUM_FOR_EDICT(self));
	}

	if (item->s.v.items == IT_AXE)
	{
		item->netname = "Axe";
		self->ps.wpn[wpAXE].drops++;
	}
	else if (item->s.v.items == IT_SHOTGUN)
	{
		item->netname = "Shotgun";
		self->ps.wpn[wpSG].drops++;
	}
	else if (item->s.v.items == IT_SUPER_SHOTGUN)
	{
		item->netname = "Double-barrelled Shotgun";
		//item->mdl = "progs/g_shot.mdl";
		self->ps.wpn[wpSSG].drops++;
	}
	else if (item->s.v.items == IT_NAILGUN)
	{
		item->netname = "Nailgun";
		//item->mdl = "progs/g_nail.mdl";
		self->ps.wpn[wpNG].drops++;
	}
	else if (item->s.v.items == IT_SUPER_NAILGUN)
	{
		item->netname = "Super Nailgun";
		//item->mdl = "progs/g_nail2.mdl";
		self->ps.wpn[wpSNG].drops++;
	}
	else if (item->s.v.items == IT_GRENADE_LAUNCHER)
	{
		item->netname = "Grenade Launcher";
		//item->mdl = "progs/g_rock.mdl";
		self->ps.wpn[wpGL].drops++;
	}
	else if (item->s.v.items == IT_ROCKET_LAUNCHER)
	{
		item->netname = "Rocket Launcher";
		//item->mdl = "progs/g_rock2.mdl";
		self->ps.wpn[wpRL].drops++;
	}
	else if (item->s.v.items == IT_LIGHTNING)
	{
		item->netname = "Thunderbolt";
		//item->mdl = "progs/g_light.mdl";
		self->ps.wpn[wpLG].drops++;
	}
	else
	{
		item->netname = "";
	}

	item->s.v.ammo_shells = self->s.v.ammo_shells;
	item->s.v.ammo_nails = self->s.v.ammo_nails;
	item->s.v.ammo_rockets = self->s.v.ammo_rockets;
	item->s.v.ammo_cells = self->s.v.ammo_cells;

	// Yawnmode: maximum backpack-capacity is 1/4 of player-capacity
	// - Molgrum
	if (k_yawnmode)
	{
		item->s.v.ammo_shells = min(25, item->s.v.ammo_shells);
		item->s.v.ammo_nails = min(50, item->s.v.ammo_nails);
		item->s.v.ammo_rockets = min(25, item->s.v.ammo_rockets);
		item->s.v.ammo_cells = min(25, item->s.v.ammo_cells);
	}

	if (fresh_packs)
	{
		item->s.v.ammo_shells = bound(0, item->s.v.ammo_shells, cvar("k_freshteams_pack_shells"));
		item->s.v.ammo_nails = bound(0, item->s.v.ammo_nails, cvar("k_freshteams_pack_nails"));
		item->s.v.ammo_rockets = bound(0, item->s.v.ammo_rockets, cvar("k_freshteams_pack_rockets"));
		item->s.v.ammo_cells = bound(0, item->s.v.ammo_cells, cvar("k_freshteams_pack_cells"));
	}

	playername = self->netname;

	log_printf("\t\t<event>\n"
				"\t\t\t<drop_backpack>\n"
				"\t\t\t\t<time>%f</time>\n"
				"\t\t\t\t<weapon>%s</weapon>\n"
				"\t\t\t\t<shells>%d</shells>\n"
				"\t\t\t\t<nails>%d</nails>\n"
				"\t\t\t\t<rockets>%d</rockets>\n"
				"\t\t\t\t<cells>%d</cells>\n"
				"\t\t\t\t<player>%s</player>\n"
				"\t\t\t</drop_backpack>\n"
				"\t\t</event>\n",
				g_globalvars.time - match_start_time, item->netname, (int)item->s.v.ammo_shells,
				(int)item->s.v.ammo_nails, (int)item->s.v.ammo_rockets,
				(int)item->s.v.ammo_cells, cleantext(playername));

	item->s.v.velocity[2] = 300;
	item->s.v.velocity[0] = -100 + (g_random() * 200);
	item->s.v.velocity[1] = -100 + (g_random() * 200);

	item->s.v.flags = FL_ITEM;
	item->s.v.solid = SOLID_TRIGGER;
	item->s.v.movetype = MOVETYPE_TOSS;
	//setmodel( item, k_yawnmode ? item->mdl : "progs/backpack.mdl" );
	setsize(item, -16, -16, 0, 16, 16, 56);
	item->touch = (func_t) BackpackTouch;

	// remove after 2 minutes, and after 30 seconds if backpack dropped by monster
	item->s.v.nextthink = g_globalvars.time + (self->ct == ctPlayer ? 120 : 30);
	item->think = (func_t) SUB_Remove;

	item->classname = "backpack"; // we do need to be able to get rid of these things between points (hoony mode)

	if (isTeam())
	{
		item->backpack_player_name = playername;
		strlcpy(item->backpack_team_name, getteam(self), MAX_TEAM_NAME);
	}

#ifdef BOT_SUPPORT
	BotsBackpackDropped(self, item);
#endif
}

/*
 ===============================================================================

 SPAWN POINT MARKERS

 ===============================================================================
 */

gedict_t* Spawn_OnePoint(gedict_t *spawn_point, vec3_t org, int effects)
{
	gedict_t *p;

	p = spawn();
	p->s.v.flags = FL_ITEM;
	p->s.v.solid = SOLID_NOT;
	p->s.v.movetype = MOVETYPE_NONE;
	setmodel(p, cvar("k_spm_custom_model") ? "progs/wizard.mdl" : "progs/w_g_key.mdl");
	p->netname = "Spawn Point";
	p->classname = "spawnpoint";
	p->k_lastspawn = spawn_point;

	p->s.v.effects = (int)p->s.v.effects | effects;

	// store references for changing selections in hoonymode
	spawn_point->wizard = p;
	p->wizard = spawn_point;

	setorigin(p, PASSVEC3(org));

	VectorCopy(spawn_point->s.v.angles, p->s.v.angles);
	trap_makevectors(p->s.v.angles);

	return p;
}

void Spawn_SpawnPoints(char *classname, int effects)
{
	gedict_t *e;
	vec3_t org;

	for (e = world; (e = ez_find(e, classname));)
	{
		VectorCopy(e->s.v.origin, org);
		org[2] += 0; // qqshka: it was 16, but I like more how it looks when it more close to ground

		if (isHoonyModeDuel())
		{
			effects = (e->hoony_nomination ? (EF_GREEN | EF_RED) : 0);
		}

		Spawn_OnePoint(e, org, effects);
	}
}

void ShowSpawnPoints(void)
{
	Spawn_SpawnPoints("info_player_deathmatch", cvar("k_spm_glow") ? ( EF_GREEN | EF_RED) : 0);

	if (isCTF())
	{
		Spawn_SpawnPoints("info_player_team1", cvar("k_spm_glow") ? EF_RED : 0);
		Spawn_SpawnPoints("info_player_team2", cvar("k_spm_glow") ? EF_BLUE : 0);
	}
}

void HideSpawnPoints(void)
{
	gedict_t *e;

	for (e = world; (e = ez_find(e, "spawnpoint"));)
	{
		if (e->wizard)
		{
			e->wizard->wizard = 0;
		}

		ent_remove(e);
	}
}

