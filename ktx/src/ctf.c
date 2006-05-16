/*
 *  $Id: ctf.c,v 1.12 2006/05/16 04:53:41 ult_ Exp $
 */

#include "g_local.h"

// CTF todo:
// . option to disable pents, but leave quads on?
// . some sort of workaround to be able to add ctf specific armors\weapons\ammo but still be dynamic
// . ctf all id maps

// Changes from purectf:
// . Team damage on by default
// . No longer spawn with 50 green armor
// . Close range grapple exploit removed
// . Grapple speed stays consistent regardless of sv_maxspeed
// . Status bar removed
// . Clients can use key icons to display flag status same way as old ctf status bar
// . Physics are the same as those found on dm servers (you can accel jump, etc)
// . Spawns are now random (except initial spawn in your base) with default spawnsafety mode
// . Untouched runes respawn at 90 seconds instead of 120 (happens if in lava or falls through level, etc)
// . Defending the flag is now two bonus points rather than one

// Server config changes:
// . add 64 to k_allowed_free_modes to enable ctf, 127 now enables all modes
// . set k_mode 4 if you want server to default to ctf mode
 
#define FLAG_AT_BASE   0
#define FLAG_CARRIED   1
#define FLAG_DROPPED   2
#define FLAG_RETURNED  3 // here only 200ms before going back to FLAG_AT_BASE

#define FLAG_RETURN_TIME     30
#define CARRIER_ASSIST_TIME  6
#define RETURN_ASSIST_TIME   4
#define RETURN_BONUS         1
#define CAPTURE_BONUS        15
#define TEAM_BONUS           10
#define CARRIER_ASSIST_BONUS 2
#define RETURN_ASSIST_BONUS  1

void DropFlag( gedict_t *flag );
void PlaceFlag();
void FlagThink();
void FlagTouch();
void SP_item_flag_team1();
void SP_item_flag_team2();

// Allows us to add flags (or other items) to dm maps when ctfing without actually changing bsp
void G_CallSpawn( gedict_t *ent );
void SpawnCTFItem( char* classname, float x, float y, float z, float angle )
{
	gedict_t *item = spawn();

	item->s.v.classname = classname;
	setorigin( item, x, y, z );
	item->s.v.angles[0] = 0;
	item->s.v.angles[1] = angle;
 
	G_CallSpawn( item );
}

void spawn_item_flag()
{
	if ( k_ctf_custom_models )
		setmodel( self, "progs/flag.mdl" );

	self->s.v.noise = "misc/flagtk.wav";
	self->s.v.noise1 = "doors/runetry.wav";
	setsize( self, -16, -16, 0, 16, 16, 74);
	self->mdl = self->s.v.model;
	self->s.v.flags = FL_ITEM;
	self->s.v.solid = SOLID_TRIGGER;
	self->s.v.movetype = MOVETYPE_TOSS;
	SetVector( self->s.v.velocity, 0, 0, 0 );
	self->s.v.origin[2] += 6;
	self->s.v.think = (func_t) FlagThink;
	self->s.v.touch = (func_t) FlagTouch;
	self->s.v.nextthink = g_globalvars.time + 0.1;
	self->cnt = FLAG_AT_BASE;
	self->cnt2 = 0.0;
	VectorCopy( self->s.v.angles, self->mangle );
	self->s.v.effects = (int) self->s.v.effects | EF_DIMLIGHT;

	if ( !droptofloor( self ) )
		ent_remove( self );
	else
    	VectorCopy( self->s.v.origin, self->s.v.oldorigin );

	if ( !isCTF() )
	{
		setmodel( self, "" );
		self->s.v.touch = (func_t) SUB_Null;
	}
}

void SP_item_flag_team1()
{
	self->k_teamnumber = 1;
	self->s.v.items = IT_KEY2;
	self->s.v.skin = 0;
	self->s.v.effects = (int) self->s.v.effects | EF_RED;

	if ( !k_ctf_custom_models )
		setmodel( self, "progs/w_g_key.mdl" );

	spawn_item_flag();
}

void SP_item_flag_team2()
{
	self->k_teamnumber = 2;
	self->s.v.items = IT_KEY1;
	self->s.v.skin = 1;
	self->s.v.effects = (int) self->s.v.effects | EF_BLUE;

	if ( !k_ctf_custom_models )
		setmodel( self, "progs/w_s_key.mdl" );

	spawn_item_flag();
}

// would love to know what a ctf wall is :O!
void SP_func_ctf_wall()
{
	SetVector( self->s.v.angles, 0, 0, 0 );
	self->s.v.movetype = MOVETYPE_PUSH;
	self->s.v.solid = SOLID_BSP;
	setmodel( self, self->s.v.model ); 
}

void RegenFlag( gedict_t *flag )
{
	flag->s.v.movetype = MOVETYPE_TOSS;
	flag->s.v.solid = SOLID_TRIGGER;
	setmodel( flag, flag->mdl );
	VectorCopy( flag->mangle, flag->s.v.angles );
	flag->cnt = FLAG_RETURNED;
	flag->cnt2 = 0.0;
	flag->s.v.owner = EDICT_TO_PROG( world );
	SetVector( flag->s.v.velocity, 0, 0, 0 );
	sound( flag, CHAN_VOICE, "items/itembk2.wav", 1, ATTN_NORM);
	flag->s.v.nextthink = g_globalvars.time + 0.2;
	flag->s.v.groundentity = EDICT_TO_PROG( world );
	flag->s.v.touch = (func_t) FlagTouch;
}

void RegenFlags()
{
	gedict_t * flag;
	flag = find( world, FOFCLSN, "item_flag_team1" );
	if ( flag )
		RegenFlag( flag );
	flag = find( world, FOFCLSN, "item_flag_team2" );
	if ( flag )
		RegenFlag( flag );
}

void FlagThink()
{
	if ( !isCTF() )
		return;

	self->s.v.nextthink = g_globalvars.time + 0.1;

	if (self->cnt == FLAG_AT_BASE)
		return;

	if (self->cnt == FLAG_DROPPED)
	{
		self->cnt2 += 0.1;
		if ( g_globalvars.time > self->super_time )
		{
			RegenFlag( self );
			G_bprint( 2, "The %s flag has been returned\n", redtext( ( (int) self->s.v.items & IT_KEY1) ? "BLUE" : "RED" ) );
		}
		return;
	}

	if (self->cnt == FLAG_RETURNED)
	{
		setorigin( self, PASSVEC3(self->s.v.oldorigin) );
		self->cnt = FLAG_AT_BASE;
		return;
	}
  
	self->cnt2 += 0.1;
}

void FlagTouch()
{
	gedict_t *p, *owner;

	if ( match_in_progress != 2 )
		return;

	if ( !streq( other->s.v.classname, "player" ) )
		return;

	if ( other->s.v.health < 1 )
		return;

	if ( self->cnt == FLAG_RETURNED )
		return;

	// touching their own flag
	if ((self->k_teamnumber == 1 && streq(getteam(other), "red")) ||
		(self->k_teamnumber == 2 && streq(getteam(other), "blue")) )
	{
		if ( self->cnt == FLAG_AT_BASE )
		{
			if ( other->ctf_flag & CTF_FLAG )
			{
				gedict_t *cflag = NULL;

				// capture
				other->ctf_flag -= ( (int) other->ctf_flag & CTF_FLAG );
				other->s.v.effects -= ( (int) other->s.v.effects & (EF_FLAG1 | EF_FLAG2) );
        
				if ( !other->super_damage_finished && !other->invincible_finished )
					other->s.v.effects -= ( (int) other->s.v.effects & EF_DIMLIGHT );

				sound( other, CHAN_VOICE, "misc/flagcap.wav", 1, ATTN_NONE);

				G_bprint( 2, other->s.v.netname );
				if ( self->k_teamnumber == 1 )
				{
					cflag = find( world, FOFCLSN, "item_flag_team2" );
					G_bprint( 2, " %s the %s flag!\n", redtext("captured"), redtext("BLUE") );
				}
				else
				{
					cflag = find( world, FOFCLSN, "item_flag_team1" );
					G_bprint( 2, " %s the %s flag!\n", redtext("captured"), redtext("RED") );
 				}

				if ( cflag ) 
					G_bprint( 2, "The capture took %.1f seconds\n", cflag->cnt2 );

				other->s.v.frags += CAPTURE_BONUS;
				other->ps.ctf_points += CAPTURE_BONUS;
				other->ps.caps++;
	    
				// loop through all players on team to give bonus
				p = find( world, FOFCLSN, "player" );
				while ( p )
				{
					p->s.v.items -= ( (int) p->s.v.items & (IT_KEY1 | IT_KEY2) );
					if ( streq(getteam(p), getteam(other)) )
					{
						if ( p->return_flag_time + RETURN_ASSIST_TIME > g_globalvars.time )
						{
							p->return_flag_time = -1;
							p->s.v.frags += RETURN_ASSIST_BONUS;
							p->ps.ctf_points += RETURN_ASSIST_BONUS;
							G_bprint( 2, "%s gets an assist for returning his flag!\n", p->s.v.netname );
						}
						if ( p->carrier_frag_time + CARRIER_ASSIST_TIME > g_globalvars.time )
						{
							p->carrier_frag_time = -1;
							p->s.v.frags += CARRIER_ASSIST_BONUS;
							p->s.v.frags += CARRIER_ASSIST_BONUS;
							G_bprint( 2, "%s gets an assist for fragging the flag carrier!\n", p->s.v.netname );
						}
	           
						if ( p != other ) 
						{
							p->s.v.frags += TEAM_BONUS;
							p->ps.ctf_points += TEAM_BONUS;
						}
					}
					else
						p->carrier_hurt_time = -1;
					p = find ( p, FOFCLSN, "player" );
				}

				RegenFlags();
				return;
			}
			return;
		}
		else if ( self->cnt == FLAG_DROPPED )
		{
			other->s.v.frags += RETURN_BONUS;
			other->ps.ctf_points += RETURN_BONUS;
			other->ps.returns++;
			other->return_flag_time = g_globalvars.time;
			sound (other, CHAN_ITEM, self->s.v.noise1, 1, ATTN_NORM);
			RegenFlag( self );

			p = find( world, FOFCLSN, "player" );
			while ( p ) {
				p->s.v.items -= ( (int) p->s.v.items & (int) self->s.v.items );
 				p = find ( p, FOFCLSN, "player" );
			}

			G_bprint( 2, other->s.v.netname);

			if ( self->k_teamnumber == 1)
				G_bprint( 2, " %s the %s flag!\n", redtext("returned"), redtext("RED") );
			else
				G_bprint( 2, " %s the %s flag!\n", redtext("returned"), redtext("BLUE") );
			return;
		}
	}

	// Pick up the flag
	sound( other, CHAN_ITEM, self->s.v.noise, 1, ATTN_NORM );
	other->ctf_flag |= CTF_FLAG;

	// give key icon to all players if a flag is taken
	p = find( world, FOFCLSN, "player");
	while ( p ) {	
		p->s.v.items = (int) p->s.v.items | (int) self->s.v.items;
 		p = find ( p, FOFCLSN, "player" );
	}

	self->cnt = FLAG_CARRIED;
	self->s.v.solid = SOLID_NOT;
	self->s.v.owner = EDICT_TO_PROG( other );

	owner = PROG_TO_EDICT( self->s.v.owner );
	owner->ps.pickups++;

	G_bprint( 2, other->s.v.netname );
	if ( streq(getteam(other), "red"))
	{
		G_bprint( 2, " %s the %s flag!\n", redtext("got"), redtext("BLUE") );
		owner->s.v.effects = (int) owner->s.v.effects | EF_FLAG2 | EF_DIMLIGHT;
	}
	else
	{
		G_bprint( 2, " %s the %s flag!\n", redtext("got"), redtext("RED") );
		owner->s.v.effects = (int) owner->s.v.effects | EF_FLAG1 | EF_DIMLIGHT;
	}
	setmodel( self, "" );
}

void PlayerDropFlag( gedict_t *player )
{
	gedict_t *flag;
	char *cn;

	if (!(player->ctf_flag & CTF_FLAG))
		return;

	if ( streq(getteam(player), "red") )
		cn = "item_flag_team2";
	else
		cn = "item_flag_team1";

	player->ctf_flag -= ( player->ctf_flag & CTF_FLAG );

	flag = find( world, FOFCLSN, cn );
	if ( flag )
		DropFlag( flag );
}

void DropFlag( gedict_t *flag)
{
	gedict_t *p = PROG_TO_EDICT( flag->s.v.owner );

	p->s.v.effects -= ( (int) p->s.v.effects & ( EF_FLAG1 | EF_FLAG2 ));
  
	if ( !other->super_damage_finished && !other->invincible_finished )
		other->s.v.effects -= ( (int) other->s.v.effects & EF_DIMLIGHT );

	p->s.v.effects -= ( (int) p->s.v.effects & EF_DIMLIGHT );
	setorigin( flag, PASSVEC3(p->s.v.origin) );
	flag->s.v.origin[2] -= 24;
	flag->cnt = FLAG_DROPPED;
	SetVector( flag->s.v.velocity, 0, 0, 300 );
	flag->s.v.flags = FL_ITEM;
	flag->s.v.solid = SOLID_TRIGGER;
	flag->s.v.movetype = MOVETYPE_TOSS;
	setmodel( flag, flag->mdl );
	setsize ( self, -16, -16, 0, 16, 16, 74 );
	flag->super_time = g_globalvars.time + FLAG_RETURN_TIME;

	G_bprint( 2, p->s.v.netname );
	if ( streq(getteam(p), "red") )
		G_bprint( 2, " %s the %s flag!\n", redtext("lost"), redtext("BLUE") );
	else
		G_bprint( 2, " %s the %s flag!\n", redtext("lost"), redtext("RED") );	
}

void FlagStatus()
{
	gedict_t *flag1, *flag2;

	if ( !isCTF() )
		return;

	flag1 = find( world, FOFCLSN, "item_flag_team1" );
	flag2 = find( world, FOFCLSN, "item_flag_team2" );

	if (!flag1 || !flag2)
		return;

	if ( streq(self->s.v.classname, "spectator") )
	{
		switch ( (int) flag1->cnt )
		{
			case FLAG_AT_BASE:
				G_sprint( self, 2, "The %s flag is in base.\n", redtext("RED") );
				break;
 			case FLAG_CARRIED:
				G_sprint( self, 2, "%s has the %s flag.\n", PROG_TO_EDICT( flag1->s.v.owner )->s.v.netname, redtext("RED") );
				break;
			case FLAG_DROPPED:
				G_sprint( self, 2, "The %s flag is lying about.\n", redtext("RED") );
				break;
		}

		switch ( (int) flag2->cnt )
		{
			case FLAG_AT_BASE:
				G_sprint( self, 2, "The %s flag is in base. ", redtext("BLUE") );
				break;
			case FLAG_CARRIED:
				G_sprint( self, 2, "%s has the %s flag. ", PROG_TO_EDICT( flag1->s.v.owner )->s.v.netname, redtext("BLUE") );
				break;
			case FLAG_DROPPED:
				G_sprint( self, 2, "The %s flag is lying about. ", redtext("BLUE") );
				break;
		}
		return;
	}

	// Swap flags so that flag1 is "your" flag
	if ( streq(getteam(self), "blue") )
	{
		gedict_t *swap = flag1;
		flag1 = flag2;
		flag2 = swap;
	}
  
	switch ( (int) flag1->cnt )
	{
		case FLAG_AT_BASE:
			G_sprint( self, 2, "Your flag is in base. " );
			break;
		case FLAG_CARRIED:
			G_sprint( self, 2, "%s has your flag. ", PROG_TO_EDICT( flag1->s.v.owner )->s.v.netname );
			break;
		case FLAG_DROPPED:
			G_sprint( self, 2, "Your flag is lying about. " );
			break;
	}

	switch ( (int) flag2->cnt )
	{
		case FLAG_AT_BASE:
			G_sprint ( self, 2, "The enemy flag is in their base.\n" );
			break;
		case FLAG_CARRIED:
			if ( self == PROG_TO_EDICT( flag2->s.v.owner ))
				G_sprint ( self, 2, "You have the enemy flag.\n" );
			else
				G_sprint ( self, 2, "%s has the enemy flag.\n", PROG_TO_EDICT( flag2->s.v.owner)->s.v.netname );
			break;
		case FLAG_DROPPED:
			G_sprint ( self, 2, "The enemy flag is lying about.\n" );
			break;
		default:
			G_sprint ( self, 2, "\n" );
	}
}
