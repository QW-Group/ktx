//// Start imported code.
// Code in this section was copied from
// https://prng.di.unimi.it/splitmix64.c
// then lightly edited to remove a global variable and rename a function.

/*  Written in 2015 by Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

#include "rng_seed_impl.h"

/* This is a fixed-increment version of Java 8's SplittableRandom generator
   See http://dx.doi.org/10.1145/2714064.2660195 and
   http://docs.oracle.com/javase/8/docs/api/java/util/SplittableRandom.html

   It is a very fast generator passing BigCrush, and it can be useful if
   for some reason you absolutely want 64 bits of state. */

#ifdef Q3_VM
# define C1 0x9e3779b9
# define C2 0xbf58476d
# define C3 0x94d049bb
#else
# define C1 0x9e3779b97f4a7c15
# define C2 0xbf58476d1ce4e5b9
# define C3 0x94d049bb133111eb
#endif

#define SHIFT1 (sizeof(rng_seed_t) == 8 ? 30 : 15)
#define SHIFT2 (sizeof(rng_seed_t) == 8 ? 27 : 13)
#define SHIFT3 (sizeof(rng_seed_t) == 8 ? 31 : 16)

rng_seed_t rng_seed_impl_next(rng_seed_t* x) {
	rng_seed_t z = (*x += C1);
	z = (z ^ (z >> SHIFT1)) * C2;
	z = (z ^ (z >> SHIFT2)) * C3;
	return z ^ (z >> SHIFT3);
}
//// End imported code.
