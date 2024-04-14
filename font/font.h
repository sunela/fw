/*
 * font.h - Bitmap fonts
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef FONT__H
#define	FONT_H

#include <stdbool.h>
#include <stdint.h>


struct character {
	uint16_t code;		/* character code (in Unicode) */
	uint8_t w, h;		/* bounding box */
	int8_t ox, oy;		/* bounding box offset */
	uint8_t advance;	/* specing to next character */
	int8_t bits;		/* 0: no pixels; 1: bitmap;
				   >= 2: run-length encoded */
	bool start;		/* initial pixel value, ignored if bits < 2 */
	const uint8_t *data;	/* packed bit string */
};

struct font {
//	uint8_t ascent, descent; /* @@@ redundant ? */
	uint8_t w, h;		/* font bounding box */
	int8_t ox, oy;		/* font bounding box offset */
	unsigned n_chars;	/* number of character definitions */
	const struct character chars[];
};


const struct character *font_find_char(const struct font *font, uint16_t code);

#endif /* !FONT_H */
