/*
 * The predicted lightning beam.
 *
 * The engine keys a beam to whoever owns it, and replaces one only with another
 * of the same owner. A beam created here can never claim the player as its
 * owner, so the server's bolt does not supersede ours: both are drawn, and ours
 * stays where it was last put. That shapes everything below. We re-issue the
 * bolt every frame so it tracks the player, stand down once the server starts
 * sending its own, and retract ours when we do.
 */

static float	beam_until;		// keep re-issuing our beam until this client time
static float	beam_server_until;	// the server is feeding us beams until this client time
static float	beam_issued;		// we have a bolt live that needs retracting

/*
 * Stop predicting a beam. beam_issued is deliberately left alone: a bolt that
 * is already in the air still has to be retracted, which the next Beam_Draw
 * does on its way out.
 */
void Beam_Clear(void)
{
	beam_until = 0;
	beam_server_until = 0;
}

/*
 * True while the server's own beam is arriving. Our beam and the server's are
 * keyed to different entities, so drawing both would double the bolt; but the
 * server only sends one while the player is actually firing, so this has to
 * lapse or the next burst would never be predicted.
 */
static float Beam_ServerLive(void)
{
	return cltime < beam_server_until;
}

// Called when a lightning state fires. Only arms the beam; it is drawn every
// rendered frame so it tracks the view rather than lurching once per shot.
void Beam_Predict(void)
{
	if (!autocvar_cl_predict_beam)
		return;

	if (Beam_ServerLive())
		return;

	beam_until = cltime + BEAM_HOLD;
}

/*
 * Retract our bolt.
 *
 * A beam cannot be deleted from here, and ours is not owned by the player, so
 * the server's bolt never replaces it: left alone it hangs in the air for a
 * fifth of a second wherever the player was standing, which is glaring while
 * strafing. Re-issuing it with no length replaces it with one the engine draws
 * no segments for, which comes to the same thing.
 */
static void Beam_Retract(void)
{
	if (!beam_issued)
		return;

	te_lightning2(viewweapon, pmove_org, pmove_org);
	beam_issued = FALSE;
}

void Beam_Draw(void)
{
	vector start, end;
	float contents;

	if (cltime >= beam_until || Beam_ServerLive()
		|| !Pred_Enabled() || !player_playable)
	{
		Beam_Retract();
		return;
	}

	/*
	 * Start at the player's origin, not at the eye and not at the chest height
	 * the server writes into its own message.
	 *
	 * For a beam it owns, the engine ignores the start the server sent and
	 * re-anchors it here every frame. Ours is never owned by the player, so it
	 * is drawn from exactly the point we give it, and we have to place it where
	 * the engine would: the origin, raised by the same clamped view height the
	 * engine uses. Anything nearer the camera puts the first segment of the
	 * bolt on the lens, which is what fills the screen with white.
	 */
	start = pmove_org;
	start_z += bound(-7, cvar("v_viewheight"), 4);

	// A discharge underwater is not a beam; the server handles that case and we
	// would only draw a bolt that never existed.
	contents = pointcontents(start);
	if (contents == CONTENT_WATER || contents == CONTENT_SLIME || contents == CONTENT_LAVA)
	{
		Beam_Retract();
		return;
	}

	makevectors(view_angles);
	end = start + v_forward * BEAM_RANGE;

	/*
	 * Trace against the world alone. The server ignores the firing player when
	 * it traces its own bolt; we cannot name that player as an entity to skip,
	 * but viewweapon carries the same world-only restriction the projectiles
	 * use. Without it the trace stops on the player standing at the muzzle and
	 * the bolt is a stub drawn across the camera.
	 */
	traceline(start, end, MOVE_NOMONSTERS, viewweapon);

	te_lightning2(viewweapon, start, trace_endpos);
	beam_issued = TRUE;
}

// The server's beam for us has arrived: stand down until it stops coming.
void Beam_ServerArrived(void)
{
	beam_server_until = cltime + BEAM_SERVER_HOLD;
	beam_until = 0;
}
