/*
 * font.c - Bitmap fonts
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdint.h>
#include <stdlib.h>

#include "font.h"


static int comp(const void *key, const void *p)
{
	const struct character *ch = p;

	return (int) *(const uint16_t *) key - (int) ch->code;
}


const struct character *font_find_char(const struct font *font, uint16_t code)
{
	return bsearch(&code, font->chars, font->n_chars,
	    sizeof(struct character), comp);
}
