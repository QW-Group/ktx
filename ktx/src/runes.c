/*
 *  $Id: runes.c,v 1.6 2006/05/16 04:53:41 ult_ Exp $
 */

#include "g_local.h"

void RegenLostRot();
void RuneRespawn();
void RuneTouch();
void RuneResetOwner();
gedict_t* SelectRuneSpawnPoint();

void DoDropRune(int rune)
{
	gedict_t *item;

	item = spawn();
	setorigin( item, PASSVEC3( self->s.v.origin ) );
	item->s.v.classname = "rune";
	item->ctf_flag = rune;
	item->s.v.velocity[0] = i_rnd( -100, 100 );
	item->s.v.velocity[1] = i_rnd( -100, 100 );
	item->s.v.velocity[2] = 400;
	item->s.v.flags = FL_ITEM;
	item->s.v.solid = SOLID_TRIGGER;
	item->s.v.movetype = MOVETYPE_TOSS;

	if ( rune & CTF_RUNE_RES )
		setmodel( item, "progs/end1.mdl" );
	else if ( rune & CTF_RUNE_STR )
		setmodel( item, "progs/end2.mdl" );
	else if ( rune & CTF_RUNE_HST )
 		setmodel( item, "progs/end3.mdl" );
	else if ( rune & CTF_RUNE_RGN )
		setmodel( item, "progs/end4.mdl" );

	setsize( item, -16, -16, 0, 16, 16, 56 );
	item->s.v.touch = (func_t) RuneTouch;
	item->s.v.nextthink = g_globalvars.time + 90;
	item->s.v.think = (func_t) RuneRespawn;
}

void DoTossRune( int rune )
{
	gedict_t *item;

	item = spawn();
	item->ctf_flag = rune;
	item->s.v.classname = "rune";

	setorigin( item, PASSVEC3( self->s.v.origin ) );
	item->s.v.origin[2] += 24; 
	makevectors( self->s.v.angles );
	aim( item->s.v.velocity );
	VectorScale( item->s.v.velocity, 800, item->s.v.velocity);
	vectoangles( item->s.v.velocity, item->s.v.angles );
	item->s.v.velocity[2] = -item->s.v.velocity[2];
	item->s.v.flags = FL_ITEM;
	item->s.v.solid = SOLID_TRIGGER;
	item->s.v.movetype = MOVETYPE_TOSS;
  
	if ( rune & CTF_RUNE_RES )
		setmodel( item, "progs/end1.mdl" );
	else if ( rune & CTF_RUNE_STR )
		setmodel( item, "progs/end2.mdl" );
	else if ( rune & CTF_RUNE_HST )
		setmodel( item, "progs/end3.mdl" );
	else if ( rune & CTF_RUNE_RGN )
		setmodel( item, "progs/end4.mdl" );
 
	setsize( item, -16, -16, 0, 16, 16, 56 );
	item->s.v.owner = EDICT_TO_PROG( self );
	item->s.v.touch = (func_t) RuneTouch;
	item->s.v.nextthink = g_globalvars.time + 0.75;
	item->s.v.think = (func_t) RuneResetOwner;
}

void DropRune()
{
	if ( self->ctf_flag & CTF_RUNE_RES ) {
		DoDropRune( CTF_RUNE_RES );
		self->ps.res_time += g_globalvars.time - self->rune_pickup_time;
	}
	if ( self->ctf_flag & CTF_RUNE_STR ) {
		DoDropRune( CTF_RUNE_STR );
		self->ps.str_time += g_globalvars.time - self->rune_pickup_time;
	}
	if ( self->ctf_flag & CTF_RUNE_HST ) {
		DoDropRune( CTF_RUNE_HST );
		self->ps.hst_time += g_globalvars.time - self->rune_pickup_time;
	}
	if ( self->ctf_flag & CTF_RUNE_RGN ) {
		DoDropRune( CTF_RUNE_RGN );
		self->ps.rgn_time += g_globalvars.time - self->rune_pickup_time;
	}

	self->ctf_flag -= ( self->ctf_flag & (CTF_RUNE_MASK) );
	// self->s.v.items -= ( (int) self->s.v.items & (CTF_RUNE_MASK) );
}

void TossRune()
{
	if ( self->ctf_flag & CTF_RUNE_RES ) 
	{
		DoTossRune( CTF_RUNE_RES );
		self->ps.res_time += g_globalvars.time - self->rune_pickup_time;
	}
	if ( self->ctf_flag & CTF_RUNE_STR ) 
	{
		DoTossRune( CTF_RUNE_STR );
		self->ps.str_time += g_globalvars.time - self->rune_pickup_time;
	}
	if ( self->ctf_flag & CTF_RUNE_HST )
	{
		DoTossRune( CTF_RUNE_HST );
		self->ps.hst_time += g_globalvars.time - self->rune_pickup_time;
		self->maxspeed = cvar("sv_maxspeed");
	}

	if ( self->ctf_flag & CTF_RUNE_RGN )
	{
		gedict_t *regenrot = spawn();
		DoTossRune( CTF_RUNE_RGN );
		self->ps.rgn_time += g_globalvars.time - self->rune_pickup_time;
		regenrot->s.v.nextthink = g_globalvars.time + 5;
		regenrot->s.v.think = (func_t) RegenLostRot;
		regenrot->s.v.owner = EDICT_TO_PROG( self );
	}   

	self->ctf_flag -= ( self->ctf_flag & (CTF_RUNE_MASK) );
	//self->s.v.items -= ( (int) self->s.v.items & (CTF_RUNE_MASK) );
}

void RegenLostRot()
{
	other = PROG_TO_EDICT( self->s.v.owner );
	if ( other->s.v.health < 101 || 
		other->ctf_flag & CTF_RUNE_RGN || 
		(int) other->s.v.items & IT_SUPERHEALTH )
	{
		ent_remove( self );
		return;
	}
	other->s.v.health--;
	self->s.v.nextthink = g_globalvars.time + 1;
}

void RuneResetOwner()
{
	self->s.v.owner     = EDICT_TO_PROG( self );
	self->s.v.think     = (func_t) RuneRespawn;
	self->s.v.nextthink = g_globalvars.time + 90;
}

void RuneRespawn()
{  
	int rune = self->ctf_flag;
	ent_remove( self );
	self = SelectRuneSpawnPoint();
	DoDropRune( rune );
}

void RuneTouch()
{
	if ( !streq(other->s.v.classname, "player") )
		return;

	if ( other->s.v.health < 1 )
		return;
 
	if ( match_in_progress != 2 )
		return;

	if ( other == PROG_TO_EDICT ( self->s.v.owner ) )
		return;
  
	self->s.v.nextthink = g_globalvars.time + 90;

	if ( other->ctf_flag & CTF_RUNE_MASK ) 
	{
		if ( g_globalvars.time > other->rune_notify_time )
		{
			other->rune_notify_time = g_globalvars.time + 10;   
			G_sprint( other, 1, "You already have a rune. Use \"%s\" to drop\n", redtext("tossrune") );
		}
		return;
	}
 
	other->ctf_flag |= self->ctf_flag;
	other->rune_pickup_time = g_globalvars.time;
 
	if ( other->ctf_flag & CTF_RUNE_RES )
	{
		// other->s.v.items = (int) other->s.v.items | IT_SIGIL1;
		G_sprint( other, 2, "You got the %s rune\n", redtext("resistance") );
	}

	if ( other->ctf_flag & CTF_RUNE_STR )
	{
		// other->s.v.items = (int) other->s.v.items | IT_SIGIL2;
		G_sprint( other, 2, "You got the %s rune\n", redtext("strength") );
	}

	if ( other->ctf_flag & CTF_RUNE_HST )
	{
		other->maxspeed *= 1.25;
		// other->s.v.items = (int) other->s.v.items | CTF_RUNE_HST;
		G_sprint( other, 2, "You got the %s rune\n", redtext("haste") );
  	}

	if ( other->ctf_flag & CTF_RUNE_RGN )
	{
		// other->s.v.items = (int) other->s.v.items | CTF_RUNE_RGN;
		G_sprint( other, 2, "You got the %s rune\n", redtext("regeneration") );
	}

	sound( other, CHAN_ITEM, "weapons/lock4.wav", 1, ATTN_NORM );
	stuffcmd( other, "bf\n" );
	ent_remove( self );
}

gedict_t* SelectSpawnPoint();
gedict_t* SelectRuneSpawnPoint()
{
	gedict_t *runespawn;
  
	// we'll just use the player spawn point selector for runes as well
	runespawn = SelectSpawnPoint( "info_player_deathmatch" );
	return runespawn;
}

void SpawnRunes()
{
	gedict_t *oself, *e;
	oself = self;

	e = find( world, FOFCLSN, "rune" );
	while ( e ) {
		ent_remove( e );
		e = find( world, FOFCLSN, "rune" );
	}
		
	self = SelectRuneSpawnPoint();
	DoDropRune( CTF_RUNE_RES );
	self = SelectRuneSpawnPoint();
	DoDropRune( CTF_RUNE_STR );
	self =  SelectRuneSpawnPoint();
	DoDropRune( CTF_RUNE_HST );
	self = SelectRuneSpawnPoint();
	DoDropRune( CTF_RUNE_RGN );
  
	self = oself;
}

void ResistanceSound( gedict_t *player )
{
	if ( player->ctf_flag & CTF_RUNE_RES )
	{
		if ( player->rune_sound_time < g_globalvars.time )
		{
			player->rune_sound_time = g_globalvars.time + 1;
			sound( player, CHAN_BODY, "rune/rune1.wav", 1, ATTN_NORM );
		}
	}  
}

void HasteSound( gedict_t *player )
{
	if ( player->ctf_flag & CTF_RUNE_HST )
	{
		if ( player->rune_sound_time < g_globalvars.time )
		{	  
			player->rune_sound_time = g_globalvars.time + 1;
			sound( player, CHAN_BODY, "rune/rune3.wav", 1, ATTN_NORM );
		}
	}
}

void RegenerationSound( gedict_t *player )
{
	if ( player->ctf_flag & CTF_RUNE_RGN )
	{
		if ( player->rune_sound_time < g_globalvars.time )
		{
			player->rune_sound_time = g_globalvars.time + 1;
			sound( player, CHAN_BODY, "rune/rune4.wav", 1, ATTN_NORM );
		}
	}
}

