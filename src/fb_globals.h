// Converted from .qc on 05/02/2016

#define PATH_SCORE_NULL -1000000

#define FB_OPTION_SHOW_MARKERS 1

typedef void (*fb_spawn_func_t)(gedict_t* ent);

typedef struct fb_spawn_s {
	char* name;                 // classname
	fb_spawn_func_t func;       // initialisation function
} fb_spawn_t;

typedef struct fb_path_eval_s {
	gedict_t* touch_marker;
	gedict_t* test_marker;
	qbool rocket_alert;
	int description;
	float path_time;
	qbool path_normal;
	vec3_t player_origin;
	gedict_t* goalentity_marker;
	float goal_late_time;
	float lookahead_time_;
	vec3_t player_direction;
	qbool be_quiet;
} fb_path_eval_t;

// Globals used for general path-finding
extern qbool path_normal;
extern gedict_t* look_marker;
extern gedict_t* from_marker;
extern gedict_t* middle_marker;
extern gedict_t* to_marker;
extern gedict_t* next_marker;
extern gedict_t* prev_marker;
extern float traveltime;
extern float look_traveltime;
extern float zone_time;

// Globals for physics variables
extern float sv_maxspeed;
extern float sv_maxstrafespeed;
extern float sv_maxwaterspeed;
extern float half_sv_maxspeed;
extern float inv_sv_maxspeed;
extern float sv_accelerate;

extern gedict_t* dropper;

#ifndef FL_ONGROUND_PARTIALGROUND
#define FL_ONGROUND_PARTIALGROUND (FL_ONGROUND | FL_PARTIALGROUND)
#endif
#ifndef IT_EITHER_NAILGUN
#define IT_EITHER_NAILGUN (IT_NAILGUN | IT_SUPER_NAILGUN)
#endif
#ifndef IT_NAILGUN_ROCKET
#define IT_NAILGUN_ROCKET (IT_ROCKET_LAUNCHER | IT_SUPER_NAILGUN | IT_NAILGUN)
#endif
#ifndef IT_CONTINUOUS
#define IT_CONTINUOUS (IT_LIGHTNING | IT_SUPER_NAILGUN | IT_NAILGUN)
#endif
#ifndef IT_AXE_SHOTGUN
#define IT_AXE_SHOTGUN (IT_AXE | IT_SHOTGUN)
#endif
#ifndef IT_ALL_BUT_GRENADE
#define IT_ALL_BUT_GRENADE (IT_AXE | IT_SHOTGUN | IT_SUPER_SHOTGUN | IT_NAILGUN | IT_SUPER_NAILGUN | IT_ROCKET_LAUNCHER | IT_LIGHTNING)
#endif
#ifndef IT_ALL
#define IT_ALL (IT_AXE | IT_SHOTGUN | IT_SUPER_SHOTGUN | IT_NAILGUN | IT_SUPER_NAILGUN | IT_GRENADE_LAUNCHER | IT_ROCKET_LAUNCHER | IT_LIGHTNING)
#endif
#ifndef IT_NOT_AMMO
#define IT_NOT_AMMO 16773375
#endif
#ifndef IT_ARMOR
#define IT_ARMOR (IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3)
#endif
#ifndef IT_NOT_ARMOR
#define IT_NOT_ARMOR (~IT_ARMOR)
#endif
#ifndef IT_INVULNERABILITY_QUAD
#define IT_INVULNERABILITY_QUAD (IT_INVULNERABILITY | IT_QUAD)
#endif
#ifndef IT_NOT_INVISIBILITY
#define IT_NOT_INVISIBILITY (~IT_INVISIBILITY)
#endif
#ifndef IT_NOT_INVULNERABILITY
#define IT_NOT_INVULNERABILITY (~IT_INVULNERABILITY)
#endif
#ifndef IT_NOT_SUIT
#define IT_NOT_SUIT (~IT_SUIT)
#endif
#ifndef IT_NOT_QUAD
#define IT_NOT_QUAD (~IT_QUAD)
#endif
#ifndef ITEM_RUNE_MASK
#define ITEM_RUNE_MASK (CTF_RUNE_RES | CTF_RUNE_STR | CTF_RUNE_HST | CTF_RUNE_RGN)
#endif
#ifndef NOT_EF_DIMLIGHT
#define NOT_EF_DIMLIGHT 16777207
#endif
#ifndef EF_DIMLIGHT_BLUE
#define EF_DIMLIGHT_BLUE (EF_DIMLIGHT | EF_BLUE)
#endif
#ifndef EF_DIMLIGHT_RED
#define EF_DIMLIGHT_RED (EF_DIMLIGHT | EF_RED)
#endif
#ifndef CLIENTKILL
#define CLIENTKILL 11
#endif

#define FB_PREFER_ROCKET_LAUNCHER 1
#define FB_PREFER_LIGHTNING_GUN   2

#ifndef GAME_ENABLE_POWERUPS
#define GAME_ENABLE_POWERUPS 1
#endif
#ifndef GAME_ENABLE_RUNES
#define GAME_ENABLE_RUNES 2
#endif
#ifndef GAME_RUNE_RJ
#define GAME_RUNE_RJ 4
#endif
#ifndef GAME_MATCH
#define GAME_MATCH 64
#endif
#ifndef GAME_ENABLE_DROPWEAP
#define GAME_ENABLE_DROPWEAP 512
#endif
#ifndef GAME_ENABLE_AUTOREPORT
#define GAME_ENABLE_AUTOREPORT 1024
#endif
#ifndef GAME_ENABLE_AUTOSTEAMS
#define GAME_ENABLE_AUTOSTEAMS 2048
#endif
#ifndef FORWARD
#define FORWARD 1
#endif
#ifndef BACK
#define BACK 2
#endif
#ifndef LEFT
#define LEFT 4
#endif
#ifndef RIGHT
#define RIGHT 8
#endif
#ifndef FORWARD_LEFT
#define FORWARD_LEFT 5
#endif
#ifndef FORWARD_RIGHT
#define FORWARD_RIGHT 9
#endif
#ifndef BACK_LEFT
#define BACK_LEFT 6
#endif
#ifndef BACK_RIGHT
#define BACK_RIGHT 10
#endif
#ifndef UP
#define UP 16
#endif
#ifndef DOWN
#define DOWN 32
#endif
#ifndef JUMPSPEED
#define JUMPSPEED 270
#endif

// Fall results
#ifndef FALL_FALSE
#define FALL_FALSE 0
#endif
#ifndef FALL_BLOCKED
#define FALL_BLOCKED 1
#endif
#ifndef FALL_LAND
#define FALL_LAND 2
#endif
#ifndef FALL_DEATH
#define FALL_DEATH 3
#endif

// Path flags
#ifndef WATERJUMP_
#define WATERJUMP_ (1 << 1)
#endif
#ifndef DM6_DOOR
#define DM6_DOOR (1 << 8)
#endif
#ifndef ROCKET_JUMP
#define ROCKET_JUMP (1 << 9)
#endif
#ifndef JUMP_LEDGE
#define JUMP_LEDGE (1 << 10)
#endif
#ifndef VERTICAL_PLATFORM
#define VERTICAL_PLATFORM (1 << 11)
#endif
#ifndef BOTPATH_DOOR
#define BOTPATH_DOOR (1 << 12)
#endif
#ifndef BOTPATH_DOOR_CLOSED
#define BOTPATH_DOOR_CLOSED (1 << 13)
#endif
#ifndef SAVED_DESCRIPTION
#define SAVED_DESCRIPTION (DM6_DOOR | ROCKET_JUMP | JUMP_LEDGE | VERTICAL_PLATFORM | BOTPATH_DOOR | BOTPATH_DOOR_CLOSED)
#endif
#ifndef NOT_ROCKET_JUMP
#define NOT_ROCKET_JUMP (~ROCKET_JUMP)
#endif
#ifndef REVERSIBLE
#define REVERSIBLE (1 << 14)
#endif
#ifndef WATER_PATH
#define WATER_PATH (1 << 15)
#endif
#ifndef DELIBERATE_AIR
#define DELIBERATE_AIR (1 << 17)
#endif
#ifndef WAIT_GROUND
#define WAIT_GROUND (1 << 18)
#endif
#ifndef STUCK_PATH
#define STUCK_PATH (1 << 19)
#endif
#ifndef AIR_ACCELERATION
#define AIR_ACCELERATION (1 << 20)
#endif
#ifndef NO_DODGE
#define NO_DODGE (1 << 21)
#endif
#ifndef DELIBERATE_AIR_WAIT_GROUND
#define DELIBERATE_AIR_WAIT_GROUND (DELIBERATE_AIR | WAIT_GROUND)
#endif

// Bot flags
#ifndef BOTFLAG_UNREACHABLE
#define BOTFLAG_UNREACHABLE 1
#endif

// Marker flags
#ifndef UNREACHABLE
#define UNREACHABLE 1
#endif
#ifndef T_WATER
#define T_WATER 2
#endif
#ifndef T_NO_AIR
#define T_NO_AIR 4
#endif
#ifndef MARKER_IS_DM6_DOOR
#define MARKER_IS_DM6_DOOR 8
#endif
#ifndef MARKER_BLOCKED_ON_STATE_TOP
#define MARKER_BLOCKED_ON_STATE_TOP 16
#endif
#ifndef MARKER_FIRE_ON_MATCH_START
#define MARKER_FIRE_ON_MATCH_START 32
#endif
#ifndef MARKER_DOOR_TOUCHABLE
#define MARKER_DOOR_TOUCHABLE 64
#endif

// Bot flags
#ifndef NOTARGET_ENEMY
#define NOTARGET_ENEMY 32
#endif
#ifndef AWARE_SURROUNDINGS
#define AWARE_SURROUNDINGS 128
#endif
#ifndef HURT_SELF
#define HURT_SELF 1024
#endif
#ifndef CHASE_ENEMY
#define CHASE_ENEMY 2048
#endif
#ifndef RUNAWAY
#define RUNAWAY 4096
#endif
#ifndef WAIT
#define WAIT 8192
#endif
#ifndef NOT_HURT_SELF
#define NOT_HURT_SELF (~HURT_SELF)
#endif
#ifndef NOT_NOTARGET_ENEMY
#define NOT_NOTARGET_ENEMY ~(NOTARGET_ENEMY)
#endif
#ifndef NOT_AWARE_SURROUNDINGS
#define NOT_AWARE_SURROUNDINGS ~(AWARE_SURROUNDINGS)
#endif

#ifndef CAMPBOT
#define CAMPBOT 1
#endif
#ifndef SHOT_FOR_LUCK
#define SHOT_FOR_LUCK 2
#endif
#ifndef HELP_TEAMMATE
#define HELP_TEAMMATE 128
#endif

qbool ExistsPath (gedict_t* from_marker, gedict_t* to_marker, int* new_path_state);
float boomstick_only (void);

float CountTeams (void);
qbool EnemyDefenceless (gedict_t* self);

float enemy_shaft_attack (void);
float W_BestWeapon (void);

gedict_t* LocateMarker (vec3_t org);
float RankForWeapon (float w);
float WeaponCode (float w);

float crandom (void);

qbool able_rj (gedict_t* self);
qbool PointVisible (vec3_t vec);
qbool VisibleEntity (gedict_t* ent);
qbool TP_CouldDamageTeammate (gedict_t* self);
gedict_t* IdentifyMostVisibleTeammate (gedict_t* self);
float anglemod (float v);
gedict_t* HelpTeammate (void);

// 
char* SetNetName (int botNumber);
char* SetTeamNetName (int botNumber, const char* teamName);

qbool Visible_360 (gedict_t* self, gedict_t* visible_object);
qbool Visible_infront (gedict_t* self, gedict_t* visible_object);
unsigned long ClientFlag (gedict_t* client);

// marker_util.qc
void marker_touch (void);
void BecomeMarker (gedict_t* marker);

// route_calc.qc
void CheckWaterColumn (gedict_t* m, vec3_t m_pos, vec3_t testplace);

// botwater.qc
void BotWaterMove (gedict_t* self);

// items.qc
float goal_NULL (gedict_t* self);

// client.qc
void BotDeathThink (void);
void BotClientEntersEvent (gedict_t* self, gedict_t* spawn_pos);
void BotPreThink (gedict_t* self);
void BotClientConnectedEvent (gedict_t* self);
void BotOutOfWater (gedict_t* self);
void BotSetCommand (gedict_t* self);
void BotsThinkTime (gedict_t* self);

// botphys.qc
void FrogbotPrePhysics1 (void);
void FrogbotPrePhysics2 (void);

// bothazd.qc
void AvoidHazards (gedict_t* self);
void NewVelocityForArrow (gedict_t* self, vec3_t dir_move, const char* explanation);

// route_lookup.qc
gedict_t* SightMarker (gedict_t* from_marker, gedict_t* to_marker, float max_distance, float min_height_diff);
gedict_t* HigherSightFromFunction (gedict_t* from_marker, gedict_t* to_marker);
gedict_t* SightFromMarkerFunction (gedict_t* from_marker, gedict_t* to_marker);
gedict_t* SubZoneNextPathMarker (gedict_t* from_marker, gedict_t* to_marker);
float SubZoneArrivalTime (float zone_time, gedict_t* middle_marker, gedict_t* to_marker);
float SightFromTime (gedict_t* from_marker, gedict_t* to_marker);
void ZoneMarker (gedict_t* from_marker, gedict_t* to_marker, qbool path_normal);
gedict_t* ZonePathMarker (gedict_t* from_marker, gedict_t* to_marker, qbool path_normal);

// botweap.qc
void FrogbotSetFirepower (gedict_t* self);
void SelectWeapon (void);
void SetFireButton (gedict_t* self, vec3_t rel_pos, float rel_dist);
void FrogbotWeaponFiredEvent (gedict_t* self);

// marker_util.qc
void AssignVirtualGoal (gedict_t* item);
void AssignVirtualGoal_apply (gedict_t* marker_);
void adjust_view_ofs_z (gedict_t* ent);

// botenemy.qc
void ClearLookObject (gedict_t* client);
void LookEnemy (gedict_t* player, gedict_t* enemy);
qbool BotsPickBestEnemy (gedict_t* self);
void BotDamageInflictedEvent (gedict_t* attacker, gedict_t* targ);

// botjump.qc
void CheckCombatJump (void);
//void BotInLava(void);
void BotPerformRocketJump (gedict_t* self);

// botgoal.qc
void UpdateGoal (gedict_t* self);

// botpath.qc
void ProcessNewLinkedMarker (gedict_t* self);

// marker_load.qc
gedict_t* CreateMarker (float x, float y, float z);
void AllMarkersLoaded (void);

void SetZone (int zone_number, int marker_number);
void SetMarkerFlag (int marker_number, int flags);
void SetMarkerFixedSize (int marker_number, int size_x, int size_y, int size_z);
void AddToQue (gedict_t* ent);
void SetGoal (int goal, int marker_number);
void SetGoalForMarker (int goal, gedict_t* marker);
void SetMarkerPathFlags (int marker_number, int path_index, int flags);
void SetMarkerPath (int source_marker, int path_index, int next_marker);
void SetMarkerViewOffset (int marker, float zOffset);

// added for ktx
qbool fb_lg_disabled (void);
//void StartItems(void);

// maps
void LoadMap (void);
qbool FrogbotsCheckMapSupport (void);

void map_aerowalk (void);
void map_amphi2 (void);
void map_dm4 (void);
void map_dm3 (void);
void map_dm6 (void);
void map_e1m2 (void);
void map_frobodm2 (void);
void map_pkeg1 (void);
void map_povdmm4 (void);
void map_spinev2 (void);
void map_ukooldm2 (void);
void map_ultrav (void);
void map_ztndm3 (void);
void map_ztndm4 (void);
void map_ztndm5 (void);

// 
float pr1_rint (float f);

// 
void BotEvadeLogic (gedict_t* self);
qbool SameTeam (gedict_t* p1, gedict_t* p2);

// botstat.qc
void FrogbotSetHealthArmour (gedict_t* client);
void UpdateGoalEntity (gedict_t* item);

// bot_client.qc
void BotPlayerDeathEvent (gedict_t* self);

// bot_items.qc
int ItemSpawnFunctionCount (void);
void StartItemFB (gedict_t* ent);
qbool NoItemTouch (gedict_t* self, gedict_t* other);
qbool BotsPreTeleport (gedict_t* self, gedict_t* other);

#define FB_CVAR_GAMEMODE "k_fb_gamemode"
#define FB_CVAR_SKILL "k_fb_skill"

// FBCA code just set nextthink to 0 when item respawned...
qbool WaitingToRespawn (gedict_t* ent);

int NumberOfClients (void);
qbool TimeTrigger (float *next_time, float time_increment);

qbool IsMarkerFrame (void);
qbool IsHazardFrame (void);

// fb_globals.c
gedict_t* FirstZoneMarker (int zone);
void AddZoneMarker (gedict_t* marker);

// bot_commands.qc
void SetLinkedMarker (gedict_t* player, gedict_t* marker, char* explanation);

// bot_routing.qc
void PathScoringLogic (
	float goal_respawn_time, qbool be_quiet, float lookahead_time, qbool path_normal, vec3_t player_origin, vec3_t player_velocity, gedict_t* touch_marker_,
	gedict_t* goalentity_marker, qbool rocket_alert, qbool rocket_jump_routes_allowed,
	qbool trace_bprint, float *best_score, gedict_t** linked_marker_, int* new_path_state
);

int BotVersionNumber (void);
qbool FrogbotOptionEnabled (int option);

void SetDirectionMove (gedict_t* self, vec3_t dir_move, const char* explanation);

void BotEventPlatformHitTop (gedict_t* self);
void BotEventPlatformHitBottom (gedict_t* self);
void BotEventDoorHitTop (gedict_t* self);
void BotEventDoorHitBottom (gedict_t* self);

void BotsBackpackTouchedNonPlayer (gedict_t* backpack, gedict_t* entity);
void BotsMatchStart (void);
void BotsAssignTeamFlags (void);
qbool HasWeapon (gedict_t* player, int weapon);

// botthink.qc
void SetMarker (gedict_t* client, gedict_t* marker);

#define PAST(x) (g_globalvars.time >= self->fb.x)
#define FUTURE(x) (g_globalvars.time < self->fb.x)

#define MIN_FROGBOT_SKILL  0
#define MAX_FROGBOT_SKILL 20

// debugging
void RunRandomTrials (float min, float max, float mult);
