
#include "g_local.h"
#include "fb_globals.h"

// If multiple items have the same goal, markers on the map will point to the closest
//   So (my logic) if you only care about the closest item rather than timing all items, assign same goal to all objects
//   Statically assign goals for minor items and leave other goal numbers free for items to be tracked across the whole map
#define FB_FIRST_AUTOGOAL  15
#define FB_GOAL_GA         15
#define FB_GOAL_HEALTH     16
#define FB_GOAL_GL         17
#define FB_GOAL_ROCKETS    18

// These were set in the original FBCA
#define FB_GOAL_CELLS      19
#define FB_GOAL_SNG        20
#define FB_GOAL_SSG        21
#define FB_GOAL_NG         22
#define FB_GOAL_SPIKES     23
#define FB_GOAL_SHELLS     24

// This automatically assigns default goal numbers
void AssignGoalNumbers (void)
{
	gedict_t* ent;
	int unassigned_goal = 1;

	for (ent = world; ent = nextent (ent); ) {
		switch (ent->tp_flags) {
		case it_ra:
		case it_ya:
		case it_rl:
		case it_lg:
		case it_mh:
		case it_quad:
		case it_pent:
		case it_suit:
		case it_ring:
			if (unassigned_goal < FB_FIRST_AUTOGOAL) {
				SetGoalForMarker (unassigned_goal, ent);

				++unassigned_goal;
			}
			else {
				G_bprint (PRINT_HIGH, "Unable to assign goal to %s @ [%d %d %d]\n", ent->s.v.classname, PASSINTVEC3 (ent->s.v.origin));
			}
			break;
		case it_ga:
			SetGoalForMarker (FB_GOAL_GA, ent);
			break;
		case it_health:
			SetGoalForMarker (FB_GOAL_HEALTH, ent);
			break;
		case it_ng:
			SetGoalForMarker (FB_GOAL_NG, ent);
			break;
		case it_sng:
			SetGoalForMarker (FB_GOAL_SNG, ent);
			break;
		case it_gl:
			SetGoalForMarker (FB_GOAL_GL, ent);
			break;
		case it_shells:
			SetGoalForMarker (FB_GOAL_SHELLS, ent);
			break;
		case it_nails:
			SetGoalForMarker (FB_GOAL_SPIKES, ent);
			break;
		case it_rockets:
			SetGoalForMarker (FB_GOAL_ROCKETS, ent);
			break;
		case it_cells:
			SetGoalForMarker (FB_GOAL_CELLS, ent);
			break;
		}
	}
}

// Goal functions
void item_megahealth_rot ();

static void LocateDynamicItem (gedict_t* item)
{
	vec3_t point;
	int content = trap_pointcontents (PASSVEC3 (item->s.v.origin));

	VectorAdd (item->s.v.absmin, item->s.v.view_ofs, point);
	item->fb.touch_marker = LocateMarker (point);
	if (item->fb.touch_marker) {
		if ((item->fb.touch_marker->fb.T & UNREACHABLE) || (content == CONTENT_LAVA)) {
			item->fb.touch_marker = NULL;
		}
	}

	if (item->fb.touch_marker) {
		item->fb.Z_ = item->fb.touch_marker->fb.Z_;
	}
}

static float goal_health0 (gedict_t* self)
{
	return self->fb.desire_health0;
}

static float goal_health2 (gedict_t* self)
{
	return self->fb.desire_health2;
}

float goal_NULL (gedict_t* self)
{
	return 0;
}

static float goal_armor1 (gedict_t* self)
{
	return self->fb.desire_armor1;
}

static float goal_armor2 (gedict_t* self)
{
	if (self->fb.desire_armor2) {
		return (self->fb.desire_armor2 + (self->fb.virtual_enemy ? self->fb.virtual_enemy->fb.desire_armor2 : 0));
	}
	else {
		qbool low_armor = (qbool)(self->fb.total_armor <= 100 && self->s.v.health >= 50);
		qbool has_rl = (qbool)(((int)self->s.v.items & IT_ROCKET_LAUNCHER) && self->s.v.ammo_rockets);
		qbool has_quad = (qbool)(self->super_damage_finished <= g_globalvars.time);

		if (low_armor && has_rl && has_quad) {
			return self->fb.virtual_enemy ? self->fb.virtual_enemy->fb.desire_armor2 : 0;
		}
		return 0;
	}
}

static float goal_armorInv (gedict_t* self)
{
	if (self->fb.desire_armorInv) {
		return (self->fb.desire_armorInv + (self->fb.virtual_enemy ? self->fb.virtual_enemy->fb.desire_armorInv : 0));
	}
	else {
		qbool has_rl = (qbool)(((int)self->s.v.items & IT_ROCKET_LAUNCHER) && (self->s.v.ammo_rockets));
		qbool has_quad = (qbool)(self->super_damage_finished <= g_globalvars.time);
		qbool ok_health = (qbool)(self->s.v.health >= 50);

		if (has_rl && has_quad && ok_health) {
			return self->fb.virtual_enemy ? self->fb.virtual_enemy->fb.desire_armorInv : 0;
		}
		return 0;
	}
}

static float goal_supershotgun1 (gedict_t* self)
{
	return self->fb.desire_supershotgun + (self->fb.virtual_enemy ? self->fb.virtual_enemy->fb.desire_supershotgun * 0.5 : 0);
}

static float goal_supershotgun2 (gedict_t* self)
{
	if (deathmatch != 1 && ((int)self->s.v.items & IT_SUPER_SHOTGUN))
		return 0;

	return goal_supershotgun1 (self);
}

static float goal_nailgun1 (gedict_t* self)
{
	return self->fb.desire_nailgun + (self->fb.virtual_enemy ? self->fb.virtual_enemy->fb.desire_nailgun * 0.5 : 0);
}

static float goal_nailgun2 (gedict_t* self)
{
	if (deathmatch != 1 && ((int)self->s.v.items & IT_NAILGUN))
		return 0;

	return goal_nailgun1 (self);
}

static float goal_supernailgun1 (gedict_t* self)
{
	return self->fb.desire_supernailgun + (self->fb.virtual_enemy ? self->fb.virtual_enemy->fb.desire_supernailgun * 0.5 : 0);
}

static float goal_supernailgun2 (gedict_t* self)
{
	if (deathmatch != 1 && ((int)self->s.v.items & IT_SUPER_NAILGUN))
		return 0;

	return goal_supernailgun1 (self);
}

static float goal_grenadelauncher1 (gedict_t* self)
{
	return self->fb.desire_grenadelauncher + (self->fb.virtual_enemy ? self->fb.virtual_enemy->fb.desire_grenadelauncher * 0.5 : 0);
}

static float goal_grenadelauncher2 (gedict_t* self)
{
	if (deathmatch != 1 && ((int)self->s.v.items & IT_GRENADE_LAUNCHER))
		return 0;

	return goal_grenadelauncher1 (self);
}

static float goal_rocketlauncher1 (gedict_t* self)
{
	return self->fb.desire_rocketlauncher + (self->fb.virtual_enemy ? self->fb.virtual_enemy->fb.desire_rocketlauncher : 0);
}

static float goal_rocketlauncher2 (gedict_t* self)
{
	if (deathmatch != 1 && ((int)self->s.v.items & IT_ROCKET_LAUNCHER))
		return 0;

	return goal_rocketlauncher1 (self);
}

static float goal_lightning1 (gedict_t* self)
{
	return self->fb.desire_lightning + (self->fb.virtual_enemy ? self->fb.virtual_enemy->fb.desire_lightning * 0.5 : 0);
}

static float goal_lightning2 (gedict_t* self)
{
	if (deathmatch != 1 && ((int)self->s.v.items & IT_LIGHTNING))
		return 0;

	return goal_lightning1 (self);
}

static float goal_shells (gedict_t* self)
{
	if (self->s.v.ammo_shells < 100) {
		return self->fb.desire_shells;
	}
	return 0;
}

static float goal_spikes (gedict_t* self)
{
	if (self->s.v.ammo_nails < 200) {
		return self->fb.desire_nails;
	}
	return 0;
}

static float goal_rockets (gedict_t* self)
{
	if (self->s.v.ammo_rockets < 100) {
		return self->fb.desire_rockets + (self->fb.virtual_enemy ? self->fb.virtual_enemy->fb.desire_rockets : 0);
	}
	return 0;
}

static float goal_cells (gedict_t* self)
{
	if (self->s.v.ammo_cells < 100) {
		return self->fb.desire_cells + (self->fb.virtual_enemy ? self->fb.virtual_enemy->fb.desire_cells * 0.5 : 0);
	}
	return 0;
}

static float goal_artifact_invulnerability (gedict_t* self)
{
	if (Get_Powerups ())
		return 200 + self->fb.total_damage;
	return 0;
}

static float goal_artifact_invisibility (gedict_t* self)
{
	if (Get_Powerups ())
		return 200 + self->fb.total_damage;
	return 0;
}

static float goal_artifact_super_damage (gedict_t* self)
{
	if (Get_Powerups ())
		return 200 + self->fb.total_damage;
	return 0;
}

// Pickup functions (TODO)

qbool pickup_health0 (void)
{
	return (qbool)(self->s.v.health < 100);
}

qbool pickup_health2 (void)
{
	return (qbool)(self->s.v.health < 250);
}

qbool pickup_armor1 (void)
{
	return (qbool)(self->fb.total_armor < 30);
}

qbool pickup_armor2 (void)
{
	return (qbool)(self->fb.total_armor < 90);
}

qbool pickup_armorInv (void)
{
	return (qbool)(self->fb.total_armor < 160);
}

qbool pickup_supershotgun2 (void)
{
	return (qbool)!((int)self->s.v.items & IT_SUPER_SHOTGUN);
}

qbool pickup_true (void)
{
	return true;
}

qbool pickup_nailgun2 (void)
{
	return deathmatch == 1 || (qbool)!((int)self->s.v.items & IT_NAILGUN);
}

qbool pickup_supernailgun2 (void)
{
	return deathmatch == 1 || (qbool)!((int)self->s.v.items & IT_SUPER_NAILGUN);
}

qbool pickup_grenadelauncher2 (void)
{
	return deathmatch == 1 || (qbool)!((int)self->s.v.items & IT_GRENADE_LAUNCHER);
}

qbool pickup_rocketlauncher2 (void)
{
	return deathmatch == 1 || (qbool)!((int)self->s.v.items & IT_ROCKET_LAUNCHER);
}

qbool pickup_lightning2 (void)
{
	return deathmatch == 1 || (qbool)!((int)self->s.v.items & IT_LIGHTNING);
}

qbool pickup_shells (void)
{
	return (qbool)(self->s.v.ammo_shells < 100);
}

qbool pickup_spikes (void)
{
	return (qbool)(self->s.v.ammo_nails < 200);
}

qbool pickup_rockets (void)
{
	return (qbool)(self->s.v.ammo_rockets < 100);
}

qbool pickup_cells (void)
{
	return (qbool)(self->s.v.ammo_cells < 100);
}

void StartItemFB (gedict_t* ent)
{
	AddToQue (ent);
	VectorSet (ent->s.v.view_ofs, 80, 80, 24);
	if (!ent->s.v.touch) {
		ent->s.v.touch = (func_t)marker_touch;
		ent->s.v.nextthink = -1;
	}

	ent->fb.goal_respawn_time = g_globalvars.time;
	ent->s.v.solid = SOLID_TRIGGER;
}

static void BotTookMessage (gedict_t* item, gedict_t* player)
{
	if (player->isBot && teamplay) {
		TeamplayMessageByName (player, "took");
	}
}

static void BotDroppedMessage (gedict_t* item, gedict_t* player)
{
}

//
// Health
static void fb_health_taken (gedict_t* item, gedict_t* player)
{
	if ((int)item->s.v.spawnflags & H_MEGA) {
		BotTookMessage (item, player);
		item->fb.goal_respawn_time = g_globalvars.time + 5 + max (player->s.v.health - 100, 0);
	}
	else {
		item->fb.goal_respawn_time = item->s.v.nextthink;
	}

	AssignVirtualGoal (item);
	FrogbotSetHealthArmour (player);
	UpdateGoalEntity (item, player);
	item->s.v.solid = SOLID_TRIGGER;
}

static qbool fb_health_touch (gedict_t* item, gedict_t* player)
{
	if (player->ct != ctPlayer)
		return true;
	if (IsMarkerFrame ())
		check_marker (item, player);
	if (WaitingToRespawn (item) || self->s.v.think == (func_t)item_megahealth_rot)
		return true;
	if (NoItemTouch (item, player))
		return true;

	return false;
}

static void fb_health_rot (gedict_t* item, gedict_t* player)
{
	FrogbotSetHealthArmour (player);

	if (player->s.v.health <= 100) {
		item->fb.goal_respawn_time = item->s.v.nextthink;
		AssignVirtualGoal (item);
	}
}

static void fb_spawn_health (gedict_t* ent)
{
	if ((int)ent->s.v.spawnflags & H_MEGA) {
		ent->fb.desire = goal_health2;
		ent->fb.pickup = pickup_health2;
	}
	else {
		ent->fb.desire = goal_health0;
		ent->fb.pickup = pickup_health0;
	}

	ent->fb.item_taken = fb_health_taken;
	ent->fb.item_touch = fb_health_touch;
	ent->fb.item_affect = fb_health_rot;
	ent->fb.item_respawned = AssignVirtualGoal;
	StartItemFB (ent);
}

//
// Armor

static void fb_armor_taken (gedict_t* item, gedict_t* player)
{
	item->fb.goal_respawn_time = item->s.v.nextthink;
	AssignVirtualGoal (item);
	FrogbotSetHealthArmour (player);
	UpdateGoalEntity (item, player);
	BotTookMessage (item, player);

	item->s.v.solid = SOLID_TRIGGER;
}

static qbool fb_armor_touch (gedict_t* item, gedict_t* player)
{
	if (IsMarkerFrame ())
		check_marker (item, player);

	if (WaitingToRespawn (item))
		return true;

	// allow the bot to hurt themselves to pickup armor
	if (player->isBot && player->s.v.takedamage) {
		qbool have_more_armor = player->fb.total_armor >= item->fb.total_armor;
		qbool want_armor = player->s.v.goalentity == NUM_FOR_EDICT (item);
		qbool has_rl = ((int)player->s.v.items & IT_ROCKET_LAUNCHER) && player->s.v.ammo_rockets > 0;
		qbool targetting_player = player->fb.look_object && player->fb.look_object->ct == ctPlayer;

		if (want_armor && have_more_armor && has_rl && IsMarkerFrame () && !targetting_player && !player->fb.firing) {
			player->fb.state |= HURT_SELF;
			SetLinkedMarker (player, item, "fb_armor_touch");
			player->fb.path_state = 0;
			player->fb.linked_marker_time = g_globalvars.time + 0.5f;
			player->fb.goal_refresh_time = g_globalvars.time + 2 + g_random ();
			//G_bprint (2, "Wait(HURT_SELF): %f vs %f armor\n", player->fb.total_armor, item->fb.total_armor);
			return true; // wait
		}
	}

	return NoItemTouch (item, player);
}

static void fb_spawn_armor (gedict_t* ent)
{
	if (streq (ent->s.v.classname, "item_armor1")) {
		ent->fb.desire = goal_armor1;
		ent->fb.pickup = pickup_armor1;
		ent->fb.total_armor = (k_yawnmode ? 0.4 : 0.3) * 100.0f;
	}
	else if (streq (ent->s.v.classname, "item_armor2")) {
		ent->fb.desire = goal_armor2;
		ent->fb.pickup = pickup_armor2;
		ent->fb.total_armor = (k_yawnmode ? 0.6 : 0.6) * 150.0f;
	}
	else if (streq (ent->s.v.classname, "item_armorInv")) {
		ent->fb.desire = goal_armorInv;
		ent->fb.pickup = pickup_armorInv;
		ent->fb.total_armor = (k_yawnmode ? 0.8 : 0.8) * 200.0f;
	}

	ent->fb.item_taken = fb_armor_taken;
	ent->fb.item_touch = fb_armor_touch;
	ent->fb.item_respawned = AssignVirtualGoal;

	StartItemFB (ent);
}

//
// weapons

static qbool fb_weapon_touch (gedict_t* item, gedict_t* player)
{
	if (player->ct != ctPlayer)
		return true;
	if (IsMarkerFrame ())
		check_marker (item, player);
	if (WaitingToRespawn (item))
		return true;
	if (NoItemTouch (item, player))
		return true;

	return false;
}

static void fb_weapon_taken (gedict_t* item, gedict_t* player)
{
	player->fb.weapon_refresh_time = 0;
	switch ((int)item->tp_flags) {
		case it_rl:
		case it_lg:
		case it_gl:
			BotTookMessage (item, player);
			break;
	}

	if (deathmatch == 2 || deathmatch == 3 || deathmatch == 5 || coop) {
		// Weapon left
	}
	else {
		UpdateGoalEntity (item, player);
		item->fb.goal_respawn_time = item->s.v.nextthink;
		AssignVirtualGoal (item);
		item->s.v.solid = SOLID_TRIGGER;
	}
}

static void StartWeapon (gedict_t* ent)
{
	ent->fb.item_taken = fb_weapon_taken;
	ent->fb.item_touch = fb_weapon_touch;
	ent->fb.item_respawned = AssignVirtualGoal;

	StartItemFB (ent);
}

static void fb_spawn_ssg (gedict_t* ent)
{
	SetGoalForMarker (FB_GOAL_SSG, ent);

	ent->fb.desire = goal_supershotgun2;
	ent->fb.pickup = pickup_supershotgun2;

	StartWeapon (ent);
}

static void fb_spawn_ng (gedict_t* ent)
{
	SetGoalForMarker (FB_GOAL_NG, ent);

	ent->fb.desire = goal_nailgun2;
	ent->fb.pickup = pickup_nailgun2;

	StartWeapon (ent);
}

static void fb_spawn_sng (gedict_t* ent)
{
	SetGoalForMarker (FB_GOAL_SNG, ent);

	ent->fb.desire = goal_supernailgun2;
	ent->fb.pickup = pickup_supernailgun2;

	StartWeapon (ent);
}

static void fb_spawn_gl (gedict_t* ent)
{
	// no goal set for GL
	ent->fb.desire = goal_grenadelauncher2;
	ent->fb.pickup = pickup_grenadelauncher2;

	StartWeapon (ent);
}

static void fb_spawn_rl (gedict_t* ent)
{
	// no goal set for RL
	ent->fb.desire = goal_rocketlauncher2;
	ent->fb.pickup = pickup_rocketlauncher2;

	StartWeapon (ent);
}

static void fb_spawn_lg (gedict_t* ent)
{
	// no goal set for LG
	ent->fb.desire = goal_lightning2;
	ent->fb.pickup = pickup_lightning2;

	StartWeapon (ent);
}

static qbool fb_ammo_touch (gedict_t* item, gedict_t* player)
{
	if (player->ct != ctPlayer)
		return true;
	if (IsMarkerFrame ())
		check_marker (item, player);
	if (WaitingToRespawn (item))
		return true;
	if (NoItemTouch (item, player))
		return true;
	return false;
}

static void fb_ammo_taken (gedict_t* item, gedict_t* player)
{
	item->fb.goal_respawn_time = item->s.v.nextthink;
	UpdateGoalEntity (item, player);
	AssignVirtualGoal (item);

	item->s.v.solid = SOLID_TRIGGER;
}

static void StartAmmoFB (gedict_t* ent)
{
	ent->fb.item_touch = fb_ammo_touch;
	ent->fb.item_taken = fb_ammo_taken;
	ent->fb.item_respawned = AssignVirtualGoal;
	StartItemFB (ent);
}

static void fb_spawn_shells (gedict_t* ent)
{
	SetGoalForMarker (FB_GOAL_SHELLS, ent);

	ent->fb.desire = goal_shells;
	ent->fb.pickup = pickup_shells;

	StartAmmoFB (ent);
}

static void fb_spawn_spikes (gedict_t* ent)
{
	SetGoalForMarker (FB_GOAL_SPIKES, ent);

	ent->fb.desire = goal_spikes;
	ent->fb.pickup = pickup_spikes;

	StartAmmoFB (ent);
}

static void fb_spawn_rockets (gedict_t* ent)
{
	// no goal for rockets

	ent->fb.desire = goal_rockets;
	ent->fb.pickup = pickup_rockets;

	StartAmmoFB (ent);
}

static void fb_spawn_cells (gedict_t* ent)
{
	SetGoalForMarker (FB_GOAL_CELLS, ent);

	ent->fb.desire = goal_cells;
	ent->fb.pickup = pickup_cells;

	StartAmmoFB (ent);
}

// Also defined in items.qc ... support for older maps
#define WEAPON_SHOTGUN  1
#define WEAPON_ROCKET  2
#define WEAPON_SPIKES  4
#define WEAPON_BIG  8
static void fb_spawn_weapon (gedict_t* ent)
{
	if ((int)ent->s.v.spawnflags & WEAPON_SHOTGUN) {
		fb_spawn_shells (ent);
	}
	else if ((int)ent->s.v.spawnflags & WEAPON_SPIKES) {
		fb_spawn_spikes (ent);
	}
	else if ((int)ent->s.v.spawnflags & WEAPON_ROCKET) {
		fb_spawn_rockets (ent);
	}
}

static qbool fb_powerup_touch (gedict_t* item, gedict_t* player)
{
	if (player->ct != ctPlayer)
		return true;
	if (IsMarkerFrame ())
		check_marker (item, player);
	if (WaitingToRespawn (item))
		return true;
	if (NoItemTouch (item, player))
		return true;

	return false;
}

static void fb_powerup_taken (gedict_t* item, gedict_t* player)
{
	UpdateGoalEntity (item, player);
	item->fb.goal_respawn_time = item->s.v.nextthink;
	AssignVirtualGoal (item);
	item->s.v.solid = SOLID_TRIGGER;
	player->fb.last_mm2_spot = 0;
	BotTookMessage (item, player);
}

static void fb_spawn_pent (gedict_t* ent)
{
	ent->fb.desire = goal_artifact_invulnerability;
	ent->fb.pickup = pickup_true;
	ent->fb.item_taken = fb_powerup_taken;
	ent->fb.item_respawned = AssignVirtualGoal;
	if (ent->fb.fl_marker) {
		ent->fb.item_touch = fb_powerup_touch;
		StartItemFB (ent);
	}
}

static void fb_spawn_biosuit (gedict_t* ent)
{
	ent->fb.desire = goal_NULL; // FIXME
	ent->fb.pickup = pickup_true;
	ent->fb.item_taken = fb_powerup_taken;
	ent->fb.item_respawned = AssignVirtualGoal;
	if (ent->fb.fl_marker) {
		ent->fb.item_touch = fb_powerup_touch;
		StartItemFB (ent);
	}
}

static void fb_spawn_ring (gedict_t* ent)
{
	ent->fb.desire = goal_artifact_invisibility;
	ent->fb.pickup = pickup_true;
	ent->fb.item_taken = fb_powerup_taken;
	ent->fb.item_respawned = AssignVirtualGoal;
	if (ent->fb.fl_marker) {
		ent->fb.item_touch = fb_powerup_touch;
		StartItemFB (ent);
	}
}

static void fb_spawn_quad (gedict_t* ent)
{
	ent->fb.desire = goal_artifact_super_damage;
	ent->fb.pickup = pickup_true;
	ent->fb.item_taken = fb_powerup_taken;
	ent->fb.item_respawned = AssignVirtualGoal;
	if (ent->fb.fl_marker) {
		ent->fb.item_touch = fb_powerup_touch;
		if (!streq ("aerowalk", g_globalvars.mapname)) {
			StartItemFB (ent);
		}
	}
}

fb_spawn_t itemSpawnFunctions[] = {
	{ "item_armor1", fb_spawn_armor },
	{ "item_armor2", fb_spawn_armor },
	{ "item_armorInv", fb_spawn_armor },
	{ "item_artifact_invulnerability", fb_spawn_pent },
	{ "item_artifact_envirosuit", fb_spawn_biosuit },
	{ "item_artifact_invisibility", fb_spawn_ring },
	{ "item_artifact_super_damage", fb_spawn_quad },
	{ "item_cells", fb_spawn_cells },
	{ "item_health", fb_spawn_health },
	{ "item_rockets", fb_spawn_rockets },
	{ "item_shells", fb_spawn_shells },
	{ "item_spikes", fb_spawn_spikes },
	{ "item_weapon", fb_spawn_weapon },
	{ "weapon_supershotgun", fb_spawn_ssg },
	{ "weapon_nailgun", fb_spawn_ng },
	{ "weapon_supernailgun", fb_spawn_sng },
	{ "weapon_grenadelauncher", fb_spawn_gl },
	{ "weapon_rocketlauncher", fb_spawn_rl },
	{ "weapon_lightning", fb_spawn_lg },
};

fb_spawn_t* ItemSpawnFunction (int i)
{
	return &itemSpawnFunctions[i];
}

int ItemSpawnFunctionCount (void)
{
	return sizeof (itemSpawnFunctions) / sizeof (itemSpawnFunctions[0]);
}

qbool NoItemTouch (gedict_t* self, gedict_t* other)
{
	//G_bprint (2, "NoItemTouch(%f,%f,%f) = [%f,%f,%f] > [%f,%f,%f]\n", PASSVEC3 (other->s.v.origin), PASSVEC3 (self->fb.virtual_mins), PASSVEC3 (self->fb.virtual_maxs));
	if (other->s.v.origin[0] <= self->fb.virtual_maxs[0] &&
		other->s.v.origin[1] <= self->fb.virtual_maxs[1] &&
		other->s.v.origin[2] <= self->fb.virtual_maxs[2]) {
		if (other->s.v.origin[0] >= self->fb.virtual_mins[0] &&
			other->s.v.origin[1] >= self->fb.virtual_mins[1] &&
			other->s.v.origin[2] >= self->fb.virtual_mins[2]) {
			if (other->s.v.goalentity == NUM_FOR_EDICT (self)) {
				other->fb.goal_refresh_time = 0;
			}
			return false;
		}
	}
	return true;
}

static void BackpackTimedOut (void)
{
	SUB_Remove ();
}

static float goal_health_backpack (gedict_t* self)
{
	if (self->invincible_time > g_globalvars.time)
		return 0;
	if (self->super_damage_finished > g_globalvars.time)
		return 0;

	return 20;
}

static qbool fb_backpack_touch (gedict_t* item, gedict_t* player)
{
	if (player->s.v.goalentity == NUM_FOR_EDICT (item))
		player->fb.goal_refresh_time = 0;

	return false;
}

static void fb_backpack_taken (gedict_t* item, gedict_t* player)
{
	player->fb.weapon_refresh_time = 0;

	UpdateGoalEntity (item, player);
	player->fb.old_linked_marker = NULL;
	SetLinkedMarker (player, LocateMarker (player->s.v.origin), "bp taken");
	player->fb.linked_marker_time = g_globalvars.time + 5;

	if ((int)item->s.v.items & (IT_ROCKET_LAUNCHER | IT_LIGHTNING)) {
		BotTookMessage (item, player);
	}
}

void BotsBackpackTouchedNonPlayer (gedict_t* pack, gedict_t* touch_ent)
{
	LocateDynamicItem (pack);
}

void BotsBackpackDropped (gedict_t* self, gedict_t* pack)
{
	pack->s.v.think = (func_t)BackpackTimedOut;
	pack->fb.goal_respawn_time = g_globalvars.time;

	if ((int)pack->s.v.items & IT_SUPER_SHOTGUN)
		pack->fb.desire = goal_supershotgun1;
	else if ((int)pack->s.v.items & IT_NAILGUN)
		pack->fb.desire = goal_nailgun1;
	else if ((int)pack->s.v.items & IT_SUPER_NAILGUN)
		pack->fb.desire = goal_supernailgun1;
	else if ((int)pack->s.v.items & IT_GRENADE_LAUNCHER)
		pack->fb.desire = goal_grenadelauncher1;
	else if ((int)pack->s.v.items & IT_ROCKET_LAUNCHER)
		pack->fb.desire = goal_rocketlauncher1;
	else if ((int)pack->s.v.items & IT_LIGHTNING)
		pack->fb.desire = goal_lightning1;
	else if (pack->s.v.ammo_cells > (pack->s.v.ammo_rockets * 5))
		pack->fb.desire = goal_cells;
	else if (pack->s.v.ammo_rockets)
		pack->fb.desire = goal_rockets;
	else if (pack->s.v.ammo_nails >= 50)
		pack->fb.desire = goal_spikes;
	else
		pack->fb.desire = goal_shells;

	if (deathmatch == 4) {
		pack->fb.desire = goal_health_backpack;
	}

	pack->fb.item_touch = fb_backpack_touch;
	pack->fb.item_taken = fb_backpack_taken;

	if (! (self->fb.state & BACKPACK_IS_UNREACHABLE)) {
		LocateDynamicItem (pack);
		BotDroppedMessage (self, pack);
	}
}

void BotsPowerupDropped (gedict_t* player, gedict_t* powerup)
{
	if (powerup->tp_flags & it_quad) {
		fb_spawn_quad (powerup);
	}
	else if (powerup->tp_flags & it_pent) {
		fb_spawn_pent (powerup);
	}
	else if (powerup->tp_flags & it_ring) {
		fb_spawn_ring (powerup);
	}
	else {
		return;
	}

	LocateDynamicItem (powerup);
	AssignVirtualGoal (powerup);
}

void BotsPowerupTouchedNonPlayer (gedict_t* powerup, gedict_t* touch_ent)
{
	LocateDynamicItem (powerup);
}
