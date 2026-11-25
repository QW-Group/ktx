/*
 * The weapon definitions, transcribed from WPredict_Initialize() in
 * src/weaponpred_defs.c. This is the same data the server exports over the wire
 * to native EZCSQC clients, which cannot run mod code; we are mod code, so we
 * carry it directly. Keep the two in step.
 *
 * Each weapon is an animation state machine. The server tells us which state is
 * scheduled next (client_thinkindex) and when it runs (client_nextthink); we
 * walk the same graph from there.
 *
 *   frame  >= 0  set the view model to this frame
 *          <  0  advance the frame, wrapping back to 1 after |frame|
 *   len          how long the state lasts, in seconds; 0 means it terminates
 *   next         state entered when this one expires
 *   alt          state entered when a BRANCH state sees +attack released
 *
 * Values that depend on server configuration are NOT baked in here; they are
 * applied at use from the prediction flags the server sends (see the accessors
 * below).
 *
 * Every member of every row is written out, even the empty ones. Designated
 * initialisers look like the obvious improvement here and are not: fteqcc
 * leaves the members you omit holding whatever was in that memory, silently and
 * without a warning, so a row that skipped proj_off would fire nails at a
 * garbage offset. Spell it out.
 */

/*
 * Which ammo pool a weapon draws on. AMMO_NONE indexes a slot the server never
 * writes, so the axe's requirement of none is satisfied by the same test as
 * everything else.
 */
enum
{
	AMMO_NONE,
	AMMO_SHELLS,
	AMMO_NAILS,
	AMMO_ROCKETS,
	AMMO_CELLS,
	AMMO_COUNT
};

typedef struct
{
	float	frame;
	float	flags;
	string	sound;
	string	sound2;
	string	proj_model;
	vector	proj_vel;		// right, forward, up
	vector	proj_off;		// right, forward, world up
	float	next;
	float	alt;
	float	len;
} anim_t;

typedef struct
{
	string	model;
	float	attack_time;
	float	impulse;
	float	itemflag;
	float	ammo;		// the pool a shot draws from
	float	ammo_min;	// ... and how much it needs to be selectable
	float	anim_count;
	anim_t	anim[9];
} weapon_t;

/*
 * Indexed by the server's weapon_index. Slot 0 is the hook or "no weapon",
 * which is never predicted, so it is left empty.
 */
weapon_t weapons[9] =
{
	// 0: unused
	{ "", 0, 0, 0, AMMO_NONE, 0, 0, { } },

	// 1: axe. Two interchangeable swing chains, frames 1-4 and 5-8; which one
	// runs is picked by a deterministic hash of the fire time (Weapon_AxeEntry).
	{
		"progs/v_axe.mdl", 0.5, 1, IT_AXE, AMMO_NONE, 0, 9,
		{
			{ 0, ANIM_DEFAULT | ANIM_ATTACK, "", "", "", '0 0 0', '0 0 0', 1, 0, 0 },
			{ 1, ANIM_SOUND, "weapons/ax1.wav", "", "", '0 0 0', '0 0 0', 2, 0, 0.1 },
			{ 2, 0, "", "", "", '0 0 0', '0 0 0', 3, 0, 0.1 },
			{ 3, 0, "", "", "", '0 0 0', '0 0 0', 4, 0, 0.1 },
			{ 4, 0, "", "", "", '0 0 0', '0 0 0', 0, 0, 0.1 },
			{ 5, ANIM_SOUND, "weapons/ax1.wav", "", "", '0 0 0', '0 0 0', 6, 0, 0.1 },
			{ 6, 0, "", "", "", '0 0 0', '0 0 0', 7, 0, 0.1 },
			{ 7, 0, "", "", "", '0 0 0', '0 0 0', 8, 0, 0.1 },
			{ 8, 0, "", "", "", '0 0 0', '0 0 0', 0, 0, 0.1 }
		}
	},

	// 2: shotgun
	{
		"progs/v_shot.mdl", 0.5, 2, IT_SHOTGUN, AMMO_SHELLS, 1, 7,
		{
			{ 0, ANIM_DEFAULT | ANIM_ATTACK, "", "", "", '0 0 0', '0 0 0', 1, 0, 0 },
			{ 1, ANIM_MUZZLEFLASH | ANIM_SOUND, "weapons/guncock.wav", "", "", '0 0 0', '0 0 0', 2, 0, 0.1 },
			{ 2, 0, "", "", "", '0 0 0', '0 0 0', 3, 0, 0.1 },
			{ 3, 0, "", "", "", '0 0 0', '0 0 0', 4, 0, 0.1 },
			{ 4, 0, "", "", "", '0 0 0', '0 0 0', 5, 0, 0.1 },
			{ 5, 0, "", "", "", '0 0 0', '0 0 0', 6, 0, 0.1 },
			{ 6, 0, "", "", "", '0 0 0', '0 0 0', 0, 0, 0.1 }
		}
	},

	// 3: super shotgun. Refire is 0.8 under yawn mode, see Weapon_AttackTime.
	{
		"progs/v_shot2.mdl", 0.7, 3, IT_SUPER_SHOTGUN, AMMO_SHELLS, 2, 7,
		{
			{ 0, ANIM_DEFAULT | ANIM_ATTACK, "", "", "", '0 0 0', '0 0 0', 1, 0, 0 },
			{ 1, ANIM_MUZZLEFLASH | ANIM_SOUND, "weapons/shotgn2.wav", "", "", '0 0 0', '0 0 0', 2, 0, 0.1 },
			{ 2, 0, "", "", "", '0 0 0', '0 0 0', 3, 0, 0.1 },
			{ 3, 0, "", "", "", '0 0 0', '0 0 0', 4, 0, 0.1 },
			{ 4, 0, "", "", "", '0 0 0', '0 0 0', 5, 0, 0.1 },
			{ 5, 0, "", "", "", '0 0 0', '0 0 0', 6, 0, 0.1 },
			{ 6, 0, "", "", "", '0 0 0', '0 0 0', 0, 0, 0.1 }
		}
	},

	// 4: nailgun. States 1 and 2 alternate, offsetting the muzzle left and
	// right. Under yawn mode the offset is dropped and nails fly faster.
	{
		"progs/v_nail.mdl", 0.2, 4, IT_NAILGUN, AMMO_NAILS, 1, 3,
		{
			{ 0, ANIM_DEFAULT | ANIM_ATTACK, "", "", "", '0 0 0', '0 0 0', 1, 0, 0 },
			{ -8, ANIM_MUZZLEFLASH | ANIM_SOUND | ANIM_PROJECTILE | ANIM_ATTACK | ANIM_BRANCH,
			  "weapons/rocket1i.wav", "", "progs/spike.mdl", '0 1000 0', '4 0 16', 2, 0, 0.1 },
			{ -8, ANIM_MUZZLEFLASH | ANIM_SOUND | ANIM_PROJECTILE | ANIM_ATTACK | ANIM_BRANCH,
			  "weapons/rocket1i.wav", "", "progs/spike.mdl", '0 1000 0', '-4 0 16', 1, 0, 0.1 }
		}
	},

	// 5: super nailgun
	{
		"progs/v_nail2.mdl", 0.2, 5, IT_SUPER_NAILGUN, AMMO_NAILS, 2, 3,
		{
			{ 0, ANIM_DEFAULT | ANIM_ATTACK, "", "", "", '0 0 0', '0 0 0', 1, 0, 0 },
			{ -8, ANIM_MUZZLEFLASH | ANIM_SOUND | ANIM_PROJECTILE | ANIM_ATTACK | ANIM_BRANCH,
			  "weapons/spike2.wav", "", "progs/s_spike.mdl", '0 1000 0', '0 0 16', 2, 0, 0.1 },
			{ -8, ANIM_MUZZLEFLASH | ANIM_SOUND | ANIM_PROJECTILE | ANIM_ATTACK | ANIM_BRANCH,
			  "weapons/spike2.wav", "", "progs/s_spike.mdl", '0 1000 0', '0 0 16', 1, 0, 0.1 }
		}
	},

	// 6: grenade launcher. Grenades leave from the player origin with no offset.
	{
		"progs/v_rock.mdl", 0.6, 6, IT_GRENADE_LAUNCHER, AMMO_ROCKETS, 1, 7,
		{
			{ 0, ANIM_DEFAULT | ANIM_ATTACK, "", "", "", '0 0 0', '0 0 0', 1, 0, 0 },
			{ 1, ANIM_MUZZLEFLASH | ANIM_SOUND | ANIM_PROJECTILE,
			  "weapons/grenade.wav", "", "progs/grenade.mdl", '0 600 200', '0 0 0', 2, 0, 0.1 },
			{ 2, 0, "", "", "", '0 0 0', '0 0 0', 3, 0, 0.1 },
			{ 3, 0, "", "", "", '0 0 0', '0 0 0', 4, 0, 0.1 },
			{ 4, 0, "", "", "", '0 0 0', '0 0 0', 5, 0, 0.1 },
			{ 5, 0, "", "", "", '0 0 0', '0 0 0', 6, 0, 0.1 },
			{ 6, 0, "", "", "", '0 0 0', '0 0 0', 0, 0, 0.1 }
		}
	},

	// 7: rocket launcher. Rockets fly at 2000 while quad-boosted in midair
	// mode, see Weapon_ProjectileSpeedScale.
	{
		"progs/v_rock2.mdl", 0.8, 7, IT_ROCKET_LAUNCHER, AMMO_ROCKETS, 1, 7,
		{
			{ 0, ANIM_DEFAULT | ANIM_ATTACK, "", "", "", '0 0 0', '0 0 0', 1, 0, 0 },
			{ 1, ANIM_MUZZLEFLASH | ANIM_SOUND | ANIM_PROJECTILE,
			  "weapons/sgun1.wav", "", "progs/missile.mdl", '0 1000 0', '0 8 16', 2, 0, 0.1 },
			{ 2, 0, "", "", "", '0 0 0', '0 0 0', 3, 0, 0.1 },
			{ 3, 0, "", "", "", '0 0 0', '0 0 0', 4, 0, 0.1 },
			{ 4, 0, "", "", "", '0 0 0', '0 0 0', 5, 0, 0.1 },
			{ 5, 0, "", "", "", '0 0 0', '0 0 0', 6, 0, 0.1 },
			{ 6, 0, "", "", "", '0 0 0', '0 0 0', 0, 0, 0.1 }
		}
	},

	// 8: lightning gun. State 0 is the only attack entry without ANIM_DEFAULT,
	// so firing starts there; states 1 and 2 loop while +attack is held. Its
	// refire is fixed at LG_ATTACK_TIME rather than the table's attack_time,
	// matching the server's own special case.
	{
		"progs/v_light.mdl", 0.1, 8, IT_LIGHTNING, AMMO_CELLS, 1, 3,
		{
			{ 0, ANIM_ATTACK | ANIM_SOUND | ANIM_SOUND2 | ANIM_SOUNDAUTO | ANIM_LGBEAM,
			  "weapons/lstart.wav", "weapons/lhit.wav", "", '0 0 0', '0 0 0', 1, 0, 0.1 },
			{ -4, ANIM_MUZZLEFLASH | ANIM_SOUND | ANIM_LGBEAM | ANIM_LTIME | ANIM_ATTACK | ANIM_BRANCH,
			  "weapons/lhit.wav", "", "", '0 0 0', '0 0 0', 2, 0, 0.1 },
			{ -4, ANIM_MUZZLEFLASH | ANIM_SOUND | ANIM_LGBEAM | ANIM_LTIME | ANIM_ATTACK | ANIM_BRANCH,
			  "weapons/lhit.wav", "", "", '0 0 0', '0 0 0', 1, 0, 0.1 }
		}
	}
};

// Cached model indices, filled on first use: the view model is looked up on
// every rendered frame.
static float weapon_modelindex[weapons.length];

float Weapon_Valid(float idx)
{
	if (idx < 1 || idx >= weapons.length)
		return FALSE;

	return weapons[idx].anim_count > 0;
}

float Weapon_ModelIndex(float idx)
{
	if (!Weapon_Valid(idx))
		return 0;

	if (!weapon_modelindex[idx])
		weapon_modelindex[idx] = getmodelindex(weapons[idx].model);

	return weapon_modelindex[idx];
}

/*
 * The item bit a weapon index stands for, or 0 when it names no weapon.
 *
 * Worth going through a function: && does not short-circuit in this dialect, so
 * a range check written beside the array read would not stop it happening. The
 * bounds test has to be a statement of its own, and here it is one.
 */
float Weapon_ItemFlag(float idx)
{
	if (!Weapon_Valid(idx))
		return 0;

	return weapons[idx].itemflag;
}

/*
 * The weapon an impulse selects, or 0 when the impulse names none.
 *
 * Every impulse in the table happens to equal the index of the weapon it
 * selects, which is a coincidence of how the weapons are numbered rather than a
 * rule, so it is looked up by field.
 */
float Weapon_ForImpulse(float impulse)
{
	for (float i = 1; i < weapons.length; i++)
	{
		if (Weapon_Valid(i) && weapons[i].impulse == impulse)
			return i;
	}

	return 0;
}

// Refire delay. Yawn mode slows the super shotgun; the lightning gun ignores
// the table entirely.
float Weapon_AttackTime(float idx, float predflags)
{
	if (weapons[idx].itemflag == IT_SUPER_SHOTGUN && (predflags & PRDFL_YAWNMODE))
		return 0.8;

	return weapons[idx].attack_time;
}

// Nails fly at 1800 in yawn mode, and a quad-boosted rocket in midair mode
// flies at 2000 instead of 1000.
float Weapon_ProjectileSpeedScale(float idx, float predflags)
{
	float item = weapons[idx].itemflag;

	if ((item & (IT_NAILGUN | IT_SUPER_NAILGUN)) && (predflags & PRDFL_YAWNMODE))
		return 1.8;

	if (item == IT_ROCKET_LAUNCHER && (predflags & PRDFL_MIDAIR))
		return 2;

	return 1;
}

// Yawn mode fires nails straight down the middle instead of alternating.
vector Weapon_ProjectileOffset(float idx, float predflags, vector off)
{
	if ((weapons[idx].itemflag & (IT_NAILGUN | IT_SUPER_NAILGUN))
		&& (predflags & PRDFL_YAWNMODE))
	{
		off_x = 0;
	}

	return off;
}

void Weapon_PrecacheAll(void)
{
	for (float w = 1; w < weapons.length; w++)
	{
		if (!Weapon_Valid(w))
			continue;

		precache_model(weapons[w].model);

		for (float a = 0; a < weapons[w].anim_count; a++)
		{
			anim_t *anim = &weapons[w].anim[a];

			if (anim->sound != "")
				precache_sound(anim->sound);
			if (anim->sound2 != "")
				precache_sound(anim->sound2);
			if (anim->proj_model != "")
				precache_model(anim->proj_model);
		}
	}
}
