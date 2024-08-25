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
	 * One entry for each first-level key, 0 to 9. Each entry is either
	 * NULL, in which case the first character from the first level is
	 * used, or a string with one character for each of the keys 0 to 9.
	 */
	const char *second[10];
};

struct ui_entry_input {
	const char	*title;
	char		*buf;
	unsigned	max_len;
	/*
	 * 1: accept input. String in buffer is valid for "accept".
	 * 0: accept input, but string in buffer is NOT valid.
	 * -1: ignore input
	 */
	int		(*validate)(void *user, const char *s);
	void		*user;
};

struct ui_entry_ops {
	void (*init)(void *user, struct ui_entry_input *input,
	    const struct ui_entry_style *style);
	void (*input)(void *user);
};

struct ui_entry_params {
	struct ui_entry_input input;
	const struct ui_entry_style *style;
	const struct ui_entry_maps *maps;
	const struct ui_entry_ops *entry_ops;
	void *entry_user;
};


extern const struct ui_entry_maps ui_entry_text_maps;
extern const struct ui_entry_maps ui_entry_decimal_maps;

bool ui_entry_valid(struct ui_entry_input *in);

#endif /* !UI_ENTRY_H */
