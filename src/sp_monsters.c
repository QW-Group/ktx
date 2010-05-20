/*  Copyright (C) 1996-1997  Id Software, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    See file, 'COPYING', for details.
*/

#include "g_local.h"

/* ALL MONSTERS SHOULD BE 1 0 0 IN COLOR */

// name =[framenum,	nexttime, nextthink] {code}
// expands to:
// name ()
// {
//		self.frame=framenum;
//		self.nextthink = time + nexttime;
//		self.think = nextthink
//		<code>
// }

//============================================================================

void MonsterDropPowerups()
{
	int i;

	if ( skill < 3 )
		return; // skill 3 or more required

	if ( !Get_Powerups() )
		return;

	if ( g_random() > cvar("k_nightmare_pu_droprate") )
		return;

	i = i_rnd( 0, 5 );

	switch ( i )
	{
		case 0:

		if ( /* cvar( "dp" ) && */ cvar("k_pow_p") )
			DropPowerup( 30, IT_INVULNERABILITY );

		break;

		case 1:

		if ( /* cvar( "dr" ) && */ cvar("k_pow_r") )
			DropPowerup( 30, IT_INVISIBILITY );

		break;

		// more chances for quad compared to pent and ring

		default:

		if ( /* cvar( "dq" ) && */ cvar("k_pow_q") )
			DropPowerup( 30, IT_QUAD );

		break;
	}
}

//============================================================================

/*
================
monster_use

Using a monster makes it angry at the current activator
================
*/
void monster_use ()
{
	if ( self->s.v.enemy )
		return;

	if ( ISDEAD( self ) )
		return;

	if ( activator->ct != ctPlayer )
		return;
		
	if ( (int)activator->s.v.items & IT_INVISIBILITY )
		return;

	if ( (int)activator->s.v.flags & FL_NOTARGET )
		return;

	// delay reaction so if the monster is teleported, its sound is still heard
	self->s.v.enemy = EDICT_TO_PROG( activator );
	self->s.v.nextthink = g_globalvars.time + 0.1;
	self->s.v.think = ( func_t ) FoundTarget;
}

/*
================
monster_death_use

When a mosnter dies, it fires all of its targets with the current
enemy as activator.
================
*/
void monster_death_use ()
{
	if ( !( ( int )self->s.v.flags & FL_MONSTER ) )
		return; // not a monster

	if ( cvar("k_nightmare_pu"))
		MonsterDropPowerups();

	// fall to ground
	self->s.v.flags = (int)self->s.v.flags & ~( FL_FLY | FL_SWIM);

	if ( !self->s.v.target )
		return;

	activator = PROG_TO_EDICT( self->s.v.enemy );
	SUB_UseTargets ();
}

//============================================================================

void check_monsters_respawn( void )
{
	gedict_t *p, *oself;

	if ( deathmatch )
		return; // no need in dm

	if ( skill < 3 )
		return; // skill 3 or more required

	if ( cvar( "k_monster_spawn_time") <= 0 )
		return;

    for( p = world; ( p = nextent( p ) ); )
    {
		if ( !( (int)p->s.v.flags & FL_MONSTER ) )
			continue; // not a monster

		if ( ISLIVE( p ) )
			continue; // not dead

		if ( !p->th_respawn )
			continue; // respawn function is not set

		if ( p->monster_desired_spawn_time > g_globalvars.time )
			continue; // time is not come yet

		oself = self;	// save
		self = p;		// WARNING !!!
		
		self->th_respawn();

		self = oself;	// restore
    }
}

//============================================================================

typedef enum
{
	mtWalk,
	mtFly,
	mtSwim,
} monsterType_t;

void monster_start_go( monsterType_t mt )
{
	vec3_t tmpv;

	self->s.v.enemy = EDICT_TO_PROG( world );
	self->s.v.goalentity = EDICT_TO_PROG( world );
	self->oldenemy = NULL;
	self->lefty = 0;
	self->search_time = 0;
	self->attack_state = 0;

	if ( mt == mtWalk )
	{
		self->s.v.origin[2] += 1;	// raise off floor a bit
		droptofloor( self );
	}

	VectorSet(tmpv, 0, 1, 0);
	self->s.v.ideal_yaw = DotProduct( self->s.v.angles, tmpv );
	if ( !self->s.v.yaw_speed )
	{
		self->s.v.yaw_speed = ( mt == mtWalk ? 20 : 10 );
	}

	VectorSet( self->s.v.view_ofs, 0, 0, mt == mtSwim ? 10 : 25 );
	self->s.v.use = ( func_t ) monster_use;

	self->s.v.solid = SOLID_SLIDEBOX;
	self->s.v.takedamage = DAMAGE_AIM;

	if ( !walkmove( self, 0, 0 ) )
	{
//		G_bprint( 2, "monster in wall at: %.1f %.1f %.1f\n", self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] );
		G_cprint( "monster in wall at: %.1f %.1f %.1f\n", self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] );

		self->s.v.model = ""; // turn off model
		self->s.v.solid = SOLID_NOT;
		self->s.v.takedamage = DAMAGE_NO;

		self->s.v.nextthink = g_globalvars.time + 5; // reshedule

		setorigin( self, PASSVEC3( self->s.v.oldorigin ) );

		return;
	}

	if ( !strnull( self->mdl ) )
		setmodel( self, self->mdl );	// restore original model

	// seems we respawning monster again, use some funky effects
	if ( self->dead_time )
	{
		play_teleport( self );
		spawn_tfog( self->s.v.origin );
	}

	if ( self->s.v.target )
	{
		self->movetarget = find( world, FOFS( s.v.targetname ), self->s.v.target );
		if ( !self->movetarget ) // NOTE: this is a damn difference with qc
			self->movetarget = world;
		self->s.v.goalentity = EDICT_TO_PROG( self->movetarget );

		VectorSubtract( PROG_TO_EDICT( self->s.v.goalentity )->s.v.origin, self->s.v.origin, tmpv );
		self->s.v.ideal_yaw = vectoyaw( tmpv );

		if ( !self->movetarget || self->movetarget == world )
		{
//			G_bprint( 2, "monster can't find target at: %.1f %.1f %.1f\n", self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] );
			G_cprint( "monster can't find target at: %.1f %.1f %.1f\n", self->s.v.origin[0], self->s.v.origin[1], self->s.v.origin[2] );
		}
		// this used to be an objerror

		if ( self->movetarget && streq( self->movetarget->s.v.classname, "path_corner" ) )
		{
			if ( self->th_walk )
				self->th_walk();
		}
		else
		{
			self->pausetime = 99999999;
			if ( self->th_stand )
				self->th_stand();
		}
	}
	else
	{
		self->pausetime = 99999999;
		if ( self->th_stand )
			self->th_stand();
	}

	// spread think times so they don't all happen at same time
	self->s.v.nextthink = g_globalvars.time + g_random() * 0.5;
}

void walkmonster_start_go()
{
	monster_start_go( mtWalk );
}

void flymonster_start_go()
{
	monster_start_go( mtFly );
}

void swimmonster_start_go()
{
	monster_start_go( mtSwim );
}

static void common_monster_start( char *model, int flags )
{
	self->s.v.flags = (int)self->s.v.flags & ~FL_ONGROUND;

	self->s.v.movetype = MOVETYPE_STEP;

	g_globalvars.total_monsters += 1; // bump monsters total count

	if ( (int)self->s.v.flags & FL_MONSTER )
	{
		// FL_MONSTER set, so we spawn monster non first time, it should be nightmare mode...

		self->s.v.model = ""; // turn off model
		self->s.v.solid = SOLID_NOT;
		setorigin( self, PASSVEC3( self->s.v.oldorigin ) ); // move monster back to his first spawn origin
		VectorCopy( self->oldangles, self->s.v.angles ); // restore angles
	}
	else
	{
		// FL_MONSTER not set, should be first monster spawn

		setmodel( self, model ); // set model
		self->mdl = self->s.v.model; // save model
		VectorCopy( self->s.v.origin, self->s.v.oldorigin ); // save first spawn origin
		VectorCopy( self->s.v.angles, self->oldangles ); // save angles
	}

	// always set FL_MONSTER and possibily add some additional flags
	self->s.v.flags = (int)self->s.v.flags | FL_MONSTER | flags;
}

void walkmonster_start( char *model )
{
	common_monster_start( model, 0 );

	// delay drop to floor to make sure all doors have been spawned
	// spread think times so they don't all happen at same time
	self->s.v.nextthink = g_globalvars.time + 0.1 + g_random() * 0.5;
	self->s.v.think = ( func_t ) walkmonster_start_go;
}

void flymonster_start( char *model )
{
	// set FL_FLY early so that we're not affected by gravity
	common_monster_start( model, FL_FLY );

	// spread think times so they don't all happen at same time
	self->s.v.nextthink = g_globalvars.time + 0.1 + g_random() * 0.5;
	self->s.v.think = ( func_t ) flymonster_start_go;
}

void swimmonster_start( char *model )
{
	// set FL_SWIM early so that we're not affected by gravity
	common_monster_start( model, FL_SWIM );

	// spread think times so they don't all happen at same time
	self->s.v.nextthink = g_globalvars.time + 0.1 + g_random() * 0.5;
	self->s.v.think = (func_t) swimmonster_start_go;
}
