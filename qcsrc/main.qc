/*
 * Entry points: what the engine calls, and the order things happen in a frame.
 */

// ==========================================================================
// Entry points
// ==========================================================================

void CSQC_Init(float apilevel, string enginename, float engineversion)
{
	// All motion here is integrated per frame; the engine's own entity physics
	// runs on a coarse tick that would visibly stutter a close-up rocket.
	physics_mode = 0;

	Weapon_PrecacheAll();
	Sound_Reset();
	WeaponState_Reset();
	Beam_Clear();

	viewweapon = spawn();
	viewweapon.renderflags = RF_VIEWMODEL;
	// It doubles as the entity the predicted bolt traces with; see Beam_Draw.
	viewweapon.dimension_hit = TRACE_WORLD_ONLY;

	sv_gravity = DEFAULT_GRAVITY;
}

void CSQC_Shutdown(void)
{
	// The progs is torn down wholesale on map change; nothing to release.
}

void CSQC_Ent_Update(float isnew)
{
	float kind = readbyte();

	switch (kind)
	{
	case EZCSQC_WEAPONINFO:
		WeaponState_Parse(isnew);
		break;

	case EZCSQC_PROJECTILE:
		Projectile_Parse(isnew);
		break;

	default:
		// Anything else means our idea of the stream has diverged from the
		// server's, and every following byte is garbage.
		error(sprintf("csqc: unknown entity type %g\n", kind));
	}
}

void CSQC_Ent_Remove(void)
{
	if (autocvar_cl_predict_debug)
		print(sprintf("csqc: server removed #%g\n", self.entnum));

	remove(self);
}

/*
 * Peek at temp entities to notice the server's lightning beam. Everything is
 * passed straight back to the engine: returning false rewinds the read, so the
 * engine parses and draws it exactly as it normally would.
 */
float CSQC_Parse_TempEntity(void)
{
	float kind = readbyte();

	if (kind == TE_LIGHTNING2)
	{
		// The server's beam for us has caught up with the predicted one.
		if (readshort() == player_localentnum)
			Beam_ServerArrived();

		return FALSE;
	}

	return FALSE;
}

void CSQC_UpdateView(float vwidth, float vheight, float notmenu)
{
	clearscene();

	setproperty(VF_DRAWENGINESBAR, TRUE);
	setproperty(VF_DRAWCROSSHAIR, TRUE);

	Pred_UpdateGravity();
	Pred_RecordOrigin();

	// Coming back into play: every prediction we hold belongs to the life that
	// just ended.
	if (Pred_UpdatePlayer())
		WeaponState_Discard();

	/*
	 * Replay first. addentities() runs the projectile predraws and turns beams
	 * into render entities as it goes, so anything spawned or drawn after it
	 * would not appear until the next frame.
	 */
	WeaponState_Replay();
	Beam_Draw();

	addentities(MASK_ENGINE);

	// Fall back to the engine's view model whenever we decline to draw one, so
	// the player is never left without a gun.
	if (!WeaponState_Present())
		addentities(MASK_VIEWMODEL);

	renderscene();
}

/*
 * The engine asks before playing any server sound. Returning true drops it,
 * which is how the server's echo of a sound we already predicted is suppressed.
 */
float CSQC_Event_Sound(float entnum, float channel, string soundname,
                       float vol, float attenuation, vector pos,
                       float pitchmod, float flags)
{
	if (!Pred_Enabled() || !Pred_SoundEnabled())
		return FALSE;

	// Nothing was predicted for this state, so the server's sound is the only one.
	if (!Pred_ServerAllows(ws_server.predflags))
		return FALSE;

	// Only ever silence our own weapon. While spectating, player_localentnum is
	// the tracked player, whose sounds we are not predicting.
	if (!player_playable || entnum != player_localentnum)
		return FALSE;

	return Sound_Consume(soundname, channel);
}

// Declared __used by the engine definitions, so it must exist.
void CSQC_Input_Frame(void)
{
}
