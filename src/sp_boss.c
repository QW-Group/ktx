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

 BOSS-ONE

 ==============================================================================
 */

#include "g_local.h"

enum
{
	rise1,
	rise2,
	rise3,
	rise4,
	rise5,
	rise6,
	rise7,
	rise8,
	rise9,
	rise10,
	rise11,
	rise12,
	rise13,
	rise14,
	rise15,
	rise16,
	rise17,

	walk1,
	walk2,
	walk3,
	walk4,
	walk5,
	walk6,
	walk7,
	walk8,
	walk9,
	walk10,
	walk11,
	walk12,
	walk13,
	walk14,
	walk15,
	walk16,
	walk17,
	walk18,
	walk19,
	walk20,
	walk21,
	walk22,
	walk23,
	walk24,
	walk25,
	walk26,
	walk27,
	walk28,
	walk29,
	walk30,
	walk31,

	death1,
	death2,
	death3,
	death4,
	death5,
	death6,
	death7,
	death8,
	death9,

	attack1,
	attack2,
	attack3,
	attack4,
	attack5,
	attack6,
	attack7,
	attack8,
	attack9,
	attack10,
	attack11,
	attack12,
	attack13,
	attack14,
	attack15,
	attack16,
	attack17,
	attack18,
	attack19,
	attack20,
	attack21,
	attack22,
	attack23,

	shocka1,
	shocka2,
	shocka3,
	shocka4,
	shocka5,
	shocka6,
	shocka7,
	shocka8,
	shocka9,
	shocka10,

	shockb1,
	shockb2,
	shockb3,
	shockb4,
	shockb5,
	shockb6,

	shockc1,
	shockc2,
	shockc3,
	shockc4,
	shockc5,
	shockc6,
	shockc7,
	shockc8,
	shockc9,
	shockc10,

};

void boss_rise1();
void boss_rise2();
void boss_rise3();
void boss_rise4();
void boss_rise5();
void boss_rise6();
void boss_rise7();
void boss_rise8();
void boss_rise9();
void boss_rise10();
void boss_rise11();
void boss_rise12();
void boss_rise13();
void boss_rise14();
void boss_rise15();
void boss_rise16();
void boss_rise17();
void boss_idle1();
void boss_idle2();
void boss_idle3();
void boss_idle4();
void boss_idle5();
void boss_idle6();
void boss_idle7();
void boss_idle8();
void boss_idle9();
void boss_idle10();
void boss_idle11();
void boss_idle12();
void boss_idle13();
void boss_idle14();
void boss_idle15();
void boss_idle16();
void boss_idle17();
void boss_idle18();
void boss_idle19();
void boss_idle20();
void boss_idle21();
void boss_idle22();
void boss_idle23();
void boss_idle24();
void boss_idle25();
void boss_idle26();
void boss_idle27();
void boss_idle28();
void boss_idle29();
void boss_idle30();
void boss_idle31();
void boss_missile1();
void boss_missile2();
void boss_missile3();
void boss_missile4();
void boss_missile5();
void boss_missile6();
void boss_missile7();
void boss_missile8();
void boss_missile9();
void boss_missile10();
void boss_missile11();
void boss_missile12();
void boss_missile13();
void boss_missile14();
void boss_missile15();
void boss_missile16();
void boss_missile17();
void boss_missile18();
void boss_missile19();
void boss_missile20();
void boss_missile21();
void boss_missile22();
void boss_missile23();
void boss_shocka1();
void boss_shocka2();
void boss_shocka3();
void boss_shocka4();
void boss_shocka5();
void boss_shocka6();
void boss_shocka7();
void boss_shocka8();
void boss_shocka9();
void boss_shocka10();
void boss_shockb1();
void boss_shockb2();
void boss_shockb3();
void boss_shockb4();
void boss_shockb5();
void boss_shockb6();
void boss_shockb7();
void boss_shockb8();
void boss_shockb9();
void boss_shockb10();
void boss_shockc1();
void boss_shockc2();
void boss_shockc3();
void boss_shockc4();
void boss_shockc5();
void boss_shockc6();
void boss_shockc7();
void boss_shockc8();
void boss_shockc9();
void boss_shockc10();
void boss_death1();
void boss_death2();
void boss_death3();
void boss_death4();
void boss_death5();
void boss_death6();
void boss_death7();
void boss_death8();
void boss_death9();
void boss_death10();

//=============================================================================

ANIM(boss_rise1, rise1, boss_rise2; sound( self, CHAN_WEAPON, "boss1/out1.wav", 1, ATTN_NORM);)
ANIM(boss_rise2, rise2, boss_rise3; sound( self, CHAN_VOICE, "boss1/sight1.wav", 1, ATTN_NORM);)
ANIM(boss_rise3, rise3, boss_rise4;)
ANIM(boss_rise4, rise4, boss_rise5;)
ANIM(boss_rise5, rise5, boss_rise6;)
ANIM(boss_rise6, rise6, boss_rise7;)
ANIM(boss_rise7, rise7, boss_rise8;)
ANIM(boss_rise8, rise8, boss_rise9;)
ANIM(boss_rise9, rise9, boss_rise10;)
ANIM(boss_rise10, rise10, boss_rise11;)
ANIM(boss_rise11, rise11, boss_rise12;)
ANIM(boss_rise12, rise12, boss_rise13;)
ANIM(boss_rise13, rise13, boss_rise14;)
ANIM(boss_rise14, rise14, boss_rise15;)
ANIM(boss_rise15, rise15, boss_rise16;)
ANIM(boss_rise16, rise16, boss_rise17;)
ANIM(boss_rise17, rise17, boss_missile1;)

void boss_face()
{
	gedict_t *e = PROG_TO_EDICT(self->s.v.enemy);

	// go for another player if multi player
	if ((e == world) || ISDEAD(e) || (g_random() < 0.02))
	{
		// FIXME: search alive players... but its minor, since on next frame boss start searching for new player
		if (!(e = find_plr(e))) // first attempt
		{
			e = find_plr(world); // last resort
		}

		e = (e ? e : world);

		self->s.v.enemy = EDICT_TO_PROG(e);
	}

	ai_face();
}

void _boss_idle1(void)
{
	boss_face();

	if (ISLIVE(PROG_TO_EDICT(self->s.v.enemy)))
	{
		boss_missile1();
	}
}

ANIM(boss_idle1, walk1, boss_idle2; _boss_idle1();)
ANIM(boss_idle2, walk2, boss_idle3; boss_face();)
ANIM(boss_idle3, walk3, boss_idle4; boss_face();)
ANIM(boss_idle4, walk4, boss_idle5; boss_face();)
ANIM(boss_idle5, walk5, boss_idle6; boss_face();)
ANIM(boss_idle6, walk6, boss_idle7; boss_face();)
ANIM(boss_idle7, walk7, boss_idle8; boss_face();)
ANIM(boss_idle8, walk8, boss_idle9; boss_face();)
ANIM(boss_idle9, walk9, boss_idle10; boss_face();)
ANIM(boss_idle10, walk10, boss_idle11; boss_face();)
ANIM(boss_idle11, walk11, boss_idle12; boss_face();)
ANIM(boss_idle12, walk12, boss_idle13; boss_face();)
ANIM(boss_idle13, walk13, boss_idle14; boss_face();)
ANIM(boss_idle14, walk14, boss_idle15; boss_face();)
ANIM(boss_idle15, walk15, boss_idle16; boss_face();)
ANIM(boss_idle16, walk16, boss_idle17; boss_face();)
ANIM(boss_idle17, walk17, boss_idle18; boss_face();)
ANIM(boss_idle18, walk18, boss_idle19; boss_face();)
ANIM(boss_idle19, walk19, boss_idle20; boss_face();)
ANIM(boss_idle20, walk20, boss_idle21; boss_face();)
ANIM(boss_idle21, walk21, boss_idle22; boss_face();)
ANIM(boss_idle22, walk22, boss_idle23; boss_face();)
ANIM(boss_idle23, walk23, boss_idle24; boss_face();)
ANIM(boss_idle24, walk24, boss_idle25; boss_face();)
ANIM(boss_idle25, walk25, boss_idle26; boss_face();)
ANIM(boss_idle26, walk26, boss_idle27; boss_face();)
ANIM(boss_idle27, walk27, boss_idle28; boss_face();)
ANIM(boss_idle28, walk28, boss_idle29; boss_face();)
ANIM(boss_idle29, walk29, boss_idle30; boss_face();)
ANIM(boss_idle30, walk30, boss_idle31; boss_face();)
ANIM(boss_idle31, walk31, boss_idle1; boss_face();)

extern void T_MissileTouch();

void boss_missile(float p_x, float p_y, float p_z)
{
	vec3_t offang;
	vec3_t org, vec, d;
	float t;

	VectorSubtract(PROG_TO_EDICT(self->s.v.enemy)->s.v.origin, self->s.v.origin, offang);
	vectoangles(offang, offang);
	trap_makevectors(offang);

	//org = self->s.v.origin + p[0]*v_forward + p[1]*v_right + p[2]*'0 0 1';
	VectorMA(self->s.v.origin, p_x, g_globalvars.v_forward, org);
	VectorMA(org, p_y, g_globalvars.v_right, org);
	org[2] += p_z;

	// lead the player on hard mode
	if (skill > 1)
	{
		VectorSubtract(PROG_TO_EDICT(self->s.v.enemy)->s.v.origin, org, vec);
		t = vlen(vec) / 300;
		VectorCopy(PROG_TO_EDICT(self->s.v.enemy)->s.v.velocity, vec);
		vec[2] = 0;
		VectorMA( PROG_TO_EDICT(self->s.v.enemy)->s.v.origin, t, vec, d);
	}
	else
	{
		VectorCopy(PROG_TO_EDICT(self->s.v.enemy)->s.v.origin, d);
	}

	VectorSubtract(d, org, vec);
	normalize(vec, vec);

	launch_spike(org, vec);
	setmodel(newmis, "progs/lavaball.mdl");
	SetVector(newmis->s.v.avelocity, 200, 100, 300);
	setsize(newmis, PASSVEC3(VEC_ORIGIN), PASSVEC3(VEC_ORIGIN));
	VectorScale(vec, 300, newmis->s.v.velocity);
	newmis->touch = (func_t) T_MissileTouch; // rocket explosion
	sound(self, CHAN_WEAPON, "boss1/throw.wav", 1, ATTN_NORM);

	// check for dead enemy
	if (ISDEAD(PROG_TO_EDICT(self->s.v.enemy)))
	{
		boss_idle1();
	}
}

ANIM(boss_missile1, attack1, boss_missile2; boss_face();)
ANIM(boss_missile2, attack2, boss_missile3; boss_face();)
ANIM(boss_missile3, attack3, boss_missile4; boss_face();)
ANIM(boss_missile4, attack4, boss_missile5; boss_face();)
ANIM(boss_missile5, attack5, boss_missile6; boss_face();)
ANIM(boss_missile6, attack6, boss_missile7; boss_face();)
ANIM(boss_missile7, attack7, boss_missile8; boss_face();)
ANIM(boss_missile8, attack8, boss_missile9; boss_face();)
ANIM(boss_missile9, attack9, boss_missile10; boss_missile( 100, 100, 200 );)
ANIM(boss_missile10, attack10, boss_missile11; boss_face();)
ANIM(boss_missile11, attack11, boss_missile12; boss_face();)
ANIM(boss_missile12, attack12, boss_missile13; boss_face();)
ANIM(boss_missile13, attack13, boss_missile14; boss_face();)
ANIM(boss_missile14, attack14, boss_missile15; boss_face();)
ANIM(boss_missile15, attack15, boss_missile16; boss_face();)
ANIM(boss_missile16, attack16, boss_missile17; boss_face();)
ANIM(boss_missile17, attack17, boss_missile18; boss_face();)
ANIM(boss_missile18, attack18, boss_missile19; boss_face();)
ANIM(boss_missile19, attack19, boss_missile20; boss_face();)
ANIM(boss_missile20, attack20, boss_missile21; boss_missile( 100, -100, 200 );)
ANIM(boss_missile21, attack21, boss_missile22; boss_face();)
ANIM(boss_missile22, attack22, boss_missile23; boss_face();)
ANIM(boss_missile23, attack23, boss_missile1; boss_face();)

ANIM(boss_shocka1, shocka1, boss_shocka2;)
ANIM(boss_shocka2, shocka2, boss_shocka3;)
ANIM(boss_shocka3, shocka3, boss_shocka4;)
ANIM(boss_shocka4, shocka4, boss_shocka5;)
ANIM(boss_shocka5, shocka5, boss_shocka6;)
ANIM(boss_shocka6, shocka6, boss_shocka7;)
ANIM(boss_shocka7, shocka7, boss_shocka8;)
ANIM(boss_shocka8, shocka8, boss_shocka9;)
ANIM(boss_shocka9, shocka9, boss_shocka10;)
ANIM(boss_shocka10, shocka10, boss_missile1;)

ANIM(boss_shockb1, shockb1, boss_shockb2;)
ANIM(boss_shockb2, shockb2, boss_shockb3;)
ANIM(boss_shockb3, shockb3, boss_shockb4;)
ANIM(boss_shockb4, shockb4, boss_shockb5;)
ANIM(boss_shockb5, shockb5, boss_shockb6;)
ANIM(boss_shockb6, shockb6, boss_shockb7;)
ANIM(boss_shockb7, shockb1, boss_shockb8;)
ANIM(boss_shockb8, shockb2, boss_shockb9;)
ANIM(boss_shockb9, shockb3, boss_shockb10;)
ANIM(boss_shockb10, shockb4, boss_missile1;)

ANIM(boss_shockc1, shockc1, boss_shockc2;)
ANIM(boss_shockc2, shockc2, boss_shockc3;)
ANIM(boss_shockc3, shockc3, boss_shockc4;)
ANIM(boss_shockc4, shockc4, boss_shockc5;)
ANIM(boss_shockc5, shockc5, boss_shockc6;)
ANIM(boss_shockc6, shockc6, boss_shockc7;)
ANIM(boss_shockc7, shockc7, boss_shockc8;)
ANIM(boss_shockc8, shockc8, boss_shockc9;)
ANIM(boss_shockc9, shockc9, boss_shockc10;)
ANIM(boss_shockc10, shockc10, boss_death1;)

void _boss_death9(void)
{
	sound(self, CHAN_BODY, "boss1/out1.wav", 1, ATTN_NORM);
	WriteByte( MSG_BROADCAST, SVC_TEMPENTITY);
	WriteByte( MSG_BROADCAST, TE_LAVASPLASH);
	WriteCoord( MSG_BROADCAST, self->s.v.origin[0]);
	WriteCoord( MSG_BROADCAST, self->s.v.origin[1]);
	WriteCoord( MSG_BROADCAST, self->s.v.origin[2]);
}
void _boss_death10(void)
{
	g_globalvars.killed_monsters++;
	WriteByte( MSG_ALL, SVC_KILLEDMONSTER);	// FIXME: reliable broadcast
	SUB_UseTargets();
	ent_remove(self);
}

ANIM(boss_death1, death1, boss_death2; sound( self, CHAN_VOICE, "boss1/death.wav", 1, ATTN_NORM );)
ANIM(boss_death2, death2, boss_death3;)
ANIM(boss_death3, death3, boss_death4;)
ANIM(boss_death4, death4, boss_death5;)
ANIM(boss_death5, death5, boss_death6;)
ANIM(boss_death6, death6, boss_death7;)
ANIM(boss_death7, death7, boss_death8;)
ANIM(boss_death8, death8, boss_death9;)
ANIM(boss_death9, death9, boss_death10; _boss_death9();)
ANIM(boss_death10, death9, boss_death10; _boss_death10();)

void boss_awake()
{
	self->s.v.solid = SOLID_SLIDEBOX;
	self->s.v.movetype = MOVETYPE_STEP;
	self->s.v.takedamage = DAMAGE_NO;

	setmodel(self, "progs/boss.mdl");
	setsize(self, -128, -128, -24, 128, 128, 256);

	self->s.v.health = (!skill ? 1 : 3);
	self->s.v.enemy = EDICT_TO_PROG(activator);

	WriteByte( MSG_BROADCAST, SVC_TEMPENTITY);
	WriteByte( MSG_BROADCAST, TE_LAVASPLASH);
	WriteCoord( MSG_BROADCAST, self->s.v.origin[0]);
	WriteCoord( MSG_BROADCAST, self->s.v.origin[1]);
	WriteCoord( MSG_BROADCAST, self->s.v.origin[2]);

	self->s.v.yaw_speed = 20;
	boss_rise1();
}

//=============================================================================

/*QUAKED monster_boss (1 0 0) (-128 -128 -24) (128 128 256)
 */
void SP_monster_boss()
{
	if (deathmatch)
	{
		ent_remove(self);

		return;
	}

	trap_precache_model("progs/boss.mdl");
	trap_precache_model("progs/lavaball.mdl");

	trap_precache_sound("weapons/rocket1i.wav");
	trap_precache_sound("boss1/out1.wav");
	trap_precache_sound("boss1/sight1.wav");
	trap_precache_sound("misc/power.wav");
	trap_precache_sound("boss1/throw.wav");
	trap_precache_sound("boss1/pain.wav");
	trap_precache_sound("boss1/death.wav");

	g_globalvars.total_monsters++;

	self->use = (func_t) boss_awake;
}

//===========================================================================

static float lightning_end;

extern void door_go_down();
void lightning_fire()
{
	gedict_t *le1, *le2, *oself;
	vec3_t p1, p2, tmpv;

	le1 = le2 = NULL;
	le1 = find(world, FOFS(target), "lightning");

	if (le1)
	{
		le2 = find(le1, FOFS(target), "lightning");
	}

	if (!le1 || !le2)
	{
//		G_bprint( 2, "missing lightning targets\n");

		return;
	}

	if (g_globalvars.time >= lightning_end)
	{
		// done here, put the terminals back up
		oself = self; // save self

		self = le1;
		door_go_down();

		self = le2;
		door_go_down();

		self = oself; // restore self

		return;
	}

	//p1 = (le1.mins + le1.maxs) * 0.5;
	VectorAdd(le1->s.v.mins, le1->s.v.maxs, p1);
	VectorScale(p1, 0.5, p1);
	p1[2] = le1->s.v.absmin[2] - 16;

	//p2 = (le2.mins + le2.maxs) * 0.5;
	VectorAdd(le2->s.v.mins, le2->s.v.maxs, p2);
	VectorScale(p2, 0.5, p2);
	p2[2] = le2->s.v.absmin[2] - 16;

	// compensate for length of bolt
	//p2 = p2 - normalize( p2 - p1 ) * 100;
	VectorSubtract(p2, p1, tmpv);
	normalize(tmpv, tmpv);
	VectorMA(p2, -100, tmpv, p2);

	self->s.v.nextthink = g_globalvars.time + 0.1;
	self->think = (func_t) lightning_fire;

	WriteByte( MSG_ALL, SVC_TEMPENTITY);
	WriteByte( MSG_ALL, TE_LIGHTNING3);
	WriteEntity( MSG_ALL, world);
	WriteCoord( MSG_ALL, p1[0]);
	WriteCoord( MSG_ALL, p1[1]);
	WriteCoord( MSG_ALL, p1[2]);
	WriteCoord( MSG_ALL, p2[0]);
	WriteCoord( MSG_ALL, p2[1]);
	WriteCoord( MSG_ALL, p2[2]);
}

void lightning_use()
{
	gedict_t *le1, *le2, *oself, *boss;

	if (lightning_end >= (g_globalvars.time + 1))
	{
		return;
	}

	le1 = le2 = NULL;
	le1 = find(world, FOFS(target), "lightning");

	if (le1)
	{
		le2 = find(le1, FOFS(target), "lightning");
	}

	if (!le1 || !le2)
	{
//		G_bprint( 2, "missing lightning targets\n");

		return;
	}

	if ((le1->state != STATE_TOP && le1->state != STATE_BOTTOM)
			|| (le2->state != STATE_TOP && le2->state != STATE_BOTTOM)
			|| (le1->state != le2->state))
	{
//		dprint ("not aligned\n");

		return;
	}

	// don't let the electrodes go back up until the bolt is done
	le1->s.v.nextthink = -1;
	le2->s.v.nextthink = -1;
	lightning_end = g_globalvars.time + 1;

	sound(self, CHAN_VOICE, "misc/power.wav", 1, ATTN_NORM);
	lightning_fire();

	// advance the boss pain if down
	boss = ez_find(world, "monster_boss");
	if (!boss)
	{
		return;
	}

	oself = self; // save self
	self = boss;

	self->s.v.enemy = EDICT_TO_PROG(activator);

	if (le1->state == STATE_TOP && ISLIVE(self))
	{
		sound(self, CHAN_VOICE, "boss1/pain.wav", 1, ATTN_NORM);
		self->s.v.health--;

		if (self->s.v.health >= 2)
		{
			boss_shocka1();
		}
		else if (self->s.v.health == 1)
		{
			boss_shockb1();
		}
		else
		{
			boss_shockc1();
		}
	}

	self = oself; // restore self
}

/*QUAKED event_lightning (0 1 1) (-16 -16 -16) (16 16 16)
 Just for boss level.
 */
void SP_event_lightning()
{
	self->use = (func_t)lightning_use;
}
