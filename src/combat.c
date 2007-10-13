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
void	T_MissileTouch( gedict_t * e1, gedict_t * e2 );
void	ClientObituary( gedict_t * e1, gedict_t * e2 );

char	*dmg_type[] = { "none", "axe", "sg", "ssg", "ng", "sng", "gl", "rl", 
						"lg_beam", "lg_dis", "lg_dis_self", "hook",
						"changelevel", "lava", "slime", "water", "fall", "stomp",
						"tele1", "tele2", "tele3", "explo_box", "laser", "fireball",
						"squish", "trigger", "suicide", "unknown" };

int	dmg_type_cnt = sizeof(dmg_type) / sizeof(dmg_type[0]);


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
	vec3_t	dif;

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

	traceline( PASSVEC3( inflictor->s.v.origin ), PASSVEC3( targ->s.v.origin ),	true, self );
	if ( g_globalvars.trace_fraction == 1 )
		return true;

// 1998-09-16 CanDamage fix by Maddes/Kryten start

	// testing middle of half-size bounding box
	dif[2] = 0;

	// ...front right
	dif[1] = targ->s.v.maxs[1] * 0.5;
	dif[0] = targ->s.v.maxs[0] * 0.5;
	traceline(PASSVEC3( inflictor->s.v.origin ), 
		targ->s.v.origin[0] + dif[0], targ->s.v.origin[1] + dif[1], targ->s.v.origin[2] + dif[2], true, self);
	if ( g_globalvars.trace_fraction == 1 )
		return true;

	// ...front left
	dif[0] = targ->s.v.mins[0] * 0.5;
	traceline(PASSVEC3( inflictor->s.v.origin ),
		targ->s.v.origin[0] + dif[0], targ->s.v.origin[1] + dif[1], targ->s.v.origin[2] + dif[2], true, self);
	if ( g_globalvars.trace_fraction == 1 )
		return true;

	// ...back left
	dif[1] = targ->s.v.mins[1] * 0.5;
	traceline(PASSVEC3( inflictor->s.v.origin ),
		targ->s.v.origin[0] + dif[0], targ->s.v.origin[1] + dif[1], targ->s.v.origin[2] + dif[2], true, self);
	if ( g_globalvars.trace_fraction == 1 )
		return true;

	// ...back right
	dif[0] = targ->s.v.maxs[0] * 0.5;
	traceline(PASSVEC3( inflictor->s.v.origin ),
		targ->s.v.origin[0] + dif[0], targ->s.v.origin[1] + dif[1], targ->s.v.origin[2] + dif[2], true, self);
	if ( g_globalvars.trace_fraction == 1 )
		return true;

	// testing top of half-sized bounding box
	dif[2] = targ->s.v.maxs[2] * 0.5;

	// ...front right
	dif[1] = targ->s.v.maxs[1] * 0.5;
	dif[0] = targ->s.v.maxs[0] * 0.5;
	traceline(PASSVEC3( inflictor->s.v.origin ),
		targ->s.v.origin[0] + dif[0], targ->s.v.origin[1] + dif[1], targ->s.v.origin[2] + dif[2], true, self);
	if ( g_globalvars.trace_fraction == 1 )
		return true;

	// ...front left
	dif[0] = targ->s.v.mins[0] * 0.5;
	traceline(PASSVEC3( inflictor->s.v.origin ),
		targ->s.v.origin[0] + dif[0], targ->s.v.origin[1] + dif[1], targ->s.v.origin[2] + dif[2], true, self);
	if ( g_globalvars.trace_fraction == 1 )
		return true;

	// ...back left
	dif[1] = targ->s.v.mins[1] * 0.5;
	traceline(PASSVEC3( inflictor->s.v.origin ),
		targ->s.v.origin[0] + dif[0], targ->s.v.origin[1] + dif[1], targ->s.v.origin[2] + dif[2], true, self);
	if ( g_globalvars.trace_fraction == 1 )
		return true;

	// ...back right
	dif[0] = targ->s.v.maxs[0] * 0.5;
	traceline(PASSVEC3( inflictor->s.v.origin ),
		targ->s.v.origin[0] + dif[0], targ->s.v.origin[1] + dif[1], targ->s.v.origin[2] + dif[2], true, self);
	if ( g_globalvars.trace_fraction == 1 )
		return true;

	// testing bottom of half-sized bounding box
	dif[2] = targ->s.v.mins[2] * 0.5;

	// ...front right
	dif[1] = targ->s.v.maxs[1] * 0.5;
	dif[0] = targ->s.v.maxs[0] * 0.5;
	traceline(PASSVEC3( inflictor->s.v.origin ),
		targ->s.v.origin[0] + dif[0], targ->s.v.origin[1] + dif[1], targ->s.v.origin[2] + dif[2], true, self);
	if ( g_globalvars.trace_fraction == 1 )
		return true;

	// ...front left
	dif[0] = targ->s.v.mins[0] * 0.5;
	traceline(PASSVEC3( inflictor->s.v.origin ),
		targ->s.v.origin[0] + dif[0], targ->s.v.origin[1] + dif[1], targ->s.v.origin[2] + dif[2], true, self);
	if ( g_globalvars.trace_fraction == 1 )
		return true;

	// ...back left
	dif[1] = targ->s.v.mins[1] * 0.5;
	traceline(PASSVEC3( inflictor->s.v.origin ),
		targ->s.v.origin[0] + dif[0], targ->s.v.origin[1] + dif[1], targ->s.v.origin[2] + dif[2], true, self);
	if ( g_globalvars.trace_fraction == 1 )
		return true;

	// ...back right
	dif[0] = targ->s.v.maxs[0] * 0.5;
	traceline(PASSVEC3( inflictor->s.v.origin ),
		targ->s.v.origin[0] + dif[0], targ->s.v.origin[1] + dif[1], targ->s.v.origin[2] + dif[2], true, self);
	if ( g_globalvars.trace_fraction == 1 )
		return true;

// 1998-09-16 CanDamage fix by Maddes/Kryten end

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

	if ( self->s.v.health < -99 )
		self->s.v.health = -99;	// don't let sbar look bad if a player

	if ( self->ct == ctPlayer )
	{
        self->dead_time = g_globalvars.time;
	}
	else if ( self->s.v.movetype == MOVETYPE_PUSH || self->s.v.movetype == MOVETYPE_NONE )
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

	//if(self->th_die)
	self->th_die();

	self = oself;

	// KTEAMS: check if sudden death is the case
	Check_SD( targ );

	// check fraglimit
	if (	fraglimit
		&& (   ( targ->s.v.frags >= fraglimit && targ->ct == ctPlayer )
			|| ( attacker->s.v.frags >= fraglimit && attacker->ct == ctPlayer )
		   )
		)
		EndMatch( 0 );
}

#ifndef Q3_VM

// qvm have some bugs/round problem as i get from SD-Angel, so this trick

float newceil( float f )
{
	return ceil(((int)(f*1000.0))/1000.0);
}

#else

// native use lib ceil function

#define newceil ceil

#endif

// this was part of T_Damage(), but I split it, so less mess
void MidairDamageBonus(gedict_t *attacker, float midheight)
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


/*
============
T_Damage

The damage is coming from inflictor, but get mad at attacker
This should be the only function that ever reduces health.
============
*/

gedict_t       *damage_attacker, *damage_inflictor;

static int	dmg_is_splash = 0;

void T_Damage( gedict_t * targ, gedict_t * inflictor, gedict_t * attacker, float damage )
{
	vec3_t          dir;
	gedict_t       *oldself;
	float           save;
	float           take;
	int				i, c1 = 8, c2 = 4, hdp;
	float			dmg_dealt = 0, non_hdp_damage;
	char            *attackerteam, *targteam, *attackername;

	//midair and instagib
	float playerheight = 0, midheight = 0;
	qboolean lowheight = false, midair = false, inwater = false, do_dmg = false, rl_dmg = false;

	// can't apply damage to dead
	if ( !targ->s.v.takedamage || ISDEAD( targ ) )
		return;

	// used by buttons and triggers to set activator for target firing
	damage_attacker = attacker;
	damage_inflictor = inflictor;

	if ( (int)cvar("k_midair") )
		midair = true;

	// check for quad damage powerup on the attacker
	// midair quad makes rockets fast, but no change to damage
	if ( attacker->super_damage_finished > g_globalvars.time
	     && strneq( inflictor->s.v.classname, "door" ) && dtSTOMP != targ->deathtype
		 && !midair 
	   )
		damage *= ( deathmatch == 4 ? 8 : 4 ); // in dmm4 quad is octa actually

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

	if ( midair || cvar("k_instagib") )
	{
		traceline( PASSVEC3(targ->s.v.origin),
				targ->s.v.origin[0], 
				targ->s.v.origin[1], 
				targ->s.v.origin[2] - 2048,
				true, targ );

		playerheight = targ->s.v.absmin[2] - g_globalvars.trace_endpos[2] + ( cvar("k_instagib") ? 1 : 0 );
	}

	// get some data before apply damage in mid air mode
	if ( midair )
	{
		inwater = ( ((int)targ->s.v.flags & FL_INWATER) && targ->s.v.waterlevel > 1 );

		if ( streq( inflictor->s.v.classname, "rocket" ))
		{
			midheight = targ->s.v.origin[2] - inflictor->s.v.oldorigin[2];
			if ( midheight <= 190 )
				midheight = 0;
		}

		if ( playerheight < 45 )
		{
			lowheight = true;
		}
		else
		{
			damage *= ( 1 + ( playerheight - 45 ) / 64 );
			lowheight = false;
		}

		rl_dmg = ( targ->ct == ctPlayer && dtRL == targ->deathtype );

		if ( !rl_dmg ) {
			// damage types which ignore "lowheight"
			do_dmg =   targ->ct != ctPlayer				// always do damage to non player, secret doors etc...
				 	|| dtAXE == targ->deathtype			// always do axe damage
				 	|| dtWATER_DMG == targ->deathtype	// always do water damage
				 	|| dtLAVA_DMG  == targ->deathtype	// always do lava damage
				 	|| dtSLIME_DMG == targ->deathtype	// always do slime damage
				 	|| dtTELE1 == targ->deathtype	// always do tele damage
				 	|| dtTELE2 == targ->deathtype	// always do tele damage
				 	|| dtTELE3 == targ->deathtype	// always do tele damage
					|| dtSUICIDE == targ->deathtype; // do suicide damage anyway
		}
	}

	non_hdp_damage = damage; // save damage before handicap apply for kickback calculation

	// #handicap#
	if ( attacker != targ ) // attack no self
	if ( attacker->ct == ctPlayer && targ->ct == ctPlayer ) // player vs player
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

	save = newceil( targ->s.v.armortype * damage );

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

	take = newceil( damage - save );

	// mid air damage modificators
	if ( midair )
	{
		if ( lowheight && !inwater && rl_dmg )
			take = 0; // no rl dmg in such case

		if ( !rl_dmg && !do_dmg )
			take = 0; // unknown damage for midair, so do not damage

		if ( rl_dmg && targ == attacker )
			take = 0; // no self rl damage
	}

	// instagib damage modificators
	if ( cvar("k_instagib") )
	{
		if ( inflictor->ct == ctPlayer )
			take = 5000;

		if ( attacker == targ )
			take = 0;
	}

	take = max(0, take); // avoid negative take, if any

	if ( cvar("k_dmgfrags") )
	{
		// damage dealt _not_ capped by victim's health
		dmg_dealt += take;
	}
	else
	{
		// damage dealt capped by victim's health
		dmg_dealt += bound( 0, take, targ->s.v.health );
	}

	// add to the damage total for clients, which will be sent as a single
	// message at the end of the frame
	// FIXME: remove after combining shotgun blasts?
	if ( ( int ) targ->s.v.flags & FL_CLIENT )
	{
		targ->s.v.dmg_take += take;
		targ->s.v.dmg_save += save;
		targ->s.v.dmg_inflictor = EDICT_TO_PROG( inflictor );
	}

	if ( save )
	{
		if ( streq( inflictor->s.v.classname, "worldspawn" ) || strnull( attacker->s.v.classname ) )
			attackername = "world";
		else
			attackername = attacker->s.v.classname;

		log_printf( "\t\t\t<event time=\"%f\" tag=\"dmg\" at=\"%s\" "
					"tg=\"%s\" ty=\"%s\" q=\"%d\" s=\"%d\" val=\"%d\" ab=\"1\"/>\n",
					g_globalvars.time - match_start_time,
					attackername,
					targ->s.v.netname,
					dmg_type[ (int)bound(0, targ->deathtype, dmg_type_cnt-1) ],
					(int)(attacker->super_damage_finished > g_globalvars.time ? 1 : 0 ),
					dmg_is_splash,
					(int)save );
	}

	// figure momentum add
	if ( inflictor != world && targ->s.v.movetype == MOVETYPE_WALK )
	{
		float nailkick;

		for ( i = 0; i < 3; i++ )
			dir[i] = targ->s.v.origin[i] - ( inflictor->s.v.absmin[i] + inflictor->s.v.absmax[i] ) * 0.5;

		VectorNormalize( dir );

		dir[2] = ((dtLG_DIS_SELF == targ->deathtype || dtLG_DIS == targ->deathtype) && dir[2] < 0) ? -dir[2] : dir[2];

		if ( midair && non_hdp_damage < 60 && attacker != targ ) {
			c1 = 11;
			c2 = 6;
		}

		// Jawnmode: nails increases kickback
		// - Molgrum
		if ( k_jawnmode && streq( inflictor->s.v.classname, "spike" ) )
			nailkick = 1.2;
		else
			nailkick = 1.0;

		for ( i = 0; i < 3; i++ ) 
			targ->s.v.velocity[i] += dir[i] * non_hdp_damage * c1 * nailkick;

		if ( midair && lowheight )
			targ->s.v.velocity[2] += dir[2] * non_hdp_damage * c2 * nailkick; // only for z component
	}

	// team play damage avoidance
	//ZOID 12-13-96: self.team doesn't work in QW.  Use keys
   	attackerteam = getteam( attacker );
	targteam = getteam( targ );

	if ( match_in_progress == 2 && (int)cvar("k_dmgfrags") )
	{
		if ( attacker->ct == ctPlayer && targ->ct == ctPlayer && attacker != targ )
		{
			if ( isDuel() || isFFA() || strneq(attackerteam, targteam) )
			{
				int dmg_frags;
				attacker->ps.dmg_frags += dmg_dealt; // add dealt
				dmg_frags = attacker->ps.dmg_frags / 100; // 1 frag = 100 damage
				attacker->s.v.frags = (int)(attacker->s.v.frags + dmg_frags);
				attacker->ps.dmg_frags -= dmg_frags * 100;
			}
		}
	}

	// ignore this check for suicide damage
	if ( dtSUICIDE != targ->deathtype )
	{
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

		// teamplay == 1 don't damage self and mates (armor affected anyway)
		if ( tp_num() == 1
		 	&& !strnull( attackerteam )
		 	&& streq( targteam, attackerteam )
		 	&& attacker->ct == ctPlayer
		 	&& strneq( inflictor->s.v.classname, "door" )
		 	&& strneq( inflictor->s.v.classname, "teledeath" ) // do telefrag damage in tp
	   	)
			return;

		// teamplay == 3 don't damage mates, do damage to self (armor affected anyway)
		if ( tp_num() == 3
			 && !strnull( attackerteam )
		 	&& streq( targteam, attackerteam )
		 	&& attacker->ct == ctPlayer
		 	&& strneq( inflictor->s.v.classname, "door" )
		 	&& strneq( inflictor->s.v.classname, "teledeath" ) // do telefrag damage in tp
		 	&& targ != attacker
	   	)
			return;
	}

	// do the damage

	if (    match_in_progress == 2
		 || dtSUICIDE == targ->deathtype // do suicide damage anyway
		 || streq( inflictor->s.v.classname, "teledeath" ) 
		 || ( k_practice && targ->ct != ctPlayer ) // #practice mode#
	   ) 
	{
		targ->s.v.health -= take;

		if ( take )
		{
			if ( streq( inflictor->s.v.classname, "worldspawn" ) || strnull( attacker->s.v.classname ) )
				attackername = "world";
			else
				attackername = attacker->s.v.netname;

			log_printf( "\t\t\t<event time=\"%f\" tag=\"dmg\" at=\"%s\" tg=\"%s\" ty=\"%s\" "
						"q=\"%d\" s=\"%d\" val=\"%d\" ab=\"0\"/>\n",
						g_globalvars.time - match_start_time,
						attackername,
						targ->s.v.netname,
						dmg_type[ (int)bound(0, targ->deathtype, dmg_type_cnt-1) ],
						(int)(attacker->super_damage_finished > g_globalvars.time ? 1 : 0 ),
						dmg_is_splash,
						(int)take );
		}

		if ( !targ->s.v.health || dtSUICIDE == targ->deathtype )
			targ->s.v.health = -1; // qqshka, no zero health, heh, imo less bugs after this
	}

	if ( attacker->ct == ctPlayer && targ->ct == ctPlayer && attacker != targ )
	{
		if ( take || save )
		{
			if ( targ->deathtype == dtRL )
				attacker->ps.wpn[wpRL].rhits++;
    
			if ( targ->deathtype == dtGL )
				attacker->ps.wpn[wpGL].rhits++;
		}
	}

	// show damage in sbar
	if ( match_in_progress != 2 )
	{
		if ( !midair || ( (int)targ->s.v.flags & FL_ONGROUND ) )
		{
			targ->s.v.currentammo = 1000 + Q_rint(damage);
			if (attacker != targ)
				attacker->s.v.health = 1000 + Q_rint(damage);
		}
	}

	// update damage stats like: give/taked/team damage
	if ( attacker->ct == ctPlayer && targ->ct == ctPlayer && attacker != targ )
	{
		if ( streq(attackerteam, targteam) && !isDuel() && !isFFA() )
		{
			attacker->ps.dmg_team += dmg_dealt;
		}
		else 
		{
			if ( dtRL == targ->deathtype )
				attacker->ps.dmg_g_rl += dmg_dealt;

			attacker->ps.dmg_g += dmg_dealt;
			targ->ps.dmg_t     += dmg_dealt;
		}
	}

	// mid air bonuses
	if ( midair && match_in_progress == 2 && midheight > 190 && attacker != targ )
		MidairDamageBonus(attacker, midheight);

 	// if targed killed, do appropriate action and return
	if ( ISDEAD( targ ) )
	{
		Killed( targ, attacker, inflictor );
		return;
	}

	// react to the damage - call pain function
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

						dmg_is_splash = 1; // mark damage as splash

						if ( cvar("k_instagib") ) // in instagib splash applied to inflictor only, for coil jump
						{
							if ( head == attacker )
								T_Damage( head, inflictor, attacker, points );
						}
						else
						{
							T_Damage( head, inflictor, attacker, points );
						}

						dmg_is_splash = 0; // unmark splash
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
