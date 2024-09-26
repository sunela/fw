/*
 * bip39dec.c - BIP39 (Mnemonic Code) decoding
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdint.h>
#include <assert.h>

#include "debug.h"
#include "sha.h"
#include "bip39enc.h"
#include "bip39dec.h"


/*
 * "cs" and "ent" are from the table at
 * https://en.bitcoin.it/wiki/BIP_0039
 */

int bip39_decode(void *buf, unsigned bytes, const uint16_t *words,
    unsigned num_words)
{
	unsigned cs, ent;

	switch (num_words) {
	case 12:
	case 15:
	case 18:
	case 21:
	case 24:
		cs = num_words / 3;
		ent = 10 * num_words + 2 * cs;
		break;
	default:
		return BIP39_DECODE_UNRECOGNIZED;
	}

	if (bytes * 8 < ent)
		return BIP39_DECODE_OVERFLOW;

	uint8_t *p = buf;
	uint32_t pending = 0;
	unsigned bits = 0;

	while (num_words--) {
		pending = pending << BIP39_WORD_BITS | *words++;
		bits += BIP39_WORD_BITS;
		while (bits >= 8 && (void *) p - buf != ent / 8) {
			*p++ = pending >> (bits - 8);
			bits -= 8;
		}
	}
	assert((unsigned) ((void *) p - buf) == ent / 8);
	assert(bits == cs);

	uint8_t hash[SHA256_HASH_BYTES];

	sha256_begin();
	sha256_hash(buf, ent / 8);
	sha256_end(hash);

	debug("sha pending 0x%x bits %u sha 0x%02x\n",
	    (unsigned) pending, bits, hash[0]);

	/* @@@ why do we need a cast here ??! */
	return (pending & ((1 << cs) - 1)) == (uint32_t) hash[0] >> (8 - cs) ?
	    (void *) p - buf : BIP39_DECODE_CHECKSUM;
}
