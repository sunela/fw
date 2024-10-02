/*
 * bip39dec.h - BIP39 (Mnemonic Code) decoding
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef BIP39DEC_H
#define BIP39DEC_H

#include <stdint.h>


#define	BIP39_MAX_BYTES			32
#define	BIP39_MAX_WORDS			24

#define	BIP39_DECODE_UNRECOGNIZED	(-1)
#define	BIP39_DECODE_OVERFLOW		(-2)
#define	BIP39_DECODE_CHECKSUM		(-3)


/*
 * Convert num_words indices stored in "words" to up to "bytes" in "buf".
 * Returns the number of bytes written to "buf", or a negative error code:
 * -1  the num_words is not one of the specified sizes, or
 * -2  more than "bytes" would be needed,
 * -3  the checksum is incorrect.
 */

int bip39_decode(void *buf, unsigned bytes, const uint16_t *words,
    unsigned num_words);

#endif /* !BIP39DEC_H */
