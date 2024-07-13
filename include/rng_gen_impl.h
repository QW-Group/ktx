// rng_gen_impl.h -- Xoroshiro128** implementation.

#ifndef __RNG_GEN_IMPL_H__
#define __RNG_GEN_IMPL_H__

#include "rng_gen_state.h"

RngState rng_gen_impl_initial_state(int seed);
unsigned int rng_gen_impl_next(RngState* state);

#endif
