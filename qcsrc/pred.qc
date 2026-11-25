/*
 * Whether we are predicting at all, and the per-frame state the other modules
 * ask about: where the player was, whether they are in play, and the gravity
 * their grenades fall under.
 */

float Pred_Enabled(void)
{
	if (autocvar_cl_nopred_weapon)
		return FALSE;

	// A demo has no input frames to replay and no local player to predict for.
	if (isdemo())
		return FALSE;

	if (intermission)
		return FALSE;

	return TRUE;
}

/*
 * Resolve one of the tri-state cvars, cl_predict_weaponsound and
 * cl_predict_projectiles: 0 off, 2 on regardless, and 1 to follow the server.
 *
 * Following the server is the honest default. Prediction only agrees with what
 * the server does with a shot when the server rewinds to meet the client, so
 * without antilag a predicted shot would land somewhere the server disagrees
 * with.
 */
static float Pred_Allowed(float setting)
{
	if (!setting)
		return FALSE;

	if (setting >= 2)
		return TRUE;

	return serverkey("sv_antilag") == "1";
}

float Pred_SoundEnabled(void)
{
	return Pred_Allowed(autocvar_cl_predict_weaponsound);
}

float Pred_ProjectilesEnabled(void)
{
	return Pred_Allowed(autocvar_cl_predict_projectiles);
}

/*
 * Whether the server is letting us predict this weapon state at all.
 *
 * Instagib's coilgun replaces the shotgun's model, sound and refire delay, but
 * the flag does not say which instagib mode is running and the delays differ
 * between them. Guessing would desync the animation and play the wrong sound
 * alongside the server's, so we stand aside and let the engine draw the gun.
 */
float Pred_ServerAllows(float predflags)
{
	if (predflags == PRDFL_FORCEOFF)
		return FALSE;

	if (predflags & PRDFL_COILGUN)
		return FALSE;

	return TRUE;
}

/*
 * The server republishes its gravity as a stat, because the movevars the client
 * was sent at connect are not reachable from here. A server too old to publish
 * it leaves the stat at zero, which is not a gravity any server runs, so zero
 * doubles as "not published" and we keep the stock value.
 */
void Pred_UpdateGravity(void)
{
	float g = getstatf(STAT_KTX_GRAVITY);

	sv_gravity = g ? g : DEFAULT_GRAVITY;
}

// ==========================================================================
// Predicted origin history
//
// A projectile has to leave from where the player was on the frame that fired
// it, which may be a few frames back. The engine only ever exposes its
// prediction for the present, so we keep a short history of it.
// ==========================================================================

static vector	origin_hist[PRED_BACKUP];
static float	origin_hist_frame[PRED_BACKUP];

void Pred_RecordOrigin(void)
{
	float slot = clientcommandframe & PRED_MASK;

	origin_hist[slot] = pmove_org;
	origin_hist_frame[slot] = clientcommandframe;
}

vector Pred_OriginForFrame(float f)
{
	float slot = f & PRED_MASK;

	// At low frame rates some input frames never got sampled; the present
	// position is the closest thing we have.
	if (origin_hist_frame[slot] != f)
		return pmove_org;

	return origin_hist[slot];
}

// ==========================================================================
// Player status
// ==========================================================================

float player_playable;	// alive, in play, not spectating

// The item bits the player is carrying. STAT_ITEMS packs items2 above these,
// so the low 23 are the ones that name a weapon.
float Player_Items(void)
{
	return getstatbits(STAT_ITEMS, 0, 23);
}

/*
 * Refresh player_playable. Returns true on the frame the player comes back into
 * play, by respawning or by stopping spectating, which is the moment everything
 * predicted for the previous life has to be thrown away.
 */
float Pred_UpdatePlayer(void)
{
	float was_playable = player_playable;

	player_playable = TRUE;

	if (getstatf(STAT_HEALTH) <= 0)
		player_playable = FALSE;
	if (intermission)
		player_playable = FALSE;
	if (getplayerkeyfloat(player_localnum, "*spectator"))
		player_playable = FALSE;

	return player_playable && !was_playable;
}
