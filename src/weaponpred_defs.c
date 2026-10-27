//========================================================================
//
//  Copyright (C)  2020 - 2021	Samuel Piper
//
//  This code is free software; you can redistribute it and/or modify
//  it under the terms of the GNU GPL (General Public License); either
//  version 2 of the License, or (at your option) any later version.
//
//========================================================================
// CSQC weapon-prediction definitions: the per-weapon animation/sound/projectile
// tables streamed to clients, split out of antilag.c.
#include "g_local.h"
#include "fb_globals.h"

weppreddef_t wpredict_definitions[16];

qbool WeaponDefinition_SendEntity(int sendflags)
{
	weppreddef_t *wep;
	int i, k;

	WriteByte(MSG_CSQC, EZCSQC_WEAPONDEF);

	WriteByte(MSG_CSQC, sendflags);
	wep = &wpredict_definitions[(int)self->s.v.weapon];
	WriteByte(MSG_CSQC, (int)self->s.v.weapon);

	if (sendflags & WEAPONDEF_INIT)
	{
		WriteShort(MSG_CSQC, wep->attack_time);
		WriteShort(MSG_CSQC, wep->modelindex);
	}

	if (sendflags & WEAPONDEF_FLAGS)
	{
		int bitmask = 0;

		WriteByte(MSG_CSQC, wep->impulse);

		for (i = 0; i < 24; i++) // find bit number, there's probably a better way to do this
		{
			bitmask = 1 << i;
			if (bitmask & wep->itemflag)
			{
				bitmask = -1;
				WriteByte(MSG_CSQC, i);
				break;
			}
		}
		if (bitmask != -1)
		{
			WriteByte(MSG_CSQC, 255);
		}
	}

	if (sendflags & WEAPONDEF_ANIM)
	{
		WriteByte(MSG_CSQC, wep->anim_number = wep->anim_number & 255);
		for (i = 0; i < wep->anim_number; i++)
		{
			weppredanim_t *anim = &wep->anim_states[i];

			WriteByte(MSG_CSQC, anim->mdlframe + 127);
			WriteByte(MSG_CSQC, anim->flags);
			if (anim->flags & WEPPREDANIM_MOREBYTES)
			{
				WriteByte(MSG_CSQC, anim->flags >> 8);
			}
			if (anim->flags & WEPPREDANIM_SOUND)
			{
				WriteShort(MSG_CSQC, anim->sound);
				WriteShort(MSG_CSQC, anim->soundmask);
			}
			if (WEPPREDANIM_HAS(anim->flags, WEPPREDANIM_SOUND2))
			{
				WriteShort(MSG_CSQC, anim->sound2);
				WriteShort(MSG_CSQC, anim->soundmask2);
			}
			if (anim->flags & WEPPREDANIM_PROJECTILE)
			{
				WriteShort(MSG_CSQC, anim->projectile_model);
				for (k = 0; k < 3; k++)
				{
					WriteShort(MSG_CSQC, anim->projectile_velocity[k]);
				}
				for (k = 0; k < 3; k++)
				{
					WriteByte(MSG_CSQC, anim->projectile_offset[k]);
				}
			}
			WriteByte(MSG_CSQC, anim->nextanim);
			if (anim->flags & WEPPREDANIM_BRANCH)
			{
				WriteByte(MSG_CSQC, anim->altanim);
			}
			WriteByte(MSG_CSQC, anim->length / 10);
		}
	}

	return true;
}

void WPredict_Initialize(void)
{
	weppreddef_t *axe = &wpredict_definitions[0];
	weppreddef_t *sg = &wpredict_definitions[1];
	weppreddef_t *ssg = &wpredict_definitions[2];
	weppreddef_t *ng = &wpredict_definitions[3];
	weppreddef_t *sng = &wpredict_definitions[4];
	weppreddef_t *gl = &wpredict_definitions[5];
	weppreddef_t *rl = &wpredict_definitions[6];
	weppreddef_t *lg = &wpredict_definitions[7];
	weppreddef_t *coilgun = &wpredict_definitions[8];
	weppreddef_t *hook = &wpredict_definitions[9];

	weppredanim_t *player_shot0;
	weppredanim_t *player_shot1;
	weppredanim_t *player_shot2;
	weppredanim_t *player_shot3;
	weppredanim_t *player_shot4;
	weppredanim_t *player_shot5;
	weppredanim_t *player_shot6;


	gedict_t *wepdef;
	// SHOTGUN
	wepdef = spawn();
	ExtFieldSetPvsFlags(wepdef, 3);
	ExtFieldSetSendEntity(wepdef, (func_t)WeaponDefinition_SendEntity);
	wepdef->s.v.weapon = sg - wpredict_definitions;
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
	// idle anim
	player_shot0->flags = WEPPREDANIM_DEFAULT | WEPPREDANIM_ATTACK;
	player_shot0->mdlframe = 0;
	player_shot0->nextanim = 1;
	// shot 1
	player_shot1->flags = WEPPREDANIM_MUZZLEFLASH | WEPPREDANIM_SOUND;
	player_shot1->sound = trap_precache_sound("weapons/guncock.wav");
	player_shot1->soundmask = 0x04;
	player_shot1->mdlframe = 1;
	player_shot1->nextanim = 2;
	player_shot1->length = 100;
	// shot 2
	player_shot2->mdlframe = 2;
	player_shot2->nextanim = 3;
	player_shot2->length = 100;
	// shot 3
	player_shot3->mdlframe = 3;
	player_shot3->nextanim = 4;
	player_shot3->length = 100;
	// shot 4
	player_shot4->mdlframe = 4;
	player_shot4->nextanim = 5;
	player_shot4->length = 100;
	// shot 5
	player_shot5->mdlframe = 5;
	player_shot5->nextanim = 6;
	player_shot5->length = 100;
	// shot 6
	player_shot6->mdlframe = 6;
	player_shot6->nextanim = 0;
	player_shot6->length = 100;
	// END OF SHOTGUN

	// SUPER SHOTGUN
	wepdef = spawn();
	ExtFieldSetPvsFlags(wepdef, 3);
	ExtFieldSetSendEntity(wepdef, (func_t)WeaponDefinition_SendEntity);
	wepdef->s.v.weapon = ssg - wpredict_definitions;
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
	// idle anim
	player_shot0->flags = WEPPREDANIM_DEFAULT | WEPPREDANIM_ATTACK;
	player_shot0->mdlframe = 0;
	player_shot0->nextanim = 1;
	// shot 1
	player_shot1->flags = WEPPREDANIM_MUZZLEFLASH | WEPPREDANIM_SOUND;
	player_shot1->sound = trap_precache_sound("weapons/shotgn2.wav");
	player_shot1->soundmask = 0x08;
	player_shot1->mdlframe = 1;
	player_shot1->nextanim = 2;
	player_shot1->length = 100;
	// shot 2
	player_shot2->mdlframe = 2;
	player_shot2->nextanim = 3;
	player_shot2->length = 100;
	// shot 3
	player_shot3->mdlframe = 3;
	player_shot3->nextanim = 4;
	player_shot3->length = 100;
	// shot 4
	player_shot4->mdlframe = 4;
	player_shot4->nextanim = 5;
	player_shot4->length = 100;
	// shot 5
	player_shot5->mdlframe = 5;
	player_shot5->nextanim = 6;
	player_shot5->length = 100;
	// shot 6
	player_shot6->mdlframe = 6;
	player_shot6->nextanim = 0;
	player_shot6->length = 100;
	// END OF SUPER SHOTGUN

	// NAILGUN
	wepdef = spawn();
	ExtFieldSetPvsFlags(wepdef, 3);
	ExtFieldSetSendEntity(wepdef, (func_t)WeaponDefinition_SendEntity);
	wepdef->s.v.weapon = ng - wpredict_definitions;
	player_shot0 = &ng->anim_states[0];
	player_shot1 = &ng->anim_states[1];
	player_shot2 = &ng->anim_states[2];
	ng->modelindex = trap_precache_model("progs/v_nail.mdl");
	ng->attack_time = 200;
	ng->impulse = 4;
	ng->itemflag = IT_NAILGUN;
	ng->anim_number = 3;
	// idle anim
	player_shot0->flags = WEPPREDANIM_DEFAULT | WEPPREDANIM_ATTACK;
	player_shot0->mdlframe = 0;
	player_shot0->nextanim = 1;
	// fire1 anim
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
	// fire2 anim
	player_shot2->flags = WEPPREDANIM_MUZZLEFLASH | WEPPREDANIM_SOUND | WEPPREDANIM_PROJECTILE | WEPPREDANIM_ATTACK | WEPPREDANIM_BRANCH;
	player_shot2->mdlframe = -8;
	player_shot2->length = 100;
	player_shot2->nextanim = 1;
	player_shot2->altanim = 0;
	player_shot2->sound = trap_precache_sound("weapons/rocket1i.wav");
	player_shot2->soundmask = 0x10;
	player_shot2->projectile_model = trap_precache_model("progs/spike.mdl");
	player_shot2->projectile_velocity[1] = 1000;
	player_shot2->projectile_offset[0] = 4; // TODO: was -4
	player_shot2->projectile_offset[2] = 16;
	// END OF NAILGUN

	// SUPER NAILGUN
	wepdef = spawn();
	ExtFieldSetPvsFlags(wepdef, 3);
	ExtFieldSetSendEntity(wepdef, (func_t)WeaponDefinition_SendEntity);
	wepdef->s.v.weapon = sng - wpredict_definitions;
	player_shot0 = &sng->anim_states[0];
	player_shot1 = &sng->anim_states[1];
	player_shot2 = &sng->anim_states[2];
	sng->modelindex = trap_precache_model("progs/v_nail2.mdl");
	sng->attack_time = 200;
	sng->impulse = 5;
	sng->itemflag = IT_SUPER_NAILGUN;
	sng->anim_number = 3;
	// idle anim
	player_shot0->flags = WEPPREDANIM_DEFAULT | WEPPREDANIM_ATTACK;
	player_shot0->mdlframe = 0;
	player_shot0->nextanim = 1;
	// fire1 anim
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
	// fire2 anim
	*player_shot2 = *player_shot1;
	player_shot2->nextanim = 1;
	// END OF SUPER NAILGUN

	// GRENADE LAUNCHER
	wepdef = spawn();
	ExtFieldSetPvsFlags(wepdef, 3);
	ExtFieldSetSendEntity(wepdef, (func_t)WeaponDefinition_SendEntity);
	wepdef->s.v.weapon = gl - wpredict_definitions;
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
	// idle anim
	player_shot0->flags = WEPPREDANIM_DEFAULT | WEPPREDANIM_ATTACK;
	player_shot0->mdlframe = 0;
	player_shot0->nextanim = 1;
	// shot 1
	player_shot1->flags = WEPPREDANIM_MUZZLEFLASH | WEPPREDANIM_SOUND | WEPPREDANIM_PROJECTILE;
	player_shot1->mdlframe = 1;
	player_shot1->nextanim = 2;
	player_shot1->length = 100;
	player_shot1->sound = trap_precache_sound("weapons/grenade.wav");
	player_shot1->soundmask = 0x40;
	player_shot1->projectile_model = trap_precache_model("progs/grenade.mdl");
	player_shot1->projectile_velocity[1] = 600;
	player_shot1->projectile_velocity[2] = 200;
	player_shot1->projectile_offset[2] = 16;
	// shot 2
	player_shot2->mdlframe = 2;
	player_shot2->nextanim = 3;
	player_shot2->length = 100;
	// shot 3
	player_shot3->mdlframe = 3;
	player_shot3->nextanim = 4;
	player_shot3->length = 100;
	// shot 4
	player_shot4->mdlframe = 4;
	player_shot4->nextanim = 5;
	player_shot4->length = 100;
	// shot 5
	player_shot5->mdlframe = 5;
	player_shot5->nextanim = 6;
	player_shot5->length = 100;
	// shot 6
	player_shot6->mdlframe = 6;
	player_shot6->nextanim = 0;
	player_shot6->length = 100;
	// END OF GRENADE LAUNCHER

	// ROCKET LAUNCHER
	wepdef = spawn();
	ExtFieldSetPvsFlags(wepdef, 3);
	ExtFieldSetSendEntity(wepdef, (func_t)WeaponDefinition_SendEntity);
	wepdef->s.v.weapon = rl - wpredict_definitions;
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
	// idle anim
	player_shot0->flags = WEPPREDANIM_DEFAULT | WEPPREDANIM_ATTACK;
	player_shot0->mdlframe = 0;
	player_shot0->nextanim = 1;
	// shot 1
	player_shot1->flags = WEPPREDANIM_MUZZLEFLASH | WEPPREDANIM_SOUND | WEPPREDANIM_PROJECTILE;
	player_shot1->mdlframe = 1;
	player_shot1->nextanim = 2;
	player_shot1->length = 100;
	player_shot1->sound = trap_precache_sound("weapons/sgun1.wav");
	player_shot1->soundmask = 0x80;
	player_shot1->projectile_model = trap_precache_model("progs/missile.mdl");
	player_shot1->projectile_velocity[1] = 1000;
	player_shot1->projectile_offset[2] = 16;
	// shot 2
	player_shot2->mdlframe = 2;
	player_shot2->nextanim = 3;
	player_shot2->length = 100;
	// shot 3
	player_shot3->mdlframe = 3;
	player_shot3->nextanim = 4;
	player_shot3->length = 100;
	// shot 4
	player_shot4->mdlframe = 4;
	player_shot4->nextanim = 5;
	player_shot4->length = 100;
	// shot 5
	player_shot5->mdlframe = 5;
	player_shot5->nextanim = 6;
	player_shot5->length = 100;
	// shot 6
	player_shot6->mdlframe = 6;
	player_shot6->nextanim = 0;
	player_shot6->length = 100;
	// END OF ROCKET LAUNCHER

	// LIGHTNING GUN
	wepdef = spawn();
	ExtFieldSetPvsFlags(wepdef, 3);
	ExtFieldSetSendEntity(wepdef, (func_t)WeaponDefinition_SendEntity);
	wepdef->s.v.weapon = lg - wpredict_definitions;
	player_shot0 = &lg->anim_states[0];
	player_shot1 = &lg->anim_states[1];
	player_shot2 = &lg->anim_states[2];
	lg->modelindex = trap_precache_model("progs/v_light.mdl");
	lg->attack_time = 100;
	lg->impulse = 8;
	lg->itemflag = IT_LIGHTNING;
	lg->anim_number = 3;
	// startup anim: play lstart once and bootstrap the lhit loop cadence via the
	// second sound channel (CSQC gates lhit on lg_twidth so it is not doubled by
	// the LTIME fire anims below).
	player_shot0->flags = WEPPREDANIM_ATTACK | WEPPREDANIM_SOUND | WEPPREDANIM_SOUND2 | WEPPREDANIM_SOUNDAUTO | WEPPREDANIM_LGBEAM;
	player_shot0->mdlframe = 0;
	player_shot0->nextanim = 1;
	player_shot0->length = 100;
	player_shot0->sound = trap_precache_sound("weapons/lstart.wav");
	player_shot0->soundmask = 0x0100;
	player_shot0->sound2 = trap_precache_sound("weapons/lhit.wav");
	player_shot0->soundmask2 = 0x0100;
	// fire1 anim
	player_shot1->flags = WEPPREDANIM_MUZZLEFLASH | WEPPREDANIM_SOUND | WEPPREDANIM_LGBEAM | WEPPREDANIM_LTIME | WEPPREDANIM_ATTACK | WEPPREDANIM_BRANCH;
	player_shot1->mdlframe = -4;
	player_shot1->length = 100;
	player_shot1->nextanim = 2;
	player_shot1->altanim = 0;
	player_shot1->sound = trap_precache_sound("weapons/lhit.wav");
	player_shot1->soundmask = 0x0100;
	// fire2 anim
	player_shot2->flags = WEPPREDANIM_MUZZLEFLASH | WEPPREDANIM_SOUND | WEPPREDANIM_LGBEAM | WEPPREDANIM_LTIME | WEPPREDANIM_ATTACK | WEPPREDANIM_BRANCH;
	player_shot2->mdlframe = -4;
	player_shot2->length = 100;
	player_shot2->nextanim = 1;
	player_shot2->altanim = 0;
	player_shot2->sound = trap_precache_sound("weapons/lhit.wav");
	player_shot2->soundmask = 0x0100;
	// END OF LIGHTNING GUN

	// AXE
	wepdef = spawn();
	ExtFieldSetPvsFlags(wepdef, 3);
	ExtFieldSetSendEntity(wepdef, (func_t)WeaponDefinition_SendEntity);
	wepdef->s.v.weapon = axe - wpredict_definitions;
	axe->modelindex = trap_precache_model("progs/v_axe.mdl");
	axe->attack_time = 500;
	axe->impulse = 1;
	axe->itemflag = IT_AXE;
	axe->anim_number = 9;
	// idle anim
	axe->anim_states[0].flags = WEPPREDANIM_DEFAULT | WEPPREDANIM_ATTACK;
	axe->anim_states[0].mdlframe = 0;
	axe->anim_states[0].nextanim = 1;
	// swing group A (frames 1-4)
	axe->anim_states[1].flags = WEPPREDANIM_MUZZLEFLASH | WEPPREDANIM_SOUND;
	axe->anim_states[1].sound = trap_precache_sound("weapons/ax1.wav");
	axe->anim_states[1].soundmask = 0x02;
	axe->anim_states[1].mdlframe = 1;
	axe->anim_states[1].nextanim = 2;
	axe->anim_states[1].length = 100;
	axe->anim_states[2].mdlframe = 2;
	axe->anim_states[2].nextanim = 3;
	axe->anim_states[2].length = 100;
	axe->anim_states[3].mdlframe = 3;
	axe->anim_states[3].nextanim = 4;
	axe->anim_states[3].length = 100;
	axe->anim_states[4].mdlframe = 4;
	axe->anim_states[4].nextanim = 0;
	axe->anim_states[4].length = 100;
	// swing group B (frames 5-8)
	axe->anim_states[5].flags = WEPPREDANIM_MUZZLEFLASH | WEPPREDANIM_SOUND;
	axe->anim_states[5].sound = trap_precache_sound("weapons/ax1.wav");
	axe->anim_states[5].soundmask = 0x02;
	axe->anim_states[5].mdlframe = 5;
	axe->anim_states[5].nextanim = 6;
	axe->anim_states[5].length = 100;
	axe->anim_states[6].mdlframe = 6;
	axe->anim_states[6].nextanim = 7;
	axe->anim_states[6].length = 100;
	axe->anim_states[7].mdlframe = 7;
	axe->anim_states[7].nextanim = 8;
	axe->anim_states[7].length = 100;
	axe->anim_states[8].mdlframe = 8;
	axe->anim_states[8].nextanim = 0;
	axe->anim_states[8].length = 100;
	// END OF AXE

	// COILGUN (instagib shotgun variant); base timing baked from the mode.
	if (cvar("k_instagib") && cvar("k_instagib_custom_models"))
	{
		int i;
		wepdef = spawn();
		ExtFieldSetPvsFlags(wepdef, 3);
		ExtFieldSetSendEntity(wepdef, (func_t)WeaponDefinition_SendEntity);
		wepdef->s.v.weapon = coilgun - wpredict_definitions;
		coilgun->modelindex = trap_precache_model("progs/v_coil.mdl");
		coilgun->attack_time = (cvar("k_instagib") == 1) ? 1200 : ((cvar("k_instagib") == 2) ? 700 : 500);
		coilgun->impulse = 2;
		coilgun->itemflag = IT_SHOTGUN;
		coilgun->anim_number = 7;
		coilgun->anim_states[0].flags = WEPPREDANIM_DEFAULT | WEPPREDANIM_ATTACK;
		coilgun->anim_states[0].mdlframe = 0;
		coilgun->anim_states[0].nextanim = 1;
		// Hitscan: no predicted projectile; sound stays server-authoritative to
		// avoid double audio in modes we cannot verify.
		coilgun->anim_states[1].flags = WEPPREDANIM_MUZZLEFLASH;
		for (i = 1; i <= 6; i++)
		{
			coilgun->anim_states[i].mdlframe = i;
			coilgun->anim_states[i].nextanim = (i < 6) ? (i + 1) : 0;
			coilgun->anim_states[i].length = 100;
		}
		// END OF COILGUN
	}

	// HOOK (grapple; viewmodel + attack gate only, pull/beam stay server-side)
	wepdef = spawn();
	ExtFieldSetPvsFlags(wepdef, 3);
	ExtFieldSetSendEntity(wepdef, (func_t)WeaponDefinition_SendEntity);
	wepdef->s.v.weapon = hook - wpredict_definitions;
	hook->modelindex = k_ctf_custom_models
			? trap_precache_model("progs/v_star.mdl")
			: trap_precache_model("progs/v_axe.mdl");
	hook->attack_time = 100;
	hook->impulse = 22;
	hook->itemflag = IT_HOOK;
	hook->anim_number = 3;
	hook->anim_states[0].flags = WEPPREDANIM_DEFAULT | WEPPREDANIM_ATTACK;
	hook->anim_states[0].mdlframe = 0;
	hook->anim_states[0].nextanim = 1;
	hook->anim_states[1].mdlframe = 2;
	hook->anim_states[1].nextanim = 2;
	hook->anim_states[1].length = 100;
	hook->anim_states[2].mdlframe = 3;
	hook->anim_states[2].nextanim = 0;
	hook->anim_states[2].length = 100;
	// END OF HOOK
}
