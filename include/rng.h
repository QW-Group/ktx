// rng.h -- High-quality random number generation.

#ifndef __RNG_H__
#define __RNG_H__

void rng_seed(int seed);
uint32_t rng_next(void);

#endif
