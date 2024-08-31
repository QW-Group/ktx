// rng_seed_impl.h -- SplitMix64 implementation.

#ifndef __RNG_SEED_IMPL_H__
#define __RNG_SEED_IMPL_H__

#ifndef Q3_VM
#include <stdint.h>
typedef uint64_t rng_seed_t;
#else
typedef unsigned int rng_seed_t;
#endif

rng_seed_t rng_seed_impl_next(rng_seed_t*);

#endif
