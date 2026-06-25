// This code is a line by line port of the re-release hiprot QuakeC code.
// https://github.com/id-Software/quake-rerelease-qc/blob/main/quakec_hipnotic/hiprot.qc

#include "g_local.h"

#define STATE_ACTIVE      0
#define STATE_INACTIVE    1
#define STATE_SPEEDINGUP  2
#define STATE_SLOWINGDOWN 3

#define STATE_CLOSED   4
#define STATE_OPEN     5
#define STATE_OPENING  6
#define STATE_CLOSING  7

#define STATE_WAIT 0
#define STATE_MOVE 1
#define STATE_STOP 2
#define STATE_FIND 3
#define STATE_NEXT 4

#define OBJECT_ROTATE    0
#define OBJECT_MOVEWALL  1
#define OBJECT_SETORIGIN 2

#define TOGGLE   1
#define START_ON 2

#define ROTATION    1
#define ANGLES      2
#define STOP        4
#define NO_ROTATE   8
#define DAMAGE     16
#define MOVETIME   32
#define SET_DAMAGE 64

#define VISIBLE     1
#define TOUCH       2
#define NONBLOCKING 4

#define STAYOPEN 1

/*QUAKED info_rotate (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as the point of rotation for rotatable objects.
*/
void SP_info_rotate(void)
{
	// remove self after a little while, to make sure that entities that
	// have targeted it have had a chance to spawn
	self->s.v.nextthink = g_globalvars.time + 2;
	self->think = (func_t) SUB_Remove;
}

static void RotateTargets(void)
{
	gedict_t *ent;
	vec3_t vx, vy, vz, org;

	trap_makevectors(self->s.v.angles);

	ent = find(world, FOFS(targetname), self->target);
	while (ent)
	{
		VectorCopy(ent->s.v.oldorigin, org);
		VectorScale(g_globalvars.v_forward, org[0], vx);
		VectorScale(g_globalvars.v_right, org[1], vy);
		VectorScale(vy, -1, vy);
		VectorScale(g_globalvars.v_up, org[2], vz);
		VectorAdd(vx, vy, ent->neworigin);
		VectorAdd(ent->neworigin, vz, ent->neworigin);

		if (ent->rotate_type == OBJECT_SETORIGIN)
		{
			VectorAdd(ent->neworigin, self->s.v.origin, org);
			setorigin(ent, PASSVEC3(org));
		}
		else if ( ent->rotate_type == OBJECT_ROTATE)
		{
			VectorCopy(self->s.v.angles, ent->s.v.angles);
			VectorAdd(ent->neworigin, self->s.v.origin, org);
			setorigin(ent, PASSVEC3(org));
		}
		else
		{
			VectorSubtract(self->s.v.origin, self->s.v.oldorigin, org);
			VectorAdd(org, ent->neworigin, org);
			VectorSubtract(org, ent->s.v.oldorigin, ent->neworigin);

			VectorSubtract(ent->neworigin, ent->s.v.origin, org);
			VectorScale(org, 25, ent->s.v.velocity);
		}
		ent = find(ent, FOFS(targetname), self->target);
	}
}

static void RotateTargetsFinal(void)
{
	gedict_t *ent;

	ent = find(world, FOFS(targetname), self->target);
	while (ent)
	{
		SetVector(ent->s.v.velocity, 0, 0, 0);
		if (ent->rotate_type == OBJECT_ROTATE)
		{
			VectorCopy(self->s.v.angles, ent->s.v.angles);
		}
		ent = find(ent, FOFS(targetname), self->target);
	}
}

static void SetTargetOrigin(void)
{
	gedict_t *ent;
	vec3_t org;

	ent = find(world, FOFS(targetname), self->target);
	while (ent)
	{
		if (ent->rotate_type == OBJECT_MOVEWALL)
		{
			VectorSubtract(self->s.v.origin, self->s.v.oldorigin, org);
			VectorAdd(org, ent->neworigin, org);
			VectorSubtract(org, ent->s.v.oldorigin, org);
			setorigin(ent, PASSVEC3(org));
		}
		else
		{
			VectorAdd(ent->neworigin, self->s.v.origin, org);
			setorigin(ent, PASSVEC3(org));
		}
		ent = find(ent, FOFS(targetname), self->target);
	}
}

static void LinkRotateTargets(void)
{
	gedict_t *ent;
	vec3_t tempvec;

	VectorCopy(self->s.v.origin, self->s.v.oldorigin);

	ent = find(world, FOFS(targetname), self->target);
	while (ent)
	{
		if (streq(ent->classname, "rotate_object"))
		{
			ent->rotate_type = OBJECT_ROTATE;
			VectorSubtract(ent->s.v.origin, self->s.v.oldorigin, ent->s.v.oldorigin);
			VectorSubtract(ent->s.v.origin, self->s.v.oldorigin, ent->neworigin);
			ent->s.v.owner = EDICT_TO_PROG(self);
		}
		else if (streq(ent->classname, "func_movewall"))
		{
			ent->rotate_type = OBJECT_MOVEWALL;
			VectorAdd(ent->s.v.absmin, ent->s.v.absmax, tempvec);
			VectorScale(tempvec, 0.5f, tempvec);

			VectorSubtract(tempvec, self->s.v.oldorigin, ent->s.v.oldorigin);
			VectorCopy(ent->s.v.oldorigin, ent->neworigin);
			ent->s.v.owner = EDICT_TO_PROG(self);
		}
		else
		{
			ent->rotate_type = OBJECT_SETORIGIN;
			VectorSubtract(ent->s.v.origin, self->s.v.oldorigin, ent->s.v.oldorigin);
			VectorSubtract(ent->s.v.origin, self->s.v.oldorigin, ent->neworigin);
		}
		ent = find(ent, FOFS(targetname), self->target);
	}
}

static void HurtSetDamage(gedict_t *ent, float amount)
{
	ent->dmg = amount;
	if (!amount)
	{
		ent->s.v.solid = SOLID_NOT;
	}
	else
	{
		ent->s.v.solid = SOLID_TRIGGER;
	}
	ent->s.v.nextthink = -1;
}

static void SetDamageOnTargets(float amount)
{
	gedict_t *ent;

	ent = find(world, FOFS(targetname), self->target);
	while (ent)
	{
		if (streq(ent->classname, "trigger_hurt"))
		{
			HurtSetDamage(ent, amount);
		}
		else if (streq(ent->classname, "func_movewall"))
		{
			ent->dmg = amount;
		}

		ent = find(ent, FOFS(targetname), self->target);
	}
}


//************************************************
//
// Simple continual rotatation
//
//************************************************

static void SUB_NormalizeAngles(vec3_t v)
{
	v[0] = (float) fmod(v[0], 360.0);
	v[1] = (float) fmod(v[1], 360.0);
	v[2] = (float) fmod(v[2], 360.0);
}

static void rotate_entity_think(void)
{
	vec3_t delta;
	float t;

	t = g_globalvars.time - self->s.v.ltime;
	self->s.v.ltime = g_globalvars.time;

	if (self->state == STATE_SPEEDINGUP)
	{
		self->count = self->count + self->cnt * t;
		if (self->count > 1)
		{
			self->count = 1;
		}

		// get rate of rotation
		t = t * self->count;
	}
	else if (self->state == STATE_SLOWINGDOWN)
	{
		self->count = self->count - self->cnt * t;
		if (self->count < 0)
		{
			RotateTargetsFinal();
			self->state = STATE_INACTIVE;
			self->think = (func_t) SUB_Null;
			return;
		}

		// get rate of rotation
		t = t * self->count;
	}

	VectorScale(self->rotate, t, delta);
	VectorAdd(self->s.v.angles, delta, self->s.v.angles);
	SUB_NormalizeAngles(self->s.v.angles);
	RotateTargets();
	self->s.v.nextthink = g_globalvars.time + g_globalvars.frametime;
}

static void rotate_entity_use(void)
{
	// change to alternate textures
	self->s.v.frame = 1 - self->s.v.frame;

	if (self->state == STATE_ACTIVE)
	{
		if ((int) self->s.v.spawnflags & TOGGLE)
		{
			if (self->speed)
			{
				self->count = 1;
				self->state = STATE_SLOWINGDOWN;
			}
			else
			{
				self->state = STATE_INACTIVE;
				self->think = (func_t) SUB_Null;
			}
		}
	}
	else if (self->state == STATE_INACTIVE)
	{
		self->think = (func_t) rotate_entity_think;
		self->s.v.nextthink = g_globalvars.time + g_globalvars.frametime;
		self->s.v.ltime = g_globalvars.time;
		if (self->speed)
		{
			self->count = 0;
			self->state = STATE_SPEEDINGUP;
		}
		else
		{
			self->state = STATE_ACTIVE;
		}
	}
	else if (self->state == STATE_SPEEDINGUP)
	{
		if ((int) self->s.v.spawnflags & TOGGLE)
		{
			self->state = STATE_SLOWINGDOWN;
		}
	}
	else
	{
		self->state = STATE_SPEEDINGUP;
	}
}

static void rotate_entity_firstthink(void)
{
	LinkRotateTargets();
	if ((int) self->s.v.spawnflags & START_ON )
	{
		self->state = STATE_ACTIVE;
		self->think = (func_t) rotate_entity_think;
		self->s.v.nextthink = g_globalvars.time + g_globalvars.frametime;
		self->s.v.ltime = g_globalvars.time;
	}
	else
	{
		self->state = STATE_INACTIVE;
		self->think = (func_t) SUB_Null;
	}
	self->use = (func_t) rotate_entity_use;
}

/*QUAKED func_rotate_entity (0 .5 .8) (-8 -8 -8) (8 8 8) TOGGLE START_ON
Creates an entity that continually rotates.	 Can be toggled on and
off if targeted.

TOGGLE = allows the rotation to be toggled on/off

START_ON = wether the entity is spinning when spawned.	If TOGGLE is 0, entity can be turned on, but not off.

If "deathtype" is set with a string, this is the message that will appear when a player is killed by the train.

"rotate" is the rate to rotate.
"target" is the center of rotation.
"speed"	 is how long the entity takes to go from standing still to full speed and vice-versa.
*/

void SP_func_rotate_entity(void)
{
	self->s.v.solid = SOLID_NOT;
	self->s.v.movetype = MOVETYPE_NONE;

	setmodel(self, self->model);
	setsize(self, PASSVEC3(self->s.v.mins), PASSVEC3(self->s.v.maxs));

	if (self->speed != 0 )
	{
		self->cnt = 1 / self->speed;
	}

	self->think = (func_t) rotate_entity_firstthink;
	self->s.v.nextthink = g_globalvars.time + g_globalvars.frametime;
	self->s.v.ltime = g_globalvars.time;
}

//************************************************
//
// Train with rotation functionality
//
//************************************************

/*QUAKED path_rotate (0.5 0.3 0) (-8 -8 -8) (8 8 8) ROTATION ANGLES STOP NO_ROTATE DAMAGE MOVETIME SET_DAMAGE
 Path for rotate_train.

 ROTATION tells train to rotate at rate specified by "rotate".	Use '0 0 0' to stop rotation.

 ANGLES tells train to rotate to the angles specified by "angles" while traveling to this path_rotate.	Use values < 0 or > 360 to guarantee that it turns in a certain direction.	Having this flag set automatically clears any rotation.

 STOP tells the train to stop and wait to be retriggered.

 NO_ROTATE tells the train to stop rotating when waiting to be triggered.

 DAMAGE tells the train to cause damage based on "dmg".

 MOVETIME tells the train to interpret "speed" as the length of time to take moving from one corner to another.

 SET_DAMAGE tells the train to set all targets damage to "dmg"

 "noise" contains the name of the sound to play when train stops.
 "noise1" contains the name of the sound to play when train moves.
 "event" is a target to trigger when train arrives at path_rotate.
*/
void SP_path_rotate(void)
{
	if (self->noise)
	{
		trap_precache_sound(self->noise);
	}
	if (self->noise1)
	{
		trap_precache_sound(self->noise1);
	}
}


static void rotate_train_next(void);
static void rotate_train_find(void);

static void rotate_train_think(void)
{
	float t, timeelapsed;
	vec3_t delta;

	t = g_globalvars.time - self->s.v.ltime;
	self->s.v.ltime = g_globalvars.time;

	if (self->endtime && (g_globalvars.time >= self->endtime))
	{
		self->endtime = 0;
		if (self->state == STATE_MOVE)
		{
			setorigin(self, PASSVEC3(self->finaldest));
			SetVector(self->s.v.velocity, 0, 0, 0);
		}

		if (self->think1)
		{
			self->think1();
		}
	}
	else
	{
		timeelapsed = (g_globalvars.time - self->cnt) * self->duration;
		if (timeelapsed > 1)
		{
			timeelapsed = 1;
		}
		VectorScale(self->dest2, timeelapsed, delta);
		VectorAdd(self->dest1, delta, delta);
		setorigin(self, PASSVEC3(delta));
	}

	VectorScale(self->rotate, t, delta);
	VectorAdd(self->s.v.angles, delta, self->s.v.angles);
	SUB_NormalizeAngles(self->s.v.angles);

	RotateTargets();

	self->s.v.nextthink = g_globalvars.time + g_globalvars.frametime;
}

static void rotate_train_use(void)
{
	if (self->think1 != rotate_train_find)
	{
		if (VectorLength(self->s.v.velocity))
		{
			return; // already activated
		}
		if (self->think1)
		{
			self->think1();
		}
	}
}

static void rotate_train_wait(void)
{
	gedict_t *goalentity = PROG_TO_EDICT(self->s.v.goalentity);
	self->state = STATE_WAIT;

	if (!strnull(goalentity->noise))
	{
		sound(self, CHAN_VOICE, goalentity->noise, 1, ATTN_NORM);
	}
	else
	{
		sound(self, CHAN_VOICE, self->noise, 1, ATTN_NORM);
	}
	if ((int) goalentity->s.v.spawnflags & ANGLES)
	{
		SetVector(self->rotate, 0, 0, 0);
		VectorCopy(self->finalangle, self->s.v.angles);
	}
	if ((int) goalentity->s.v.spawnflags & NO_ROTATE)
	{
		SetVector(self->rotate, 0, 0, 0);
	}
	self->endtime = self->s.v.ltime + goalentity->wait;
	self->think1 = rotate_train_next;
}

static void rotate_train_stop(void)
{
	gedict_t *goalentity = PROG_TO_EDICT(self->s.v.goalentity);

	self->state = STATE_STOP;

	if (!strnull(goalentity->noise))
	{
		sound(self, CHAN_VOICE, goalentity->noise, 1, ATTN_NORM);
	}
	else
	{
		sound(self, CHAN_VOICE, self->noise, 1, ATTN_NORM);
	}
	if ((int) goalentity->s.v.spawnflags & ANGLES)
	{
		SetVector(self->rotate, 0, 0, 0);
		VectorCopy(self->finalangle, self->s.v.angles);
	}
	if ((int) goalentity->s.v.spawnflags & NO_ROTATE)
	{
		SetVector(self->rotate, 0, 0, 0);
	}

	self->dmg = 0;
	self->think1 = rotate_train_next;
}

static void rotate_train_next(void)
{
	gedict_t *targ, *current, *goalentity;
	vec3_t vdestdelta;
	float len, traintraveltime, div;
	char *temp;

	self->state = STATE_NEXT;

	goalentity = PROG_TO_EDICT(self->s.v.goalentity);
	current = goalentity;
	targ = find (world, FOFS(targetname), self->path);
	if (!streq(targ->classname, "path_rotate"))
	{
		G_Error( "Next target is not path_rotate");
	}

	if (goalentity->noise1)
	{
		self->noise1 = goalentity->noise1;
	}

	sound(self, CHAN_VOICE, self->noise1, 1, ATTN_NORM);

	self->s.v.goalentity = EDICT_TO_PROG(targ);
	self->path = targ->target;
	if (strnull(self->path))
		G_Error("rotate_train_next: no next target");

	if ((int) targ->s.v.spawnflags & STOP)
	{
		self->think1 = rotate_train_stop;
	}
	else if (targ->wait)
	{
		self->think1 = rotate_train_wait;
	}
	else
	{
		self->think1 = rotate_train_next;
	}

	if (current->event)
	{
		// Trigger any events that should happen at the corner.
		temp = self->target;
		self->target = current->event;
		self->message = current->message;
		SUB_UseTargets();
		self->target = temp;
		self->message = NULL;
	}

	if ((int) current->s.v.spawnflags & ANGLES)
	{
		SetVector(self->rotate, 0, 0, 0);
		VectorCopy(self->finalangle, self->s.v.angles);
	}

	if ((int) current->s.v.spawnflags & ROTATION)
	{
		VectorCopy(current->rotate, self->rotate);
	}

	if ((int) current->s.v.spawnflags & DAMAGE)
	{
		self->dmg = current->dmg;
	}

	if ((int) current->s.v.spawnflags & SET_DAMAGE)
	{
		SetDamageOnTargets( current->dmg);
	}

	if (current->speed == -1 )
	{
		// Warp to the next path_corner
		setorigin( self, PASSVEC3(targ->s.v.origin));
		self->endtime = self->s.v.ltime + g_globalvars.frametime;
		SetTargetOrigin();

		if ((int) targ->s.v.spawnflags & ANGLES)
		{
			VectorCopy(targ->s.v.angles, self->s.v.angles);
		}

		self->duration = 1;                        // 1 / duration
		self->cnt = g_globalvars.time;             // start time
		SetVector(self->dest2, 0, 0, 0);           // delta
		VectorCopy(self->s.v.origin, self->dest1); // original position
		VectorCopy(self->s.v.origin, self->finaldest);
	}
	else
	{
		self->state = STATE_MOVE;

		VectorCopy(targ->s.v.origin, self->finaldest);
		if (VectorCompare(self->finaldest, self->s.v.origin))
		{
			SetVector(self->s.v.velocity, 0, 0, 0);
			self->endtime = self->s.v.ltime + 0.1f;

			self->duration = 1;                        // 1 / duration
			self->cnt = g_globalvars.time;             // start time
			SetVector(self->dest2, 0, 0, 0);           // delta
			VectorCopy(self->s.v.origin, self->dest1); // original position
			VectorCopy(self->s.v.origin, self->finaldest);
			return;
		}
		// set destdelta to the vector needed to move
		VectorSubtract(self->finaldest, self->s.v.origin, vdestdelta);

		// calculate length of vector
		len = vlen (vdestdelta);

		if ((int) current->s.v.spawnflags & MOVETIME)
		{
			traintraveltime = current->speed;
		}
		else
		{
			// check if there's a speed change
			if (current->speed > 0)
			{
				self->speed = current->speed;
			}

			if (!self->speed)
			{
				G_Error("No speed is defined!");
			}

			// divide by speed to get time to reach dest
			traintraveltime = len / self->speed;
		}

		if (traintraveltime < 0.1f)
		{
			SetVector(self->s.v.velocity, 0, 0, 0);
			self->endtime = self->s.v.ltime + 0.1f;
			if ((int) targ->s.v.spawnflags & ANGLES)
			{
				VectorCopy(targ->s.v.angles, self->s.v.angles);
			}
			return;
		}

		// qcc won't take vec/float
		div = 1.0f / traintraveltime;

		if ((int) targ->s.v.spawnflags & ANGLES)
		{
			VectorCopy(targ->s.v.angles, self->finalangle);
			SUB_NormalizeAngles(self->finalangle);

			VectorSubtract(targ->s.v.angles, self->s.v.angles, self->rotate);
			VectorScale(self->rotate, div, self->rotate);
		}

		// set endtime to trigger a think when dest is reached
		self->endtime = self->s.v.ltime + traintraveltime;

		// scale the destdelta vector by the time spent traveling to get velocity
		VectorScale(vdestdelta, div, self->s.v.velocity);

		self->duration = div;                      // 1 / duration
		self->cnt = g_globalvars.time;             // start time
		VectorCopy(vdestdelta, self->dest2);       // delta
		VectorCopy(self->s.v.origin, self->dest1); // original position
	}
}

static void rotate_train_find(void)
{
	gedict_t *targ;

	self->state = STATE_FIND;

	LinkRotateTargets();

	// the first target is the point of rotation.
	// the second target is the path.
	targ = find (world, FOFS(targetname), self->path);
	if (!streq(targ->classname, "path_rotate"))
	{
		G_Error("Next target is not path_rotate");
	}

	// Save the current entity
	self->s.v.goalentity = EDICT_TO_PROG(targ);

	if ((int) targ->s.v.spawnflags & ANGLES)
	{
		VectorCopy(targ->s.v.angles, self->s.v.angles);
		SUB_NormalizeAngles(targ->s.v.angles);
		VectorCopy(targ->s.v.angles, self->finalangle);
	}

	self->path = targ->target;
	setorigin (self, PASSVEC3(targ->s.v.origin));
	SetTargetOrigin();
	RotateTargetsFinal();
	self->think1 = rotate_train_next;
	if (strnull(self->targetname))
	{
		// not triggered, so start immediately
		self->endtime = self->s.v.ltime + 0.1f;
	}
	else
	{
		self->endtime = 0;
	}

	self->duration = 1;                        // 1 / duration
	self->cnt = g_globalvars.time;             // start time
	SetVector(self->dest2, 0, 0, 0);           // delta
	VectorCopy(self->s.v.origin, self->dest1); // original position
}

/*QUAKED func_rotate_train (0 .5 .8) (-8 -8 -8) (8 8 8)
In path_rotate, set speed to be the new speed of the train after it reaches
the path change.  If speed is -1, the train will warp directly to the next
path change after the specified wait time.  If MOVETIME is set on the
path_rotate, the train to interprets "speed" as the length of time to
take moving from one corner to another.

"noise" contains the name of the sound to play when train stops.
"noise1" contains the name of the sound to play when train moves.
Both "noise" and "noise1" defaults depend upon "sounds" variable and
can be overridden by the "noise" and "noise1" variable in path_rotate.

Also in path_rotate, if STOP is set, the train will wait until it is
retriggered before moving on to the next goal.

Trains are moving platforms that players can ride.
"path" specifies the first path_rotate and is the starting position.
If the train is the target of a button or trigger, it will not begin moving until activated.
The func_rotate_train entity is the center of rotation of all objects targeted by it.

If "deathtype" is set with a string, this is the message that will appear when a player is killed by the train.

speed	default 100
dmg		 default  0
sounds
1) ratchet metal
*/

void SP_func_rotate_train(void)
{
	if (!self->speed)
	{
		self->speed = 100;
	}

	if (!self->target)
	{
		G_Error ("rotate_train without a target");
	}

	if (!self->noise)
	{
		if (self->s.v.sounds == 0)
		{
			self->noise = ("misc/null.wav");
		}

		if (self->s.v.sounds == 1)
		{
			self->noise = ("plats/train2.wav");
		}
	}
	if (!self->noise1)
	{
		if (self->s.v.sounds == 0)
		{
			self->noise1 = ("misc/null.wav");
		}
		if (self->s.v.sounds == 1)
		{
			self->noise1 = ("plats/train1.wav");
		}
	}

	trap_precache_sound( self->noise );
	trap_precache_sound( self->noise1 );

	self->cnt = 1;
	self->s.v.solid	 = SOLID_NOT;
	self->s.v.movetype = MOVETYPE_STEP;
	self->use = (func_t) rotate_train_use;

	setmodel (self, self->model);
	setsize (self, PASSVEC3(self->s.v.mins), PASSVEC3(self->s.v.maxs));
	setorigin (self, PASSVEC3(self->s.v.origin));

	// start trains on the second frame, to make sure their targets have had
	// a chance to spawn
	self->s.v.ltime = g_globalvars.time;
	self->s.v.nextthink = self->s.v.ltime + 0.1f;
	self->endtime = self->s.v.ltime + 0.1f;
	self->think = (func_t) rotate_train_think;
	self->think1 = rotate_train_find;
	self->state = STATE_FIND;

	self->duration = 1;						   // 1 / duration
	self->cnt = 0.1f;						   // start time
	SetVector(self->dest2, 0, 0, 0);		   // delta
	VectorCopy(self->s.v.origin, self->dest1); // original position

	self->s.v.flags = (float)((int) self->s.v.flags | FL_ONGROUND);
}

//************************************************
//
// Moving clip walls
//
//************************************************

static void rotate_door_reversedirection(void);
static void rotate_door_group_reversedirection(void);

static void movewall_touch(void)
{
	gedict_t *owner = PROG_TO_EDICT(self->s.v.owner);

	if (g_globalvars.time < owner->attack_finished)
	{
		return;
	}

	if (self->dmg)
	{
		T_Damage (other, self, owner, self->dmg);
		owner->attack_finished = g_globalvars.time + 0.5f;
	}
	else if (owner->dmg)
	{
		T_Damage (other, self, owner, owner->dmg);
		owner->attack_finished = g_globalvars.time + 0.5f;
	}
}

static void movewall_blocked(void)
{
	gedict_t *owner = PROG_TO_EDICT(self->s.v.owner);
	gedict_t *temp;

	if (g_globalvars.time < owner->attack_finished)
	{
		return;
	}

	owner->attack_finished = g_globalvars.time + 0.5f;

	if (streq(owner->classname, "func_rotate_door"))
	{
		temp = self;
		self = owner;
		rotate_door_group_reversedirection();
		self = temp;
	}

	if ( self->dmg)
	{
		T_Damage (other, self, owner, self->dmg);
		owner->attack_finished = g_globalvars.time + 0.5f;
	}
	else if (owner->dmg)
	{
		T_Damage (other, self, owner, owner->dmg);
		owner->attack_finished = g_globalvars.time + 0.5f;
	}
}

static void movewall_think(void)
{
	self->s.v.ltime = g_globalvars.time;
	self->s.v.nextthink = g_globalvars.time + g_globalvars.frametime;
}

/*QUAKED func_movewall (0 .5 .8) ? VISIBLE TOUCH NONBLOCKING
Used to emulate collision on rotating objects.

VISIBLE causes brush to be displayed.

TOUCH specifies whether to cause damage when touched by player.

NONBLOCKING makes the brush non-solid.	This is useless if VISIBLE is set.

"dmg" specifies the damage to cause when touched or blocked.
*/
void SP_func_movewall(void)
{
	SetVector(self->s.v.angles, 0, 0, 0);
	self->s.v.movetype = MOVETYPE_PUSH;
	if ((int) self->s.v.spawnflags & NONBLOCKING)
	{
		self->s.v.solid = SOLID_NOT;
	}
	else
	{
		self->s.v.solid = SOLID_BSP;
		self->blocked = (func_t) movewall_blocked;
	}
	if ((int) self->s.v.spawnflags & TOUCH)
	{
		self->touch = (func_t) movewall_touch;
	}
	setmodel(self, self->model);
	if (!((int) self->s.v.spawnflags & VISIBLE))
	{
		self->model = NULL;
	}
	self->think = (func_t) movewall_think;
	self->s.v.nextthink = g_globalvars.time + g_globalvars.frametime;
	self->s.v.ltime = g_globalvars.time;
}

/*QUAKED rotate_object (0 .5 .8) ?
This defines an object to be rotated.  Used as the target of func_rotate_door.
*/
void SP_rotate_object(void)
{
	self->classname = "rotate_object";
	self->s.v.solid = SOLID_NOT;
	self->s.v.movetype = MOVETYPE_NONE;
	setmodel(self, self->model);
	setsize(self, PASSVEC3(self->s.v.mins), PASSVEC3(self->s.v.maxs));
	self->think = (func_t) SUB_Null;
};

//************************************************
//
// Rotating doors
//
//************************************************

static void rotate_door_think2(void)
{
	self->s.v.ltime = g_globalvars.time;

	// change to alternate textures
	self->s.v.frame = 1 - self->s.v.frame;

	VectorCopy(self->dest, self->s.v.angles);

	if (self->state == STATE_OPENING)
	{
		self->state = STATE_OPEN;
	}
	else
	{
		if ((int) self->s.v.spawnflags & STAYOPEN )
		{
			rotate_door_group_reversedirection();
			return;
		}
		self->state = STATE_CLOSED;
	}

	sound(self, CHAN_VOICE, self->noise3, 1, ATTN_NORM);
	self->think = (func_t) SUB_Null;

	RotateTargetsFinal();
}

static void rotate_door_think(void)
{
	float t;
	vec3_t delta;

	t = g_globalvars.time - self->s.v.ltime;
	self->s.v.ltime = g_globalvars.time;

	if (g_globalvars.time < self->endtime )
	{
		VectorScale(self->rotate, t, delta);
		VectorAdd(self->s.v.angles, delta, self->s.v.angles);
		RotateTargets();
	}
	else
	{
		VectorCopy(self->dest, self->s.v.angles);
		RotateTargets();
		self->think = (func_t) rotate_door_think2;
	}

	self->s.v.nextthink = g_globalvars.time + g_globalvars.frametime;
}

static void rotate_door_reversedirection(void)
{
	vec3_t start;

   // change to alternate textures
	self->s.v.frame = 1 - self->s.v.frame;

	if (self->state == STATE_CLOSING)
	{
		VectorCopy(self->dest1, start);
		VectorCopy(self->dest2, self->dest);
		self->state = STATE_OPENING;
	}
	else
	{
		VectorCopy(self->dest2, start);
		VectorCopy(self->dest1, self->dest);
		self->state = STATE_CLOSING;
	}

	sound(self, CHAN_VOICE, self->noise2, 1, ATTN_NORM);

	VectorSubtract(self->dest, start, self->rotate);
	VectorScale(self->rotate, 1.0f / self->speed, self->rotate);

	self->think = (func_t) rotate_door_think;
	self->s.v.nextthink = g_globalvars.time + g_globalvars.frametime;
	self->endtime = g_globalvars.time + self->speed - (self->endtime - g_globalvars.time);
	self->s.v.ltime = g_globalvars.time;
}

static void rotate_door_group_reversedirection(void)
{
	char *name;

	// tell all associated rotaters to reverse direction
	if (self->group)
	{
		name = self->group;
		self = find(world, FOFS(group), name);
		while (self)
		{
			rotate_door_reversedirection();
			self = find(self, FOFS(group), name);
		}
	}
	else
	{
		rotate_door_reversedirection();
	}
}

static void rotate_door_use(void)
{
	vec3_t start;

	if ((self->state != STATE_OPEN) && (self->state != STATE_CLOSED))
	{
		return;
	}

	if (!self->cnt)
	{
		self->cnt = 1;
		LinkRotateTargets();
	}

	// change to alternate textures
	self->s.v.frame = 1 - self->s.v.frame;

	if (self->state == STATE_CLOSED)
	{
		VectorCopy(self->dest1, start);
		VectorCopy(self->dest2, self->dest);
		self->state = STATE_OPENING;
	}
	else
	{
		VectorCopy(self->dest2, start);
		VectorCopy(self->dest1, self->dest);
		self->state = STATE_CLOSING;
	}

	sound(self, CHAN_VOICE, self->noise2, 1, ATTN_NORM);

	VectorSubtract(self->dest, start, self->rotate);
	VectorScale(self->rotate, 1.0f / self->speed, self->rotate);
	self->think = (func_t) rotate_door_think;
	self->s.v.nextthink = g_globalvars.time + g_globalvars.frametime;
	self->endtime = g_globalvars.time + self->speed;
	self->s.v.ltime = g_globalvars.time;
}


/*QUAKED func_rotate_door (0 .5 .8) (-8 -8 -8) (8 8 8) STAYOPEN
Creates a door that rotates between two positions around a point of
rotation each time it's triggered.

STAYOPEN tells the door to reopen after closing.  This prevents a trigger-
once door from closing again when it's blocked.

"dmg" specifies the damage to cause when blocked.  Defaults to 2.  Negative numbers indicate no damage.
"speed" specifies how the time it takes to rotate

"sounds"
1) medieval (default)
2) metal
3) base
*/

void SP_func_rotate_door(void)
{
	if (strnull(self->target))
	{
		G_Error("rotate_door without target.");
	}

	SetVector(self->dest1, 0, 0, 0);
	VectorCopy(self->s.v.angles, self->dest2);
	VectorCopy(self->dest1, self->s.v.angles);

	// default to 2 seconds
	if (!self->speed)
	{
		self->speed = 2;
	}

	self->cnt = 0;

	if (!self->dmg)
	{
		self->dmg = 2;
	}
	else if (self->dmg < 0)
	{
		self->dmg = 0;
	}

	if (self->s.v.sounds == 0)
	{
		self->s.v.sounds = 1;
	}

	if (self->s.v.sounds == 1)
	{
		trap_precache_sound("doors/latch2.wav");
		trap_precache_sound("doors/winch2.wav");
		trap_precache_sound("doors/drclos4.wav");
		self->noise1 = "doors/latch2.wav";
		self->noise2 = "doors/winch2.wav";
		self->noise3 = "doors/drclos4.wav";
	}
	if (self->s.v.sounds == 2)
	{
		trap_precache_sound("doors/airdoor1.wav");
		trap_precache_sound("doors/airdoor2.wav");
		self->noise2 = "doors/airdoor1.wav";
		self->noise1 = "doors/airdoor2.wav";
		self->noise3 = "doors/airdoor2.wav";
	}
	if (self->s.v.sounds == 3)
	{
		trap_precache_sound("doors/basesec1.wav");
		trap_precache_sound("doors/basesec2.wav");
		self->noise2 = "doors/basesec1.wav";
		self->noise1 = "doors/basesec2.wav";
		self->noise3 = "doors/basesec2.wav";
	}

	self->s.v.solid = SOLID_NOT;
	self->s.v.movetype = MOVETYPE_NONE;
	setmodel(self, self->model);
	setsize(self, PASSVEC3(self->s.v.mins), PASSVEC3(self->s.v.maxs));
	setorigin(self, PASSVEC3(self->s.v.origin));

	self->state = STATE_CLOSED;
	self->use = (func_t) rotate_door_use;
	self->think = (func_t) SUB_Null;
}
