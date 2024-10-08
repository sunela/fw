/*
 * bip39in.c - BIP39 (Mnemonic Code) input
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "util.h"
#include "hal.h"
#include "bip39enc.h"
#include "bip39in.h"


PSRAM_NOINIT uint16_t bip39_matches[BIP39_WORDS];
const char *bip39_sets[10] = {
	"ab", "cd", "ef", "ghi", "jklm", "nop", "qr", "s", "tu", "vwxyz" };


unsigned bip39_match(const char *s, char *next, unsigned next_size)
{
	bool next_set[10] = { 0, };
	unsigned n_matches = 0;
	unsigned i;

	for (i = 0; i != BIP39_WORDS; i++) {
		const char *w = bip39_words[i];
		unsigned j = 0;
		const char *t;

		for (t = s; *t; t++) {
			assert(*t >= '0' && *t <= '9');
			if (!w[j])
				break;
			if (!strchr(bip39_sets[*t - '0'], w[j]))
				break;
			j++;
		}
		if (*t)
			continue;
		bip39_matches[n_matches] = i;
		n_matches++;
		if (next && w[j]) {
			unsigned k;

			for (k = 0; k != 10; k++)
				if (strchr(bip39_sets[k], w[j])) {
					next_set[k] = 1;
					break;
				}
		}
	}

	if (next) {
		char *p;

		p = next;
		for (i = 0; i != 10; i++)
			if (next_set[i]) {
				assert(p + 1 <= next + next_size);
				*p++ = '0' + i;
			}
		assert(p + 1 <= next + next_size);
		*p = 0;
	}

	return n_matches;
}


int bip39_word_to_sets(const char *s, char *buf, unsigned max_len)
{
	char *p;
	unsigned i;

	for (p = buf; *s && p < buf + max_len; s++) {
		for (i = 0; i != ARRAY_ENTRIES(bip39_sets); i++)
			if (strchr(bip39_sets[i], *s)) {
				*p++ = '0' + i;
				break;
			}
		if (i == ARRAY_ENTRIES(bip39_sets))
			return -1;
	}
	*p = 0;
	return p - buf;
}
