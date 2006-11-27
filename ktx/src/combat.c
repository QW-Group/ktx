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
 *  $Id: combat.c,v 1.35 2006/11/27 22:47:06 qqshka Exp $
 */

#include "g_local.h"
void            T_MissileTouch( gedict_t * e1, gedict_t * e2 );
void            ClientObituary( gedict_t * e1, gedict_t * e2 );

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
void Killed( gedict_t * targ, gedict_t * attacker, gedict_t * inflictor )
{
	gedict_t       *oself;

	oself = self;
	self = targ;

	if ( streq( self->s.v.classname, "player" ) )
        self->dead_time = g_globalvars.time;

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
		WriteByte( MSG_ALL, SVC_KILLEDMONSTER );
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
//	int				wp_num;
	int				dmg_frags, i, c1 = 8, c2 = 4, hdp;
	float			dmg_dealt = 0, non_hdp_damage;
	char            *attackerteam, *targteam;

	//midair
	float playerheight, midheight = 0;
	qboolean lowheight = false, midair = false, inwater = false, do_dmg = false, rl_dmg = false;

	if ( !targ->s.v.takedamage || ISDEAD( targ ) )
		return;

	if ( (int)cvar("k_midair") )
		midair = true;

//	wp_num = attacker->s.v.weapon;

// used by buttons and triggers to set activator for target firing
	damage_attacker = attacker;

// check for quad damage powerup on the attacker
// midair quad makes rockets fast, but no change to damage
	if ( ( attacker->super_damage_finished > g_globalvars.time )
	     && strneq( inflictor->s.v.classname, "door" ) && dtSTOMP != targ->deathtype
		 && !midair )
	{
		if ( deathmatch == 4 )
			damage = damage * 8;
		else
			damage = damage * 4;
	}

// ctf strength rune
	if ( attacker->ctf_flag & CTF_RUNE_STR )
		damage *= 2;
          
// ctf resistance rune
	if ( targ->ctf_flag & CTF_RUNE_RES )
	{
		damage /= 2;
		ResistanceSound( targ );
	}

// did we hurt enemy flag carrier?
	if ( (targ->ctf_flag & CTF_FLAG) && (!streq(getteam(targ), getteam(attacker))) )
	{
		attacker->carrier_hurt_time = g_globalvars.time;
	}

	if ( midair )
	{
		inwater = ( ((int)targ->s.v.flags & FL_INWATER) && targ->s.v.waterlevel > 1 );

		if ( streq( inflictor->s.v.classname, "rocket" ) )
		{
			midheight = targ->s.v.origin[2] - inflictor->s.v.oldorigin[2];
			if ( midheight <= 190 )
				midheight = 0;
		}
		traceline( PASSVEC3(targ->s.v.origin),
				targ->s.v.origin[0], 
				targ->s.v.origin[1], 
				targ->s.v.origin[2] - 2048,
				true, targ );

		playerheight = targ->s.v.absmin[2] - g_globalvars.trace_endpos[2];

		if ( playerheight < 45 )
			lowheight = true;
		else
		{
			damage *= ( 1 + ( playerheight - 45 ) / 64 );
			lowheight = false;
		}

		rl_dmg = ( targ->k_player && dtRL == targ->deathtype );

		if ( !rl_dmg ) {
			// damage types which ignore "lowheight"
			do_dmg =   !targ->k_player					// always do damage to non player, secret doors etc...
				 	|| dtAXE == targ->deathtype			// always do axe damage
				 	|| dtWATER_DMG == targ->deathtype	// always do water damage
				 	|| dtLAVA_DMG  == targ->deathtype	// always do lava damage
				 	|| dtSLIME_DMG == targ->deathtype	// always do slime damage
				 	|| dtTELE1 == targ->deathtype	// always do tele damage
				 	|| dtTELE2 == targ->deathtype	// always do tele damage
				 	|| dtTELE3 == targ->deathtype;	// always do tele damage
		}
	}

	non_hdp_damage = damage; // save damage before handicap apply for kickback calculation

	// #handicap#
	if ( attacker != targ ) // attack no self
	if ( attacker->k_player && targ->k_player ) // player vs player
	if ( ( hdp = GetHandicap(attacker) ) != 100 ) // skip checks if hdp == 100
	if (    dtAXE  == targ->deathtype
 		 || dtSG   == targ->deathtype
		 || dtSSG  == targ->deathtype
		 || dtNG   == targ->deathtype
		 || dtSNG  == targ->deathtype
		 || dtGL   == targ->deathtype
		 || dtRL   == targ->deathtype
		 || dtLG_BEAM     == targ->deathtype
		 || dtLG_DIS      == targ->deathtype
		 || dtLG_DIS_SELF == targ->deathtype // even that impossible
	   ) {
		damage *= 0.01f * hdp;
	}

// save damage based on the target's armor level

#ifndef Q3_VM
	save = newceil( targ->s.v.armortype * damage );
#else
	save = ceil( targ->s.v.armortype * damage );
#endif

	if ( midair && lowheight && !inwater && rl_dmg )
		save *= 0.5; // take half of armor in such case

	if ( save >= targ->s.v.armorvalue )
	{
		save = targ->s.v.armorvalue;
		targ->s.v.armortype = 0;	// lost all armor
		targ->s.v.items -= ( ( int ) targ->s.v.items & ( IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3 ) );
	}
	dmg_dealt += save;

	if ( match_in_progress == 2 )
		targ->s.v.armorvalue = targ->s.v.armorvalue - save;

#ifndef Q3_VM
	take = newceil( damage - save );
#else
	take = ceil( damage - save );
#endif

	if ( midair ) {
		if ( lowheight && !inwater && rl_dmg )
			take = 0; // no rl dmg in such case

		if ( !rl_dmg && !do_dmg )
			take = 0; // unknown damage for midair, so do not damage

		if ( rl_dmg && targ == attacker )
			take = 0; // no self rl damage
	}

	take = max(0, take); // avoid negative take, if any

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
	if ( inflictor != world && targ->s.v.movetype == MOVETYPE_WALK )
	{
		for ( i = 0; i < 3; i++ )
			dir[i] = targ->s.v.origin[i] - ( inflictor->s.v.absmin[i] + inflictor->s.v.absmax[i] ) * 0.5;

		VectorNormalize( dir );

		if ( midair && non_hdp_damage < 60 && attacker != targ ) {
			c1 = 11;
			c2 = 6;
		}

		for ( i = 0; i < 3; i++ )
			targ->s.v.velocity[i] += dir[i] * non_hdp_damage * c1;

		if ( midair && lowheight )
			targ->s.v.velocity[2] += dir[2] * non_hdp_damage * c2; // only for z component

		// Rocket Jump modifiers
/*
		if ( ( rj > 1 )
		     && ( ( streq( attacker->s.v.classname, "player" ) )
			  && streq( targ->s.v.classname, "player" ) )
		     && streq( attacker->s.v.netname, targ->s.v.netname ) )
		{
			VectorAdd( targ->s.v.velocity, dir, targ->s.v.velocity );
			VectorScale( targ->s.v.velocity, non_hdp_damage * 8 * rj,
				     targ->s.v.velocity );
		}
*/
	}

// check for godmode or invincibility
	if ( ( int ) targ->s.v.flags & FL_GODMODE )
		return;

	if ( targ->invincible_finished >= g_globalvars.time )
	{
		if ( targ->invincible_sound < g_globalvars.time )
		{
			sound( targ, CHAN_AUTO, "items/protect3.wav", 1, ATTN_NORM );
			targ->invincible_sound = g_globalvars.time + 2;
		}
		return;
	}

// team play damage avoidance
//ZOID 12-13-96: self.team doesn't work in QW.  Use keys
    attackerteam = getteam( attacker );
	targteam = getteam( targ );

	// teamplay == 1 don't damage self and mates (armor affected anyway)
	if ( tp_num() == 1
		 && !strnull( attackerteam )
		 && streq( targteam, attackerteam )
		 && streq( attacker->s.v.classname, "player" )
		 && strneq( inflictor->s.v.classname, "door" )
		 && strneq( inflictor->s.v.classname, "teledeath" ) // do telefrag damage in tp
	   )
		return;

	// teamplay == 3 don't damage mates, do damage to self (armor affected anyway)
	if ( tp_num() == 3
		 && !strnull( attackerteam )
		 && streq( targteam, attackerteam )
		 && streq( attacker->s.v.classname, "player" )
		 && strneq( inflictor->s.v.classname, "door" )
		 && strneq( inflictor->s.v.classname, "teledeath" ) // do telefrag damage in tp
		 && targ != attacker
	   )
		return;

// do the damage

	if (    match_in_progress == 2
		 || streq( inflictor->s.v.classname, "teledeath" ) 
		 || ( k_practice && strneq( targ->s.v.classname, "player" ) ) // #practice mode#
	   ) {

		dmg_dealt += targ->s.v.health > take ? take : targ->s.v.health;
		targ->s.v.health -= take;

		if ( !targ->s.v.health )
			targ->s.v.health = -1; // qqshka, no zero health, heh, imo less bugs after this
	}

	if (match_in_progress != 2) {
		if ( !midair || (int)targ->s.v.flags & FL_ONGROUND ) {
			targ->s.v.currentammo = 1000 + Q_rint(damage);
			if (attacker != targ)
				attacker->s.v.health = 1000 + Q_rint(damage);
		}
	}

	if ( attacker->k_player && targ->k_player ) 
	{
		if ( attacker != targ )
		{
			if ( streq(attackerteam, targteam) && !isDuel() )
				attacker->ps.dmg_team += dmg_dealt;
			else 
			{
				attacker->ps.dmg_g += dmg_dealt;
				targ->ps.dmg_t += dmg_dealt;
				if ( (int)cvar("k_dmgfrags") )
				{
					dmg_frags = attacker->ps.dmg_g / 100;
					attacker->s.v.frags = attacker->s.v.frags - attacker->ps.dmg_frags + dmg_frags;
					attacker->ps.dmg_frags = dmg_frags;
				}
			}
		}
	}

	if ( midair && match_in_progress == 2 && midheight > 190 )
 	{
 		attacker->ps.midairs++;
 		G_bprint( 2, "%s got ", attacker->s.v.netname );

		if ( midheight > 900 )
 		{
 			attacker->ps.midairs_d++;
 			attacker->s.v.frags += 8;
 			G_bprint( 2, "%s\n", redtext("diam0nd midair") );
 		}
 		else if ( midheight > 500 )
 		{
 			G_bprint( 2, "%s\n", redtext("g0ld midair") );
 			attacker->s.v.frags += 4;
 			attacker->ps.midairs_g++;
 		}
 		else if ( midheight > 380 )
 		{
 			G_bprint( 2, "%s\n", redtext("silver midair") );
 			attacker->s.v.frags += 2;
 			attacker->ps.midairs_s++;
 		}
 		else
 		{
 			G_bprint( 2, "%s\n", redtext("midair") );
 			attacker->s.v.frags++;
 		}
 		G_bprint(2, "%.1f (midheight)\n", midheight);
 	}

	if ( ISDEAD( targ ) )
	{
		Killed( targ, attacker, inflictor );

        // KTEAMS: check if sudden death is the case
		Check_SD( targ );

		// check fraglimit
		if (	fraglimit
			&& (   ( targ->s.v.frags >= fraglimit && streq( targ->s.v.classname, "player" ) )
			 	|| ( attacker->s.v.frags >= fraglimit && streq( attacker->s.v.classname, "player" ) )
			   )
		   )
           	EndMatch( 0 );

		return;
	}
// react to the damage
	oldself = self;
	self = targ;

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
void T_RadiusDamage( gedict_t * inflictor, gedict_t * attacker, float damage, gedict_t * ignore, deathType_t dtype )
{
	float           points;
	gedict_t       *head;
	vec3_t          org;

	head = trap_findradius( world, inflictor->s.v.origin, damage + 40 );

	while ( head )
	{
		//bprint (PRINT_HIGH, "%s", head->s.v.classname );
		//bprint (PRINT_HIGH, " | ");
		//bprint (PRINT_HIGH, "%s", head.netname);
		//bprint (PRINT_HIGH, "\n");

		if ( head != ignore )
		{
			if ( head->s.v.takedamage )
			{
				org[0] = inflictor->s.v.origin[0] - ( head->s.v.origin[0] + ( head->s.v.mins[0] + head->s.v.maxs[0] ) * 0.5 );
				org[1] = inflictor->s.v.origin[1] - ( head->s.v.origin[1] + ( head->s.v.mins[1] + head->s.v.maxs[1] ) * 0.5 );
				org[2] = inflictor->s.v.origin[2] - ( head->s.v.origin[2] + ( head->s.v.mins[2] + head->s.v.maxs[2] ) * 0.5 );
				points = 0.5 * vlen( org );

				if ( points < 0 )
					points = 0;

				points = damage - points;

				if ( head == attacker )
					points = points * 0.5;
				// no out of water discharge damage if k_dis 2
				else if ( cvar("k_dis") == 2 && dtLG_DIS == dtype && !head->s.v.waterlevel )
					points = 0;

				if ( points > 0 )
				{
					if ( CanDamage( head, inflictor ) )
					{
						head->deathtype = dtype;
						T_Damage( head, inflictor, attacker, points );
					}
				}
			}
		}
		head = trap_findradius( head, inflictor->s.v.origin, damage + 40 );
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

	head = trap_findradius( world, attacker->s.v.origin, damage + 40 );

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
		head = trap_findradius( head, attacker->s.v.origin, damage + 40 );
	}
}
