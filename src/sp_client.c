/*  Copyright (C) 1996-1997  Id Software, Inc.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

 See file, 'COPYING', for details.
 */

// sp_client.c - single player specific functions
#include "g_local.h"

void ExitIntermission()
{
	// skip any text in deathmatch
	if (deathmatch)
	{
		GotoNextMap();

		return;
	}

	intermission_exittime = g_globalvars.time + 1;
	intermission_running++;

	//
	// run some text if at the end of an episode
	//

	if (intermission_running == 2)
	{
		if (streq(g_globalvars.mapname, "e1m7"))
		{
			WriteByte( MSG_ALL, SVC_CDTRACK);
			WriteByte( MSG_ALL, 2);

			WriteByte( MSG_ALL, SVC_FINALE);
			WriteString( MSG_ALL, "As the corpse of the monstrous entity\n"
						"Chthon sinks back into the lava whence\n"
						"it rose, you grip the Rune of Earth\n"
						"Magic tightly. Now that you have\n"
						"conquered the Dimension of the Doomed,\n"
						"realm of Earth Magic, you are ready to\n"
						"complete your task. A Rune of magic\n"
						"power lies at the end of each haunted\n"
						"land of Quake. Go forth, seek the\n"
						"totality of the four Runes!");

			return;
		}
		else if (streq(g_globalvars.mapname, "e2m6"))
		{
			WriteByte( MSG_ALL, SVC_CDTRACK);
			WriteByte( MSG_ALL, 2);

			WriteByte( MSG_ALL, SVC_FINALE);
			WriteString( MSG_ALL, "The Rune of Black Magic throbs evilly in\n"
						"your hand and whispers dark thoughts\n"
						"into your brain. You learn the inmost\n"
						"lore of the Hell-Mother; Shub-Niggurath!\n"
						"You now know that she is behind all the\n"
						"terrible plotting which has led to so\n"
						"much death and horror. But she is not\n"
						"inviolate! Armed with this Rune, you\n"
						"realize that once all four Runes are\n"
						"combined, the gate to Shub-Niggurath's\n"
						"Pit will open, and you can face the\n"
						"Witch-Goddess herself in her frightful\n"
						"otherworld cathedral.");

			return;
		}
		else if (streq(g_globalvars.mapname, "e3m6"))
		{
			WriteByte( MSG_ALL, SVC_CDTRACK);
			WriteByte( MSG_ALL, 2);

			WriteByte( MSG_ALL, SVC_FINALE);
			WriteString( MSG_ALL, "The charred viscera of diabolic horrors\n"
						"bubble viscously as you seize the Rune\n"
						"of Hell Magic. Its heat scorches your\n"
						"hand, and its terrible secrets blight\n"
						"your mind. Gathering the shreds of your\n"
						"courage, you shake the devil's shackles\n"
						"from your soul, and become ever more\n"
						"hard and determined to destroy the\n"
						"hideous creatures whose mere existence\n"
						"threatens the souls and psyches of all\n"
						"the population of Earth.");

			return;
		}
		else if (streq(g_globalvars.mapname, "e4m7"))
		{
			WriteByte( MSG_ALL, SVC_CDTRACK);
			WriteByte( MSG_ALL, 2);

			WriteByte( MSG_ALL, SVC_FINALE);
			WriteString( MSG_ALL, "Despite the awful might of the Elder\n"
						"World, you have achieved the Rune of\n"
						"Elder Magic, capstone of all types of\n"
						"arcane wisdom. Beyond good and evil,\n"
						"beyond life and death, the Rune\n"
						"pulsates, heavy with import. Patient and\n"
						"potent, the Elder Being Shub-Niggurath\n"
						"weaves her dire plans to clear off all\n"
						"life from the Earth, and bring her own\n"
						"foul offspring to our world! For all the\n"
						"dwellers in these nightmare dimensions\n"
						"are her descendants! Once all Runes of\n"
						"magic power are united, the energy\n"
						"behind them will blast open the Gateway\n"
						"to Shub-Niggurath, and you can travel\n"
						"there to foil the Hell-Mother's plots\n"
						"in person.");

			return;
		}

		GotoNextMap();
	}

	if (intermission_running == 3)
	{
		if (((int)g_globalvars.serverflags & 15) == 15)
		{
			WriteByte( MSG_ALL, SVC_FINALE);
			WriteString( MSG_ALL, "Now, you have all four Runes. You sense\n"
						"tremendous invisible forces moving to\n"
						"unseal ancient barriers. Shub-Niggurath\n"
						"had hoped to use the Runes Herself to\n"
						"clear off the Earth, but now instead,\n"
						"you will use them to enter her home and\n"
						"confront her as an avatar of avenging\n"
						"Earth-life. If you defeat her, you will\n"
						"be remembered forever as the savior of\n"
						"the planet. If she conquers, it will be\n"
						"as if you had never been born.");

			return;
		}

	}

	GotoNextMap();
}

char* ObituaryForMonster(char *attacker_class)
{
	if (streq(attacker_class, "monster_army"))
	{
		return " was shot by a Grunt\n";
	}

	if (streq(attacker_class, "monster_demon1"))
	{
		return " was eviscerated by a Fiend\n";
	}

	if (streq(attacker_class, "monster_dog"))
	{
		return " was mauled by a Rottweiler\n";
	}

	if (streq(attacker_class, "monster_dragon"))
	{
		return " was fried by a Dragon\n";
	}

	if (streq(attacker_class, "monster_enforcer"))
	{
		return " was blasted by an Enforcer\n";
	}

	if (streq(attacker_class, "monster_fish"))
	{
		return " was fed to the Rotfish\n";
	}

	if (streq(attacker_class, "monster_hell_knight"))
	{
		return " was slain by a Death Knight\n";
	}

	if (streq(attacker_class, "monster_knight"))
	{
		return " was slashed by a Knight\n";
	}

	if (streq(attacker_class, "monster_ogre"))
	{
		return " was destroyed by an Ogre\n";
	}

	if (streq(attacker_class, "monster_oldone"))
	{
		return " became one with Shub-Niggurath\n";
	}

	if (streq(attacker_class, "monster_shalrath"))
	{
		return " was exploded by a Vore\n";
	}

	if (streq(attacker_class, "monster_shambler"))
	{
		return " was smashed by a Shambler\n";
	}

	if (streq(attacker_class, "monster_tarbaby"))
	{
		return " was slimed by a Spawn\n";
	}

	if (streq(attacker_class, "monster_vomit"))
	{
		return " was vomited on by a Vomitus\n";
	}

	if (streq(attacker_class, "monster_wizard"))
	{
		return " was scragged by a Scrag\n";
	}

	if (streq(attacker_class, "monster_zombie"))
	{
		return " joins the Zombies\n";
	}

	return " killed by monster? :)\n\n";
}
