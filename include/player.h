/*
 *  QWProgs-DM
 *  Copyright (C) 2004  [sd] angel
 *
 *  This code is based on QuakeWorld DM mod code by Id Software, Inc.
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *  $Id$
 */

void player_stand1(void);
void player_run(void);
void player_shot1(void);
void player_shot2(void);
void player_shot3(void);
void player_shot4(void);
void player_shot5(void);
void player_shot6(void);
void player_axe1(void);
void player_axe2(void);
void player_axe3(void);
void player_axe4(void);
void player_axeb1(void);
void player_axeb2(void);
void player_axeb3(void);
void player_axeb4(void);
void player_axec1(void);
void player_axec2(void);
void player_axec3(void);
void player_axec4(void);
void player_axed1(void);
void player_axed2(void);
void player_axed3(void);
void player_axed4(void);
void player_chain1(void);
void player_chain2(void);
void player_chain3(void);
void player_chain4(void);
void player_chain5(void);
void player_nail1(void);
void player_nail2(void);
void player_light1(void);
void player_light2(void);
void player_rocket1(void);
void player_rocket2(void);
void player_rocket3(void);
void player_rocket4(void);
void player_rocket5(void);
void player_rocket6(void);
void player_pain1(void);
void player_pain2(void);
void player_pain3(void);
void player_pain4(void);
void player_pain5(void);
void player_pain6(void);
void player_axpain1(void);
void player_axpain2(void);
void player_axpain3(void);
void player_axpain4(void);
void player_axpain5(void);
void player_axpain6(void);
void player_pain(struct gedict_s *attacker, float take);
void player_diea1(void);
void player_diea2(void);
void player_diea3(void);
void player_diea4(void);
void player_diea5(void);
void player_diea6(void);
void player_diea7(void);
void player_diea8(void);
void player_diea9(void);
void player_diea10(void);
void player_diea11(void);
void player_dieb1(void);
void player_dieb2(void);
void player_dieb3(void);
void player_dieb4(void);
void player_dieb5(void);
void player_dieb6(void);
void player_dieb7(void);
void player_dieb8(void);
void player_dieb9(void);
void player_diec1(void);
void player_diec2(void);
void player_diec3(void);
void player_diec4(void);
void player_diec5(void);
void player_diec6(void);
void player_diec7(void);
void player_diec8(void);
void player_diec9(void);
void player_diec10(void);
void player_diec11(void);
void player_diec12(void);
void player_diec13(void);
void player_diec14(void);
void player_diec15(void);
void player_died1(void);
void player_died2(void);
void player_died3(void);
void player_died4(void);
void player_died5(void);
void player_died6(void);
void player_died7(void);
void player_died8(void);
void player_died9(void);
void player_diee1(void);
void player_diee2(void);
void player_diee3(void);
void player_diee4(void);
void player_diee5(void);
void player_diee6(void);
void player_diee7(void);
void player_diee8(void);
void player_diee9(void);
void player_die_ax1(void);
void player_die_ax2(void);
void player_die_ax3(void);
void player_die_ax4(void);
void player_die_ax5(void);
void player_die_ax6(void);
void player_die_ax7(void);
void player_die_ax8(void);
void player_die_ax9(void);
void PlayerDead(void);
void PlayerDie(void);

gedict_t* ThrowGib(char *gibname, float dm);
void ThrowHead(char *gibname, float dm);

void muzzleflash(void);
