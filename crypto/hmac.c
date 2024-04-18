/*
 * hmac.c - HMAC-SHA1
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

/*
 * HMAC specification:
 *
 * https://www.rfc-editor.org/rfc/rfc2104.txt
 * https://en.wikipedia.org/wiki/HMAC
 */

#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>

#include "sha.h"
#include "hmac.h"


void hmac_sha1(uint8_t res[HMAC_SHA1_BYTES], const void *k, size_t k_size, 
    const void *c, size_t c_size)
{
	uint8_t key[SHA1_BLOCK_BYTES];
	uint8_t key_xor[SHA1_BLOCK_BYTES];
	uint8_t h_text[SHA1_HASH_BYTES];
	unsigned i;

	// generate key from K

	if (k_size > SHA1_HASH_BYTES) {
		sha1_begin();
		sha1_hash(k, k_size);
		sha1_end(key);
		k_size = SHA1_HASH_BYTES;
	} else {
		memcpy(key, k, k_size);
	}
	if (k_size < SHA1_HASH_BYTES)
		memset(key+ k_size, 0, SHA1_BLOCK_BYTES - k_size);

	// h_text = H(K XOR ipad, text)

	for (i = 0; i != SHA1_BLOCK_BYTES; i++)
		key_xor[i] = key[i] ^ 0x36;	// ipad

	sha1_begin();
	sha1_hash(key_xor, SHA1_BLOCK_BYTES);
	sha1_hash(c, c_size);
	sha1_end(h_text);

	// H(K XOR opad, h_text)

	for (i = 0; i != SHA1_BLOCK_BYTES; i++)
		key_xor[i] = key[i] ^ 0x5c;	// ppad

	sha1_begin();
	sha1_hash(key_xor, SHA1_BLOCK_BYTES);
	sha1_hash(h_text, SHA1_HASH_BYTES);
	sha1_end(res);

	memset(key, 0, sizeof(key));
	memset(key_xor, 0, sizeof(key_xor));
	memset(h_text, 0, sizeof(h_text));
}
