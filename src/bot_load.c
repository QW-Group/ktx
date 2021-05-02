// Converted from .qc on 05/02/2016

#include "g_local.h"
#include "fb_globals.h"

#if 0

void a_think()
{
}

void spawn_load()
{
	load_position = dropper;
	while (load_position)
	{
		if (!load_position->s.v.think)
		{
			load_position->s.v.nextthink = 0.001;
			load_position->s.v.think = dropper->s.v.think;
			load_position->fb.next_load = current_load_position;
			current_load_position = load_position;
		}

		load_position = nextent(load_position);
	}
	while (total_entity_count < 400)
	{
		load_position = spawn();
		load_position->s.v.classname = "load";
		load_position->s.v.nextthink = 0.001;
		load_position->s.v.think = dropper->s.v.think;
		load_position->fb.next_load = current_load_position;
		current_load_position = load_position;

		++total_entity_count;
	}
}
void set_load()
{
	load_position = current_load_position;
	while (load_position)
	{
		load_position->s.v.think = dropper->s.v.think;
		load_position = load_position->fb.next_load;
	}
}

void remove_load()
{
	gedict_t *marker_;

	time_start = g_globalvars.time;
	framecount_start = framecount;
	markers_loaded = true;
	/*
	 while (current_load_position) {
	 if (current_load_position->s.v.classname == "load") {
	 trap_remove(NUM_FOR_EDICT(current_load_position));
	 }
	 else  {
	 current_load_position->s.v.nextthink = 0;
	 }
	 current_load_position = current_load_position->fb.next_load;
	 }*/
	StartItems();

	for (marker_ = first_item; marker_ && marker_ != world; marker_ = marker_->fb.next)
	{
		AssignVirtualGoal_apply(marker_);
	}
}

#endif
