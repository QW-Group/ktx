#ifdef BOT_SUPPORT

// Converted from .qc on 05/02/2016
#include "g_local.h"
#include "fb_globals.h"

float predict_dist = 0;
gedict_t *test_enemy = 0;

gedict_t *current_waiting_bot = 0;
gedict_t *dropper = 0;
gedict_t *m_P = 0;
int m_D = 0;

gedict_t *markers[NUMBER_MARKERS] =
	{ 0 };
gedict_t *zone_head[NUMBER_ZONES] =
	{ 0 };
gedict_t *zone_tail[NUMBER_ZONES] =
	{ 0 };

qbool path_normal = false;
gedict_t *from_marker = 0;
gedict_t *middle_marker = 0;
gedict_t *next_marker = 0;
gedict_t *to_marker = 0;
gedict_t *look_marker = 0;
gedict_t *test_marker = 0;
gedict_t *prev_marker = 0;
gedict_t *goal_entity = 0;
float lookahead_time_ = 0;       // Safe to replace with self->fb.skill.lookahead_time
vec3_t origin_ =
	{ 0 };
float impulse_ = 0;
float time_start = 0;
float framecount_start = 0;
float sv_accelerate = 0;
float sv_maxfriction = 0;
float sv_accelerate_frametime = 0;
float sv_maxspeed = 0;
float sv_maxwaterspeed = 0;
float half_sv_maxspeed = 0;
float inv_sv_maxspeed = 0;
float sv_maxstrafespeed = 0;
float friction_factor = 0;
float old_time = 0;
float distance = 0;
vec3_t dir_forward =
	{ 0 };
float current_maxspeed = 0;
float max_accel_forward = 0;
vec3_t desired_accel =
	{ 0 };
vec3_t hor_velocity =
	{ 0 };
vec3_t new_velocity =
	{ 0 };                   // Can make local to functions that use it.
vec3_t jump_velocity =
	{ 0 };
vec3_t jump_origin =
	{ 0 };
float oldflags = 0;
float current_arrow = 0;
float content = 0;
float content1 = 0;
float content2 = 0;
float content3 = 0;
float fall = 0;
float new_fall = 0;
float current_fallspot = 0;
vec3_t edge_normal =
	{ 0 };
vec3_t self_view =
	{ 0 };
vec3_t testplace =
	{ 0 };						// FIXME: Make local...
vec3_t last_clear_velocity =
	{ 0 };
float jumpspeed = 0;
float path_score = 0;
float look_score = 0;
float best_goal_score = 0;
//gedict_t* linked_marker_ = 0;
vec3_t linked_marker_origin =
	{ 0 };
float goal_score = 0;
float goal_score2 = 0;
float enemy_score = 0;
vec3_t rel_pos =
	{ 0 };
vec3_t rel_pos2 =
	{ 0 };
vec3_t rel_dir =
	{ 0 };
float rel_dist = 0;
float rel_time = 0;
vec3_t rel_hor_dir =
	{ 0 };
float hor_component = 0;
gedict_t *enemy_touch_marker = 0;
vec3_t src =
	{ 0 };
vec3_t direction =
	{ 0 };
float risk = 0;
float risk_factor = 0;
vec3_t enemy_angles =
	{ 0 };
gedict_t *bot = 0;
float rnd = 0;
gedict_t *spawn_pos = 0;
gedict_t *spots = 0;
float pcount = 0;
gedict_t *thing = 0;
float numspots = 0;
float totalspots = 0;
vec3_t vec1 =
	{ 0 };
vec3_t vec2 =
	{ 0 };
vec3_t vec_ =
	{ 0 };
gedict_t *think_ent = 0;
gedict_t *trigger = 0;
vec3_t cmins =
	{ 0 };
vec3_t cmaxs =
	{ 0 };
gedict_t *item = 0;
vec3_t tmin =
	{ 0 };
vec3_t tmax =
	{ 0 };
float or = 0;
float nr = 0;
float best_weapon = 0;
float score_count = 0;
vec3_t item_pos =
	{ 0 };
vec3_t marker_pos =
	{ 0 };
gedict_t *marker_ = 0;
gedict_t *marker2 = 0;
float zone_time = 0;
float real_yaw_ = 0;
gedict_t *fireball = 0;
gedict_t *bubble = 0;
gedict_t *bubble_spawner = 0;
vec3_t org_ =
	{ 0 };
float rnd1 = 0;
float rnd2 = 0;
float rnd3 = 0;
float character = 0;
float block_ = 0;
float char_count = 0;
float word_count = 0;
float spawnflags_ = 0;
float digit = 0;
float exponent = 0;
float previous_exponent = 0;
gedict_t *target_ = 0;
float traveltime = 0;
float traveltime2 = 0;
float traveltime3 = 0;
float look_traveltime = 0;
float look_traveltime_squared = 0;
gedict_t *flag_self = 0;
float flag_pos = 0;
gedict_t *flag1 = 0;
gedict_t *flag2 = 0;
gedict_t *tfog = 0;
vec3_t weapons_vel =
	{ 0 };
gedict_t *death = 0;
gedict_t *trace_ent1 = 0;
gedict_t *trace_ent2 = 0;
gedict_t *old_self = 0;
gedict_t *old_other = 0;
float forward = 0;
vec3_t start =
	{ 0 };
vec3_t end =
	{ 0 };
int description = 0;
float path_time = 0;
float component_speed = 0;
float dm = 0;
float count_ = 0;
gedict_t *array_sub_object_ = 0;
float bind_char = 0;
float dodge_factor = 0;
vec3_t hor_normal_vec =
	{ 0 };
vec3_t last_clear_angle =
	{ 0 };
vec3_t velocity_hor_angle =
	{ 0 };
gedict_t *to_zone = 0;
gedict_t *search_entity = 0;

qbool fb_lg_disabled(void)
{
	return (qbool)((int)cvar("k_disallow_weapons") & IT_LIGHTNING) != 0;
}

// taken from pr1 implementation
float pr1_rint(float f)
{
	if (f > 0)
	{
		return (int)(f + 0.5);
	}
	else
	{
		return (int)(f - 0.5);
	}
}

// match.qc
qbool HasWeapon(gedict_t *player, int weapon)
{
	return ((int)player->s.v.items & weapon);
}

qbool enemy_shaft_attack(gedict_t *self, gedict_t *enemy)
{
	if (enemy->ct != ctPlayer)
	{
		return false;
	}

	return (HasWeapon(enemy, IT_LIGHTNING) && enemy->s.v.ammo_cells && (self->fb.enemy_dist < 630)
			&& (g_globalvars.time < enemy->attack_finished));
}

qbool bots_enabled(void)
{
	return cvar(FB_CVAR_ENABLED) == 1;
}

static qbool HasRLOrLG(gedict_t *self)
{
	return ((((int)self->s.v.items & IT_ROCKET_LAUNCHER) && (self->s.v.ammo_rockets > 1))
			|| (((int)self->s.v.items & IT_LIGHTNING) && (self->s.v.ammo_cells > 5)));
}

static qbool EnemyHasRLorLG(gedict_t *self)
{
	gedict_t *enemy = &g_edicts[self->s.v.enemy];
	if (self->s.v.enemy == 0)
	{
		return false;
	}

	return ((((int)enemy->s.v.items & IT_ROCKET_LAUNCHER) && (enemy->s.v.ammo_rockets > 1))
			|| (((int)enemy->s.v.items & IT_LIGHTNING) && (enemy->s.v.ammo_cells > 5)));
}

static qbool IsDanger(gedict_t *self)
{
	gedict_t *enemy = &g_edicts[self->s.v.enemy];
	if (self->s.v.enemy == 0)
	{
		return false;
	}

	if ((self->s.v.health < enemy->s.v.health) && (self->s.v.armorvalue < enemy->s.v.armorvalue)
			&& (self->s.v.armortype < enemy->s.v.armortype)
			&& (self->fb.firepower < enemy->fb.firepower))
	{
		return true;
	}

	if (((int)enemy->s.v.items & (IT_INVULNERABILITY | IT_QUAD | IT_INVISIBILITY))
			&& (!((int)self->s.v.items & (IT_INVULNERABILITY | IT_INVISIBILITY))))
	{
		return false;
	}

	return false;
}

qbool EnemyDefenceless(gedict_t *self)
{
	if (!EnemyHasRLorLG(self) && HasRLOrLG(self))
	{
		return (!IsDanger(self) && (self->s.v.health > 50) && (self->s.v.armorvalue >= 50));
	}

	return false;
}

gedict_t* FirstZoneMarker(int zone)
{
	return zone_head[zone - 1];
}

void AddZoneMarker(gedict_t *marker)
{
	int zone = marker->fb.Z_ - 1;

	if ((zone < 0) || (zone >= NUMBER_ZONES))
	{
		return;
	}

	if (zone_tail[zone])
	{
		zone_tail[zone]->fb.Z_next = marker;
		zone_tail[zone] = marker;
	}
	else
	{
		zone_head[zone] = zone_tail[zone] = marker;
	}

	marker->fb.Z_head = zone_head[zone]; // FIXME: this can't be trustworthy in future?
}

#endif
