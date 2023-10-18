#include <stdint.h>

#include "rng.h"

#include "rng_gen_impl.h"
#include "rng_seed_impl.h"

/* Seed the PRNG.

   Since the main game loop only provides 4 bytes of entropy, but the
   xoroshiro128** PRNG expects 16 bytes of entropy, this function uses the
   splitmix64 algorithm to expand the seed. This approach is recommended by
   the authors of xoroshiro128**, as described here: https://prng.di.unimi.it .

   You must initialize your RngStates with rng_seed/rng_seed_global!
   (The all-zeros state has bad statistics.)
*/
void rng_seed(RngState* s, int seed) {
	uint64_t x = (uint64_t)(seed);

	s->s[0] = (uint32_t)rng_seed_impl_next(&x);
	s->s[1] = (uint32_t)rng_seed_impl_next(&x);
	s->s[2] = (uint32_t)rng_seed_impl_next(&x);
	s->s[3] = (uint32_t)rng_seed_impl_next(&x);
}

uint32_t rng_next(RngState* state) {
	return rng_gen_impl_next(state);
}

// You must initialize this with rng_seed/rng_seed_global!
static RngState global_rng = { 0 };

void rng_seed_global(int seed) {
	return rng_seed(&global_rng, seed);
}

uint32_t rng_next_global(void) {
	return rng_next(&global_rng);
}

