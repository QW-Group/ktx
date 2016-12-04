
#include "g_local.h"
#include "fb_globals.h"

qbool DM6DoorClosed (fb_path_eval_t* eval);
qbool BotDoorIsClosed (gedict_t* door);

#define PATH_NOISE_PENALTY 2.5
#define PATH_AVOID_PENALTY 2.5

#define G_bprint_debug(...) if (self->fb.debug) { G_bprint(__VA_ARGS__); }
#define STOP_DEBUGGING { self->fb.debug = false; }

static qbool DetectIncomingRocket(gedict_t* self, qbool rocket_alert, vec3_t marker_pos) {
	// if the path location is too close to an incoming rocket, 
	if (rocket_alert && VectorDistance(marker_pos, self->fb.rocket_endpos) < 200) {
		traceline(PASSVEC3(self->fb.rocket_endpos), PASSVEC3(marker_pos), true, self);

		return (g_globalvars.trace_fraction == 1);
	}

	return false;
}

static float EvalPath(fb_path_eval_t* eval, qbool allowRocketJumps, qbool trace_bprint, float current_goal_time, float current_goal_time_125) {
	float path_score;
	float same_dir;
	vec3_t marker_position;

	// don't try and pass through closed doors
	if (BotDoorIsClosed(eval->test_marker)) {
		G_bprint_debug (2, "> Door closed, ignoring...\n");
		return PATH_SCORE_NULL;
	}

	// Determine position
	VectorAdd (eval->test_marker->s.v.absmin, eval->test_marker->s.v.view_ofs, marker_position);

	// if it's a rocket jump, calculate path time (TODO: fix this for horizontal rocket jumps, also precalculate)
	// FIXME: /sv_maxspeed is wrong, should be based on velocity of flight
	if ((eval->description & ROCKET_JUMP) && allowRocketJumps) {
		vec3_t m_pos;

		VectorAdd(eval->touch_marker->s.v.absmin, eval->touch_marker->s.v.view_ofs, m_pos);

		eval->path_time = (VectorDistance(marker_position, m_pos) / sv_maxspeed);
	}

	//
	if (!eval->path_normal && !(eval->description & REVERSIBLE)) {
		G_bprint_debug (2, "> path not normal and not reversible, stopping\n");
		return PATH_SCORE_NULL;
	}

	// FIXME: Map specific logic
	if (DM6DoorClosed(eval)) {
		G_bprint_debug (2, "> DM6 Door Closed, stopping\n");
		return PATH_SCORE_NULL;
	}

	if ((eval->description & ROCKET_JUMP) && !allowRocketJumps) {
		G_bprint_debug (2, "> RJ required, stopping\n");
		return PATH_SCORE_NULL;
	}

	// FIXME: This code gives a bonus to routes carrying the player in the same direction
	{
		vec3_t direction_to_marker;

		VectorSubtract (marker_position, eval->player_origin, direction_to_marker);
		VectorNormalize (direction_to_marker);
		same_dir = DotProduct (eval->player_direction, direction_to_marker);
	}

	path_score = same_dir + g_random();
	self->fb.avoiding = (g_globalvars.time < eval->test_marker->fb.arrow_time) || DetectIncomingRocket(self, eval->rocket_alert, marker_position);
	G_bprint_debug (2, "> Temp path_score = %f\n", path_score);

	// If we'd pickup an item as we travel, negatively impact score
	if (eval->be_quiet && eval->test_marker->fb.pickup && ! WaitingToRespawn(eval->test_marker)) {
		if (eval->test_marker != eval->goalentity_marker) {
			if (eval->test_marker->fb.pickup()) {
				path_score -= PATH_NOISE_PENALTY;
				G_bprint_debug (2, "> be_quiet penalty, new path_score %f\n", path_score);
			}
		}
	}

	if (self->fb.avoiding) {
		path_score -= PATH_AVOID_PENALTY;
		G_bprint_debug (2, "> avoid penalty, final path_score %f\n", path_score);
	}
	else if (eval->goalentity_marker) {
		float total_goal_time;

		// Calculate time from marker > goal entity
		from_marker = eval->test_marker;
		path_normal = eval->path_normal;
		ZoneMarker (from_marker, eval->goalentity_marker, path_normal);
		traveltime = SubZoneArrivalTime (zone_time, middle_marker, eval->goalentity_marker);
		total_goal_time = eval->path_time + traveltime;
		G_bprint_debug (2, "> total_goal_time = %f + %f = %f\n", eval->path_time, traveltime, total_goal_time);

		if (total_goal_time > eval->goal_late_time) {
			G_bprint_debug (2, "> total_goal time > goal_late_time (%f)\n", eval->goal_late_time);
			if (traveltime < current_goal_time) {
				path_score += eval->lookahead_time_ - total_goal_time;
				G_bprint_debug (2, "> += %f - %f\n", eval->lookahead_time_, total_goal_time);
			}
			else if (total_goal_time > current_goal_time_125) {
				path_score -= total_goal_time;
				G_bprint_debug (2, "> -= %f\n", total_goal_time);
			}
		}
	}

	return path_score;
}

static void BotRouteEvaluation (qbool be_quiet, float lookahead_time, gedict_t* from_marker, gedict_t* to_marker, qbool rocket_alert, qbool rocket_jump_routes_allowed, vec3_t player_origin, vec3_t player_direction, qbool path_normal, qbool trace_bprint, float current_goal_time, float current_goal_time_125, float goal_late_time, float* best_score, gedict_t** next_marker, int* next_description, int* new_angle_hint)
{ 
	fb_path_eval_t eval = { 0 };
	int i = 0;

	eval.goalentity_marker = to_marker;
	VectorCopy (player_origin, eval.player_origin);
	VectorCopy (player_direction, eval.player_direction);
	eval.rocket_alert = rocket_alert;
	eval.path_normal = path_normal;
	eval.touch_marker = from_marker;
	eval.goal_late_time = goal_late_time;
	eval.lookahead_time_ = lookahead_time;
	eval.be_quiet = be_quiet;

	for (i = 0; i < NUMBER_PATHS; ++i) {
		eval.description = from_marker->fb.paths[i].flags;
		eval.path_time = from_marker->fb.paths[i].time;
		eval.test_marker = from_marker->fb.paths[i].next_marker;

		if (eval.test_marker) {
			float path_score = 0;

			G_bprint_debug (2, "Path from %d > %d\n", eval.touch_marker->fb.index, eval.test_marker->fb.index);
			path_score = EvalPath(&eval, rocket_jump_routes_allowed, trace_bprint, current_goal_time, current_goal_time_125);
			G_bprint_debug (2, ">> path score %f vs %f\n", path_score, *best_score);
			if (path_score > *best_score) {
				*best_score = path_score;
				*next_marker = eval.test_marker;
				*next_description = eval.description;
				*new_angle_hint = from_marker->fb.paths[i].angle_hint;
			}
		}
	}
}

void PathScoringLogic(
	float goal_respawn_time, qbool be_quiet, float lookahead_time, qbool path_normal, vec3_t player_origin, vec3_t player_direction, gedict_t* touch_marker_,
	gedict_t* goalentity_marker, qbool rocket_alert, qbool rocket_jump_routes_allowed,
	qbool trace_bprint, float *best_score, gedict_t** linked_marker_, int* new_path_state, int* new_angle_hint
)
{
	float current_goal_time = 0;
	float current_goal_time_125 = 0;
	float goal_late_time = 0;

	G_bprint_debug (2, "PathScoringLogic(\n");
	G_bprint_debug (2, "  goal_respawn_time = %f\n", goal_respawn_time);
	G_bprint_debug (2, "  path_normal = %s\n", path_normal ? "true" : "false");
	G_bprint_debug (2, "  player_origin = [%f %f %f]\n", PASSVEC3(player_origin));
	G_bprint_debug (2, "  touch_marker_ = %d (%s)\n", touch_marker_->fb.index, touch_marker_->s.v.classname);
	G_bprint_debug (2, "  goalentity_marker  = %d (%s)\n", (goalentity_marker ? goalentity_marker->fb.index : -1), goalentity_marker ? goalentity_marker->s.v.classname : "(null)");
	G_bprint_debug (2, "  rocket_alert = %s\n", rocket_alert ? "true" : "false");
	G_bprint_debug (2, "  rj_allowed = %s\n", rocket_jump_routes_allowed ? "true" : "false");
	G_bprint_debug (2, "  *best_score = %f\n", *best_score);
	G_bprint_debug (2, "  *linked_marker = %d (%s)\n", (*linked_marker_) ? (*linked_marker_)->fb.index : -1, (*linked_marker_) ? (*linked_marker_)->s.v.classname : "(null)");
	G_bprint_debug (2, ") = \n");

	if (goalentity_marker) {
		from_marker = touch_marker_;
		ZoneMarker (from_marker, goalentity_marker, path_normal);
		traveltime = SubZoneArrivalTime (zone_time, middle_marker, goalentity_marker);
		current_goal_time = traveltime;
		current_goal_time_125 = traveltime + 1.25;

		G_bprint_debug (2, "Init: goal_time %f, \n", current_goal_time);

		// FIXME: Estimating respawn times should be skill-based
		if (current_goal_time < 2.5) {
			goal_late_time = (goal_respawn_time - (g_random () * 5)) - g_globalvars.time;
		}
		else {
			goal_late_time = (goal_respawn_time - (g_random () * 10)) - g_globalvars.time;
		}
	}

	// Direct from touch marker to goal entity
	if (goalentity_marker && touch_marker_) {
		float path_score = 0;
		fb_path_eval_t eval = { 0 };

		eval.path_normal = path_normal;
		eval.rocket_alert = rocket_alert;
		eval.touch_marker = touch_marker_;
		eval.goalentity_marker = goalentity_marker;
		eval.goal_late_time = goal_late_time;
		eval.lookahead_time_ = lookahead_time;
		eval.be_quiet = be_quiet;
		VectorCopy (player_origin, eval.player_origin);
		VectorCopy (player_direction, eval.player_direction);

		eval.test_marker = touch_marker_;
		eval.description = 0;
		eval.path_time = 0;

		G_bprint_debug (2, "Marker > GoalEntity (%s) %d\n", goalentity_marker->s.v.classname, goalentity_marker->fb.index);
		path_score = EvalPath(&eval, rocket_jump_routes_allowed, trace_bprint, current_goal_time, current_goal_time_125);
		G_bprint_debug (2, ">> path score %f vs %f\n", path_score, *best_score);
		if (path_score > *best_score) {
			*best_score = path_score;
			*linked_marker_ = eval.test_marker;
			*new_path_state = eval.description;
			*new_angle_hint = 0;
		}
	}

	// Evaluate all paths from touched marker to the goal entity
	BotRouteEvaluation (be_quiet, lookahead_time, touch_marker_, goalentity_marker, rocket_alert, rocket_jump_routes_allowed, player_origin, player_direction, path_normal, trace_bprint, current_goal_time, current_goal_time_125, goal_late_time, best_score, linked_marker_, new_path_state, new_angle_hint);
}
