/*
 * hotp.c - HMAC-Based One-Time Password (HOTP)
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

/*
 * Specification:
 * https://www.ietf.org/rfc/rfc4226.txt
 * https://en.wikipedia.org/wiki/HMAC-based_one-time_password
 */

#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include "hmac.h"
#include "hotp.h"

//#define DEBUG

#ifdef DEBUG
#include "debug.h"
#endif


uint32_t hotp(const void *k, size_t k_size, const void *c, size_t c_size)
{
	uint8_t hash[HMAC_SHA1_BYTES];
	unsigned i;
	uint32_t res;

#ifdef DEBUG
	debug("K");
	for (i = 0; i != k_size; i++)
		debug(" %02x", ((const uint8_t *) k)[i]);
	debug("\n");
	debug("C");
	for (i = 0; i != c_size; i++)
		debug(" %02x", ((const uint8_t *) c)[i]);
	debug("\n");
#endif
	hmac_sha1(hash, k, k_size, c, c_size);
#ifdef DEBUG
	debug("H");
	for (i = 0; i != HMAC_SHA1_BYTES; i++)
		debug(" %02x", hash[i]);
	debug("\n");
#endif
	i = hash[HMAC_SHA1_BYTES - 1] & 15;
	res = (hash[i] & 0x7f) << 24 | hash[i + 1] << 16 | hash[i + 2] << 8 |
	    hash[i + 3];
	memset(hash, 0, sizeof(hash));
	return res;
}


static uint32_t hotp_n(const void *k, size_t k_size, uint64_t count,
    unsigned bytes)
{
	uint8_t c_bytes[8];
	unsigned i;

	for (i = 0; i != bytes; i++)
		c_bytes[i] = count >> 8 * (bytes - i - 1);
	return hotp(k, k_size, c_bytes, bytes);
}


uint32_t hotp64(const void *k, size_t k_size, uint64_t count)
{
	return hotp_n(k, k_size, count, 8);
}
