// Converted from .qc on 05/02/2016

#include "g_local.h"
#include "fb_globals.h"

static int marker_index = 0;

void InitialiseMarkerRoutes(void);

// FIXME: globals
extern gedict_t* markers[];

void AddToQue(gedict_t* ent) {
	markers[marker_index] = ent;
	ent->fb.index = marker_index++;
}

static gedict_t* spawn_marker(float x, float y, float z) {
	gedict_t* marker_ = spawn();
	marker_->s.v.classname = "marker";
	marker_->s.v.flags = FL_ITEM;
	BecomeMarker(marker_);
	marker_->s.v.origin[0] = pr1_rint(x);
	marker_->s.v.origin[1] = pr1_rint(y);
	marker_->s.v.origin[2] = pr1_rint(z);
	marker_->s.v.solid = SOLID_TRIGGER;
	marker_->s.v.touch = (func_t) marker_touch;
	if ( FrogbotOptionEnabled(FB_OPTION_SHOW_MARKERS) )
		setmodel( marker_, "progs/w_g_key.mdl" );
	VectorSet(marker_->s.v.view_ofs, 80, 80, 24);
	setsize (marker_, -65, -65, -24, 65, 65, 32);
	return marker_;
}

gedict_t* CreateMarker(float x, float y, float z) {
	gedict_t* marker = spawn_marker (x, y, z);

	AddToQue(marker);

	return marker;
}

void AllMarkersLoaded() {
	self = NULL;
	path_normal = true;

	InitialiseMarkerRoutes();
}

void SetMarkerPathFlags(int marker_number, int path_index, int flags) {
	--marker_number;

	if (marker_number < 0 || marker_number >= NUMBER_MARKERS) {
		return;
	}

	markers[marker_number]->fb.paths[path_index].flags = flags;
}

void SetMarkerPath(int source_marker, int path_index, int next_marker) {
	--source_marker;
	--next_marker;

	if (source_marker < 0 || source_marker >= NUMBER_MARKERS) {
		return;
	}
	if (next_marker < 0 || next_marker >= NUMBER_MARKERS) {
		return;
	}
	if (path_index < 0 || path_index >= NUMBER_PATHS) {
		return;
	}
	if (markers[source_marker] == NULL || markers[next_marker] == NULL) {
		Com_Printf ("Invalid path: %d to %d\n", source_marker, next_marker);
		return;
	}

	markers[source_marker]->fb.paths[path_index].next_marker = markers[next_marker];
}

void SetMarkerViewOffset (int marker, float zOffset)
{
	--marker;

	if (marker < 0 || marker >= NUMBER_MARKERS || markers[marker] == 0) {
		return;
	}

	markers[marker]->s.v.view_ofs[2] = zOffset;
}

qbool LoadBotRoutingFromFile (void)
{
	fileHandle_t file = -1;
	char lineData[128];
	char argument[128];
	
	// Load bot definition file: frogbots rely on objects spawning 
	//    markers, so be aware of alternative .ent files
	char* entityFile = cvar_string ("k_entityfile");
	if (!strnull (entityFile)) {
		file = std_fropen ("maps/%s.bot", entityFile);
		if (file == -1) {
			file = std_fropen ("bots/maps/%s.bot", entityFile);
		}
	}
	
	if (file == -1) {
		file = std_fropen ("maps/%s.bot", g_globalvars.mapname);
		if (file == -1) {
			file = std_fropen ("bots/maps/%s.bot", g_globalvars.mapname);
		}
	}
	
	if (file == -1) {
		return false;
	}

	while (std_fgets (file, lineData, sizeof (lineData))) {
		trap_CmdTokenize (lineData);

		trap_CmdArgv (0, argument, sizeof (argument));

		Com_Printf ("> %s\n", lineData);
		if (strnull (argument) || (argument[0] == '/' && argument[1] == '/'))
			continue;

		if (streq (argument, "CreateMarker")) {
			// CreateMarker %f %f %f
			float x, y, z;

			if (trap_CmdArgc () != 4)
				continue;

			trap_CmdArgv (1, argument, sizeof (argument));
			x = atof (argument);
			trap_CmdArgv (2, argument, sizeof (argument));
			y = atof (argument);
			trap_CmdArgv (3, argument, sizeof (argument));
			z = atof (argument);

			CreateMarker (x, y, z);
		}
		else if (streq (argument, "SetGoal")) {
			// SetGoal %d %d
			int marker, goal;

			if (trap_CmdArgc () != 3)
				continue;

			trap_CmdArgv (1, argument, sizeof (argument));
			goal = atoi (argument);
			trap_CmdArgv (2, argument, sizeof (argument));
			marker = atoi (argument);

			SetGoal (goal, marker);
		}
		else if (streq (argument, "SetZone")) {
			// SetZone %d %d
			int marker, zone;

			if (trap_CmdArgc () != 3)
				continue;

			trap_CmdArgv (1, argument, sizeof (argument));
			marker = atoi (argument);
			trap_CmdArgv (2, argument, sizeof (argument));
			zone = atoi (argument);

			SetZone (zone, marker);
		}
		else if (streq (argument, "SetMarkerPath")) {
			// SetMarkerPath %d %d %d
			int source_marker, path_number, next_marker;

			if (trap_CmdArgc () != 4)
				continue;

			trap_CmdArgv (1, argument, sizeof (argument));
			source_marker = atoi (argument);
			trap_CmdArgv (2, argument, sizeof (argument));
			path_number = atoi (argument);
			trap_CmdArgv (3, argument, sizeof (argument));
			next_marker = atoi (argument);

			SetMarkerPath (source_marker, path_number, next_marker);
		}
		else if (streq (argument, "SetMarkerPathFlags")) {
			// SetMarkerPathFlags %d %d %s
			int source_marker, path_number, path_flags;
			size_t i;

			if (trap_CmdArgc () != 4)
				continue;

			trap_CmdArgv (1, argument, sizeof (argument));
			source_marker = atoi (argument);
			trap_CmdArgv (2, argument, sizeof (argument));
			path_number = atoi (argument);
			trap_CmdArgv (3, argument, sizeof (argument));
			path_flags = 0;
			for (i = 0; i < strlen (argument); ++i) {
				switch (argument[i]) {
				case 'w':
					path_flags |= WATERJUMP_; break;
				case '6':
					path_flags |= DM6_DOOR; break;
				case 'r':
					path_flags |= ROCKET_JUMP; break;
				case 'j':
					path_flags |= JUMP_LEDGE; break;
				case 'v':
					path_flags |= VERTICAL_PLATFORM; break;
				}
			}

			SetMarkerPathFlags (source_marker, path_number, path_flags);
		}
		else if (streq (argument, "SetMarkerFlag")) {
			// SetMarkerFlag %d %s
			int source_marker, marker_flags;
			size_t i;

			if (trap_CmdArgc () != 3)
				continue;

			trap_CmdArgv (1, argument, sizeof (argument));
			source_marker = atoi (argument);
			trap_CmdArgv (2, argument, sizeof (argument));
			marker_flags = 0;
			for (i = 0; i < strlen (argument); ++i) {
				switch (argument[i]) {
				case 'u':
					marker_flags |= UNREACHABLE;
					break;
				case '6':
					marker_flags |= MARKER_IS_DM6_DOOR;
					break;
				case 't':
					marker_flags |= MARKER_DOOR_TOUCHABLE;
					break;
				case 'e':
					marker_flags |= MARKER_ESCAPE_ROUTE;
					break;
				}
			}

			SetMarkerFlag (source_marker, marker_flags);
		}
		else if (streq (argument, "SetMarkerViewOfs")) {
			// SetMarkerViewOfs %d %f
			int source_marker;
			float offset;

			if (trap_CmdArgc () != 3)
				continue;

			trap_CmdArgv (1, argument, sizeof (argument));
			source_marker = atoi (argument);
			trap_CmdArgv (2, argument, sizeof (argument));
			offset = atof (argument);

			SetMarkerViewOffset (source_marker, offset);
		}
	}

	std_fclose( file );
	return true;
}
