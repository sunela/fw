/*
 * bip39in.h - BIP39 (Mnemonic Code) input
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef BIP39IN_H
#define BIP39IN_H

#include <stdint.h>


#define	BIP39_MAX_FINAL_CHOICES	4


extern const char *bip39_sets[10];


/*
 * bip39_match performs the pattern matching needed for BIP-0039 input with a
 * keypad. It uses a set of key assignments that narrows down the selection of
 * words to no more than four choices after the 4th key.
 *
 * Keys are encoded with the characters '0' to '9', where '0' is in the upper
 * left corner and '9' in the bottom center.
 *
 * bip39_match stores up to max_matches indices in "matches". It returns the
 * total number of matching words.
 *
 * A NUL-terminated list of sets covering possible next letters is stored in
 * "next". If "next" is empty, the returned matches should be presented to the
 * user to make the final choice.
 *
 * Note that matching three-letter words are returned at the 3rd position but
 * not at the 4th position. It is up to the caller to detect when an input
 * could end at the 3rd position. (The minimum length of english BIP-0039
 * words is three.)
 */

unsigned bip39_match(const char *s, uint16_t *matches, unsigned max_matches,
    char *next, unsigned next_size);

#endif /* !BIP39IN_H */
