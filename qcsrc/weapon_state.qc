/*
 * The weapon state machine: parse the server's snapshot, replay unacknowledged
 * input on top of it, present the result as a view model.
 */

typedef struct
{
	float	impulse;
	float	weapon_index;
	float	ammo[AMMO_COUNT];
	float	attack_finished;
	float	client_nextthink;
	float	client_thinkindex;
	float	client_time;
	float	frame;
	float	predflags;
	float	ping;
	float	nails_spent;	// by shots replayed on top of the snapshot; never sent
} wstate_t;

wstate_t	ws_server;		// the last snapshot the server sent
static wstate_t	ws_predicted;	// scratch, rebuilt from ws_server every view frame
static wstate_t	ws_presented;	// what the view model actually shows

static float	ws_have_state;		// a snapshot has arrived
static float	ws_state_frame;		// input frame the snapshot already accounts for
static float	ws_stale;			// holding a snapshot from a previous life
static float	weapon_generation;	// respawn counter, -1 until the first snapshot
static float	respawn_guard_until;// server ignores attacks until this client_time
static float	lg_twidth;			// lightning sound may retrigger from this client_time

// The view model interpolates between two poses; this stamps the swap.
static float	vw_lerptime;

/*
 * Replay walks the same input frames again every time we draw, so a one-shot
 * effect records the newest frame it has already fired on, and each replayed
 * frame is told whether it is allowed to fire one.
 */
static float	replay_frame;
static float	frame_may_sound;
static float	frame_may_projectile;
static float	last_sound_frame;
static float	last_projectile_frame;

void WeaponState_Reset(void)
{
	ws_have_state = FALSE;
	ws_state_frame = 0;
	weapon_generation = -1;
	respawn_guard_until = 0;
	lg_twidth = 0;
	last_sound_frame = 0;
	last_projectile_frame = 0;
	vw_lerptime = 0;
}

// The lightning gun owns two pieces of state outside the snapshot: its sound
// cadence and the predicted bolt.
static void WeaponState_ResetLightning(void)
{
	lg_twidth = 0;
	Beam_Clear();
}

/*
 * Show a state's frame. A negative one in the table means "cycle", counting up
 * to its magnitude and starting again at 1.
 */
static void WeaponState_SetFrame(float frame)
{
	if (frame >= 0)
	{
		ws_predicted.frame = frame;
		return;
	}

	ws_predicted.frame++;

	if (ws_predicted.frame > -frame)
		ws_predicted.frame = 1;
}

// Nothing is scheduled: the animation has run out.
static void WeaponState_Unschedule(void)
{
	ws_predicted.client_thinkindex = 0;
	ws_predicted.client_nextthink = 0;
}

/*
 * Stop predicting until a fresh snapshot arrives. Whatever we hold describes a
 * life that has ended, and animating from it would replay the previous life's
 * last shot.
 */
void WeaponState_Discard(void)
{
	ws_stale = TRUE;
	WeaponState_ResetLightning();
}

// ==========================================================================
// Parsing
// ==========================================================================

/*
 * The generation changes when the player respawns. Everything predicted before
 * that belongs to the previous life, and the server throws away attacks for a
 * moment after spawning, so we do the same. Called once the timing fields are
 * read: the guard runs off the clock in this snapshot, not the one it replaces.
 *
 * The server counts that moment from the weapon clock at the spawn. It sets
 * attack_finished to the same value, and nothing can move it while attacks are
 * being thrown away, so that is where the spawn time is read from. A snapshot
 * that only arrives after the player has fired carries a later attack_finished;
 * its own clock is the earlier of the two then, and the guard is over anyway.
 */
static void WeaponState_NoteGeneration(float generation)
{
	if (weapon_generation >= 0 && generation != weapon_generation)
	{
		respawn_guard_until = min(ws_server.attack_finished, ws_server.client_time)
			+ RESPAWN_GUARD;
		WeaponState_ResetLightning();
	}

	weapon_generation = generation;
}

/*
 * The axe runs one of two interchangeable swing chains. The server reports both
 * as the same think index, so recover the second chain from the frame it is
 * showing.
 */
static void WeaponState_RecoverAxeChain(void)
{
	if (Weapon_ItemFlag(ws_server.weapon_index) != IT_AXE)
		return;

	if (ws_server.frame >= 5
		&& ws_server.client_thinkindex >= 2 && ws_server.client_thinkindex <= 4)
	{
		ws_server.client_thinkindex += 4;
	}
}

void WeaponState_Parse(float isnew)
{
	float sendflags = readbyte();
	float enc, generation = weapon_generation;

	if (sendflags & WEAPONINFO_INDEX)
	{
		ws_server.impulse = readbyte();

		enc = readbyte();
		generation = enc >> WEAPONINFO_GENERATION_SHIFT;
		ws_server.weapon_index = enc & WEAPONINFO_WEAPON_MASK;
	}

	if (sendflags & WEAPONINFO_AMMO_SHELLS)
		ws_server.ammo[AMMO_SHELLS] = readbyte();
	if (sendflags & WEAPONINFO_AMMO_NAILS)
		ws_server.ammo[AMMO_NAILS] = readbyte();
	if (sendflags & WEAPONINFO_AMMO_ROCKETS)
		ws_server.ammo[AMMO_ROCKETS] = readbyte();
	if (sendflags & WEAPONINFO_AMMO_CELLS)
		ws_server.ammo[AMMO_CELLS] = readbyte();

	if (sendflags & WEAPONINFO_ATTACK)
	{
		ws_server.attack_finished = readfloat();
		ws_server.client_nextthink = readfloat();
		ws_server.client_thinkindex = readbyte();
	}

	if (sendflags & WEAPONINFO_TIMING)
	{
		ws_server.client_time = readfloat();
		ws_server.frame = readbyte();
	}

	if (sendflags & WEAPONINFO_PRED_PING)
	{
		ws_server.predflags = readbyte();
		ws_server.ping = readbyte() / 1000;
	}

	WeaponState_NoteGeneration(generation);
	WeaponState_RecoverAxeChain();

	/*
	 * Say once, when the first snapshot lands, that prediction is live; nothing
	 * else would tell the player the csprogs is doing anything. Receiving this
	 * at all means the server has accepted us as a predicting client, which is
	 * the whole of the handshake we have.
	 */
	if (!ws_have_state)
		print("\nCSQC Antilag ready\n");

	/*
	 * Replay has to start from the input frame this snapshot already includes.
	 * Capture it now: by the time we draw, the global has moved on, and any
	 * frame the snapshot did not see would be skipped.
	 */
	ws_state_frame = servercommandframe;
	ws_have_state = TRUE;
	ws_stale = FALSE;

	if (isnew)
		vw_lerptime = 0;
}

// ==========================================================================
// State machine
//
// A state is entered, sets the view frame and any effects, and schedules the
// next one. client_thinkindex names the state that is scheduled, not the one
// being shown.
// ==========================================================================

// Whether the player could switch to this weapon: they have to be carrying it
// and hold enough of what it fires.
static float WeaponState_CanSelect(float idx)
{
	if (!(Player_Items() & weapons[idx].itemflag))
		return FALSE;

	return ws_predicted.ammo[weapons[idx].ammo] >= weapons[idx].ammo_min;
}

enum
{
	SWITCH_APPLIED,
	SWITCH_BLOCKED,		// mid-animation; the impulse stays queued
	SWITCH_UNRESOLVED	// cannot select it; drop the impulse
};

static float WeaponState_Switch(float impulse)
{
	float idx;

	// The server refuses to switch until the current animation finishes, and
	// remembers the impulse until then.
	if (ws_predicted.client_time < ws_predicted.attack_finished)
		return SWITCH_BLOCKED;

	idx = Weapon_ForImpulse(impulse);

	if (!idx)
		return SWITCH_UNRESOLVED;

	if (!WeaponState_CanSelect(idx))
		return SWITCH_UNRESOLVED;

	if (ws_predicted.weapon_index == idx)
		return SWITCH_APPLIED;

	if (Weapon_ItemFlag(ws_predicted.weapon_index) == IT_LIGHTNING)
		WeaponState_ResetLightning();

	ws_predicted.weapon_index = idx;
	WeaponState_Unschedule();
	ws_predicted.frame = max(0, weapons[idx].anim[0].frame);
	return SWITCH_APPLIED;
}

/*
 * The sounds a state makes on entry.
 *
 * The lightning gun ticks every 100 ms but its sounds repeat far more slowly,
 * so lg_twidth holds the next client_time either may be heard at.
 */
static void WeaponState_PlaySounds(anim_t *anim)
{
	float flags = anim->flags;
	float chan = (flags & ANIM_SOUNDAUTO) ? CHAN_AUTO : CHAN_WEAPON;

	if ((flags & ANIM_LTIME) && ws_predicted.client_time < lg_twidth)
		return;

	Sound_PlayPredicted(anim->sound, chan);

	if (flags & ANIM_SOUND2)
	{
		/*
		 * Lightning restarts its loop sound freely but only re-strikes on the
		 * same slow cadence as the beam.
		 */
		if ((flags & ANIM_LGBEAM) && !(flags & ANIM_LTIME))
		{
			if (ws_predicted.client_time >= lg_twidth)
			{
				Sound_PlayPredicted(anim->sound2, CHAN_WEAPON);
				lg_twidth = ws_predicted.client_time + LG_SOUND_WINDOW;
			}
		}
		else
		{
			Sound_PlayPredicted(anim->sound2, CHAN_WEAPON);
		}
	}

	if (flags & ANIM_LTIME)
		lg_twidth = ws_predicted.client_time + LG_SOUND_WINDOW;
}

// Effects fire on entering a state, but only on a frame we have not already
// played them for: replay walks the same frames again every time we draw.
static void WeaponState_PlayEffects(float idx, float a)
{
	anim_t *anim = &weapons[idx].anim[a];
	float flags = anim->flags;

	if (frame_may_sound && (flags & ANIM_SOUND))
	{
		WeaponState_PlaySounds(anim);
		last_sound_frame = max(last_sound_frame, replay_frame);
	}

	if (frame_may_projectile && (flags & ANIM_PROJECTILE))
	{
		if (Projectile_SpawnLocal(idx, a, replay_frame, ws_predicted.ping,
		                          ws_predicted.predflags))
		{
			last_projectile_frame = max(last_projectile_frame, replay_frame);
		}
	}

	if (frame_may_projectile && (flags & ANIM_LGBEAM))
	{
		Beam_Predict();
		last_projectile_frame = max(last_projectile_frame, replay_frame);
	}
}

/*
 * The weapon whose shot a firing state produces, which is not always the one in
 * hand. The super nailgun spends two nails a shot; with a single nail left the
 * server fires that one as the nailgun would, with its sound, its spike and its
 * alternating muzzle offset.
 *
 * The snapshot's nail count does not include shots still being replayed, so
 * those are counted here. That assumes shots cost ammo, which they do not
 * before a match or in deathmatch 4; there the count is high enough for the
 * few replayed shots not to matter.
 */
static float WeaponState_FiredAs(float idx, float flags)
{
	float item = weapons[idx].itemflag;
	float nails, nailgun;

	if (!(flags & ANIM_PROJECTILE))
		return idx;
	if (item != IT_NAILGUN && item != IT_SUPER_NAILGUN)
		return idx;

	nails = ws_predicted.ammo[AMMO_NAILS] - ws_predicted.nails_spent;

	if (item == IT_SUPER_NAILGUN && nails >= 2)
	{
		ws_predicted.nails_spent += 2;
		return idx;
	}

	ws_predicted.nails_spent += 1;

	if (item == IT_SUPER_NAILGUN)
	{
		nailgun = Weapon_ForItem(IT_NAILGUN);

		if (nailgun)
			return nailgun;
	}

	return idx;
}

static void WeaponState_Enter(float a, float buttons)
{
	float idx = ws_predicted.weapon_index;

	if (a < 0 || a >= weapons[idx].anim_count)
	{
		WeaponState_Unschedule();
		return;
	}

	anim_t *anim = &weapons[idx].anim[a];
	float flags = anim->flags;

	// The idle state ends the animation.
	if (a == 0 && (flags & ANIM_DEFAULT))
	{
		WeaponState_Unschedule();
		if (anim->frame >= 0)
			ws_predicted.frame = anim->frame;
		return;
	}

	/*
	 * A looping fire state re-checks the trigger when it comes round. Letting
	 * go, or queueing a weapon switch, drops out to the idle state.
	 */
	if ((flags & ANIM_ATTACK) && (flags & ANIM_BRANCH)
		&& (!(buttons & BUTTON_ATTACK) || ws_predicted.impulse))
	{
		/*
		 * Every state in the table leaves .alt at 0, so this asks whether this
		 * weapon has an idle state to fall back to. Lightning's state 0 is an
		 * attack entry rather than an idle one, so it simply stops.
		 */
		if (weapons[idx].anim[anim->alt].flags & ANIM_DEFAULT)
		{
			WeaponState_Enter(anim->alt, buttons);
		}
		else
		{
			WeaponState_Unschedule();
		}
		return;
	}

	WeaponState_PlayEffects(WeaponState_FiredAs(idx, flags), a);

	WeaponState_SetFrame(anim->frame);

	if (flags & ANIM_ATTACK)
	{
		if (weapons[idx].itemflag == IT_LIGHTNING)
			ws_predicted.attack_finished = ws_predicted.client_time + LG_ATTACK_TIME;
		else
			ws_predicted.attack_finished = ws_predicted.client_time
				+ Weapon_AttackTime(idx, ws_predicted.predflags);
	}

	ws_predicted.client_thinkindex = anim->next;

	if (anim->len)
		ws_predicted.client_nextthink = ws_predicted.client_time + anim->len;
	else
		ws_predicted.client_nextthink = 0;
}

// The server reproduces one scheduled transition per player frame, so we do the
// same: no catching up on several at once.
static void WeaponState_Think(float buttons)
{
	float a = ws_predicted.client_thinkindex;
	float idx = ws_predicted.weapon_index;
	float held;

	if (!ws_predicted.client_nextthink)
		return;
	if (a < 0 || a >= weapons[idx].anim_count)
		return;
	if (ws_predicted.client_time < ws_predicted.client_nextthink)
		return;

	// The transition happens at its scheduled time, not at the end of the frame
	// that crossed it, so timings stay on the server's 100ms grid.
	held = ws_predicted.client_time;
	ws_predicted.client_time = ws_predicted.client_nextthink;
	ws_predicted.client_nextthink = 0;
	WeaponState_Enter(a, buttons);
	ws_predicted.client_time = held;
}

/*
 * The axe alternates between two swing animations. The server picks with a hash
 * of the fire time rather than a random number, so we can usually reach the same
 * answer.
 *
 * Usually, not always: the server evaluates this in double precision and QuakeC
 * has only floats, so late in a long match the product loses enough precision to
 * truncate to a different integer. The cost is that the swing occasionally pops
 * to the other chain when the snapshot lands, which is cosmetic.
 */
static float WeaponState_AxeEntry(void)
{
	float t = ws_predicted.client_time;
	float r;

	r = fabs((((int)(t * 931.75) << 11) + ((int)t >> 6)) % 1000) / 1000;

	if ((r >= 0.25 && r < 0.5) || r >= 0.75)
		return 5;

	return 1;
}

static void WeaponState_Attack(float buttons)
{
	float idx = ws_predicted.weapon_index;
	anim_t *entry = &weapons[idx].anim[0];

	if (ws_predicted.client_time < ws_predicted.attack_finished)
		return;
	if (!(buttons & BUTTON_ATTACK))
		return;

	// State 0 is where every weapon is entered: the idle state it returns to
	// between shots, and the one it fires from. Not being an attack entry means
	// this weapon does not fire at all.
	if (!(entry->flags & ANIM_ATTACK))
		return;

	// Lightning has no idle state, so firing starts at state 0 itself.
	if (!(entry->flags & ANIM_DEFAULT))
	{
		WeaponState_Enter(0, buttons);
		return;
	}

	if (weapons[idx].itemflag == IT_AXE)
		WeaponState_Enter(WeaponState_AxeEntry(), buttons);
	else
		WeaponState_Enter(entry->next, buttons);

	ws_predicted.attack_finished = ws_predicted.client_time
		+ Weapon_AttackTime(idx, ws_predicted.predflags);
}

// ==========================================================================
// Replay
// ==========================================================================

static void WeaponState_Step(void)
{
	/*
	 * The server has switched prediction off, usually because the round has not
	 * started or the player is hooked. Keep the clock moving and hold the
	 * weapon closed.
	 */
	if (!Pred_ServerAllows(ws_predicted.predflags))
	{
		ws_predicted.client_time += input_timelength;
		ws_predicted.impulse = 0;
		ws_predicted.attack_finished = max(ws_predicted.attack_finished,
		                                   ws_predicted.client_time + ATTACK_HOLDOFF);
		return;
	}

	if (!player_playable)
	{
		// Dead or spectating: the weapon clock does not advance at all.
		ws_predicted.impulse = 0;
		ws_predicted.attack_finished = ws_predicted.client_time + ATTACK_HOLDOFF;
		return;
	}

	ws_predicted.client_time += input_timelength;

	if (input_impulse)
		ws_predicted.impulse = input_impulse;

	// Just after respawning the server discards attacks and switches.
	if (ws_predicted.client_time < respawn_guard_until)
		return;

	WeaponState_Think(input_buttons);

	if (ws_predicted.impulse)
	{
		// A blocked switch stays queued, and the old weapon does not fire on a
		// frame where one is waiting.
		if (WeaponState_Switch(ws_predicted.impulse) == SWITCH_BLOCKED)
			return;

		ws_predicted.impulse = 0;
	}

	WeaponState_Attack(input_buttons);
}

void WeaponState_Replay(void)
{
	float threshold, last_shown;

	// Not predicting at all: the engine draws the gun and plays every sound.
	if (!Pred_Enabled())
		return;

	if (!ws_have_state)
		return;

	/*
	 * The player has just become playable again but every snapshot we hold
	 * predates that. Replaying now would animate, and fire the effects of, a
	 * shot belonging to the previous life.
	 */
	if (ws_stale)
	{
		ws_presented = ws_server;
		return;
	}

	// Too far behind to reconstruct: the input ring has wrapped.
	if (clientcommandframe - ws_state_frame >= PRED_MASK)
	{
		ws_presented = ws_server;
		return;
	}

	ws_predicted = ws_server;
	ws_presented = ws_server;

	/*
	 * Effects and the shown animation lag the newest prediction slightly, so a
	 * shot's sound, its projectile and its animation all appear together rather
	 * than the animation running ahead of the rest.
	 */
	threshold = bound(ws_state_frame + 1,
	                  clientcommandframe - (autocvar_cl_predict_buffer + 1),
	                  clientcommandframe - 1);

	last_shown = ws_state_frame;

	// The frame being assembled now has not been sent, so it is not replayed.
	for (float f = ws_state_frame + 1; f < clientcommandframe; f++)
	{
		if (!getinputstate(f))
			break;

		replay_frame = f;
		// QuakeC's = binds tighter than &&: without the outer parentheses
		// these would assign (f <= threshold) alone.
		frame_may_sound = ((f <= threshold) && (f > last_sound_frame));
		frame_may_projectile = ((f <= threshold) && (f > last_projectile_frame));

		WeaponState_Step();

		if (f <= threshold)
		{
			ws_presented = ws_predicted;
			last_shown = f;
		}
	}

	frame_may_sound = FALSE;
	frame_may_projectile = FALSE;

	if (autocvar_cl_predict_debug > 1)
	{
		print(sprintf("wpred: %g..%g thr %g shown %g idx %g frame %g\n",
		               ws_state_frame + 1, clientcommandframe - 1, threshold,
		               last_shown, ws_presented.weapon_index, ws_presented.frame));
	}
}

// ==========================================================================
// Presentation
// ==========================================================================

// Everything that would make the engine hide its own gun, plus the cases where
// we have nothing trustworthy to show.
static float WeaponState_Visible(void)
{
	if (!Pred_Enabled())
		return FALSE;
	if (!ws_have_state || ws_stale)
		return FALSE;
	if (!player_playable)
		return FALSE;
	if (!Weapon_Valid(ws_presented.weapon_index))
		return FALSE;
	if (!Pred_ServerAllows(ws_presented.predflags))
		return FALSE;
	if (!cvar("r_drawviewmodel"))
		return FALSE;
	if ((Player_Items() & IT_INVISIBILITY) && !cvar("r_drawviewmodelinvis"))
		return FALSE;

	return TRUE;
}

// Returns true when we are drawing the gun ourselves.
float WeaponState_Present(void)
{
	float mdl, pose;

	if (!WeaponState_Visible())
	{
		viewweapon.modelindex = 0;
		return FALSE;
	}

	mdl = Weapon_ModelIndex(ws_presented.weapon_index);
	pose = ws_presented.frame;

	if (!mdl)
	{
		viewweapon.modelindex = 0;
		return FALSE;
	}

	// Start a new blend whenever the pose changes.
	if (viewweapon.modelindex != mdl)
	{
		viewweapon.modelindex = mdl;
		viewweapon.frame = pose;
		viewweapon.frame2 = pose;
		vw_lerptime = cltime;
	}
	else if (viewweapon.frame != pose)
	{
		viewweapon.frame2 = viewweapon.frame;
		viewweapon.frame = pose;
		vw_lerptime = cltime;
	}

	viewweapon.lerpfrac = bound(0, 1 - (cltime - vw_lerptime) * VIEWMODEL_BLEND_RATE, 1);

	/*
	 * A view model entity is positioned in view space. The engine applies its
	 * own bob, but the user's gun offsets are ours to reproduce.
	 */
	viewweapon.origin = [cvar("cl_gunz"), -cvar("cl_gunx"), -cvar("cl_guny")];
	viewweapon.angles = [cvar("cl_gunanglex"), cvar("cl_gunangley"),
	                     cvar("cl_gunanglez")];

	addentity(viewweapon);
	return TRUE;
}
