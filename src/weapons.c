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

void ReportMe(void);
void AdminImpBot(void);
void CaptainPickPlayer(void);
void ChasecamToggleButton(void);
void ClanArenaTrackingToggleButton(void);

// Bots support
void BotsRocketSpawned(gedict_t *newmis);
void BotsGrenadeSpawned(gedict_t *newmis);

void AmmoUsed(gedict_t *player)
{
#ifdef BOT_SUPPORT
	if (player->fb.ammo_used)
	{
		player->fb.ammo_used(player);
	}
#endif
}

// called by SP_worldspawn
void W_Precache(void)
{
	trap_precache_sound("weapons/r_exp3.wav");	// new rocket explosion
	trap_precache_sound("weapons/rocket1i.wav");	// spike gun
	trap_precache_sound("weapons/sgun1.wav");
	trap_precache_sound("weapons/guncock.wav");	// player shotgun
	trap_precache_sound("weapons/ric1.wav");	// ricochet (used in c code)
	trap_precache_sound("weapons/ric2.wav");	// ricochet (used in c code)
	trap_precache_sound("weapons/ric3.wav");	// ricochet (used in c code)
	trap_precache_sound("weapons/spike2.wav");	// super spikes
	trap_precache_sound("weapons/tink1.wav");	// spikes tink (used in c code)
	trap_precache_sound("weapons/grenade.wav");	// grenade launcher
	trap_precache_sound("weapons/bounce.wav");	// grenade bounce
	trap_precache_sound("weapons/shotgn2.wav");	// super shotgun

	if (cvar("k_instagib_custom_models"))
	{
		trap_precache_sound("weapons/coilgun.wav");	// player railgun for instagib
	}
}

void W_FireSpikes(float ox);
void W_FireLightning(void);

qbool SendEntity_Projectile(int sendflags)
{
	WriteByte(MSG_CSQC, EZCSQC_PROJECTILE);

	if (self->pos1[0] == 0 && self->pos1[1] == 0 && self->pos1[2] == 0)
	{
		sendflags &= ~16; // if our pos1 is blank, don't send it.
	}

	WriteByte(MSG_CSQC, sendflags);

	if (sendflags & PROJECTILE_ORIGIN)
	{
		WriteCoord(MSG_CSQC, self->s.v.origin[0]);
		WriteCoord(MSG_CSQC, self->s.v.origin[1]);
		WriteCoord(MSG_CSQC, self->s.v.origin[2]);

		WriteCoord(MSG_CSQC, self->s.v.velocity[0]);
		WriteCoord(MSG_CSQC, self->s.v.velocity[1]);
		WriteCoord(MSG_CSQC, self->s.v.velocity[2]);

		WriteFloat(MSG_CSQC, g_globalvars.time);
	}


	if (sendflags & PROJECTILE_MODEL)
	{
		WriteShort(MSG_CSQC, self->s.v.modelindex);
		WriteShort(MSG_CSQC, self->s.v.effects);
	}


	if (sendflags & PROJECTILE_ANGLES)
	{
		WriteAngle(MSG_CSQC, self->s.v.angles[0]);
		WriteAngle(MSG_CSQC, self->s.v.angles[1]);
		WriteAngle(MSG_CSQC, self->s.v.angles[2]);
	}

	if (sendflags & PROJECTILE_OWNER)
	{
		WriteEntity(MSG_CSQC, PROG_TO_EDICT(self->s.v.owner)); // we only care about the owner if it's a player, otherwise world
	}

	if (sendflags & PROJECTILE_SPAWN_ORIGIN)
	{
		WriteCoord(MSG_CSQC, self->pos1[0]);
		WriteCoord(MSG_CSQC, self->pos1[1]);
		WriteCoord(MSG_CSQC, self->pos1[2]);
	}

	return true;
}


/*
 ================
 W_FireAxe
 ================
 */
void W_FireAxe(void)
{
	vec3_t source, dest;
	vec3_t org;

	antilag_lagmove_all_hitscan(self);

	WS_Mark(self, wpAXE);

	self->ps.wpn[wpAXE].attacks++;

	trap_makevectors(self->s.v.v_angle);

	VectorCopy(self->s.v.origin, source);
	source[2] += 16;
	VectorScale(g_globalvars.v_forward, 64, dest);
	VectorAdd(dest, source, dest)
	//source = self->s.v.origin + '0 0 16';

	traceline(PASSVEC3(source), PASSVEC3(dest), false, self);
	if (g_globalvars.trace_fraction == 1.0)
	{
		antilag_clearflags_all();
		antilag_unmove_all();
		return;
	}

	antilag_unmove_all();

	VectorScale(g_globalvars.v_forward, 4, org);
	VectorSubtract(g_globalvars.trace_endpos, org, org);
// org = trace_endpos - v_forward*4;

	if ( PROG_TO_EDICT(g_globalvars.trace_ent)->s.v.takedamage)
	{
		int damage = 20; // default damage is 20

		// can't touch/damage others in race
		if (isRACE() && (PROG_TO_EDICT(g_globalvars.trace_ent)->ct == ctPlayer)
				&& (self != PROG_TO_EDICT(g_globalvars.trace_ent)))
		{
			return;
		}

		if (PROG_TO_EDICT(g_globalvars.trace_ent)->ct == ctPlayer)
		{
			WS_Mark(self, wpAXE);
			self->ps.wpn[wpAXE].hits++;
		}

		if (deathmatch > 3)
		{
			damage = 75;
		}
		else if (deathmatch == 3)
		{
			damage = k_yawnmode ? 50 : 20; // Yawnmode: 50 axe dmg in dmm3
		}

		PROG_TO_EDICT(g_globalvars.trace_ent)->axhitme = 1;
		SpawnBlood(org, damage);
		PROG_TO_EDICT(g_globalvars.trace_ent)->deathtype = dtAXE;

		T_Damage(PROG_TO_EDICT(g_globalvars.trace_ent), self, self, damage);
	}
	else
	{
		// hit wall

		//crt - get rid of axe sound for spec
		if (!isRA() || (isWinner(self) || isLoser(self)))
		{
			sound(self, CHAN_WEAPON, "player/axhit2.wav", 1, ATTN_NORM);
		}

		WriteByte( MSG_MULTICAST, SVC_TEMPENTITY);
		WriteByte( MSG_MULTICAST, TE_GUNSHOT);
		WriteByte( MSG_MULTICAST, 3);
		WriteCoord( MSG_MULTICAST, org[0]);
		WriteCoord( MSG_MULTICAST, org[1]);
		WriteCoord( MSG_MULTICAST, org[2]);
		trap_multicast(PASSVEC3(org), MULTICAST_PVS);
	}

	antilag_clearflags_all();
}

//============================================================================

//============================================================================

void wall_velocity(vec3_t v)
{
	vec3_t vel;

	normalize(self->s.v.velocity, vel);

	vel[0] += g_globalvars.v_up[0] * (g_random() - 0.5)
			+ g_globalvars.v_right[0] * (g_random() - 0.5);

	vel[1] += g_globalvars.v_up[1] * (g_random() - 0.5)
			+ g_globalvars.v_right[1] * (g_random() - 0.5);
	vel[2] += g_globalvars.v_up[2] * (g_random() - 0.5)
			+ g_globalvars.v_right[2] * (g_random() - 0.5);

	VectorNormalize(vel);

	vel[0] += 2 * g_globalvars.trace_plane_normal[0];
	vel[1] += 2 * g_globalvars.trace_plane_normal[1];
	vel[2] += 2 * g_globalvars.trace_plane_normal[2];

	VectorScale(vel, 200, v);

	//return vel;
}

/*
 ================
 SpawnMeatSpray
 ================
 */
void SpawnMeatSpray(vec3_t org, vec3_t vel)
{
	gedict_t *missile;

	//vec3_t  org;

	missile = spawn();
	missile->s.v.owner = EDICT_TO_PROG(self);
	missile->s.v.movetype = MOVETYPE_BOUNCE;
	missile->isMissile = true;
	missile->s.v.solid = SOLID_NOT;

	trap_makevectors(self->s.v.angles);

	VectorCopy(vel, missile->s.v.velocity);
// missile->s.v.velocity = vel;
	missile->s.v.velocity[2] = missile->s.v.velocity[2] + 250 + 50 * g_random();

	SetVector(missile->s.v.avelocity, 3000, 1000, 2000);
// missile.avelocity = '3000 1000 2000';

// set missile duration
	missile->s.v.nextthink = g_globalvars.time + 1;
	missile->think = (func_t) SUB_Remove;

	setmodel(missile, "progs/zom_gib.mdl");
	setsize(missile, 0, 0, 0, 0, 0, 0);
	setorigin(missile, PASSVEC3(org));
}

/*
 ================
 SpawnBlood
 ================
 */
void SpawnBlood(vec3_t org, float damage)
{
	if (damage < 1)
	{
		return;
	}

	WriteByte( MSG_MULTICAST, SVC_TEMPENTITY);
	WriteByte( MSG_MULTICAST, TE_BLOOD);
	WriteByte( MSG_MULTICAST, damage);
	WriteCoord( MSG_MULTICAST, org[0]);
	WriteCoord( MSG_MULTICAST, org[1]);
	WriteCoord( MSG_MULTICAST, org[2]);
	trap_multicast(PASSVEC3(org), MULTICAST_PVS);
}

/*
 ================
 spawn_touchblood
 ================
 */
void spawn_touchblood(float damage)
{
	vec3_t vel;

	wall_velocity(vel);
	VectorScale(vel, 0.2 * 0.01, vel);
	VectorAdd(self->s.v.origin, vel, vel);
	SpawnBlood(vel, damage);
}

/*
 ==============================================================================

 MULTI-DAMAGE

 Collects multiple small damages into a single damage

 ==============================================================================
 */
gedict_t *multi_ent;
float multi_damage;
deathType_t multi_damage_type;

vec3_t blood_org;
float blood_count;

vec3_t puff_org;
float puff_count;

void ClearMultiDamage(void)
{
	multi_ent = world;
	multi_damage = 0;
	blood_count = 0;
	puff_count = 0;
	multi_damage_type = dtNONE;
}

void ApplyMultiDamage(void)
{
	if (multi_ent == world)
	{
		return;
	}

	multi_ent->deathtype = multi_damage_type;
	T_Damage(multi_ent, self, self, multi_damage);
}

void AddMultiDamage(gedict_t *hit, float damage)
{
	if (!hit)
	{
		return;
	}

	if (hit != multi_ent)
	{
		ApplyMultiDamage();
		multi_damage = damage;
		multi_ent = hit;
	}
	else
	{
		multi_damage = multi_damage + damage;
	}
}

void Multi_Finish(void)
{
	if (puff_count)
	{
		WriteByte( MSG_MULTICAST, SVC_TEMPENTITY);
		WriteByte( MSG_MULTICAST, TE_GUNSHOT);
		WriteByte( MSG_MULTICAST, puff_count);
		WriteCoord( MSG_MULTICAST, puff_org[0]);
		WriteCoord( MSG_MULTICAST, puff_org[1]);
		WriteCoord( MSG_MULTICAST, puff_org[2]);
		trap_multicast(PASSVEC3(puff_org), MULTICAST_PVS);
	}

	SpawnBlood(puff_org, blood_count);
}

void CoilgunTrail(vec3_t org, vec3_t endpos, int entnum, int color)
{
	if ((color < 1) || (color > 7))
	{
		color = 1;
	}

	if ((entnum < 0) || (entnum > MAX_CLIENTS))
	{
		entnum = 0;
	}

	WriteByte( MSG_MULTICAST, SVC_TEMPENTITY);
	WriteByte( MSG_MULTICAST, TE_LIGHTNING1);
	WriteShort( MSG_MULTICAST, -264 + 8 * entnum + color);
	WriteCoord( MSG_MULTICAST, org[0]);
	WriteCoord( MSG_MULTICAST, org[1]);
	WriteCoord( MSG_MULTICAST, org[2]);
	WriteCoord( MSG_MULTICAST, endpos[0]);
	WriteCoord( MSG_MULTICAST, endpos[1]);
	WriteCoord( MSG_MULTICAST, endpos[2]);

	trap_multicast(PASSVEC3(org), MULTICAST_PHS);
}
/*
 ==============================================================================
 BULLETS
 ==============================================================================
 */
/*
 ================
 TraceAttack
 ================
 */
void TraceAttack(float damage, vec3_t dir, qbool send_effects)
{
	vec3_t org, tmp;

	VectorScale(dir, 4, tmp);
	VectorSubtract(g_globalvars.trace_endpos, tmp, org);
// org = trace_endpos - dir*4;

	// can't touch/damage others in race
	if (isRACE() && (PROG_TO_EDICT(g_globalvars.trace_ent)->ct == ctPlayer)
			&& (self != PROG_TO_EDICT(g_globalvars.trace_ent)))
	{
		return;
	}

	if (PROG_TO_EDICT(g_globalvars.trace_ent)->s.v.takedamage)
	{
		if (PROG_TO_EDICT(g_globalvars.trace_ent)->ct == ctPlayer)
		{
			if ((int)self->s.v.weapon == IT_SHOTGUN)
			{
				WS_Mark(self, wpSG);
				self->ps.wpn[wpSG].hits++;
			}
			else if ((int)self->s.v.weapon == IT_SUPER_SHOTGUN)
			{
				WS_Mark(self, wpSSG);
				self->ps.wpn[wpSSG].hits++;
			}
			else
			{
				; // hmmm, make error?
			}
		}
		blood_count = blood_count + 1;
		VectorCopy(org, blood_org);	//  blood_org = org;
		AddMultiDamage(PROG_TO_EDICT(g_globalvars.trace_ent), damage);
		if (send_effects)
		{
			SpawnBlood(org, 1);
		}
	}
	else
	{
		puff_count = puff_count + 1;
		if (send_effects)
		{
			WriteByte( MSG_MULTICAST, SVC_TEMPENTITY);
			WriteByte( MSG_MULTICAST, TE_GUNSHOT);
			WriteByte( MSG_MULTICAST, 1);
			WriteCoord( MSG_MULTICAST, org[0]);
			WriteCoord( MSG_MULTICAST, org[1]);
			WriteCoord( MSG_MULTICAST, org[2]);
			trap_multicast(PASSVEC3(puff_org), MULTICAST_PVS);
		}
	}
}

/*
 ================
 FireInstaBullet

 Used by coilgun for Instagib mode, bullet doesn't stop at first player, it goes through till it hits wall
 ================
 */
void T_InstaKickback(void);
void FireInstaBullet(vec3_t dir, deathType_t deathtype)
{
	vec3_t src, dst, tmp;
	int depth, solid;
	float fraction;
	gedict_t *ignore;

	if (cvar("k_cg_kb"))
	{
		newmis = spawn();
		g_globalvars.newmis = EDICT_TO_PROG(newmis);
		newmis->s.v.owner = EDICT_TO_PROG(self);
		newmis->s.v.movetype = MOVETYPE_FLYMISSILE;
		newmis->isMissile = true;
		newmis->s.v.solid = SOLID_BBOX;

		trap_makevectors(self->s.v.v_angle);
		aim(newmis->s.v.velocity);	// = aim(self, 1000);
		VectorScale(newmis->s.v.velocity, 1000, newmis->s.v.velocity);
		vectoangles(newmis->s.v.velocity, newmis->s.v.angles);
		newmis->touch = (func_t) T_InstaKickback;
		newmis->voided = 0;

		newmis->s.v.nextthink = g_globalvars.time + 1;
		newmis->think = (func_t) SUB_Remove;
		newmis->classname = "kickback";
		setmodel(newmis, "");
		setsize(newmis, 0, 0, 0, 0, 0, 0);

		setorigin(newmis, self->s.v.origin[0] + g_globalvars.v_forward[0] * 8,
					self->s.v.origin[1] + g_globalvars.v_forward[1] * 8,
					self->s.v.origin[2] + g_globalvars.v_forward[2] * 8 + 16);
	}

	ClearMultiDamage();
	multi_damage_type = deathtype;

	// init src
	trap_makevectors(self->s.v.v_angle);
	VectorScale(g_globalvars.v_forward, 10, tmp);
	VectorAdd(self->s.v.origin, tmp, src);
	src[2] = self->s.v.absmin[2] + self->s.v.size[2] * 0.7;

	for (ignore = self, depth = 0; depth < 32; depth++)
	{
//		G_sprint( self, 2, "depth %d\n", depth); // DEBUG!!!!!

		// init dst
		VectorScale(dir, 8192, tmp);
		VectorAdd(src, tmp, dst);

		traceline(PASSVEC3(src), PASSVEC3(dst), false, ignore);

		VectorScale(dir, 4, tmp);
		VectorSubtract(g_globalvars.trace_endpos, tmp, puff_org); // puff_org = trace_endpos - dir*4;

		// save trace info, since TraceAttack() may modifie it
		ignore = PROG_TO_EDICT(g_globalvars.trace_ent);
		fraction = g_globalvars.trace_fraction;
		solid = ignore->s.v.solid;

		// next interation we start traceline() from trace_endpos
		VectorCopy(g_globalvars.trace_endpos, src);

//		G_sprint( self, 2, "fraction %f\n", fraction); // DEBUG!!!!!
//		G_sprint( self, 2, "solid %d, %s\n", solid, ignore->netname); // DEBUG!!!!!

		TraceAttack(4, dir, false);

		// this is something like "fix" for one bullet hit more than one player?
		if (depth > 1)
		{
			WS_Mark(self, wpSG);
			self->ps.wpn[wpSG].hits--;

			if (depth > self->ps.i_maxmultigibs)
			{
				self->ps.i_maxmultigibs = depth;
			}
		}

		if (fraction == 1.0)
		{
			break; // no obstacle
		}

		if (solid != SOLID_SLIDEBOX)
		{
			break; // obstacle not a player/monster, probably something solid like wall or something
		}
	}

	if (depth > 1)
	{
		self->ps.i_multigibs++;
	}

	ApplyMultiDamage();
	Multi_Finish();

	VectorCopy(self->s.v.origin, tmp);	//tmp = self->s.v.origin + '0 0 16';
	tmp[2] += 16;
	CoilgunTrail(tmp, src, self - world, iKey(self, "railcolor"));
}

/*
 ================
 FireBullets

 Used by shotgun, super shotgun, and enemy soldier firing
 Go to the trouble of combining multiple pellets into a single damage call.
 ================
 */
void FireBullets(float shotcount, vec3_t dir, float spread_x, float spread_y, float spread_z,
					deathType_t deathtype)
{
	vec3_t direction;
	vec3_t src, tmp, tmp2;
	qbool classic_shotgun = cvar("k_classic_shotgun");
	qbool non_random_bullets = (k_yawnmode
			|| (!match_in_progress && self && (self->ct == ctPlayer) && iKey(self, "nrb")));
	qbool do_antilag = (self->ct == ctPlayer); // hacky check

	if (do_antilag)
	{
		antilag_lagmove_all_hitscan(self);
	}


	trap_makevectors(self->s.v.v_angle);
	VectorScale(g_globalvars.v_forward, 10, tmp);
	VectorAdd(self->s.v.origin, tmp, src);
	//src = self->s.v.origin + v_forward*10;
	src[2] = self->s.v.absmin[2] + self->s.v.size[2] * 0.7;

	ClearMultiDamage();
	multi_damage_type = deathtype;

	if (cvar("k_instagib"))
	{
		traceline(PASSVEC3(src), src[0] + dir[0] * 8192, src[1] + dir[1] * 8192,
					src[2] + dir[2] * 8192, false, self);
	}
	else
	{
		traceline(PASSVEC3(src), src[0] + dir[0] * 2048, src[1] + dir[1] * 2048,
					src[2] + dir[2] * 2048, false, self);
	}

	VectorScale(dir, 4, tmp);
	VectorSubtract(g_globalvars.trace_endpos, tmp, puff_org);	// puff_org = trace_endpos - dir*4;

	while (shotcount > 0)
	{
		if (non_random_bullets)
		{
			if (shotcount == 1)
			{
				VectorClear(tmp);
				VectorClear(tmp2);
			}
			else if (shotcount == 2)
			{
				VectorClear(tmp);
				VectorClear(tmp2);
			}
			else if (shotcount == 3)
			{
				VectorClear(tmp);
				VectorScale(g_globalvars.v_up, 1.00 * spread_y, tmp2);
			}
			else if (shotcount == 4)
			{
				VectorClear(tmp);
				VectorScale(g_globalvars.v_up, -1.00 * spread_y, tmp2);
			}
			else if (shotcount == 5)
			{
				VectorScale(g_globalvars.v_right, 1.00 * spread_x, tmp);
				VectorClear(tmp2);
			}
			else if (shotcount == 6)
			{
				VectorScale(g_globalvars.v_right, -1.00 * spread_x, tmp);
				VectorClear(tmp2);
			}
			else if (shotcount == 7)
			{
				VectorClear(tmp);
				VectorScale(g_globalvars.v_up, 0.50 * spread_y, tmp2);
			}
			else if (shotcount == 8)
			{
				VectorClear(tmp);
				VectorScale(g_globalvars.v_up, -0.50 * spread_y, tmp2);
			}
			else if (shotcount == 9)
			{
				VectorScale(g_globalvars.v_right, 0.50 * spread_x, tmp);
				VectorClear(tmp2);
			}
			else if (shotcount == 10)
			{
				VectorScale(g_globalvars.v_right, -0.50 * spread_x, tmp);
				VectorClear(tmp2);
			}
			else if (shotcount == 11)
			{
				VectorScale(g_globalvars.v_right, 0.50 * spread_x, tmp);
				VectorScale(g_globalvars.v_up, 0.50 * spread_y, tmp2);
			}
			else if (shotcount == 12)
			{
				VectorScale(g_globalvars.v_right, 0.50 * spread_x, tmp);
				VectorScale(g_globalvars.v_up, -0.50 * spread_y, tmp2);
			}
			else if (shotcount == 13)
			{
				VectorScale(g_globalvars.v_right, -0.50 * spread_x, tmp);
				VectorScale(g_globalvars.v_up, -0.50 * spread_y, tmp2);
			}
			else if (shotcount == 14)
			{
				VectorScale(g_globalvars.v_right, -0.50 * spread_x, tmp);
				VectorScale(g_globalvars.v_up, 0.50 * spread_y, tmp2);
			}
			else if (shotcount == 15)
			{
				VectorScale(g_globalvars.v_right, 0.25 * spread_x, tmp);
				VectorScale(g_globalvars.v_up, 0.25 * spread_y, tmp2);
			}
			else if (shotcount == 16)
			{
				VectorScale(g_globalvars.v_right, 0.25 * spread_x, tmp);
				VectorScale(g_globalvars.v_up, -0.25 * spread_y, tmp2);
			}
			else if (shotcount == 17)
			{
				VectorScale(g_globalvars.v_right, -0.25 * spread_x, tmp);
				VectorScale(g_globalvars.v_up, -0.25 * spread_y, tmp2);
			}
			else if (shotcount == 18)
			{
				VectorScale(g_globalvars.v_right, -0.25 * spread_x, tmp);
				VectorScale(g_globalvars.v_up, 0.25 * spread_y, tmp2);
			}
			else if (shotcount == 19)
			{
				VectorScale(g_globalvars.v_right, -1.00 * spread_x, tmp);
				VectorClear(tmp2);
			}
			else if (shotcount == 20)
			{
				VectorClear(tmp);
				VectorClear(tmp2);
			}
			else if (shotcount == 21)
			{
				VectorScale(g_globalvars.v_right, 1.00 * spread_x, tmp);
				VectorClear(tmp2);
			}
			else
			{
				// No design for this bullet, place it in the middle
				VectorClear(tmp);
				VectorClear(tmp2);
			}

			VectorAdd(dir, tmp, direction);
			VectorAdd(direction, tmp2, direction);
		}
		else
		{
			VectorScale(g_globalvars.v_right, crandom() * spread_x, tmp);
			VectorAdd(dir, tmp, direction);
			VectorScale(g_globalvars.v_up, crandom() * spread_y, tmp);
			VectorAdd(direction, tmp, direction);
		}

//		direction = dir + crandom()*spread[0]*v_right + crandom()*spread[1]*v_up;
		if (cvar("k_instagib"))
		{
			VectorScale(direction, 8192, tmp);
		}
		else
		{
			VectorScale(direction, 2048, tmp);
		}

		VectorAdd(src, tmp, tmp);
		traceline(PASSVEC3(src), PASSVEC3(tmp), false, self);
		if (g_globalvars.trace_fraction != 1.0)
		{
			TraceAttack(4, direction, classic_shotgun);
		}

		shotcount = shotcount - 1;
	}

	if (do_antilag) // unmove for proper knockback calculation
	{
		antilag_unmove_all();
	}

	ApplyMultiDamage();
	if (!classic_shotgun)
	{
		Multi_Finish();
	}

	if (do_antilag) // we must clear any flags pertaining to damage or knockback protection
	{
		antilag_clearflags_all();
	}
}

/*
 ================
 W_FireShotgun
 ================
 */

#ifdef HITBOXCHECK

void W_FireShotgun(void)
{
	qbool classic_shotgun = cvar("k_classic_shotgun");
	vec3_t dir, s_dir, src, o_src, eorg, tmp;
	float offset;
	int bullets = bound(1, cvar("k_hitboxcheck_bullets"), 64), color = iKey(self, "railcolor"), b;

	WS_Mark(self, wpSG);
	self->ps.wpn[wpSG].attacks += bullets;

	sound(self, CHAN_WEAPON, "weapons/guncock.wav", 1, ATTN_NORM);

	g_globalvars.msg_entity = EDICT_TO_PROG(self);
	WriteByte( MSG_ONE, SVC_SMALLKICK);

	trap_makevectors(self->s.v.v_angle);
	// make dir
	VectorCopy(g_globalvars.v_forward, dir);
	// make scaled dir
	VectorScale(dir, 8000, s_dir);
	// make source
	VectorScale(dir, 10, tmp);
	VectorAdd(self->s.v.origin, tmp, src);
	src[2] = self->s.v.absmin[2] + self->s.v.size[2] * 0.7;

	// prepare multi damage
	ClearMultiDamage();
	multi_damage_type = dtSG;

	// make puff
	traceline(PASSVEC3(src), src[0] + s_dir[0], src[1] + s_dir[1], src[2] + s_dir[2], false, self);
	VectorScale(dir, 4, tmp);
	VectorSubtract(g_globalvars.trace_endpos, tmp, puff_org);

	// fire each bullet
	for (offset = -(float)(bullets - 1) / 2, b = bullets; b > 0; b--, offset++)
	{
		// calculate where trace start
		VectorMA(src, offset, g_globalvars.v_right, o_src);
		// calculate where trace should end
		VectorAdd(o_src, s_dir, eorg);
		traceline(PASSVEC3(o_src), PASSVEC3(eorg), false, self);
		if (g_globalvars.trace_fraction != 1.0)
		{
			TraceAttack(1, dir, classic_shotgun);
		}

		if ((b == bullets) || (b == 1))
		{
			CoilgunTrail(o_src, g_globalvars.trace_endpos, self - world, color);
		}
	}

	// finish multi damage
	ApplyMultiDamage();
	if (!classic_shotgun)
	{
		Multi_Finish();
	}
}
#else
void W_FireShotgun(void)
{
	vec3_t dir;
	int bullets = 6;

	WS_Mark(self, wpSG);

	if (cvar("k_instagib"))
	{
		self->ps.wpn[wpSG].attacks++;
	}
	else
	{
		self->ps.wpn[wpSG].attacks += bullets;
	}

	if (cvar("k_instagib_custom_models") && cvar("k_instagib"))
	{
		sound(self, CHAN_WEAPON, "weapons/coilgun.wav", 1, ATTN_NORM);
	}
	else
	{
		sound(self, CHAN_WEAPON, "weapons/guncock.wav", 1, ATTN_NORM);
	}

	g_globalvars.msg_entity = EDICT_TO_PROG(self);

	WriteByte( MSG_ONE, SVC_SMALLKICK);

	if (match_in_progress == 2)
	{
		if ((deathmatch != 4) && !k_bloodfest)
		{
			self->s.v.currentammo = --(self->s.v.ammo_shells);
			AmmoUsed(self);
		}
	}

	//dir = aim (self, 100000);
	aim(dir);
	if (cvar("k_instagib"))
	{
		antilag_lagmove_all_hitscan(self); // we can do the antilag check out here in instagib since knockback doesn't matter
		FireInstaBullet(dir, dtSG);
		antilag_unmove_all();
		antilag_clearflags_all();
	}
	else
	{
		FireBullets(bullets, dir, 0.04, 0.04, 0, dtSG);
	}
}
#endif

/*
 ================
 W_FireSuperShotgun
 ================
 */
void W_FireSuperShotgun(void)
{
	vec3_t dir;
	int bullets = (k_yawnmode ? 21 : 14);

	if (self->s.v.currentammo == 1)
	{
		W_FireShotgun();

		return;
	}

	WS_Mark(self, wpSSG);

	self->ps.wpn[wpSSG].attacks += bullets;

	sound(self, CHAN_WEAPON, "weapons/shotgn2.wav", 1, ATTN_NORM);

	g_globalvars.msg_entity = EDICT_TO_PROG(self);

	WriteByte( MSG_ONE, SVC_BIGKICK);

	if (match_in_progress == 2)
	{
		if ((deathmatch != 4) && !k_bloodfest)
		{
			self->s.v.currentammo = self->s.v.ammo_shells = self->s.v.ammo_shells - 2;
			AmmoUsed(self);
		}
	}

	//dir = aim (self, 100000);
	aim(dir);
	if (k_yawnmode)
	{
		// Yawnmode: larger SSG spread, higher reload time, more damage
		// - Molgrum
		FireBullets(bullets, dir, 0.18, 0.12, 0, dtSSG);
	}
	else
	{
		FireBullets(bullets, dir, 0.14, 0.08, 0, dtSSG);
	}
}

/*
 ==============================================================================

 ROCKETS

 ==============================================================================
 */

void FixQuad(gedict_t *owner)
{
	// well, quaded rocket is no more quaded after owner is dead,
	// at the same time that still allow apply full quad damage to nearby players in case of "quad bore"
	if ((owner->ct == ctPlayer) && ISDEAD(owner))
	{
		owner->super_damage_finished = 0;
	}
}

void T_InstaKickback(void)
{
	vec3_t tmp;

	if (other == PROG_TO_EDICT(self->s.v.owner))
	{
		return;		// don't explode on owner
	}

	if (self->voided)
	{
		return;
	}

	self->voided = 1;
	if (trap_pointcontents(PASSVEC3(self->s.v.origin)) == CONTENT_SKY)
	{
		ent_remove(self);

		return;
	}

	T_RadiusDamage(self, PROG_TO_EDICT(self->s.v.owner), 120, other, dtRL);
	normalize(self->s.v.velocity, tmp);
	VectorScale(tmp, -8, tmp);
	VectorAdd(self->s.v.origin, tmp, self->s.v.origin)
	ent_remove(self);
}

extern int dmg_is_splash;
static void T_MissileExplode_Antilag(void)
{
	// TODO: UNUSED
	// vec3_t hold_org;
	gedict_t *own, *head;

	if ((int)self->s.v.flags & FL_GODMODE)
	{
		gedict_t *trace_hit;

		traceline(PASSVEC3(self->oldangles), PASSVEC3(self->s.v.origin), true, self);
		trap_setorigin(NUM_FOR_EDICT(self), PASSVEC3(g_globalvars.trace_endpos));

		trace_hit = self->oldenemy;
		if (trace_hit != NULL)
		{
			if (trace_hit->antilag_data != NULL && trace_hit->s.v.solid == SOLID_BSP)
			{
				vec3_t trav_off;
				// add platform travel velocity * delay to the rocket position
				// this is a hack that could misbehave in certain circumstances
				VectorScale(trace_hit->s.v.velocity, self->gravity + 0.040, trav_off);
				VectorAdd(self->s.v.origin, trav_off, trav_off);
				trap_setorigin(NUM_FOR_EDICT(self), PASSVEC3(trav_off));
			}
		}
	}

	own = PROG_TO_EDICT(self->s.v.owner);
	if (!CanDamage(own, self))
	{
		ent_remove(self);
		return;
	}

	// TODO: UNUSED!
	// VectorCopy(self->s.v.origin, hold_org);

	if ((int)self->s.v.flags & FL_NOTARGET)
	{
		VectorAdd(own->s.v.origin, self->old_vel, self->s.v.origin);
	}


	// this is awful, but it's the easiest way to exactly replicate the crappy findradius cropping of the splash radius
	head = trap_findradius(world, self->s.v.origin, 160);

	while (head)
	{
		if (head == own)
		{
			int damage = 120;
			float points;
			vec3_t org;

			org[0] = self->s.v.origin[0]
			         - (head->s.v.origin[0] + (head->s.v.mins[0] + head->s.v.maxs[0]) * 0.5f);
			org[1] = self->s.v.origin[1]
			         - (head->s.v.origin[1] + (head->s.v.mins[1] + head->s.v.maxs[1]) * 0.5f);
			org[2] = self->s.v.origin[2]
			         - (head->s.v.origin[2] + (head->s.v.mins[2] + head->s.v.maxs[2]) * 0.5f);
			points = 0.5f * vlen(org);

			if (points < 0)
			{
				points = 0;
			}

			points = damage - points;
			points = points * 0.5f;

			if (points > 0)
			{
				// make sure knockback is the right direction, even if it's not accurate to damage.
				// this is a hack to ensure self damage on rockets against players is accurate
				// TODO: UNUSED!
				// VectorCopy(self->s.v.origin, hold_org);

				head->deathtype = dtRL;
				dmg_is_splash = 1; // mark damage as splash
				T_Damage(head, self, own, points);
				dmg_is_splash = 0; // unmark splash
			}
			break;
		}

		head = trap_findradius(head, self->s.v.origin, 160);
	}
	ent_remove(self);
}

void T_RadiusDamage_Ignore2(gedict_t *inflictor, gedict_t *attacker, float damage, gedict_t *ignore, gedict_t *ignore2, deathType_t dtype); // TODO: move to header

void T_MissileTouch(void)
{
	float damg;
	vec3_t tmp;

	if (other == PROG_TO_EDICT(self->s.v.owner))
	{
		return;		// don't explode on owner
	}

	// race rockets can only hit the world
	if (isRACE() && (other->s.v.solid != SOLID_BSP))
	{
		return;
	}

	if (self->voided)
	{
		return;
	}

	self->voided = 1;
	if (trap_pointcontents(PASSVEC3(self->s.v.origin)) == CONTENT_SKY)
	{
		ent_remove(self);

		return;
	}

	FixQuad(PROG_TO_EDICT(self->s.v.owner));

	// shamblers only take half damage from rockets
	if (streq(other->classname, "monster_shambler") && !cvar("k_bloodfest"))
	{
		damg = 55;
	}
	else
	{
		// 110 dmg on direct hits for all other cases
		damg = 110;
	}

	if (other->s.v.takedamage)
	{
		if (other->ct == ctPlayer)
		{
			WS_Mark(PROG_TO_EDICT(self->s.v.owner), wpRL);
			PROG_TO_EDICT(self->s.v.owner)->ps.wpn[wpRL].hits++;
		}
	}

	if (ISLIVE(other))
	{
		other->deathtype = dtRL;
		T_Damage(other, self, PROG_TO_EDICT(self->s.v.owner), damg);
	}

	// don't do radius damage to the other, because all the damage
	// was done in the impact
	///*
	if (cvar("sv_antilag") == 1) // if this is an anti lag rocket, ignore our owner
	{
		gedict_t *local_explosion;
		vec3_t diff;
		float delay;

		T_RadiusDamage_Ignore2(self, PROG_TO_EDICT(self->s.v.owner), 120, other, PROG_TO_EDICT(self->s.v.owner), dtRL);

		VectorSubtract(self->s.v.origin, self->oldangles, diff);

		local_explosion = spawn();
		trap_setorigin(NUM_FOR_EDICT(local_explosion), PASSVEC3(self->s.v.origin));
		VectorCopy(self->oldangles, local_explosion->oldangles);
		local_explosion->s.v.owner = self->s.v.owner;
		local_explosion->oldenemy = other;
		local_explosion->gravity = g_globalvars.time - time_corrected;

		if ((int)self->s.v.flags & FL_GODMODE) // exploded within reconciliation range
		{
			if (other->s.v.solid == SOLID_SLIDEBOX)
			{
				antilag_t *list = PROG_TO_EDICT(self->s.v.owner)->antilag_data;
				if (list != NULL) // should always be true
				{
					VectorSubtract(list->rewind_origin[antilag_getseek(list, local_explosion->gravity)], self->s.v.origin, local_explosion->old_vel);
					self->s.v.flags = ((int)self->s.v.flags) | FL_NOTARGET;
				}
			}

			delay = (vlen(diff) / vlen(self->s.v.velocity));
			local_explosion->s.v.flags = (int)self->s.v.flags | FL_GODMODE;
			delay -= 0.038f;
		}
		else
		{
			vec3_t traveled;
			VectorScale(self->s.v.velocity, g_globalvars.time - self->rad_time, traveled);
			VectorAdd(self->oldangles, traveled, self->oldangles);
			VectorSubtract(self->s.v.origin, self->oldangles, diff);

			delay = (vlen(diff) / vlen(self->s.v.velocity));
			delay -= 0.038f;
			//delay = self->s.v.health;
		}

		if (delay > 0.013f)
		{
			local_explosion->s.v.nextthink = g_globalvars.time + delay;
			local_explosion->think = (func_t)T_MissileExplode_Antilag;
		}
		else
		{
			gedict_t *oself = self;
			delay = 0.0f;
			self = local_explosion;
			T_MissileExplode_Antilag();
			self = oself;
		}
	}
	else
		T_RadiusDamage(self, PROG_TO_EDICT(self->s.v.owner), 120, other, dtRL);
    //*/

//  sound (self, CHAN_WEAPON, "weapons/r_exp3.wav", 1, ATTN_NORM);
	normalize(self->s.v.velocity, tmp);
	VectorScale(tmp, -8, tmp);
	VectorAdd(self->s.v.origin, tmp, self->s.v.origin)
// self->s.v.origin = self->s.v.origin - 8 * normalize(self->s.v.velocity);
	WriteByte( MSG_MULTICAST, SVC_TEMPENTITY);
	WriteByte( MSG_MULTICAST, TE_EXPLOSION);
	WriteCoord( MSG_MULTICAST, self->s.v.origin[0]);
	WriteCoord( MSG_MULTICAST, self->s.v.origin[1]);
	WriteCoord( MSG_MULTICAST, self->s.v.origin[2]);
	trap_multicast(PASSVEC3(self->s.v.origin), MULTICAST_PHS);

	ent_remove(self);
}

/*
 ================
 W_FireRocket
 ================
 */

void W_FireRocket(void)
{
	WS_Mark(self, wpRL);

	self->ps.wpn[wpRL].attacks++;

	if (match_in_progress == 2)
	{
		if ((deathmatch != 4) && !k_bloodfest)
		{
			self->s.v.currentammo = self->s.v.ammo_rockets = self->s.v.ammo_rockets - 1;
			AmmoUsed(self);
		}
	}

	sound(self, CHAN_WEAPON, "weapons/sgun1.wav", 1, ATTN_NORM);

	g_globalvars.msg_entity = EDICT_TO_PROG(self);
	WriteByte( MSG_ONE, SVC_SMALLKICK);

	newmis = spawn();
	g_globalvars.newmis = EDICT_TO_PROG(newmis);
	newmis->s.v.owner = EDICT_TO_PROG(self);
	newmis->s.v.movetype = MOVETYPE_FLYMISSILE;
	newmis->isMissile = true;
	newmis->s.v.solid = (isRACE() ? SOLID_TRIGGER : SOLID_BBOX);

	// CSQC projectile optmization
	ExtFieldSetSendEntity(newmis, (func_t)SendEntity_Projectile);
	SetSendNeeded(newmis, 255, 0);

	// set newmis speed
	trap_makevectors(self->s.v.v_angle);
	aim(newmis->s.v.velocity);	// = aim(self, 1000);
	if (cvar("k_midair") && (self->super_damage_finished > g_globalvars.time))
	{
		VectorScale(newmis->s.v.velocity, 2000, newmis->s.v.velocity);
		newmis->s.v.effects = EF_BLUE;
	}
	else
	{
		VectorScale(newmis->s.v.velocity, 1000, newmis->s.v.velocity);
	}

	vectoangles(newmis->s.v.velocity, newmis->s.v.angles);

	newmis->touch = (func_t) T_MissileTouch;
	newmis->voided = 0;

	// set newmis duration
	newmis->s.v.nextthink = g_globalvars.time + 10;
	newmis->think = (func_t) SUB_Remove;
	newmis->classname = "rocket";

	setmodel(newmis, "progs/missile.mdl");
	setsize(newmis, 0, 0, 0, 0, 0, 0);

	// setorigin (newmis, self->s.v.origin + v_forward*8 + '0 0 16');
	setorigin(newmis, self->s.v.origin[0] + g_globalvars.v_forward[0] * 8,
				self->s.v.origin[1] + g_globalvars.v_forward[1] * 8,
				self->s.v.origin[2] + g_globalvars.v_forward[2] * 8 + 16);

	// midair 
	VectorCopy(self->s.v.origin, newmis->s.v.oldorigin);
	newmis->rad_time = g_globalvars.time;

	antilag_lagmove_all_proj(self, newmis);
	antilag_unmove_all();
	antilag_clearflags_all();

#ifdef BOT_SUPPORT
	BotsRocketSpawned(newmis);
#endif

	if (cvar("sv_antilag") == 1)
	{
		g_globalvars.newmis = EDICT_TO_PROG(world);
		newmis = world;
	}
}

/*
 ===============================================================================
 LIGHTNING
 ===============================================================================
 */

void LightningHit(gedict_t *from, float damage)
{
	if (PROG_TO_EDICT(g_globalvars.trace_ent)->ct == ctPlayer)
	{
		WS_Mark(from, wpLG);
		from->ps.wpn[wpLG].hits++;
		from->ps.wpn[wpLG].lastfraghits++;
	}

	WriteByte( MSG_MULTICAST, SVC_TEMPENTITY);
	WriteByte( MSG_MULTICAST, TE_LIGHTNINGBLOOD);
	WriteCoord( MSG_MULTICAST, g_globalvars.trace_endpos[0]);
	WriteCoord( MSG_MULTICAST, g_globalvars.trace_endpos[1]);
	WriteCoord( MSG_MULTICAST, g_globalvars.trace_endpos[2]);
	trap_multicast(PASSVEC3(g_globalvars.trace_endpos), MULTICAST_PVS);

	PROG_TO_EDICT(g_globalvars.trace_ent)->deathtype = dtLG_BEAM;
	T_Damage(PROG_TO_EDICT(g_globalvars.trace_ent), from, from, damage);
}

/*
 =================
 LightningDamage
 =================
 */
void LightningDamage(vec3_t p1, vec3_t p2, gedict_t *from, float damage, qbool is_antilagged)
{
	traceline(PASSVEC3(p1), PASSVEC3(p2), false, from);

	if (is_antilagged)
	{
		antilag_unmove_all();
	}

	if (PROG_TO_EDICT(g_globalvars.trace_ent)->s.v.takedamage)
	{
		if (lgc_enabled())
		{
			lgc_register_hit(p1, from, PROG_TO_EDICT(g_globalvars.trace_ent));
		}

		LightningHit(from, damage);

		// this code cause "dm6 secret door bug"
		if (from->ct == ctPlayer)
		{
			gedict_t *gre = PROG_TO_EDICT(from->s.v.groundentity);

			if (gre && (gre == PROG_TO_EDICT(g_globalvars.trace_ent))
					&& streq(gre->classname, "door"))
			{
				PROG_TO_EDICT(g_globalvars.trace_ent)->s.v.velocity[2] += 400;
			}
		}
	}
	else if (lgc_enabled())
	{
		lgc_register_miss(p1, from);
	}

	if (is_antilagged)
	{
		antilag_clearflags_all();
	}
}

void W_FireLightning(void)
{
	qbool do_antilag;
	vec3_t org;
	float cells;
	vec3_t tmp;

	if ((self->s.v.ammo_cells < 1) || (match_in_progress == 1))
	{
		self->s.v.weapon = W_BestWeapon();
		W_SetCurrentAmmo();

		return;
	}

	trap_makevectors(self->s.v.v_angle);

// explode if under water
	if ((self->s.v.waterlevel > 1) && (match_in_progress == 2))
	{

		if (isRA() || isCA() || k_bloodfest)
		{
			self->s.v.ammo_cells = 0;
			W_SetCurrentAmmo();
			AmmoUsed(self);

			return;
		}

		if (deathmatch > 3)
		{
			// Yawnmode: this always kills the player that discharges in dmm4
			// - Molgrum
			if (k_yawnmode || (g_random() <= 0.5))
			{
				self->deathtype = dtLG_DIS_SELF;
				T_Damage(self, self, self, 4000);

				return;
			}
			else
			{
				cells = self->s.v.ammo_cells;
				self->s.v.ammo_cells = 0;
				W_SetCurrentAmmo();
				AmmoUsed(self);

				if (!cvar("k_dis"))
				{
					return;
				}

				antilag_lagmove_all_hitscan(self);
				T_RadiusDamage(self, self, 35 * cells, world, dtLG_DIS);
				antilag_unmove_all();
				antilag_clearflags_all();

				return;
			}
		}
		else
		{
			cells = self->s.v.ammo_cells;
			self->s.v.ammo_cells = 0;
			W_SetCurrentAmmo();
			AmmoUsed(self);

			if (!cvar("k_dis"))
			{
				return;
			}

			antilag_lagmove_all_hitscan(self);
			T_RadiusDamage(self, self, 35 * cells, world, dtLG_DIS);
			antilag_unmove_all();
			antilag_clearflags_all();

			return;
		}
	}

	WS_Mark(self, wpLG);

	self->ps.wpn[wpLG].attacks++;
	self->ps.wpn[wpLG].lastfragattacks++;

	if (self->t_width < g_globalvars.time)
	{
		sound(self, CHAN_WEAPON, "weapons/lhit.wav", 1, ATTN_NORM);
		self->t_width = g_globalvars.time + 0.6;
	}

	g_globalvars.msg_entity = EDICT_TO_PROG(self);
	WriteByte( MSG_ONE, SVC_SMALLKICK);

	if (match_in_progress == 2)
	{
		if ((deathmatch != 4) && !k_bloodfest)
		{
			self->s.v.currentammo = self->s.v.ammo_cells = self->s.v.ammo_cells - 1;
			AmmoUsed(self);
		}
	}

	do_antilag = (self->ct == ctPlayer);  // hacky check
	if (do_antilag)
	{
		antilag_lagmove_all_hitscan(self);
	}

	VectorCopy(self->s.v.origin, org);	//org = self->s.v.origin + '0 0 16';
	org[2] += 16;

	traceline(PASSVEC3(org), org[0] + g_globalvars.v_forward[0] * 600,
				org[1] + g_globalvars.v_forward[1] * 600, org[2] + g_globalvars.v_forward[2] * 600,
				true, self);

	WriteByte( MSG_MULTICAST, SVC_TEMPENTITY);
	WriteByte( MSG_MULTICAST, TE_LIGHTNING2);
	WriteEntity( MSG_MULTICAST, self);
	WriteCoord( MSG_MULTICAST, org[0]);
	WriteCoord( MSG_MULTICAST, org[1]);
	WriteCoord( MSG_MULTICAST, org[2]);
	WriteCoord( MSG_MULTICAST, g_globalvars.trace_endpos[0]);
	WriteCoord( MSG_MULTICAST, g_globalvars.trace_endpos[1]);
	WriteCoord( MSG_MULTICAST, g_globalvars.trace_endpos[2]);

	trap_multicast(PASSVEC3(org), MULTICAST_PHS);

	VectorScale(g_globalvars.v_forward, 4, tmp);
	VectorAdd(g_globalvars.trace_endpos, tmp, tmp);
// qqshka - not from 'self->s.v.origin' but from 'org'
//	LightningDamage( self->s.v.origin, tmp, self, 30 );
	LightningDamage(org, tmp, self, 30, do_antilag);
}

//=============================================================================

void GrenadeExplode(void)
{
	if (self->voided)
	{
		return;
	}

	self->voided = 1;

	FixQuad(PROG_TO_EDICT(self->s.v.owner));

	// shamblers only take half damage from grenades
	if (streq(self->classname, "monster_shambler") && !cvar("k_bloodfest"))
	{
		T_RadiusDamage(self, PROG_TO_EDICT(self->s.v.owner), 60, world, dtGL);
	}
	else
	{
		T_RadiusDamage(self, PROG_TO_EDICT(self->s.v.owner), 120, world, dtGL);
	}

	WriteByte( MSG_MULTICAST, SVC_TEMPENTITY);
	WriteByte( MSG_MULTICAST, TE_EXPLOSION);
	WriteCoord( MSG_MULTICAST, self->s.v.origin[0]);
	WriteCoord( MSG_MULTICAST, self->s.v.origin[1]);
	WriteCoord( MSG_MULTICAST, self->s.v.origin[2]);
	trap_multicast(PASSVEC3(self->s.v.origin), MULTICAST_PHS);

	ent_remove(self);
}

void GrenadeTouch(void)
{
	if (other == PROG_TO_EDICT(self->s.v.owner))
	{
		return;		// don't explode on owner
	}

	// can't touch/damage others in race
	if (isRACE() && (other->ct == ctPlayer) && (other != PROG_TO_EDICT(self->s.v.owner)))
	{
		return;
	}

	if (other->s.v.takedamage)
	{
		if (other->ct == ctPlayer)
		{
			WS_Mark(PROG_TO_EDICT(self->s.v.owner), wpGL);
			PROG_TO_EDICT(self->s.v.owner)->ps.wpn[wpGL].hits++;
		}
	}

	if (other->s.v.takedamage == DAMAGE_AIM)
	{
		GrenadeExplode();

		return;
	}

	sound(self, CHAN_WEAPON, "weapons/bounce.wav", 1, ATTN_NORM);	// bounce sound
	if ((self->s.v.velocity[0] == 0) && (self->s.v.velocity[1] == 0) && (self->s.v.velocity[2] == 0))
	{
		VectorClear(self->s.v.avelocity);
	}
}

/*
 ================
 W_FireGrenade
 ================
 */
void W_FireGrenade(void)
{
	float r1 = crandom(), r2 = crandom();
	vec3_t ang;
	WS_Mark(self, wpGL);

	self->ps.wpn[wpGL].attacks++;

	if (match_in_progress == 2)
	{
		if (isCA())
		{
			self->s.v.currentammo = self->ca_ammo_grenades = self->ca_ammo_grenades - 1;

			if (!self->ca_ammo_grenades)
			{
				self->s.v.items -= IT_GRENADE_LAUNCHER;
			}
			
			AmmoUsed(self);
		}		
		else if ((deathmatch != 4) && !k_bloodfest)
		{
			self->s.v.currentammo = self->s.v.ammo_rockets = self->s.v.ammo_rockets - 1;
			AmmoUsed(self);
		}
	}

	sound(self, CHAN_WEAPON, "weapons/grenade.wav", 1, ATTN_NORM);

	g_globalvars.msg_entity = EDICT_TO_PROG(self);

	WriteByte( MSG_ONE, SVC_SMALLKICK);

	newmis = spawn();
	g_globalvars.newmis = EDICT_TO_PROG(newmis);
	newmis->voided = 0;
	newmis->s.v.owner = EDICT_TO_PROG(self);
	newmis->s.v.movetype = MOVETYPE_BOUNCE;
	newmis->isMissile = true;
	newmis->s.v.solid = SOLID_BBOX;
	newmis->classname = "grenade";

// set newmis speed     

	VectorCopy(self->s.v.v_angle, ang);

	// limit pitch in case the server allows pitch angles < -70,
	// so that grenades still go straight up and not behind
	if (ang[PITCH] < -70)
	{
		ang[PITCH] = -70;
	}

	trap_makevectors(ang);

	// Yawnmode: disable randomness in grenade aim
	// - Molgrum
	if (k_yawnmode)
	{
		r1 = r2 = 0;
	}

	newmis->s.v.velocity[0] = g_globalvars.v_forward[0] * 600 + g_globalvars.v_up[0] * 200
			+ r1 * g_globalvars.v_right[0] * 10 + r2 * g_globalvars.v_up[0] * 10;
	newmis->s.v.velocity[1] = g_globalvars.v_forward[1] * 600 + g_globalvars.v_up[1] * 200
			+ r1 * g_globalvars.v_right[1] * 10 + r2 * g_globalvars.v_up[1] * 10;
	newmis->s.v.velocity[2] = g_globalvars.v_forward[2] * 600 + g_globalvars.v_up[2] * 200
			+ r1 * g_globalvars.v_right[2] * 10 + r2 * g_globalvars.v_up[0] * 10;

	SetVector(newmis->s.v.avelocity, 300, 300, 300);
// newmis.avelocity = '300 300 300';

	vectoangles(newmis->s.v.velocity, newmis->s.v.angles);

	newmis->touch = (func_t) GrenadeTouch;
	newmis->s.v.nextthink = g_globalvars.time + 2.5;
	newmis->think = (func_t) GrenadeExplode;

	if ((deathmatch == 4) && cvar("k_dmm4_gren_mode"))
	{
		newmis->think = (func_t) SUB_Remove;
	}

	antilag_platform_move(self->antilag_data, self->client_ping / 1000.0f);

	setmodel(newmis, "progs/grenade.mdl");
	setsize(newmis, 0, 0, 0, 0, 0, 0);
	setorigin(newmis, PASSVEC3(self->s.v.origin));

	antilag_lagmove_all_proj_bounce(self, newmis);
	antilag_unmove_all();
	antilag_clearflags_all();

#ifdef BOT_SUPPORT
	BotsGrenadeSpawned(newmis);
#endif

	// we don't want to do newmis stuff, antilag 1 takes care of it for us
	if (cvar("sv_antilag") == 1)
	{
		g_globalvars.newmis = EDICT_TO_PROG(world);
		newmis = world;
	}
}

//=============================================================================

void spike_touch(void);

/*
 ===============
 launch_spike

 Used for both the player and the traps
 ===============
 */

void launch_spike(vec3_t org, vec3_t dir)
{
	newmis = spawn();
	g_globalvars.newmis = EDICT_TO_PROG(newmis);
	newmis->voided = 0;
	newmis->s.v.owner = EDICT_TO_PROG(self);
	newmis->s.v.movetype = MOVETYPE_FLYMISSILE;
	newmis->isMissile = true;
	newmis->s.v.solid = (isRACE() ? SOLID_TRIGGER : SOLID_BBOX);

	newmis->touch = (func_t) spike_touch;
	newmis->classname = "spike";
	newmis->think = (func_t) SUB_Remove;
	newmis->s.v.nextthink = g_globalvars.time + 6;
	setmodel(newmis, "progs/spike.mdl");
	setsize(newmis, 0, 0, 0, 0, 0, 0);
	setorigin(newmis, PASSVEC3(org));

	// Yawnmode: spikes velocity is 1800 instead of 1000
	// - Molgrum
	VectorScale(dir, (k_yawnmode ? 1800 : 1000), newmis->s.v.velocity);

	vectoangles(newmis->s.v.velocity, newmis->s.v.angles);

	// CSQC projectile optmization
	ExtFieldSetSendEntity(newmis, (func_t)SendEntity_Projectile);
	SetSendNeeded(newmis, 255, 0);
}

static qbool race_ignore_spike(gedict_t *self, gedict_t *other)
{
	// we're not racing, treat as normal
	if (!isRACE())
	{
		return false;
	}

	// everything collides with walls
	if (other == world)
	{
		return false;
	}

	// they only collide with players
	if (other->ct != ctPlayer)
	{
		return true;
	}

	// player fired it?  ignore
	return PROG_TO_EDICT(self->s.v.owner)->ct == ctPlayer;
}

void spike_touch(void)
{
	if (other == PROG_TO_EDICT(self->s.v.owner))
	{
		return;
	}

	if (race_ignore_spike(self, other))
	{
		return;
	}

	if (self->voided)
	{
		return;
	}

	self->voided = 1;

	if (other->s.v.solid == SOLID_TRIGGER)
	{
		G_bprint(2, "Trigger field, do nothing (%s)\n", other->netname);

		return;		// trigger field, do nothing
	}

	if (trap_pointcontents(PASSVEC3(self->s.v.origin)) == CONTENT_SKY)
	{
		ent_remove(self);

		return;
	}

	FixQuad(PROG_TO_EDICT(self->s.v.owner));

// hit something that bleeds
	if (other->s.v.takedamage)
	{
		if (other->ct == ctPlayer)
		{
			WS_Mark(PROG_TO_EDICT(self->s.v.owner), wpNG);
			PROG_TO_EDICT(self->s.v.owner)->ps.wpn[wpNG].hits++;
		}

		spawn_touchblood(1);
		other->deathtype = dtNG;
		T_Damage(other, self, PROG_TO_EDICT(self->s.v.owner), 9);
	}
	else
	{
		WriteByte( MSG_MULTICAST, SVC_TEMPENTITY);
		if (!strcmp(self->classname, "wizspike"))
		{
			WriteByte( MSG_MULTICAST, TE_WIZSPIKE);
		}
		else if (!strcmp(self->classname, "knightspike"))
		{
			WriteByte( MSG_MULTICAST, TE_KNIGHTSPIKE);
		}
		else
		{
			WriteByte( MSG_MULTICAST, TE_SPIKE);
		}

		WriteCoord( MSG_MULTICAST, self->s.v.origin[0]);
		WriteCoord( MSG_MULTICAST, self->s.v.origin[1]);
		WriteCoord( MSG_MULTICAST, self->s.v.origin[2]);
		trap_multicast(PASSVEC3(self->s.v.origin), MULTICAST_PHS);
	}

	ent_remove(self);
}

void superspike_touch(void)
{
	if (other == PROG_TO_EDICT(self->s.v.owner))
	{
		return;
	}

	if (race_ignore_spike(self, other))
	{
		return;
	}

	if (self->voided)
	{
		return;
	}

	self->voided = 1;

	if (other->s.v.solid == SOLID_TRIGGER)
	{
		return;		// trigger field, do nothing
	}

	if (trap_pointcontents(PASSVEC3(self->s.v.origin)) == CONTENT_SKY)
	{
		ent_remove(self);

		return;
	}

	FixQuad(PROG_TO_EDICT(self->s.v.owner));

// hit something that bleeds
	if (other->s.v.takedamage)
	{
		if (other->ct == ctPlayer)
		{
			WS_Mark(PROG_TO_EDICT(self->s.v.owner), wpSNG);
			PROG_TO_EDICT(self->s.v.owner)->ps.wpn[wpSNG].hits++;
		}

		spawn_touchblood(2);
		other->deathtype = dtSNG;
		T_Damage(other, self, PROG_TO_EDICT(self->s.v.owner), (k_yawnmode ? 16 : 18));
	}
	else
	{
		WriteByte( MSG_MULTICAST, SVC_TEMPENTITY);
		WriteByte( MSG_MULTICAST, TE_SUPERSPIKE);
		WriteCoord( MSG_MULTICAST, self->s.v.origin[0]);
		WriteCoord( MSG_MULTICAST, self->s.v.origin[1]);
		WriteCoord( MSG_MULTICAST, self->s.v.origin[2]);
		trap_multicast(PASSVEC3(self->s.v.origin), MULTICAST_PHS);
	}

	ent_remove(self);
}

void W_FireSuperSpikes(void)
{
	vec3_t dir, tmp;

	WS_Mark(self, wpSNG);

	self->ps.wpn[wpSNG].attacks++;

	sound(self, CHAN_WEAPON, "weapons/spike2.wav", 1, ATTN_NORM);
	self->attack_finished = self->client_time + 0.2f;

	if (match_in_progress == 2)
	{
		if ((deathmatch != 4) && !k_bloodfest)
		{
			self->s.v.currentammo = self->s.v.ammo_nails = self->s.v.ammo_nails - 2;
			AmmoUsed(self);
		}
	}

	antilag_platform_move(self->antilag_data, self->client_ping / 1000);

	aim(dir);               //dir = aim (self, 1000);

	VectorCopy(self->s.v.origin, tmp);
	tmp[2] += 16;
	launch_spike(tmp, dir);
	newmis->touch = (func_t) superspike_touch;
	setmodel(newmis, "progs/s_spike.mdl");
	setsize(newmis, 0, 0, 0, 0, 0, 0);
	g_globalvars.msg_entity = EDICT_TO_PROG(self);
	WriteByte( MSG_ONE, SVC_SMALLKICK);

	antilag_lagmove_all_proj(self, newmis);
	antilag_unmove_all();
	antilag_clearflags_all();

	if (cvar("sv_antilag") == 1)
	{
		g_globalvars.newmis = EDICT_TO_PROG(world);
		newmis = world;
	}
}

void W_FireSpikes(float ox)
{
	vec3_t dir, tmp;

	// Yawnmode: ignores alternating nails effect in nailgun
	// - Molgrum
	if (k_yawnmode)
	{
		ox = 0;
	}

	trap_makevectors(self->s.v.v_angle);

	if (match_in_progress != 1)
	{
		if ((self->s.v.ammo_nails >= 2) && (self->s.v.weapon == IT_SUPER_NAILGUN))
		{
			W_FireSuperSpikes();

			return;
		}
	}

	if ((self->s.v.ammo_nails < 1) || (match_in_progress == 1))
	{
		self->s.v.weapon = W_BestWeapon();
		W_SetCurrentAmmo();

		return;
	}

	WS_Mark(self, wpNG);

	self->ps.wpn[wpNG].attacks++;

	sound(self, CHAN_WEAPON, "weapons/rocket1i.wav", 1, ATTN_NORM);
	self->attack_finished = self->client_time + 0.2f;

	if (match_in_progress == 2)
	{
		if ((deathmatch != 4) && !k_bloodfest)
		{
			self->s.v.currentammo = self->s.v.ammo_nails = self->s.v.ammo_nails - 1;
			AmmoUsed(self);
		}
	}

	antilag_platform_move(self->antilag_data, self->client_ping / 1000.0f);

	aim(dir);		// dir = aim (self, 1000);
	VectorScale(g_globalvars.v_right, ox, tmp);
	VectorAdd(tmp, self->s.v.origin, tmp);
	tmp[2] += 16;
	launch_spike(tmp, dir);

	g_globalvars.msg_entity = EDICT_TO_PROG(self);
	WriteByte( MSG_ONE, SVC_SMALLKICK);

	antilag_lagmove_all_proj(self, newmis);
	antilag_unmove_all();
	antilag_clearflags_all();

	if (cvar("sv_antilag") == 1)
	{
		g_globalvars.newmis = EDICT_TO_PROG(world);
		newmis = world;
	}
}

/*
 ===============================================================================

 PLAYER WEAPON USE

 ===============================================================================
 */

void W_SetCurrentAmmo(void)
{
	qbool need_fix = false;
	int items;
	float old_currentammo = self->s.v.currentammo;

// { get out of any weapon firing states

	if ((self->think == (func_t)player_stand1) || (self->think == (func_t)player_run))
	{
		if ((self->s.v.weapon == IT_AXE) || (self->s.v.weapon == IT_HOOK))
		{
			if (self->s.v.velocity[0] || self->s.v.velocity[1])
			{
				// run
				if ((self->s.v.frame < 0) || (self->s.v.frame > 5))
				{
					need_fix = true; // wrong axe run frame
				}
			}
			else
			{
				// stand
				if ((self->s.v.frame < 17) || (self->s.v.frame > 28))
				{
					need_fix = true; // wrong axe stand frame
				}
			}
		}
		else
		{
			if (self->s.v.velocity[0] || self->s.v.velocity[1])
			{
				// run
				if ((self->s.v.frame < 6) || (self->s.v.frame > 11))
				{
					need_fix = true; // wrong non axe run frame
				}
			}
			else
			{
				// stand
				if ((self->s.v.frame < 12) || (self->s.v.frame > 16))
				{
					need_fix = true; // wrong non axe stand frame
				}
			}
		}
	}
	else
	{
		need_fix = true; // hm, set proper ->think
	}

	if (need_fix)
	{
		// may change ->s.v.frame
		self->walkframe = 0;
		player_run();
	}
	else
	{
		// does't change ->s.v.frame and ->walkframe, so we does't break current animation sequence on weapon change
		self->s.v.nextthink = g_globalvars.time + 0.1;
		self->s.v.weaponframe = 0;
	}

// }

	items = self->s.v.items;
	items -= items & ( IT_SHELLS | IT_NAILS | IT_ROCKETS | IT_CELLS);
	switch ((int)self->s.v.weapon)
	{
		case IT_AXE:
			self->s.v.currentammo = 0;
			self->weaponmodel = "progs/v_axe.mdl";
			self->weapon_index = 0;
			self->s.v.weaponframe = 0;
			if (vw_enabled)
			{
				self->vw_index = 1;
			}

			break;

		case IT_SHOTGUN:
			self->s.v.currentammo = self->s.v.ammo_shells;
			if (cvar("k_instagib_custom_models") && cvar("k_instagib"))
			{
				self->weaponmodel = "progs/v_coil.mdl";
			}
			else
			{
				self->weaponmodel = "progs/v_shot.mdl";
			}

			self->weapon_index = 1;
			self->s.v.weaponframe = 0;
			items |= IT_SHELLS;
			if (vw_enabled)
			{
				self->vw_index = 2;
			}

			break;

		case IT_SUPER_SHOTGUN:
			self->s.v.currentammo = self->s.v.ammo_shells;
			self->weaponmodel = "progs/v_shot2.mdl";
			self->weapon_index = 2;
			self->s.v.weaponframe = 0;
			items |= IT_SHELLS;
			if (vw_enabled)
			{
				self->vw_index = 3;
			}

			break;

		case IT_NAILGUN:
			self->s.v.currentammo = self->s.v.ammo_nails;
			self->weaponmodel = "progs/v_nail.mdl";
			self->weapon_index = 3;
			self->s.v.weaponframe = 0;
			items |= IT_NAILS;
			if (vw_enabled)
			{
				self->vw_index = 4;
			}

			break;

		case IT_SUPER_NAILGUN:
			self->s.v.currentammo = self->s.v.ammo_nails;
			self->weaponmodel = "progs/v_nail2.mdl";
			self->weapon_index = 4;
			self->s.v.weaponframe = 0;
			items |= IT_NAILS;
			if (vw_enabled)
			{
				self->vw_index = 5;
			}

			break;

		case IT_GRENADE_LAUNCHER:
			if (isCA())
			{
				self->s.v.currentammo = self->ca_ammo_grenades;
			}
			else
			{
				self->s.v.currentammo = self->s.v.ammo_rockets;
			}
			self->weaponmodel = "progs/v_rock.mdl";
			self->weapon_index = 5;
			self->s.v.weaponframe = 0;
			items |= IT_ROCKETS;
			if (vw_enabled)
			{
				self->vw_index = 6;
			}

			break;

		case IT_ROCKET_LAUNCHER:
			self->s.v.currentammo = self->s.v.ammo_rockets;
			self->weaponmodel = "progs/v_rock2.mdl";
			self->weapon_index = 6;
			self->s.v.weaponframe = 0;
			items |= IT_ROCKETS;
			if (vw_enabled)
			{
				self->vw_index = 7;
			}

			break;

		case IT_LIGHTNING:
			self->s.v.currentammo = self->s.v.ammo_cells;
			self->weaponmodel = "progs/v_light.mdl";
			self->weapon_index = 7;
			self->s.v.weaponframe = 0;
			items |= IT_CELLS;
			if (vw_enabled)
			{
				self->vw_index = 8;
			}

			break;

		case IT_HOOK:
			self->s.v.currentammo = 0;
			if (k_ctf_custom_models)
			{
				self->weaponmodel = "progs/v_star.mdl";
			}
			else
			{
				self->weaponmodel = "progs/v_axe.mdl";
			}

			self->s.v.weaponframe = 0;
			if (vw_enabled)
			{
				self->vw_index = 1;
			}

			break;

		default:
			self->s.v.currentammo = 0;
			self->weaponmodel = "";
			self->s.v.weaponframe = 0;
			self->vw_index = 0;
			break;
	}

	if (!vw_enabled || self->invisible_finished)
	{
		self->vw_index = 0;
	}

	if (match_in_progress != 2)
	{
		if (old_currentammo)
		{
			self->s.v.currentammo = old_currentammo;
		}
		else
		{
			self->s.v.currentammo = 1000;
		}
	}

	self->s.v.items = items;
}

float W_BestWeapon(void)
{
	int it = self->s.v.items;
	char *w_rank = ezinfokey(self, "w_rank");

	if (!strnull(w_rank))
	{
		while (*w_rank)
		{
			int weapon = (*w_rank - '0');

			if ((weapon == 8) && (it & IT_LIGHTNING) && (self->s.v.ammo_cells >= 1))
			{
				return IT_LIGHTNING;
			}

			if ((weapon == 7) && (it & IT_ROCKET_LAUNCHER) && (self->s.v.ammo_rockets >= 1))
			{
				return IT_ROCKET_LAUNCHER;
			}

			if ((weapon == 6) && (it & IT_GRENADE_LAUNCHER) && (self->s.v.ammo_rockets >= 1))
			{
				return IT_GRENADE_LAUNCHER;
			}

			if ((weapon == 5) && (it & IT_SUPER_NAILGUN) && (self->s.v.ammo_nails >= 2))
			{
				return IT_SUPER_NAILGUN;
			}

			if ((weapon == 4) && (it & IT_NAILGUN) && (self->s.v.ammo_nails >= 1))
			{
				return IT_NAILGUN;
			}

			if ((weapon == 3) && (it & IT_SUPER_SHOTGUN) && (self->s.v.ammo_shells >= 2))
			{
				return IT_SUPER_SHOTGUN;
			}

			if ((weapon == 2) && (it & IT_SHOTGUN) && (self->s.v.ammo_shells >= 1))
			{
				return IT_SHOTGUN;
			}

			if ((weapon == 1) && (it & IT_AXE))
			{
				return IT_AXE;
			}

			++w_rank;
		}

		if (it & IT_AXE)
		{
			return IT_AXE;
		}

		return 0;
	}

	// Standard behaviour
	if ((self->s.v.waterlevel <= 1) && (self->s.v.ammo_cells >= 1) && (it & IT_LIGHTNING))
	{
		return IT_LIGHTNING;
	}
	else if ((self->s.v.ammo_nails >= 2) && (it & IT_SUPER_NAILGUN))
	{
		return IT_SUPER_NAILGUN;
	}
	else if ((self->s.v.ammo_shells >= 2) && (it & IT_SUPER_SHOTGUN))
	{
		return IT_SUPER_SHOTGUN;
	}
	else if ((self->s.v.ammo_nails >= 1) && (it & IT_NAILGUN))
	{
		return IT_NAILGUN;
	}
	else if ((self->s.v.ammo_shells >= 1) && (it & IT_SHOTGUN))
	{
		return IT_SHOTGUN;
	}

	/*
	 if(self.ammo_rockets >= 1 && (it & IT_ROCKET_LAUNCHER) )
	 return IT_ROCKET_LAUNCHER;
	 else if(self.ammo_rockets >= 1 && (it & IT_GRENADE_LAUNCHER) )
	 return IT_GRENADE_LAUNCHER;

	 */

	return ((it & IT_AXE) ? IT_AXE : 0);
}

int W_CheckNoAmmo(void)
{
	if (self->s.v.currentammo > 0)
	{
		return true;
	}

	if ((self->s.v.weapon == IT_AXE) || (self->s.v.weapon == IT_HOOK))
	{
		return true;
	}

	self->s.v.weapon = W_BestWeapon();

	W_SetCurrentAmmo();

//	drop the weapon down
	return false;
}

/*
 ============
 W_Attack

 An attack impulse can be triggered now
 ============
 */
void W_Attack(void)
{
	float r;

	if (!W_CheckNoAmmo())
	{
		return;
	}

	self->lastwepfired = self->s.v.weapon;

	trap_makevectors(self->s.v.v_angle);	// calculate forward angle for velocity
	self->show_hostile = g_globalvars.time + 1;	// wake monsters up

	switch ((int)self->s.v.weapon)
	{
		case IT_AXE:
			if (self->ctf_flag & CTF_RUNE_HST)
			{
				self->attack_finished = self->client_time + 0.5f
						- (cvar("k_ctf_rune_power_hst") / 10.0f);
				HasteSound(self);
			}
			else
			{
				self->attack_finished = self->client_time + 0.5f;
			}

			// crt - no axe sound for spec
			if (!isRA() || (isWinner(self) || isLoser(self)))
			{
				sound(self, CHAN_WEAPON, "weapons/ax1.wav", 1, ATTN_NORM);
			}

			//r = g_random();
			//dubious axe RNG to make it predictable
			r = fabs((((int)(self->client_time * 931.75f) << 11) + ((int)(self->client_time) >> 6)) % 1000) / 1000.0f;

			if (r < 0.25)
			{
				player_axe1();
			}
			else if (r < 0.5)
			{
				player_axeb1();
			}
			else if (r < 0.75)
			{
				player_axec1();
			}
			else
			{
				player_axed1();
			}

			break;

		case IT_SHOTGUN:
			player_shot1();
			if (self->ctf_flag & CTF_RUNE_HST)
			{
				self->attack_finished = self->client_time + 0.5f
						- (cvar("k_ctf_rune_power_hst") / 10.0f);
				HasteSound(self);
			}
			else
			{
				if (cvar("k_instagib") == 1)
				{
					self->attack_finished = self->client_time + 1.2f;
				}
				else if (cvar("k_instagib") == 2)
				{
					self->attack_finished = self->client_time + 0.7f;
				}
				else
				{
					self->attack_finished = self->client_time + 0.5f;
				}
			}

			W_FireShotgun();
			break;

		case IT_SUPER_SHOTGUN:
			player_shot1();
			if (self->ctf_flag & CTF_RUNE_HST)
			{
				self->attack_finished = self->client_time + 0.5f
						- (cvar("k_ctf_rune_power_hst") / 20.0f);
				HasteSound(self);
			}
			else
			{
				self->attack_finished = self->client_time + (k_yawnmode ? 0.8 : 0.7);
			}

			W_FireSuperShotgun();
			break;

		case IT_NAILGUN:
			self->s.v.ltime = g_globalvars.time;
			player_nail1();
			break;

		case IT_SUPER_NAILGUN:
			self->s.v.ltime = g_globalvars.time;
			player_nail1();
			break;

		case IT_GRENADE_LAUNCHER:
			player_rocket1();
			if (self->ctf_flag & CTF_RUNE_HST)
			{
				self->attack_finished = self->client_time + 0.5f
						- (cvar("k_ctf_rune_power_hst") / 10.0f);
				HasteSound(self);
			}
			else
			{
				self->attack_finished = self->client_time + 0.6f;
			}

			W_FireGrenade();
			break;

		case IT_ROCKET_LAUNCHER:
			player_rocket1();
			if (self->ctf_flag & CTF_RUNE_HST)
			{
				self->attack_finished = self->client_time + 0.5f
						- (cvar("k_ctf_rune_power_hst") / 20.0f);
				HasteSound(self);
			}
			else
			{
				self->attack_finished = self->client_time + 0.8f;
			}

			W_FireRocket();
			break;

		case IT_LIGHTNING:
			self->attack_finished = self->client_time + 0.1f;
			sound(self, CHAN_AUTO, "weapons/lstart.wav", 1, ATTN_NORM);
			self->s.v.ltime = g_globalvars.time;
			player_light1();
			break;

		case IT_HOOK:
			if (self->hook_out)
			{
				player_chain3();
			}
			else
			{
				player_chain1();
			}

			self->attack_finished = self->client_time + 0.1f;
			break;
	}
}

qbool W_CanSwitch(int wp, qbool warn)
{
	int it, am, fl = 0;

	it = self->s.v.items;
	am = 0;

	switch (wp)
	{
		case 1:
			fl = IT_AXE;
			break;

		case 2:
			fl = IT_SHOTGUN;
			if (self->s.v.ammo_shells < 1)
			{
				am = 1;
			}

			break;

		case 3:
			fl = IT_SUPER_SHOTGUN;
			if (self->s.v.ammo_shells < 2)
			{
				am = 1;
			}

			break;

		case 4:
			fl = IT_NAILGUN;
			if (self->s.v.ammo_nails < 1)
			{
				am = 1;
			}

			break;

		case 5:
			fl = IT_SUPER_NAILGUN;
			if (self->s.v.ammo_nails < 2)
			{
				am = 1;
			}

			break;

		case 6:
			fl = IT_GRENADE_LAUNCHER;
			if (self->s.v.ammo_rockets < 1)
			{
				am = 1;
			}

			break;

		case 7:
			fl = IT_ROCKET_LAUNCHER;
			if (self->s.v.ammo_rockets < 1)
			{
				am = 1;
			}

			break;

		case 8:
			fl = IT_LIGHTNING;
			if (self->s.v.ammo_cells < 1)
			{
				am = 1;
			}

			break;

		case 22:
			fl = IT_HOOK;
			break;

		default:
			break;
	}

	if (!(it & fl) && !self->race_chasecam)
	{
		// don't have the weapon or the ammo
		if (warn)
		{
			G_sprint(self, PRINT_HIGH, "no weapon\n");
		}

		return false;
	}

	if (am && !self->race_chasecam)
	{
		// don't have the ammo
		if (warn)
		{
			G_sprint(self, PRINT_HIGH, "not enough ammo\n");
		}

		return false;
	}

	return true;
}

/*
 ============
 W_ChangeWeapon

 ============
 */
qbool W_ChangeWeapon(int wp)
{
	int it, am, fl = 0;

	if ((self->client_time < self->attack_finished) && wp != 22)
	{
		return false;
	}

	it = self->s.v.items;
	am = 0;

	switch (wp)
	{
		case 1:
			// ctf shortcut for newbs: selecting axe when you already have it switches to grapple
			if (isCTF() && (self->s.v.weapon == IT_AXE) && cvar("k_ctf_hook"))
			{
				fl = IT_HOOK;
			}
			else
			{
				fl = IT_AXE;
			}

			break;

		case 2:
			fl = IT_SHOTGUN;
			if (self->s.v.ammo_shells < 1)
			{
				am = 1;
			}

			break;

		case 3:
			fl = IT_SUPER_SHOTGUN;
			if (self->s.v.ammo_shells < 2)
			{
				am = 1;
			}

			break;

		case 4:
			fl = IT_NAILGUN;
			if (self->s.v.ammo_nails < 1)
			{
				am = 1;
			}

			break;

		case 5:
			fl = IT_SUPER_NAILGUN;
			if (self->s.v.ammo_nails < 2)
			{
				am = 1;
			}

			break;

		case 6:
			fl = IT_GRENADE_LAUNCHER;
			if (self->s.v.ammo_rockets < 1)
			{
				am = 1;
			}

			break;

		case 7:
			fl = IT_ROCKET_LAUNCHER;
			if (self->s.v.ammo_rockets < 1)
			{
				am = 1;
			}

			break;

		case 8:
			fl = IT_LIGHTNING;
			if (self->s.v.ammo_cells < 1)
			{
				am = 1;
			}

			break;

		case 22:
			fl = IT_HOOK;
			if (self->s.v.weapon != IT_HOOK)
			{
				if (self->hook_out)
				{
					GrappleReset(self->hook);
				}

				self->hook_out = false;
				self->on_hook = false;
			}

			break;

		default:
			break;
	}

	if (!(it & fl) && !self->race_chasecam) // don't have the weapon
	{
		G_sprint(self, PRINT_HIGH, "no weapon\n");
	}
	else if (am && !self->race_chasecam) // don't have the ammo
	{
		G_sprint(self, PRINT_HIGH, "not enough ammo\n");
	}
	else
	{
		//
		// set weapon, set ammo
		//
		self->s.v.weapon = fl;
		W_SetCurrentAmmo();
	}

	return true;
}

/*
 ============
 CycleWeaponCommand

 Go to the next weapon with ammo
 ============
 */
qbool CycleWeaponCommand(void)
{
	int i, it, am;

	if (self->client_time < self->attack_finished)
	{
		return false;
	}

	it = self->s.v.items;

	for (i = 0; i < 20; i++) // qqshka, 20 is just from head, but prevent infinite loop
	{
		am = 0;
		switch ((int)self->s.v.weapon)
		{
			case IT_LIGHTNING:
				self->s.v.weapon = IT_AXE;
				break;

			case IT_AXE:
				self->s.v.weapon = IT_HOOK;
				break;

			case IT_HOOK:
				self->s.v.weapon = IT_SHOTGUN;
				if (self->s.v.ammo_shells < 1)
				{
					am = 1;
				}
				break;

			case IT_SHOTGUN:
				self->s.v.weapon = IT_SUPER_SHOTGUN;
				if (self->s.v.ammo_shells < 2)
				{
					am = 1;
				}
				break;

			case IT_SUPER_SHOTGUN:
				self->s.v.weapon = IT_NAILGUN;
				if (self->s.v.ammo_nails < 1)
				{
					am = 1;
				}
				break;

			case IT_NAILGUN:
				self->s.v.weapon = IT_SUPER_NAILGUN;
				if (self->s.v.ammo_nails < 2)
				{
					am = 1;
				}
				break;

			case IT_SUPER_NAILGUN:
				self->s.v.weapon = IT_GRENADE_LAUNCHER;
				if (self->s.v.ammo_rockets < 1)
				{
					am = 1;
				}
				break;

			case IT_GRENADE_LAUNCHER:
				self->s.v.weapon = IT_ROCKET_LAUNCHER;
				if (self->s.v.ammo_rockets < 1)
				{
					am = 1;
				}
				break;

			case IT_ROCKET_LAUNCHER:
				self->s.v.weapon = IT_LIGHTNING;
				if (self->s.v.ammo_cells < 1)
				{
					am = 1;
				}
				break;
		}

		if ((it & (int)self->s.v.weapon) && (am == 0))
		{
			W_SetCurrentAmmo();

			return true;
		}
	}

	return true;
}

/*
 ============
 CycleWeaponReverseCommand

 Go to the prev weapon with ammo
 ============
 */
qbool CycleWeaponReverseCommand(void)
{
	int i, it, am;

	if (self->client_time < self->attack_finished)
	{
		return false;
	}

	it = self->s.v.items;

	for (i = 0; i < 20; i++) // qqshka, 20 is just from head, but prevent infinite loop
	{
		am = 0;
		switch ((int)self->s.v.weapon)
		{
			case IT_LIGHTNING:
				self->s.v.weapon = IT_ROCKET_LAUNCHER;
				if (self->s.v.ammo_rockets < 1)
				{
					am = 1;
				}

				break;

			case IT_ROCKET_LAUNCHER:
				self->s.v.weapon = IT_GRENADE_LAUNCHER;
				if (self->s.v.ammo_rockets < 1)
				{
					am = 1;
				}

				break;

			case IT_GRENADE_LAUNCHER:
				self->s.v.weapon = IT_SUPER_NAILGUN;
				if (self->s.v.ammo_nails < 2)
				{
					am = 1;
				}

				break;

			case IT_SUPER_NAILGUN:
				self->s.v.weapon = IT_NAILGUN;
				if (self->s.v.ammo_nails < 1)
				{
					am = 1;
				}

				break;

			case IT_NAILGUN:
				self->s.v.weapon = IT_SUPER_SHOTGUN;
				if (self->s.v.ammo_shells < 2)
				{
					am = 1;
				}

				break;

			case IT_SUPER_SHOTGUN:
				self->s.v.weapon = IT_SHOTGUN;
				if (self->s.v.ammo_shells < 1)
				{
					am = 1;
				}

				break;

			case IT_SHOTGUN:
				self->s.v.weapon = IT_HOOK;
				break;

			case IT_HOOK:
				self->s.v.weapon = IT_AXE;
				break;

			case IT_AXE:
				self->s.v.weapon = IT_LIGHTNING;
				if (self->s.v.ammo_cells < 1)
				{
					am = 1;
				}

				break;
		}

		if ((it & (int)self->s.v.weapon) && (am == 0))
		{
			W_SetCurrentAmmo();

			return true;
		}
	}

	return true;
}

void kfjump(void);
void krjump(void);

int CaptainImpulses(void)
{
	if (k_captains != 2)
	{
		return 2; // s: return 2 if nothing interesting
	}

	// ok - below possible captain stuff

	if ((self->s.v.impulse > MAX_CLIENTS) || !capt_num(self))
	{
		return 0; // s: return 0 if captain mode is set and no captain things were entered
	}

	return 1; // s: return 1 if it's a player picker impulse
}

/*
 ============
 ImpulseCommands

 ============
 */

void ImpulseCommands(void)
{
	qbool clear = true;
	int capt, impulse = self->s.v.impulse;

	if (self->ct != ctPlayer)
	{
		self->s.v.impulse = impulse = 0;
	}

	if (!impulse)
	{
		return;
	}

	capt = CaptainImpulses();

	if (!capt)
	{
		; // empty
	}
	else if (capt == 1)
	{
		CaptainPickPlayer();
	}
	else if (self->k_adminc && (impulse >= 1) && (impulse <= 9))
	{
		AdminImpBot();
	}
	else if (((impulse >= 1) && (impulse <= 8)) || (impulse == 22))
	{
		clear = W_ChangeWeapon(impulse);
	}
	else if (impulse == 9)
	{
		; // removed
	}
	else if (impulse == 10)
	{
		clear = CycleWeaponCommand();
	}
	else if (impulse == 11)
	{
		; // removed
	}
	else if (impulse == 12)
	{
		clear = CycleWeaponReverseCommand();
	}
	else if (impulse == 156)
	{
		kfjump();
	}
	else if (impulse == 164)
	{
		krjump();
	}

	if (clear)
	{
		self->s.v.impulse = 0;
	}
}

void can_prewar_msg(char *msg)
{
	if (g_globalvars.time > self->k_msgcount)
	{
		self->k_msgcount = g_globalvars.time + 1;

		stuffcmd(self, "bf\n");
		G_sprint(self, 2, "%s\n", msg);
	}
}

// if fire == false then can_prewar is called for jump
qbool can_prewar(qbool fire)
{
	int k_prewar;

	if (match_in_progress == 2)
	{
		return true;  // u can fire/jump
	}

	k_prewar = cvar("k_prewar");

	switch (k_prewar)
	{
		case 1:
			goto captains;
			// probably u can fire/jump

		case 2:
			if (self->ready)
			{
				goto captains;
			}

			// probably u can jump/fire if ready
			if (fire)
			{
				can_prewar_msg(redtext("type ready to enable fire"));
			}
			else
			{
				can_prewar_msg(redtext("type ready to enable jumps"));
			}

			return false; // u can't fire/jump if _not_ ready

		case 0:
		default:
			if (fire)
			{  // u can't fire
				can_prewar_msg(redtext("can't fire, prewar is disabled"));

				return false;
			}

			goto captains;
			// probably u can jump
	}

captains:

	if (k_captains != 2)
	{
		return true; // u can fire/jump
	}

	if (fire)
	{
		// u can't fire
		can_prewar_msg(redtext("can't fire until in captains mode"));

		return false;
	}

	return true; // u can jump
}

/*
 ============
 W_WeaponFrame

 Called every frame so impulse events can be handled as well as possible
 ============
 */
void W_WeaponFrame(void)
{
	if ((self->spawn_time + 0.05) > g_globalvars.time)
	{
		return; // discard +attack till 50 ms after respawn, like ktpro 
	}

	if (self->wreg_attack) // client simulate +attack via "cmd wreg" feature
	{
		self->s.v.button0 = true;
	}

	if (isRACE())
	{
		if ((self->ct == ctPlayer) && !self->racer && race.status)
		{
			if (self->s.v.button0)
			{
				ChasecamToggleButton();
			}
			else
			{
				self->s.v.flags = ((int)(self->s.v.flags)) | FL_ATTACKRELEASED;
			}

			return;
		}
	}

	if (isCA())
	{
		ClanArenaTrackingToggleButton();
	}

	ImpulseCommands();

	if (!race_weapon_allowed(self))
	{
		self->client_predflags = PRDFL_FORCEOFF;
		return;
	}

	if (self->client_time < self->attack_finished)
	{
		return;
	}

// check for attack

	if (self->s.v.button0 && !intermission_running)
	{
		if (!readytostart())
		{
			return;	// RA restrictions
		}

		if (!CA_can_fire(self))
		{
			return;	// CA restrictions
		}

		if ((match_in_progress == 1) || !can_prewar(true))
		{
			return;
		}

		SuperDamageSound();
		W_Attack();
	}
}

/*
 ========
 SuperDamageSound

 Plays sound if needed
 ========
 */
void SuperDamageSound(void)
{
	if (self->super_damage_finished > g_globalvars.time)
	{
		if (self->super_sound < g_globalvars.time)
		{
			self->super_sound = g_globalvars.time + 1;
			// Play 8x sound if quad + strength rune
			if (self->ctf_flag & CTF_RUNE_STR)
			{
				sound(self, CHAN_AUTO, "rune/rune22.wav", 1, ATTN_NORM);
			}
			else
			{
				sound(self, CHAN_AUTO, "items/damage3.wav", 1, ATTN_NORM);
			}
		}
	}
	else if (self->ctf_flag & CTF_RUNE_STR)
	{
		if (self->super_sound < g_globalvars.time)
		{
			self->super_sound = g_globalvars.time + 1;
			sound(self, CHAN_AUTO, "rune/rune2.wav", 1, ATTN_NORM);
		}
	}

	return;
}
