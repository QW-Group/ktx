#include "rng_gen_impl.h"

//// Start imported code.
// Code in this section copied from
// https://prng.di.unimi.it/xoshiro128starstar.c
// It was lightly edited to change a global variable to a local variable.

/*  Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

/* This is xoshiro128** 1.1, one of our 32-bit all-purpose, rock-solid
   generators. It has excellent speed, a state size (128 bits) that is
   large enough for mild parallelism, and it passes all tests we are aware
   of.

   Note that version 1.0 had mistakenly s[0] instead of s[1] as state
   word passed to the scrambler.

   For generating just single-precision (i.e., 32-bit) floating-point
   numbers, xoshiro128+ is even faster.

   The state must be seeded so that it is not everywhere zero. */

#ifdef Q3_VM
# define Q_INLINE
#else
# define Q_INLINE inline
#endif

static Q_INLINE unsigned int rotl(const unsigned int x, int k) {
	return (x << k) | (x >> (32 - k));
}


static Q_INLINE unsigned int next(RngState* s) {
	const unsigned int result = rotl(s->s[1] * 5, 7) * 9;

	const unsigned int t = s->s[1] << 9;

	s->s[2] ^= s->s[0];
	s->s[3] ^= s->s[1];
	s->s[1] ^= s->s[2];
	s->s[0] ^= s->s[3];

	s->s[2] ^= t;

	s->s[3] = rotl(s->s[3], 11);

	return result;
}
//// End imported code.

// Code in this section wraps the PRNG implementation.
unsigned int rng_gen_impl_next(RngState* state) {
	return next(state);
}
