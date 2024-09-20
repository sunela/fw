/*
 * bip39.h - BIP39 (Mnemonic Code) encoding and decoding
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef BIP39_H
#define BIP39_H

#include <stdint.h>


struct bip39 {
	unsigned pos;	/* bit position */
	const uint8_t *p;
	const uint8_t *end;
	unsigned bytes;
	uint8_t sha;
};


void bip39_words(struct bip39 *b, const uint8_t *data, unsigned bytes);
const char *bip39_next_word(struct bip39 *b);

#endif /* !BIP39_H */
