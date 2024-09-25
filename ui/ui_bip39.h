/*
 * ui_bip39.h - User interface: Input a set of bIP39 words
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef UI_BIP39_H
#define UI_BIP39_H

#include <stdint.h>


struct ui_bip39_params {
	uint16_t *result;
	unsigned *num_words;
	unsigned max_words;
};

#endif /* !UI_BIP39_H */
