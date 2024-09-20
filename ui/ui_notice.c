/*
 * ui_notice.c - User interface: Show a notice
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>

#include "hal.h"
#include "debug.h"
#include "fmt.h"
#include "colors.h"
#include "shape.h"
#include "text.h"
#include "ui.h"
#include "ui_notice.h"


#define	DEFAULT_BOX_R	10
#define	DEFAULT_FONT	mono18


struct ui_notice_ctx {
	const struct ui *next;
	void *next_params;
};


static const struct ui_notice_style default_style = {
	.fg		= GFX_BLACK,
	.bg		= GFX_WHITE,
	.x_align	= GFX_CENTER,
	.font		= &mono18,
};


/* --- Event handling ------------------------------------------------------ */


static void ui_notice_tap(void *ctx, unsigned x, unsigned y)
{
	const struct ui_notice_ctx *c = ctx;

	if (c->next)
		ui_switch(c->next, c->next_params);
	else
		ui_return();
}


static void ui_notice_to(void *ctx, unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe)
{
	const struct ui_notice_ctx *c = ctx;

        if (swipe == us_left) {
		if (c->next)
			ui_switch(c->next, c->next_params);
		else
                	ui_return();
	}
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_notice_open(void *ctx, void *params)
{
	struct ui_notice_ctx *c = ctx;
	const struct ui_notice_params *p = params;
	const struct ui_notice_style *style =
	    p->style ? p->style : &default_style;
	unsigned w = p->w ? p->w : GFX_WIDTH;
	unsigned r = p->r ? p->r : DEFAULT_BOX_R;
	unsigned margin = p->margin ? p->margin : r;
	const struct font *font = style->font ? style->font : &DEFAULT_FONT;
	unsigned h;

	c->next = p->next;
	c->next_params = p->next_params;
	assert(w > 2 * margin);
	h = text_format(NULL, 0, 0, w - 2 * margin, 0, 0, p->s,
	    font, style->x_align, style->fg);
	gfx_rrect_xy(&main_da, (GFX_WIDTH - w) / 2,
	    (GFX_HEIGHT - h) / 2 - margin,
	    w, h + 2 * margin, r, style->bg);
	text_format(&main_da, (GFX_WIDTH - w) / 2 + margin,
	    (GFX_HEIGHT - h) / 2, w - 2 * margin, h, 0, p->s,
	    font, style->x_align, style->fg);
	set_idle(p->idle_s ? p->idle_s : IDLE_NOTICE_S);
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_notice_events = {
	.touch_tap	= ui_notice_tap,
	.touch_to	= ui_notice_to,
};

const struct ui ui_notice = {
	.name		= "notice",
	.ctx_size	= sizeof(struct ui_notice_ctx),
	.open		= ui_notice_open,
	.events		= &ui_notice_events,
};


/* --- Wrappers ------------------------------------------------------------ */


static void vnotice_common(enum notice_type type, unsigned idle_s,
    const char *fmt, va_list ap,
    void (*chain)(const struct ui *ui, void *params), const struct ui *next,
    void *next_params)
{
	struct ui_notice_style style = {
		/* .fg and .bg are set below */
		.x_align	= GFX_CENTER,
	};
	struct ui_notice_params params = {
		.style		= &style,
		.idle_s		= idle_s,
		.next		= next,
		.next_params	= next_params,
        };

	params.s = vformat_alloc(fmt, ap);
	switch (type) {
	case nt_success:
		style.fg = SUCCESS_FG;
		style.bg = SUCCESS_BG;
		break;
	case nt_error:
		style.fg = ERROR_FG;
		style.bg = ERROR_BG;
		break;
	case nt_fault:
		style.fg = FAULT_FG;
		style.bg = FAULT_BG;
		break;
	case nt_info:
		style.fg = INFO_FG;
		style.bg = INFO_BG;
		break;
	default:
		ABORT();
	}
	chain(&ui_notice, &params);
	free((char *) params.s);
}


void vnotice_idle(enum notice_type type, unsigned idle_s, const char *fmt,
    va_list ap)
{
	vnotice_common(type, idle_s, fmt, ap, ui_switch, NULL, NULL);
}


void notice_idle(enum notice_type type, unsigned idle_s, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vnotice_idle(type, idle_s, fmt, ap);
	va_end(ap);
}


void vnotice(enum notice_type type, const char *fmt, va_list ap)
{
	vnotice_common(type, 0, fmt, ap, ui_switch, NULL, NULL);
}


void notice(enum notice_type type, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vnotice(type, fmt, ap);
	va_end(ap);
}


void vnotice_call(enum notice_type type, const char *fmt, va_list ap)
{
	vnotice_common(type, 0, fmt, ap, ui_call, NULL, NULL);
}


void notice_call(enum notice_type type, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vnotice_call(type, fmt, ap);
	va_end(ap);
}


void vnotice_switch(const struct ui *next, void *params, enum notice_type type,
    const char *fmt, va_list ap)
{
	vnotice_common(type, 0, fmt, ap, ui_switch, next, params);
}


void notice_switch(const struct ui *next, void *params, enum notice_type type,
    const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vnotice_switch(next, params, type, fmt, ap);
	va_end(ap);
}
