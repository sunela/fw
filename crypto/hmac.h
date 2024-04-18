/*
 * hmac.h - HMAC-SHA1
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef HMAC_H
#define	HMAC_H

#include <stdint.h>
#include <sys/types.h>

#include "sha.h"


#define	HMAC_SHA1_BYTES	SHA1_HASH_BYTES


void hmac_sha1(uint8_t res[HMAC_SHA1_BYTES], const void *k, size_t k_size,
    const void *c, size_t c_size);

#endif /* !HMAC_H */
