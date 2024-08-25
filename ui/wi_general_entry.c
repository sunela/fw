/*
 * wi_general_entry.c - User interface: general text entry
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <assert.h>

#include "hal.h"
#include "gfx.h"
#include "text.h"
#include "ui.h"
#include "ui_entry.h"
#include "wi_general_entry.h"


/* --- Input --------------------------------------------------------------- */


#define	INPUT_PAD_TOP		5
#define	INPUT_PAD_BOTTOM	2
#define	INPUT_MAX_X		210

#define	DEFAULT_INPUT_FONT	mono34
#define	DEFAULT_TITLE_FONT	mono24


/* @@@ we should just clear the bounding box */

static void clear_input(struct wi_general_entry_ctx *c)
{
	struct ui_entry_input *in = c->input;
	const struct ui_entry_style *style = c->style;

	gfx_rect_xy(&main_da, 0, 0, GFX_WIDTH,
	    INPUT_PAD_TOP + c->input_max_height + INPUT_PAD_BOTTOM,
	    !*in->buf && in->title ? style->title_bg :
	    ui_entry_valid(in) ? style->input_valid_bg :
	    style->input_invalid_bg);
}


static void draw_top(struct wi_general_entry_ctx *c, const char *s,
    const struct font *font, gfx_color color, gfx_color bg)
{
	struct ui_entry_input *in = c->input;
	struct text_query q;

	if (!*s)
		return;
	/*
	 * For the text width, we use the position of the next character
	 * (q.next) instread of the bounding box width (q.w), so that there is
	 * visual feedback when entering a space. (A trailing space doesn't
	 * grow the bounding box. Multiple spaces would, though, which is
	 * somewhat inconsistent.)
	 */
	text_query(0, 0, s, font, GFX_LEFT, GFX_TOP | GFX_MAX, &q);
	assert(q.next <= (int) (in->max_len * c->input_max_height));

	gfx_rect_xy(&main_da, 0, INPUT_PAD_TOP, GFX_WIDTH, q.h, bg);
	gfx_clip_xy(&main_da, 0, INPUT_PAD_TOP, GFX_WIDTH, q.h);
	if ((GFX_WIDTH + q.next) / 2 < INPUT_MAX_X)
		text_text(&main_da, (GFX_WIDTH - q.next) / 2, INPUT_PAD_TOP,
		    s, font, GFX_LEFT, GFX_TOP | GFX_MAX, color);
	else
		text_text(&main_da, INPUT_MAX_X - q.next, INPUT_PAD_TOP,
		    s, font, GFX_LEFT, GFX_TOP | GFX_MAX, color);
	gfx_clip(&main_da, NULL);
}


static void draw_input(struct wi_general_entry_ctx *c)
{
	struct ui_entry_input *in = c->input;
	const struct ui_entry_style *style = c->style;

	if (!*in->buf && in->title)
		text_text(&main_da, GFX_WIDTH / 2,
		    (INPUT_PAD_TOP + c->input_max_height +
		    INPUT_PAD_BOTTOM) / 2,
		    in->title,
		    style->title_font ? style->title_font :
		    &DEFAULT_TITLE_FONT,
		    GFX_CENTER, GFX_CENTER, style->title_fg);
	else
		draw_top(c, in->buf,
		    style->input_font ? style->input_font : &DEFAULT_INPUT_FONT,
		    style->input_fg,
		    ui_entry_valid(in) ? style->input_valid_bg :
		    style->input_invalid_bg);
}


static void wi_general_entry_input(void *user)
{
	struct wi_general_entry_ctx *c = user;

	clear_input(c);
	draw_input(c);
}


static void wi_general_entry_init(void *ctx, struct ui_entry_input *input,
    const struct ui_entry_style *style)
{
	struct wi_general_entry_ctx *c = ctx;
	struct text_query q;

	c->input = input;
	c->style = style;
	text_query(0, 0, "",
            style->input_font ? style->input_font : &DEFAULT_INPUT_FONT,
            GFX_TOP | GFX_MAX, GFX_TOP | GFX_MAX, &q);
        c->input_max_height = q.h;
}


struct ui_entry_ops wi_general_entry_ops = {
	.init	= wi_general_entry_init,
	.input	= wi_general_entry_input,
};
