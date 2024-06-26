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

struct ui_entry_params {
	const char	*title;
	char		*buf;
	unsigned	max_len;
	bool		(*validate)(void *user, const char *s);
	void		*user;
	struct ui_entry_style *style;
};

#endif /* !UI_ENTRY_H */
