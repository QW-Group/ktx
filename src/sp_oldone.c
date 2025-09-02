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
/*
 ==============================================================================

 OLD ONE

 ==============================================================================
 */

#include "g_local.h"

enum
{
	old1,
	old2,
	old3,
	old4,
	old5,
	old6,
	old7,
	old8,
	old9,
	old10,
	old11,
	old12,
	old13,
	old14,
	old15,
	old16,
	old17,
	old18,
	old19,
	old20,
	old21,
	old22,
	old23,
	old24,
	old25,
	old26,
	old27,
	old28,
	old29,
	old30,
	old31,
	old32,
	old33,
	old34,
	old35,
	old36,
	old37,
	old38,
	old39,
	old40,
	old41,
	old42,
	old43,
	old44,
	old45,
	old46,

	shake1,
	shake2,
	shake3,
	shake4,
	shake5,
	shake6,
	shake7,
	shake8,
	shake9,
	shake10,
	shake11,
	shake12,
	shake13,
	shake14,
	shake15,
	shake16,
	shake17,
	shake18,
	shake19,
	shake20,

};

void old_idle1(void);
void old_idle2(void);
void old_idle3(void);
void old_idle4(void);
void old_idle5(void);
void old_idle6(void);
void old_idle7(void);
void old_idle8(void);
void old_idle9(void);
void old_idle10(void);
void old_idle11(void);
void old_idle12(void);
void old_idle13(void);
void old_idle14(void);
void old_idle15(void);
void old_idle16(void);
void old_idle17(void);
void old_idle18(void);
void old_idle19(void);
void old_idle20(void);
void old_idle21(void);
void old_idle22(void);
void old_idle23(void);
void old_idle24(void);
void old_idle25(void);
void old_idle26(void);
void old_idle27(void);
void old_idle28(void);
void old_idle29(void);
void old_idle30(void);
void old_idle31(void);
void old_idle32(void);
void old_idle33(void);
void old_idle34(void);
void old_idle35(void);
void old_idle36(void);
void old_idle37(void);
void old_idle38(void);
void old_idle39(void);
void old_idle40(void);
void old_idle41(void);
void old_idle42(void);
void old_idle43(void);
void old_idle44(void);
void old_idle45(void);
void old_idle46(void);
void old_thrash1(void);
void old_thrash2(void);
void old_thrash3(void);
void old_thrash4(void);
void old_thrash5(void);
void old_thrash6(void);
void old_thrash7(void);
void old_thrash8(void);
void old_thrash9(void);
void old_thrash10(void);
void old_thrash11(void);
void old_thrash12(void);
void old_thrash13(void);
void old_thrash14(void);
void old_thrash15(void);
void old_thrash16(void);
void old_thrash17(void);
void old_thrash18(void);
void old_thrash19(void);
void old_thrash20(void);

//=============================================================================

ANIM(old_idle1, old1, old_idle2;)
ANIM(old_idle2, old2, old_idle3;)
ANIM(old_idle3, old3, old_idle4;)
ANIM(old_idle4, old4, old_idle5;)
ANIM(old_idle5, old5, old_idle6;)
ANIM(old_idle6, old6, old_idle7;)
ANIM(old_idle7, old7, old_idle8;)
ANIM(old_idle8, old8, old_idle9;)
ANIM(old_idle9, old9, old_idle10;)
ANIM(old_idle10, old10, old_idle11;)
ANIM(old_idle11, old11, old_idle12;)
ANIM(old_idle12, old12, old_idle13;)
ANIM(old_idle13, old13, old_idle14;)
ANIM(old_idle14, old14, old_idle15;)
ANIM(old_idle15, old15, old_idle16;)
ANIM(old_idle16, old16, old_idle17;)
ANIM(old_idle17, old17, old_idle18;)
ANIM(old_idle18, old18, old_idle19;)
ANIM(old_idle19, old19, old_idle20;)
ANIM(old_idle20, old20, old_idle21;)
ANIM(old_idle21, old21, old_idle22;)
ANIM(old_idle22, old22, old_idle23;)
ANIM(old_idle23, old23, old_idle24;)
ANIM(old_idle24, old24, old_idle25;)
ANIM(old_idle25, old25, old_idle26;)
ANIM(old_idle26, old26, old_idle27;)
ANIM(old_idle27, old27, old_idle28;)
ANIM(old_idle28, old28, old_idle29;)
ANIM(old_idle29, old29, old_idle30;)
ANIM(old_idle30, old30, old_idle31;)
ANIM(old_idle31, old31, old_idle32;)
ANIM(old_idle32, old32, old_idle33;)
ANIM(old_idle33, old33, old_idle34;)
ANIM(old_idle34, old34, old_idle35;)
ANIM(old_idle35, old35, old_idle36;)
ANIM(old_idle36, old36, old_idle37;)
ANIM(old_idle37, old37, old_idle38;)
ANIM(old_idle38, old38, old_idle39;)
ANIM(old_idle39, old39, old_idle40;)
ANIM(old_idle40, old40, old_idle41;)
ANIM(old_idle41, old41, old_idle42;)
ANIM(old_idle42, old42, old_idle43;)
ANIM(old_idle43, old43, old_idle44;)
ANIM(old_idle44, old44, old_idle45;)
ANIM(old_idle45, old45, old_idle46;)
ANIM(old_idle46, old46, old_idle1;)

void finale_1(void);
void finale_2(void);
void finale_3(void);
void finale_4(void);

void _old_thrash15(void)
{
	self->cnt++;

	if (self->cnt < 3)
	{
		self->think = (func_t) old_thrash1;
	}
}

ANIM(old_thrash1, shake1, old_thrash2; trap_lightstyle(0, "m");)
ANIM(old_thrash2, shake2, old_thrash3; trap_lightstyle(0, "k");)
ANIM(old_thrash3, shake3, old_thrash4; trap_lightstyle(0, "k");)
ANIM(old_thrash4, shake4, old_thrash5; trap_lightstyle(0, "i");)
ANIM(old_thrash5, shake5, old_thrash6; trap_lightstyle(0, "g");)
ANIM(old_thrash6, shake6, old_thrash7; trap_lightstyle(0, "e");)
ANIM(old_thrash7, shake7, old_thrash8; trap_lightstyle(0, "c");)
ANIM(old_thrash8, shake8, old_thrash9; trap_lightstyle(0, "a");)
ANIM(old_thrash9, shake9, old_thrash10; trap_lightstyle(0, "c");)
ANIM(old_thrash10, shake10, old_thrash11; trap_lightstyle(0, "e");)
ANIM(old_thrash11, shake11, old_thrash12; trap_lightstyle(0, "g");)
ANIM(old_thrash12, shake12, old_thrash13; trap_lightstyle(0, "i");)
ANIM(old_thrash13, shake13, old_thrash14; trap_lightstyle(0, "k");)
ANIM(old_thrash14, shake14, old_thrash15; trap_lightstyle(0, "m");)
ANIM(old_thrash15, shake15, old_thrash16; trap_lightstyle(0, "m"); _old_thrash15();)
ANIM(old_thrash16, shake16, old_thrash17; trap_lightstyle(0, "g");)
ANIM(old_thrash17, shake17, old_thrash18; trap_lightstyle(0, "c");)
ANIM(old_thrash18, shake18, old_thrash19; trap_lightstyle(0, "b");)
ANIM(old_thrash19, shake19, old_thrash20; trap_lightstyle(0, "a");)
ANIM(old_thrash20, shake20, old_thrash20; finale_4();)

//============================================================================

void finale_1(void)
{
	gedict_t *pl;
	gedict_t *timer;

	self->s.v.effects = (int)self->s.v.effects | EF_RED;

	g_globalvars.killed_monsters++;
	WriteByte( MSG_ALL, SVC_KILLEDMONSTER);

	pl = ez_find(world, "misc_teleporttrain");
	if (!pl)
	{
		G_Error("no teleporttrain");
	}

	ent_remove(pl);

	set_nextmap("start"); // prepare change map to start again
	g_globalvars.serverflags = (int)g_globalvars.serverflags & ~15; // remove runes

	intermission_exittime = g_globalvars.time + 45;	// never allow exit
	intermission_running = 1;

	// find the intermission spot
	intermission_spot = ez_find(world, "info_intermission");
	if (!intermission_spot)
	{
		G_Error("no info_intermission");
	}

	// svc_finale is broken in QW, so send svc_intermission first to set view angles
	// and origin, then send svc_finale to set cl.intermission to 2.

	WriteByte( MSG_ALL, SVC_INTERMISSION);
	WriteCoord( MSG_ALL, intermission_spot->s.v.origin[0]);
	WriteCoord( MSG_ALL, intermission_spot->s.v.origin[1]);
	WriteCoord( MSG_ALL, intermission_spot->s.v.origin[2]);
	WriteAngle( MSG_ALL, intermission_spot->mangle[0]);
	WriteAngle( MSG_ALL, intermission_spot->mangle[1]);
	WriteAngle( MSG_ALL, intermission_spot->mangle[2]);

	WriteByte( MSG_ALL, SVC_FINALE);
	WriteString( MSG_ALL, "");

	for (pl = world; (pl = find_plr(pl));)
	{
		VectorCopy(VEC_ORIGIN, pl->s.v.view_ofs);
		//pl->s.v.angles = other.v_angle = intermission_spot.mangle;
		VectorCopy(intermission_spot->mangle, pl->s.v.angles);
		VectorCopy(intermission_spot->mangle, /* "other" it was... */pl->s.v.v_angle);
		pl->s.v.fixangle = true;		// turn this way immediately
		pl->map = self->map;
		pl->s.v.nextthink = g_globalvars.time + 0.5;
		pl->s.v.takedamage = DAMAGE_NO;
		pl->s.v.solid = SOLID_NOT;
		pl->s.v.movetype = MOVETYPE_NONE;
		pl->s.v.modelindex = 0;
		setorigin(pl, PASSVEC3(intermission_spot->s.v.origin));
	}

	// make fake versions of all players as standins, and move the real
	// players to the intermission spot

	// wait for 1 second
	timer = spawn();
	timer->s.v.nextthink = g_globalvars.time + 1;
	timer->think = (func_t) finale_2;
}

gedict_t* shub_find(char *msg)
{
	gedict_t *shub = ez_find(world, "monster_oldone");

	if (!shub)
	{
		G_Error("shub_find() null, in %s", msg);
	}

	return shub;
}

void finale_2(void)
{
	vec3_t o;
	gedict_t *shub = shub_find("finale_2");

	// start a teleport splash inside shub

	//o = shub->s.v.origin - '0 100 0';
	VectorCopy(shub->s.v.origin, o);
	o[1] -= 100;
	WriteByte( MSG_BROADCAST, SVC_TEMPENTITY);
	WriteByte( MSG_BROADCAST, TE_TELEPORT);
	WriteCoord( MSG_BROADCAST, o[0]);
	WriteCoord( MSG_BROADCAST, o[1]);
	WriteCoord( MSG_BROADCAST, o[2]);

	sound(shub, CHAN_VOICE, "misc/r_tele1.wav", 1, ATTN_NORM);

	self->s.v.nextthink = g_globalvars.time + 2;
	self->think = (func_t) finale_3;
}

void finale_3(void)
{
	gedict_t *shub = shub_find("finale_3");

	// start shub thrashing wildly
	shub->think = (func_t) old_thrash1;
	shub->s.v.nextthink = g_globalvars.time + 0.01;
	sound(shub, CHAN_VOICE, "boss2/death.wav", 1, ATTN_NORM);
	trap_lightstyle(0, "abcdefghijklmlkjihgfedcb");

	// remove timer ent it not needed any more
	ent_remove(self);
}

void kill_all_monsters(void)
{
	gedict_t *monster;

	for (monster = world; (monster = nextent(monster));)
	{
		if (!((int)monster->s.v.flags & FL_MONSTER))
		{
			continue; // not a monster
		}

		if (ISDEAD(monster))
		{
			continue;
		}

		monster->deathtype = dtSQUISH;
		T_Damage(monster, world, world, 50000);
	}

	self->think = (func_t) kill_all_monsters;
	self->s.v.nextthink = g_globalvars.time + 0.2;
}

void finale_4(void)
{
	// throw tons of meat chunks
	vec3_t oldo;
	float x, y, z;
	float r;
	gedict_t *n;

	sound(self, CHAN_VOICE, "boss2/pop2.wav", 1, ATTN_NORM);

	VectorCopy(self->s.v.origin, oldo);

	// In QW, we only spawn 18 gibs as opposed to 50 in NetQuake
	// 50 is too much for QW's MAX_PACKET_ENTITIES = 64

	for (z = 16; z <= 144; z += 96)
	{
		for (x = -64; x <= 64; x += 64)
		{
			for (y = -64; y <= 64; y += 64)
			{
				self->s.v.origin[0] = oldo[0] + x;
				self->s.v.origin[1] = oldo[1] + y;
				self->s.v.origin[2] = oldo[2] + z;

				r = g_random();

				if (r < 0.3)
				{
					n = ThrowGib("progs/gib1.mdl", -999);
				}
				else if (r < 0.6)
				{
					n = ThrowGib("progs/gib2.mdl", -999);
				}
				else
				{
					n = ThrowGib("progs/gib3.mdl", -999);
				}

				n->s.v.effects = (int)n->s.v.effects | EF_RED;
			}
		}
	}

	// start the end text
	WriteByte( MSG_ALL, SVC_FINALE);
	WriteString( MSG_ALL, "Congratulations and well done! You have\n"
				"beaten the hideous Shub-Niggurath, and\n"
				"her hundreds of ugly changelings and\n"
				"monsters. You have proven that your\n"
				"skill and your cunning are greater than\n"
				"all the powers of Quake. You are the\n"
				"master now. Id Software salutes you.\n"
				"\n"
				"o/");

	// prepare timer commit genocide in monsters formation
	n = spawn();
	n->think = (func_t) kill_all_monsters;
	n->s.v.nextthink = g_globalvars.time + 0.01;

	// put a player model down
	n = spawn();
	setmodel(n, "progs/player.mdl");
	setorigin(n, oldo[0] - 32, oldo[1] - 264, oldo[2]);
	SetVector(n->s.v.angles, 0, 290, 0);
	n->s.v.frame = 17;
	n->think = (func_t) player_stand1;
	n->s.v.nextthink = g_globalvars.time;
	n->s.v.weapon = IT_AXE;
	n->s.v.effects = (int)n->s.v.effects | EF_BLUE;

	ent_remove(self);

	// switch cd track
	WriteByte( MSG_ALL, SVC_CDTRACK);
	WriteByte( MSG_ALL, 3);
	trap_lightstyle(0, "m");
}

//============================================================================

void nopain(gedict_t *attacker, float damage)
{
	self->s.v.health = 40000;		// kill by telefrag
}

//============================================================================

/*QUAKED monster_oldone (1 0 0) (-16 -16 -24) (16 16 32)
 */
void SP_monster_oldone(void)
{
	if (!AllowMonster(self))
	{
		ent_remove(self);

		return;
	}

	trap_precache_model("progs/oldone.mdl");

	trap_precache_sound("boss2/death.wav");
	trap_precache_sound("boss2/idle.wav");
	trap_precache_sound("boss2/sight.wav");
	trap_precache_sound("boss2/pop2.wav");

	self->s.v.solid = SOLID_SLIDEBOX;
	self->s.v.movetype = MOVETYPE_STEP;
	self->s.v.effects = (int)self->s.v.effects | EF_RED;

	setmodel(self, "progs/oldone.mdl");
	setsize(self, -160, -128, -24, 160, 128, 256);

	self->s.v.health = 40000;		// kill by telefrag
	self->think = (func_t) old_idle1;
	self->s.v.nextthink = g_globalvars.time + 0.1;
	self->s.v.takedamage = DAMAGE_YES;
	self->th_pain = nopain;
	self->th_die = finale_1;

	g_globalvars.total_monsters++;
}
