/*
 * ui_notice.h - User interface: Show a notice
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef UI_NOTICE_H
#define	UI_NOTICE_H

#include "gfx.h"


struct ui_notice_style {
	gfx_color		fg, bg;
	int8_t			x_align;
	const struct font	*font;	/* 0 for default font */
};

struct ui_notice_params {
	const char *s;
	unsigned w;	/* 0 for default width */
	unsigned r;	/* 0 for default radius */
	unsigned margin; /* 0 for radius */
	const struct ui_notice_style *style;
};

#endif /* !UI_NOTICE_H */
