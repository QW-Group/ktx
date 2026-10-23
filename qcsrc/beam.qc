/*
 * beam.qc - predicted LG beam + server-echo suppression.
 *
 * On each LGBEAM effect frame the local player's lightning beam is drawn
 * immediately from a client trace; the server's echoed TE_LIGHTNING2 for the
 * local player is then suppressed in CSQC_Parse_TempEntity.
 */

entity  lg_beam_ent;    // persistent beam key (attached to the local player)

float WeaponPred_PlayLGBeam()
{
	if (!autocvar_cl_predict_beam)
		return TRUE;

	if (!lg_beam_ent)
		lg_beam_ent = spawn();
	lg_beam_ent.entnum = player_localentnum;

	// makevectors(input_angles) already ran for this replayed frame.
	vector bstart = pmove_org;
	bstart_z += 16;
	vector bend = bstart + v_forward * 600;
	traceline(bstart, bend, MOVE_NOMONSTERS, world);

	te_lightning2(lg_beam_ent, bstart, trace_endpos);
	return TRUE;
}

void WeaponPred_ClearBeam()
{
	// Draw a degenerate beam so the predicted one clears immediately on switch.
	if (lg_beam_ent && autocvar_cl_predict_beam)
		te_lightning2(lg_beam_ent, pmove_org, pmove_org);
}

// Should the server's TE_LIGHTNING2 for entity `ent` be suppressed because we
// predicted the local player's beam locally?
float LGBeam_ShouldSuppress(float ent)
{
	if (!autocvar_cl_predict_beam || !viewweapon)
		return FALSE;
	if (ent != player_localentnum)
		return FALSE;
	if (getstatf(STAT_HEALTH) <= 0 || player_localentnum != player_localnum + 1)
		return FALSE;
	if (weapon_server.client_predflags == PRDFL_FORCEOFF)
		return FALSE;
	return TRUE;
}
