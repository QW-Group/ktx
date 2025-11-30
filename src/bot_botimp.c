/*
 bot/botimp.qc

 Copyright (C) 1997-1999 Robert 'Frog' Field
 Copyright (C) 1998-2000 Matt 'asdf' McChesney
 Copyright (C) 1999-2000 Numb
 Copyright (C) 2000-2007 ParboiL
 */

// Converted from .qc on 05/02/2016
#ifdef BOT_SUPPORT

#include "g_local.h"

#define FB_CVAR_DODGEFACTOR "k_fbskill_movement_dodgefactor"
#define FB_CVAR_LOOKANYWHERE "k_fbskill_aim_lookanywhere"
#define FB_CVAR_LOOKAHEADTIME "k_fbskill_goallookaheadtime"
#define FB_CVAR_PREDICTIONERROR "k_fbskill_goalpredictionerror"
#define FB_CVAR_DISTANCEERROR "k_fbskill_distanceerror"
#define FB_CVAR_VISIBILITY "k_fbskill_visibility"
#define FB_CVAR_LGPREF "k_fbskill_aim_lgpref"
#define FB_CVAR_ACCURACY "k_fbskill_aim_accuracy"
#define FB_CVAR_YAW_MIN_ERROR "k_fbskill_aim_yaw_min"
#define FB_CVAR_YAW_MAX_ERROR "k_fbskill_aim_yaw_max"
#define FB_CVAR_YAW_MULTIPLIER "k_fbskill_aim_yaw_multiplier"
#define FB_CVAR_YAW_SCALE "k_fbskill_aim_yaw_scale"
#define FB_CVAR_PITCH_MIN_ERROR "k_fbskill_aim_pitch_min"
#define FB_CVAR_PITCH_MAX_ERROR "k_fbskill_aim_pitch_max"
#define FB_CVAR_PITCH_MULTIPLIER "k_fbskill_aim_pitch_multiplier"
#define FB_CVAR_PITCH_SCALE "k_fbskill_aim_pitch_scale"
#define FB_CVAR_ATTACK_RESPAWNS "k_fbskill_aim_attack_respawns"
#define FB_CVAR_REACTION_TIME "k_fbskill_reactiontime"
#define FB_CVAR_REACTION_MOVETIME "k_fbskill_reactionmovetime"

#define FB_CVAR_MIN_VOLATILITY "k_fbskill_vol_min"
#define FB_CVAR_MAX_VOLATILITY "k_fbskill_vol_max"
#define FB_CVAR_INITIAL_VOLATILITY "k_fbskill_vol_init"
#define FB_CVAR_REDUCE_VOLATILITY "k_fbskill_vol_reduce"
#define FB_CVAR_OWNSPEED_VOLATILITY_THRESHOLD "k_fbskill_vol_ownvel"
#define FB_CVAR_OWNSPEED_VOLATILITY_INCREASE "k_fbskill_vol_ownvel_incr"
#define FB_CVAR_ENEMYSPEED_VOLATILITY_THRESHOLD "k_fbskill_vol_oppvel"
#define FB_CVAR_ENEMYSPEED_VOLATILITY_INCREASE "k_fbskill_vol_oppvel_incr"
#define FB_CVAR_ENEMYDIRECTION_VOLATILITY_INCREASE "k_fbskill_vol_oppdir_incr"
#define FB_CVAR_PAIN_VOLATILITY_INCREASE "k_fbskill_vol_pain_incr"
#define FB_CVAR_SELF_MIDAIR_VOLATILITY_INCREASE "k_fbskill_vol_bot_midair_incr"
#define FB_CVAR_OPPONENT_MIDAIR_VOLATILITY_INCREASE "k_fbskill_vol_opp_midair_incr"

#define FB_CVAR_MOVEMENT_SKILL "k_fbskill_movement"
#define FB_CVAR_USE_ROCKETJUMPS "k_fbskill_use_rocketjumps"
#define FB_CVAR_MOVEMENT_DMM4WIGGLE "k_fbskill_dmm4wiggle"
#define FB_CVAR_MOVEMENT_DMM4WIGGLETOGGLE "k_fbskill_dmm4wiggletoggle"
#define FB_CVAR_MOVEMENT_WIGGLEFRAMES "k_fbskill_wiggleframes"
#define FB_CVAR_COMBATJUMP_CHANCE "k_fbskill_combatjump"
#define FB_CVAR_MISSILEDODGE_TIME "k_fbskill_missiledodge"

static float RangeOverSkill(int skill_level, float minimum, float maximum)
{
	float skill = skill_level * 1.0f / (MAX_FROGBOT_SKILL - MIN_FROGBOT_SKILL);

	return (minimum + skill * (maximum - minimum));
}

void RunRandomTrials(float min, float max, float mult)
{
	int i = 0;
	int trials = 1000;
	float frac = (max - min) / 6;
	int hits[] =
		{ 0, 0, 0, 0, 0, 0 };

	for (i = 0; i < trials; ++i)
	{
		float result = dist_random(min, max, mult);

		if (result < (min + frac))
		{
			++hits[0];
		}
		else if (result < (min + (frac * 2)))
		{
			++hits[1];
		}
		else if (result < (min + (frac * 3)))
		{
			++hits[2];
		}
		else if (result < (min + (frac * 4)))
		{
			++hits[3];
		}
		else if (result < (min + (frac * 5)))
		{
			++hits[4];
		}
		else
		{
			++hits[5];
		}
	}

	G_bprint(2, "Randomisation trials: %f to %f, %f\n", min, max, mult);
	for (i = 0; i < 6; ++i)
	{
		G_bprint(2, " < %2.3f: %4d %3.2f%%\n", min + frac * (i + 1), hits[i],
					hits[i] * 100.0f / trials);
	}
}

void RegisterSkillVariables(void)
{
	extern qbool RegisterCvar(const char *var);

	RegisterCvar(FB_CVAR_DODGEFACTOR);
	RegisterCvar(FB_CVAR_LOOKANYWHERE);
	RegisterCvar(FB_CVAR_LOOKAHEADTIME);
	RegisterCvar(FB_CVAR_PREDICTIONERROR);
	RegisterCvar(FB_CVAR_VISIBILITY);
	RegisterCvar(FB_CVAR_LGPREF);
	RegisterCvar(FB_CVAR_ACCURACY);
	RegisterCvar(FB_CVAR_YAW_MIN_ERROR);
	RegisterCvar(FB_CVAR_YAW_MAX_ERROR);
	RegisterCvar(FB_CVAR_YAW_MULTIPLIER);
	RegisterCvar(FB_CVAR_YAW_SCALE);
	RegisterCvar(FB_CVAR_PITCH_MIN_ERROR);
	RegisterCvar(FB_CVAR_PITCH_MAX_ERROR);
	RegisterCvar(FB_CVAR_PITCH_MULTIPLIER);
	RegisterCvar(FB_CVAR_PITCH_SCALE);
	RegisterCvar(FB_CVAR_ATTACK_RESPAWNS);
	RegisterCvar(FB_CVAR_REACTION_TIME);
	RegisterCvar(FB_CVAR_REACTION_MOVETIME);

	RegisterCvar(FB_CVAR_MIN_VOLATILITY);
	RegisterCvar(FB_CVAR_MAX_VOLATILITY);
	RegisterCvar(FB_CVAR_INITIAL_VOLATILITY);
	RegisterCvar(FB_CVAR_REDUCE_VOLATILITY);
	RegisterCvar(FB_CVAR_OWNSPEED_VOLATILITY_THRESHOLD);
	RegisterCvar(FB_CVAR_OWNSPEED_VOLATILITY_INCREASE);
	RegisterCvar(FB_CVAR_ENEMYSPEED_VOLATILITY_THRESHOLD);
	RegisterCvar(FB_CVAR_ENEMYSPEED_VOLATILITY_INCREASE);
	RegisterCvar(FB_CVAR_ENEMYDIRECTION_VOLATILITY_INCREASE);

	RegisterCvar(FB_CVAR_MOVEMENT_SKILL);
	RegisterCvar(FB_CVAR_USE_ROCKETJUMPS);
	RegisterCvar(FB_CVAR_MOVEMENT_DMM4WIGGLE);
	RegisterCvar(FB_CVAR_MOVEMENT_WIGGLEFRAMES);
	RegisterCvar(FB_CVAR_MOVEMENT_DMM4WIGGLETOGGLE);
	RegisterCvar(FB_CVAR_COMBATJUMP_CHANCE);
	RegisterCvar(FB_CVAR_MISSILEDODGE_TIME);

	RegisterCvar(FB_CVAR_DISTANCEERROR);
	RegisterCvar(FB_CVAR_PAIN_VOLATILITY_INCREASE);
	RegisterCvar(FB_CVAR_SELF_MIDAIR_VOLATILITY_INCREASE);
	RegisterCvar(FB_CVAR_OPPONENT_MIDAIR_VOLATILITY_INCREASE);
}

void setSkillAttributes(int skill, int aimskill) {
	// Old frogbot settings (items generally)
	cvar_fset(FB_CVAR_ACCURACY, 45 - min(skill, 10) * 2.25);
	cvar_fset(FB_CVAR_DODGEFACTOR, RangeOverSkill(skill, 0.0f, 1.0f));
	cvar_fset(FB_CVAR_LOOKANYWHERE, RangeOverSkill(skill, 0.0f, 1.0f));
	cvar_fset(FB_CVAR_LOOKAHEADTIME, RangeOverSkill(skill, 5.0f, 30.0f));
	cvar_fset(FB_CVAR_PREDICTIONERROR, RangeOverSkill(skill, 1.0f, 0.0f));
	cvar_fset(FB_CVAR_DISTANCEERROR, RangeOverSkill(skill, 0.15f, 0.0f));

	// Old, but used to be global
	cvar_fset(FB_CVAR_LGPREF, RangeOverSkill(skill, 0.2f, 1.0f));
	cvar_fset(FB_CVAR_VISIBILITY, 0.7071067f - (0.02f * min(skill, 10))); // equivalent of 90 => 120 fov

	cvar_fset(FB_CVAR_YAW_MIN_ERROR, RangeOverSkill(aimskill, 1.5, 1));
	cvar_fset(FB_CVAR_YAW_MAX_ERROR, RangeOverSkill(aimskill, 4.5, 3));
	cvar_fset(FB_CVAR_YAW_MULTIPLIER, RangeOverSkill(aimskill, 4, 2.5));
	cvar_fset(FB_CVAR_YAW_SCALE, RangeOverSkill(aimskill, 5, 2));

	cvar_fset(FB_CVAR_PITCH_MIN_ERROR, RangeOverSkill(aimskill, 1.5, 1));
	cvar_fset(FB_CVAR_PITCH_MAX_ERROR, RangeOverSkill(aimskill, 4.5, 3));
	cvar_fset(FB_CVAR_PITCH_MULTIPLIER, RangeOverSkill(aimskill, 4, 2));
	cvar_fset(FB_CVAR_PITCH_SCALE, RangeOverSkill(aimskill, 5, 2));

	cvar_fset(FB_CVAR_ATTACK_RESPAWNS, skill >= 15 ? 1 : 0);
	cvar_fset(FB_CVAR_REACTION_TIME, RangeOverSkill(skill, 0.75f, 0.3f));
	cvar_fset(FB_CVAR_REACTION_MOVETIME, RangeOverSkill(skill, 0.3f, 0.1f));

	// Volatility
	cvar_fset(FB_CVAR_MIN_VOLATILITY, 1.0f);
	cvar_fset(FB_CVAR_MAX_VOLATILITY, RangeOverSkill(skill, 4.0f, 2.5f));
	cvar_fset(FB_CVAR_INITIAL_VOLATILITY, RangeOverSkill(skill, 3.0f, 1.4f));
	cvar_fset(FB_CVAR_REDUCE_VOLATILITY, RangeOverSkill(skill, 0.98f, 0.96f));
	cvar_fset(FB_CVAR_OWNSPEED_VOLATILITY_THRESHOLD, RangeOverSkill(skill, 360, 450));
	cvar_fset(FB_CVAR_OWNSPEED_VOLATILITY_INCREASE, RangeOverSkill(skill, 0.2f, 0.1f));
	cvar_fset(FB_CVAR_ENEMYSPEED_VOLATILITY_THRESHOLD, RangeOverSkill(skill, 360, 450));
	cvar_fset(FB_CVAR_ENEMYSPEED_VOLATILITY_INCREASE, RangeOverSkill(skill, 0.4f, 0.2f));
	cvar_fset(FB_CVAR_ENEMYDIRECTION_VOLATILITY_INCREASE, RangeOverSkill(skill, 0.6f, 0.4f));
	cvar_fset(FB_CVAR_PAIN_VOLATILITY_INCREASE, RangeOverSkill(skill, 0.5f, 0.1f));
	cvar_fset(FB_CVAR_SELF_MIDAIR_VOLATILITY_INCREASE, RangeOverSkill(skill, 1.0f, 0.0f));
	cvar_fset(FB_CVAR_OPPONENT_MIDAIR_VOLATILITY_INCREASE, RangeOverSkill(skill, 1.0f, 0.0f));

	// Movement
	cvar_fset(FB_CVAR_MOVEMENT_SKILL, RangeOverSkill(skill, 0.3f, 1.0f));
	cvar_fset(FB_CVAR_MOVEMENT_DMM4WIGGLE, skill > 10 ? 1 : 0);
	cvar_fset(FB_CVAR_MOVEMENT_DMM4WIGGLETOGGLE,
				skill > 10 ? RangeOverSkill((skill - 10) * 2, 0.0f, 0.25f) : 0);
	cvar_fset(FB_CVAR_MOVEMENT_WIGGLEFRAMES, RangeOverSkill(skill, 30, 20));
	cvar_fset(FB_CVAR_COMBATJUMP_CHANCE, RangeOverSkill(skill, 0.03f, 0.1f));
	cvar_fset(FB_CVAR_MISSILEDODGE_TIME, RangeOverSkill(skill, 1.0f, 0.5f));
}

void setSkillAttributesEasySkillMode(int skill, int aimskill) {
	// Old frogbot settings (items generally)
	cvar_fset(FB_CVAR_ACCURACY, 45 - min(skill, 10) * 2.25);
	cvar_fset(FB_CVAR_DODGEFACTOR, RangeOverSkill(skill, 0.0f, 1.0f));
	cvar_fset(FB_CVAR_LOOKANYWHERE, RangeOverSkill(skill, 0.0f, 1.0f));
	cvar_fset(FB_CVAR_LOOKAHEADTIME, RangeOverSkill(skill, 5.0f, 30.0f));
	cvar_fset(FB_CVAR_PREDICTIONERROR, RangeOverSkill(skill, 1.0f, 0.0f));
	cvar_fset(FB_CVAR_DISTANCEERROR, RangeOverSkill(skill, 0.25f, 0.0f));

	// Old, but used to be global
	cvar_fset(FB_CVAR_LGPREF, RangeOverSkill(skill, 0.2f, 1.0f));
	cvar_fset(FB_CVAR_VISIBILITY, 0.7071067f - (0.02f * min(skill, 10))); // equivalent of 90 => 120 fov

	cvar_fset(FB_CVAR_YAW_MIN_ERROR, RangeOverSkill(aimskill, 3, 1));
	cvar_fset(FB_CVAR_YAW_MAX_ERROR, RangeOverSkill(aimskill, 6, 3));
	cvar_fset(FB_CVAR_YAW_MULTIPLIER, RangeOverSkill(aimskill, 5, 2.5));
	cvar_fset(FB_CVAR_YAW_SCALE, RangeOverSkill(aimskill, 7, 2));

	cvar_fset(FB_CVAR_PITCH_MIN_ERROR, RangeOverSkill(aimskill, 3, 1));
	cvar_fset(FB_CVAR_PITCH_MAX_ERROR, RangeOverSkill(aimskill, 6, 3));
	cvar_fset(FB_CVAR_PITCH_MULTIPLIER, RangeOverSkill(aimskill, 5, 2));
	cvar_fset(FB_CVAR_PITCH_SCALE, RangeOverSkill(aimskill, 7, 2));

	cvar_fset(FB_CVAR_ATTACK_RESPAWNS, skill >= 15 ? 1 : 0);
	cvar_fset(FB_CVAR_REACTION_TIME, RangeOverSkill(skill, 1.5f, 0.3f));
	cvar_fset(FB_CVAR_REACTION_MOVETIME, RangeOverSkill(skill, 0.3f, 0.1f));

	// Volatility
	cvar_fset(FB_CVAR_MIN_VOLATILITY, 1.0f);
	cvar_fset(FB_CVAR_MAX_VOLATILITY, RangeOverSkill(skill, 4.0f, 2.5f));
	cvar_fset(FB_CVAR_INITIAL_VOLATILITY, RangeOverSkill(skill, 3.0f, 1.4f));
	cvar_fset(FB_CVAR_REDUCE_VOLATILITY, RangeOverSkill(skill, 0.98f, 0.96f));
	cvar_fset(FB_CVAR_OWNSPEED_VOLATILITY_THRESHOLD, RangeOverSkill(skill, 360, 450));
	cvar_fset(FB_CVAR_OWNSPEED_VOLATILITY_INCREASE, RangeOverSkill(skill, 0.2f, 0.1f));
	cvar_fset(FB_CVAR_ENEMYSPEED_VOLATILITY_THRESHOLD, RangeOverSkill(skill, 360, 450));
	cvar_fset(FB_CVAR_ENEMYSPEED_VOLATILITY_INCREASE, RangeOverSkill(skill, 0.4f, 0.2f));
	cvar_fset(FB_CVAR_ENEMYDIRECTION_VOLATILITY_INCREASE, RangeOverSkill(skill, 0.6f, 0.4f));
	cvar_fset(FB_CVAR_PAIN_VOLATILITY_INCREASE, RangeOverSkill(skill, 0.5f, 0.1f));
	cvar_fset(FB_CVAR_SELF_MIDAIR_VOLATILITY_INCREASE, RangeOverSkill(skill, 1.0f, 0.0f));
	cvar_fset(FB_CVAR_OPPONENT_MIDAIR_VOLATILITY_INCREASE, RangeOverSkill(skill, 1.0f, 0.0f));

	// Movement
	cvar_fset(FB_CVAR_MOVEMENT_SKILL, RangeOverSkill(skill, 0.0f, 1.0f));
	cvar_fset(FB_CVAR_USE_ROCKETJUMPS, skill > 5 ? 1 : 0);
	cvar_fset(FB_CVAR_MOVEMENT_DMM4WIGGLE, skill > 10 ? 1 : 0);
	cvar_fset(FB_CVAR_MOVEMENT_DMM4WIGGLETOGGLE,
				skill > 10 ? RangeOverSkill((skill - 10) * 2, 0.0f, 0.25f) : 0);
	cvar_fset(FB_CVAR_MOVEMENT_WIGGLEFRAMES, RangeOverSkill(skill, 30, 20));
	cvar_fset(FB_CVAR_COMBATJUMP_CHANCE, RangeOverSkill(skill, 0.0f, 0.1f));
	cvar_fset(FB_CVAR_MISSILEDODGE_TIME, RangeOverSkill(skill, 1.0f, 0.5f));
}

qbool SetAttributesBasedOnSkill(int skill)
{
	char *cfg_name;
	qbool customised = false;
	int aimskill;

	skill = bound(MIN_FROGBOT_SKILL, skill, MAX_FROGBOT_SKILL);
	aimskill = bound(MIN_FROGBOT_SKILL, skill, MAX_FROGBOT_AIM_SKILL);

	if (FrogbotEasySkillMode())
	{
		G_bprint(2, "%s\n", redtext("Using easy bot skill mode"));
		setSkillAttributesEasySkillMode(skill, aimskill);
	}
	else
	{
		G_bprint(2, "%s\n", redtext("Using default bot skill mode"));
		setSkillAttributes(skill, aimskill);
	}

	// Customise
	{
		char buf[1024 * 4];

		cfg_name = va("bots/configs/skill_all.cfg");
		if (can_exec(cfg_name))
		{
			trap_readcmd(va("exec %s\n", cfg_name), buf, sizeof(buf));
			customised = true;
		}

		cfg_name = va("bots/configs/skill_%02d.cfg", skill);
		if (can_exec(cfg_name))
		{
			trap_readcmd(va("exec %s\n", cfg_name), buf, sizeof(buf));
			customised = true;
		}
	}

	return customised;
}

// TODO: Exchange standard attributes for different bot characters/profiles
void SetAttribs(gedict_t *self, qbool customised)
{
	self->fb.skill.accuracy = bound(0, cvar( FB_CVAR_ACCURACY), 45);

	self->fb.skill.dodge_amount = bound(0, cvar( FB_CVAR_DODGEFACTOR), 1);
	self->fb.skill.look_anywhere = bound(0, cvar( FB_CVAR_LOOKANYWHERE), 1);
	self->fb.skill.lookahead_time = bound(0, cvar( FB_CVAR_LOOKAHEADTIME), 45);
	self->fb.skill.prediction_error = bound(0, cvar(FB_CVAR_PREDICTIONERROR), 1);
	self->fb.skill.movement_estimate_error = bound(0, cvar(FB_CVAR_DISTANCEERROR), 0.25);

	self->fb.skill.lg_preference = bound(0, cvar( FB_CVAR_LGPREF), 1);
	self->fb.skill.visibility = bound(0.5, cvar( FB_CVAR_VISIBILITY), 0.7071067f); // fov 90 (0.707) => fov 120 (0.5)

	self->fb.skill.aim_params[YAW].minimum = bound(0, cvar(FB_CVAR_YAW_MIN_ERROR), 1);
	self->fb.skill.aim_params[YAW].maximum = bound(0, cvar(FB_CVAR_YAW_MAX_ERROR), 10);
	self->fb.skill.aim_params[YAW].multiplier = bound(0, cvar(FB_CVAR_YAW_MULTIPLIER), 10);
	self->fb.skill.aim_params[YAW].scale = bound(0, cvar(FB_CVAR_YAW_SCALE), 5);

	self->fb.skill.aim_params[PITCH].minimum = bound(0, cvar(FB_CVAR_PITCH_MIN_ERROR), 10);
	self->fb.skill.aim_params[PITCH].maximum = bound(0, cvar(FB_CVAR_PITCH_MAX_ERROR), 10);
	self->fb.skill.aim_params[PITCH].multiplier = bound(0, cvar(FB_CVAR_PITCH_MULTIPLIER), 10);
	self->fb.skill.aim_params[PITCH].scale = bound(0, cvar(FB_CVAR_PITCH_SCALE), 5);

	self->fb.skill.attack_respawns = cvar(FB_CVAR_ATTACK_RESPAWNS) > 0;

	// Volatility
	self->fb.skill.min_volatility = bound(0, cvar(FB_CVAR_MIN_VOLATILITY), 5.0f);
	self->fb.skill.max_volatility = bound(0, cvar(FB_CVAR_MAX_VOLATILITY), 5.0f);
	self->fb.skill.initial_volatility = bound(0, cvar(FB_CVAR_INITIAL_VOLATILITY), 5.0f);
	self->fb.skill.reduce_volatility = bound(0, cvar(FB_CVAR_REDUCE_VOLATILITY), 1.0f);
	self->fb.skill.ownspeed_volatility_threshold = bound(
			0, cvar(FB_CVAR_OWNSPEED_VOLATILITY_THRESHOLD), 1000);
	self->fb.skill.ownspeed_volatility = bound(0, cvar(FB_CVAR_OWNSPEED_VOLATILITY_INCREASE), 5.0f);
	self->fb.skill.enemyspeed_volatility_threshold = bound(
			0, cvar(FB_CVAR_ENEMYSPEED_VOLATILITY_THRESHOLD), 1000);
	self->fb.skill.enemyspeed_volatility = bound(0, cvar(FB_CVAR_ENEMYSPEED_VOLATILITY_INCREASE),
													5.0f);
	self->fb.skill.enemydirection_volatility = bound(
			0, cvar(FB_CVAR_ENEMYDIRECTION_VOLATILITY_INCREASE), 5.0f);
	self->fb.skill.awareness_delay = bound(0, cvar(FB_CVAR_REACTION_TIME), 1.5f);
	self->fb.skill.spawn_move_delay = bound(0, cvar(FB_CVAR_REACTION_MOVETIME), 1.0f);
	self->fb.skill.pain_volatility = bound(0, cvar(FB_CVAR_PAIN_VOLATILITY_INCREASE), 2.0f);
	self->fb.skill.self_midair_volatility = bound(0, cvar(FB_CVAR_SELF_MIDAIR_VOLATILITY_INCREASE),
													2.0f);
	self->fb.skill.opponent_midair_volatility = bound(
			0, cvar(FB_CVAR_OPPONENT_MIDAIR_VOLATILITY_INCREASE), 2.0f);

	// Movement
	self->fb.skill.movement = bound(0, cvar(FB_CVAR_MOVEMENT_SKILL), 1.0f);
	self->fb.skill.use_rocketjumps = cvar(FB_CVAR_USE_ROCKETJUMPS) > 0;
	self->fb.skill.wiggle_run_dmm4 = bound(0, (int)cvar(FB_CVAR_MOVEMENT_DMM4WIGGLE), 1.0f);
	self->fb.skill.wiggle_run_limit = bound(0, (int)cvar(FB_CVAR_MOVEMENT_WIGGLEFRAMES), 45.0f);
	self->fb.skill.wiggle_toggle = bound(0, cvar(FB_CVAR_MOVEMENT_DMM4WIGGLETOGGLE), 1.0f);
	self->fb.skill.combat_jump_chance = bound(0, cvar(FB_CVAR_COMBATJUMP_CHANCE), 1.0f);
	self->fb.skill.missile_dodge_time = bound(0, cvar(FB_CVAR_MISSILEDODGE_TIME), 1.5f);

	self->fb.skill.customised = customised;
}

char* BotNameEnemy(int botNumber)
{
	char *names[] =
		{ ": Timber", ": Sujoy", ": Nightwing", ": Cenobite", ": Thresh", ": Frick", ": Unholy",
				": Reptile", ": Nikodemus", ": Paralyzer", ": Xenon", ": Spice"
						": Kornelia", ": Rix", ": Batch", ": Gollum" };
	char *custom_name = cvar_string(va("k_fb_name_enemy_%d", botNumber));

	if (strnull(custom_name))
	{
		return names[(int)bound(0, botNumber, sizeof(names) / sizeof(names[0]) - 1)];
	}

	return custom_name;
}

char* BotNameFriendly(int botNumber)
{
	char *names[] =
		{ "> MrJustice", "> DanJ", "> Gunner", "> Tele", "> Jakey", "> Parrais", "> Thurg",
				"> Kool", "> Zaphod", "> Dreamer", "> Mandrixx", "> Skill5", "> Vid", "> Soul99",
				"> Jon", "> Gaz" };
	char *custom_name = cvar_string(va("k_fb_name_team_%d", botNumber));

	if (strnull(custom_name))
	{
		return names[(int)bound(0, botNumber, sizeof(names) / sizeof(names[0]) - 1)];
	}

	return custom_name;
}

char* BotNameGeneric(int botNumber)
{
	char *names[] =
		{ "/ bro", "/ goldenboy", "/ tincan", "/ grue", "/ dizzy", "/ daisy", "/ denzil", "/ dora",
				"/ shortie", "/ machina", "/ gudgie", "/ scoosh", "/ frazzle", "/ pop", "/ junk",
				"/ overflow" };
	char *hf_names[] =
		{
			"mutilator", "drejfus", "griffin", "heddan", "legio", "wigorf", "madmax",
			"mrlame", "aptiva", "nepra", "nikke", "parasite", "rushing",
			"lipton", "xorcist" };

	char *custom_name = cvar_string(va("k_fb_name_%d", botNumber));

	if (strnull(custom_name))
	{
		return tot_mode_enabled()
			? hf_names[(int)bound(0, botNumber, sizeof(hf_names) / sizeof(hf_names[0]) - 1)]
			: names[(int)bound(0, botNumber, sizeof(names) / sizeof(names[0]) - 1)];
	}

	return custom_name;
}

#endif
