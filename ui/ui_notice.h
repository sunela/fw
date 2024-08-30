/*
 * ui_notice.h - User interface: Show a notice
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef UI_NOTICE_H
#define	UI_NOTICE_H

#include <stdarg.h>

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

enum notice_type {
	nt_success,
	nt_error,
	nt_info,
	nt_fault,
};


/*
 * vnotice and notice end by calling ui_switch, while vnotice_call and
 * progress_call end * with ui_call. All invoke ui_notice.
 */

void vnotice(enum notice_type type, const char *fmp, va_list ap);
void notice(enum notice_type type, const char *fmt, ...);
void vnotice_call(enum notice_type type, const char *fmp, va_list ap);
void notice_call(enum notice_type type, const char *fmt, ...);

#endif /* !UI_NOTICE_H */
