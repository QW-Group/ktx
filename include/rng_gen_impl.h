// rng_gen_impl.h -- Xoroshiro128** implementation.

#ifndef __RNG_GEN_IMPL_H__
#define __RNG_GEN_IMPL_H__

#include <stdint.h>

void rng_gen_impl_set_initial_state(uint32_t, uint32_t, uint32_t, uint32_t);
uint32_t rng_gen_impl_next(void);

#endif
