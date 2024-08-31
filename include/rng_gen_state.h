#ifndef __RNG_STATE_H__
#define __RNG_STATE_H__

// You must initialize this with rng_seed/rng_seed_global!
typedef struct RngState { unsigned int s[4]; } RngState;

#endif
