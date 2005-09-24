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
 *  $Id: combat.c,v 1.1.1.1 2005/09/24 12:45:02 disconn3ct Exp $
 */

#include "g_local.h"
void            T_MissileTouch( gedict_t * e1, gedict_t * e2 );
void            ClientObituary( gedict_t * e1, gedict_t * e2 );

//void(entity inflictor, entity attacker, float damage, entity ignore, string dtype) T_RadiusDamage;

#ifdef KTEAMS
void EndMatch (float skip_log);
#endif

//============================================================================

/*
============
CanDamage

Returns true if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
qboolean CanDamage( gedict_t * targ, gedict_t * inflictor )
{
// bmodels need special checking because their origin is 0,0,0
	if ( targ->s.v.movetype == MOVETYPE_PUSH )
	{

		traceline( PASSVEC3( inflictor->s.v.origin ),
				0.5 * ( targ->s.v.absmin[0] + targ->s.v.absmax[0] ),
				0.5 * ( targ->s.v.absmin[1] + targ->s.v.absmax[1] ),
				0.5 * ( targ->s.v.absmin[2] + targ->s.v.absmax[2] ),
				true, self );

		if ( g_globalvars.trace_fraction == 1 )
			return true;

		if ( PROG_TO_EDICT( g_globalvars.trace_ent ) == targ )
			return true;

		return false;
	}

	traceline( PASSVEC3( inflictor->s.v.origin ), PASSVEC3( targ->s.v.origin ),
			true, self );
	if ( g_globalvars.trace_fraction == 1 )
		return true;

	traceline( PASSVEC3( inflictor->s.v.origin ),
			targ->s.v.origin[0] + 15, targ->s.v.origin[1] + 15,
			targ->s.v.origin[2] + 0, true, self );
	if ( g_globalvars.trace_fraction == 1 )
		return true;

	traceline( PASSVEC3( inflictor->s.v.origin ),
			targ->s.v.origin[0] - 15, targ->s.v.origin[1] - 15,
			targ->s.v.origin[2] + 0, true, self );
	if ( g_globalvars.trace_fraction == 1 )
		return true;

	traceline( PASSVEC3( inflictor->s.v.origin ),
			targ->s.v.origin[0] - 15, targ->s.v.origin[1] + 15,
			targ->s.v.origin[2] + 0, true, self );
	if ( g_globalvars.trace_fraction == 1 )
		return true;

	traceline( PASSVEC3( inflictor->s.v.origin ),
			targ->s.v.origin[0] + 15, targ->s.v.origin[1] - 15,
			targ->s.v.origin[2] + 0, true, self );
	if ( g_globalvars.trace_fraction == 1 )
		return true;


	return false;
}


/*
============
Killed
============
*/
void Killed( gedict_t * targ, gedict_t * attacker )
{
	gedict_t       *oself;

	oself = self;
	self = targ;

#ifdef KTEAMS
	if ( streq( self->s.v.classname, "player" ) )
        self->dead_time = g_globalvars.time;
#endif

	if ( self->s.v.health < -99 )
		self->s.v.health = -99;	// don't let sbar look bad if a player

	if ( self->s.v.movetype == MOVETYPE_PUSH || self->s.v.movetype == MOVETYPE_NONE )
	{			// doors, triggers, etc
//  if(self->th_die)
		self->th_die();
		self = oself;
		return;
	}

	self->s.v.enemy = EDICT_TO_PROG( attacker );

// bump the monster counter
	if ( ( ( int ) ( self->s.v.flags ) ) & FL_MONSTER )
	{
		g_globalvars.killed_monsters++;
		trap_WriteByte( MSG_ALL, SVC_KILLEDMONSTER );
	}

	ClientObituary( self, attacker );

	self->s.v.takedamage = DAMAGE_NO;
	self->s.v.touch = ( func_t ) SUB_Null;
	self->s.v.effects = 0;

/*SERVER
 monster_death_use();
*/
	//if(self->th_die)
	self->th_die();

	self = oself;
}

#ifndef Q3_VM
float newceil( float f )
{
        return ceil(((int)(f*1000.0))/1000.0);

}
#endif

/*
============
T_Damage

The damage is coming from inflictor, but get mad at attacker
This should be the only function that ever reduces health.
============
*/
gedict_t       *damage_attacker, *damage_inflictor;
void T_Damage( gedict_t * targ, gedict_t * inflictor, gedict_t * attacker, float damage )
{
	vec3_t          dir;
	gedict_t       *oldself;
	float           save;
	float           take;

	//char*  s;
	char            attackerteam[10], targteam[10];


	if ( !targ->s.v.takedamage )
		return;

// used by buttons and triggers to set activator for target firing
	damage_attacker = attacker;


// check for quad damage powerup on the attacker
	if ( ( attacker->super_damage_finished > g_globalvars.time )
	     && strneq( inflictor->s.v.classname, "door" ) && strneq( targ->deathtype, "stomp" ) )
	{
		if ( deathmatch == 4 )
			damage = damage * 8;
		else
			damage = damage * 4;
	}
// save damage based on the target's armor level

#ifndef Q3_VM
        save = newceil( targ->s.v.armortype * damage );
#else
        save = ceil( targ->s.v.armortype * damage );
#endif
	if ( save >= targ->s.v.armorvalue )
	{
		save = targ->s.v.armorvalue;
		targ->s.v.armortype = 0;	// lost all armor
		targ->s.v.items -=
		    ( ( int ) targ->s.v.items & ( IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3 ) );
	}

	targ->s.v.armorvalue = targ->s.v.armorvalue - save;
#ifndef Q3_VM
        take = newceil( damage - save );
#else
        take = ceil( damage - save );
#endif

// add to the damage total for clients, which will be sent as a single
// message at the end of the frame
// FIXME: remove after combining shotgun blasts?
	if ( ( int ) targ->s.v.flags & FL_CLIENT )
	{
		targ->s.v.dmg_take += take;
		targ->s.v.dmg_save += save;
		targ->s.v.dmg_inflictor = EDICT_TO_PROG( inflictor );
	}

	damage_inflictor = inflictor;


// figure momentum add
	if ( ( inflictor != world ) && ( targ->s.v.movetype == MOVETYPE_WALK ) )
	{
		dir[0] =
		    targ->s.v.origin[0] - ( inflictor->s.v.absmin[0] +
					    inflictor->s.v.absmax[0] ) * 0.5;
		dir[1] =
		    targ->s.v.origin[1] - ( inflictor->s.v.absmin[1] +
					    inflictor->s.v.absmax[1] ) * 0.5;
		dir[2] =
		    targ->s.v.origin[2] - ( inflictor->s.v.absmin[2] +
					    inflictor->s.v.absmax[2] ) * 0.5;
		VectorNormalize( dir );
		// Set kickback for smaller weapons
//Zoid -- use normal NQ kickback
//  // Read: only if it's not yourself doing the damage
//  if ( (damage < 60) & ((attacker->s.v.classname  == "player") & (targ->s.v.classname  == "player")) & ( attacker.netname != targ.netname)) 
//   targ.velocity = targ.velocity + dir * damage * 11;
//  else                        
		// Otherwise, these rules apply to rockets and grenades                        
		// for blast velocity

		targ->s.v.velocity[0] += dir[0] * damage * 8;
		targ->s.v.velocity[1] += dir[1] * damage * 8;
		targ->s.v.velocity[2] += dir[2] * damage * 8;
		// Rocket Jump modifiers
		if ( ( rj > 1 )
		     && ( ( streq( attacker->s.v.classname, "player" ) )
			  && streq( targ->s.v.classname, "player" ) )
		     && streq( attacker->s.v.netname, targ->s.v.netname ) )
		{
			VectorAdd( targ->s.v.velocity, dir, targ->s.v.velocity );
			VectorScale( targ->s.v.velocity, damage * 8 * rj,
				     targ->s.v.velocity );
		}

	}


// check for godmode or invincibility
	if ( ( int ) targ->s.v.flags & FL_GODMODE )
		return;

	if ( targ->invincible_finished >= g_globalvars.time )
	{
		if ( self->invincible_sound < g_globalvars.time )
		{
			sound( targ, CHAN_ITEM, "items/protect3.wav", 1, ATTN_NORM );
			self->invincible_sound = g_globalvars.time + 2;
		}
		return;
	}
// team play damage avoidance
//ZOID 12-13-96: self.team doesn't work in QW.  Use keys
	infokey( attacker, "team", attackerteam, sizeof( attackerteam ) );
	infokey( targ, "team", targteam, sizeof( targteam ) );

	if ( ( teamplay == 1 ) && !strcmp( targteam, attackerteam ) &&
	     !strcmp( attacker->s.v.classname, "player" ) && strcmp( attackerteam, "" ) &&
	     strcmp( inflictor->s.v.classname, "door" ) )
		return;

	if ( ( teamplay == 3 ) && !strcmp( targteam, attackerteam ) &&
	     !strcmp( attacker->s.v.classname, "player" ) && strcmp( attackerteam, "" ) &&
	     targ != attacker && strcmp( inflictor->s.v.classname, "door" ) )
		return;

// do the damage
#ifdef KTEAMS
	if ( match_in_progress == 2 || !strcmp( attacker->s.v.classname, "teledeath" ) )
#endif
		targ->s.v.health -= take;

	if ( targ->s.v.health <= 0 )
	{
		Killed( targ, attacker );
#ifdef KTEAMS
        // KTEAMS: check if sudden death is the case
        if (k_sudden_death && !strcmp( targ->s.v.classname, "player" ) ) 
            EndMatch(0);
#endif
		return;
	}
// react to the damage
	oldself = self;
	self = targ;

/*SERVER
 if ( (self.flags & FL_MONSTER) && attacker != world)
 {
 // get mad unless of the same class (except for soldiers)
  if (self != attacker && attacker != self.enemy)
  {
   if ( (self->s.v.classname  != attacker->s.v.classname ) 
   || (self->s.v.classname  == "monster_army" ) )
   {
    if (self.enemy->s.v.classname  == "player")
     self.oldenemy = self.enemy;
    self.enemy = attacker;
    FoundTarget ();
   }
  }
 }
*/
	if ( self->th_pain )
	{
		self->th_pain( attacker, take );
	}

	self = oldself;
}

/*
============
T_RadiusDamage
============
*/
void T_RadiusDamage( gedict_t * inflictor, gedict_t * attacker, float damage,
		     gedict_t * ignore, char *dtype )
{
	float           points;
	gedict_t       *head;
	vec3_t          org;

	head = findradius( world, inflictor->s.v.origin, damage + 40 );

	while ( head )
	{
		//bprint (PRINT_HIGH, head->s.v.classname );
		//bprint (PRINT_HIGH, " | ");
		//bprint (PRINT_HIGH, head.netname);
		//bprint (PRINT_HIGH, "\n");

		if ( head != ignore )
		{
			if ( head->s.v.takedamage )
			{
				org[0] =
				    inflictor->s.v.origin[0] - ( head->s.v.origin[0] +
								 ( head->s.v.mins[0] +
								   head->s.v.maxs[0] ) *
								 0.5 );
				org[1] =
				    inflictor->s.v.origin[1] - ( head->s.v.origin[1] +
								 ( head->s.v.mins[1] +
								   head->s.v.maxs[1] ) *
								 0.5 );
				org[2] =
				    inflictor->s.v.origin[2] - ( head->s.v.origin[2] +
								 ( head->s.v.mins[2] +
								   head->s.v.maxs[2] ) *
								 0.5 );
				points = 0.5 * vlen( org );
				if ( points < 0 )
					points = 0;

				points = damage - points;

				if ( head == attacker )
					points = points * 0.5;

				if ( points > 0 )
				{
					if ( CanDamage( head, inflictor ) )
					{
						head->deathtype = dtype;
						T_Damage( head, inflictor, attacker,
							  points );
					}
				}
			}
		}
		head = findradius( head, inflictor->s.v.origin, damage + 40 );
	}
}

/*
============
T_BeamDamage
============
*/
void T_BeamDamage( gedict_t * attacker, float damage )
{
	vec3_t          tmpv;
	float           points;
	gedict_t       *head;

	head = findradius( world, attacker->s.v.origin, damage + 40 );

	while ( head )
	{
		if ( head->s.v.takedamage )
		{
			VectorSubtract( attacker->s.v.origin, head->s.v.origin, tmpv )
			    points = 0.5 * vlen( tmpv );
			if ( points < 0 )
				points = 0;

			points = damage - points;
			if ( head == attacker )
				points = points * 0.5;

			if ( points > 0 )
			{
				if ( CanDamage( head, attacker ) )
					T_Damage( head, attacker, attacker, points );
			}
		}
		head = findradius( head, attacker->s.v.origin, damage + 40 );
	}
}
