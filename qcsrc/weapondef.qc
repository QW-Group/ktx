/*
 * weapondef.qc - parse and store EZCSQC_WEAPONDEF entities.
 *
 * The server ships one definition per weapon describing the viewmodel, the
 * animation FSM, firing sounds and projectile spawn parameters. Earlier code
 * parsed and discarded these; the data-driven FSM in weaponpred.qc now consumes
 * them.
 */

float WeaponPred_DefinitionReady(int windex)
{
	if (windex < 0 || windex >= MAX_PREDWEPS)
		return FALSE;

	// A slot is ready once it has both a viewmodel and at least one anim state.
	// (Unlike the unezQuake reference we accept index 0, because this server
	// assigns the axe to slot 0; the empty-slot test below still guards it.)
	if (!wpredict_definitions[windex].modelindex || !wpredict_definitions[windex].anim_number)
		return FALSE;

	return TRUE;
}

// Decode a wire byte written as signed char on the server.
float WeaponDef_ReadSignedByte()
{
	float b = readbyte();
	if (b > 127)
		b -= 256;
	return b;
}

void EntUpdate_WeaponDef(float is_new)
{
	int sendflags = readbyte();
	int windex = readbyte();

	// Guard against a malformed index before indexing the table.
	int slot = windex;
	if (slot < 0 || slot >= MAX_PREDWEPS)
		slot = 0;

	if (sendflags & WEAPONDEF_INIT)
	{
		wpredict_definitions[slot].attack_time = LENGTH2S(readshort());
		wpredict_definitions[slot].modelindex = readshort();
	}

	if (sendflags & WEAPONDEF_FLAGS)
	{
		wpredict_definitions[slot].impulse = readbyte();
		int bitnum = readbyte();
		if (bitnum == 255)
			wpredict_definitions[slot].itemflag = 0;
		else
			wpredict_definitions[slot].itemflag = (1 << bitnum);
	}

	if (sendflags & WEAPONDEF_ANIM)
	{
		int anim_number = readbyte();
		if (anim_number > WEPPRED_MAXSTATES)
			anim_number = WEPPRED_MAXSTATES;
		wpredict_definitions[slot].anim_number = anim_number;

		for (int i = 0; i < anim_number; i++)
		{
			int mdlframe = readbyte() - 127;
			int anim_flags = readbyte();
			if (anim_flags & WEPPREDANIM_MOREBYTES)
				anim_flags |= readbyte() << 8;

			wpredict_definitions[slot].anims[i].mdlframe = mdlframe;
			wpredict_definitions[slot].anims[i].flags = anim_flags;
			wpredict_definitions[slot].anims[i].sound = 0;
			wpredict_definitions[slot].anims[i].soundmask = 0;
			wpredict_definitions[slot].anims[i].sound_name = "";
			wpredict_definitions[slot].anims[i].sound2 = 0;
			wpredict_definitions[slot].anims[i].soundmask2 = 0;
			wpredict_definitions[slot].anims[i].sound2_name = "";
			wpredict_definitions[slot].anims[i].projectile_model = 0;
			wpredict_definitions[slot].anims[i].projectile_velocity = '0 0 0';
			wpredict_definitions[slot].anims[i].projectile_offset = '0 0 0';

			if (anim_flags & WEPPREDANIM_SOUND)
			{
				int snd = readshort();
				int mask = readshort();
				wpredict_definitions[slot].anims[i].sound = snd;
				wpredict_definitions[slot].anims[i].soundmask = mask;
				if (snd > 0)
				{
					wpredict_definitions[slot].anims[i].sound_name = soundnameforindex(snd);
					int chan = WEPPREDANIM_HAS(anim_flags, WEPPREDANIM_SOUNDAUTO) ? 0 : 1;
					WeaponPred_RegisterSound(snd, mask, chan);
				}
			}

			if (WEPPREDANIM_HAS(anim_flags, WEPPREDANIM_SOUND2))
			{
				int snd2 = readshort();
				int mask2 = readshort();
				wpredict_definitions[slot].anims[i].sound2 = snd2;
				wpredict_definitions[slot].anims[i].soundmask2 = mask2;
				if (snd2 > 0)
				{
					wpredict_definitions[slot].anims[i].sound2_name = soundnameforindex(snd2);
					WeaponPred_RegisterSound(snd2, mask2, 1);
				}
			}

			if (anim_flags & WEPPREDANIM_PROJECTILE)
			{
				wpredict_definitions[slot].anims[i].projectile_model = readshort();
				vector pvel;
				pvel_x = readshort();
				pvel_y = readshort();
				pvel_z = readshort();
				wpredict_definitions[slot].anims[i].projectile_velocity = pvel;
				vector poff;
				poff_x = WeaponDef_ReadSignedByte();
				poff_y = WeaponDef_ReadSignedByte();
				poff_z = WeaponDef_ReadSignedByte();
				wpredict_definitions[slot].anims[i].projectile_offset = poff;
			}

			wpredict_definitions[slot].anims[i].nextanim = readbyte();

			if (anim_flags & WEPPREDANIM_BRANCH)
				wpredict_definitions[slot].anims[i].altanim = readbyte();
			else
				wpredict_definitions[slot].anims[i].altanim = 0;

			wpredict_definitions[slot].anims[i].length = readbyte() * 10;  // ms
			wpredict_definitions[slot].anims[i].length = LENGTH2S(wpredict_definitions[slot].anims[i].length);
		}
	}

	if (autocvar_csqc_ezdbg > 1)
		print("EZCSQC weapondef slot=", ftos(slot), " flags=", ftos(sendflags), "\n");
}
