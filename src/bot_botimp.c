// Converted from .qc on 05/02/2016

#include "g_local.h"
#include "fb_globals.h"

static char* EnemyTeamName (int botNumber);
static char* FriendTeamName (int botNumber);

static float RangeOverSkill (gedict_t* self, float minimum, float maximum)
{
	float skill = self->fb.skill.skill_level * 1.0f / (MAX_FROGBOT_SKILL - MIN_FROGBOT_SKILL);

	return minimum + skill * (maximum - minimum);
}

void RunRandomTrials (float min, float max, float mult)
{
	int i = 0;
	int trials = 1000;
	float frac = (max - min) / 6;
	int hits[] = { 0, 0, 0, 0, 0, 0 };
	for (i = 0; i < trials; ++i) {
		float result = dist_random (min, max, mult);

		if (result < min + frac)
			++hits[0];
		else if (result < min + frac * 2)
			++hits[1];
		else if (result < min + frac * 3)
			++hits[2];
		else if (result < min + frac * 4)
			++hits[3];
		else if (result < min + frac * 5)
			++hits[4];
		else
			++hits[5];
	}

	G_bprint (2, "Randomisation trials: %f to %f, %f\n", min, max, mult);
	for (i = 0; i < 6; ++i)
		G_bprint (2, " < %2.3f: %4d %3.2f%%\n", min + frac * (i+1), hits[i], hits[i] * 100.0f / trials);
}

// TODO: Exchange standard attributes for different bot characters/profiles
void SetAttribs(gedict_t* self) {
	int skill_ = self->fb.skill.skill_level;

	G_bprint (2, "skill &cf00%d&r\n", self->fb.skill.skill_level);
	if (skill_ > 10) {
		self->fb.skill.fast_aim = (skill_ - 10) * 0.1;
		skill_ = 10;
	}
	else  {
		self->fb.skill.fast_aim = 0;
	}
	self->fb.skill.accuracy = 45 - (skill_ * 2.25);

	self->fb.skill.dodge_amount = 1;
	self->fb.skill.look_anywhere = 1;
	self->fb.skill.lookahead_time = 30;
	self->fb.skill.prediction_error = 0;

	self->fb.skill.lg_preference = self->fb.skill.fast_aim;

	self->fb.skill.visibility = 0.7071067f - (0.02f * skill_);   // fov 90 (0.707) => fov 120 (0.5)

	self->fb.skill.aim_params[YAW].minimum = RangeOverSkill(self, 2, 1);
	self->fb.skill.aim_params[YAW].maximum = RangeOverSkill (self, 8, 4);
	self->fb.skill.aim_params[YAW].multiplier = RangeOverSkill(self, 3, 2);
	self->fb.skill.aim_params[YAW].scale = RangeOverSkill (self, 5, 1);

	self->fb.skill.aim_params[PITCH].minimum = RangeOverSkill(self, 2, 1);
	self->fb.skill.aim_params[PITCH].maximum = RangeOverSkill(self, 5, 2);
	self->fb.skill.aim_params[PITCH].multiplier = RangeOverSkill(self, 3, 2);
	self->fb.skill.aim_params[PITCH].scale = RangeOverSkill (self, 5, 1);

	self->fb.skill.attack_respawns = self->fb.skill.skill_level >= 15;
}

char* SetTeamNetName(int botNumber, const char* teamName) {
	float playersOnThisTeam = 0,
	      playersOnOtherTeams = 0,
	      frogbotsOnThisTeam = 0;
	char* attemptedName = NULL;
	gedict_t* search_entity = NULL;

	playersOnThisTeam = 0;
	frogbotsOnThisTeam = 0;
	playersOnOtherTeams = 0;

	for (search_entity = world; search_entity = find_plr (search_entity); ) {
		if (!search_entity->isBot) {
			if ( streq( getteam(search_entity), teamName)) {
				playersOnThisTeam = playersOnThisTeam + 1;
			}
			else {
				playersOnOtherTeams = playersOnOtherTeams + 1;
			}
		}
		else if ( streq( getteam(search_entity), teamName ) ) {
			frogbotsOnThisTeam = frogbotsOnThisTeam + 1;
		}
	}

	if (playersOnOtherTeams > 0 && playersOnThisTeam == 0) {
		attemptedName = EnemyTeamName(botNumber);
	}
	else if (playersOnThisTeam > 0 && playersOnOtherTeams == 0) {
		attemptedName = FriendTeamName(botNumber);
	}
	else {
		attemptedName = SetNetName(botNumber);
	}

	if (attemptedName) {
		return attemptedName;
	}
	return SetNetName(botNumber);
}

static char* EnemyTeamName(int botNumber) {
	char* names[] = {
		": Timber",
		": Sujoy",
		": Rix",
		": Batch",

		": Nikodemus",
		": Paralyzer",
		": Kane",
		": Gollum",

		": sCary",
		": Xenon",
		": Thresh",
		": Frick",

		": B2",
		": Reptile",
		": Unholy",
		": Spice"
	};
	char* custom_name = cvar_string (va ("k_fb_name_enemy_%d", botNumber));

	if (strnull (custom_name)) {
		return names[(int)bound(0, botNumber, sizeof(names) / sizeof(names[0]) - 1)];
	}

	return custom_name;
}

static char* FriendTeamName(int botNumber) {
	char* names[] = {
		"> MrJustice",
		"> Parrais",
		"> Jon",
		"> Gaz",

		"> Jakey",
		"> Tele",
		"> Thurg",
		"> Kool",

		"> Zaphod",
		"> Dreamer",
		"> Mandrixx",
		"> Skill5",

		"> Gunner",
		"> DanJ",
		"> Vid",
		"> Soul99"
	};
	char* custom_name = cvar_string (va ("k_fb_name_team_%d", botNumber));

	if (strnull (custom_name)) {
		return names[(int)bound(0, botNumber, sizeof(names) / sizeof(names[0]) - 1)];
	}

	return custom_name;
}

// FIXME
char* SetNetName(int botNumber) {
	char* names[] = {
		"/ bro",
		"/ goldenboy",
		"/ tincan",
		"/ grue",

		"/ dizzy",
		"/ daisy",
		"/ denzil",
		"/ dora",

		"/ shortie",
		"/ machina",
		"/ gudgie",
		"/ scoosh",

		"/ frazzle",
		"/ pop",
		"/ junk",
		"/ overflow"
	};
	char* custom_name = cvar_string (va ("k_fb_name_%d", botNumber));

	if (strnull (custom_name)) {
		return names[(int)bound(0, botNumber, sizeof(names) / sizeof(names[0]) - 1)];
	}

	return custom_name;
}
