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
gedict_t *stemp, *otemp, *old;

qbool BotsPreTeleport(gedict_t *self, gedict_t *other);
void BotsPostTeleport(gedict_t *self, gedict_t *other, gedict_t *teleport_destination);

void trigger_reactivate(void)
{
	self->s.v.solid = SOLID_TRIGGER;
}

//=============================================================================

#define SPAWNFLAG_NOMESSAGE  1
#define SPAWNFLAG_NOTOUCH  1

// the wait g_globalvars.time has passed, so set back up for another activation
void multi_wait(void)
{
	if (self->s.v.max_health)
	{
		self->s.v.health = self->s.v.max_health;
		self->s.v.takedamage = DAMAGE_YES;
		self->s.v.solid = SOLID_BBOX;
	}
}

// the trigger was just touched/killed/used
// self->s.v.enemy should be set to the activator so it can be held through a delay
// so wait for the delay g_globalvars.time before firing
void multi_trigger(void)
{
	if (self->s.v.nextthink > g_globalvars.time)
	{
		return;		// allready been triggered
	}

	if (streq(self->classname, "trigger_secret"))
	{
		if (PROG_TO_EDICT(self->s.v.enemy)->ct != ctPlayer)
		{
			return;
		}

		g_globalvars.found_secrets = g_globalvars.found_secrets + 1;
		WriteByte( MSG_ALL, SVC_FOUNDSECRET);
	}

	if (self->noise)
	{
		sound(self, CHAN_VOICE, self->noise, 1, ATTN_NORM);
	}

// don't trigger again until reset
	self->s.v.takedamage = DAMAGE_NO;

	activator = PROG_TO_EDICT(self->s.v.enemy);

	SUB_UseTargets();

	if (self->wait > 0)
	{
		self->think = (func_t) multi_wait;
		self->s.v.nextthink = g_globalvars.time + self->wait;
	}
	else
	{			// we can't just ent_remove(self) here, because this is a touch function
		// called wheil C code is looping through area links...
		self->touch = (func_t) SUB_Null;
		self->s.v.nextthink = g_globalvars.time + 0.1;
		self->think = (func_t) SUB_Remove;
	}
}

void multi_killed(void)
{
	self->s.v.enemy = EDICT_TO_PROG(damage_attacker);
	multi_trigger();
}

void multi_use(void)
{
	self->s.v.enemy = EDICT_TO_PROG(activator);
	multi_trigger();
}

void multi_touch(void)
{
	// #practice mode#
	if (!k_practice && (match_in_progress != 2))
	{
		return;
	}

	if (!other->classname)
	{
		return;
	}

	if (other->ct != ctPlayer)
	{
		return;
	}

// if the trigger has an angles field, check player's facing direction
	if ((self->s.v.movedir[0] != 0) && (self->s.v.movedir[1] != 0) && (self->s.v.movedir[2] != 0))
	{
		trap_makevectors(other->s.v.angles);
		if (DotProduct(g_globalvars.v_forward, self->s.v.movedir) < 0)
		{
			return;	// not facing the right way
		}
	}

	self->s.v.enemy = EDICT_TO_PROG(other);
	multi_trigger();
}

/*QUAKED trigger_multiple (.5 .5 .5) ? notouch
 Variable sized repeatable trigger.  Must be targeted at one or more entities.  If "health" is set, the trigger must be killed to activate each g_globalvars.time.
 If "delay" is set, the trigger waits some time after activating before firing.
 "wait" : Seconds between triggerings. (.2 default)
 If notouch is set, the trigger is only fired by other entities, not by touching.
 NOTOUCH has been obsoleted by trigger_relay!
 sounds
 1) secret
 2) beep beep
 3) large switch
 4)
 set "message" to text string
 */
void SP_trigger_multiple(void)
{
	if (self->s.v.sounds == 1)
	{
		trap_precache_sound("misc/secret.wav");
		self->noise = "misc/secret.wav";
	}
	else if (self->s.v.sounds == 2)
	{
		trap_precache_sound("misc/talk.wav");
		self->noise = "misc/talk.wav";
	}
	else if (self->s.v.sounds == 3)
	{
		trap_precache_sound("misc/trigger1.wav");
		self->noise = "misc/trigger1.wav";
	}

	if (!self->wait)
	{
		self->wait = 0.2;
	}

	self->use = (func_t) multi_use;

	InitTrigger();

	if (ISLIVE(self))
	{
		if ((int)(self->s.v.spawnflags) & SPAWNFLAG_NOTOUCH)
		{
			G_Error("health and notouch don't make sense\n");
		}

		self->s.v.max_health = self->s.v.health;
		self->th_die = multi_killed;
		self->s.v.takedamage = DAMAGE_YES;
		self->s.v.solid = SOLID_BBOX;
		setorigin(self, PASSVEC3(self->s.v.origin));	// make sure it links into the world
	}
	else
	{
		if (!((int)(self->s.v.spawnflags) & SPAWNFLAG_NOTOUCH))
		{
			self->touch = (func_t) multi_touch;
		}
	}
}

/*QUAKED trigger_once (.5 .5 .5) ? notouch
 Variable sized trigger. Triggers once, then removes itself.  You must set the key "target" to the name of another object in the level that has a matching
 "targetname".  If "health" is set, the trigger must be killed to activate.
 If notouch is set, the trigger is only fired by other entities, not by touching.
 if "killtarget" is set, any objects that have a matching "target" will be removed when the trigger is fired.
 if "angle" is set, the trigger will only fire when someone is facing the direction of the angle.  Use "360" for an angle of 0.
 sounds
 1) secret
 2) beep beep
 3) large switch
 4)
 set "message" to text string
 */
void SP_trigger_once(void)
{
	self->wait = -1;
	SP_trigger_multiple();
}

//=============================================================================

/*QUAKED trigger_relay (.5 .5 .5) (-8 -8 -8) (8 8 8)
 This fixed size trigger cannot be touched, it can only be fired by other events.  It can contain killtargets, targets, delays, and messages.
 */
void SP_trigger_relay(void)
{
	self->use = (func_t) SUB_UseTargets;
}

//=============================================================================

/*QUAKED trigger_secret (.5 .5 .5) ?
 secret counter trigger
 sounds
 1) secret
 2) beep beep
 3)
 4)
 set "message" to text string
 */
void SP_trigger_secret(void)
{
	g_globalvars.total_secrets = g_globalvars.total_secrets + 1;
	self->wait = -1;

	if (!self->s.v.sounds)
	{
		self->s.v.sounds = 1;
	}

	if (self->s.v.sounds == 1)
	{
		trap_precache_sound("misc/secret.wav");
		self->noise = "misc/secret.wav";
	}
	else if (self->s.v.sounds == 2)
	{
		trap_precache_sound("misc/talk.wav");
		self->noise = "misc/talk.wav";
	}

	SP_trigger_multiple();
}

//=============================================================================

void counter_use(void)
{
// char* junk;

	self->count = self->count - 1;
	if (self->count < 0)
	{
		return;
	}

	if (self->count != 0)
	{
		if (activator->ct == ctPlayer && ((int)(self->s.v.spawnflags) & SPAWNFLAG_NOMESSAGE) == 0)
		{
			if (self->count >= 4)
			{
				G_centerprint(activator, "There are more to go...");
			}
			else if (self->count == 3)
			{
				G_centerprint(activator, "Only 3 more to go...");
			}
			else if (self->count == 2)
			{
				G_centerprint(activator, "Only 2 more to go...");
			}
			else
			{
				G_centerprint(activator, "Only 1 more to go...");
			}
		}

		return;
	}

	if ((activator->ct == ctPlayer) && (((int)(self->s.v.spawnflags) & SPAWNFLAG_NOMESSAGE) == 0))
	{
		G_centerprint(activator, "Sequence completed!");
	}

	self->s.v.enemy = EDICT_TO_PROG(activator);
	multi_trigger();
}

/*QUAKED trigger_counter (.5 .5 .5) ? nomessage
 Acts as an intermediary for an action that takes multiple inputs.

 If nomessage is not set, t will print "1 more.. " etc when triggered and "sequence complete" when finished.

 After the counter has been triggered "count" g_globalvars.times (default 2), it will fire all of it's targets and remove itself.
 */
void SP_trigger_counter(void)
{
	self->wait = -1;
	if (!self->count)
	{
		self->count = 2;
	}

	self->use = (func_t) counter_use;
}

/*
 ==============================================================================

 TELEPORT TRIGGERS

 ==============================================================================
 */

#define PLAYER_ONLY  1
#define SILENT  2

// changed the function for rapid sound so sndspot parameter included
void play_teleport(gedict_t *sndspot)
{
	float v;
	char *tmpstr;

	v = g_random() * 5;
	if (v < 1)
	{
		tmpstr = "misc/r_tele1.wav";
	}
	else if (v < 2)
	{
		tmpstr = "misc/r_tele2.wav";
	}
	else if (v < 3)
	{
		tmpstr = "misc/r_tele3.wav";
	}
	else if (v < 4)
	{
		tmpstr = "misc/r_tele4.wav";
	}
	else
	{
		tmpstr = "misc/r_tele5.wav";
	}

	sound(sndspot, CHAN_VOICE, tmpstr, 1, ATTN_NORM);
}

void spawn_tfog(vec3_t org)
{
// qqshka: no need for this 
// {
//	gedict_t *s = spawn();
//	VectorCopy( org, s->s.v.origin );// s->s.v.origin = org;
//	s->s.v.nextthink = g_globalvars.time + 0.2;
//	s->think = ( func_t ) SUB_Remove;
// }

	WriteByte( MSG_MULTICAST, SVC_TEMPENTITY);
	WriteByte( MSG_MULTICAST, TE_TELEPORT);
	WriteCoord( MSG_MULTICAST, org[0]);
	WriteCoord( MSG_MULTICAST, org[1]);
	WriteCoord( MSG_MULTICAST, org[2]);
	trap_multicast(PASSVEC3(org), MULTICAST_PHS);
}

/*
 void tdeath_touch(void)
 {
 gedict_t       *other2;

 // { ktpro way to reduce double telefrags
 if ( other->tdeath_time > self->tdeath_time )
 return; // this mean we go through tele after some guy, so we must not be telefragged
 // }

 other2 = PROG_TO_EDICT(self->s.v.owner);

 if ( other == other2 )
 return;

 if ( ISDEAD( other ) )
 return;

 // frag anyone who teleports in on top of an invincible player
 if ( other->ct == ctPlayer )
 {
 // check if both players have invincible
 if ( other->invincible_finished > g_globalvars.time && other2->invincible_finished > g_globalvars.time )
 {
 // remove invincible for both players
 other->invincible_finished = other2->invincible_finished = 0;

 // probably this must kill both players
 other2->deathtype = other->deathtype = dtTELE3;
 T_Damage( other,  self, other2, 50000 );
 T_Damage( other2, self, other,  50000 );

 return;
 }

 // mortal trying telefrag someone who has 666
 if ( other->invincible_finished > g_globalvars.time )
 {
 other2->deathtype = dtTELE2;
 T_Damage( other2, self, other, 50000 );

 return;
 }
 }

 if ( ISLIVE( other ) )
 {
 other->deathtype = dtTELE1;
 T_Damage( other, self, other2, 50000 );
 }
 }
 */

// { NOTE: retarded code, just so monsters able double telefrag each other, for example "e1m7"
void tdeath_touch(void)
{
	gedict_t *other2;

	// should not be used against players
	if (other->ct == ctPlayer)
	{
		return;
	}

	if (ISDEAD(other))
	{
		return;
	}

// { ktpro way to reduce double telefrags
	if (other->tdeath_time > self->tdeath_time)
	{
		return; // this mean we go through tele after some guy, so we must not be telefragged
	}
// }

	other2 = PROG_TO_EDICT(self->s.v.owner);

	if (other == other2)
	{
		return;
	}

	other->deathtype = dtTELE1;
	T_Damage(other, self, other2, 50000);
}

void spawn_tdeath(vec3_t org, gedict_t *death_owner)
{
	gedict_t *death;

	death = spawn();
	death->classname = "teledeath";
	death->s.v.movetype = MOVETYPE_NONE;
	death->s.v.solid = SOLID_TRIGGER;
	SetVector(death->s.v.angles, 0, 0, 0);

	setsize(death, death_owner->s.v.mins[0] - 1, death_owner->s.v.mins[1] - 1,
			death_owner->s.v.mins[2] - 1, death_owner->s.v.maxs[0] + 1,
			death_owner->s.v.maxs[1] + 1, death_owner->s.v.maxs[2] + 1);
	setorigin(death, PASSVEC3(org));

	death->touch = (func_t) tdeath_touch;
	// fixes the telefrag bug from previous kteams
	death->s.v.nextthink = g_globalvars.time + 0.1;
	death->think = (func_t) SUB_Remove;
	death->s.v.owner = EDICT_TO_PROG(death_owner);

// { ktpro way to reduce double telefrags
	death_owner->tdeath_time = death->tdeath_time = g_globalvars.time;
// }

	g_globalvars.force_retouch = 2;	// make sure even still objects get hit
}
// }

void teleport_player(gedict_t *player, vec3_t origin, vec3_t angles, int flags)
{
	qbool dm = deathmatch;
	gedict_t *p, *p2;
	deathType_t dt = dtNONE;

// protect(in some case) player from be spawnfragged for some time
	player->k_1spawn = g_globalvars.time + 0.78;

// put a tfog where the player was and play teleporter sound
// For some odd reason (latency?), no matter if the sound was issued to play
// at the spot of the entity before it entered the teleporter, it actually
// plays at the teleporter destination. So we just create a body double of the
// player at the departure side which stands still, plays the sound and lives
// for 1/10 of a second, then is removed. All these efforts were needed to get
// rid of that annoying 2/10 second delay in playing the teleporter sound.

	// play sound where the player was
	if (flags & TFLAGS_SND_SRC)
	{
		gedict_t *othercopy = spawn();

		setorigin(othercopy, PASSVEC3(player->s.v.origin));
		othercopy->s.v.nextthink = g_globalvars.time + 0.1;
		othercopy->think = (func_t) SUB_Remove;
		play_teleport(othercopy);
	}

	//put a tfog where the player was
	if (flags & TFLAGS_FOG_SRC)
	{
		spawn_tfog(player->s.v.origin);
	}

	trap_makevectors(angles);

	// spawn a tfog flash in front of the destination
	if (flags & TFLAGS_FOG_DST)
	{
		vec3_t fog_org;

		VectorMA(origin, (flags & TFLAGS_FOG_DST_SPAWN) ? 20 : 32, g_globalvars.v_forward, fog_org);
		spawn_tfog(fog_org);
	}

	setorigin(player, PASSVEC3(origin));

	// play sound at destination
	if (flags & TFLAGS_SND_DST)
	{
		play_teleport(player);
	}

	VectorCopy(angles, player->s.v.angles); // player.angles = angles;

	if (player->ct == ctPlayer)
	{
		if ((player->s.v.weapon == IT_HOOK) && player->hook_out)
		{
			GrappleReset(player->hook);
			player->attack_finished = g_globalvars.time + 0.25;
		}

		player->s.v.fixangle = 1;	// turn this way immediately

// <Tonik> qqshka|Tara, teleport_time these days is used by waterjump code
//		player->s.v.teleport_time = g_globalvars.time + 0.7;

		if (flags & TFLAGS_VELOCITY_ADJUST)
		{
			// Yawnmode: preserve velocity, I'm copying my own mod's code since i found it interesting to play with
			// - Molgrum
			if (k_yawnmode)
			{
				float vel;

				// Scale the original speed like airstep does
				player->s.v.velocity[2] = 0;
				vel = vlen(player->s.v.velocity) * (1.0 - k_teleport_cap / 100.0);

				// Only preserve speed above 300
				vel = max(300, vel);

				VectorScale(g_globalvars.v_forward, vel, player->s.v.velocity);
			}
			else
			{
				//  player->s.v.velocity = v_forward * 300;
				VectorScale(g_globalvars.v_forward, 300, player->s.v.velocity);
			}
		}
	}

	player->s.v.flags -= (int)player->s.v.flags & FL_ONGROUND;

	// perform telefragging code

	// { NOTE: retarded code, just so monsters able double telefrag each other, for example "e1m7"
	if (player->ct != ctPlayer)
	{
		spawn_tdeath(player->s.v.origin, player);
	}
	// }

	p2 = NULL;
	// If 'deathmatch' then use fast find_plr(), if non dm then more slow nextent().
	// Since 'deathmatch' variable is global and may be changed during next 'for' we use local var 'dm' instead.
	for (p = world; (p = (dm ? find_plr(p) : nextent(p)));)
	{
		if (p == player)
		{
			continue; // ignore self
		}

		if (ISDEAD(p))
		{
			continue; // alredy dead
		}

		if (p->s.v.solid != SOLID_SLIDEBOX)
		{
			continue; // not a monster or player
		}

		if ((player->s.v.absmin[0] > p->s.v.absmax[0]) || (player->s.v.absmin[1] > p->s.v.absmax[1])
				|| (player->s.v.absmin[2] > p->s.v.absmax[2])
				|| (player->s.v.absmax[0] < p->s.v.absmin[0])
				|| (player->s.v.absmax[1] < p->s.v.absmin[1])
				|| (player->s.v.absmax[2] < p->s.v.absmin[2]))
		{
			/*
			 G_bprint(2, "%s do not intersects\n", p->netname);
			 G_bprint(2, "%.1f %.1f\n", player->s.v.absmin[0] , p->s.v.absmax[0]);
			 G_bprint(2, "%.1f %.1f\n", player->s.v.absmin[1] , p->s.v.absmax[1]);
			 G_bprint(2, "%.1f %.1f\n", player->s.v.absmin[2] , p->s.v.absmax[2]);
			 G_bprint(2, "%.1f %.1f\n", player->s.v.absmax[0] , p->s.v.absmin[0]);
			 G_bprint(2, "%.1f %.1f\n", player->s.v.absmax[1] , p->s.v.absmin[1]);
			 G_bprint(2, "%.1f %.1f\n", player->s.v.absmax[2] , p->s.v.absmin[2]);
			 */
			continue; // bboxes not intersects
		}

//		G_bprint(2, "%s intersects\n", p->netname);

		// frag anyone who teleports in on top of an invincible player
		if (p->ct == ctPlayer)
		{
			if (p->invincible_finished > g_globalvars.time)
			{
				if (player->invincible_finished > g_globalvars.time)
				{
					// both players have invincible
					p->invincible_finished = 0; // remove invincible
					p->deathtype = dt = dtTELE3;
					p2 = p; // remember
					T_Damage(p, player, player, 50000);

					continue;
				}
				else
				{
					// mortal trying telefrag someone who has 666, gib mortal instead
					dt = dtTELE2;
					p2 = p; // remember

					continue;
				}
			}

		}

		if (ISLIVE(p))
		{
			p->deathtype = dtTELE1;
			T_Damage(p, player, player, 50000);
		}
	}

	if (p2 && dt != dtNONE)
	{
		// gib self
		player->invincible_finished = 0; // remove invincible
		player->deathtype = dt;
		T_Damage(player, p2, p2, 50000);
	}
}

void teleport_touch(void)
{
	gedict_t *t;

	if (self->targetname)
	{
		if (self->s.v.nextthink < g_globalvars.time)
		{
			return;	// not fired yet
		}
	}

	if ((int)(self->s.v.spawnflags) & PLAYER_ONLY)
	{
		if (other->ct != ctPlayer)
		{
			return;
		}
	}

// only teleport living creatures
	if (ISDEAD(other) || (!isRACE() && (other->s.v.solid != SOLID_SLIDEBOX)))
	{
		return;
	}

	if (isRA() && !isWinner(other) && !isLoser(other))
	{
		return;
	}

	if (isRACE() && race_handle_event(other, self, "touch"))
	{
		return;
	}

// activator = other;
	SUB_UseTargets();

	t = find(world, FOFS(targetname), self->target);
	if (!t)
	{
		// G_Error( "couldn't find target" );
		return;
	}

#ifdef BOT_SUPPORT
	if (bots_enabled() && BotsPreTeleport(self, other))
	{
		return;
	}
#endif

	if ((match_in_progress == 0) && !strnull(self->ktx_votemap))
	{
		gedict_t *tele = self;
		self = other;
		VoteMapSpecific(tele->ktx_votemap);
		self = tele;
	}

	other->teleported = 1;
	other->teleport_time = g_globalvars.time;
	teleport_player(other, t->s.v.origin, t->mangle,
	TFLAGS_FOG_SRC | TFLAGS_FOG_DST | TFLAGS_SND_SRC | TFLAGS_SND_DST | TFLAGS_VELOCITY_ADJUST);

#ifdef BOT_SUPPORT
	if (bots_enabled())
	{
		BotsPostTeleport(self, other, t);
	}
#endif
}

/*QUAKED info_teleport_destination (.5 .5 .5) (-8 -8 -8) (8 8 32)
 This is the destination marker for a teleporter.  It should have a "targetname" field with the same value as a teleporter's "target" field.
 */
void SP_info_teleport_destination(void)
{
// this does nothing, just serves as a target spot
	VectorCopy(self->s.v.angles, self->mangle);
// self.mangle = self.angles;
	SetVector(self->s.v.angles, 0, 0, 0);
	self->model = "";
	self->s.v.origin[2] += 27;
	if (!self->targetname)
	{
		G_Error("no targetname");
	}
}

void teleport_use(void)
{
	self->s.v.nextthink = g_globalvars.time + 0.2;
	g_globalvars.force_retouch = 2;	// make sure even still objects get hit
	self->think = (func_t) SUB_Null;
}

/*QUAKED trigger_teleport (.5 .5 .5) ? PLAYER_ONLY SILENT
 Any object touching this will be transported to the corresponding info_teleport_destination gedict_t*. You must set the "target" field, and create an object with a "targetname" field that matches.

 If the trigger_teleport has a targetname, it will only teleport entities when it has been fired.
 */
void SP_trigger_teleport(void)
{
	vec3_t o;

	InitTrigger();
	self->touch = (func_t) teleport_touch;
	// find the destination 
	if (!self->target)
	{
		G_Error("no target");
	}

	self->use = (func_t) teleport_use;

	if (!((int)(self->s.v.spawnflags) & SILENT))
	{
		trap_precache_sound("ambience/hum1.wav");
		VectorAdd(self->s.v.mins, self->s.v.maxs, o);
		VectorScale(o, 0.5, o);
		//o = (self.mins + self.maxs)*0.5;
		trap_ambientsound(PASSVEC3(o), "ambience/hum1.wav", 0.5, ATTN_STATIC);
	}
}

void SP_trigger_custom_teleport(void)
{
	// set real classname
	self->classname = "trigger_teleport";
	// some size hack.
	setsize(self, -self->s.v.size[0], -self->s.v.size[1], -self->s.v.size[2], self->s.v.size[0],
			self->s.v.size[1], self->s.v.size[2]);
	// and call proper spawn function.
	SP_trigger_teleport();

	// reset origin, well, you have to set origin each time you change solid type...
	setorigin(self, PASSVEC3(self->s.v.origin));

//	G_cprint("SP_trigger_custom_teleport: size %.1f %.1f %.1f\n", PASSVEC3(self->s.v.size));
//	G_cprint("SP_trigger_custom_teleport: org %.1f %.1f %.1f\n", PASSVEC3(self->s.v.origin));
}

/*
 ==============================================================================

 trigger_setskill

 ==============================================================================
 */

void trigger_skill_touch(void)
{
	if (other->ct != ctPlayer)
	{
		return;
	}

	cvar_set("skill", self->message);
}

/*QUAKED trigger_setskill (.5 .5 .5) ?
 sets skill level to the value of "message".
 Only used on start map.
 */
void SP_trigger_setskill(void)
{
	if (deathmatch)
	{
		ent_remove(self);

		return;
	}

	if (!self->message)
	{
		self->message = "";
	}

	InitTrigger();
	self->touch = (func_t) trigger_skill_touch;
}

/*
 ==============================================================================

 ONLY REGISTERED TRIGGERS

 ==============================================================================
 */

void trigger_onlyregistered_touch(void)
{
	if (other->ct != ctPlayer)
	{
		return;
	}

	if (self->attack_finished > g_globalvars.time)
	{
		return;
	}

	self->attack_finished = g_globalvars.time + 2;
	if (/*trap_cvar( "registered" )*/true)
	{
		self->message = "";
		activator = other;
		SUB_UseTargets();
		ent_remove(self);
	}
	else
	{
		if (self->message && strneq(self->message, ""))
		{
			G_centerprint(other, "%s", self->message);
			sound(other, CHAN_BODY, "misc/talk.wav", 1, ATTN_NORM);
		}
	}
}

/*QUAKED trigger_onlyregistered (.5 .5 .5) ?
 Only fires if playing the registered version, otherwise prints the message
 */
void SP_trigger_onlyregistered(void)
{
	trap_precache_sound("misc/talk.wav");
	InitTrigger();
	self->touch = (func_t) trigger_onlyregistered_touch;
}

//============================================================================

void hurt_on(void)
{
	self->s.v.solid = SOLID_TRIGGER;
	self->s.v.nextthink = -1;
	setorigin(self, PASSVEC3(self->s.v.origin)); // it matter
	g_globalvars.force_retouch = 2; // make sure even still objects get hit
}

void hurt_items(void)
{
	if (cvar("k_ctf_hurt_items"))
	{
		if (streq(other->classname, "item_flag_team1") || streq(other->classname, "item_flag_team2"))
		{
			// Cause flag to return to spawn position.
			other->super_time = g_globalvars.time;
		}
		else if (streq(other->classname, "rune"))
		{
			// Cause rune to respawn.
			other->s.v.nextthink = g_globalvars.time;
		}
	}
}

void hurt_touch(void)
{
	if (!other->s.v.takedamage)
	{
		hurt_items();
		return;
	}

	self->s.v.solid = SOLID_NOT;
	other->deathtype = dtTRIGGER_HURT;
	T_Damage(other, self, self, self->dmg);
	self->think = (func_t) hurt_on;
	self->s.v.nextthink = g_globalvars.time + 1;
}

/*QUAKED trigger_hurt (.5 .5 .5) ?
 Any object touching this will be hurt
 set dmg to damage amount
 defalt dmg = 5
 */
void SP_trigger_hurt(void)
{
	if (streq("end", mapname) && cvar("k_remove_end_hurt"))
	{
		soft_ent_remove(self);

		return;
	}

	InitTrigger();
	self->touch = (func_t) hurt_touch;
	if (!self->dmg)
	{
		self->dmg = 5;
	}
}

//============================================================================

#define PUSH_ONCE  1

void trigger_push_touch(void)
{
	if (streq(other->classname, "grenade"))
	{
		other->s.v.velocity[0] = self->speed * self->s.v.movedir[0] * 10;
		other->s.v.velocity[1] = self->speed * self->s.v.movedir[1] * 10;
		other->s.v.velocity[2] = self->speed * self->s.v.movedir[2] * 10;
	}
	else if (ISLIVE(other))
	{
//		other->s.v.velocity = self->speed  * self->s.v.movedir * 10;
		other->s.v.velocity[0] = self->speed * self->s.v.movedir[0] * 10;
		other->s.v.velocity[1] = self->speed * self->s.v.movedir[1] * 10;
		other->s.v.velocity[2] = self->speed * self->s.v.movedir[2] * 10;

		if (other->ct == ctPlayer)
		{
			if (other->fly_sound < g_globalvars.time)
			{
				other->fly_sound = g_globalvars.time + 1.5;
				sound(other, CHAN_AUTO, "ambience/windfly.wav", 1, ATTN_NORM);
			}
		}
	}

	if ((int)(self->s.v.spawnflags) & PUSH_ONCE)
	{
		ent_remove(self);
	}
}

/*QUAKED trigger_push (.5 .5 .5) ? PUSH_ONCE
 Pushes the player
 */
void SP_trigger_push(void)
{
	InitTrigger();
	self->touch = (func_t) trigger_push_touch;
	if (!self->speed)
	{
		self->speed = 1000;
	}
}

void SP_trigger_custom_push(void)
{
	// set real classname
	self->classname = "trigger_push";
	// some size hack.
	setsize(self, -self->s.v.size[0], -self->s.v.size[1], -self->s.v.size[2], self->s.v.size[0],
			self->s.v.size[1], self->s.v.size[2]);
	// and call proper spawn function.
	SP_trigger_push();

	// reset origin, well, you have to set origin each time you change solid type...
	setorigin(self, PASSVEC3(self->s.v.origin));
}

//============================================================================

void trigger_monsterjump_touch(void)
{
	if (((int)other->s.v.flags & (FL_MONSTER | FL_FLY | FL_SWIM)) != FL_MONSTER)
	{
		return;
	}

// set XY even if not on ground, so the jump will clear lips
	other->s.v.velocity[0] = self->s.v.movedir[0] * self->speed;
	other->s.v.velocity[1] = self->s.v.movedir[1] * self->speed;

	if (!((int)other->s.v.flags & FL_ONGROUND))
	{
		return;
	}

	other->s.v.flags = other->s.v.flags - FL_ONGROUND;

	other->s.v.velocity[2] = self->height;
}

/*QUAKED trigger_monsterjump (.5 .5 .5) ?
 Walking monsters that touch this will jump in the direction of the trigger's angle
 "speed" default to 200, the speed thrown forward
 "height" default to 200, the speed thrown upwards
 */
void SP_trigger_monsterjump(void)
{
	if (!self->speed)
	{
		self->speed = 200;
	}

	if (!self->height)
	{
		self->height = 200;
	}

	if ((self->s.v.angles[0] == 0) && (self->s.v.angles[1] == 0) && (self->s.v.angles[2] == 0))
	{
		SetVector(self->s.v.angles, 0, 360, 0);
	}

	InitTrigger();
	self->touch = (func_t) trigger_monsterjump_touch;
}

void SP_trigger_custom_monsterjump(void)
{
	// set real classname
	self->classname = "trigger_monsterjump";
	// some size hack.
	setsize(self, -self->s.v.size[0], -self->s.v.size[1], -self->s.v.size[2], self->s.v.size[0],
			self->s.v.size[1], self->s.v.size[2]);
	// and call proper spawn function.
	SP_trigger_monsterjump();

	// reset origin, well, you have to set origin each time you change solid type...
	setorigin(self, PASSVEC3(self->s.v.origin));
}

float T_Heal(gedict_t *e, float healamount, float ignore);

static void trigger_heal_touch(void)
{
	if ((match_in_progress == 1) || (!match_in_progress && cvar("k_freeze")))
	{
		return;
	}

	if (!streq(other->classname, "player") || ISDEAD(other))
	{
		return;
	}

	if (other->healtimer > g_globalvars.time)
	{
		return;
	}

	if (other->s.v.takedamage && (other->s.v.health < self->healmax))
	{
		sound (self, CHAN_AUTO, self->noise, 1, ATTN_NORM);

		if ((other->s.v.health + self->healamount) > self->healmax)
		{
			T_Heal (other, (self->healmax - other->s.v.health), 1);
		}
		else
		{
			T_Heal (other, self->healamount, 1);
		}

		other->healtimer = g_globalvars.time + self->wait;
	}
}

void SP_trigger_heal(void)
{
	if (strnull(self->noise))
		self->noise = "items/r_item1.wav";

	trap_precache_sound (self->noise);

	InitTrigger ();

	if (self->wait == 0)
	{
		self->wait = 1;
	}
	if (self->healamount == 0)
	{
		self->healamount = 5;
	}
	if (self->healmax == 0)
	{
		self->healmax = 100;
	}
	else if (self->healmax > 250)
	{
		self->healmax = 250;
	}

	self->healtimer = g_globalvars.time;
	self->touch = (func_t) trigger_heal_touch;
}
