#include "g_local.h"

// Port of https://github.com/id-Software/quake-rerelease-qc/blob/main/quakec_hipnotic/hipcount.qc

// spawnflags
#define COUNTER_TOGGLE       1
#define COUNTER_LOOP         2
#define COUNTER_STEP         4
#define COUNTER_RESET        8
#define COUNTER_RANDOM      16
#define COUNTER_FINISHCOUNT 32
#define COUNTER_START_ON    64

static void counter_on_use(void);
static void counter_off_use(void);

static void counter_think(void)
{
    self->cnt = self->cnt + 1;
    if ((int) self->s.v.spawnflags & COUNTER_RANDOM)
    {
        self->state = g_random() * self->count;
        self->state = floor(self->state) + 1;
    }
    else
    {
        self->state = self->cnt;
    }

    activator = other;
    SUB_UseTargets();
    self->s.v.nextthink = g_globalvars.time + self->wait;

    if ((int) self->s.v.spawnflags & COUNTER_STEP )
    {
        counter_on_use();
    }

    if (self->cnt >= self->count) {
        self->cnt = 0;
        if ((self->aflag) || !((int) self->s.v.spawnflags & COUNTER_LOOP ))
        {
            if ((int) self->s.v.spawnflags & COUNTER_TOGGLE)
            {
                counter_on_use();
            }
            else
            {
                ent_remove(self);
            }
        }
    }
}

static void counter_on_use(void)
{
    if ((self->cnt != 0) && ((int) self->s.v.spawnflags & COUNTER_FINISHCOUNT ))
    {
        self->aflag = true;
        return;
    }

    self->use = (func_t) counter_off_use;
    self->think = (func_t) SUB_Null;
    self->aflag = false;
}

static void counter_off_use(void)
{
    self->aflag = false;
    if ((int) self->s.v.spawnflags & COUNTER_TOGGLE)
    {
        self->use = (func_t) counter_on_use;
    }
    else
    {
        self->use = (func_t) SUB_Null;
    }

    if ((int) self->s.v.spawnflags & COUNTER_RESET )
    {
        self->cnt = 0;
        self->state = 0;
    }
    self->think = (func_t) counter_think;
    if (self->delay) {
        self->s.v.nextthink = g_globalvars.time + self->delay;
    } else {
        counter_think();
    }
}

static float counter_GetCount(gedict_t *counter)
{
    if (streq(counter->classname, "counter")) {
        return counter->state;
    }
    return 0;
}

/*QUAKED func_counter (0 0 0.5) (0 0 0) (32 32 32) TOGGLE LOOP STEP RESET RANDOM FINISHCOUNT START_ON
TOGGLE causes the counter to switch between an on and off state
each time the counter is triggered.

LOOP causes the counter to repeat infinitly.  The count resets to zero
after reaching the value in "count".

STEP causes the counter to only increment when triggered.  Effectively,
this turns the counter into a relay with counting abilities.

RESET causes the counter to reset to 0 when restarted.

RANDOM causes the counter to generate random values in the range 1 to "count"
at the specified interval.

FINISHCOUNT causes the counter to continue counting until it reaches "count"
before shutting down even after being set to an off state.

START_ON causes the counter to be on when the level starts.

"count" specifies how many times to repeat the event.  If LOOP is set,
it specifies how high to count before reseting to zero.  Default is 10.

"wait"  the length of time between each trigger event. Default is 1 second.

"delay" how much time to wait before firing after being switched on.
*/
void SP_func_counter(void)
{
    if (!self->wait) {
        self->wait = 1;
    }

    self->count = floor(self->count);
    if (self->count <= 0) {
        self->count = 10;
    }
    self->cnt = 0;
    self->state = 0;

    self->classname = "counter";
    self->use = (func_t) counter_off_use;
    self->think = (func_t) SUB_Null;
    if ((int) self->s.v.spawnflags & COUNTER_START_ON )
    {
        self->think = (func_t) counter_off_use;
        self->s.v.nextthink = g_globalvars.time + 0.1f;
    }
}

static void oncount_use(void)
{
    if (counter_GetCount(other) == self->count) {
        activator = other;
        SUB_UseTargets();
    }
}

/*QUAKED func_oncount (0 0 0.5) (0 0 0) (16 16 16)
Must be used as the target for func_counter.  When the counter
reaches the value set by count, func_oncount triggers its targets.

"count" specifies the value to trigger on.  Default is 1.

"delay" how much time to wait before firing after being triggered.
*/

void SP_func_oncount(void)
{
    self->count = floor(self->count);
    if (self->count <= 0) {
        self->count = 1;
    }

    self->classname = "oncount";
    self->use = (func_t) oncount_use;
    self->think = (func_t) SUB_Null;
}
