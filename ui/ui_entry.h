/*
 * ui_entry.h - User interface: Text entry
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef UI_ENTRY_H
#define	UI_ENTRY_H

#include "gfx.h"


struct ui_entry_style {
	gfx_color	input_fg;
	gfx_color	input_valid_bg;
	gfx_color	input_invalid_bg;
	const struct font *input_font;	/* NULL for default */
	gfx_color	title_fg;
	gfx_color	title_bg;
	const struct font *title_font;	/* NULL for default */
};

struct ui_entry_maps {
	/*
	 * One entry for each key, from 0 to 9 in phone keypad arrangement
	 * (top left: 1, top center: 2, etc., with 0 at the bottom center,
	 * between "back/cancel" and "accept/enter").
	 *
	 * Each entry is a string with the following characters:
	 * - main character
	 * - 0-4 secondary characters
	 */
	const char *first[10];
	/*
	 * One entry for each first-level key, 0 to 9. Each entry is a string
	 * with one character for the keys 0 to 9.
	 */
	const char *second[10];
};

struct ui_entry_params {
	const char	*title;
	char		*buf;
	unsigned	max_len;
	bool		(*validate)(void *user, const char *s);
	void		*user;
	const struct ui_entry_style *style;
	const struct ui_entry_maps *maps;
};


extern const struct ui_entry_maps ui_entry_text_maps;

#endif /* !UI_ENTRY_H */
