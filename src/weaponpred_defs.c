#include "g_local.h"

weppreddef_t wpredict_definitions[16];

static qbool WeaponDefinition_Exists(int weapon_index)
{
	weppreddef_t *wep;

	if (weapon_index < 0 || weapon_index >= (int)(sizeof(wpredict_definitions) / sizeof(wpredict_definitions[0])))
	{
		return false;
	}

	wep = &wpredict_definitions[weapon_index];
	return wep->modelindex || wep->anim_number;
}

static qbool WeaponDefinition_WriteTo(int to, int weapon_index, int sendflags)
{
	weppreddef_t *wep;
	int i, k;

	if (weapon_index < 0 || weapon_index >= (int)(sizeof(wpredict_definitions) / sizeof(wpredict_definitions[0])))
	{
		return false;
	}

	sendflags &= WEAPONDEF_INIT | WEAPONDEF_FLAGS | WEAPONDEF_ANIM;

	WriteByte(to, EZCSQC_WEAPONDEF);
	WriteByte(to, sendflags);

	wep = &wpredict_definitions[weapon_index];
	WriteByte(to, weapon_index);

	if (sendflags & WEAPONDEF_INIT)
	{
		WriteShort(to, wep->attack_time);
		WriteShort(to, wep->modelindex);
	}

	if (sendflags & WEAPONDEF_FLAGS)
	{
		int bitmask = 0;

		WriteByte(to, wep->impulse);

		for (i = 0; i < 24; i++)
		{
			bitmask = 1 << i;
			if (bitmask & wep->itemflag)
			{
				bitmask = -1;
				WriteByte(to, i);
				break;
			}
		}
		if (bitmask != -1)
		{
			WriteByte(to, 255);
		}
	}

	if (sendflags & WEAPONDEF_ANIM)
	{
		WriteByte(to, wep->anim_number & 255);
		for (i = 0; i < wep->anim_number; i++)
		{
			weppredanim_t *anim = &wep->anim_states[i];

			WriteByte(to, anim->mdlframe + 127);
			WriteByte(to, anim->flags);
			if (anim->flags & WEPPREDANIM_MOREBYTES)
			{
				WriteByte(to, anim->flags >> 8);
			}
			if (anim->flags & WEPPREDANIM_SOUND)
			{
				WriteShort(to, anim->sound);
				WriteShort(to, anim->soundmask);
			}
			if (WEPPREDANIM_HAS(anim->flags, WEPPREDANIM_SOUND2))
			{
				WriteShort(to, anim->sound2);
				WriteShort(to, anim->soundmask2);
			}
			if (anim->flags & WEPPREDANIM_PROJECTILE)
			{
				WriteShort(to, anim->projectile_model);
				for (k = 0; k < 3; k++)
				{
					WriteShort(to, anim->projectile_velocity[k]);
				}
				for (k = 0; k < 3; k++)
				{
					WriteByte(to, anim->projectile_offset[k] & 255);
				}
			}
			WriteByte(to, anim->nextanim);
			if (anim->flags & WEPPREDANIM_BRANCH)
			{
				WriteByte(to, anim->altanim);
			}
			WriteByte(to, anim->length / 10);
		}
	}

	return true;
}

void WPredict_SendDefinitionsTo(gedict_t *player)
{
	int i;
	int count = 0;
	int old_msg_entity;

	if (!player)
	{
		return;
	}

	/*
	 * Native EZCSQC clients only. This writes svc 77, which FTE reads as
	 * svcfte_precache and would silently desync on; clients running our csprogs
	 * have these definitions compiled in and must never be sent them.
	 */
	if (!iKey(player, "ezcsqc"))
	{
		return;
	}

	for (i = 0; i < (int)(sizeof(wpredict_definitions) / sizeof(wpredict_definitions[0])); i++)
	{
		if (WeaponDefinition_Exists(i))
		{
			count++;
		}
	}

	old_msg_entity = g_globalvars.msg_entity;
	g_globalvars.msg_entity = EDICT_TO_PROG(player);
	WriteByte(MSG_ONE_NORECORD, SVC_EZCSQC_SETUP);
	WriteByte(MSG_ONE_NORECORD, 1);
	WriteByte(MSG_ONE_NORECORD, count);
	for (i = 0; i < (int)(sizeof(wpredict_definitions) / sizeof(wpredict_definitions[0])); i++)
	{
		if (WeaponDefinition_Exists(i))
		{
			WeaponDefinition_WriteTo(MSG_ONE_NORECORD, i, SENDFLAGS_ALL);
		}
	}
	g_globalvars.msg_entity = old_msg_entity;
}

void WPredict_Initialize(void)
{
	weppreddef_t *axe, *sg, *ssg, *ng, *sng, *gl, *rl, *lg;
	weppredanim_t *player_shot0, *player_shot1, *player_shot2;
	weppredanim_t *player_shot3, *player_shot4, *player_shot5, *player_shot6;

	memset(wpredict_definitions, 0, sizeof(wpredict_definitions));

	axe = &wpredict_definitions[1];
	sg = &wpredict_definitions[2];
	ssg = &wpredict_definitions[3];
	ng = &wpredict_definitions[4];
	sng = &wpredict_definitions[5];
	gl = &wpredict_definitions[6];
	rl = &wpredict_definitions[7];
	lg = &wpredict_definitions[8];

	// AXE
	player_shot0 = &axe->anim_states[0];
	player_shot1 = &axe->anim_states[1];
	player_shot2 = &axe->anim_states[2];
	player_shot3 = &axe->anim_states[3];
	player_shot4 = &axe->anim_states[4];
	player_shot5 = &axe->anim_states[5];
	player_shot6 = &axe->anim_states[6];
	axe->modelindex = trap_precache_model("progs/v_axe.mdl");
	axe->attack_time = 500;
	axe->impulse = 1;
	axe->itemflag = IT_AXE;
	axe->anim_number = 9;
	player_shot0->flags = WEPPREDANIM_DEFAULT | WEPPREDANIM_ATTACK;
	player_shot0->mdlframe = 0;
	player_shot0->nextanim = 1;
	player_shot1->flags = WEPPREDANIM_SOUND;
	player_shot1->sound = trap_precache_sound("weapons/ax1.wav");
	player_shot1->soundmask = 0x02;
	player_shot1->mdlframe = 1;
	player_shot1->nextanim = 2;
	player_shot1->length = 100;
	player_shot2->mdlframe = 2;
	player_shot2->nextanim = 3;
	player_shot2->length = 100;
	player_shot3->mdlframe = 3;
	player_shot3->nextanim = 4;
	player_shot3->length = 100;
	player_shot4->mdlframe = 4;
	player_shot4->nextanim = 0;
	player_shot4->length = 100;
	player_shot5->flags = WEPPREDANIM_SOUND;
	player_shot5->sound = trap_precache_sound("weapons/ax1.wav");
	player_shot5->soundmask = 0x02;
	player_shot5->mdlframe = 5;
	player_shot5->nextanim = 6;
	player_shot5->length = 100;
	player_shot6->mdlframe = 6;
	player_shot6->nextanim = 7;
	player_shot6->length = 100;
	axe->anim_states[7].mdlframe = 7;
	axe->anim_states[7].nextanim = 8;
	axe->anim_states[7].length = 100;
	axe->anim_states[8].mdlframe = 8;
	axe->anim_states[8].nextanim = 0;
	axe->anim_states[8].length = 100;

	// SHOTGUN
	player_shot0 = &sg->anim_states[0];
	player_shot1 = &sg->anim_states[1];
	player_shot2 = &sg->anim_states[2];
	player_shot3 = &sg->anim_states[3];
	player_shot4 = &sg->anim_states[4];
	player_shot5 = &sg->anim_states[5];
	player_shot6 = &sg->anim_states[6];
	sg->modelindex = trap_precache_model("progs/v_shot.mdl");
	sg->attack_time = 500;
	sg->impulse = 2;
	sg->itemflag = IT_SHOTGUN;
	sg->anim_number = 7;
	player_shot0->flags = WEPPREDANIM_DEFAULT | WEPPREDANIM_ATTACK;
	player_shot0->mdlframe = 0;
	player_shot0->nextanim = 1;
	player_shot1->flags = WEPPREDANIM_MUZZLEFLASH | WEPPREDANIM_SOUND;
	player_shot1->sound = trap_precache_sound("weapons/guncock.wav");
	player_shot1->soundmask = 0x04;
	player_shot1->mdlframe = 1;
	player_shot1->nextanim = 2;
	player_shot1->length = 100;
	player_shot2->mdlframe = 2;
	player_shot2->nextanim = 3;
	player_shot2->length = 100;
	player_shot3->mdlframe = 3;
	player_shot3->nextanim = 4;
	player_shot3->length = 100;
	player_shot4->mdlframe = 4;
	player_shot4->nextanim = 5;
	player_shot4->length = 100;
	player_shot5->mdlframe = 5;
	player_shot5->nextanim = 6;
	player_shot5->length = 100;
	player_shot6->mdlframe = 6;
	player_shot6->nextanim = 0;
	player_shot6->length = 100;

	// SUPER SHOTGUN
	player_shot0 = &ssg->anim_states[0];
	player_shot1 = &ssg->anim_states[1];
	player_shot2 = &ssg->anim_states[2];
	player_shot3 = &ssg->anim_states[3];
	player_shot4 = &ssg->anim_states[4];
	player_shot5 = &ssg->anim_states[5];
	player_shot6 = &ssg->anim_states[6];
	ssg->modelindex = trap_precache_model("progs/v_shot2.mdl");
	ssg->attack_time = k_yawnmode ? 800 : 700;
	ssg->impulse = 3;
	ssg->itemflag = IT_SUPER_SHOTGUN;
	ssg->anim_number = 7;
	player_shot0->flags = WEPPREDANIM_DEFAULT | WEPPREDANIM_ATTACK;
	player_shot0->mdlframe = 0;
	player_shot0->nextanim = 1;
	player_shot1->flags = WEPPREDANIM_MUZZLEFLASH | WEPPREDANIM_SOUND;
	player_shot1->sound = trap_precache_sound("weapons/shotgn2.wav");
	player_shot1->soundmask = 0x08;
	player_shot1->mdlframe = 1;
	player_shot1->nextanim = 2;
	player_shot1->length = 100;
	player_shot2->mdlframe = 2;
	player_shot2->nextanim = 3;
	player_shot2->length = 100;
	player_shot3->mdlframe = 3;
	player_shot3->nextanim = 4;
	player_shot3->length = 100;
	player_shot4->mdlframe = 4;
	player_shot4->nextanim = 5;
	player_shot4->length = 100;
	player_shot5->mdlframe = 5;
	player_shot5->nextanim = 6;
	player_shot5->length = 100;
	player_shot6->mdlframe = 6;
	player_shot6->nextanim = 0;
	player_shot6->length = 100;

	// NAILGUN
	player_shot0 = &ng->anim_states[0];
	player_shot1 = &ng->anim_states[1];
	player_shot2 = &ng->anim_states[2];
	ng->modelindex = trap_precache_model("progs/v_nail.mdl");
	ng->attack_time = 200;
	ng->impulse = 4;
	ng->itemflag = IT_NAILGUN;
	ng->anim_number = 3;
	player_shot0->flags = WEPPREDANIM_DEFAULT | WEPPREDANIM_ATTACK;
	player_shot0->mdlframe = 0;
	player_shot0->nextanim = 1;
	player_shot1->flags = WEPPREDANIM_MUZZLEFLASH | WEPPREDANIM_SOUND | WEPPREDANIM_PROJECTILE | WEPPREDANIM_ATTACK | WEPPREDANIM_BRANCH;
	player_shot1->mdlframe = -8;
	player_shot1->length = 100;
	player_shot1->nextanim = 2;
	player_shot1->altanim = 0;
	player_shot1->sound = trap_precache_sound("weapons/rocket1i.wav");
	player_shot1->soundmask = 0x10;
	player_shot1->projectile_model = trap_precache_model("progs/spike.mdl");
	player_shot1->projectile_velocity[1] = 1000;
	player_shot1->projectile_offset[0] = 4;
	player_shot1->projectile_offset[2] = 16;
	player_shot2->flags = WEPPREDANIM_MUZZLEFLASH | WEPPREDANIM_SOUND | WEPPREDANIM_PROJECTILE | WEPPREDANIM_ATTACK | WEPPREDANIM_BRANCH;
	player_shot2->mdlframe = -8;
	player_shot2->length = 100;
	player_shot2->nextanim = 1;
	player_shot2->altanim = 0;
	player_shot2->sound = trap_precache_sound("weapons/rocket1i.wav");
	player_shot2->soundmask = 0x10;
	player_shot2->projectile_model = trap_precache_model("progs/spike.mdl");
	player_shot2->projectile_velocity[1] = 1000;
	player_shot2->projectile_offset[0] = -4;
	player_shot2->projectile_offset[2] = 16;

	// SUPER NAILGUN
	player_shot0 = &sng->anim_states[0];
	player_shot1 = &sng->anim_states[1];
	player_shot2 = &sng->anim_states[2];
	sng->modelindex = trap_precache_model("progs/v_nail2.mdl");
	sng->attack_time = 200;
	sng->impulse = 5;
	sng->itemflag = IT_SUPER_NAILGUN;
	sng->anim_number = 3;
	player_shot0->flags = WEPPREDANIM_DEFAULT | WEPPREDANIM_ATTACK;
	player_shot0->mdlframe = 0;
	player_shot0->nextanim = 1;
	player_shot1->flags = WEPPREDANIM_MUZZLEFLASH | WEPPREDANIM_SOUND | WEPPREDANIM_PROJECTILE | WEPPREDANIM_ATTACK | WEPPREDANIM_BRANCH;
	player_shot1->mdlframe = -8;
	player_shot1->length = 100;
	player_shot1->nextanim = 2;
	player_shot1->altanim = 0;
	player_shot1->sound = trap_precache_sound("weapons/spike2.wav");
	player_shot1->soundmask = 0x20;
	player_shot1->projectile_model = trap_precache_model("progs/s_spike.mdl");
	player_shot1->projectile_velocity[1] = 1000;
	player_shot1->projectile_offset[2] = 16;
	*player_shot2 = *player_shot1;
	player_shot2->nextanim = 1;

	// GRENADE LAUNCHER
	player_shot0 = &gl->anim_states[0];
	player_shot1 = &gl->anim_states[1];
	player_shot2 = &gl->anim_states[2];
	player_shot3 = &gl->anim_states[3];
	player_shot4 = &gl->anim_states[4];
	player_shot5 = &gl->anim_states[5];
	player_shot6 = &gl->anim_states[6];
	gl->modelindex = trap_precache_model("progs/v_rock.mdl");
	gl->attack_time = 600;
	gl->impulse = 6;
	gl->itemflag = IT_GRENADE_LAUNCHER;
	gl->anim_number = 7;
	player_shot0->flags = WEPPREDANIM_DEFAULT | WEPPREDANIM_ATTACK;
	player_shot0->mdlframe = 0;
	player_shot0->nextanim = 1;
	player_shot1->flags = WEPPREDANIM_MUZZLEFLASH | WEPPREDANIM_SOUND | WEPPREDANIM_PROJECTILE;
	player_shot1->mdlframe = 1;
	player_shot1->nextanim = 2;
	player_shot1->length = 100;
	player_shot1->sound = trap_precache_sound("weapons/grenade.wav");
	player_shot1->soundmask = 0x40;
	player_shot1->projectile_model = trap_precache_model("progs/grenade.mdl");
	player_shot1->projectile_velocity[1] = 600;
	player_shot1->projectile_velocity[2] = 200;
	player_shot2->mdlframe = 2;
	player_shot2->nextanim = 3;
	player_shot2->length = 100;
	player_shot3->mdlframe = 3;
	player_shot3->nextanim = 4;
	player_shot3->length = 100;
	player_shot4->mdlframe = 4;
	player_shot4->nextanim = 5;
	player_shot4->length = 100;
	player_shot5->mdlframe = 5;
	player_shot5->nextanim = 6;
	player_shot5->length = 100;
	player_shot6->mdlframe = 6;
	player_shot6->nextanim = 0;
	player_shot6->length = 100;

	// ROCKET LAUNCHER
	player_shot0 = &rl->anim_states[0];
	player_shot1 = &rl->anim_states[1];
	player_shot2 = &rl->anim_states[2];
	player_shot3 = &rl->anim_states[3];
	player_shot4 = &rl->anim_states[4];
	player_shot5 = &rl->anim_states[5];
	player_shot6 = &rl->anim_states[6];
	rl->modelindex = trap_precache_model("progs/v_rock2.mdl");
	rl->attack_time = 800;
	rl->impulse = 7;
	rl->itemflag = IT_ROCKET_LAUNCHER;
	rl->anim_number = 7;
	player_shot0->flags = WEPPREDANIM_DEFAULT | WEPPREDANIM_ATTACK;
	player_shot0->mdlframe = 0;
	player_shot0->nextanim = 1;
	player_shot1->flags = WEPPREDANIM_MUZZLEFLASH | WEPPREDANIM_SOUND | WEPPREDANIM_PROJECTILE;
	player_shot1->mdlframe = 1;
	player_shot1->nextanim = 2;
	player_shot1->length = 100;
	player_shot1->sound = trap_precache_sound("weapons/sgun1.wav");
	player_shot1->soundmask = 0x80;
	player_shot1->projectile_model = trap_precache_model("progs/missile.mdl");
	player_shot1->projectile_velocity[1] = 1000;
	player_shot1->projectile_offset[1] = 8;
	player_shot1->projectile_offset[2] = 16;
	player_shot2->mdlframe = 2;
	player_shot2->nextanim = 3;
	player_shot2->length = 100;
	player_shot3->mdlframe = 3;
	player_shot3->nextanim = 4;
	player_shot3->length = 100;
	player_shot4->mdlframe = 4;
	player_shot4->nextanim = 5;
	player_shot4->length = 100;
	player_shot5->mdlframe = 5;
	player_shot5->nextanim = 6;
	player_shot5->length = 100;
	player_shot6->mdlframe = 6;
	player_shot6->nextanim = 0;
	player_shot6->length = 100;

	// LIGHTNING GUN
	player_shot0 = &lg->anim_states[0];
	player_shot1 = &lg->anim_states[1];
	player_shot2 = &lg->anim_states[2];
	lg->modelindex = trap_precache_model("progs/v_light.mdl");
	lg->attack_time = 100;
	lg->impulse = 8;
	lg->itemflag = IT_LIGHTNING;
	lg->anim_number = 3;
	player_shot0->flags = WEPPREDANIM_ATTACK | WEPPREDANIM_SOUND | WEPPREDANIM_SOUND2 | WEPPREDANIM_SOUNDAUTO | WEPPREDANIM_LGBEAM;
	player_shot0->mdlframe = 0;
	player_shot0->nextanim = 1;
	player_shot0->length = 100;
	player_shot0->sound = trap_precache_sound("weapons/lstart.wav");
	player_shot0->soundmask = 0x0100;
	player_shot0->sound2 = trap_precache_sound("weapons/lhit.wav");
	player_shot0->soundmask2 = 0x0100;
	player_shot1->flags = WEPPREDANIM_MUZZLEFLASH | WEPPREDANIM_SOUND | WEPPREDANIM_LGBEAM | WEPPREDANIM_LTIME | WEPPREDANIM_ATTACK | WEPPREDANIM_BRANCH;
	player_shot1->mdlframe = -4;
	player_shot1->length = 100;
	player_shot1->nextanim = 2;
	player_shot1->altanim = 0;
	player_shot1->sound = trap_precache_sound("weapons/lhit.wav");
	player_shot1->soundmask = 0x0100;
	player_shot2->flags = WEPPREDANIM_MUZZLEFLASH | WEPPREDANIM_SOUND | WEPPREDANIM_LGBEAM | WEPPREDANIM_LTIME | WEPPREDANIM_ATTACK | WEPPREDANIM_BRANCH;
	player_shot2->mdlframe = -4;
	player_shot2->length = 100;
	player_shot2->nextanim = 1;
	player_shot2->altanim = 0;
	player_shot2->sound = trap_precache_sound("weapons/lhit.wav");
	player_shot2->soundmask = 0x0100;
}
