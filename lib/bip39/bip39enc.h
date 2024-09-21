/*
 * bip39enc.h - BIP39 (Mnemonic Code) encoding
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef BIP39ENC_H
#define BIP39ENC_H

#include <stdint.h>


struct bip39enc {
	unsigned pos;	/* bit position */
	const uint8_t *p;
	const uint8_t *end;
	unsigned bytes;
	uint8_t sha;
};


extern const char *bip39_words[];


void bip39_encode(struct bip39enc *b, const uint8_t *data, unsigned bytes);
int bip39_next_word(struct bip39enc *b);

#endif /* !BIP39ENC_H */
