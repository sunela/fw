/*
 * bip39in.c - BIP39 (Mnemonic Code)ainput 
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "bip39enc.h"
#include "bip39in.h"


const char *bip39_sets[10] = {
	"ab", "cd", "ef", "ghi", "jklm", "nop", "qr", "s", "tu", "vwxyz" };


unsigned bip39_match(const char *s, uint16_t *matches, unsigned max_matches,
    char *next, unsigned next_size)
{
	bool next_set[10] = { 0, };
	unsigned n_matches = 0;
	unsigned i;

	for (i = 0; i != BIP39_WORDS; i++) {
		const char *w = bip39_words[i];
		unsigned j = 0;
		const char *t;

		for (t = s; *t; t++) {
			if (!w[j])
				break;
			if (!strchr(bip39_sets[*t - '0'], w[j]))
				break;
			j++;
		}
		if (*t)
			continue;
		if (n_matches < max_matches)
			matches[n_matches] = i;
		n_matches++;
		if (w[j]) {
			unsigned k;

			for (k = 0; k != 10; k++)
				if (strchr(bip39_sets[k], w[j])) {
					next_set[k] = 1;
					break;
				}
		}
	}

	char *p;

	p = next;
	for (i = 0; i != 10; i++)
		if (next_set[i]) {
			assert(p + 1 <= next + next_size);
			*p++ = '0' + i;
		}
	assert(p + 1 <= next + next_size);
	*p = 0;
	return n_matches;
}
