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

void            bubble_bob();

/*
==============================================================================

PLAYER

==============================================================================
*/

void            player_run();

void player_stand1()
{
	self->s.v.frame = 17;
	self->s.v.think = ( func_t ) player_stand1;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 0;
	
	if ( self->s.v.velocity[0] || self->s.v.velocity[1] )
	{
		self->walkframe = 0;
		player_run();
		return;
	}

	if ( self->s.v.weapon == IT_AXE || self->s.v.weapon == IT_HOOK )
	{
		if ( self->walkframe >= 12 )
			self->walkframe = 0;
		self->s.v.frame = 17 + self->walkframe;
	} else
	{
		if ( self->walkframe >= 5 )
			self->walkframe = 0;
		self->s.v.frame = 12 + self->walkframe;
	}
	self->walkframe = self->walkframe + 1;
}

void player_run()
{
	self->s.v.frame = 6;
	self->s.v.think = ( func_t ) player_run;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 0;
	if ( !self->s.v.velocity[0] && !self->s.v.velocity[1] )
	{
		self->walkframe = 0;
		player_stand1();
		return;
	}

	if ( self->s.v.weapon == IT_AXE || self->s.v.weapon == IT_HOOK )
	{
		if ( self->walkframe >= 6 )
			self->walkframe = 0;
		if ( self->movement[0] < 0 )
			self->s.v.frame = 5 - self->walkframe;
		else
			self->s.v.frame = 0 + self->walkframe;
	} else
	{
		if ( self->walkframe >= 6 )
			self->walkframe = 0;
		if ( self->movement[0] < 0 )
			self->s.v.frame = 11 - self->walkframe;
		else
			self->s.v.frame = 6 + self->walkframe;
	}
	self->walkframe = self->walkframe + 1;
}

void muzzleflash()
{
	WriteByte( MSG_MULTICAST, SVC_MUZZLEFLASH );
	WriteEntity( MSG_MULTICAST, self );
	trap_multicast( PASSVEC3( self->s.v.origin ), MULTICAST_PVS );
}

void player_chain1()
{
	self->s.v.frame = 137;
	self->s.v.think = ( func_t ) player_chain2;
 	self->s.v.nextthink = g_globalvars.time + 0.1;
	self->s.v.weaponframe = 2;
	GrappleThrow();
}

void player_chain2()
{
	self->s.v.frame = 138;
	self->s.v.think = ( func_t ) player_chain3;
	self->s.v.nextthink = g_globalvars.time + 0.1;
	self->s.v.weaponframe = 3;
}

void player_chain3()
{
	self->s.v.frame = 139;
	self->s.v.think = ( func_t ) player_chain4;
	self->s.v.nextthink = g_globalvars.time + 0.1;
	self->s.v.weaponframe = 3;
	if ( !self->hook_out )
		player_chain5();
	else if ( vlen(self->s.v.velocity) >= 750 )
		player_chain4();
}

void player_chain4()
{
	// Original ctf grapple used frame 73 here, but that causes problems with cl_deadbodyfilter 2
	// Frame 139 is a decent alternative especially given that 73 never looked good anyway
	// self->s.v.frame = 73;
	self->s.v.frame = 139;
	self->s.v.think = ( func_t ) player_chain5;
	self->s.v.nextthink = g_globalvars.time + 0.1;
	self->s.v.weaponframe = 4;
	if ( !self->hook_out )
		player_chain5();
	else if ( vlen(self->s.v.velocity) < 750 )  
		player_chain3(); 
}

void player_chain5()
{
	self->s.v.frame = 140;
	self->walkframe = 0;
	self->s.v.think = ( func_t ) player_run;
	self->s.v.nextthink = g_globalvars.time + 0.1;
	self->s.v.weaponframe = 5;
}

void player_shot1()
{
	self->s.v.frame = 113;
	self->s.v.think = ( func_t ) player_shot2;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 1;
	muzzleflash();
}

void player_shot2()
{
	self->s.v.frame = 114;
	self->s.v.think = ( func_t ) player_shot3;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 2;
}

void player_shot3()
{
	self->s.v.frame = 115;
	self->s.v.think = ( func_t ) player_shot4;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 3;
}

void player_shot4()
{
	self->s.v.frame = 116;
	self->s.v.think = ( func_t ) player_shot5;
	self->s.v.nextthink = g_globalvars.time + 0.1;
	self->s.v.weaponframe = 4;
}

void player_shot5()
{
	self->s.v.frame = 117;
	self->s.v.think = ( func_t ) player_shot6;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 5;
}

void player_shot6()
{
	self->s.v.frame = 118;
	self->walkframe = 0;
	self->s.v.think = ( func_t ) player_run;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 6;
}

void player_axe1()
{
	self->s.v.frame = 119;
	self->s.v.think = ( func_t ) player_axe2;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 1;
}

void player_axe2()
{
	self->s.v.frame = 120;
	self->s.v.think = ( func_t ) player_axe3;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 2;
}

void player_axe3()
{
	self->s.v.frame = 121;
	self->s.v.think = ( func_t ) player_axe4;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 3;
	W_FireAxe();
}

void player_axe4()
{
	self->s.v.frame = 122;
	self->walkframe = 0;
	self->s.v.think = ( func_t ) player_run;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 4;
}

void player_axeb1()
{
	self->s.v.frame = 125;
	self->s.v.think = ( func_t ) player_axeb2;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 5;
}

void player_axeb2()
{
	self->s.v.frame = 126;
	self->s.v.think = ( func_t ) player_axeb3;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 6;
}

void player_axeb3()
{
	self->s.v.frame = 127;
	self->s.v.think = ( func_t ) player_axeb4;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 7;
	W_FireAxe();
}

void player_axeb4()
{
	self->s.v.frame = 128;
	self->walkframe = 0;
	self->s.v.think = ( func_t ) player_run;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 8;
}

void player_axec1()
{
	self->s.v.frame = 131;
	self->s.v.think = ( func_t ) player_axec2;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 1;
}

void player_axec2()
{
	self->s.v.frame = 132;
	self->s.v.think = ( func_t ) player_axec3;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 2;
}

void player_axec3()
{
	self->s.v.frame = 133;
	self->s.v.think = ( func_t ) player_axec4;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 3;
	W_FireAxe();
}

void player_axec4()
{
	self->s.v.frame = 134;
	self->walkframe = 0;
	self->s.v.think = ( func_t ) player_run;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 4;
}

void player_axed1()
{
	self->s.v.frame = 137;
	self->s.v.think = ( func_t ) player_axed2;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 5;
}

void player_axed2()
{
	self->s.v.frame = 138;
	self->s.v.think = ( func_t ) player_axed3;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 6;
}

void player_axed3()
{
	self->s.v.frame = 139;
	self->s.v.think = ( func_t ) player_axed4;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 7;
	W_FireAxe();
}

void player_axed4()
{
	self->s.v.frame = 140;
	self->walkframe = 0;
	self->s.v.think = ( func_t ) player_run;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 8;
}


//============================================================================

// this shit present in ktpro, but test does't show why we need this, because 99.9% of time diff is just 0
void set_idealtime()
{
	float diff = self->s.v.ltime - g_globalvars.time;

	if ( diff < -0.05 ) 
		diff = -0.05;
	
	self->s.v.nextthink = self->s.v.ltime = g_globalvars.time + diff + 0.1;
}

void player_nail1()
{
	self->s.v.frame = 103;
	self->s.v.think = ( func_t ) player_nail2;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	if ( !self->s.v.button0 || intermission_running || self->s.v.impulse )
	{
		self->walkframe = 0;
		player_run();
		return;
	}

	set_idealtime();
	muzzleflash();

	self->s.v.weaponframe = self->s.v.weaponframe + 1;
	if ( self->s.v.weaponframe >= 9 )
		self->s.v.weaponframe = 1;

	SuperDamageSound();
	W_FireSpikes( 4 );
	self->attack_finished = g_globalvars.time + 0.2;
}

void player_nail2()
{
	self->s.v.frame = 104;
	self->s.v.think = ( func_t ) player_nail1;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	if ( !self->s.v.button0 || intermission_running || self->s.v.impulse )
	{
		self->walkframe = 0;
		player_run();
		return;
	}

	set_idealtime();
	muzzleflash();

	self->s.v.weaponframe = self->s.v.weaponframe + 1;
	if ( self->s.v.weaponframe >= 9 )
		self->s.v.weaponframe = 1;

	SuperDamageSound();
	W_FireSpikes( -4 );
	self->attack_finished = g_globalvars.time + 0.2;
}

//============================================================================

void player_light1()
{
	self->s.v.frame = 105;
	self->s.v.think = ( func_t ) player_light2;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	if ( !self->s.v.button0 || intermission_running || self->s.v.impulse )
	{
		self->walkframe = 0;
		player_run();
		return;
	}

	set_idealtime();
	muzzleflash();

	self->s.v.weaponframe = self->s.v.weaponframe + 1;
	if ( self->s.v.weaponframe >= 5 )
		self->s.v.weaponframe = 1;

	SuperDamageSound();
	W_FireLightning();
	self->attack_finished = g_globalvars.time + 0.2;
}

void player_light2()
{
	self->s.v.frame = 106;
	self->s.v.think = ( func_t ) player_light1;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	if ( !self->s.v.button0 || intermission_running || self->s.v.impulse )
	{
		self->walkframe = 0;
		player_run();
		return;
	}

	set_idealtime();
	muzzleflash();

	self->s.v.weaponframe = self->s.v.weaponframe + 1;
	if ( self->s.v.weaponframe >= 5 )
		self->s.v.weaponframe = 1;

	SuperDamageSound();
	W_FireLightning();
	self->attack_finished = g_globalvars.time + 0.2;
}

//============================================================================
void player_rocket1()
{
	self->s.v.frame = 107;
	self->s.v.think = ( func_t ) player_rocket2;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 1;
	muzzleflash();
}

void player_rocket2()
{
	self->s.v.frame = 108;
	self->s.v.think = ( func_t ) player_rocket3;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 2;
}

void player_rocket3()
{
	self->s.v.frame = 109;
	self->s.v.think = ( func_t ) player_rocket4;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 3;
}

void player_rocket4()
{
	self->s.v.frame = 110;
	self->s.v.think = ( func_t ) player_rocket5;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 4;
}

void player_rocket5()
{
	self->s.v.frame = 111;
	self->s.v.think = ( func_t ) player_rocket6;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 5;
}

void player_rocket6()
{
	self->s.v.frame = 112;
	self->walkframe = 0;
	self->s.v.think = ( func_t ) player_run;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	self->s.v.weaponframe = 6;
}

void            DeathBubbles( float num_bubbles );

void PainSound()
{
	int             rs;

	if ( ISDEAD( self ) )
		return;

// water pain sounds
	if ( (self->s.v.watertype == CONTENT_WATER || self->s.v.watertype == CONTENT_SLIME)  && self->s.v.waterlevel == 3 )
	{
		DeathBubbles( 1 );

        if ( match_in_progress != 2 )
            return;

		if ( g_random() > 0.5 )
			sound( self, CHAN_VOICE, "player/drown1.wav", 1, ATTN_NORM );
		else
			sound( self, CHAN_VOICE, "player/drown2.wav", 1, ATTN_NORM );
		return;
	}
// slime pain sounds
	if ( self->s.v.watertype == CONTENT_SLIME )
	{
// FIX ME       put in some steam here
        if ( match_in_progress != 2 )
            return;

		if ( g_random() > 0.5 )
			sound( self, CHAN_VOICE, "player/lburn1.wav", 1, ATTN_NORM );
		else
			sound( self, CHAN_VOICE, "player/lburn2.wav", 1, ATTN_NORM );
		return;
	}

	if ( self->s.v.watertype == CONTENT_LAVA )
	{
        if ( match_in_progress != 2 )
            return;

		if ( g_random() > 0.5 )
			sound( self, CHAN_VOICE, "player/lburn1.wav", 1, ATTN_NORM );
		else
			sound( self, CHAN_VOICE, "player/lburn2.wav", 1, ATTN_NORM );
		return;
	}

	if ( self->pain_finished > g_globalvars.time )
	{
		self->axhitme = 0;
		return;
	}
	self->pain_finished = g_globalvars.time + 0.5;

// don't make multiple pain sounds right after each other

// ax pain sound
	if ( self->axhitme == 1 )
	{
		self->axhitme = 0;
		sound( self, CHAN_VOICE, "player/axhit1.wav", 1, ATTN_NORM );
		return;
	}

	rs = ( g_random() * 5 ) + 1;

	self->s.v.noise = "";
	if ( rs == 1 )
		self->s.v.noise = "player/pain1.wav";
	else if ( rs == 2 )
		self->s.v.noise = "player/pain2.wav";
	else if ( rs == 3 )
		self->s.v.noise = "player/pain3.wav";
	else if ( rs == 4 )
		self->s.v.noise = "player/pain4.wav";
	else if ( rs == 5 )
		self->s.v.noise = "player/pain5.wav";
	else
		self->s.v.noise = "player/pain6.wav";

	sound( self, CHAN_VOICE, self->s.v.noise, 1, ATTN_NORM );
	return;
}

void player_pain1()
{
	self->s.v.frame = 35;
	self->s.v.think = ( func_t ) player_pain2;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	PainSound();
	self->s.v.weaponframe = 0;
}

void player_pain2()
{
	self->s.v.frame = 36;
	self->s.v.think = ( func_t ) player_pain3;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_pain3()
{
	self->s.v.frame = 37;
	self->s.v.think = ( func_t ) player_pain4;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_pain4()
{
	self->s.v.frame = 38;
	self->s.v.think = ( func_t ) player_pain5;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_pain5()
{
	self->s.v.frame = 39;
	self->s.v.think = ( func_t ) player_pain6;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_pain6()
{
	self->s.v.frame = 40;
	self->walkframe = 0;
	self->s.v.think = ( func_t ) player_run;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_axpain1()
{
	self->s.v.frame = 29;
	self->s.v.think = ( func_t ) player_axpain2;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	PainSound();
	self->s.v.weaponframe = 0;
}

void player_axpain2()
{
	self->s.v.frame = 30;
	self->s.v.think = ( func_t ) player_axpain3;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_axpain3()
{
	self->s.v.frame = 31;
	self->s.v.think = ( func_t ) player_axpain4;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_axpain4()
{
	self->s.v.frame = 32;
	self->s.v.think = ( func_t ) player_axpain5;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_axpain5()
{
	self->s.v.frame = 33;
	self->s.v.think = ( func_t ) player_axpain6;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_axpain6()
{
	self->s.v.frame = 34;
	self->walkframe = 0;
	self->s.v.think = ( func_t ) player_run;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_pain( struct gedict_s *attacker, float take )
{
//	G_bprint(2, "player_pain\n");

	if ( match_in_progress != 2 )
		return; // no pain at all in prewar

	if ( self->s.v.weaponframe )
		return;

	if ( self->invisible_finished > g_globalvars.time )
		return;		// eyes don't have pain frames

	if ( self->s.v.weapon == IT_AXE )
		player_axpain1();
	else
		player_pain1();
}

void            player_diea1();
void            player_dieb1();
void            player_diec1();
void            player_died1();
void            player_diee1();
void            player_die_ax1();

void DeathBubblesSpawn()
{
	gedict_t       *bubble;

	if ( PROG_TO_EDICT( self->s.v.owner )->s.v.waterlevel != 3 )
		return;
	bubble = spawn();
	setmodel( bubble, "progs/s_bubble.spr" );
	setorigin( bubble, PROG_TO_EDICT( self->s.v.owner )->s.v.origin[0],
			PROG_TO_EDICT( self->s.v.owner )->s.v.origin[1],
			PROG_TO_EDICT( self->s.v.owner )->s.v.origin[2] + 24 );

	bubble->s.v.movetype = MOVETYPE_NOCLIP;
	bubble->s.v.solid = SOLID_NOT;

	SetVector( bubble->s.v.velocity, 0, 0, 15 );

	bubble->s.v.nextthink = g_globalvars.time + 0.5;
	bubble->s.v.think = ( func_t ) bubble_bob;
	bubble->s.v.classname = "bubble";
	bubble->s.v.frame = 0;
	bubble->cnt = 0;
	
	setsize( bubble, -8, -8, -8, 8, 8, 8 );

	self->s.v.nextthink = g_globalvars.time + 0.1;
	self->s.v.think = ( func_t ) DeathBubblesSpawn;
	self->air_finished = self->air_finished + 1;

	if ( self->air_finished >= self->bubble_count )
		ent_remove( self );
}

void DeathBubbles( float num_bubbles )
{
	gedict_t       *bubble_spawner;

	bubble_spawner = spawn();
	setorigin( bubble_spawner, PASSVEC3( self->s.v.origin ) );

	bubble_spawner->s.v.movetype = MOVETYPE_NONE;
	bubble_spawner->s.v.solid = SOLID_NOT;
	bubble_spawner->s.v.nextthink = g_globalvars.time + 0.1;
	bubble_spawner->s.v.think = ( func_t ) DeathBubblesSpawn;
	bubble_spawner->air_finished = 0;
	bubble_spawner->s.v.owner = EDICT_TO_PROG( self );
	bubble_spawner->bubble_count = num_bubbles;
	return;
}


void DeathSound()
{
	int             rs;

	// water death sounds
	if ( self->s.v.waterlevel == 3 )
	{
		DeathBubbles( 5 );
		sound( self, CHAN_VOICE, "player/h2odeath.wav", 1, ATTN_NONE );
		return;
	}

	rs = ( ( g_random() * 4 ) + 1 );
	if ( rs == 1 )
		self->s.v.noise = "player/death1.wav";
	if ( rs == 2 )
		self->s.v.noise = "player/death2.wav";
	if ( rs == 3 )
		self->s.v.noise = "player/death3.wav";
	if ( rs == 4 )
		self->s.v.noise = "player/death4.wav";
	if ( rs == 5 )
		self->s.v.noise = "player/death5.wav";

	sound( self, CHAN_VOICE, self->s.v.noise, 1, ATTN_NONE );
	return;
}


void PlayerDead()
{
	self->s.v.nextthink = -1;
// allow respawn after a certain time
	self->s.v.deadflag = DEAD_DEAD;

	// Yawnmode: hide player corpses after animation
	// - Molgrum
	if ( k_yawnmode )
		setmodel( self, "" );
}

void VelocityForDamage( float dm, vec3_t v )
{
	vec3_t          v2;

	if ( vlen( damage_inflictor->s.v.velocity ) > 0 )
	{
		VectorScale( damage_inflictor->s.v.velocity, 0.5, v );
		VectorSubtract( self->s.v.origin, damage_inflictor->s.v.origin, v2 );
		VectorNormalize( v2 );
		VectorScale( v2, 25, v2 );
		VectorAdd( v, v2, v );
//  v = 0.5 * damage_inflictor->s.v.velocity;
//  v = v + (25 * normalize((self->s.v.origin)-damage_inflictor->s.v.origin));
		v[2] = 100 + 240 * g_random();
		v[0] = v[0] + ( 200 * crandom() );
		v[1] = v[1] + ( 200 * crandom() );
		//dprint ("Velocity gib\n");                
	} else
	{
		v[0] = 100 * crandom();
		v[1] = 100 * crandom();
		v[2] = 200 + 100 * g_random();
	}

	//v[0] = 100 * crandom();
	//v[1] = 100 * crandom();
	//v[2] = 200 + 100 * g_random();

	if ( dm > -50 )
	{
		//      dprint ("level 1\n");
		VectorScale( v, 0.7, v );
//  v = v * 0.7;
	} else if ( dm > -200 )
	{
		//      dprint ("level 3\n");
		VectorScale( v, 2, v );
//  v = v * 2;
	} else
		VectorScale( v, 10, v );
//  v = v * 10;

	return;			//v;
}

gedict_t *ThrowGib( char *gibname, float dm )
{
	gedict_t       *newent;
	int			    k_short_gib = cvar( "k_short_gib" ); // if set - remove faster

	newent = spawn();
	VectorCopy( self->s.v.origin, newent->s.v.origin );
	setmodel( newent, gibname );
	setsize( newent, 0, 0, 0, 0, 0, 0 );
	VelocityForDamage( dm, newent->s.v.velocity );
	newent->s.v.movetype = MOVETYPE_BOUNCE;
	newent->isMissile = true;
	newent->s.v.solid = SOLID_NOT;
	newent->s.v.avelocity[0] = g_random() * 600;
	newent->s.v.avelocity[1] = g_random() * 600;
	newent->s.v.avelocity[2] = g_random() * 600;
	newent->s.v.think = ( func_t ) SUB_Remove;
	newent->s.v.ltime = g_globalvars.time;
	newent->s.v.nextthink = g_globalvars.time + ( k_short_gib ? 2 : ( 10 + g_random() * 10 ) );
	newent->s.v.frame = 0;
	newent->s.v.flags = 0;

	return newent;
}

void ThrowHead( char *gibname, float dm )
{
	setmodel( self, gibname );
	self->s.v.frame = 0;
	self->s.v.movetype = MOVETYPE_BOUNCE;
// qqshka: NO, you can't do that, that NOT projectile, its player entity!
//	self->isMissile = true;
	self->s.v.takedamage = DAMAGE_NO;
	self->s.v.solid = SOLID_NOT;
	SetVector( self->s.v.view_ofs, 0, 0, 8 );
	setsize( self, -16, -16, 0, 16, 16, 56 );
	VelocityForDamage( dm, self->s.v.velocity );
	self->s.v.origin[2] = self->s.v.origin[2] - 24;
	self->s.v.flags -= ( ( int ) ( self->s.v.flags ) ) & FL_ONGROUND;

	SetVector( self->s.v.avelocity, 0, crandom() * 600, 0 );
}

void GibPlayer()
{
	qbool bloodfest_round_connect = ( k_bloodfest && !self->ready ); // in case of bloodfest and connecion during round.
	gedict_t *p;

	self->vw_index = 0;

	if ( isRACE() || bloodfest_round_connect )
		ThrowHead( "", self->s.v.health );
	else
		ThrowHead( "progs/h_player.mdl", self->s.v.health );

	if ( bloodfest_round_connect )
		return; // do not spawn sounds and gibs, preventing abuse.

    if( match_in_progress == 2 )
	{
		ThrowGib( "progs/gib1.mdl", self->s.v.health );
		ThrowGib( "progs/gib2.mdl", self->s.v.health );
		ThrowGib( "progs/gib3.mdl", self->s.v.health );
    }

	if ( isRACE() && race.status )
		return;

	// spawn temporary entity.
	p = spawn();
	setorigin( p, PASSVEC3( self->s.v.origin ) );
	p->s.v.nextthink = g_globalvars.time + 0.1;
	p->s.v.think = ( func_t ) SUB_Remove;

	if ( TELEDEATH( self )	)
	{
		sound( p, CHAN_VOICE, "player/teledth1.wav", 1, ATTN_NONE );
	}
	else
	{
		sound( p, CHAN_VOICE, (g_random() < 0.5 ? "player/gib.wav" : "player/udeath.wav"), 1, ATTN_NONE );
	}
}


void StartDie ();

void PlayerDie()
{
	self->ca_alive = false;

	DropPowerups();

	if ( isCTF() )
	{
		if ( self->hook_out )
		{
			GrappleReset( self->hook );
			self->attack_finished = g_globalvars.time + 0.75;
			self->hook_out = true; // FIXME: for which reason this set to true?
		}

		DropRune();
		PlayerDropFlag( self, false );
	} 

	self->s.v.items -= ( int ) self->s.v.items & IT_INVISIBILITY;
	self->invisible_finished = 0;	// don't die as eyes
	self->invincible_finished = 0;
// so we have quad few milleseconds after death
//	self->super_damage_finished = 0; // moved to prethink, like in ktpro
	self->radsuit_finished = 0;

	self->s.v.modelindex = modelindex_player;	// don't use eyes

	DropBackpack();

	self->s.v.weaponmodel = "";
	if (vw_enabled)
		self->vw_index = 9;	// null vwep model

	SetVector( self->s.v.view_ofs, 0, 0, -8 );
	self->s.v.deadflag = DEAD_DYING;
	self->s.v.solid = SOLID_NOT;
	self->s.v.flags -= ( ( int ) ( self->s.v.flags ) ) & FL_ONGROUND;
	self->s.v.movetype = MOVETYPE_TOSS;
	if ( self->s.v.velocity[2] < 10 )
		self->s.v.velocity[2] = self->s.v.velocity[2] + g_random() * 300;

    if ( self->s.v.health < -40 || dtSQUISH == self->deathtype || dtSUICIDE == self->deathtype || isRA() || isCA() )
	{
		GibPlayer();

		// Yawnmode: respawn has the same delay (900ms) regardless of deathtype gib/normal
		// - Molgrum

		// Hoonymode: Also force some time, e.g. to prevent instant respawn after /kill which
		// can cause bug if kill telefrags an idle player (counts as two points...)
		// only ever happens in testing, but oh well --phil
		if ( k_yawnmode || isHoonyMode() )
		{
			self->s.v.nextthink = g_globalvars.time + 0.9;
			self->s.v.think = ( func_t ) PlayerDead;
			return;
		}

		PlayerDead();
		return;
	}

    if( match_in_progress == 2 )
		DeathSound();

	self->s.v.angles[0] = 0;
	self->s.v.angles[2] = 0;


    // function part split and called here
	StartDie();
}

// created this function because it is called from client.qc as well
// was originally part of PlayerDie() and hasn't been altered
void StartDie ()
{
	if ( self->s.v.weapon == IT_AXE )
	{
		player_die_ax1();
		return;
	}

//	if ( k_yawnmode )
// qqshka: this way it better
	if ( 1 )
	{
		// Yawnmode: exclude diea1 and diec1 so the respawn time is always 900 ms
		switch( i_rnd(1, 3) ) {
			case  1: player_dieb1(); break;
			case  2: player_died1(); break;
			default: player_diee1(); break;
		}
	}
	else
	{
		// Note that this generates random values in 1..6 range, so player_diee1 is
		// executed twice as often as other death sequences. Dunno if this should be fixed -- Tonik
		int i = 1 + floor( g_random() * 6 );

	    switch( i ) {
			case  1: player_diea1(); break;
			case  2: player_dieb1(); break;
			case  3: player_diec1(); break;
			case  4: player_died1(); break;
			default: player_diee1(); break;
	    }
	}
}

void player_diea1()
{
	self->s.v.frame = 50;
	self->s.v.think = ( func_t ) player_diea2;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diea2()
{
	self->s.v.frame = 51;
	self->s.v.think = ( func_t ) player_diea3;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diea3()
{
	self->s.v.frame = 52;
	self->s.v.think = ( func_t ) player_diea4;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diea4()
{
	self->s.v.frame = 53;
	self->s.v.think = ( func_t ) player_diea5;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diea5()
{
	self->s.v.frame = 54;
	self->s.v.think = ( func_t ) player_diea6;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diea6()
{
	self->s.v.frame = 55;
	self->s.v.think = ( func_t ) player_diea7;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diea7()
{
	self->s.v.frame = 56;
	self->s.v.think = ( func_t ) player_diea8;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diea8()
{
	self->s.v.frame = 57;
	self->s.v.think = ( func_t ) player_diea9;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diea9()
{
	self->s.v.frame = 58;
	self->s.v.think = ( func_t ) player_diea10;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diea10()
{
	self->s.v.frame = 59;
	self->s.v.think = ( func_t ) player_diea11;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diea11()
{
	self->s.v.frame = 60;
	self->s.v.think = ( func_t ) player_diea11;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	PlayerDead();
}

void player_dieb1()
{
	self->s.v.frame = 61;
	self->s.v.think = ( func_t ) player_dieb2;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_dieb2()
{
	self->s.v.frame = 62;
	self->s.v.think = ( func_t ) player_dieb3;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_dieb3()
{
	self->s.v.frame = 63;
	self->s.v.think = ( func_t ) player_dieb4;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_dieb4()
{
	self->s.v.frame = 64;
	self->s.v.think = ( func_t ) player_dieb5;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_dieb5()
{
	self->s.v.frame = 65;
	self->s.v.think = ( func_t ) player_dieb6;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_dieb6()
{
	self->s.v.frame = 66;
	self->s.v.think = ( func_t ) player_dieb7;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_dieb7()
{
	self->s.v.frame = 67;
	self->s.v.think = ( func_t ) player_dieb8;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_dieb8()
{
	self->s.v.frame = 68;
	self->s.v.think = ( func_t ) player_dieb9;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_dieb9()
{
	self->s.v.frame = 69;
	self->s.v.think = ( func_t ) player_dieb9;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	PlayerDead();
}

void player_diec1()
{
	self->s.v.frame = 70;
	self->s.v.think = ( func_t ) player_diec2;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diec2()
{
	self->s.v.frame = 71;
	self->s.v.think = ( func_t ) player_diec3;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diec3()
{
	self->s.v.frame = 72;
	self->s.v.think = ( func_t ) player_diec4;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diec4()
{
	self->s.v.frame = 73;
	self->s.v.think = ( func_t ) player_diec5;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diec5()
{
	self->s.v.frame = 74;
	self->s.v.think = ( func_t ) player_diec6;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diec6()
{
	self->s.v.frame = 75;
	self->s.v.think = ( func_t ) player_diec7;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diec7()
{
	self->s.v.frame = 76;
	self->s.v.think = ( func_t ) player_diec8;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diec8()
{
	self->s.v.frame = 77;
	self->s.v.think = ( func_t ) player_diec9;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diec9()
{
	self->s.v.frame = 78;
	self->s.v.think = ( func_t ) player_diec10;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diec10()
{
	self->s.v.frame = 79;
	self->s.v.think = ( func_t ) player_diec11;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diec11()
{
	self->s.v.frame = 80;
	self->s.v.think = ( func_t ) player_diec12;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diec12()
{
	self->s.v.frame = 81;
	self->s.v.think = ( func_t ) player_diec13;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diec13()
{
	self->s.v.frame = 82;
	self->s.v.think = ( func_t ) player_diec14;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diec14()
{
	self->s.v.frame = 83;
	self->s.v.think = ( func_t ) player_diec15;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diec15()
{
	self->s.v.frame = 84;
	self->s.v.think = ( func_t ) player_diec15;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	PlayerDead();
}

void player_died1()
{
	self->s.v.frame = 85;
	self->s.v.think = ( func_t ) player_died2;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_died2()
{
	self->s.v.frame = 86;
	self->s.v.think = ( func_t ) player_died3;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_died3()
{
	self->s.v.frame = 87;
	self->s.v.think = ( func_t ) player_died4;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_died4()
{
	self->s.v.frame = 88;
	self->s.v.think = ( func_t ) player_died5;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_died5()
{
	self->s.v.frame = 89;
	self->s.v.think = ( func_t ) player_died6;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_died6()
{
	self->s.v.frame = 90;
	self->s.v.think = ( func_t ) player_died7;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_died7()
{
	self->s.v.frame = 91;
	self->s.v.think = ( func_t ) player_died8;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_died8()
{
	self->s.v.frame = 92;
	self->s.v.think = ( func_t ) player_died9;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_died9()
{
	self->s.v.frame = 93;
	self->s.v.think = ( func_t ) player_died9;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	PlayerDead();
}

void player_diee1()
{
	self->s.v.frame = 94;
	self->s.v.think = ( func_t ) player_diee2;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diee2()
{
	self->s.v.frame = 95;
	self->s.v.think = ( func_t ) player_diee3;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diee3()
{
	self->s.v.frame = 96;
	self->s.v.think = ( func_t ) player_diee4;
	self->s.v.nextthink = g_globalvars.time + 0.1;

}

void player_diee4()
{
	self->s.v.frame = 97;
	self->s.v.think = ( func_t ) player_diee5;
	self->s.v.nextthink = g_globalvars.time + 0.1;
}

void player_diee5()
{
	self->s.v.frame = 98;
	self->s.v.think = ( func_t ) player_diee6;
	self->s.v.nextthink = g_globalvars.time + 0.1;
}

void player_diee6()
{
	self->s.v.frame = 99;
	self->s.v.think = ( func_t ) player_diee7;
	self->s.v.nextthink = g_globalvars.time + 0.1;
}

void player_diee7()
{
	self->s.v.frame = 100;
	self->s.v.think = ( func_t ) player_diee8;
	self->s.v.nextthink = g_globalvars.time + 0.1;
}

void player_diee8()
{
	self->s.v.frame = 101;
	self->s.v.think = ( func_t ) player_diee9;
	self->s.v.nextthink = g_globalvars.time + 0.1;
}

void player_diee9()
{
	self->s.v.frame = 102;
	self->s.v.think = ( func_t ) player_diee9;
	self->s.v.nextthink = g_globalvars.time + 0.1;
	PlayerDead();
}

void player_die_ax1()
{
	self->s.v.frame = 41;
	self->s.v.think = ( func_t ) player_die_ax2;
	self->s.v.nextthink = g_globalvars.time + 0.1;
}

void player_die_ax2()
{
	self->s.v.frame = 42;
	self->s.v.think = ( func_t ) player_die_ax3;
	self->s.v.nextthink = g_globalvars.time + 0.1;
}

void player_die_ax3()
{
	self->s.v.frame = 43;
	self->s.v.think = ( func_t ) player_die_ax4;
	self->s.v.nextthink = g_globalvars.time + 0.1;
}

void player_die_ax4()
{
	self->s.v.frame = 44;
	self->s.v.think = ( func_t ) player_die_ax5;
	self->s.v.nextthink = g_globalvars.time + 0.1;
}

void player_die_ax5()
{
	self->s.v.frame = 45;
	self->s.v.think = ( func_t ) player_die_ax6;
	self->s.v.nextthink = g_globalvars.time + 0.1;
}

void player_die_ax6()
{
	self->s.v.frame = 46;
	self->s.v.think = ( func_t ) player_die_ax7;
	self->s.v.nextthink = g_globalvars.time + 0.1;
}

void player_die_ax7()
{
	self->s.v.frame = 47;
	self->s.v.think = ( func_t ) player_die_ax8;
	self->s.v.nextthink = g_globalvars.time + 0.1;
}

void player_die_ax8()
{
	self->s.v.frame = 48;
	self->s.v.think = ( func_t ) player_die_ax9;
	self->s.v.nextthink = g_globalvars.time + 0.1;
}

void player_die_ax9()
{
	self->s.v.frame = 49;
	self->s.v.think = ( func_t ) player_die_ax9;
	self->s.v.nextthink = g_globalvars.time + 0.1;

	PlayerDead();
}
