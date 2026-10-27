/*
 * weaponpred.qc - data-driven weapon/viewmodel prediction FSM.
 *
 * Rebuilds the local viewmodel animation, predicted firing sounds, predicted
 * projectiles and LG beams ahead of server acknowledgement by replaying pending
 * usercmds through the animation FSM described by the server's weapon
 * definitions (see weapondef.qc). Authoritative game state stays server-side;
 * this only hides latency.
 *
 * Ported from unezQuake's native EZCSQC (src/cl_ezcsqc.c). All FSM helpers
 * operate on the global weapon_predicted state, matching the reference's ws
 * pointer during replay.
 */

//============================================================================
// Predicted-sound registry + playback (shared with CSQC_Event_Sound suppression)
//============================================================================

#define WEPPRED_MAXSOUNDS 32

string  reg_sound_name[WEPPRED_MAXSOUNDS];
int     reg_sound_mask[WEPPRED_MAXSOUNDS];
int     reg_sound_chan[WEPPRED_MAXSOUNDS];
int     reg_sound_count;

void WeaponPred_RegisterSound(int sound, int mask, int chan)
{
	string nm = soundnameforindex(sound);
	if (nm == "" || !nm)
		return;

	// De-duplicate on name+channel so the table stays small.
	for (int i = 0; i < reg_sound_count; i++)
	{
		if (reg_sound_name[i] == nm && reg_sound_chan[i] == chan)
		{
			reg_sound_mask[i] |= mask;
			return;
		}
	}

	if (reg_sound_count >= WEPPRED_MAXSOUNDS)
		return;

	reg_sound_name[reg_sound_count] = nm;
	reg_sound_mask[reg_sound_count] = mask;
	reg_sound_chan[reg_sound_count] = chan;
	reg_sound_count++;
}

float WeaponPred_SoundMaskEnabled(int mask)
{
	int value = autocvar_cl_predict_weaponsound;
	if (!value)
		return FALSE;
	return (value == 1) || !(value & mask);
}

void WeaponPred_PlaySound(string name, int mask, int chan)
{
	if (name == "" || !name)
		return;
	if (!WeaponPred_SoundMaskEnabled(mask))
		return;
	sound(viewweapon, chan, name, 1, ATTN_NONE);
}

// Used by CSQC_Event_Sound: should the given server sound on the local player be
// suppressed because we predicted it locally? SOUNDAUTO predictions register on
// channel 0 and also match the server's weapon-channel echo.
float WeaponPred_SuppressSound(float channel, string sample)
{
	for (int i = 0; i < reg_sound_count; i++)
	{
		if (reg_sound_name[i] != sample)
			continue;
		if (!(reg_sound_chan[i] == channel || reg_sound_chan[i] == 0))
			continue;
		if (!WeaponPred_SoundMaskEnabled(reg_sound_mask[i]))
			continue;
		return TRUE;    // suppress: we already played this locally
	}
	return FALSE;
}

//============================================================================
// Viewmodel model/frame selection
//============================================================================

void WeaponPred_SetModel()
{
	int windex = weapon_presented.weapon_index;

	if (!WeaponPred_DefinitionReady(windex))
	{
		// No definition (e.g. axe on a server without an axe def): hide the
		// predicted viewmodel and let the engine draw its own.
		viewweapon.modelindex = 0;
		weaponpred_drawn = FALSE;
		return;
	}

	weaponpred_drawn = TRUE;
	viewweapon.origin = [autocvar_cl_gunz, -autocvar_cl_gunx, -autocvar_cl_guny];

	float oldframe = viewweapon.frame;
	float oldmodel = viewweapon.modelindex;

	viewweapon.modelindex = wpredict_definitions[windex].modelindex;
	viewweapon.frame = weapon_presented.frame;

	if (oldframe != viewweapon.frame)
	{
		viewweapon.frame2 = oldframe;
		viewweapon.lerpfrac = 1;
	}
	if (oldmodel != viewweapon.modelindex)
		viewweapon.lerpfrac = 0;

	viewweapon.lerpfrac = max(0, viewweapon.lerpfrac - frametime * 10);
}

//============================================================================
// Effect firing (sounds / projectiles / beams) on newly-crossed frames
//============================================================================

void WeaponPred_PlayEffects(int windex, int animindex)
{
	// Prediction replays historical usercmds each render frame; only fire an
	// effect once, for a newly-crossed frame.
	if (!is_effectframe)
		return;
	if (autocvar_cl_nopred_weapon)
		return;

	int flags = wpredict_definitions[windex].anims[animindex].flags;

	if ((current_effect_flags & EZCSQC_EFFECT_SOUND) && (flags & WEPPREDANIM_SOUND))
	{
		int chan = WEPPREDANIM_HAS(flags, WEPPREDANIM_SOUNDAUTO) ? 0 : 1;
		float lg_throttled = WEPPREDANIM_HAS(flags, WEPPREDANIM_LTIME) && weapon_predicted.client_time < lg_twidth;

		if (!lg_throttled)
		{
			WeaponPred_PlaySound(wpredict_definitions[windex].anims[animindex].sound_name,
				wpredict_definitions[windex].anims[animindex].soundmask, chan);

			if (WEPPREDANIM_HAS(flags, WEPPREDANIM_SOUND2))
			{
				if ((flags & WEPPREDANIM_LGBEAM) && !WEPPREDANIM_HAS(flags, WEPPREDANIM_LTIME))
				{
					if (weapon_predicted.client_time >= lg_twidth)
					{
						WeaponPred_PlaySound(wpredict_definitions[windex].anims[animindex].sound2_name,
							wpredict_definitions[windex].anims[animindex].soundmask2, 1);
						lg_twidth = weapon_predicted.client_time + 0.6;
					}
				}
				else
				{
					WeaponPred_PlaySound(wpredict_definitions[windex].anims[animindex].sound2_name,
						wpredict_definitions[windex].anims[animindex].soundmask2, 1);
				}
			}
		}
		if (WEPPREDANIM_HAS(flags, WEPPREDANIM_LTIME) && !lg_throttled)
			lg_twidth = weapon_predicted.client_time + 0.6;

		last_sound_effectframe = max(last_sound_effectframe, current_predframe);
	}

	if ((current_effect_flags & EZCSQC_EFFECT_PROJECTILE) && (flags & WEPPREDANIM_PROJECTILE))
	{
		if (WeaponPred_SpawnProjectile(windex, animindex))
			last_projectile_effectframe = max(last_projectile_effectframe, current_predframe);
	}
	if ((current_effect_flags & EZCSQC_EFFECT_PROJECTILE) && (flags & WEPPREDANIM_LGBEAM))
	{
		if (WeaponPred_PlayLGBeam())
			last_projectile_effectframe = max(last_projectile_effectframe, current_predframe);
	}

	last_effectframe = min(last_sound_effectframe, last_projectile_effectframe);
}

//============================================================================
// Animation FSM (ports of unezQuake WeaponPred_* helpers)
//============================================================================

void WeaponPred_StartFrame(int windex, int framenum)
{
	if (framenum < 0 || framenum >= wpredict_definitions[windex].anim_number)
	{
		weapon_predicted.client_thinkindex = 0;
		weapon_predicted.client_nextthink = 0;
		return;
	}

	int flags = wpredict_definitions[windex].anims[framenum].flags;
	int mdlframe = wpredict_definitions[windex].anims[framenum].mdlframe;
	int nextanim = wpredict_definitions[windex].anims[framenum].nextanim;
	int altanim = wpredict_definitions[windex].anims[framenum].altanim;

	// State 0 with DEFAULT is the idle/player_run state.
	if (framenum == 0 && (flags & WEPPREDANIM_DEFAULT))
	{
		weapon_predicted.client_thinkindex = 0;
		weapon_predicted.client_nextthink = 0;
		if (mdlframe >= 0)
			weapon_predicted.frame = mdlframe;
		return;
	}

	// Branch attack states re-check +attack when their think executes.
	if (((flags & (WEPPREDANIM_ATTACK | WEPPREDANIM_BRANCH)) == (WEPPREDANIM_ATTACK | WEPPREDANIM_BRANCH)) &&
		(!(input_buttons & 1) || weapon_predicted.impulse))
	{
		if (altanim >= 0 && altanim < wpredict_definitions[windex].anim_number &&
			(wpredict_definitions[windex].anims[altanim].flags & WEPPREDANIM_DEFAULT))
		{
			WeaponPred_StartFrame(windex, altanim);
		}
		else
		{
			// LG has no idle state; stop the loop until a fresh press.
			weapon_predicted.client_thinkindex = 0;
			weapon_predicted.client_nextthink = 0;
		}
		return;
	}

	// Entering a state is when predicted sounds/projectiles fire.
	WeaponPred_PlayEffects(windex, framenum);

	if (mdlframe >= 0)
		weapon_predicted.frame = mdlframe;
	else
	{
		// Negative mdlframe encodes a looping range 1..(-mdlframe).
		weapon_predicted.frame = weapon_predicted.frame + 1;
		if (weapon_predicted.frame > -mdlframe)
			weapon_predicted.frame = 1;
	}

	if (flags & WEPPREDANIM_ATTACK)
	{
		if ((flags & WEPPREDANIM_BRANCH) && wpredict_definitions[windex].itemflag == IT_LIGHTNING)
			// LG schedules the next beam think at 100ms but holds W_Attack for 200ms.
			weapon_predicted.attack_finished = weapon_predicted.client_time + 0.2;
		else
			weapon_predicted.attack_finished = weapon_predicted.client_time + wpredict_definitions[windex].attack_time;
	}

	weapon_predicted.client_thinkindex = nextanim;
	if (wpredict_definitions[windex].anims[framenum].length)
		weapon_predicted.client_nextthink = weapon_predicted.client_time + wpredict_definitions[windex].anims[framenum].length;
	else
		weapon_predicted.client_nextthink = 0;
}

void WeaponPred_WAttack()
{
	int windex = weapon_predicted.weapon_index;
	if (windex < 0 || windex >= MAX_PREDWEPS)
		return;

	// Do not begin a new predicted attack until the weapon is ready and held.
	if (weapon_predicted.client_time < weapon_predicted.attack_finished || !(input_buttons & 1))
		return;

	int anim_number = wpredict_definitions[windex].anim_number;
	int i;
	for (i = 0; i < anim_number; i++)
	{
		int flags = wpredict_definitions[windex].anims[i].flags;
		if (!(flags & WEPPREDANIM_DEFAULT))
			continue;
		if (!(flags & WEPPREDANIM_ATTACK))
			break;

		if (wpredict_definitions[windex].itemflag == IT_AXE)
		{
			// KTX picks between axe frame groups deterministically from client_time.
			int ct1 = (int)(weapon_predicted.client_time * 931.75);
			int ct2 = (int)(weapon_predicted.client_time);
			int raw = (ct1 << 11) + (ct2 >> 6);
			float r = fabs((float)(raw % 1000)) / 1000;
			if ((r >= 0.25 && r < 0.5) || r >= 0.75)
				WeaponPred_StartFrame(windex, 5);
			else
				WeaponPred_StartFrame(windex, 1);
		}
		else
		{
			WeaponPred_StartFrame(windex, wpredict_definitions[windex].anims[i].nextanim);
		}
		weapon_predicted.attack_finished = weapon_predicted.client_time + wpredict_definitions[windex].attack_time;
		break;
	}

	// Weapons without a default state (LG) start directly from state 0.
	if (i == anim_number && anim_number > 0 && (wpredict_definitions[windex].anims[0].flags & WEPPREDANIM_ATTACK))
		WeaponPred_StartFrame(windex, 0);
}

void WeaponPred_Logic()
{
	int windex = weapon_predicted.weapon_index;
	int frame_to_go = weapon_predicted.client_thinkindex;

	if (windex < 0 || windex >= MAX_PREDWEPS)
		return;
	if (!weapon_predicted.client_nextthink || frame_to_go < 0 ||
		frame_to_go >= wpredict_definitions[windex].anim_number ||
		weapon_predicted.client_time < weapon_predicted.client_nextthink)
		return;

	// Evaluate the scheduled state at its think time, then restore replay time.
	float time_held = weapon_predicted.client_time;
	weapon_predicted.client_time = weapon_predicted.client_nextthink;
	weapon_predicted.client_nextthink = 0;
	WeaponPred_StartFrame(windex, frame_to_go);
	weapon_predicted.client_time = time_held;
}

float WeaponPred_CanUseWeapon(int windex)
{
	int items = getstatbits(STAT_ITEMS, 0, 23);
	if (!(items & wpredict_definitions[windex].itemflag))
		return FALSE;

	switch (wpredict_definitions[windex].impulse)
	{
	case 2: return weapon_predicted.ammo_shells >= 1;
	case 3: return weapon_predicted.ammo_shells >= 2;
	case 4: return weapon_predicted.ammo_nails >= 1;
	case 5: return weapon_predicted.ammo_nails >= 2;
	case 6:
	case 7: return weapon_predicted.ammo_rockets >= 1;
	case 8: return weapon_predicted.ammo_cells >= 1;
	}
	return TRUE;
}

// Switch results.
#define WEPSWITCH_UNRESOLVED 0
#define WEPSWITCH_BLOCKED    1
#define WEPSWITCH_APPLIED    2

float WeaponPred_SwitchWeapon(int impulse)
{
	// Weapon switches obey the same attack_finished gate as the server.
	if (weapon_predicted.client_time < weapon_predicted.attack_finished)
		return WEPSWITCH_BLOCKED;

	for (int i = 0; i < MAX_PREDWEPS; i++)
	{
		if (wpredict_definitions[i].impulse != impulse)
			continue;

		if (!WeaponPred_CanUseWeapon(i))
			return WEPSWITCH_UNRESOLVED;

		if (weapon_predicted.weapon_index == i)
			return WEPSWITCH_APPLIED;

		// Switching away from LG retires its persistent local beam + sound gate.
		int cur = weapon_predicted.weapon_index;
		if (cur >= 0 && cur < MAX_PREDWEPS &&
			wpredict_definitions[cur].itemflag == IT_LIGHTNING &&
			wpredict_definitions[i].itemflag != IT_LIGHTNING)
		{
			WeaponPred_ClearBeam();
			lg_twidth = 0;
		}

		weapon_predicted.weapon_index = i;
		weapon_predicted.weapon = wpredict_definitions[i].itemflag;
		weapon_predicted.client_thinkindex = 0;
		weapon_predicted.client_nextthink = 0;
		weapon_predicted.frame = wpredict_definitions[i].anims[0].mdlframe;
		return WEPSWITCH_APPLIED;
	}

	return WEPSWITCH_UNRESOLVED;
}

void WeaponPred_Simulate()
{
	// KTX can force prediction off while still advancing time.
	if (weapon_predicted.client_predflags == PRDFL_FORCEOFF)
	{
		weapon_predicted.client_time += input_timelength;
		weapon_predicted.impulse = 0;
		weapon_predicted.attack_finished = max(weapon_predicted.attack_finished, weapon_predicted.client_time + 0.05);
		return;
	}

	// Dead / non-playable / tracking states must not synthesize local attacks.
	if (getstatf(STAT_HEALTH) <= 0 || player_localentnum != player_localnum + 1)
	{
		weapon_predicted.client_time += input_timelength;
		weapon_predicted.impulse = 0;
		weapon_predicted.attack_finished = weapon_predicted.client_time + 0.05;
		return;
	}

	weapon_predicted.client_time += input_timelength;
	if (input_impulse)
		weapon_predicted.impulse = input_impulse;

	// KTX discards attacks/switches for 50ms after respawn.
	if (weapon_predicted.client_time < respawn_attack_guard_until)
		return;

	WeaponPred_Logic();

	float had_impulse = FALSE;
	float switch_result = WEPSWITCH_APPLIED;
	if (weapon_predicted.impulse)
	{
		had_impulse = TRUE;
		switch_result = WeaponPred_SwitchWeapon(weapon_predicted.impulse);
		// Blocked switches stay queued until attack_finished expires.
		if (switch_result != WEPSWITCH_BLOCKED)
			weapon_predicted.impulse = 0;
	}

	if (had_impulse)
	{
		// A timing-blocked switch remains queued and must not fire the old weapon.
		if (switch_result == WEPSWITCH_BLOCKED)
			return;
		// Unavailable impulses are consumed but the current weapon still fires.
	}

	WeaponPred_WAttack();
}

//============================================================================
// Replay driver (predraw for the viewmodel entity)
//============================================================================

float WeaponPred_Think()
{
	// Freeze the viewmodel animation while paused.
	if (serverkeyfloat(SERVERKEY_PAUSESTATE) == 1)
	{
		WeaponPred_SetModel();
		return PREDRAW_AUTOADD;
	}

	// Seed from the last confirmed server baseline and replay pending commands.
	weapon_predicted = weapon_server;
	weapon_presented = weapon_predicted;

	int effect_threshold = bound(servercommandframe + 1,
		clientcommandframe - autocvar_cl_predict_buffer,
		clientcommandframe);

	int pframe;
	for (pframe = servercommandframe + 1; pframe <= clientcommandframe; pframe++)
	{
		if (!getinputstate(pframe))
			break;
		if (input_timelength <= 0)
			break;

		makevectors(input_angles);

		if (chat_info)
			input_buttons = 0;

		current_predframe = pframe;
		is_effectframe = FALSE;
		current_effect_flags = 0;
		if (pframe <= effect_threshold && pframe > last_sound_effectframe)
			current_effect_flags |= EZCSQC_EFFECT_SOUND;
		if (pframe <= effect_threshold && pframe > last_projectile_effectframe)
			current_effect_flags |= EZCSQC_EFFECT_PROJECTILE;
		if (current_effect_flags)
			is_effectframe = TRUE;

		WeaponPred_Simulate();

		if (pframe <= effect_threshold)
			weapon_presented = weapon_predicted;
	}

	WeaponPred_SetModel();
	return PREDRAW_AUTOADD;
}

//============================================================================
// EZCSQC_WEAPONINFO parsing
//============================================================================

void EntUpdate_WeaponPred(float is_new)
{
	float sendflags = readbyte();
	float was_viewweapon = (viewweapon == self);

	if (sendflags & WEAPONINFO_INDEX)
	{
		weapon_server.impulse = readbyte();
		weapon_server.weapon_index = readbyte();
	}
	if (sendflags & WEAPONINFO_AMMO_SHELLS)
		weapon_server.ammo_shells = readbyte();
	if (sendflags & WEAPONINFO_AMMO_NAILS)
		weapon_server.ammo_nails = readbyte();
	if (sendflags & WEAPONINFO_AMMO_ROCKETS)
		weapon_server.ammo_rockets = readbyte();
	if (sendflags & WEAPONINFO_AMMO_CELLS)
		weapon_server.ammo_cells = readbyte();

	if (sendflags & WEAPONINFO_ATTACK)
	{
		weapon_server.attack_finished = readfloat();
		weapon_server.client_nextthink = readfloat();
		weapon_server.client_thinkindex = readbyte();
	}
	if (sendflags & WEAPONINFO_TIMING)
	{
		weapon_server.client_time = readfloat();
		weapon_server.frame = readbyte();
	}
	if (sendflags & WEAPONINFO_PRED_PING)
	{
		weapon_server.client_predflags = readbyte();
		weapon_server.client_ping = readbyte() / 1000;
	}

	// KTX reuses axe think indices 2-4 for both viewmodel frame groups.
	int wi = weapon_server.weapon_index;
	if (wi >= 0 && wi < MAX_PREDWEPS && wpredict_definitions[wi].itemflag == IT_AXE &&
		weapon_server.frame >= 5 &&
		weapon_server.client_thinkindex >= 2 && weapon_server.client_thinkindex <= 4)
	{
		weapon_server.client_thinkindex += 4;
	}

	// KTX sends a full weapon baseline on respawn before its 50ms attack guard.
	if (was_viewweapon && sendflags == WEAPONINFO_ALL)
		respawn_attack_guard_until = weapon_server.client_time + 0.05;

	self.drawmask = MASK_PRED_VIEWMODEL;
	self.predraw = WeaponPred_Think;
	self.renderflags = RF_VIEWMODEL;
	viewweapon = self;
}

//============================================================================
// Init
//============================================================================

void WeaponPred_Init()
{
	reg_sound_count = 0;
	last_sound_effectframe = 0;
	last_projectile_effectframe = 0;
	last_effectframe = 0;
	lg_twidth = 0;
	respawn_attack_guard_until = 0;
}
