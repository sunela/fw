/*
 * base32.h - Base32 (RFC4648) encoder/decoder
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

/*
 * Specification:
 * https://datatracker.ietf.org/doc/html/rfc4648
 * https://en.wikipedia.org/wiki/Base32
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include "base32.h"


static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
static const char ignore[] = "\r\n=";


ssize_t base32_encode(char *res, size_t res_size, const void *data, size_t size)
{
	const uint8_t *p = data;
	char *q = res;
	uint16_t buf = 0;
	unsigned got = 0;

	while (p != data + size) {
		if (got < 5) {
			buf = (buf << 8) | *p++;
			got += 8;
		}
		while (got >= 5) {
			if (q == res + res_size)
				return -1;
			*q++ = alphabet[(buf >> (got - 5)) & 31];
			got -= 5;
		}
	}
	if (q == res + res_size)
		return -1;
	if (got)
		*q++ = alphabet[(buf << (5 - got)) & 31];
	while ((q - res) & 7) {
		if (q == res + res_size)
			return -1;
		*q++ = '=';
	}
	if (q == res + res_size)
		return -1;
	*q = 0;
	return q - res;
}


ssize_t base32_decode(void *res, size_t res_size, const char *s)
{
	uint8_t *q = res;
	uint16_t buf = 0;
	unsigned got = 0;

	while (*s) {
		char ch = *s++;
		const char *c;

		if (strchr(ignore, ch))
			continue;
		c = strchr(alphabet, ch);
		buf = buf << 5 | (c - alphabet);
		got += 5;
		if (got > 7) {
			if (res) {
				if (res + res_size == q)
					return -1;
				*q++ = buf >> (got - 8);
			} else {
				q++;
			}
			got -= 8;
		}
	}
	if (got && buf & ((1 << got) - 1))
		return -1;
	return (void *) q - res;
}
