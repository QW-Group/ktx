/*
 *  $Id$
 */

#include "g_local.h"

void RegenLostRot();
void RuneRespawn();
void RuneTouch();
void RuneResetOwner();
gedict_t* SelectRuneSpawnPoint();

void DoDropRune(int rune, qboolean s)
{
	gedict_t *item;

	cl_refresh_plus_scores( self );

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

	// qqshka, add spawn sound to rune if rune respawned, not for player dropped from corpse rune
	if ( s )
		sound( item, CHAN_VOICE, "items/itembk2.wav", 1, ATTN_NORM );	// play respawn sound
}

void DoTossRune( int rune )
{
	gedict_t *item;

	cl_refresh_plus_scores( self );

	item = spawn();
	item->ctf_flag = rune;
	item->s.v.classname = "rune";

	setorigin( item, PASSVEC3( self->s.v.origin ) );
	item->s.v.origin[2] += 24; 
	trap_makevectors( self->s.v.angles );
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
		DoDropRune( CTF_RUNE_RES, false );
		self->ps.res_time += g_globalvars.time - self->rune_pickup_time;
	}
	if ( self->ctf_flag & CTF_RUNE_STR ) {
		DoDropRune( CTF_RUNE_STR, false );
		self->ps.str_time += g_globalvars.time - self->rune_pickup_time;
	}
	if ( self->ctf_flag & CTF_RUNE_HST ) {
		DoDropRune( CTF_RUNE_HST, false );
		self->ps.hst_time += g_globalvars.time - self->rune_pickup_time;
	}
	if ( self->ctf_flag & CTF_RUNE_RGN ) {
		DoDropRune( CTF_RUNE_RGN, false );
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
	DoDropRune( rune, true );
}

void RuneTouch()
{
	if ( other->ct != ctPlayer )
		return;

	if ( ISDEAD( other ) )
		return;
 
	if( !k_practice )
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

	cl_refresh_plus_scores( other );
 
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

// spawn/remove runes
void SpawnRunes( qboolean yes )
{
	gedict_t *oself, *e;

	for ( e = world; (e = find( e, FOFCLSN, "rune")); )
		ent_remove( e );

	if ( !yes )
		return;

	oself = self;
		
	self = SelectRuneSpawnPoint();
	DoDropRune( CTF_RUNE_RES, true );
	self = SelectRuneSpawnPoint();
	DoDropRune( CTF_RUNE_STR, true );
	self =  SelectRuneSpawnPoint();
	DoDropRune( CTF_RUNE_HST, true );
	self = SelectRuneSpawnPoint();
	DoDropRune( CTF_RUNE_RGN, true );
  
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

void CheckStuffRune()
{

	char *rune = "";
	
	if ( cvar("k_instagib") ) {
		if ( self->i_agmr ) {
			self->items2 = ( int ) self->items2 | (CTF_RUNE_RES << 5);
			return;
		}
	}

	if ( !isCTF() ) {
		self->items2 = 0; // no runes/sigils in HUD

		if ( self->last_rune && iKey(self, "runes") ) {
			self->last_rune = NULL;
			stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "set rune \"\"\n");
		}

		return;
	}

	self->items2 = (self->ctf_flag & CTF_RUNE_MASK) << 5;

	if ( !iKey(self, "runes") )
		return;

	if ( self->ctf_flag & CTF_RUNE_RES )
		rune = "res";
	else if ( self->ctf_flag & CTF_RUNE_STR )
		rune = "str";
	else if ( self->ctf_flag & CTF_RUNE_HST )
		rune = "hst";
	else if ( self->ctf_flag & CTF_RUNE_RGN )
		rune = "rgn";
	else
		rune = "";

	if ( !self->last_rune || strneq(rune, self->last_rune) ) {
		self->last_rune = rune;
		stuffcmd_flags(self, STUFFCMD_IGNOREINDEMO, "set rune \"%s\"\n", rune);
	}
}

