// rng.h -- High-quality random number generation.

#ifndef __RNG_H__
#define __RNG_H__

#include "rng_gen_state.h"

void rng_seed(RngState*, int seed);
uint32_t rng_next(RngState*);

void rng_seed_global(int seed);
uint32_t rng_next_global(void);

#endif
