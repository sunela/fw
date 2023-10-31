/*
 * rnd.c - Random number generators
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdint.h>
#include <stdlib.h>

#include "rnd.h"


/*
 * This is a very poor random number generator:
 * - its output is highly predictable
 * - depending on the range, it may be biased towards smaller values
 * Use only for simulation !
 */

uint32_t rnd(uint32_t range)
{
	return random() % range;
}


void rnd_bytes(uint8_t *buf, unsigned size)
{
	unsigned i;

	for (i = 0; i != size; i++)
		buf[i] = rnd(256);
}
