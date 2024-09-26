/*
 * bip39in.h - BIP39 (Mnemonic Code) input
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef BIP39IN_H
#define BIP39IN_H

#include <stdint.h>

#include "hal.h"
#include "bip39enc.h"


#define	BIP39_MAX_SETS		4	/* [AB] type of sets */
#define	BIP39_MAX_FINAL_CHOICES	4	/* max. results after MAX_SETS inputs */


/*
 * We use a global array for bip39_matches since matches are used at several
 * places and we don't want to have lots of large (4 kB) arrays scattered all
 * over the place.
 */

extern PSRAM_NOINIT uint16_t bip39_matches[BIP39_WORDS];

extern const char *bip39_sets[10];


/*
 * bip39_match performs the pattern matching needed for BIP-0039 input with a
 * keypad. It uses a set of key assignments that narrows down the selection of
 * words to no more than four choices after the 4th key.
 *
 * Keys are encoded with the characters '0' to '9', where '0' is in the upper
 * left corner and '9' in the bottom center.
 *
 * bip39_match stores indices (0-based) of all matching words in bip39_matches.
 * It returns the number of matching words.
 *
 * A NUL-terminated list of sets covering possible next letters is stored in
 * "next". If "next" is empty, the returned matches should be presented to the
 * user to make the final choice. If no next letters are needed, "next" can be
 * NULL.
 *
 * Note that matching three-letter words are returned at the 3rd position but
 * not at the 4th position. It is up to the caller to detect when an input
 * could end at the 3rd position. (The minimum length of english BIP-0039
 * words is three.)
 */

unsigned bip39_match(const char *s, char *next, unsigned next_size);

/*
 * bip39_word_to_sets converts the word in "s" to a string of key sets in
 * "buf". Up to max_len characters plus a terminating NUL are stored. If "s" is
 * longer, the remaining characters are ignored.
 *
 * bip39_word_to_sets returns the length of the string in "buf", or -1 if there
 * were characters that could not be converted.
 */

int bip39_word_to_sets(const char *s, char *buf, unsigned max_len);

#endif /* !BIP39IN_H */
