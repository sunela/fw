/*
 * rnd.h - Random number generators
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef RND_H
#define	RND_H

#include <stdint.h>


uint32_t rnd(uint32_t range);
void rnd_bytes(uint8_t *buf, unsigned size);

#endif /* !RND_H */
