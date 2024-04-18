/*
 * sha.h - SHA1 API
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef SHA_H
#define	SHA_H

#include <stdint.h>
#include <sys/types.h>


#define	SHA1_HASH_BYTES		20	/* 160 bits */
#define	SHA1_BLOCK_BYTES	64      /* 512 bits */


void sha1_begin(void);
void sha1_hash(const uint8_t *data, size_t size);
void sha1_end(uint8_t res[SHA1_HASH_BYTES]);

#endif /* !SHA_H */
