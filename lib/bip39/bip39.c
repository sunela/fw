/*
 * bip39.c - BIP39 (Mnemonic Code) encoding and decoding
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#include "util.h"
#include "sha.h"
#include "bip39.h"


#define	BITS	11


static const char *words[] = {
#include "english.inc"
};


void bip39_words(struct bip39 *b, const uint8_t *data, unsigned bytes)
{
	uint8_t tmp[SHA256_HASH_BYTES];

	b->pos = 0;
	b->p = data;
	b->bytes = bytes;
	b->end = data + bytes;
	sha256_begin();
	sha256_hash(data, bytes);
	sha256_end(tmp);
	b->sha = tmp[0];
}


#include <stdio.h>
const char *bip39_next_word(struct bip39 *b)
{
	uint16_t buf = 0;
	unsigned got = 0;
	unsigned n;

	if (b->p == b->end)
		return NULL;
//printf("pos %u %p\n", b->pos, b->p);
	assert(ARRAY_ENTRIES(words) == 1 << BITS);
	while (got < BITS) {
		unsigned left = 8 - (b->pos & 7);
		unsigned want = BITS - got;
		unsigned mask = (1 << want) - 1;

//printf("\tbuf 0x%x pos %u got left %u %u want %u\n",
//    buf, b->pos, left, got, want);
		assert(b->p <= b->end);
		if (b->p == b->end) {
//printf("\t\tsha 0x%02x\n", b->sha);
			buf = buf << want | (b->sha >> (8 - want));
//			assert(((b->pos + want) & 7) == 0);
			break;
		}
		if (left > want) {
			unsigned shift = left - want;

//printf("\t\t0x%02x shift %u\n", *b->p, shift);
			buf = buf << want | ((*b->p >> shift) & mask);
			b->pos += want;
			break;
			
		}
//printf("\t\t0x%02x\n", *b->p);
		buf = buf << left | (*b->p & mask);
		b->p++;
		b->pos += left;
		got += left;
	}
	n = buf & ((1 << BITS) - 1);
//printf("\t0x%x -> 0x%x\n", buf, n);
	return words[n];
}
