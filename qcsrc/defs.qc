/*
 * defs.qc - shared constants, types, globals and cross-file prototypes for the
 * KTX CSQC (csprogs.dat). Included immediately after fteextensions.qc so every
 * later unit can rely on these declarations regardless of compile order.
 *
 * This CSQC mirrors the client-side prediction that unezQuake reimplements
 * natively (EZCSQC). The KTX server drives it by sending CSQC entities of a few
 * types plus a per-weapon "weapon definition" table that scripts the viewmodel
 * animation FSM.
 */

//
// Networked entity types (must match include/g_consts.h on the server).
//
#define EZCSQC_WEAPONINFO       1
#define EZCSQC_PROJECTILE       2
#define EZCSQC_PLAYER           3
#define EZCSQC_WEAPONDEF        4
#define EZCSQC_HUDELEMENT       32

//
// EZCSQC_PROJECTILE sendflags.
//
#define PROJECTILE_ORIGIN       (1 << 0)
#define PROJECTILE_MODEL        (1 << 1)
#define PROJECTILE_ANGLES       (1 << 2)
#define PROJECTILE_OWNER        (1 << 3)
#define PROJECTILE_SPAWN_ORIGIN (1 << 4)

//
// EZCSQC_WEAPONINFO sendflags.
//
#define WEAPONINFO_INDEX        (1 << 0)
#define WEAPONINFO_AMMO_SHELLS  (1 << 1)
#define WEAPONINFO_AMMO_NAILS   (1 << 2)
#define WEAPONINFO_AMMO_ROCKETS (1 << 3)
#define WEAPONINFO_AMMO_CELLS   (1 << 4)
#define WEAPONINFO_ATTACK       (1 << 5)
#define WEAPONINFO_TIMING       (1 << 6)
#define WEAPONINFO_PRED_PING    (1 << 7)
#define WEAPONINFO_ALL          255

//
// EZCSQC_PLAYER sendflags (Phase 8).
//
#define PLAYERPRED_STATE        (1 << 0)
#define PLAYERPRED_FLAGS        (1 << 1)
// pmflags byte
#define PLAYERPREDF_ONGROUND    (1 << 0)
#define PLAYERPREDF_NOPREDICT   (1 << 1)
#define PLAYERPREDF_TELEPORT    (1 << 2)

//
// EZCSQC_WEAPONDEF sendflags.
//
#define WEAPONDEF_INIT          (1 << 0)
#define WEAPONDEF_FLAGS         (1 << 1)
#define WEAPONDEF_ANIM          (1 << 2)

#define WEPPRED_MAXSTATES       32
#define MAX_PREDWEPS            16

//
// Weapon definition animation flags. Low byte fits in one wire byte; the high
// byte only ships when WEPPREDANIM_MOREBYTES is set.
//
#define WEPPREDANIM_SOUND       0x0001
#define WEPPREDANIM_PROJECTILE  0x0002
#define WEPPREDANIM_LGBEAM      0x0004
#define WEPPREDANIM_MUZZLEFLASH 0x0008
#define WEPPREDANIM_DEFAULT     0x0010  // +attack is checked while resting on this state
#define WEPPREDANIM_ATTACK      0x0020
#define WEPPREDANIM_BRANCH      0x0040
#define WEPPREDANIM_MOREBYTES   0x0080  // an extra flags byte follows on the wire
#define WEPPREDANIM_SOUNDAUTO   (0x0100 | WEPPREDANIM_MOREBYTES)
#define WEPPREDANIM_LTIME       (0x0200 | WEPPREDANIM_MOREBYTES)
#define WEPPREDANIM_SOUND2      (0x0400 | WEPPREDANIM_MOREBYTES)
#define WEPPREDANIM_HAS(fl, f)  (((fl) & (f)) == (f))

// ms -> seconds, matching the server's timing units.
#define LENGTH2S(x)             ((x) * 0.001)

//
// Quake item bits.
//
#define IT_SHOTGUN              1
#define IT_SUPER_SHOTGUN        2
#define IT_NAILGUN              4
#define IT_SUPER_NAILGUN        8
#define IT_GRENADE_LAUNCHER     16
#define IT_ROCKET_LAUNCHER      32
#define IT_LIGHTNING            64
#define IT_AXE                  4096
#define IT_HOOK                 32768

//
// KTX prediction flags carried by WEAPONINFO_PRED_PING.
//
#define PRDFL_MIDAIR            1
#define PRDFL_COILGUN           2
#define PRDFL_FORCEOFF          255

// Custom drawmask bit for the predicted viewmodel pass.
#define MASK_PRED_VIEWMODEL     256

// Effect-gate flags used by the weapon replay loop.
#define EZCSQC_EFFECT_SOUND      1
#define EZCSQC_EFFECT_PROJECTILE 2

// Predicted-explosion ring buffer size (shared by explosions.qc + playerpred.qc).
#define EXPL_RING 8

//
// Weapon definition tables (populated from EZCSQC_WEAPONDEF).
//
typedef struct weppredanim_s
{
	int     mdlframe;       // >=0 absolute viewmodel frame, <0 looping range length
	int     flags;
	int     sound;
	int     soundmask;
	string  sound_name;     // resolved name for CSQC_Event_Sound suppression
	int     sound2;
	int     soundmask2;
	string  sound2_name;
	int     projectile_model;
	vector  projectile_velocity;    // weapon-local: x=right, y=forward, z=up
	vector  projectile_offset;      // weapon-local: x=right, y=forward, z=world-up
	int     nextanim;
	int     altanim;
	float   length;         // seconds (wire byte * 10ms)
} weppredanim_t;

typedef struct weppreddef_s
{
	int     modelindex;
	float   attack_time;    // seconds
	int     impulse;
	int     itemflag;
	int     anim_number;
	weppredanim_t anims[WEPPRED_MAXSTATES];
} weppreddef_t;

weppreddef_t wpredict_definitions[MAX_PREDWEPS];

//
// Weapon prediction state, replayed each frame from the server baseline.
//
typedef struct weapon_state_s
{
	int     impulse;
	int     weapon;         // itemflag of the current weapon (parity field)
	int     weapon_index;   // index into wpredict_definitions

	int     frame;

	int     ammo_shells;
	int     ammo_nails;
	int     ammo_rockets;
	int     ammo_cells;

	float   attack_finished;
	float   client_time;

	float   client_nextthink;
	int     client_thinkindex;

	int     client_predflags;
	float   client_ping;
} weapon_state;

weapon_state weapon_server;     // latest server baseline (delta-accumulated)
weapon_state weapon_predicted;  // scratch state during replay
weapon_state weapon_presented;  // state chosen for rendering (buffered)

//
// Shared globals.
//
float   ping_time;
float   chat_info;
float   lg_twidth;                  // LG loop-sound cadence gate
float   respawn_attack_guard_until; // 50ms post-respawn attack/switch lockout

// Effect gating (set per replayed frame).
float   is_effectframe;
int     current_effect_flags;
int     current_predframe;
int     last_sound_effectframe;
int     last_projectile_effectframe;
int     last_effectframe;

entity  viewweapon;                 // the predicted viewmodel entity
float   weaponpred_drawn;           // set when the predicted viewmodel rendered this frame

//
// Entity fields.
//
.vector oldorigin;
.vector s_origin;
.vector s_velocity;
.float  s_time;
.float  p_time;
.float  traileffectnum;
.float  ownnum;

.float  starttime;
.float  endtime;
.vector start;
.float  simtime;
.vector sim_origin;
.vector sim_angles;
.vector vel;
.vector avel;
.float  grounded;
.float  angle_simtime;
.float  projectile_type;    // 0 = linear (rocket/nail), 1 = grenade
.float  local_projectile;
.float  predicted_explosion;
.float  prediction_start_state_time;
.vector last_draw_origin;
.float  has_last_draw_origin;
.float  handoff_smoothing;
.vector handoff_delta;
.float  handoff_starttime;
.float  handoff_endtime;
.entity _prev;
.entity _next;

//
// Autocvars.
//
string  autocvar_r_rockettrail;
string  autocvar_r_grenadetrail;
float   autocvar_r_rocketlight;
vector  autocvar_r_rocketlight_colour;
float   autocvar_cl_sproj_xerp;
float   autocvar_cl_gunx;
float   autocvar_cl_guny;
float   autocvar_cl_gunz;
float   autocvar_cl_predict_weaponsound;
float   autocvar_cl_predict_projectiles;
float   autocvar_cl_predict_buffer;
float   autocvar_cl_predict_beam;
float   autocvar_cl_predict_explosions;
float   autocvar_cl_predict_player;
float   autocvar_cl_predict_smoothview;
float   autocvar_cl_nopred_weapon;
float   autocvar_csqc_gravity;
float   autocvar_csqc_ezdbg;

//
// Cross-file function prototypes.
//
void    WeaponPred_Init();
float   WeaponPred_Think();
void    EntUpdate_WeaponPred(float is_new);
void    EntUpdate_WeaponDef(float is_new);
void    WeaponPred_RegisterSound(int sound, int mask, int chan);
float   WeaponPred_SoundMaskEnabled(int mask);
float   WeaponPred_SuppressSound(float channel, string sample);
float   WeaponPred_DefinitionReady(int windex);

float   WeaponPred_SpawnProjectile(int windex, int animindex);
float   WeaponPred_PlayLGBeam();
void    WeaponPred_ClearBeam();
float   LGBeam_ShouldSuppress(float ent);

void    EntUpdate_Projectile(float is_new);
void    EntRemove_Projectile();
int     Projectile_FindTrail(int is_grenade);
float   Predraw_Projectile();
float   PredProjectile_MatchProjectile();

void    Explosion_Record(vector org);
float   Explosion_MatchServer(vector org);
float   Predict_OwnerRocketWorldImpact(entity proj, vector start, vector end);
float   Predict_RocketSpawnTouch(entity proj, vector origin, vector velocity);
void    Projectile_Bounce(entity proj, float dt, float update_angles);
vector  Projectile_ClipVelocity(vector vel, vector norm, float f);

void    EntUpdate_Player(float is_new);
void    PlayerPred_RunReplay();
