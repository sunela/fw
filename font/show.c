/*
 * show.c - Render characters from bitmap fonts (as ASCII, for development)
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

/*
 * Note that the fonts support UTF-16 characters, but "show" is currently
 * limited to 7-bit ASCII.
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "font.h"

#include "mono14.font"
#include "mono18.font"
#include "mono24.font"
#include "mono34.font"
#include "mono36.font"
#include "mono58.font"


struct font_ref {
	const char *name;
	const struct font *font;
};


static struct font_ref fonts[] = {
	{ "mono14",	&mono14 },
	{ "mono18",	&mono18 },
	{ "mono24",	&mono24 },
	{ "mono34",	&mono34 },
	{ "mono36",	&mono36 },
	{ "mono58",	&mono58 },
	{ NULL }
};


static void render(const struct font *font, const struct character *ch)
{
	const uint8_t *p = ch->data;
	uint8_t x, y;
	bool on = !ch->start;
	uint8_t got = 0;
	uint16_t more = 0;
	uint16_t buf = 0;

	(void) font;

	printf("%3s", "");
	for (x = 0; x != ch->w; x++)
		printf("%2u", x);
	printf("\n");

	for (y = 0; y != ch->h; y++) {
		printf("%2u ", y);
		for (x = 0; x != ch->w; x++) {
			if (!more) {
				/* uncompressed */
				if (ch->bits == 1) {
					if (!got) {
						buf = *p++;
						got = 8;
					}
					on = buf & 1;
					buf >>= 1;
					got--;
					more = 1;
				} else {
					while (1) {
						uint8_t this;

						if (got < ch->bits) {
							buf |= *p++ << got;
							got += 8;
						}
						this = (buf &
						    ((1 << ch->bits) - 1)) + 1;
						more += this;
						buf >>= ch->bits;
						got -= ch->bits;
						if (this != 1 << ch->bits)
							break;
						more--;
					}
					on = !on;
				}
			}
			printf("%s", on ? "##" : "--");
			more--;
		}
		putchar('\n');
	}
}


static void usage(const char *name)
{
	fprintf(stderr, "usage: %s font character\n", name);
	exit(1);
}


int main(int argc, char **argv)
{
	const struct font_ref *font;
	const struct character *ch;

	if (argc != 3)
		usage(*argv);
	for (font = fonts; font->name; font++)
		if (!strcmp(font->name, argv[1]))
			break;
	if (!font->name) {
		fprintf(stderr, "font not found\n");
		exit(1);
	}
	ch = font_find_char(font->font, *argv[2]);
	if (!ch) {
		fprintf(stderr, "character not found\n");
		return 1;
	}
	render(font->font, ch);
	return 0;
}
