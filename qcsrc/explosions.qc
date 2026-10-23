/*
 * explosions.qc - predicted rocket explosions (visual + sound) and server-echo
 * suppression. The self-damage velocity kick is recorded here and applied by
 * the player-prediction replay in playerpred.qc (Phase 8).
 *
 * Only the local player's own rockets hitting world/brush geometry are
 * predicted; rockets hitting players/entities remain server-authoritative.
 */

vector  expl_org[EXPL_RING];
float   expl_time[EXPL_RING];
// Phase 8 self-kick bookkeeping.
vector  expl_impact[EXPL_RING];
float   expl_statetime[EXPL_RING];
float   expl_kick_valid[EXPL_RING];
int     expl_next;

void Explosion_Record(vector org)
{
	expl_org[expl_next] = org;
	expl_time[expl_next] = time;
	expl_next = (expl_next + 1) % EXPL_RING;
}

// Suppress the server's TE_EXPLOSION if it matches a recent local prediction.
float Explosion_MatchServer(vector org)
{
	if (!autocvar_cl_predict_explosions)
		return FALSE;

	for (int i = 0; i < EXPL_RING; i++)
	{
		if (expl_time[i] <= 0)
			continue;
		if (time - expl_time[i] > 0.5)
			continue;
		if (vlen(org - expl_org[i]) <= 96)
		{
			expl_time[i] = 0;   // consume
			return TRUE;
		}
	}
	return FALSE;
}

// Record a predicted explosion for the self-kick replay (Phase 8).
void Explosion_RecordKick(vector impact, float state_time)
{
	int slot;
	// Reuse the most recent echo-record slot so the two stay paired.
	slot = (expl_next - 1 + EXPL_RING) % EXPL_RING;
	expl_impact[slot] = impact;
	expl_statetime[slot] = state_time;
	expl_kick_valid[slot] = TRUE;
}

void Explosion_Predict(vector vis_origin, vector impact_origin, float state_time)
{
	te_explosion(vis_origin);       // visual + sound
	Explosion_Record(vis_origin);   // for server-echo suppression
	if (state_time > 0)
		Explosion_RecordKick(impact_origin, state_time);
}

float RocketImpactStateTime(entity proj, vector impact, vector velocity, float speed)
{
	// The kick must replay at impact time, not the current render frame.
	if (proj.prediction_start_state_time <= 0 || speed <= 1)
		return 0;

	vector dir = velocity * (1 / speed);
	vector diff = impact - proj.start;
	float travel = max(0, diff * dir);
	return proj.prediction_start_state_time + travel / speed;
}

float Projectile_OwnerIsMe(entity proj)
{
	return proj.local_projectile || (proj.ownnum == player_localnum + 1);
}

// Point-blank rocket at spawn: sweep the classic newmis step (+ render
// lookahead) and, on a BSP hit, explode locally instead of spawning a model.
float Predict_RocketSpawnTouch(entity proj, vector origin, vector velocity)
{
	if (!autocvar_cl_predict_explosions)
		return FALSE;
	if (!(proj.modelflags & MF_ROCKET))
		return FALSE;

	float speed = vlen(velocity);
	if (speed <= 1)
		return FALSE;

	vector predicted_origin;

	// MOVE_NOMONSTERS -> world + brush models only (never players); this is the
	// "predictable BSP" set, so any hit here is safe to predict.
	vector end = origin + velocity * 0.05;
	traceline(origin, end, MOVE_NOMONSTERS, proj);
	if (trace_fraction < 1)
	{
		predicted_origin = trace_endpos - velocity * (8 / speed);
		Explosion_Predict(predicted_origin, trace_endpos, weapon_predicted.client_time);
		return TRUE;
	}

	vector la_end = end + velocity * 0.02;
	traceline(end, la_end, MOVE_NOMONSTERS, proj);
	if (trace_fraction < 1)
	{
		predicted_origin = trace_endpos - velocity * (8 / speed);
		Explosion_Predict(predicted_origin, trace_endpos, weapon_predicted.client_time);
		return TRUE;
	}

	return FALSE;
}

// In-flight owner rocket world impact prediction (called from the projectile
// predraw). Returns TRUE when the projectile has hit and should be retired.
float Predict_OwnerRocketWorldImpact(entity proj, vector start, vector end)
{
	if (!autocvar_cl_predict_explosions || proj.predicted_explosion)
		return FALSE;
	if (!(proj.modelflags & MF_ROCKET) || !Projectile_OwnerIsMe(proj))
		return FALSE;

	vector velocity = proj.local_projectile ? proj.vel : proj.s_velocity;
	float speed = vlen(velocity);
	if (speed <= 1)
		return FALSE;

	traceline(start, end, MOVE_NOMONSTERS, proj);
	if (trace_fraction == 1)
		return FALSE;

	vector predicted_origin = trace_endpos - velocity * (8 / speed);
	float ist = RocketImpactStateTime(proj, trace_endpos, velocity, speed);
	if (ist <= 0)
		return TRUE;    // impacted, but no valid kick time; retire without effect

	// Visual/audio at KTX's backed-off point; kick uses the true impact.
	Explosion_Predict(predicted_origin, trace_endpos, ist);
	proj.predicted_explosion = TRUE;
	return TRUE;
}
