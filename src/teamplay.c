
// Teamplay.c
// Format & logic of messages based on ezQuake's built-in team messages

// Future development:
// - Allow (based on fpd) automatic reporting server-side
// - If client supports it, send raw stats to client and allow it to construct format of message

#include "g_local.h"
#include "fb_globals.h"

// Time before we forget item (ezquake allows this to be specified)
#define TOOK_TIMEOUT     5
#define POINT_TIMEOUT    5

#define TP_THRESHOLD_NEED_RA          120
#define TP_THRESHOLD_NEED_YA           80
#define TP_THRESHOLD_NEED_GA           60
#define TP_THRESHOLD_NEED_HEALTH       50
#define TP_THRESHOLD_NEED_WEAPON       87
#define TP_THRESHOLD_NEED_RL            1
#define TP_THRESHOLD_NEED_CELLS        13
#define TP_THRESHOLD_NEED_ROCKETS       5
#define TP_THRESHOLD_NEED_NAILS         0
#define TP_THRESHOLD_NEED_SHELLS        0

#define TP_NAME_SG         "sg"
#define TP_NAME_SSG        "ssg"
#define TP_NAME_NG         "ng"
#define TP_NAME_SNG        "sng"
#define TP_NAME_GL         "gl"
#define TP_NAME_RLG        "{&cf13rl&cfff}{&c2aag&cfff}"
#define TP_NAME_RL         "{&cf13rl&cfff}"
#define TP_NAME_LG         "{&c2aalg&cfff}"
#define TP_NAME_SHELLS     "shells"
#define TP_NAME_NAILS      "nails"
#define TP_NAME_ROCKETS    "rox"
#define TP_NAME_CELLS      "cells"
#define TP_NAME_RL_PACK    "{&cf13rl pack&cfff}"
#define TP_NAME_LG_PACK    "{&c2aalg pack&cfff}"
#define TP_NAME_BACKPACK   "pack"
#define TP_NAME_MEGA       "{&c0a0mega&cfff}"
#define TP_NAME_RA         "{&cf00ra&cfff}"
#define TP_NAME_YA         "{&cff0ya&cfff}"
#define TP_NAME_GA         "{&c0b0ga&cfff}"
#define TP_NAME_QUAD       "{&c05fquad&cfff}"
#define TP_NAME_PENT       "{&cf00pent&cfff}"
#define TP_NAME_RING       "{&cff0ring&cfff}"
#define TP_NAME_SUIT       "suit"
#define TP_NAME_FLAG       "flag"
#define TP_NAME_RUNE1      "{&c0f0resistance&cfff}"
#define TP_NAME_RUNE2      "{&cf00strength&cfff}"
#define TP_NAME_RUNE3      "{&cff0haste&cfff}"
#define TP_NAME_RUNE4      "{&c0ffregeneration&cfff}"
#define TP_NAME_ARMOR      "armor"
#define TP_SEPARATOR       "/"
#define TP_NAME_HEALTH     "health"
#define TP_NAME_AMMO       "ammo"
#define TP_NAME_WEAPON     "weapon"

#define TP_NAME_QUADDED    "{&c05fquaded&cfff}"
#define TP_NAME_PENTED     "{&cf00pented&cfff}"
#define TP_NAME_EYES       "{&cff0eyes&cfff}"
#define TP_NAME_ENEMY      "{&cf00enemy&cfff}"

#define TP_NAME_SOMEPLACE  "someplace"
#define TP_NAME_HASQUAD    "{&c05fquaded&cfff}"
#define TP_NAME_HADPENT    "{&cf00pented&cfff}"

char* LocationName (float x, float y, float z);
static void TeamplayQuadDead (gedict_t* client);
static void TeamplayAreaLost (gedict_t* client);
static void TeamplayAreaSecure (gedict_t* client);
static void TeamplayAreaHelp (gedict_t* client);

unsigned long item_flags[] = {
	it_rl, it_lg, it_gl, it_sng, it_pack,
	it_cells, it_rockets, it_mh, it_ra, it_ya,
	it_ga, it_flag, it_rune1, it_rune2, it_rune3,
	it_rune4, it_quad, it_pent, it_ring
};
char* item_names[] = {
	TP_NAME_RL, TP_NAME_LG, TP_NAME_GL, TP_NAME_SNG, TP_NAME_BACKPACK,
	TP_NAME_CELLS, TP_NAME_ROCKETS, TP_NAME_MEGA, TP_NAME_RA, TP_NAME_YA,
	TP_NAME_GA, TP_NAME_FLAG, TP_NAME_RUNE1, TP_NAME_RUNE2, TP_NAME_RUNE3,
	TP_NAME_RUNE4, TP_NAME_QUAD, TP_NAME_PENT, TP_NAME_RING
};

void TeamplayEventItemTaken (gedict_t* client, gedict_t* item)
{
	char* took_flags_s = ezinfokey(client, "tptook");
	unsigned long took_flags = ~0;
	unsigned long items = (unsigned long)client->s.v.items;
	if (!strnull (took_flags_s))
		took_flags = strtoul (took_flags_s, NULL, 10);
	if (took_flags == 0)
		took_flags = ~0;

	if (!(took_flags & item->tp_flags))
		return;

	client->tp.took.flags = 0;
	if (streq (item->s.v.classname, "weapon_lightning")) {
		client->tp.took.item = it_lg;
	}
	else if (streq (item->s.v.classname, "weapon_rocketlauncher")) {
		client->tp.took.item = it_rl;
	}
	else if (streq (item->s.v.classname, "weapon_grenadelauncher")) {
		client->tp.took.item = it_gl;
	}
	else if (streq (item->s.v.classname, "weapon_supernailgun")) {
		client->tp.took.item = it_sng;
	}
	else if (streq (item->s.v.classname, "weapon_nailgun")) {
		client->tp.took.item = it_ng;
	}
	else if (streq (item->s.v.classname, "weapon_supershotgun")) {
		client->tp.took.item = it_ssg;
	}
	else if (streq (item->s.v.classname, "item_armorInv")) {
		client->tp.took.item = it_ra;
	}
	else if (streq (item->s.v.classname, "item_armor2")) {
		client->tp.took.item = it_ya;
	}
	else if (streq (item->s.v.classname, "item_armor1")) {
		client->tp.took.item = it_ga;
	}
	else if (streq (item->s.v.classname, "item_artifact_envirosuit")) {
		client->tp.took.item = it_suit;
	}
	else if (streq (item->s.v.classname, "item_artifact_invulnerability")) {
		client->tp.took.item = it_pent;
	}
	else if (streq (item->s.v.classname, "item_artifact_invisibility")) {
		client->tp.took.item = it_ring;
	}
	else if (streq (item->s.v.classname, "item_artifact_super_damage")) {
		client->tp.took.item = it_quad;
	}
	else if (streq (item->s.v.classname, "backpack")) {
		client->tp.took.item = it_pack;
		client->tp.took.flags = item->s.v.items;
	}
	else if (streq (item->s.v.classname, "item_shells")) {
		client->tp.took.item = it_shells;
	}
	else if (streq (item->s.v.classname, "item_spikes")) {
		client->tp.took.item = it_nails;
	}
	else if (streq (item->s.v.classname, "item_rockets")) {
		client->tp.took.item = it_rockets;
	}
	else if (streq (item->s.v.classname, "item_cells")) {
		client->tp.took.item = it_cells;
	}
	else if (streq (item->s.v.classname, "item_health")) {
		client->tp.took.item = (item->healamount >= 100 ? it_mh : it_health);
	}
	else {
		return;
	}

	VectorCopy (item->s.v.origin, client->tp.took.location);
	client->tp.took.time = g_globalvars.time;
}

static qbool TookEmpty (gedict_t* client)
{
	return client->tp.took.time == 0 || client->tp.took.time < g_globalvars.time - TOOK_TIMEOUT;
}

static qbool Took (gedict_t* client, unsigned long flag)
{
	return !TookEmpty (client) && client->tp.took.item == flag;
}

static qbool TookSpecific (gedict_t* client, unsigned long flag, unsigned long specific)
{
	return Took (client, flag) && client->tp.took.flags == specific;
}

static qbool NEED (unsigned long player_flags, unsigned long flags)
{
	return player_flags & flags;
}

static qbool HAVE_POWERUP (gedict_t* client)
{
	return client && (int)client->s.v.items & (IT_QUAD | IT_INVULNERABILITY | IT_INVISIBILITY);
}

static qbool HAVE_RING (gedict_t* client)
{
	return client && (int)client->s.v.items & IT_INVISIBILITY;
}

static qbool HAVE_QUAD (gedict_t* client)
{
	return client && (int)client->s.v.items & IT_QUAD;
}

static qbool HAVE_PENT (gedict_t* client)
{
	return client && (int)client->s.v.items & IT_INVULNERABILITY;
}

static qbool HAVE_GA (gedict_t* client)
{
	return client && (int)client->s.v.items & IT_ARMOR1;
}

static qbool HAVE_YA (gedict_t* client)
{
	return (int)client->s.v.items & IT_ARMOR2;
}

static qbool HAVE_RA (gedict_t* client)
{
	return (int)client->s.v.items & IT_ARMOR3;
}

static qbool HAVE_RL (gedict_t* client)
{
	return (int)client->s.v.items & IT_ROCKET_LAUNCHER;
}

static qbool HAVE_LG (gedict_t* client)
{
	return (int)client->s.v.items & IT_LIGHTNING;
}

static qbool HAVE_SNG (gedict_t* client)
{
	return (int)client->s.v.items & IT_SUPER_NAILGUN;
}

static qbool HAVE_NG (gedict_t* client)
{
	return (int)client->s.v.items & IT_NAILGUN;
}

static qbool HAVE_GL (gedict_t* client)
{
	return (int)client->s.v.items & IT_GRENADE_LAUNCHER;
}

static qbool HAVE_SSG (gedict_t* client)
{
	return (int)client->s.v.items & IT_SUPER_SHOTGUN;
}

typedef struct item_vis_s {
	vec3_t	  vieworg;
	vec3_t	  forward;
	vec3_t	  right;
	vec3_t	  up;
	vec3_t	  entorg;
	float	  radius;
	vec3_t	  dir;
	float	  dist;
	gedict_t* viewent;
} item_vis_t;

// TODO: Do not support tp_pointpriorities at the moment
static float TeamplayRankPoint(item_vis_t *visitem)
{
	vec3_t v2, v3;
	float miss;

	if (visitem->dist < 10) {
		return -1;
	}

	VectorScale (visitem->forward, visitem->dist, v2);
	VectorSubtract (v2, visitem->dir, v3);
	miss = VectorLength (v3);
	if (miss > 300) {
		return -1;
	}
	if (miss > visitem->dist * 1.7) {
		return -1;		// over 60 degrees off
	}

	if (visitem->dist < 3000.0 / 8.0) {
		return miss * (visitem->dist * 8.0 * 0.0002f + 0.3f);
	}
	else {
		return miss;
	}
}

static qbool TP_IsItemVisible(item_vis_t *visitem)
{
	vec3_t end, v;

	if (visitem->dist <= visitem->radius)
		return true;

	VectorScale (visitem->dir, -1, v);
	VectorNormalize (v);
	VectorMA (visitem->entorg, visitem->radius, v, end);
	traceline (PASSVEC3 (visitem->vieworg), PASSVEC3 (end), 0, visitem->viewent);
	if (g_globalvars.trace_fraction == 1)
		return true;

	VectorMA (visitem->entorg, visitem->radius, visitem->right, end);
	VectorSubtract (visitem->vieworg, end, v);
	VectorNormalize (v);
	VectorMA (end, visitem->radius, v, end);
	traceline (PASSVEC3 (visitem->vieworg), PASSVEC3 (end), 0, visitem->viewent);
	if (g_globalvars.trace_fraction == 1)
		return true;

	VectorMA(visitem->entorg, -visitem->radius, visitem->right, end);
	VectorSubtract(visitem->vieworg, end, v);
	VectorNormalize(v);
	VectorMA(end, visitem->radius, v, end);
	traceline (PASSVEC3 (visitem->vieworg), PASSVEC3 (end), 0, visitem->viewent);
	if (g_globalvars.trace_fraction == 1)
		return true;

	VectorMA(visitem->entorg, visitem->radius, visitem->up, end);
	VectorSubtract(visitem->vieworg, end, v);
	VectorNormalize(v);
	VectorMA (end, visitem->radius, v, end);
	traceline (PASSVEC3 (visitem->vieworg), PASSVEC3 (end), 0, visitem->viewent);
	if (g_globalvars.trace_fraction == 1)
		return true;

	// use half the radius, otherwise it's possible to see through floor in some places
	VectorMA(visitem->entorg, -visitem->radius / 2, visitem->up, end);
	VectorSubtract(visitem->vieworg, end, v);
	VectorNormalize(v);
	VectorMA(end, visitem->radius, v, end);
	traceline (PASSVEC3 (visitem->vieworg), PASSVEC3 (end), 0, visitem->viewent);
	if (g_globalvars.trace_fraction == 1)
		return true;

	return false;
}

unsigned int ClientFlag (gedict_t* client)
{
	return (unsigned int)1 << (NUM_FOR_EDICT (client) - 1);
}

static gedict_t* TeamplayFindPoint (gedict_t* client)
{
	gedict_t* e = world;
	unsigned long pointflags = ~0;
	vec3_t ang;
	item_vis_t visitem;
	unsigned int clientflag = ClientFlag (client);
	float best = -1;
	gedict_t* bestent = NULL;

	if (deathmatch >= 1 && deathmatch <= 4) {
		if (deathmatch == 4)
			pointflags &= ~it_ammo;
		if (deathmatch != 1)
			pointflags &= ~it_weapons;
	}

	VectorCopy (client->s.v.v_angle, ang);
	ang[2] = 0;
	AngleVectors(ang, visitem.forward, visitem.right, visitem.up);
	VectorCopy (client->s.v.origin, visitem.vieworg);
	visitem.viewent = client;
	VectorAdd (visitem.vieworg, client->s.v.view_ofs, visitem.vieworg);   // FIXME: v_viewheight not taken into account

	for (e = world; e = nextent (e); ) {
		vec3_t size;
		float rank;

		if (e->ct == ctPlayer && !ISLIVE (e))
			continue;
		if (strnull (e->s.v.model))
			continue;
		if (!(e->visclients & clientflag))
			continue;
		if (e->ct != ctPlayer && !(e->tp_flags & pointflags))
			continue;

		VectorSubtract (e->s.v.absmax, e->s.v.absmin, size);
		if (e->ct == ctPlayer) {
			VectorAdd (e->s.v.origin, e->s.v.view_ofs, visitem.entorg)
		}
		else {
			VectorCopy (e->s.v.origin, visitem.entorg);
			visitem.entorg[2] += size[2] / 2;
		}
		VectorSubtract (visitem.entorg, visitem.vieworg, visitem.dir);
		visitem.dist = DotProduct (visitem.dir, visitem.forward);
		visitem.radius = (int)e->s.v.effects & (EF_BLUE|EF_RED|EF_DIMLIGHT|EF_BRIGHTLIGHT) ? 200 : max(max(size[0] / 2, size[1] / 2), size[2] / 2);

		if ((rank = TeamplayRankPoint(&visitem)) < 0)
			continue;

		// check if we can actually see the object (TODO: with pointflags, player detection is different)
		if ((rank < best || best < 0) && TP_IsItemVisible(&visitem)) {
			best = rank;
			bestent = e;
		}
	}

	return bestent;
}

static char* PowerupText (gedict_t* client)
{
	static char buffer[128];

	buffer[0] = '\0';
	if (HAVE_PENT (client))
		strlcat (buffer, TP_NAME_PENT, sizeof (buffer));
	if (HAVE_QUAD (client))
		strlcat (buffer, TP_NAME_QUAD, sizeof (buffer));
	if (HAVE_RING (client))
		strlcat (buffer, TP_NAME_RING, sizeof (buffer));

	return buffer;
}

static void TeamplayMM2 (gedict_t* client, char* text)
{
	extern qbool ClientSay (qbool isTeamSay);
	char buffer[128];
	gedict_t* oldself = self;
	char* name = NULL;

	self = client;
	g_globalvars.self = EDICT_TO_PROG (self);

	strlcpy (buffer, "say_team \"", sizeof (buffer));
	name = ezinfokey(client, "k_nick");
	if ( strnull( name ) )
		name = ezinfokey(client, "k");
	if ( strnull( name ) && client->isBot )
		name = client->s.v.netname;
	if (!strnull (name)) {
		strlcat (buffer, "\r", sizeof (buffer));
		strlcat (buffer, name, sizeof (buffer));
		strlcat (buffer, " ", sizeof (buffer));
	}
	strlcat (buffer, text, sizeof (buffer));
	strlcat (buffer, "\"", sizeof (buffer));
	trap_CmdTokenize (buffer);
	ClientSay (true);
	self = oldself;
	g_globalvars.self = EDICT_TO_PROG (oldself);
}

static char* TeamplayNeedText (unsigned long needFlags)
{
	static char buffer[128];

	buffer[0] = '\0';

	if (needFlags & it_armor) {
		strlcat (buffer, TP_NAME_ARMOR, sizeof (buffer));
	}
	if (needFlags & it_health) {
		if (buffer[0])
			strlcat (buffer, TP_SEPARATOR, sizeof (buffer));
		strlcat (buffer, TP_NAME_HEALTH, sizeof (buffer));
	}
	if (needFlags & it_ammo) {
		if (buffer[0])
			strlcat (buffer, TP_SEPARATOR, sizeof (buffer));
		strlcat (buffer, TP_NAME_AMMO, sizeof (buffer));
	}
	if (needFlags & (it_rl | it_lg)) {
		if (buffer[0])
			strlcat (buffer, TP_SEPARATOR, sizeof (buffer));
		strlcat (buffer, TP_NAME_WEAPON, sizeof (buffer));
	}

	return buffer;
}

static unsigned long TeamplayNeedFlags (gedict_t* client)
{
	unsigned long needflags = 0;
	int items = (int)client->s.v.items;
	const char* need_weapons = ezinfokey(self, "tp_need_weapon");
	int i = 0;

	if (strnull(need_weapons))
		need_weapons = NEED_WEAPONS_DEFAULT;

	if (((items & IT_ARMOR1) && client->s.v.armorvalue < TP_THRESHOLD_NEED_GA) ||
		((items & IT_ARMOR2) && client->s.v.armorvalue < TP_THRESHOLD_NEED_YA) ||
		((items & IT_ARMOR3) && client->s.v.armorvalue < TP_THRESHOLD_NEED_RA) ||
		(!(items & (IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3))) ) {
		needflags |= it_armor;
	}

	if (client->s.v.health < TP_THRESHOLD_NEED_HEALTH) {
		needflags |= it_health;
	}

	// (team fortress logic removed)
	{
		qbool found_weapon = false;
		const char* needammo = NULL;

		while (*need_weapons) {
			int weapon = *need_weapons - '0' - 1;
			if (weapon == 8 && (items & IT_LIGHTNING)) {
				if (client->s.v.ammo_cells < TP_THRESHOLD_NEED_CELLS) {
					needflags |= it_cells;
					needammo = "cells";
					found_weapon = true;
				}
				break;
			}
			else if ((weapon == 7 && (items & IT_ROCKET_LAUNCHER)) ||
				     (weapon == 6 && (items & IT_GRENADE_LAUNCHER))) {
				if (client->s.v.ammo_rockets < TP_THRESHOLD_NEED_ROCKETS) {
					needflags |= it_rockets;
					needammo = "rox";
					found_weapon = true;
				}
			}
			else if ((weapon == 5 && (items & IT_SUPER_NAILGUN)) ||
				     (weapon == 4 && (items & IT_NAILGUN))) {
				if (client->s.v.ammo_nails < TP_THRESHOLD_NEED_NAILS) {
					needflags |= it_nails;
					needammo = "nails";
					found_weapon = true;
				}
			}
			else if ((weapon == 3 && (items & IT_SUPER_SHOTGUN)) ||
				     (weapon == 2 && (items & IT_SHOTGUN))) {
				if (client->s.v.ammo_shells < TP_THRESHOLD_NEED_SHELLS) {
					needflags |= it_shells;
					needammo = "shells";
					found_weapon = true;
				}
			}

			++need_weapons;
		}

		if (needammo) {
			needflags |= it_ammo;
		}
		else if (!found_weapon) {
			needflags |= it_weapons;
		}
		else if (!(items & IT_ROCKET_LAUNCHER)) {
			needflags |= it_rl;
		}
	}

	return needflags;
}

// Cmd_AddCommand ("tp_msgtook", TP_Msg_Took_f);
static void TeamplayReportTaken (gedict_t* client)
{
	char message[128];
	char* at_location = LocationName (PASSVEC3(client->tp.took.location));

	if (TookEmpty(client))
		return;

	message[0] = '\0';
	if (client->tp.took.item & it_powerups) {
		unsigned long needFlags = TeamplayNeedFlags(client);

		if (! ISLIVE(client))	{
			TeamplayQuadDead (client);
			return;
		}
		else if ((NEED (needFlags, it_health) || NEED (needFlags, it_armor) || NEED (needFlags, it_rl) || NEED (needFlags, it_lg) || NEED (needFlags, it_rockets) || NEED (needFlags, it_cells)) && HAVE_POWERUP (client)) {
			// Note that we check if you are holding powerup. This is because TOOK remembers for 15 seconds.
			// So a case could arise where you took quad then died less than 15 seconds later, and you'd be reporting "team need %u" (because $colored_powerups would be empty)
			strlcpy (message, "{&c0b0team&cfff} ", sizeof (message));
			strlcat (message, PowerupText (client), sizeof (message));
			strlcat (message, " need ", sizeof (message));
			strlcat (message, TeamplayNeedText (needFlags), sizeof (message));
		}
		else if (HAVE_QUAD (client) || HAVE_RING (client)) {
			// notice we can't send this check to tp_msgenemypwr, because if enemy with powerup is in your view, tp_enemypwr reports enemypwr first, but in this function you want to report TEAM powerup.
			strlcpy (message, "{&c0b0team&cfff} ", sizeof (message));
			strlcat (message, PowerupText (client), sizeof (message));
		}
		else { // In this case, you took quad or ring and died before 15 secs later. So just report what you need, nothing about powerups.
			strlcat (message, "need ", sizeof (message));
			strlcat (message, TeamplayNeedText (needFlags), sizeof (message));
		}
	}
	else {
		int i = 0;

		if (HAVE_POWERUP (client)) {
			strlcpy (message, PowerupText(client), sizeof (message));
			strlcat (message, " ", sizeof (message));
		}
		if (TookSpecific (client, it_pack, IT_ROCKET_LAUNCHER)) {
			strlcat (message, "took " TP_NAME_RL_PACK, sizeof (message));
		}
		else if (TookSpecific (client, it_pack, IT_LIGHTNING)) {
			strlcat (message, "took " TP_NAME_LG_PACK, sizeof (message));
		}
		else {
			for (i = 0; i < sizeof (item_flags) / sizeof (item_flags[0]); ++i) {
				if (Took (client, item_flags[i])) {
					strlcat (message, "took ", sizeof (message));
					strlcat (message, item_names[i], sizeof (message));
					break;
				}
			}

			if (i > sizeof (item_flags) / sizeof (item_flags[0])) {
				G_bprint (2, "TOOK ITEM FLAG: %u\n", (unsigned)client->tp.took.item);
			}
		}
	}
	strlcat (message, " \20{", sizeof (message));
	strlcat (message, at_location, sizeof (message));
	strlcat (message, "}\21", sizeof (message));

	TeamplayMM2 (client, message);
}

static char* ColoredArmor (gedict_t* client)
{
	if (HAVE_GA (client))
		return va ("{&c0b0%d&cfff}", (int)client->s.v.armorvalue);
	if (HAVE_GA (client))
		return va ("{&c0b0%d&cfff}", (int)client->s.v.armorvalue);
	if (HAVE_GA (client))
		return va ("{&c0b0%d&cfff}", (int)client->s.v.armorvalue);
	return "0";
}

// Cmd_AddCommand ("tp_msgreport", TP_Msg_Report_f);
static void TeamplayReportPersonalStatus (gedict_t* client)
{
	const char* powerup = "";
	const char* rl = "";
	const char* lg = "";
	const char* weapon = "";
	char buffer[128];
	int ammo = -1;

	buffer[0] = '\0';

	if (!ISLIVE (client))
	{
		TeamplayAreaLost (client);
		return;
	}

	if (HAVE_POWERUP (client)) {
		strlcpy (buffer, PowerupText (client), sizeof (buffer));
	}

	strlcat (buffer, va("%s/%d ", ColoredArmor (client), (int)max(0, client->s.v.health)), sizeof (buffer));
	if (HAVE_RL (client) && HAVE_LG (client)) {
		strlcat (buffer, va ("%s:%d ", TP_NAME_RL, (int)client->s.v.ammo_rockets), sizeof (buffer));
		strlcat (buffer, va ("%s:%d ", TP_NAME_LG, (int)client->s.v.ammo_cells), sizeof (buffer));
	}
	else if (HAVE_RL (client)) {
		strlcat (buffer, va ("%s:%d ", TP_NAME_RL, (int)client->s.v.ammo_rockets), sizeof (buffer));
	}
	else if (HAVE_LG (client)) {
		strlcat (buffer, va ("%s:%d ", TP_NAME_LG, (int)client->s.v.ammo_cells), sizeof (buffer));
	}
	else if (HAVE_GL (client)) {
		strlcat (buffer, va ("%s:%d ", TP_NAME_GL, (int)client->s.v.ammo_rockets), sizeof (buffer));
	}
	else if (HAVE_SNG (client)) {
		strlcat (buffer, va ("%s:%d ", TP_NAME_SNG, (int)client->s.v.ammo_nails), sizeof (buffer));
	}
	else if (HAVE_SSG(client)) {
		strlcat (buffer, va ("%s:%d ", TP_NAME_SSG, (int)client->s.v.ammo_shells), sizeof (buffer));
	}

	strlcat (buffer, "\20{", sizeof (buffer));
	strlcat (buffer, LocationName(PASSVEC3(client->s.v.origin)), sizeof (buffer));
	strlcat (buffer, "}\21", sizeof (buffer));
	
	if (!HAVE_RL (client) && client->s.v.ammo_rockets) {
		strlcat (buffer, va (" {&cf13r&cfff}:%d", (int)client->s.v.ammo_rockets), sizeof (buffer));
	}
	if (!HAVE_LG (client) && client->s.v.ammo_cells) {
		strlcat (buffer, va (" {&c2aac&cfff}:%d", (int)client->s.v.ammo_cells), sizeof (buffer));
	}

	TeamplayMM2(client, buffer);
}

// Cmd_AddCommand ("tp_msgsafe", TP_Msg_Safe_f);
static void TeamplayAreaSecure (gedict_t* client)
{
	qbool have_armor = (int)client->s.v.items & IT_ARMOR;
	qbool have_weapon = (int)client->s.v.items & (IT_ROCKET_LAUNCHER | IT_LIGHTNING);
	char buffer[128];

	buffer[0] = '\0';
	if (!ISLIVE (client)) {
		return;
	}

	// TODO: if pointing at enemy, degrade to TP_EnemyPowerup or no-op

	strlcpy (buffer, "{&c0b0safe&cfff} {&c0b0[&cfff}{", sizeof (buffer));
	strlcat (buffer, LocationName (PASSVEC3(client->s.v.origin)), sizeof (buffer));
	strlcat (buffer, "}{&c0b0]&cfff} ", sizeof (buffer));
	if (have_armor) {
		strlcat (buffer, ColoredArmor (client), sizeof (buffer));
	}
	if (have_armor && have_weapon) {
		strlcat (buffer, " ", sizeof (buffer));
	}
	if (HAVE_RL (client) && HAVE_LG (client)) {
		strlcat (buffer, TP_NAME_RLG, sizeof (buffer));
	}
	else if (HAVE_RL (client)) {
		strlcat (buffer, TP_NAME_RL, sizeof (buffer));
	}
	else if (HAVE_LG (client)) {
		strlcat (buffer, TP_NAME_LG, sizeof (buffer));
	}

	TeamplayMM2(client, buffer);
}

// Cmd_AddCommand ("tp_msghelp", TP_Msg_Help_f);
static void TeamplayAreaHelp (gedict_t* client)
{
	qbool have_armor = (int)client->s.v.items & IT_ARMOR;
	qbool have_weapon = (int)client->s.v.items & (IT_ROCKET_LAUNCHER | IT_LIGHTNING);
	char buffer[128];

	buffer[0] = '\0';
	strlcpy (buffer, PowerupText (client), sizeof (buffer));
	strlcat (buffer, "{&cff0help&cfff} {&cff0[&cfff}{", sizeof (buffer));
	strlcat (buffer, LocationName (PASSVEC3(client->s.v.origin)), sizeof (buffer));
	strlcat (buffer, va("}{&cff0]&cfff} {%d}", client->tp.enemy_count), sizeof (buffer));

	TeamplayMM2(client, buffer);
}

// Cmd_AddCommand ("tp_msglost", TP_Msg_Lost_f);
static void TeamplayAreaLost (gedict_t* client)
{
	char buffer[128];

	buffer[0] = '\0';
	if (! ISLIVE(client)) {
		if (client->tp.death_items & IT_QUAD) {
			strlcpy (buffer, TP_NAME_QUAD, sizeof (buffer));
			strlcat (buffer, " over ", sizeof (buffer));
		}
		else {
			strlcpy (buffer, "{&cf00lost&cfff} ", sizeof (buffer));
		}

		if (client->tp.death_weapon == IT_ROCKET_LAUNCHER) {
			strlcat (buffer, "{&cf00DROPPED} " TP_NAME_RL " ", sizeof (buffer));
		}
		else if (client->tp.death_weapon == IT_LIGHTNING) {
			strlcat (buffer, "{&cf00DROPPED} " TP_NAME_LG " ", sizeof (buffer));
		}
	}
	else {
		strlcpy (buffer, "{&cf00lost&cfff} ", sizeof (buffer));
	}

	strlcat (buffer, "{&cf00[&cfff}{", sizeof (buffer));
	if (client->tp.death_time > 0) {
		strlcat (buffer, LocationName (PASSVEC3 (client->tp.death_location)), sizeof (buffer));
	}
	else {
		strlcat (buffer, TP_NAME_SOMEPLACE, sizeof (buffer));
	}
	strlcat (buffer, va("}{&cf00]&cfff} {%d}", client->tp.enemy_count), sizeof (buffer));
	TeamplayMM2(client, buffer);
}

// Cmd_AddCommand ("tp_msgkillme", TP_Msg_KillMe_f);
static void TeamplayKillMe (gedict_t* client)
{
	char buffer[128];
	gedict_t* point = NULL;

	if (!ISLIVE (client))
		return;

	buffer[0] = '\0';
	point = TeamplayFindPoint (client);
	if (point && point->ct == ctPlayer && SameTeam(client, point)) {
		strlcpy (buffer, "{&c0b0", sizeof (buffer));
		strlcat (buffer, point->s.v.netname, sizeof (buffer));
		strlcat (buffer, "&cfff} ", sizeof (buffer));
	}
	strlcat (buffer, "{&cb1akill me [&cfff}{", sizeof (buffer));
	strlcat (buffer, LocationName(PASSVEC3(client->s.v.origin)), sizeof (buffer));
	strlcat (buffer, "}{&cf2a]&cfff} ", sizeof (buffer));

	if ((int)client->s.v.weapon & IT_ROCKET_LAUNCHER) {
		strlcat (buffer, TP_NAME_RL, sizeof (buffer));
		strlcat (buffer, va(":%d ", (int)client->s.v.ammo_rockets), sizeof (buffer));
	}
	else if ((int)client->s.v.weapon & IT_LIGHTNING) {
		strlcat (buffer, TP_NAME_LG, sizeof (buffer));
		strlcat (buffer, va(":%d ", (int)client->s.v.ammo_cells), sizeof (buffer));
	}

	if (!HAVE_RL (client) && client->s.v.ammo_rockets > 0) {
		strlcat (buffer, va("{&cf13r&cfff}:%d ", (int)client->s.v.ammo_rockets), sizeof (buffer));
	}
	if (!HAVE_LG (client) && client->s.v.ammo_cells > 0) {
		strlcat (buffer, va("{&c2aac&cfff}:%d", (int)client->s.v.ammo_cells), sizeof (buffer));
	}

	TeamplayMM2 (client, buffer);
}

// Cmd_AddCommand ("tp_msgutake", TP_Msg_YouTake_f);
static void TeamplayYouTake (gedict_t* client)
{
	char buffer[128];
	gedict_t* point = NULL;

	buffer[0] = '\0';
	point = TeamplayFindPoint (client);
	if (point && point->ct == ctPlayer && SameTeam(client, point)) {
		strlcpy (buffer, "{&c0b0", sizeof (buffer));
		strlcpy (buffer, point->s.v.netname, sizeof (buffer));
		strlcpy (buffer, "&cfff} take \20{", sizeof (buffer));
	}
	else {
		strlcpy (buffer, "you take \20{", sizeof (buffer));
	}

	strlcat (buffer, LocationName (PASSVEC3(client->s.v.origin)), sizeof (buffer));
	strlcat (buffer, "}\21", sizeof (buffer));
}

// Cmd_AddCommand ("tp_msgneed", TP_Msg_Need_f);
static void TeamplayReportNeeds (gedict_t* client)
{
	unsigned long needFlags;
	char buffer[128];

	if (!ISLIVE (client))
		return;

	needFlags = TeamplayNeedFlags (client);

	buffer[0] = '\0';
	if (NEED (needFlags, it_health) || NEED (needFlags, it_armor) || 
		NEED (needFlags, it_rl) || NEED (needFlags, it_lg) ||
		NEED (needFlags, it_rockets) || NEED (needFlags, it_cells) || 
		NEED (needFlags, it_shells) || NEED (needFlags, it_nails)) {

		if (HAVE_POWERUP (client)) {
			strlcpy (buffer, "{&c0b0team&cfff} ", sizeof (buffer));
			strlcat (buffer, PowerupText (client), sizeof (buffer));
			strlcat (buffer, " ", sizeof (buffer));
		}

		strlcat (buffer, "need ", sizeof (buffer));
		strlcat (buffer, TeamplayNeedText(needFlags), sizeof (buffer));
		strlcat (buffer, " \20{", sizeof (buffer));
		strlcat (buffer, LocationName(PASSVEC3(client->s.v.origin)), sizeof (buffer));
		strlcat (buffer, "}\21", sizeof (buffer));

		TeamplayMM2 (client, buffer);
	}
}

static char* TeamplayLastEnemyPowerupText (gedict_t* client, qbool* location_included)
{
	static char buffer[128];

	buffer[0] = '\0';
	// whatever last seen powerup was (or quad if not)
	if (client->tp.enemy_items == 0 || g_globalvars.time - client->tp.enemy_itemtime > 5) {
		// too long ago, assume quad
		strlcat (buffer, TP_NAME_QUAD, sizeof (buffer));
		*location_included = false;
	}
	else {
		qbool eyes = client->tp.enemy_items & IT_INVISIBILITY;
		qbool quaded = client->tp.enemy_items & IT_QUAD;
		qbool pented = client->tp.enemy_items & IT_INVULNERABILITY;

		if (quaded && pented)
			strlcat (buffer, TP_NAME_QUAD " " TP_NAME_PENT, sizeof (buffer));
		else if (quaded)
			strlcat (buffer, TP_NAME_QUAD, sizeof (buffer));
		else if (pented)
			strlcat (buffer, TP_NAME_PENT, sizeof (buffer));

		if (eyes && (quaded || pented))
			strlcat (buffer, " " TP_NAME_RING, sizeof (buffer));
		else if (eyes)
			strlcat (buffer, TP_NAME_RING, sizeof (buffer));

		strlcat (buffer, " \20{", sizeof (buffer));
		strlcat (buffer, LocationName (PASSVEC3(client->tp.enemy_location)), sizeof (buffer));
		strlcat (buffer, "}\21", sizeof (buffer));
		*location_included = true;
	}

	return buffer;
}

// Cmd_AddCommand ("tp_msgenemypwr", TP_Msg_EnemyPowerup_f);
static void TeamplayEnemyPowerup (gedict_t* client)
{
	gedict_t* point = TeamplayFindPoint (client);
	qbool quaded = HAVE_QUAD (point);
	qbool pented = HAVE_PENT (point);
	qbool suppressLocation = false;
	char buffer[128];

	buffer[0] = '\0';
	if (HAVE_RING (point)) {
		if (quaded && pented) {
			strlcpy (buffer, TP_NAME_QUADDED " " TP_NAME_PENTED " " TP_NAME_EYES, sizeof (buffer));
		}
		else if (quaded) {
			strlcpy (buffer, TP_NAME_QUADDED " " TP_NAME_EYES, sizeof (buffer));
		}
		else if (pented) {
			strlcpy (buffer, TP_NAME_PENTED " " TP_NAME_EYES, sizeof (buffer));
		}
		else {
			strlcpy (buffer, TP_NAME_EYES, sizeof (buffer));
		}
	}
	else if (point && point->ct == ctPlayer && !SameTeam (client, point)) {
		strlcpy (buffer, TP_NAME_ENEMY, sizeof (buffer));
		strlcat (buffer, " ", sizeof (buffer));

		if (quaded && pented) {
			strlcat (buffer, TP_NAME_QUAD " " TP_NAME_PENT " ", sizeof (buffer));
		}
		else if (quaded) {
			strlcat (buffer, TP_NAME_QUAD " ", sizeof (buffer));
		}
		else if (pented) {
			strlcat (buffer, TP_NAME_PENT " ", sizeof (buffer));
		}
		else {
			strlcat (buffer, TeamplayLastEnemyPowerupText (client, &suppressLocation), sizeof (buffer));
		}
	}
	else if (HAVE_POWERUP (client)) {
		unsigned long needFlags = 0;

		if (!ISLIVE (client)) {
			TeamplayAreaLost (client);
			return;
		}

		needFlags = TeamplayNeedFlags (client);
		if (NEED (needFlags, it_health) || NEED (needFlags, it_armor) ||
			NEED (needFlags, it_rockets) || NEED (needFlags, it_cells) ||
			NEED (needFlags, it_rl) || NEED (needFlags, it_lg)) {
			TeamplayReportNeeds (client);
			return;
		}

		strlcpy (buffer, "{&c0b0team&cfff} ", sizeof (buffer));
		strlcat (buffer, PowerupText (client), sizeof (buffer));
		strlcat (buffer, " ", sizeof (buffer));
	}
	else if (point && point->ct == ctPlayer && SameTeam(point, client)) {
		quaded = HAVE_QUAD (point);
		pented = HAVE_PENT (point);

		if (quaded && pented) {
			strlcpy (buffer, "{&c0b0team&cfff} " TP_NAME_QUAD " " TP_NAME_PENT, sizeof (buffer));
		}
		else if (quaded) {
			strlcpy (buffer, "{&c0b0team&cfff} " TP_NAME_QUAD, sizeof (buffer));
		}
		else if (pented) {
			strlcpy (buffer, "{&c0b0team&cfff} " TP_NAME_PENT, sizeof (buffer));
		}
		else {
			strlcpy (buffer, TP_NAME_ENEMY " ", sizeof (buffer));
			strlcat (buffer, TeamplayLastEnemyPowerupText (client, &suppressLocation), sizeof (buffer));
		}
	}
	else {
		strlcpy (buffer, TP_NAME_ENEMY " ", sizeof (buffer));
		strlcat (buffer, TeamplayLastEnemyPowerupText (client, &suppressLocation), sizeof (buffer));
	}

	if (!suppressLocation) {
		strlcat (buffer, " at \20{", sizeof (buffer));
		if (point) {
			strlcat (buffer, LocationName(PASSVEC3(point->s.v.origin)), sizeof (buffer));
		}
		else {
			strlcat (buffer, LocationName(PASSVEC3(client->s.v.origin)), sizeof (buffer));
		}
		strlcat (buffer, "}\21", sizeof (buffer));
	}

	TeamplayMM2 (client, buffer);
}

static void TeamplaySetEnemyFlags (gedict_t* client)
{
	gedict_t* plr = world;
	unsigned int clientFlag = ClientFlag (client);
	int enemy_items = 0;
	int enemy_count = 0;
	int friend_count = 0;
	for (plr = world; plr = find_plr (plr); ) {
		if (plr == client)
			continue;
		if (!(plr->visclients & clientFlag))
			continue;
		enemy_items |= ((int)plr->s.v.items & (IT_INVISIBILITY));
		if (SameTeam (plr, client)) {
			++friend_count;
		}
		else {
			enemy_items |= ((int)plr->s.v.items & (IT_QUAD | IT_INVULNERABILITY));
			++enemy_count;
		}
	}
	if (enemy_items) {
		client->tp.enemy_items = enemy_items;
		client->tp.enemy_itemtime = g_globalvars.time;
		VectorAdd (client->s.v.origin, client->s.v.view_ofs, client->tp.enemy_location);
	}
	client->tp.enemy_count = enemy_count;
	client->tp.teammate_count = friend_count;
}

void TeamplayGameTick (void)
{
	// Set all enemy powerup flags
	gedict_t* plr = NULL;
	for (plr = world; plr = find_plr (plr); ) {
		TeamplaySetEnemyFlags (plr);
	}
}

// Cmd_AddCommand ("tp_msgquaddead", TP_Msg_QuadDead_f);
static void TeamplayQuadDead (gedict_t* client)
{
	gedict_t* point = NULL;
	
	if (HAVE_QUAD (client) && !ISLIVE (client)) {
		TeamplayAreaLost (client);
		return;
	}
	
	point = TeamplayFindPoint (client);
	if (HAVE_QUAD (point)) {
		TeamplayEnemyPowerup (client);
		return;
	}

	TeamplayMM2 (client, TP_NAME_QUAD " dead/over");
}


// Cmd_AddCommand ("tp_msggetquad", TP_Msg_GetQuad_f);
static void TeamplayGetQuad (gedict_t* client)
{
	gedict_t* point = TeamplayFindPoint (client);

	if (HAVE_RING (point) && HAVE_QUAD (point)) {
		return; // Don't know for sure if it's enemy or not, and can't assume like we do in tp_enemypwr because this isn't tp_ENEMYpwr
	}
	else if (HAVE_QUAD (client) || HAVE_QUAD (point)) {
		TeamplayEnemyPowerup (client);
		return;
	}
	else {
		TeamplayMM2 (client, "get " TP_NAME_QUAD);
	}
}

// Cmd_AddCommand ("tp_msggetpent", TP_Msg_GetPent_f);
static void TeamplayGetPent (gedict_t* client)
{
	gedict_t* point = TeamplayFindPoint (client);

	if (HAVE_RING (point) && HAVE_PENT (point)) {
		return;
	}
	else if (HAVE_PENT (client) || HAVE_PENT (point)) {
		TeamplayEnemyPowerup (client);
		return;
	}
	else {
		TeamplayMM2 (client, "get " TP_NAME_PENT);
	}
}

// GLOBAL void TP_Msg_Point_f (void)
static void TeamplayPoint (gedict_t* client)
{
	char buffer[128];
	gedict_t* point = NULL;
	int point_items = 0;

	buffer[0] = '\0';
	point = TeamplayFindPoint (client);
	if (point == NULL) {
		return;
	}

	point_items = (int)point->s.v.items;
	if (match_in_progress == 2 && (point_items & (IT_INVISIBILITY | IT_QUAD | IT_INVULNERABILITY))) {
		TeamplayEnemyPowerup (client);
		return;
	}

	if (point->ct == ctPlayer) {
		if (!SameTeam (point, client)) {
			strlcat (buffer, va ("{%d} %s at \20{%s}\21", client->tp.enemy_count, TP_NAME_ENEMY, LocationName (PASSVEC3 (point->s.v.origin))), sizeof (buffer));
		}
		else {
			strlcat (buffer, "{&c0b0", sizeof (buffer));
			strlcat (buffer, point->s.v.netname, sizeof (buffer));
			strlcat (buffer, "&cfff}", sizeof (buffer));
		}
	}
	else {
		int i;

		for (i = 0; i < sizeof (item_flags) / sizeof (item_flags[0]); ++i) {
			if (point->tp_flags & item_flags[i]) {
				strlcat (buffer, va ("%s at \20{%s}\21 {%d}", item_names[i], LocationName (PASSVEC3 (point->s.v.origin)), client->tp.enemy_count), sizeof (buffer));
				break;
			}
		}

		if (i >= sizeof (item_flags) / sizeof (item_flags[0]))
			return;
	}

	TeamplayMM2 (client, buffer);
}

static void TeamplayBasicCommand(gedict_t* client, char* text)
{
	char buffer[128];
	buffer[0] = '\0';
	if (HAVE_POWERUP(client)) {
		strlcpy(buffer, PowerupText(client), sizeof(buffer));
		strlcat(buffer, " ", sizeof(buffer));
	}
	strlcat(buffer, text, sizeof(buffer));
	strlcat(buffer, " \20{", sizeof(buffer));
	strlcat(buffer, LocationName(PASSVEC3(client->s.v.origin)), sizeof(buffer));
	strlcat(buffer, "}\21", sizeof(buffer));
	TeamplayMM2(client, buffer);
}

#define TEAMPLAY_BASIC(FunctionName, Text) static void FunctionName(gedict_t* client) { TeamplayBasicCommand(client, Text); }

// Cmd_AddCommand ("tp_msgyesok", TP_Msg_YesOk_f);
TEAMPLAY_BASIC(TeamplayYesOk, "{yes/ok}")
// Cmd_AddCommand ("tp_msgnocancel", TP_Msg_NoCancel_f);
TEAMPLAY_BASIC(TeamplayNoCancel, "{&cf00no/cancel&cfff}")
// Cmd_AddCommand ("tp_msgitemsoon", TP_Msg_ItemSoon_f);
TEAMPLAY_BASIC(TeamplayItemSoon, "item soon")
// Cmd_AddCommand ("tp_msgwaiting", TP_Msg_Waiting_f);
TEAMPLAY_BASIC(TeamplayWaiting, "waiting")
// Cmd_AddCommand ("tp_msgslipped", TP_Msg_Slipped_f);
TEAMPLAY_BASIC(TeamplaySlipped, "enemy slipped")
// Cmd_AddCommand ("tp_msgreplace", TP_Msg_Replace_f);
TEAMPLAY_BASIC(TeamplayReplace, "replace")
// Cmd_AddCommand ("tp_msgtrick", TP_Msg_Trick_f);
TEAMPLAY_BASIC(TeamplayTrick, "trick")
// Cmd_AddCommand ("tp_msgcoming", TP_Msg_Coming_f);
TEAMPLAY_BASIC(TeamplayComing, "coming")

void TeamplayDeathEvent (gedict_t* client)
{
	VectorCopy (client->s.v.origin, client->tp.death_location);
	client->tp.death_items = (int)client->s.v.items;
	client->tp.death_weapon = (int)client->s.v.weapon;
	client->tp.death_time = g_globalvars.time;
}

// Cmd_AddCommand ("tp_msgpoint", TP_Msg_Point_f);

typedef struct location_node_s {
	vec3_t point;
	char name[64];
} location_node_t;

typedef struct locmacro_s {
	char* name;
	char* value;
} locmacro_t;

static locmacro_t locmacros[] = {
	{"ssg", "ssg"},
	{"ng", "ng"},
	{"sng", "sng"},
	{"gl", "gl"},
	{"rl", "rl"},
	{"lg", "lg"},
	{"separator", "-"},
	{"ga", "ga"},
	{"ya", "ya"},
	{"ra", "ra"},
	{"quad", "quad"},
	{"pent", "pent"},
	{"ring", "ring"},
	{"suit", "suit"},
	{"mh", "mega"},
};

static int node_count = 0;
static location_node_t nodes[256];

char* LocationName (float x, float y, float z)
{
	int i = 0;
	int best = -1;
	float best_distance = 0.0f;
	vec3_t point = { x, y, z };

	for (i = 0; i < node_count; ++i) {
		float distance = VectorDistance (point, nodes[i].point);

		if (best < 0 || distance < best_distance) {
			best = i;
			best_distance = distance;
		}
	}

	if (best < 0)
		return TP_NAME_SOMEPLACE;
	return nodes[best].name;
}

void LocationInitialise (void)
{
	fileHandle_t file = -1;
	char* entityFile = cvar_string ("k_entityfile");
	char lineData[128];
	char argument[128];

	if (!strnull (entityFile)) {
		file = std_fropen ("locs/%s.loc", entityFile);
	}
	if (file == -1) {
		file = std_fropen ("locs/%s.loc", g_globalvars.mapname);
	}

	if (file == -1) {
		Com_Printf ("Couldn't load %s.loc\n", g_globalvars.mapname);
		return;
	}

	while (std_fgets (file, lineData, sizeof (lineData))) {
		char x[16], y[16], z[16];
		char* name = nodes[node_count].name;
		int i = 0;

		trap_CmdTokenize (lineData);
		trap_CmdArgv (0, x, sizeof (x));
		trap_CmdArgv (1, y, sizeof (y));
		trap_CmdArgv (2, z, sizeof (z));
		VectorSet (nodes[node_count].point, atof (x) / 8, atof (y) / 8, atof (z) / 8);

		name[0] = '\0';
		for (i = 3; i < trap_CmdArgc (); ++i) {
			trap_CmdArgv (i, argument, sizeof (argument));

			if (i > 3)
				strlcat (name, " ", sizeof (nodes[node_count].name));
			strlcat (name, argument, sizeof (nodes[node_count].name));
		}

		// Replace tokens (don't allow customisation)
		for (i = 0; i < (int) strlen (name); ++i) {
			if (!strncmp (name + i, "$loc_name_", 10)) {
				char* stub = name + i + 10;
				int j;
				qbool found = false;

				for (j = 0; j < sizeof (locmacros) / sizeof (locmacros[0]); ++j) {
					if (!strncmp (stub, locmacros[j].name, strlen (locmacros[j].name))) {
						int old_length = 10 + strlen (locmacros[j].name);
						int new_length = strlen (locmacros[j].value);

						if (new_length < old_length) {
							strncpy (name + i, locmacros[j].value, new_length);
							memmove (name + i + new_length, name + i + old_length, strlen (name + i + old_length) + 1);
							found = true;
							--i;
						}
						break;
					}
				}

				if (!found) {
					i += 10;
				}
			}
		}

		++node_count;
		if (node_count >= sizeof (nodes) / sizeof (nodes[0]))
			break;
	}
	Com_Printf ("Loaded %d locations\n", node_count);

	std_fclose( file );
}

typedef struct teamplay_message_s {
	char* cmdname;
	char* description;
	void (*function)(gedict_t* client);
} teamplay_message_t;

static teamplay_message_t messages[] = {
	{ "yesok", "yes/ok", TeamplayYesOk },
	{ "nocancel", "no/cancel", TeamplayNoCancel },
	{ "soon", "item soon", TeamplayItemSoon },
	{ "waiting", "waiting", TeamplayWaiting },
	{ "slipped", "enemy slipped", TeamplaySlipped },
	{ "replace", "replace me", TeamplayReplace },
	{ "trick", "trick", TeamplayTrick },
	{ "coming", "coming", TeamplayComing },
	{ "getquad", "get quad", TeamplayGetQuad },
	{ "getpent", "get pent", TeamplayGetPent },
	{ "quaddead", "quad dead", TeamplayQuadDead },
	{ "enemypwr", "enemy powerup", TeamplayEnemyPowerup },
	{ "youtake", "you take", TeamplayYouTake },
	{ "kill me", "kill me", TeamplayKillMe },
	{ "lost", "area lost", TeamplayAreaLost },
	{ "secure", "area secure", TeamplayAreaSecure },
	{ "help", "area needs help", TeamplayAreaHelp },
	{ "need", "report needs", TeamplayReportNeeds },
	{ "report", "report status", TeamplayReportPersonalStatus },
	{ "took", "item taken", TeamplayReportTaken },
	{ "point", "player/item point", TeamplayPoint }
};

qbool TeamplayMessageByName (gedict_t* client, const char* message)
{
	int i;
	for (i = 0; i < sizeof (messages) / sizeof (messages[0]); ++i) {
		if (streq (messages[i].cmdname, message)) {
			messages[i].function (client);
			return true;
		}
	}
	return false;
}

void TeamplayMessage (void)
{
	int i;

	if (trap_CmdArgc () == 2) {
		char argument[32];

		trap_CmdArgv (1, argument, sizeof (argument));

		if (TeamplayMessageByName (self, argument)) {
			return;
		}
	}

	// Print usage
	int max_len = 0;
	for (i = 0; i < sizeof (messages) / sizeof (messages[0]); ++i) {
		max_len = max (max_len, strlen(messages[i].cmdname));
	}
	max_len += 2;

	G_sprint (self, 2, "Usage:\n");
	for (i = 0; i < sizeof (messages) / sizeof (messages[0]); ++i) {
		int len = max_len - strlen (messages[i].cmdname);
		G_sprint (self, 2, "  &cff0%s&r %.*s %s\n", messages[i].cmdname, len, "................", messages[i].description);
	}
}

