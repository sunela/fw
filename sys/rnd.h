/*
 * rnd.h - Random number generators
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef RND_H
#define	RND_H

#include <stdint.h>


extern uint8_t rnd_fixed;	/* for testing, only in simulator */


uint32_t rnd(uint32_t range);
void rnd_bytes(void *buf, unsigned size);
void randombytes(uint8_t *bytes, uint64_t len);

#endif /* !RND_H */
