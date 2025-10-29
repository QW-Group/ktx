/*
 *  QWProgs-DM
 *  Copyright (C) 2004  [sd] angel
 *
 *  This code is based on Q3 VM code by Id Software, Inc.
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

//
// constants
//
// edict.flags
#define FL_FLY							1
#define FL_SWIM							2
#define FL_CLIENT						8			// set for all client edicts
#define FL_INWATER						16			// for enter / leave water splash
#define FL_MONSTER						32
#define FL_GODMODE						64			// player cheat
#define FL_NOTARGET						128			// player cheat
#define FL_ITEM							256			// extra wide size for bonus items
#define FL_ONGROUND						512			// standing on something
#define FL_PARTIALGROUND				1024		// not all corners are valid
#define FL_WATERJUMP					2048		// player jumping out of water
#define FL_JUMPRELEASED					4096		// for jump debouncing
#define FL_ATTACKRELEASED				8192		// for attack debouncing

// edict.movetype values
#define MOVETYPE_NONE					0			// never moves
//#define MOVETYPE_ANGLENOCLIP			1
//#define MOVETYPE_ANGLECLIP			2
#define MOVETYPE_WALK					3			// players only
#define MOVETYPE_STEP					4			// discrete, not real time unless fall
#define MOVETYPE_FLY					5
#define MOVETYPE_TOSS					6			// gravity
#define MOVETYPE_PUSH					7			// no clip to world, push and crush
#define MOVETYPE_NOCLIP					8
#define MOVETYPE_FLYMISSILE				9			// fly with extra size against monsters
#define MOVETYPE_BOUNCE					10
#define MOVETYPE_BOUNCEMISSILE			11			// bounce with extra size

#define MOVETYPE_LOCK					15			// used for player spectating != spectator

// edict.solid values
#define SOLID_NOT						0			// no interaction with other objects
#define SOLID_TRIGGER					1			// touch on edge, but not blocking
#define SOLID_BBOX						2			// touch on edge, block
#define SOLID_SLIDEBOX					3			// touch on edge, but not an onground
#define SOLID_BSP						4			// bsp clip, touch on edge, block

// range values
#define RANGE_MELEE						0
#define RANGE_NEAR						1
#define RANGE_MID						2
#define RANGE_FAR						3

// deadflag values
#define DEAD_NO							0
#define DEAD_DYING						1
#define DEAD_DEAD						2
#define DEAD_RESPAWNABLE				3

// takedamage values
#define DAMAGE_NO						0
#define DAMAGE_YES						1
#define DAMAGE_AIM						2

// items
#define IT_AXE							4096
#define IT_SHOTGUN						1
#define IT_SUPER_SHOTGUN				2
#define IT_NAILGUN						4
#define IT_SUPER_NAILGUN				8
#define IT_GRENADE_LAUNCHER				16
#define IT_ROCKET_LAUNCHER				32
#define IT_LIGHTNING					64
#define IT_EXTRA_WEAPON					128

#define IT_RL_AND_LG					(IT_ROCKET_LAUNCHER | IT_LIGHTNING)

#define IT_SHELLS						256
#define IT_NAILS						512
#define IT_ROCKETS						1024
#define IT_CELLS						2048

#define IT_ARMOR1						8192
#define IT_ARMOR2						16384
#define IT_ARMOR3						32768
#define IT_SUPERHEALTH					65536

#define IT_KEY1							131072
#define IT_KEY2							262144

#define IT_INVISIBILITY					524288
#define IT_INVULNERABILITY				1048576
#define IT_SUIT							2097152
#define IT_QUAD							4194304
#define IT_HOOK							8388608

/* Could use these so clients can display which rune they have
 #define IT_SIGIL1 (1<<28)
 #define IT_SIGIL2 (1<<29)
 #define IT_SIGIL3 (1<<30)
 #define IT_SIGIL4 (1<<31)
 */

// point content values
#define CONTENT_EMPTY					-1
#define CONTENT_SOLID					-2
#define CONTENT_WATER					-3
#define CONTENT_SLIME					-4
#define CONTENT_LAVA					-5
#define CONTENT_SKY						-6

#define STATE_TOP						0
#define STATE_BOTTOM					1
#define STATE_UP						2
#define STATE_DOWN						3

// sound channels
// channel 0 never willingly overrides
// other channels (1-7) allways override a playing sound on that channel
#define CHAN_AUTO						0
#define CHAN_WEAPON						1
#define CHAN_VOICE						2
#define CHAN_ITEM						3
#define CHAN_BODY						4
#define CHAN_FLAG						5
#define CHAN_NO_PHS_ADD					8

#define ATTN_NONE						0
#define ATTN_NORM						1
#define ATTN_IDLE						2
#define ATTN_STATIC						3

// update types
#define UPDATE_GENERAL					0
#define UPDATE_STATIC					1
#define UPDATE_BINARY					2
#define UPDATE_TEMP						3

// entity effects
#define EF_BRIGHTFIELD					1
//#define EF_MUZZLEFLASH				2
#define EF_GREEN						2
#define EF_BRIGHTLIGHT					4
#define EF_DIMLIGHT						8
#define EF_FLAG1						16
#define EF_FLAG2						32
#define EF_BLUE							64
#define EF_RED							128

// messages
// Since BROADCAST is never used in QW 1.5, and MULTICAST is used instead,
// just define BROADCAST as MULTICAST for QW 1.5
#define MSG_MULTICAST					4
#define MSG_BROADCAST					MSG_MULTICAST
#define MSG_ONE							1			// reliable to one (msg_entity)
#define MSG_ALL							2			// reliable to all
#define MSG_INIT						3			// write to the init string

// message levels
#define PRINT_LOW						0			// pickup messages
#define PRINT_MEDIUM					1			// death messages
#define PRINT_HIGH						2			// critical messages
#define PRINT_CHAT						3			// also goes to chat console

#define AS_STRAIGHT						1
#define AS_SLIDING						2
#define AS_MELEE						3
#define AS_MISSILE						4

#define	SPAWNFLAG_NOT_EASY				256
#define	SPAWNFLAG_NOT_MEDIUM			512
#define	SPAWNFLAG_NOT_HARD				1024
#define	SPAWNFLAG_NOT_DEATHMATCH		2048
#define	SPAWNFLAG_NOT_SP				(SPAWNFLAG_NOT_EASY | SPAWNFLAG_NOT_MEDIUM | SPAWNFLAG_NOT_HARD)

// protocol bytes

#define SVC_UPDATEFRAGS					14
#define SVC_TEMPENTITY					23
#define SVC_SETPAUSE					24
#define SVC_CENTERPRINT					26
#define SVC_KILLEDMONSTER				27
#define SVC_FOUNDSECRET					28
#define SVC_INTERMISSION				30
#define SVC_FINALE						31
#define SVC_CDTRACK						32
#define SVC_SELLSCREEN					33

#define SVC_SMALLKICK					34
#define SVC_BIGKICK						35
#define SVC_UPDATEPING					36
#define	SVC_UPDATEENTERTIME				37
#define SVC_MUZZLEFLASH					39
#define SVC_UPDATEUSERINFO				40

#define TE_SPIKE						0
#define TE_SUPERSPIKE					1
#define TE_GUNSHOT						2
#define TE_EXPLOSION					3
#define TE_TAREXPLOSION					4
#define TE_LIGHTNING1					5
#define TE_LIGHTNING2					6
#define TE_WIZSPIKE						7
#define TE_KNIGHTSPIKE					8
#define TE_LIGHTNING3					9
#define TE_LAVASPLASH					10
#define TE_TELEPORT						11
#define TE_BLOOD						12
#define TE_LIGHTNINGBLOOD				13

// multicast sets
#define MULTICAST_ALL					0	// every client
#define MULTICAST_PHS					1	// within hearing
#define MULTICAST_PVS					2	// within sight
#define MULTICAST_ALL_R					3	// every client, reliable
#define MULTICAST_PHS_R					4	// within hearing, reliable
#define MULTICAST_PVS_R					5	// within sight, reliable
#define MULTICAST_KTX1_EXT				6	// Only send to those using ktx1 protocol extension (todo)
#define MULTICAST_MVD_HIDDEN			7	// Insert into MVD stream only, as dem_multiple(0)

// health flags
#define H_ROTTEN						(1)
#define H_MEGA							(2)

// kombat flags
#define KF_KTSOUNDS						(   1)			// use KTSounds
#define KF_SCREEN						(   2)			// use autoscreenshot
//										(   4)			use cfgmap
//										(   8)			female gender if it is set, if not - male
//										(  16)			based on server type called alias (see below)
#define KF_ON_ENTER						(  32)			// don't call on_enter event (every map change)
#define KF_SPEED						(  64)			// show speed in prewar
//										( 128)			if it is set mod will call menu_in.cfg
//										( 256)			if it is set menu_out.cfg will be not called
//										( 512)			use dmm related map cfg
//										(1024)			do not use stuff sounds

// events
#define EV_ON_CONNECT					(   1)			//  connect
//										(   2)			weapon switch
#define EV_ON_MATCH_START				(   4)			// match start
#define EV_ON_MATCH_END					(   8)			// match end
#define EV_ON_MATCH_BREAK				(  16)			// match break
//										(  32)			map change
//										(  64)			deathmatch mode change
#define EV_ON_ADMIN						( 128)			// admin
#define EV_ON_UNADMIN					( 256)			// unadmin
#define EV_ON_COUNTDOWN					( 512)			// countdown end

// CTF
#define CTF_RUNE_RES					1			// IT_SIGIL1
#define CTF_RUNE_STR					2			// IT_SIGIL2
#define CTF_RUNE_HST					4			// IT_SIGIL3
#define CTF_RUNE_RGN					8			// IT_SIGIL4
#define CTF_RUNE_MASK					15
#define CTF_FLAG						16

#define FLAG_AT_BASE					0
#define FLAG_CARRIED					1
#define FLAG_DROPPED					2
#define FLAG_RETURNED					3			// here only 200ms before going back to FLAG_AT_BASE

// spec moreinfo
#define MI_ON							(   1<<0)	// on/off
#define MI_ADM_ONLY						(   1<<1)	// send info to admined specs only or to normal specs too

// sv_specprint stuff
#define SPECPRINT_CENTERPRINT			(0x1)
#define SPECPRINT_SPRINT				(0x2)
#define SPECPRINT_STUFFCMD				(0x4)

// mmode stuff
#define MMODE_NONE						(0)
#define MMODE_PLAYER					(1)
#define MMODE_TEAM						(2)
#define MMODE_MULTI						(3)
#define MMODE_RCON						(4)
#define MMODE_NAME						(5)

// rocket arena
#define PLAYERSTATTIME					(0.5f)
#define MAXIDLETIME						(300)		//5 minutes

// k_sudden_death types
#define SD_NORMAL						(1)
#define SD_TIEBREAK						(2)

//
#define CALLALIAS_SIZE					(128)

#define F_CHECK_SIZE					(1024*10)	/* FIXME: which size is reason able */

#define TEAM_LOCATION_UPDATE_TIME		(1.0)		/* 1 second */

#define DMM4_INVINCIBLE_DEFAULT			(2.0)
#define DMM4_INVINCIBLE_MAX				(30.0)

// 
#define MAX_STUFFED_ALIASES_PER_FRAME	(15)

#define MAX_STUFFED_QUICKMAPS_PER_FRAME	(32)

enum
{
	mvdhidden_antilag_position = 0x0000,		// mvdhidden_antilag_position_header_t mvdhidden_antilag_position_t*
	mvdhidden_usercmd = 0x0001,					// <byte: playernum> <byte:dropnum> <byte: msec, vec3_t: angles, short[3]: forward side up> <byte: buttons> <byte: impulse>
	mvdhidden_usercmd_weapons = 0x0002,			// <byte: source playernum> <int: items> <byte[4]: ammo> <byte: result> <byte*: weapon priority (nul terminated)>
	mvdhidden_demoinfo = 0x0003,				// <short: block#> <byte[] content>
	mvdhidden_commentary_track = 0x0004,		// <byte: track#> [todo... <byte: audioformat> <string: short-name> <string: author(s)> <float: start-offset>?]
	mvdhidden_commentary_data = 0x0005,			// <byte: track#> [todo... format-specific]
	mvdhidden_commentary_text_segment = 0x0006,	// <byte: track#> [todo... <float: duration> <string: text (utf8)>]
	mvdhidden_dmgdone = 0x0007,					// <byte: damaging ent#> <byte: damaged ent#> <byte: damage>
	mvdhidden_usercmd_weapons_ss = 0x0008,		// (same format as mvdhidden_usercmd_weapons)
	mvdhidden_extended = 0xFFFF					// doubt we'll ever get here: read next short...
};

#define SV_EXTENSIONS_KTXEXTENSION1		1
#define SV_EXTENSIONS_MVDHIDDEN			2

#define MVDHIDDEN_DMGDONE_SPLASHDAMAGE	(1 << 15)

#define CLIENT_NAME_LEN					32		// Maximum client name, same value as in server.
