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
 *  $Id: items.c,v 1.14 2006/04/09 22:36:38 qqshka Exp $
 */

#include "g_local.h"

void            SP_item_artifact_invisibility();
void            SP_item_artifact_super_damage();


void SUB_regen()
{
	self->s.v.model = self->mdl;	// restore original model
	self->s.v.solid = SOLID_TRIGGER;	// allow it to be touched again
	sound( self, CHAN_VOICE, "items/itembk2.wav", 1, ATTN_NORM );	// play respawn sound
	setorigin( self, PASSVEC3( self->s.v.origin ) );
}

void DropPowerup( float timeleft, int powerup )
{
	gedict_t       *swp = self; // save self

	if ( timeleft <= 0 )
		return;
	
	if ( powerup != IT_QUAD && powerup != IT_INVISIBILITY ) // only this supported
		return;

	self = spawn(); // WARNING!

	setorigin (self, PASSVEC3( swp->s.v.origin ));
	self->cnt = g_globalvars.time + timeleft;

	if (powerup == IT_QUAD)
		SP_item_artifact_super_damage();
	else if (powerup == IT_INVISIBILITY)
		SP_item_artifact_invisibility();
	else
		G_Error("DropPowerup");

	G_bprint( PRINT_HIGH, "%s lost a %s with %.0f seconds remaining\n",
					  	  swp->s.v.netname, self->s.v.netname, timeleft );

	self = swp;// restore self
}

void PlaceItem()
{
	self->s.v.solid = SOLID_TRIGGER;
	self->s.v.movetype = MOVETYPE_TOSS;
	self->s.v.flags = FL_ITEM;
	self->mdl = strnull( self->s.v.model ) ? self->mdl : self->s.v.model; // save .mdl if .model is not init
	self->mdl = strnull( self->mdl ) ? "" : self->mdl; // init .mdl with empty string if not set

	SetVector( self->s.v.velocity, 0, 0, 0 );
	self->s.v.origin[2] += 6;

	if ( !droptofloor( self ) )
	{
		G_Printf( "Bonus item fell out of level at  '%f %f %f'\n",
			  self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] );
		ent_remove( self );
	}

	// if powerups disabled - hide
	if ( (int)self->s.v.items & (IT_INVISIBILITY | IT_INVULNERABILITY | IT_SUIT | IT_QUAD) ) {
		if ( !Get_Powerups() ) {
			self->s.v.model = "";
			self->s.v.solid = SOLID_NOT;
		}
	}
}

void PlaceItemIngame()
{
	self->s.v.solid = SOLID_TRIGGER;
	self->s.v.movetype = MOVETYPE_TOSS;
	self->s.v.flags = FL_ITEM;
	self->mdl = self->s.v.model;

	SetVector( self->s.v.velocity, 0, 0, 0 );
}

/*
============
StartItem

Sets the clipping size and plants the object on the floor
============
*/
void StartItem()
{
//	G_bprint(2, "StartItem: %s\n", self->s.v.classname);

	self->mdl = self->s.v.model; // qqshka - save model ASAP

	self->s.v.nextthink = g_globalvars.time + 0.2;	// items start after other solids
	self->s.v.think = ( func_t ) PlaceItem;
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
float T_Heal( gedict_t * e, float healamount, float ignore )
{
	if ( ISDEAD( e ) )
		return 0;

	if ( ( !ignore ) && ( e->s.v.health >= other->s.v.max_health ) )
		return 0;

	healamount = ceil( healamount );

	e->s.v.health = e->s.v.health + healamount;
	if ( ( !ignore ) && ( e->s.v.health >= other->s.v.max_health ) )
		e->s.v.health = other->s.v.max_health;

	if ( e->s.v.health > 250 )
		e->s.v.health = 250;
	return 1;
}

void            health_touch();
void            item_megahealth_rot();

/*QUAKED item_health (.3 .3 1) (0 0 0) (32 32 32) rotten megahealth
Health box. Normally gives 25 points.
Rotten box heals 5-10 points,
megahealth will add 100 health, then 
rot you down to your maximum health limit, 
one point per second.
*/

void SP_item_health()
{

	self->s.v.touch = ( func_t ) health_touch;
	if ( ( int ) self->s.v.spawnflags & H_ROTTEN )
	{
		trap_precache_model( "maps/b_bh10.bsp" );
		trap_precache_sound( "items/r_item1.wav" );
		setmodel( self, "maps/b_bh10.bsp" );
		self->s.v.noise = "items/r_item1.wav";
		self->healamount = 15;
		self->healtype = 0;
	} else
	{
		if ( ( int ) self->s.v.spawnflags & H_MEGA )
		{
			trap_precache_model( "maps/b_bh100.bsp" );
			trap_precache_sound( "items/r_item2.wav" );
			setmodel( self, "maps/b_bh100.bsp" );
			self->s.v.noise = "items/r_item2.wav";
			self->healamount = 100;
			self->healtype = 2;
		} else
		{
			trap_precache_model( "maps/b_bh25.bsp" );
			trap_precache_sound( "items/health1.wav" );
			setmodel( self, "maps/b_bh25.bsp" );
			self->s.v.noise = "items/health1.wav";
			self->healamount = 25;
			self->healtype = 1;
		}
	}

	setsize( self, 0, 0, 0, 32, 32, 56 );
	StartItem( self );
}

void health_touch()
{
	if ( deathmatch == 4 )
		if ( other->invincible_time > 0 )
			return;

	if ( strneq( other->s.v.classname, "player" ) )
		return;

#ifdef KTEAMS
	if (match_in_progress != 2)
        return;
#endif

	if ( self->healtype == 2 )	// Megahealth?  Ignore max_health...
	{
		if ( other->s.v.health >= 250 )
			return;
		if ( !T_Heal( other, self->healamount, 1 ) )
			return;

		other->ps.mh++;
	} else
	{
		if ( !T_Heal( other, self->healamount, 0 ) )
			return;
	}

	G_sprint( other, PRINT_LOW, "You receive %.0f health\n", self->healamount );

// health touch sound
	sound( other, CHAN_ITEM, self->s.v.noise, 1, ATTN_NORM );

	stuffcmd( other, "bf\n" );

	self->s.v.model = "";
	self->s.v.solid = SOLID_NOT;

	// Megahealth = rot down the player's super health
	if ( self->healtype == 2 )
	{
		other->s.v.items = ( int ) other->s.v.items | IT_SUPERHEALTH;
		if ( deathmatch != 4 )
		{
			self->s.v.nextthink = g_globalvars.time + 5;
			self->s.v.think = ( func_t ) item_megahealth_rot;
		}
		self->s.v.owner = EDICT_TO_PROG( other );
	} else
	{
		if ( deathmatch != 2 )	// deathmatch 2 is the silly old rules
		{
			self->s.v.nextthink = g_globalvars.time + 20;
			self->s.v.think = ( func_t ) SUB_regen;
		}
	}

	activator = other;
	SUB_UseTargets();	// fire all targets / killtargets
}

void item_megahealth_rot()
{
	other = PROG_TO_EDICT( self->s.v.owner );

	if ( other->s.v.health > other->s.v.max_health )
	{
		if ( !other->ctf_flag & CTF_RUNE_RGN )
			other->s.v.health -= 1;

		self->s.v.nextthink = g_globalvars.time + 1;
		return;
	}

// it is possible for a player to die and respawn between rots, so don't
// just blindly subtract the flag off
	other->s.v.items -= ( int ) other->s.v.items & IT_SUPERHEALTH;

	if ( deathmatch != 2 )	// deathmatch 2 is silly old rules
	{
		self->s.v.nextthink = g_globalvars.time + 20;
		self->s.v.think = ( func_t ) SUB_regen;
	}
}

/*
===============================================================================

ARMOR

===============================================================================
*/
void armor_touch()
{
	float           type = 0, value = 0;
	int             bit = 0;
	int				*armor = NULL;

	if ( ISDEAD( other ) )
		return;

	if ( strneq( other->s.v.classname, "player" ) )
		return;

#ifdef KTEAMS
	if ( match_in_progress != 2 )
        return;
#endif

	if ( deathmatch == 4 )
		if ( other->invincible_time > 0 )
			return;

	if ( !strcmp( self->s.v.classname, "item_armor1" ) )
	{
		armor = &(other->ps.ga);
		type = 0.3;
		value = 100;
		bit = IT_ARMOR1;
	}
	if ( !strcmp( self->s.v.classname, "item_armor2" ) )
	{
		armor = &(other->ps.ya);
		type = 0.6;
		value = 150;
		bit = IT_ARMOR2;
	}
	if ( !strcmp( self->s.v.classname, "item_armorInv" ) )
	{
		armor = &(other->ps.ra);
		type = 0.8;
		value = 200;
		bit = IT_ARMOR3;
	}
	if ( other->s.v.armortype * other->s.v.armorvalue >= type * value )
		return;

	if ( armor )
		(*armor)++;

	other->s.v.armortype = type;
	other->s.v.armorvalue = value;
	other->s.v.items +=
		-( ( int ) other->s.v.items & ( IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3 ) ) + bit;

	self->s.v.solid = SOLID_NOT;
	self->s.v.model = "";
	if ( deathmatch != 2 )
		self->s.v.nextthink = g_globalvars.time + 20;
	self->s.v.think = ( func_t ) SUB_regen;


	G_sprint( other, PRINT_LOW, "You got armor\n" );
// armor touch sound
	sound( other, CHAN_ITEM, "items/armor1.wav", 1, ATTN_NORM );
	stuffcmd( other, "bf\n" );

	activator = other;
	SUB_UseTargets();	// fire all targets / killtargets
}

/*QUAKED item_armor1 (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void SP_item_armor1()
{
	self->s.v.touch = ( func_t ) armor_touch;
	trap_precache_model( "progs/armor.mdl" );
	setmodel( self, "progs/armor.mdl" );
	self->s.v.skin = 0;
	setsize( self, -16, -16, 0, 16, 16, 56 );
	StartItem( self );
}

/*QUAKED item_armor2 (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void SP_item_armor2()
{
	self->s.v.touch = ( func_t ) armor_touch;
	trap_precache_model( "progs/armor.mdl" );
	setmodel( self, "progs/armor.mdl" );
	self->s.v.skin = 1;
	setsize( self, -16, -16, 0, 16, 16, 56 );
	StartItem( self );
}

/*QUAKED item_armorInv (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void SP_item_armorInv()
{
	self->s.v.touch = ( func_t ) armor_touch;
	trap_precache_model( "progs/armor.mdl" );
	setmodel( self, "progs/armor.mdl" );
	self->s.v.skin = 2;
	setsize( self, -16, -16, 0, 16, 16, 56 );
	StartItem( self );
}

/*
===============================================================================

WEAPONS

===============================================================================
*/

void bound_other_ammo()
{
	if ( other->s.v.ammo_shells > 100 )
		other->s.v.ammo_shells = 100;
	if ( other->s.v.ammo_nails > 200 )
		other->s.v.ammo_nails = 200;
	if ( other->s.v.ammo_rockets > 100 )
		other->s.v.ammo_rockets = 100;
	if ( other->s.v.ammo_cells > 100 )
		other->s.v.ammo_cells = 100;
}
float RankForWeapon( float w )
{
	if ( w == IT_LIGHTNING )
		return 1;
	if ( w == IT_ROCKET_LAUNCHER )
		return 2;
	if ( w == IT_SUPER_NAILGUN )
		return 3;
	if ( w == IT_GRENADE_LAUNCHER )
		return 4;
	if ( w == IT_SUPER_SHOTGUN )
		return 5;
	if ( w == IT_NAILGUN )
		return 6;
	return 7;
}

float WeaponCode( float w )
{
	if ( w == IT_SHOTGUN )
		return 2;
	if ( w == IT_SUPER_SHOTGUN )
		return 3;
	if ( w == IT_NAILGUN )
		return 4;
	if ( w == IT_SUPER_NAILGUN )
		return 5;
	if ( w == IT_GRENADE_LAUNCHER )
		return 6;
	if ( w == IT_ROCKET_LAUNCHER )
		return 7;
	if ( w == IT_LIGHTNING )
		return 8;
	return 1;
}

/*
=============
Deathmatch_Weapon

Deathmatch weapon change rules for picking up a weapon

.float          ammo_shells, ammo_nails, ammo_rockets, ammo_cells;
=============
*/
void Deathmatch_Weapon( int new )
{
	int           or, nr;

	if ( self->s.v.weapon == IT_HOOK && self->s.v.button0 )
		return;

// change self.weapon if desired
	or = RankForWeapon( self->s.v.weapon );
	nr = RankForWeapon( new );
	if ( nr < or )
		self->s.v.weapon = new;
}

void DoWeaponChange( int new )
{
	int w_switch = iKey( self, "w_switch" );

	if ( !w_switch )
		w_switch = 8;

	if ( WeaponCode( new ) <= w_switch )
	{
		if ( ( ( int ) ( self->s.v.flags ) ) & FL_INWATER )
		{
			if ( new != IT_LIGHTNING )
				Deathmatch_Weapon( new );
		} else
		{
			Deathmatch_Weapon( new );
		}
	}

	W_SetCurrentAmmo();
}

/*
=============
weapon_touch
=============
*/
float           W_BestWeapon();
void weapon_touch()
{
	int             hadammo, new = 0;
	gedict_t       *stemp;
	int             leave;

	if ( !( ( int ) other->s.v.flags & FL_CLIENT ) )
		return;

	if ( match_in_progress != 2 )
        return;
	
	if ( deathmatch == 2 || deathmatch == 3 || deathmatch == 5 )
		leave = 1;
	else
		leave = 0;

	if ( !strcmp( self->s.v.classname, "weapon_nailgun" ) )
	{
		if ( leave && ( ( int ) other->s.v.items & IT_NAILGUN ) )
			return;
		hadammo = other->s.v.ammo_nails;
		new = IT_NAILGUN;
		other->s.v.ammo_nails += 30;

	} else if ( !strcmp( self->s.v.classname, "weapon_supernailgun" ) )
	{
		if ( leave && ( ( int ) other->s.v.items & IT_SUPER_NAILGUN ) )
			return;

		hadammo = other->s.v.ammo_rockets;
		new = IT_SUPER_NAILGUN;
		other->s.v.ammo_nails += 30;

	} else if ( !strcmp( self->s.v.classname, "weapon_supershotgun" ) )
	{
		if ( leave && ( ( int ) other->s.v.items & IT_SUPER_SHOTGUN ) )
			return;

		hadammo = other->s.v.ammo_rockets;
		new = IT_SUPER_SHOTGUN;
		other->s.v.ammo_shells += 5;
	
	} else if ( !strcmp( self->s.v.classname, "weapon_rocketlauncher" ) )
	{
		if ( leave && ( ( int ) other->s.v.items & IT_ROCKET_LAUNCHER ) )
			return;

		hadammo = other->s.v.ammo_rockets;
		new = IT_ROCKET_LAUNCHER;
		other->s.v.ammo_rockets += 5;

	} else if ( !strcmp( self->s.v.classname, "weapon_grenadelauncher" ) )
	{
		if ( leave && ( ( int ) other->s.v.items & IT_GRENADE_LAUNCHER ) )
			return;

		hadammo = other->s.v.ammo_rockets;
		new = IT_GRENADE_LAUNCHER;
		other->s.v.ammo_rockets += 5;

	} else if ( !strcmp( self->s.v.classname, "weapon_lightning" ) )
	{
		if ( leave && ( ( int ) other->s.v.items & IT_LIGHTNING ) )
			return;

		hadammo = other->s.v.ammo_rockets;
		new = IT_LIGHTNING;
		other->s.v.ammo_cells += 15;

	} else
		G_Error( "weapon_touch: unknown classname" );

	G_sprint( other, PRINT_LOW, "You got the %s\n", self->s.v.netname );
// weapon touch sound
	sound( other, CHAN_ITEM, "weapons/pkup.wav", 1, ATTN_NORM );
	stuffcmd( other, "bf\n" );

	bound_other_ammo();

// change to the weapon
	other->s.v.items = ( int ) other->s.v.items | new;

	stemp = self;
	self = other;

	DoWeaponChange( new ); // change to the weapon

	self = stemp;

	if ( leave )
		return;

	if ( deathmatch != 3 || deathmatch != 5 )
	{
		// remove it in single player, or setup for respawning in deathmatch
		self->s.v.model = "";
		self->s.v.solid = SOLID_NOT;
		if ( deathmatch != 2 )
			self->s.v.nextthink = g_globalvars.time + 30;
		self->s.v.think = ( func_t ) SUB_regen;
	}
	activator = other;
	SUB_UseTargets();	// fire all targets / killtargets
}

/*QUAKED weapon_supershotgun (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void SP_weapon_supershotgun()
{
#ifndef KTEAMS
	if (deathmatch > 3)
		return;
#endif

	trap_precache_model( "progs/g_shot.mdl" );
	setmodel( self, "progs/g_shot.mdl" );

	self->s.v.weapon = IT_SUPER_SHOTGUN;
	self->s.v.netname = "Double-barrelled Shotgun";
	self->s.v.touch = ( func_t ) weapon_touch;

	setsize( self, -16, -16, 0, 16, 16, 56 );

	StartItem();
}

/*QUAKED weapon_nailgun (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void SP_weapon_nailgun()
{
#ifndef KTEAMS
	if (deathmatch > 3)
		return;
#endif

	trap_precache_model( "progs/g_nail.mdl" );
	setmodel( self, "progs/g_nail.mdl" );

	self->s.v.weapon = IT_NAILGUN;
	self->s.v.netname = "nailgun";
	self->s.v.touch = ( func_t ) weapon_touch;

	setsize( self, -16, -16, 0, 16, 16, 56 );
	
	StartItem();
}

/*QUAKED weapon_supernailgun (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void SP_weapon_supernailgun()
{
#ifndef KTEAMS
	if (deathmatch > 3)
		return;
#endif

	trap_precache_model( "progs/g_nail2.mdl" );
	setmodel( self, "progs/g_nail2.mdl" );

	self->s.v.weapon = IT_SUPER_NAILGUN;
	self->s.v.netname = "Super Nailgun";
	self->s.v.touch = ( func_t ) weapon_touch;

	setsize( self, -16, -16, 0, 16, 16, 56 );

	StartItem();

}

/*QUAKED weapon_grenadelauncher (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void SP_weapon_grenadelauncher()
{
#ifndef KTEAMS
	if (deathmatch > 3)
		return;
#endif

	trap_precache_model( "progs/g_rock.mdl" );
	setmodel( self, "progs/g_rock.mdl" );

	self->s.v.weapon = 3;
	self->s.v.netname = "Grenade Launcher";
	self->s.v.touch = ( func_t ) weapon_touch;

	setsize( self, -16, -16, 0, 16, 16, 56 );

	StartItem();
}

/*QUAKED weapon_rocketlauncher (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void SP_weapon_rocketlauncher()
{
#ifndef KTEAMS
	if (deathmatch > 3)
		return;
#endif

	trap_precache_model( "progs/g_rock2.mdl" );
	setmodel( self, "progs/g_rock2.mdl" );

	self->s.v.weapon = 3;
	self->s.v.netname = "Rocket Launcher";
	self->s.v.touch = ( func_t ) weapon_touch;

	setsize( self, -16, -16, 0, 16, 16, 56 );

	StartItem();
}


/*QUAKED weapon_lightning (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void SP_weapon_lightning()
{
#ifndef KTEAMS
	if (deathmatch > 3)
		return;
#endif

	trap_precache_model( "progs/g_light.mdl" );
	setmodel( self, "progs/g_light.mdl" );

	self->s.v.weapon = 3;
	self->s.v.netname = "Thunderbolt";
	self->s.v.touch = ( func_t ) weapon_touch;

	setsize( self, -16, -16, 0, 16, 16, 56 );

	StartItem();
}


/*
===============================================================================

AMMO

===============================================================================
*/

void ammo_touch()
{
	int ammo, weapon, best;
	gedict_t       *stemp;

	if ( ISDEAD( other ) )
		return;

	if ( strneq( other->s.v.classname, "player" ) )
		return;

    if ( match_in_progress != 2 )
        return;

// if the player was using his best weapon, change up to the new one if better          
	stemp = self;
	self = other;
	best = W_BestWeapon(); // save best weapon before update ammo
	self = stemp;

	ammo = self->aflag;
	weapon = self->s.v.weapon;

// shotgun
	if ( weapon == 1 )
	{
		if ( other->s.v.ammo_shells >= 100 )
			return;
		other->s.v.ammo_shells += ammo;
	}
// spikes
	if ( weapon == 2 )
	{
		if ( other->s.v.ammo_nails >= 200 )
			return;
		other->s.v.ammo_nails += ammo;
	}
// rockets
	if ( weapon == 3 )
	{
		if ( other->s.v.ammo_rockets >= 100 )
			return;
		other->s.v.ammo_rockets += ammo;
	}
// cells
	if ( weapon == 4 )
	{
		if ( other->s.v.ammo_cells >= 100 )
			return;
		other->s.v.ammo_cells += ammo;
	}

	bound_other_ammo();

	G_sprint( other, PRINT_LOW, "You got the %s\n", self->s.v.netname );
// ammo touch sound
	sound( other, CHAN_ITEM, "weapons/lock4.wav", 1, ATTN_NORM );
	stuffcmd( other, "bf\n" );

// change to a better weapon if appropriate
// before we got ammo we use best weapon - best weapon may change due to ammo, so check this
	if ( other->s.v.weapon == best )
	{
		stemp = self;
		self = other;

		DoWeaponChange( W_BestWeapon() ); // change to the weapon

		self = stemp;
	}

// if changed current ammo, update it
	stemp = self;
	self = other;

	W_SetCurrentAmmo();

	self = stemp;

// remove it in single player, or setup for respawning in deathmatch
	self->s.v.model = "";
	self->s.v.solid = SOLID_NOT;
	if ( deathmatch != 2 )
		self->s.v.nextthink = g_globalvars.time + 30;

// Xian -- If playing in DM 3.0 mode, halve the time ammo respawns        

	if ( deathmatch == 3 || deathmatch == 5 )
		self->s.v.nextthink = g_globalvars.time + 15;

	self->s.v.think = ( func_t ) SUB_regen;

	activator = other;
	SUB_UseTargets();	// fire all targets / killtargets
}




#define  WEAPON_BIG2  1

/*QUAKED item_shells (0 .5 .8) (0 0 0) (32 32 32) big
*/

void SP_item_shells()
{
#ifndef KTEAMS
	if ( deathmatch == 4 )
		return;
#endif

	self->s.v.touch = ( func_t ) ammo_touch;

	if ( ( int ) ( self->s.v.spawnflags ) & WEAPON_BIG2 )
	{
		trap_precache_model( "maps/b_shell1.bsp" );
		setmodel( self, "maps/b_shell1.bsp" );
		self->aflag = 40;
	} else
	{
		trap_precache_model( "maps/b_shell0.bsp" );
		setmodel( self, "maps/b_shell0.bsp" );
		self->aflag = 20;
	}

	self->s.v.weapon = 1;
	self->s.v.netname = "shells";

	setsize( self, 0, 0, 0, 32, 32, 56 );
	StartItem();
}

/*QUAKED item_spikes (0 .5 .8) (0 0 0) (32 32 32) big
*/

void SP_item_spikes()
{
#ifndef KTEAMS
	if ( deathmatch == 4 )
		return;
#endif

	self->s.v.touch = ( func_t ) ammo_touch;

	if ( ( int ) ( self->s.v.spawnflags ) & WEAPON_BIG2 )
	{
		trap_precache_model( "maps/b_nail1.bsp" );
		setmodel( self, "maps/b_nail1.bsp" );
		self->aflag = 50;
	} else
	{
		trap_precache_model( "maps/b_nail0.bsp" );
		setmodel( self, "maps/b_nail0.bsp" );
		self->aflag = 25;
	}
	self->s.v.weapon = 2;
	self->s.v.netname = "nails";

	setsize( self, 0, 0, 0, 32, 32, 56 );

	StartItem();

}

/*QUAKED item_rockets (0 .5 .8) (0 0 0) (32 32 32) big
*/

void SP_item_rockets()
{
#ifndef KTEAMS
	if ( deathmatch == 4 )
		return;
#endif

	self->s.v.touch = ( func_t ) ammo_touch;

	if ( ( int ) ( self->s.v.spawnflags ) & WEAPON_BIG2 )
	{
		trap_precache_model( "maps/b_rock1.bsp" );
		setmodel( self, "maps/b_rock1.bsp" );
		self->aflag = 10;
	} else
	{
		trap_precache_model( "maps/b_rock0.bsp" );
		setmodel( self, "maps/b_rock0.bsp" );
		self->aflag = 5;
	}
	self->s.v.weapon = 3;
	self->s.v.netname = "rockets";

	setsize( self, 0, 0, 0, 32, 32, 56 );

	StartItem();

}


/*QUAKED item_cells (0 .5 .8) (0 0 0) (32 32 32) big
*/

void SP_item_cells()
{
#ifndef KTEAMS
	if ( deathmatch == 4 )
		return;
#endif

	self->s.v.touch = ( func_t ) ammo_touch;

	if ( ( int ) ( self->s.v.spawnflags ) & WEAPON_BIG2 )
	{
		trap_precache_model( "maps/b_batt1.bsp" );
		setmodel( self, "maps/b_batt1.bsp" );
		self->aflag = 12;
	} else
	{
		trap_precache_model( "maps/b_batt0.bsp" );
		setmodel( self, "maps/b_batt0.bsp" );
		self->aflag = 6;
	}

	self->s.v.weapon = 4;
	self->s.v.netname = "cells";

	setsize( self, 0, 0, 0, 32, 32, 56 );

	StartItem();

}


/*QUAKED item_weapon (0 .5 .8) (0 0 0) (32 32 32) shotgun rocket spikes big
DO NOT USE THIS!!!! IT WILL BE REMOVED!
*/

#define WEAPON_SHOTGUN  1
#define WEAPON_ROCKET  2
#define WEAPON_SPIKES  4
#define WEAPON_BIG  8
void SP_item_weapon()
{
#ifndef KTEAMS
	if (deathmatch == 4)
		return;
#endif

	self->s.v.touch = ( func_t ) ammo_touch;

	if ( ( int ) ( self->s.v.spawnflags ) & WEAPON_SHOTGUN )
	{
		if ( ( int ) ( self->s.v.spawnflags ) & WEAPON_BIG )
		{
			trap_precache_model( "maps/b_shell1.bsp" );
			setmodel( self, "maps/b_shell1.bsp" );
			self->aflag = 40;
		} else
		{
			trap_precache_model( "maps/b_shell0.bsp" );
			setmodel( self, "maps/b_shell0.bsp" );
			self->aflag = 20;
		}
		self->s.v.weapon = 1;
		self->s.v.netname = "shells";
	}

	if ( ( int ) ( self->s.v.spawnflags ) & WEAPON_SPIKES )
	{
		if ( ( int ) ( self->s.v.spawnflags ) & WEAPON_BIG )
		{
			trap_precache_model( "maps/b_nail1.bsp" );
			setmodel( self, "maps/b_nail1.bsp" );
			self->aflag = 40;
		} else
		{
			trap_precache_model( "maps/b_nail0.bsp" );
			setmodel( self, "maps/b_nail0.bsp" );
			self->aflag = 20;
		}
		self->s.v.weapon = 2;
		self->s.v.netname = "spikes";
	}

	if ( ( int ) ( self->s.v.spawnflags ) & WEAPON_ROCKET )
	{
		if ( ( int ) ( self->s.v.spawnflags ) & WEAPON_BIG )
		{
			trap_precache_model( "maps/b_rock1.bsp" );
			setmodel( self, "maps/b_rock1.bsp" );
			self->aflag = 10;
		} else
		{
			trap_precache_model( "maps/b_rock0.bsp" );
			setmodel( self, "maps/b_rock0.bsp" );
			self->aflag = 5;
		}
		self->s.v.weapon = 3;
		self->s.v.netname = "rockets";
	}

	setsize( self, 0, 0, 0, 32, 32, 56 );

	StartItem();
}

/*
===============================================================================

KEYS

===============================================================================
*/
void key_touch()
{
//gedict_t*    stemp;
//float             best;

	if ( strneq( other->s.v.classname, "player" ) )
		return;

	if ( ISDEAD( other ) )
		return;

	if ( ( int ) other->s.v.items & ( int ) self->s.v.items )
		return;

#ifdef KTEAMS
	if (match_in_progress != 2)
        return;
#endif

	G_sprint( other, PRINT_LOW, "You got the %s\n", self->s.v.netname );

	sound( other, CHAN_ITEM, self->s.v.noise, 1, ATTN_NORM );
	stuffcmd( other, "bf\n" );
	other->s.v.items = ( int ) other->s.v.items | ( int ) self->s.v.items;

	self->s.v.solid = SOLID_NOT;
	self->s.v.model = "";

	activator = other;
	SUB_UseTargets();	// fire all targets / killtargets
}


void key_setsounds()
{
	if ( world->worldtype == 0 )
	{
		trap_precache_sound( "misc/medkey.wav" );
		self->s.v.noise = "misc/medkey.wav";
	}
	if ( world->worldtype == 1 )
	{
		trap_precache_sound( "misc/runekey.wav" );
		self->s.v.noise = "misc/runekey.wav";
	}
	if ( world->worldtype == 2 )
	{
		trap_precache_sound( "misc/basekey.wav" );
		self->s.v.noise = "misc/basekey.wav";
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

void SP_item_key1()
{
	if ( world->worldtype == 0 )
	{
		trap_precache_model( "progs/w_s_key.mdl" );
		setmodel( self, "progs/w_s_key.mdl" );
		self->s.v.netname = "silver key";
	} else if ( world->worldtype == 1 )
	{
		trap_precache_model( "progs/m_s_key.mdl" );
		setmodel( self, "progs/m_s_key.mdl" );
		self->s.v.netname = "silver runekey";
	} else if ( world->worldtype == 2 )
	{
		trap_precache_model( "progs/b_s_key.mdl" );
		setmodel( self, "progs/b_s_key.mdl" );
		self->s.v.netname = "silver keycard";
	}
	key_setsounds();
	self->s.v.touch = ( func_t ) key_touch;
	self->s.v.items = IT_KEY1;
	setsize( self, -16, -16, -24, 16, 16, 32 );
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

void SP_item_key2()
{
	if ( world->worldtype == 0 )
	{
		trap_precache_model( "progs/w_g_key.mdl" );
		setmodel( self, "progs/w_g_key.mdl" );
		self->s.v.netname = "gold key";
	}
	if ( world->worldtype == 1 )
	{
		trap_precache_model( "progs/m_g_key.mdl" );
		setmodel( self, "progs/m_g_key.mdl" );
		self->s.v.netname = "gold runekey";
	}
	if ( world->worldtype == 2 )
	{
		trap_precache_model( "progs/b_g_key.mdl" );
		setmodel( self, "progs/b_g_key.mdl" );
		self->s.v.netname = "gold keycard";
	}
	key_setsounds();
	self->s.v.touch = ( func_t ) key_touch;
	self->s.v.items = IT_KEY2;
	setsize( self, -16, -16, -24, 16, 16, 32 );
	StartItem();
}



/*
===============================================================================

END OF LEVEL RUNES

===============================================================================
*/
void sigil_touch()
{
//gedict_t*    stemp;
//float             best;

	if ( strneq( other->s.v.classname, "player" ) )
		return;

	if ( ISDEAD( other ) )
		return;

#ifdef KTEAMS
    if (match_in_progress != 2)
        return;
#endif

	G_centerprint( other, "You got the rune!" );

	sound( other, CHAN_ITEM, self->s.v.noise, 1, ATTN_NORM );
	stuffcmd( other, "bf\n" );
	self->s.v.solid = SOLID_NOT;
	self->s.v.model = "";
	g_globalvars.serverflags =
	    ( int ) ( g_globalvars.serverflags ) | ( ( int ) ( self->s.v.spawnflags ) & 15 );
	self->s.v.classname = "";	// so rune doors won't find it

	activator = other;
	SUB_UseTargets();	// fire all targets / killtargets
}


/*QUAKED item_sigil (0 .5 .8) (-16 -16 -24) (16 16 32) E1 E2 E3 E4
End of level sigil, pick up to end episode and return to jrstart.
*/

void SP_item_sigil()
{
	if ( !( int ) ( self->s.v.spawnflags ) )
		G_Error( "item_sigil no spawnflags" );

	trap_precache_sound( "misc/runekey.wav" );
	self->s.v.noise = "misc/runekey.wav";

	if ( ( int ) ( self->s.v.spawnflags ) & 1 )
	{
		trap_precache_model( "progs/end1.mdl" );
		setmodel( self, "progs/end1.mdl" );
	}
	if ( ( int ) ( self->s.v.spawnflags ) & 2 )
	{
		trap_precache_model( "progs/end2.mdl" );
		setmodel( self, "progs/end2.mdl" );
	}
	if ( ( int ) ( self->s.v.spawnflags ) & 4 )
	{
		trap_precache_model( "progs/end3.mdl" );
		setmodel( self, "progs/end3.mdl" );
	}
	if ( ( int ) ( self->s.v.spawnflags ) & 8 )
	{
		trap_precache_model( "progs/end4.mdl" );
		setmodel( self, "progs/end4.mdl" );
	}

	self->s.v.touch = ( func_t ) sigil_touch;
	setsize( self, -16, -16, -24, 16, 16, 32 );
	StartItem();
}

/*
===============================================================================

POWERUPS

===============================================================================
*/

void            powerup_touch();


void powerup_touch()
{
	float *p_cnt = NULL;

	if ( strnull ( self->s.v.classname ) )
		G_Error("powerup_touch: null classname");

	if ( strneq( other->s.v.classname, "player" ) )
		return;

	if ( ISDEAD( other ) )
		return;

	if ( !k_practice ) // #practice mode#
	if ( match_in_progress != 2 )
		return;

    if ( !Get_Powerups() )
        return;

	G_sprint( other, PRINT_LOW, "You got the %s\n", self->s.v.netname );

	self->mdl = self->s.v.model;

	if ( streq( self->s.v.classname, "item_artifact_invulnerability" ) ||
	     streq( self->s.v.classname, "item_artifact_invisibility" ) )
		self->s.v.nextthink = g_globalvars.time + 60 * 5;
	else
		self->s.v.nextthink = g_globalvars.time + 60;

	// all powerups respawn after 30 seconds in practice mode
	if ( k_practice ) // #practice mode#
		self->s.v.nextthink = g_globalvars.time + 30;

	self->s.v.think = ( func_t ) SUB_regen;

// like ktpro
//	sound( other, CHAN_VOICE, self->s.v.noise, 1, ATTN_NORM );
	sound( other, CHAN_ITEM, self->s.v.noise, 1, ATTN_NORM );
	stuffcmd( other, "bf\n" );
	self->s.v.solid = SOLID_NOT;
	other->s.v.items = ( ( int ) other->s.v.items ) | ( ( int ) self->s.v.items );
	self->s.v.model = "";

// do the apropriate action
	if ( streq( self->s.v.classname, "item_artifact_envirosuit" ) )
	{
		other->rad_time = 1;
		other->radsuit_finished = g_globalvars.time + 30;
	}

	if ( streq( self->s.v.classname, "item_artifact_invulnerability" ) )
	{
		other->ps.pent++;
		other->invincible_time = 1;
		other->invincible_finished = g_globalvars.time + 30;
	}

	if ( streq( self->s.v.classname, "item_artifact_invisibility" ) )
	{
		other->ps.ring++;
		other->invisible_time = 1;
		other->invisible_finished = g_globalvars.time + 30;

		if ( self->cnt > g_globalvars.time ) // is this was a dropped powerup
			p_cnt = &(other->invisible_finished);

	}

	if ( streq( self->s.v.classname, "item_artifact_super_damage" ) )
	{
		other->ps.quad++;

		if ( deathmatch == 4 )
		{
			other->s.v.armortype = 0;
			other->s.v.armorvalue = 0 * 0.01;
			other->s.v.ammo_cells = 0;
		}
		other->super_time = 1;
		other->super_damage_finished = g_globalvars.time + 30;

		if ( self->cnt > g_globalvars.time ) // is this was a dropped powerup
			p_cnt = &(other->super_damage_finished);
	}

	if ( p_cnt ) {  // is this was a dropped powerup
			p_cnt[0] = self->cnt;
			G_bprint( PRINT_HIGH, "%s recovered a %s with %d seconds remaining!\n",
			  					other->s.v.netname, self->s.v.netname,
			  					( int ) ( p_cnt[0] - g_globalvars.time ) );

			SUB_RM_01( self );// remove later
	}

	activator = other;
	SUB_UseTargets();	// fire all targets / killtargets
}



/*QUAKED item_artifact_invulnerability (0 .5 .8) (-16 -16 -24) (16 16 32)
Player is invulnerable for 30 seconds
*/
void SP_item_artifact_invulnerability()
{
	self->s.v.touch = ( func_t ) powerup_touch;

	trap_precache_model( "progs/invulner.mdl" );
	trap_precache_sound( "items/protect.wav" );
	trap_precache_sound( "items/protect2.wav" );
	trap_precache_sound( "items/protect3.wav" );
	self->s.v.noise = "items/protect.wav";
	setmodel( self, "progs/invulner.mdl" );
	self->s.v.netname = "Pentagram of Protection";
	self->s.v.classname = "item_artifact_invulnerability";

	self->s.v.effects = ( int ) self->s.v.effects | EF_RED;

	self->s.v.items = IT_INVULNERABILITY;
	setsize( self, -16, -16, -24, 16, 16, 32 );
	StartItem();
}

/*QUAKED item_artifact_envirosuit (0 .5 .8) (-16 -16 -24) (16 16 32)
Player takes no damage from water or slime for 30 seconds
*/
void SP_item_artifact_envirosuit()
{
	self->s.v.touch = ( func_t ) powerup_touch;

	trap_precache_model( "progs/suit.mdl" );
	trap_precache_sound( "items/suit.wav" );
	trap_precache_sound( "items/suit2.wav" );
	self->s.v.noise = "items/suit.wav";
	setmodel( self, "progs/suit.mdl" );
	self->s.v.netname = "Biosuit";
	self->s.v.classname = "item_artifact_envirosuit";
	self->s.v.items = IT_SUIT;
	setsize( self, -16, -16, -24, 16, 16, 32 );
	StartItem();
}


/*QUAKED item_artifact_invisibility (0 .5 .8) (-16 -16 -24) (16 16 32)
Player is invisible for 30 seconds
*/
void SP_item_artifact_invisibility()
{
	qboolean b_dp = self->cnt > g_globalvars.time; // dropped powerup by player, not normal spawn

	self->s.v.touch = ( func_t ) powerup_touch;

	if ( !b_dp ) {
		trap_precache_model( "progs/invisibl.mdl" );
		trap_precache_sound( "items/inv1.wav" );
		trap_precache_sound( "items/inv2.wav" );
		trap_precache_sound( "items/inv3.wav" );
	}
	self->s.v.noise = "items/inv1.wav";
	setmodel( self, "progs/invisibl.mdl" );
	self->s.v.netname = "Ring of Shadows";
	self->s.v.classname = "item_artifact_invisibility";
	self->s.v.items = IT_INVISIBILITY;
	setsize( self, -16, -16, -24, 16, 16, 32 );

	if ( b_dp ) {
		PlaceItemIngame();

		self->s.v.velocity[2] = 300;
		self->s.v.velocity[0] = -100 + ( g_random() * 200 );
		self->s.v.velocity[1] = -100 + ( g_random() * 200 );

		self->s.v.nextthink = self->cnt; // remove it with the time left on it
		self->s.v.think = ( func_t ) SUB_Remove;
	}
	else
		StartItem();
}

/*QUAKED item_artifact_super_damage (0 .5 .8) (-16 -16 -24) (16 16 32)
The next attack from the player will do 4x damage
*/
void SP_item_artifact_super_damage()
{
	qboolean b_dp = self->cnt > g_globalvars.time; // dropped powerup by player, not normal spawn

	self->s.v.touch = ( func_t ) powerup_touch;

	if ( !b_dp ) {
/* need this due to aerowalk customize
		trap_precache_model( "progs/quaddama.mdl" );
		trap_precache_sound( "items/damage.wav" );
		trap_precache_sound( "items/damage2.wav" );
		trap_precache_sound( "items/damage3.wav" );
*/
	}
	self->s.v.noise = "items/damage.wav";
	setmodel( self, "progs/quaddama.mdl" );
	self->s.v.classname = "item_artifact_super_damage";
	if ( deathmatch == 4 )
		self->s.v.netname = "OctaPower";
	else
		self->s.v.netname = "Quad Damage";
	self->s.v.items = IT_QUAD;

	self->s.v.effects = ( int ) self->s.v.effects | EF_BLUE;

	setsize( self, -16, -16, -24, 16, 16, 32 );

	if ( b_dp ) { 	
		PlaceItemIngame();

		self->s.v.velocity[2] = 300;
		self->s.v.velocity[0] = -100 + ( g_random() * 200 );
		self->s.v.velocity[1] = -100 + ( g_random() * 200 );

		self->s.v.nextthink = self->cnt; // remove it with the time left on it
		self->s.v.think = ( func_t ) SUB_Remove;
	}
	else
		StartItem();
}

/*
===============================================================================

PLAYER BACKPACKS

===============================================================================
*/

void BackpackTouch()
{
	float          new;
	gedict_t       *stemp;
	float           acount;

    if ( match_in_progress != 2 )
        return;

	if ( deathmatch == 4 )
		if ( other->invincible_time > 0 )
			return;

	if ( strneq( other->s.v.classname, "player" ) )
		return;

	if ( ISDEAD( other ) )
		return;

	acount = 0;
	G_sprint( other, PRINT_LOW, "You get " );

	if ( deathmatch == 4 )
	{
		other->s.v.health += 10;
		G_sprint( other, PRINT_LOW, "10 additional health\n" );
		if ( ( other->s.v.health > 250 ) && ( other->s.v.health < 300 ) )
			sound( other, CHAN_ITEM, "items/protect3.wav", 1, ATTN_NORM );
		else
			sound( other, CHAN_ITEM, "weapons/lock4.wav", 1, ATTN_NORM );

		stuffcmd( other, "bf\n" );
		ent_remove( self );

		if ( other->s.v.health > 299 )
		{
			if ( other->invincible_time != 1 )
			{
				other->invincible_time = 1;
				other->invincible_finished = g_globalvars.time + 30;
				other->s.v.items =
				    ( int ) other->s.v.items | IT_INVULNERABILITY;

				other->super_time = 1;
				other->super_damage_finished = g_globalvars.time + 30;
				other->s.v.items = ( int ) other->s.v.items | IT_QUAD;

				other->s.v.ammo_cells = 0;


				sound( other, CHAN_VOICE, "boss1/sight1.wav", 1, ATTN_NORM );
				stuffcmd( other, "bf\n" );

				G_bprint( PRINT_HIGH, "%s attains bonus powers!!!\n",
					  other->s.v.netname );
			}
		}
		self = other; // qqshka - hmm ???
		return;
	}

	if ( self->s.v.items )
		if ( ( ( int ) other->s.v.items & ( int ) self->s.v.items ) == 0 )
		{ // new weapon - so print u got it
			acount = 1;
			G_sprint( other, PRINT_LOW, "the %s", self->s.v.netname );
		}

// change weapons
	other->s.v.ammo_shells  = other->s.v.ammo_shells  + self->s.v.ammo_shells;
	other->s.v.ammo_nails   = other->s.v.ammo_nails   + self->s.v.ammo_nails;
	other->s.v.ammo_rockets = other->s.v.ammo_rockets + self->s.v.ammo_rockets;
	other->s.v.ammo_cells   = other->s.v.ammo_cells   + self->s.v.ammo_cells;

	new = self->s.v.items;

	other->s.v.items = ( int ) other->s.v.items | ( int ) self->s.v.items;

	bound_other_ammo();

	if ( self->s.v.ammo_shells )
	{
		if ( acount )
			G_sprint( other, PRINT_LOW, ", " );
		acount = 1;
		G_sprint( other, PRINT_LOW, "%.0f shells", self->s.v.ammo_shells );
	}
	if ( self->s.v.ammo_nails )
	{
		if ( acount )
			G_sprint( other, PRINT_LOW, ", " );
		acount = 1;
		G_sprint( other, PRINT_LOW, "%.0f nails", self->s.v.ammo_nails );
	}
	if ( self->s.v.ammo_rockets )
	{
		if ( acount )
			G_sprint( other, PRINT_LOW, ", " );
		acount = 1;
		G_sprint( other, PRINT_LOW, "%.0f rockets", self->s.v.ammo_rockets );
	}
	if ( self->s.v.ammo_cells )
	{
		if ( acount )
			G_sprint( other, PRINT_LOW, ", " );
		acount = 1;
		G_sprint( other, PRINT_LOW, "%.0f cells", self->s.v.ammo_cells );
	}

	if (    ( deathmatch == 3 || deathmatch == 5 ) 
		 && ( WeaponCode( new ) == 6 || WeaponCode( new ) == 7 ) 
		 && ( other->s.v.ammo_rockets < 5 ) 
	   )
		other->s.v.ammo_rockets = 5;

	G_sprint( other, PRINT_LOW, "\n" );
// backpack touch sound
	sound( other, CHAN_ITEM, "weapons/lock4.wav", 1, ATTN_NORM );
	stuffcmd( other, "bf\n" );

	ent_remove( self );

	stemp = self;
	self = other;

	DoWeaponChange( new ); // change to the weapon

	self = stemp;
}

/*
===============
DropBackpack
===============
*/

#define IT_DROPPABLE_WEAPONS (IT_SUPER_SHOTGUN|IT_NAILGUN|IT_SUPER_NAILGUN|IT_GRENADE_LAUNCHER|IT_ROCKET_LAUNCHER|IT_LIGHTNING)

void DropBackpack()
{
	gedict_t       *item;

    float f1;

    f1 = cvar( "k_frp" );

    if ( match_in_progress != 2 || !cvar( "dp" ) )
        return;


    if ( ! ( self->s.v.ammo_shells + self->s.v.ammo_nails + self->s.v.ammo_rockets +
			 self->s.v.ammo_cells 
		   )
            && ! ( (int)self->s.v.weapon & IT_DROPPABLE_WEAPONS)
            && ! ( f1 == 2 && ( (int)self->lastwepfired & IT_DROPPABLE_WEAPONS ) ) 
	   )
		return; // nothing in it


	item = spawn();

	VectorCopy( self->s.v.origin, item->s.v.origin );
	item->s.v.origin[2] -= 24;

	item->s.v.items = self->s.v.weapon;

#ifdef KTEAMS
// drop best weapon in case of fairpacks 1 (KTEAMS)
        if( f1 == 1 )
        {
            if( ( (int)self->s.v.items & IT_NAILGUN ) 		  && self->s.v.ammo_nails   > 0 )
				item->s.v.items = IT_NAILGUN;
            if( ( (int)self->s.v.items & IT_SUPER_SHOTGUN )	  && self->s.v.ammo_shells  > 0 )
				item->s.v.items = IT_SUPER_SHOTGUN;
            if( ( (int)self->s.v.items & IT_SUPER_NAILGUN )	  && self->s.v.ammo_nails   > 0 )
				item->s.v.items = IT_SUPER_NAILGUN;
            if( ( (int)self->s.v.items & IT_GRENADE_LAUNCHER ) && self->s.v.ammo_rockets > 0 )
				item->s.v.items = IT_GRENADE_LAUNCHER;
            if( ( (int)self->s.v.items & IT_LIGHTNING )		  && self->s.v.ammo_cells   > 0 )
				item->s.v.items = IT_LIGHTNING;
            if( ( (int)self->s.v.items & IT_ROCKET_LAUNCHER )  && self->s.v.ammo_rockets > 0 )
				item->s.v.items = IT_ROCKET_LAUNCHER;
        }

// drop lastfired even if no ammo in case of fairpacks 2 (KTEAMS)
        if( f1 == 2 )
            if( (int)self->lastwepfired & IT_DROPPABLE_WEAPONS )
                item->s.v.items = self->lastwepfired;
#endif

	if ( item->s.v.items == IT_AXE )
		item->s.v.netname = "Axe";
	else if ( item->s.v.items == IT_SHOTGUN )
		item->s.v.netname = "Shotgun";
	else if ( item->s.v.items == IT_SUPER_SHOTGUN )
		item->s.v.netname = "Double-barrelled Shotgun";
	else if ( item->s.v.items == IT_NAILGUN )
		item->s.v.netname = "Nailgun";
	else if ( item->s.v.items == IT_SUPER_NAILGUN )
		item->s.v.netname = "Super Nailgun";
	else if ( item->s.v.items == IT_GRENADE_LAUNCHER )
		item->s.v.netname = "Grenade Launcher";
	else if ( item->s.v.items == IT_ROCKET_LAUNCHER )
		item->s.v.netname = "Rocket Launcher";
	else if ( item->s.v.items == IT_LIGHTNING )
		item->s.v.netname = "Thunderbolt";
	else
		item->s.v.netname = "";

	item->s.v.ammo_shells = self->s.v.ammo_shells;
	item->s.v.ammo_nails = self->s.v.ammo_nails;
	item->s.v.ammo_rockets = self->s.v.ammo_rockets;
	item->s.v.ammo_cells = self->s.v.ammo_cells;

	item->s.v.velocity[2] = 300;
	item->s.v.velocity[0] = -100 + ( g_random() * 200 );
	item->s.v.velocity[1] = -100 + ( g_random() * 200 );

	item->s.v.flags = FL_ITEM;
	item->s.v.solid = SOLID_TRIGGER;
	item->s.v.movetype = MOVETYPE_TOSS;
	setmodel( item, "progs/backpack.mdl" );
	setsize( item, -16, -16, 0, 16, 16, 56 );
	item->s.v.touch = ( func_t ) BackpackTouch;

	item->s.v.nextthink = g_globalvars.time + 120;	// remove after 2 minutes
	item->s.v.think = ( func_t ) SUB_Remove;
}
