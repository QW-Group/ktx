#include <stdint.h>

#include "rng.h"

#include "rng_gen_impl.h"
#include "rng_seed_impl.h"

/* Seed the PRNG.

   Since the main game loop only provides 4 bytes of entropy, but the
   xoroshiro128** PRNG expects 16 bytes of entropy, this function uses the
   splitmix64 algorithm to expand the seed. This approach is recommended by
   the authors of xoroshiro128**, as described here: https://prng.di.unimi.it .
*/
void rng_seed(int seed) {
	uint64_t x = (uint64_t)(seed);

	uint32_t a = (uint32_t)rng_seed_impl_next(&x);
	uint32_t b = (uint32_t)rng_seed_impl_next(&x);
	uint32_t c = (uint32_t)rng_seed_impl_next(&x);
	uint32_t d = (uint32_t)rng_seed_impl_next(&x);

	rng_gen_impl_set_initial_state(a, b, c, d);
}

/* Update the internal PRNG state and return the next value.

   Uses the xoroshiro128** algorithm.
*/
uint32_t rng_next(void) {
	return rng_gen_impl_next();
}

