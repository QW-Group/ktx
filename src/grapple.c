/*
 * Original 'Morning Star' (Grapple Hook) by "Mike" <amichael@asu.alasu.edu> 
 * Quakeworld-friendly grapple by Wedge (Steve Bond)
 * PureCTF changes by Methabol
 *
 *
 *  $Id$
 */

#include "g_local.h"

void SpawnBlood(vec3_t dest, float damage);

//
// GrappleReset - Removes the hook and resets its owner's state.
//                 expects a pointer to the hook
//
void GrappleReset(gedict_t *rhook)
{
	gedict_t *owner = PROG_TO_EDICT(rhook->s.v.owner);

	if (owner == world)
	{
		return;
	}

	sound(owner, CHAN_NO_PHS_ADD + CHAN_WEAPON, "weapons/bounce2.wav", 1, ATTN_NORM);
	owner->on_hook = false;
	owner->hook_out = false;
	owner->s.v.weaponframe = 0;

	// Original 3wave code added 0.25 to time, but instant-switch is the qw way.
	owner->attack_finished = g_globalvars.time;

	rhook->think = (func_t) SUB_Remove;
	rhook->s.v.nextthink = next_frame();
}

//
// GrappleTrack - Constantly updates the hook's position relative to
//                 what it's hooked to. Inflicts damage if attached to
//                 a player that is not on the same team as the hook's
//                 owner.
//
void GrappleTrack()
{
	gedict_t *enemy = PROG_TO_EDICT(self->s.v.enemy);
	gedict_t *owner = PROG_TO_EDICT(self->s.v.owner);

	// Release dead targets
	if ((enemy->ct == ctPlayer) && ISDEAD(enemy))
	{
		owner->on_hook = false;
	}

	// drop the hook if owner is dead or has released the button
	if (!owner->on_hook || (owner->s.v.health <= 0))
	{
		GrappleReset(self);

		return;
	}

	if (enemy->ct == ctPlayer)
	{
		if (!CanDamage(enemy, owner))
		{
			GrappleReset(self);

			return;
		}

		// move the hook along with the player.  It's invisible, but
		// we need this to make the sound come from the right spot
		setorigin(self, PASSVEC3(enemy->s.v.origin));

		// only deal damage every 100ms 	
		if (g_globalvars.time >= (owner->hook_damage_time + 0.1))
		{
			owner->hook_damage_time = g_globalvars.time;
			sound(self, CHAN_WEAPON, "blob/land1.wav", 1, ATTN_NORM);
			enemy->deathtype = dtHOOK;
			T_Damage(enemy, self, owner, 1);
			trap_makevectors(self->s.v.v_angle);
			SpawnBlood(enemy->s.v.origin, 1);
		}
	}

	// If the hook is not attached to the player, constantly copy
	// the target's velocity. Velocity copying DOES NOT work properly
	// for a hooked client. 
	if (enemy->ct != ctPlayer)
	{
		VectorCopy(enemy->s.v.velocity, self->s.v.velocity);
	}

	self->s.v.nextthink = next_frame();
}

//
// MakeLink - spawns the chain link entities
//
gedict_t* MakeLink()
{
	newmis = spawn();
	g_globalvars.newmis = EDICT_TO_PROG(newmis);

	newmis->s.v.movetype = MOVETYPE_FLYMISSILE;
	newmis->s.v.solid = SOLID_NOT;
	newmis->s.v.owner = EDICT_TO_PROG(self);
	SetVector(newmis->s.v.avelocity, 200, 200, 200);

	if (k_ctf_custom_models)
	{
		setmodel(newmis, "progs/bit.mdl");
	}
	else
	{
		setmodel(newmis, "progs/spike.mdl");
	}

	setorigin(newmis, PASSVEC3(self->s.v.origin));
	setsize(newmis, 0, 0, 0, 0, 0, 0);

	return newmis;
}

//
// RemoveChain - Removes all chain link entities; this is a separate
//                function because CLIENT also needs to be able
//                to remove the chain. Only one function required to
//                remove all links.
//
void RemoveChain()
{
	self->think = (func_t) SUB_Remove;
	self->s.v.nextthink = next_frame();

	if (self->s.v.goalentity)
	{
		gedict_t *goal = PROG_TO_EDICT(self->s.v.goalentity);
		goal->think = (func_t) SUB_Remove;
		goal->s.v.nextthink = next_frame();

		if (goal->s.v.goalentity)
		{
			gedict_t *goal2 = PROG_TO_EDICT(goal->s.v.goalentity);
			goal2->think = (func_t) SUB_Remove;
			goal2->s.v.nextthink = next_frame();
		}
	}
}

//
// Update_Chain - Repositions the chain links each frame. This single function
//                maintains the positions of all of the links. Only one link
//                is thinking every frame. 
//
void UpdateChain()
{
	vec3_t t1, t2, t3;
	vec3_t temp;
	gedict_t *owner = PROG_TO_EDICT(self->s.v.owner), *goal, *goal2;

	if (!owner->hook_out)
	{
		self->think = (func_t) RemoveChain;
		self->s.v.nextthink = next_frame();

		return;
	}

	// allow hook to reset mid-throw if clanring hook is enabled
	if (cvar("k_ctf_cr_hook"))
	{
		if (!owner->s.v.button0 && (owner->s.v.weapon == IT_HOOK))
		{
			GrappleReset(owner->hook);
		}
	}

	VectorSubtract(owner->hook->s.v.origin, owner->s.v.origin, temp);
	VectorScale(temp, 0.25, t1);
	VectorAdd(t1, owner->s.v.origin, t1);
	VectorScale(temp, 0.50, t2);
	VectorAdd(t2, owner->s.v.origin, t2);
	VectorScale(temp, 0.75, t3);
	VectorAdd(t3, owner->s.v.origin, t3);

	goal = PROG_TO_EDICT(self->s.v.goalentity);
	goal2 = PROG_TO_EDICT(goal->s.v.goalentity);
	// These numbers are correct assuming 3 links.
	// 4 links would be *20 *40 *60 and *80
	setorigin(self, PASSVEC3(t1));
	setorigin(goal, PASSVEC3(t2));
	setorigin(goal2, PASSVEC3(t3));

	self->s.v.nextthink = next_frame();
}

//
// BuildChain - Builds the chain (linked list)
//
void BuildChain()
{
	self->s.v.goalentity = EDICT_TO_PROG(MakeLink());
	PROG_TO_EDICT(self->s.v.goalentity)->think = (func_t)UpdateChain;
	PROG_TO_EDICT(self->s.v.goalentity)->s.v.nextthink = next_frame();
	PROG_TO_EDICT(self->s.v.goalentity)->s.v.owner = self->s.v.owner;
	PROG_TO_EDICT(self->s.v.goalentity)->s.v.goalentity = EDICT_TO_PROG(MakeLink());
	PROG_TO_EDICT(PROG_TO_EDICT(self->s.v.goalentity)->s.v.goalentity)->s.v.goalentity =
			EDICT_TO_PROG(MakeLink());
}

void GrappleAnchor()
{
	gedict_t *owner = PROG_TO_EDICT(self->s.v.owner);

	if (other == owner)
	{
		return;
	}

	// DO NOT allow the grapple to hook to any projectiles, no matter WHAT!
	// if you create new types of projectiles, make sure you use one of the
	// classnames below or write code to exclude your new classname so
	// grapples will not stick to them.

	if (streq(other->classname, "rocket") || streq(other->classname, "grenade")
			|| streq(other->classname, "spike") || streq(other->classname, "hook"))
	{
		return;
	}

	if (other->ct == ctPlayer)
	{
		// grappling players in prewar is annoying
		if ((match_in_progress != 2) || ((tp_num() == 4) && streq(getteam(other), getteam(owner))))
		{
			GrappleReset(self);

			return;
		}

		owner->hook_damage_time = g_globalvars.time;
		sound(self, CHAN_WEAPON, "player/axhit1.wav", 1, ATTN_NORM);
		other->deathtype = dtHOOK;
		T_Damage(other, self, owner, 10);

		// make hook invisible since we will be pulling directly
		// towards the player the hook hit. Quakeworld makes it
		// too quirky to try to match hook's velocity with that of
		// the client that it hit. 
		setmodel(self, "");
	}
	else
	{
		sound(self, CHAN_WEAPON, "player/axhit2.wav", 1, ATTN_NORM);

		// One point of damage inflicted upon impact. Subsequent
		// damage will only be done to PLAYERS... this way secret
		// doors and triggers will only be damaged once.
		if (other->s.v.takedamage)
		{
			other->deathtype = dtHOOK;
			T_Damage(other, self, owner, 1);
		}

		SetVector(self->s.v.velocity, 0, 0, 0);
		SetVector(self->s.v.avelocity, 0, 0, 0);
	}

	// conveniently clears the sound channel of the CHAIN1 sound,
	// which is a looping sample and would continue to play. Tink1 is
	// the least offensive choice, as NULL.WAV loops and clogs the
	// channel with silence
	sound(owner, CHAN_NO_PHS_ADD + CHAN_WEAPON, "weapons/tink1.wav", 1, ATTN_NORM);

	if (!owner->s.v.button0)
	{
		GrappleReset(self);

		return;
	}

	if ((int)owner->s.v.flags & FL_ONGROUND)
	{
		owner->s.v.flags -= FL_ONGROUND;
	}

	owner->on_hook = true;
	sound(owner, CHAN_WEAPON, "weapons/chain2.wav", 1, ATTN_NORM);
	owner->ctf_sound = true;

	self->s.v.enemy = EDICT_TO_PROG(other);
	self->think = (func_t) GrappleTrack;
	self->s.v.nextthink = next_frame();
	self->s.v.solid = SOLID_NOT;
	self->touch = (func_t) SUB_Null;
}

// Called from client.c
void GrappleService()
{
	vec3_t hookVector, hookVelocity;
	gedict_t *enemy;

	// drop the hook if player lets go of fire
	if (!self->s.v.button0)
	{
		if (self->s.v.weapon == IT_HOOK)
		{
			GrappleReset(self->hook);

			return;
		}
	}

	enemy = PROG_TO_EDICT(self->hook->s.v.enemy);

	// If hooked to a player, track them directly!
	if (enemy->ct == ctPlayer)
	{
		VectorSubtract(enemy->s.v.origin, self->s.v.origin, hookVector);
	}
	else
		VectorSubtract(self->hook->s.v.origin, self->s.v.origin, hookVector);

	// No longer going to factor maxspeed into grapple velocity
	// Using standard grapple velocity of 800 set from original 3wave
	// purectf velocity = 2.35 * 320 or 360 * 1 = 750 or 846

	VectorCopy(hookVector, hookVelocity);
	VectorNormalize(hookVelocity);

	if (self->ctf_flag & CTF_RUNE_HST)
	{
		VectorScale(hookVelocity, 800 + (100 * (cvar("k_ctf_rune_power_hst") / 2 + 1)),
					self->s.v.velocity);
	}
	else
	{
		VectorScale(hookVelocity, 800, self->s.v.velocity);
	}

	if (vlen(hookVector) <= 100) // cancel chain sound
	{
		if (self->ctf_sound)
		{
			// If there is a chain, ditch it now. We're
			// close enough. Having extra entities lying around
			// is never a good idea.
			if (self->hook->s.v.goalentity)
			{
				PROG_TO_EDICT(self->hook->s.v.goalentity)->think = (func_t) RemoveChain;
				PROG_TO_EDICT(self->hook->s.v.goalentity)->s.v.nextthink = next_frame();
			}

			sound(self, CHAN_NO_PHS_ADD + CHAN_WEAPON, "weapons/chain3.wav", 1, ATTN_NORM);
			self->ctf_sound = false; // reset the sound channel.
		}
	}
}

// Called from weapons.c
void GrappleThrow()
{
	if (self->hook_out) // only throw once
	{
		return;
	}

	g_globalvars.msg_entity = EDICT_TO_PROG(self);
	WriteByte( MSG_ONE, SVC_SMALLKICK);

	// chain out sound (loops)
	sound(self, CHAN_WEAPON, "weapons/chain1.wav", 1, ATTN_NORM);

	newmis = spawn();
	g_globalvars.newmis = EDICT_TO_PROG(newmis);
	newmis->s.v.movetype = MOVETYPE_FLYMISSILE;
	newmis->s.v.solid = SOLID_BBOX;
	newmis->s.v.owner = EDICT_TO_PROG(self);
	self->hook = newmis;
	newmis->classname = "hook";

	trap_makevectors(self->s.v.v_angle);

	// Weapon velocitys should not be based on server maxspeed imo
	// Removing purectf velocity changes ( 2.5 * self->maxspeed )

	if (self->ctf_flag & CTF_RUNE_HST)
	{
		VectorScale(
				g_globalvars.v_forward,
				cvar("k_ctf_cr_hook") ? 1200 : 800 + (100 * (cvar("k_ctf_rune_power_hst") / 2 + 1)),
				newmis->s.v.velocity);
	}
	else
	{
		VectorScale(g_globalvars.v_forward, cvar("k_ctf_cr_hook") ? 1200 : 800,
					newmis->s.v.velocity);
	}

	SetVector(newmis->s.v.avelocity, 0, 0, -500);

	newmis->touch = (func_t) GrappleAnchor;
	newmis->think = (func_t) BuildChain;
	newmis->s.v.nextthink = next_frame();

	if (k_ctf_custom_models)
	{
		setmodel(newmis, "progs/star.mdl");
	}
	else
	{
		setmodel(newmis, "progs/v_spike.mdl");
	}

	setorigin(newmis, self->s.v.origin[0] + g_globalvars.v_forward[0] * 16,
				self->s.v.origin[1] + g_globalvars.v_forward[1] * 16,
				self->s.v.origin[2] + g_globalvars.v_forward[2] * 16 + 16);
	setsize(newmis, 0, 0, 0, 0, 0, 0);
	self->hook_out = true;
}
