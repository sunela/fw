/*
 * imath.h - Integer math functions
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdint.h>
#include <assert.h>

#include "imath.h"


/* --- isqrt --------------------------------------------------------------- */


/*
 * Based on
 * https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Binary_numeral_system_(base_2)
 */

uint32_t isqrt(uint32_t n)
{
	uint32_t x = n;		// X_(n+1)
	uint32_t c = 0;		// c_n

   	// d_n which starts at the highest power of four <= n
	uint32_t d = 1 << 30;	// The second-to-top bit is set.
				// Same as ((unsigned) INT32_MAX + 1) / 2.
	while (d > n)
		d >>= 2;

	// for d_n ... d_
	while (d) {
		if (x >= c + d) {	// if X_(m+1) ≥ Y_m then a_m = 2^m
			x -= c + d;	// X_m = X_(m+1) - Y_m
			c = (c >> 1) + d; // c_(m-1) = c_m/2 + d_m (a_m is 2^m)
		} else {
			c >>= 1;	// c_(m-1) = c_m/2  (a_m is 0)
		}
		d >>= 2;	// d_(m-1) = d_m/4
	}
	return c;		// c_(-1)
}


/* --- isin ---------------------------------------------------------------- */


static const uint8_t sin_tab[] = {
#include "sin.inc"
};


unsigned isin(unsigned a, unsigned f)
{
	assert(a <= 45);

	return f * sin_tab[a] / 255;
}
