// Converted from .qc on 05/02/2016

#ifndef FB_GLOBALS_H
#define FB_GLOBALS_H

#define PATH_SCORE_NULL -1000000

#define FB_OPTION_SHOW_MARKERS      1
#define FB_OPTION_EDITOR_MODE       2
#define FB_OPTION_SHOW_THINKING     4

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

// FIXME: A lot of these not used anymore
#define FL_ONGROUND_PARTIALGROUND (FL_ONGROUND | FL_PARTIALGROUND)
#define IT_EITHER_NAILGUN (IT_NAILGUN | IT_SUPER_NAILGUN)
#define IT_NAILGUN_ROCKET (IT_ROCKET_LAUNCHER | IT_SUPER_NAILGUN | IT_NAILGUN)
#define IT_CONTINUOUS (IT_LIGHTNING | IT_SUPER_NAILGUN | IT_NAILGUN)
#define IT_AXE_SHOTGUN (IT_AXE | IT_SHOTGUN)
#define IT_ALL_BUT_GRENADE (IT_AXE | IT_SHOTGUN | IT_SUPER_SHOTGUN | IT_NAILGUN | IT_SUPER_NAILGUN | IT_ROCKET_LAUNCHER | IT_LIGHTNING)
#define IT_ALL (IT_AXE | IT_SHOTGUN | IT_SUPER_SHOTGUN | IT_NAILGUN | IT_SUPER_NAILGUN | IT_GRENADE_LAUNCHER | IT_ROCKET_LAUNCHER | IT_LIGHTNING)
#define IT_NOT_AMMO 16773375
#define IT_ARMOR (IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3)
#define IT_NOT_ARMOR (~IT_ARMOR)
#define IT_INVULNERABILITY_QUAD (IT_INVULNERABILITY | IT_QUAD)
#define IT_NOT_INVISIBILITY (~IT_INVISIBILITY)
#define IT_NOT_INVULNERABILITY (~IT_INVULNERABILITY)
#define IT_NOT_SUIT (~IT_SUIT)
#define IT_NOT_QUAD (~IT_QUAD)
#define ITEM_RUNE_MASK (CTF_RUNE_RES | CTF_RUNE_STR | CTF_RUNE_HST | CTF_RUNE_RGN)
#define NOT_EF_DIMLIGHT 16777207
#define EF_DIMLIGHT_BLUE (EF_DIMLIGHT | EF_BLUE)
#define EF_DIMLIGHT_RED (EF_DIMLIGHT | EF_RED)
#define CLIENTKILL 11

#define FB_PREFER_ROCKET_LAUNCHER 1
#define FB_PREFER_LIGHTNING_GUN   2

#define GAME_ENABLE_POWERUPS 1
#define GAME_ENABLE_RUNES 2
#define GAME_RUNE_RJ 4
#define GAME_MATCH 64
#define GAME_ENABLE_DROPWEAP 512
#define GAME_ENABLE_AUTOREPORT 1024
#define GAME_ENABLE_AUTOSTEAMS 2048
#define FORWARD 1
#define BACK 2
#define LEFT 4
#define RIGHT 8
#define FORWARD_LEFT 5
#define FORWARD_RIGHT 9
#define BACK_LEFT 6
#define BACK_RIGHT 10
#define UP 16
#define DOWN 32
#define JUMPSPEED 270

// Fall results
#define FALL_FALSE 0
#define FALL_BLOCKED 1
#define FALL_LAND 2
#define FALL_DEATH 3

// Path flags
#define WATERJUMP_ (1 << 1)
#define DM6_DOOR (1 << 8)
#define ROCKET_JUMP (1 << 9)
#define JUMP_LEDGE (1 << 10)
#define VERTICAL_PLATFORM (1 << 11)
#define BOTPATH_DOOR (1 << 12)
#define BOTPATH_DOOR_CLOSED (1 << 13)
#define SAVED_DESCRIPTION (DM6_DOOR | ROCKET_JUMP | JUMP_LEDGE | VERTICAL_PLATFORM | BOTPATH_DOOR | BOTPATH_DOOR_CLOSED)
#define NOT_ROCKET_JUMP (~ROCKET_JUMP)
#define REVERSIBLE (1 << 14)
#define WATER_PATH (1 << 15)
#define DELIBERATE_AIR (1 << 17)
#define WAIT_GROUND (1 << 18)
#define STUCK_PATH (1 << 19)
#define AIR_ACCELERATION (1 << 20)
#define NO_DODGE (1 << 21)
#define DELIBERATE_AIR_WAIT_GROUND (DELIBERATE_AIR | WAIT_GROUND)

// Bot flags
#define BOTFLAG_UNREACHABLE 1

// Marker flags
#define UNREACHABLE 1                         // Typically set for markers that are lava, waterlevel 3?  (automate?)
#define T_WATER 2                             // Set by server if the marker is in liquid
#define T_NO_AIR 4                            // Set by server (means the bot would be trapped underwater - dm3 tunnel for instance)
#define MARKER_IS_DM6_DOOR 8                  // Should only be set for DM6 door to LG at the moment (logic needs generalised to all shootable doors)
#define MARKER_BLOCKED_ON_STATE_TOP 16        // If set, door is considered 'closed' when STATE_TOP.
#define MARKER_FIRE_ON_MATCH_START 32         // Used to automatically fire triggers (open secret doors etc).  Ignored if no bots spawned.
#define MARKER_DOOR_TOUCHABLE 64              // If set, door will spawn a touchable marker.  If not set, door shouldn't be in bot path definitions
#define MARKER_ESCAPE_ROUTE 128               // (not currently implemented) bot should head towards marker when in lava or slime (think amphi2/end)
#define MARKER_DYNAMICALLY_ADDED 256          // Added dynamically by server.  Do not include in .bot file generation

// Bot flags
#define NOTARGET_ENEMY 32
#define AWARE_SURROUNDINGS 128
#define HURT_SELF 1024
#define CHASE_ENEMY 2048
#define RUNAWAY 4096
#define WAIT 8192
#define NOT_NOTARGET_ENEMY ~(NOTARGET_ENEMY)
#define NOT_AWARE_SURROUNDINGS ~(AWARE_SURROUNDINGS)

#define CAMPBOT 1
#define SHOT_FOR_LUCK 2
#define HELP_TEAMMATE 128

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
void check_marker (gedict_t* self, gedict_t* other);
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

#define FROGBOT_PATH_FLAG_OPTIONS "w6rjv"
#define FROGBOT_MARKER_FLAG_OPTIONS "u6te"

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
void map_ukooldm3 (void);
void map_ukooldm6 (void);
void map_ukooldm8 (void);
void map_ultrav (void);
void map_ztndm1 (void);
void map_ztndm2 (void);
void map_ztndm3 (void);
void map_ztndm4 (void);
void map_ztndm5 (void);
void map_ztndm6 (void);

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
qbool BotUsingCorrectWeapon (gedict_t* self);

// bot_items.qc
int ItemSpawnFunctionCount (void);
void StartItemFB (gedict_t* ent);
qbool NoItemTouch (gedict_t* self, gedict_t* other);
qbool BotsPreTeleport (gedict_t* self, gedict_t* other);

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
qbool FrogbotShowMarkerIndicators (void);

void SetDirectionMove (gedict_t* self, vec3_t dir_move, const char* explanation);

void BotEventPlatformHitTop (gedict_t* self);
void BotEventPlatformHitBottom (gedict_t* self);
void BotEventDoorHitTop (gedict_t* self);
void BotEventDoorHitBottom (gedict_t* self);
void BotPlatformTouched (gedict_t* platform, gedict_t* player);

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

// editor
qbool HasSavedMarker (void);

#endif // ifdef(FB_GLOBALS_H)