/*
 * Client-side weapon prediction for KTX.
 *
 * The server owns the weapon state machine and runs it on its own clock
 * (client_time, advanced per player frame). Every player frame it sends us an
 * owner-only snapshot of that machine. We replay the input frames the server
 * has not acknowledged yet on top of the snapshot, so the view weapon animates,
 * its sounds play and its projectiles appear at the moment the player fires
 * rather than a round trip later.
 *
 * Three clocks are in play and must not be conflated:
 *   client_time  the server's per-player weapon clock, advanced by replaying
 *                usercmd durations. Drives the state machine exclusively.
 *   time         interpolated server time. Drives projectile flight and the
 *                extrapolation of server-sent projectiles.

 *   cltime       real client time, which never stops. Drives everything the
 *                player sees directly: trail spacing, the view model blend,
 *                grenade spin and the beam.
 */

// ==========================================================================
// Wire protocol, mirrors ktx include/progs.h and include/g_consts.h
//
// The sendflag sets are bit lists, so they are written as such: each entry is
// the next bit up, in the order the server writes the fields.
// ==========================================================================

const float EZCSQC_WEAPONINFO	= 1;
const float EZCSQC_PROJECTILE	= 2;

// weapon state payload
enumflags
{
	WEAPONINFO_INDEX,
	WEAPONINFO_AMMO_SHELLS,
	WEAPONINFO_AMMO_NAILS,
	WEAPONINFO_AMMO_ROCKETS,
	WEAPONINFO_AMMO_CELLS,
	WEAPONINFO_ATTACK,
	WEAPONINFO_TIMING,
	WEAPONINFO_PRED_PING
};

// the weapon byte carries the respawn generation above the weapon index
const float WEAPONINFO_WEAPON_MASK		= 0x0f;
const float WEAPONINFO_GENERATION_SHIFT	= 4;

// projectile payload
enumflags
{
	PROJECTILE_ORIGIN,
	PROJECTILE_MODEL,
	PROJECTILE_ANGLES,
	PROJECTILE_OWNER,
	PROJECTILE_SPAWN_ORIGIN
};

// server prediction flags
enumflags
{
	PRDFL_MIDAIR,
	PRDFL_COILGUN,
	PRDFL_YAWNMODE
};

// Not a flag: the server sends this as the whole byte to disable prediction.
const float PRDFL_FORCEOFF	= 255;

// item bits, for the weapon a switch is allowed to select
enumflags
{
	IT_SHOTGUN,
	IT_SUPER_SHOTGUN,
	IT_NAILGUN,
	IT_SUPER_NAILGUN,
	IT_GRENADE_LAUNCHER,
	IT_ROCKET_LAUNCHER,
	IT_LIGHTNING,
	IT_AXE			= 4096,
	IT_INVISIBILITY	= 524288
};

// temp entity number we peek at
const float TE_LIGHTNING2	= 6;

/*
 * Stats the mod publishes. The engine reserves 0..31 for itself and hands mods
 * 32..127; keep these in step with include/g_local.h.
 *
 * Gravity has to come this way. The client is told the server's movevars when
 * it connects, but no builtin surfaces them to us, so the mod republishes the
 * one value the grenade simulation needs.
 */
const float STAT_KTX_GRAVITY	= 32;
const float DEFAULT_GRAVITY		= 800;

// the one usercmd button the weapon state machine cares about
const float BUTTON_ATTACK	= 1;

/*
 * A dimension_hit with the low bit cleared, which is the bit the engine gives
 * network entities. Traces from an entity carrying this see the world alone.
 */
const float TRACE_WORLD_ONLY	= 254;

// ==========================================================================
// Weapon definition flags
//
// These mirror the server's WEPPREDANIM_* bits. The server also has a
// MOREBYTES marker that only exists to frame its byte stream; it has no
// meaning here and is deliberately absent.
// ==========================================================================

enumflags
{
	ANIM_SOUND,			// play .sound on entering this state
	ANIM_PROJECTILE,	// spawn a predicted projectile on entering
	ANIM_LGBEAM,		// draw the predicted lightning beam
	ANIM_MUZZLEFLASH,	// muzzle flash (server-side only, no csqc effect)
	ANIM_DEFAULT,		// the idle state; where a finished animation lands
	ANIM_ATTACK,		// firing state: sets attack_finished
	ANIM_BRANCH,		// re-tests +attack on entry, exits to .alt
	ANIM_SOUNDAUTO,		// .sound goes on CHAN_AUTO instead of CHAN_WEAPON
	ANIM_LTIME,			// throttled by the lightning sound window
	ANIM_SOUND2			// a second sound, always on CHAN_WEAPON
};

// ==========================================================================
// Tunables
// ==========================================================================

const float PRED_BACKUP			= 64;	// input frame ring, matches UPDATE_BACKUP
const float PRED_MASK			= 63;

const float LG_SOUND_WINDOW		= 0.6;	// lightning re-triggers its sound this often
const float LG_ATTACK_TIME		= 0.2;	// lightning refire, overrides the table
const float RESPAWN_GUARD		= 0.05;	// server discards attacks this long after spawning
const float ATTACK_HOLDOFF		= 0.05;	// and holds the weapon shut this long while it cannot fire
const float NEWMIS_STEP			= 0.05;	// the server advances a new missile one frame

const float HANDOFF_GRENADE_DIST	= 128;	// grenade may be adopted within this
const float HANDOFF_DIST			= 64;	// other projectiles, generous case
const float HANDOFF_PERP			= 8;	// ... or this far off the flight line
const float HANDOFF_PHASE			= 256;	// ... and this far along it
const float HANDOFF_MIN_TIME		= 0.15;
const float HANDOFF_MAX_TIME		= 0.3;
const float HANDOFF_EASE_FACTOR		= 6;	// ... easing this much of the gap per second

const float TRAIL_PERIOD		= 0.013;	// spacing between trail emissions
const float TRAIL_SEED_RANGE	= 150;	// trust a muzzle position no further out
const vector GRENADE_SPIN		= '300 300 300';
const float SPIN_STEP_MAX		= 0.05;	// cap one frame's worth of tumble
const float TOO_CLOSE_RADIUS	= 8;	// hide our own projectile inside this
const float BOUNCE_FACTOR		= 0.5;	// grenade restitution
const float BOUNCE_MAX_BUMPS	= 5;

const float IMPACT_LOOKAHEAD	= 0.02;	// sweep this far past the present position
const float LOCAL_MIN_LIFETIME	= 0.05;	// a prediction lasts at least this long

const float ROCKET_LIGHT_RADIUS	= 200;
const vector ROCKET_LIGHT_COLOUR	= '0.4 0.2 0.05';

const float SOUND_TOKEN_TTL		= 1.0;	// unmatched suppression tokens expire
const float MAX_SOUND_TOKENS	= 8;

const float VIEWMODEL_BLEND_RATE	= 10;	// view model pose blend, per second

const float BEAM_RANGE			= 600;
const float BEAM_HOLD			= 0.3;	// keep re-issuing a predicted beam this long
const float BEAM_SERVER_HOLD	= 0.15;	// treat the server's beam as live this long after one arrives

// ==========================================================================
// Cvars
//
// Named to match ezQuake so a player's existing settings carry over.
// The tri-state ones are: 0 off, 1 only when the server runs antilag,
// 2 force on regardless.
// ==========================================================================

var float autocvar_cl_nopred_weapon			= 0;
var float autocvar_cl_predict_weaponsound	= 1;
var float autocvar_cl_predict_projectiles	= 1;
var float autocvar_cl_predict_beam			= 1;
var float autocvar_cl_predict_buffer		= 1;
// 0 off, 1 the events worth noticing, 2 adds per-frame traces
var float autocvar_cl_predict_debug			= 0;

// ==========================================================================
// Shared state
//
// Everything else each module keeps to itself; the files are listed in
// progs.src in dependency order, so nothing here needs declaring ahead.
// ==========================================================================

entity	viewweapon;	// the entity carrying our predicted view model
float	sv_gravity;	// the server's, which csqc cannot otherwise see
