/*
 * playerpred.qc - CSQC player-prediction takeover + rocket explosion self-kick.
 *
 * FTE predicts the local player in-engine; to predict the rocket-jump velocity
 * kick before the server acknowledges it, we run our own player-physics replay
 * (runstandardplayerphysics) seeded from the authoritative EZCSQC_PLAYER state,
 * inject predicted explosion kicks during the replay, and compose the result
 * onto the engine view as a delta:
 *
 *   VF_ORIGIN += (predicted_origin + smoothing_nudge) - pmove_org
 *
 * When our replay agrees with the engine's prediction the delta is ~0, so the
 * view is unchanged; the kick is the visible difference. Gated by
 * cl_predict_player; disabling it reverts fully to engine prediction.
 *
 * NOTE: enable only after verifying the zero-delta invariant at runtime (with
 * no explosions, |predicted_origin - pmove_org| should stay at sub-unit noise).
 */

entity  pred_player;

vector  pseed_origin;
vector  pseed_velocity;
float   pseed_flags;
float   pseed_freshtime;

vector  pred_log_origin[64];
float   pred_log_pmflags[64];
float   pred_log_frame[64];

vector  simerr_nudge;
float   simerr_lastcheck;
float   simerr_time;

void EntUpdate_Player(float is_new)
{
	int sendflags = readbyte();

	if (sendflags & PLAYERPRED_STATE)
	{
		pseed_origin = [readcoord(), readcoord(), readcoord()];
		pseed_velocity = [readfloat(), readfloat(), readfloat()];
	}
	if (sendflags & PLAYERPRED_FLAGS)
		pseed_flags = readbyte();

	pseed_freshtime = time;
}

// Apply predicted explosion kicks whose impact time falls in [ct_before, ct_after).
void PlayerPred_InjectKicks(float ct_before, float ct_after)
{
	for (int e = 0; e < EXPL_RING; e++)
	{
		if (!expl_kick_valid[e])
			continue;
		if (time - expl_time[e] > 1.0)
			continue;   // stale
		float st = expl_statetime[e];
		if (st < ct_before || st >= ct_after)
			continue;

		// KTX splash damage acts from the bbox centre; ~+4 z for the Quake hull.
		vector center = pred_player.origin + '0 0 4';

		// Occlusion (CanDamage parity, simplified): a clear world trace passes.
		traceline(expl_impact[e], center, MOVE_NOMONSTERS, pred_player);
		if (trace_fraction < 1 && vlen(trace_endpos - center) > 1)
			continue;

		float dist = vlen(center - expl_impact[e]);
		if (dist >= 160)
			continue;

		float kick = (120 - 0.5 * dist) * 0.5;   // self-damage halving
		if (kick <= 0)
			continue;

		vector dir = normalize(pred_player.origin - expl_impact[e]);
		pred_player.velocity = pred_player.velocity + dir * kick * 8;
	}
}

void PlayerPred_RunReplay()
{
	float active = autocvar_cl_predict_player
		&& (time - pseed_freshtime < 0.5)
		&& !(pseed_flags & PLAYERPREDF_NOPREDICT)
		&& !intermission
		&& weapon_server.client_predflags != PRDFL_FORCEOFF
		&& player_localentnum == player_localnum + 1
		&& getstatf(STAT_HEALTH) > 0;

	if (!active)
	{
		simerr_nudge = '0 0 0';
		return;
	}

	if (!pred_player)
	{
		pred_player = spawn();
		pred_player.movetype = MOVETYPE_WALK;
		pred_player.solid = SOLID_NOT;
		pred_player.mins = '-16 -16 -24';
		pred_player.maxs = '16 16 32';
		pred_player.gravity = 1;
	}
	pred_player.entnum = player_localentnum;

	int scf = servercommandframe;
	int ccf = clientcommandframe;

	// (1) Prediction-error bookkeeping on a new server ack.
	if (scf != simerr_lastcheck)
	{
		if (pred_log_frame[scf & 63] == scf)
		{
			vector err = pred_log_origin[scf & 63] - pseed_origin;
			if (vlen(err) < 160)
				simerr_nudge = simerr_nudge + err;
			else
				simerr_nudge = '0 0 0';   // teleport / large correction: snap
			if (vlen(simerr_nudge) > 96)
				simerr_nudge = simerr_nudge * (96 / vlen(simerr_nudge));
		}
		simerr_lastcheck = scf;
	}

	// (2) Seed from the authoritative pre-prediction state.
	pred_player.origin = pseed_origin;
	pred_player.velocity = pseed_velocity;
	// Fully reset to the authoritative onground state each seed so stray physics
	// flags cannot accumulate on the persistent prediction entity.
	pred_player.flags = (pseed_flags & PLAYERPREDF_ONGROUND) ? FL_ONGROUND : 0;
	if (pred_log_frame[scf & 63] == scf)
		pred_player.pmove_flags = pred_log_pmflags[scf & 63];   // self-seed jump_held
	else
		pred_player.pmove_flags = 0;

	// (3) Replay pending commands with kick injection.
	float ct = weapon_server.client_time;   // server-clock ruler for kick timing
	int f;
	int cap = 0;
	for (f = scf + 1; f <= ccf; f++)
	{
		if (!getinputstate(f))
			break;
		if (input_timelength <= 0)
			break;
		if (cap > 62)
			break;
		cap = cap + 1;

		float ct_before = ct;
		ct = ct + input_timelength;

		PlayerPred_InjectKicks(ct_before, ct);
		runstandardplayerphysics(pred_player);

		pred_log_origin[f & 63] = pred_player.origin;
		pred_log_pmflags[f & 63] = pred_player.pmove_flags;
		pred_log_frame[f & 63] = f;
	}

	// (4) Decay the error nudge, clamped away from walls.
	float dt = bound(0, time - simerr_time, 0.1);
	simerr_time = time;
	float hl = bound(0.025, 0.080 * autocvar_cl_predict_smoothview, 0.250);
	simerr_nudge = simerr_nudge * pow(0.5, dt / hl);
	if (vlen(simerr_nudge) < 1)
		simerr_nudge = '0 0 0';

	vector goal = pred_player.origin + simerr_nudge;
	tracebox(pred_player.origin, pred_player.mins, pred_player.maxs, goal, FALSE, pred_player);
	vector view_base = trace_endpos;

	// (5) Compose onto the engine's decorated view as a delta.
	vector cur = getproperty(VF_ORIGIN);
	setproperty(VF_ORIGIN, cur + (view_base - pmove_org));
}
