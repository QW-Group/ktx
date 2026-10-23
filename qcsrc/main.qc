/*
 * main.qc - CSQC entry points and network dispatch for KTX client prediction.
 */

void(float apilevel, string enginename, float engineversion) CSQC_Init =
{
	WeaponPred_Init();

	precache_model("progs/missile.mdl");
	precache_model("progs/spike.mdl");
	precache_model("progs/s_spike.mdl");
	precache_model("progs/grenade.mdl");

	registercvar("cl_predict_weaponsound", "1");
	registercvar("cl_sproj_xerp", "0");
	registercvar("cl_predict_projectiles", "1");
	registercvar("cl_predict_buffer", "1");
	registercvar("cl_predict_beam", "1");
	registercvar("cl_predict_explosions", "1");
	// Player-physics takeover (rocket self-kick) ships OFF pending the runtime
	// zero-delta verification described in playerpred.qc; enable once confirmed.
	registercvar("cl_predict_player", "0");
	registercvar("cl_predict_smoothview", "1");
	registercvar("cl_nopred_weapon", "0");
	registercvar("csqc_gravity", "0");
	registercvar("csqc_ezdbg", "0");

	print("KTX CSQC Initialized!\n");
};

void(float is_new) CSQC_Ent_Update =
{
	int ent_type = readbyte();

	switch (ent_type)
	{
	case EZCSQC_PROJECTILE:  EntUpdate_Projectile(is_new); break;
	case EZCSQC_WEAPONINFO:  EntUpdate_WeaponPred(is_new); break;
	case EZCSQC_WEAPONDEF:   EntUpdate_WeaponDef(is_new); break;
	case EZCSQC_PLAYER:      EntUpdate_Player(is_new); break;
	}
};

void() CSQC_Ent_Remove =
{
	if (self == viewweapon)
		viewweapon = __NULL__;
	EntRemove_Projectile();
	remove(self);
};

float(float entnum, float channel, string soundname, float vol, float attenuation, vector pos, float pitchmod, float flags) CSQC_Event_Sound =
{
	// Only ever suppress the local player's own predicted weapon sounds.
	if (entnum != player_localentnum)
		return FALSE;
	if (!viewweapon || autocvar_cl_nopred_weapon || !autocvar_cl_predict_weaponsound)
		return FALSE;
	// Not while dead or spectating/tracking another player.
	if (getstatf(STAT_HEALTH) <= 0 || player_localentnum != player_localnum + 1)
		return FALSE;
	// Not while the server forced prediction off.
	if (weapon_server.client_predflags == PRDFL_FORCEOFF)
		return FALSE;

	return WeaponPred_SuppressSound(channel, soundname);
};

float() CSQC_Parse_TempEntity =
{
	// Inspect a few temp entities to suppress echoes of locally-predicted
	// effects. Returning FALSE makes the engine rewind and parse normally.
	float te = readbyte();

	if (te == TE_LIGHTNING2)
	{
		float ent = readshort();
		vector bs = [readcoord(), readcoord(), readcoord()];
		vector be = [readcoord(), readcoord(), readcoord()];
		if (LGBeam_ShouldSuppress(ent))
			return TRUE;
		return FALSE;
	}

	if (te == TE_EXPLOSION)
	{
		vector org = [readcoord(), readcoord(), readcoord()];
		if (Explosion_MatchServer(org))
			return TRUE;
		return FALSE;
	}

	return FALSE;
};

void() CSQC_Input_Frame =
{
	// Player-input prediction is driven from CSQC_UpdateView; nothing to do here.
};

void(float vwidth, float vheight, float notmenu) CSQC_UpdateView =
{
	ping_time = getplayerkeyfloat(player_localnum, INFOKEY_P_PING) / 1000;
	chat_info = stof(getlocaluserinfo(0, "chat"));

	weaponpred_drawn = FALSE;

	clearscene();

	// Phase 8: CSQC player-prediction takeover (stub for now).
	PlayerPred_RunReplay();

	setproperty(VF_DRAWENGINESBAR, TRUE);
	setproperty(VF_DRAWCROSSHAIR, TRUE);

	// Running the predicted-viewmodel pass sets weaponpred_drawn.
	addentities(MASK_ENGINE | (intermission ? 0 : MASK_PRED_VIEWMODEL));

	// Fall back to the engine viewmodel for weapons without a definition
	// (e.g. the axe on servers that do not ship an axe weapondef).
	if (!weaponpred_drawn && !intermission)
		addentities(MASK_VIEWMODEL);

	renderscene();
};
