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

#include "mono18.font"
#include "mono28.font"
#include "mono34.font"
#include "mono38.font"
#include "mono58.font"


struct font_ref {
	const char *name;
	const struct font *font;
};


static struct font_ref fonts[] = {
	{ "mono18",	&mono18 },
	{ "mono28",	&mono28 },
	{ "mono34",	&mono34 },
	{ "mono38",	&mono38 },
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
	/* @@@ Support uncompressed fonts later */
	assert(ch->bits > 1);
	for (y = 0; y != ch->h; y++) {
		for (x = 0; x != ch->w; x++) {
			if (!more) {
				while (1) {
					uint8_t this;

					if (got < ch->bits) {
						buf |= *p++ << got;
						got += 8;
					}
					this =
					    (buf & ((1 << ch->bits) - 1)) + 1;
					more += this;
					buf >>= ch->bits;
					got -= ch->bits;
					if (this != 1 << ch->bits)
						break;
					more--;
				}
				on = !on;
			}
			putchar(on ? '#' : '-');
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
