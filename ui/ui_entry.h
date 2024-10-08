/*
 * ui_entry.h - User interface: Text entry
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef UI_ENTRY_H
#define	UI_ENTRY_H

#include <stdbool.h>

#include "gfx.h"


#define	UI_ENTRY_LEFT		(-1)
#define	UI_ENTRY_RIGHT		(-2)
#define	UI_ENTRY_INVALID	(-3)


/*
 * Button layout for the "pos" and "n" operations:
 *
 * row  col
 * |    0   1   3
 * |    |   |   |
 * 3   [1] [2] [3]
 * 2   [4] [5] [6]
 * 1   [7] [8] [9]
 * 0   [L] [0] [R]
 */

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
	 * "validate" results:
	 * 1: accept input. String in buffer is valid for "accept".
	 * 0: accept input, but string in buffer is NOT (yet) valid.
	 * -1: ignore input
	 */
	int		(*validate)(void *user, const char *s);
	void		*user;
};

struct ui_entry_ops {
	void (*init)(void *user, struct ui_entry_input *input,
	    const struct ui_entry_style *style);
	void (*input)(void *user);
	/*
	 * "pos" returns 1 if the (x, y) position corresponds to a button, 0
	 * otherwise. "col" and "row" contain the button position.
	 */
	bool (*pos)(void *user, unsigned x, unsigned y,
	    unsigned *col, unsigned *row);
	/*
	 * If the (col, row) position corresponds to an existing button, "n"
	 * returns the button number (0-9), UI_ENTRY_LEFT for the left special
	 * button (cancel, etc.), or UI_ENTRY_RIGHT for the right special
	 * button (accept). If (col, row) is outside the keypad, returns
	 * UI_ENTRY_INVALID.
	 */
	int (*n)(void *user, unsigned col, unsigned row);
	void (*button)(void *user, unsigned col, unsigned row,
	    const char *label, bool second, bool enabled, bool up);
	void (*clear_pad)(void *user);

	/* --- Specialized customization ----------------------------------- */

	/*
	 * Normally, which character add to the input when a given key is
	 * pressed is determined by the maps. By setting key_char, translating
	 * key positions to characters is delegated to the caller. (E.g., for
	 * BIP39, where keys correspong to character sets.)
	 */
	char (*key_char)(void *user, unsigned n);
	/*
	 * If enabled_set is set, it provides fine-grained control over when
	 * buttons are enabled. It returns a bit set of the buttons enabled at
	 * the current input.
	 */
	uint16_t (*enabled_set)(void *user);
	/*
	 * If "accept" is not NULL, it is called each time the input has grown.
	 * If it returns "true", the input is accepted immediately, without
	 * further user input.
	 */
	bool (*accept)(void *user);
};

struct ui_entry_params {
	struct ui_entry_input input;
	const struct ui_entry_style *style;
	const struct ui_entry_maps *maps;
	const struct ui_entry_ops *entry_ops;
	void *entry_user;
};


extern const struct ui_entry_style ui_entry_default_style;
extern const struct ui_entry_maps ui_entry_text_maps;
extern const struct ui_entry_maps ui_entry_decimal_maps;

bool ui_entry_valid(struct ui_entry_input *in);

#endif /* !UI_ENTRY_H */
