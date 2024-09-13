/*
 * wi_general_entry.c - User interface: general text entry
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <string.h>
#include <assert.h>

#include "hal.h"
#include "gfx.h"
#include "shape.h"
#include "text.h"
#include "ui.h"
#include "colors.h"
#include "ui_entry.h"
#include "wi_general_entry.h"


/* --- Input --------------------------------------------------------------- */

#define	INPUT_PAD_TOP		5
#define	INPUT_PAD_BOTTOM	2
#define	INPUT_MAX_X		210

#define	DEFAULT_INPUT_FONT	mono34
#define	DEFAULT_TITLE_FONT	mono24


/* --- Keypad -------------------------------------------------------------- */

/*
 * Generally approximate the golden ratio: 1:1.618 or 0.384:0.615, but increase
 * the font size since since we have only one small descender (for Q).
 */

#define	FONT_1_TOP		mono24
#define	FONT_1_BOTTOM		mono18

#define	FONT_2			mono34

#define	LABEL_TOP_OFFSET	-12
#define	LABEL_CENTER_OFFSET	0
#define	LABEL_BOTTOM_OFFSET	13

#define	BUTTON_BOTTOM_OFFSET	8

#define	BUTTON_R		12
#define	BUTTON_W		70
#define	BUTTON_H		50
#define	BUTTON_X_GAP		8
#define	BUTTON_Y_GAP		5
#define	BUTTON_X_SPACING	(BUTTON_W + BUTTON_X_GAP)
#define	BUTTON_Y_SPACING	(BUTTON_H + BUTTON_Y_GAP)
#define	BUTTON_X0		(GFX_WIDTH / 2 - BUTTON_X_SPACING)
#define	BUTTON_Y1		(GFX_HEIGHT - BUTTON_H / 2 - \
				    BUTTON_BOTTOM_OFFSET)
#define	BUTTON_Y0		(BUTTON_Y1 - 3 * BUTTON_Y_SPACING)


/* --- Input --------------------------------------------------------------- */

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


/* --- Pad layout ---------------------------------------------------------- */


static void base(unsigned x, unsigned y, gfx_color bg)
{
        gfx_rrect_xy(&main_da, x - BUTTON_W / 2, y - BUTTON_H / 2,
            BUTTON_W, BUTTON_H, BUTTON_R, bg);
}


static int wi_general_entry_n(void *user, unsigned col, unsigned row)
{
	if (row) {
		int n = 1 + col + (3 - row) * 3;

		return n  < 1 || n > 9 ? UI_ENTRY_INVALID : n;
	} else {
		switch (col) {
		case 0:
			return UI_ENTRY_LEFT;
		case 1:
			return 0;
		case 2:
			return UI_ENTRY_RIGHT;
		default:
			return UI_ENTRY_INVALID;
		}
	}
}


static void draw_label(unsigned x, unsigned y, const char *s, bool second)
{
	char top[] = { *s, 0 };

	if (s[1]) {
		text_text(&main_da, x, y + LABEL_TOP_OFFSET, top,
		    &FONT_1_TOP, GFX_CENTER, GFX_CENTER, GFX_BLACK);
		text_text(&main_da, x, y + LABEL_BOTTOM_OFFSET, s + 1,
		    &FONT_1_BOTTOM, GFX_CENTER, GFX_CENTER, GFX_BLACK);
	} else {
		if (second) {
			/*
		         * Characters that are hard to recognize if vertically
			 * centered, e.g., minus and underscore would look the
			 * same. Some lower-case letters also look a little odd
			 * when centered, but they still are easily
			 * recognizable, so we probably don't need to do
			 * anything about them.
			 */
		        bool tricky = strchr("'\"`_,.", *s);

		        text_text(&main_da, x, y, s, &FONT_2, GFX_CENTER,
		            tricky ? GFX_CENTER | GFX_MAX : GFX_CENTER,
			    GFX_BLACK);
		} else {
			text_text(&main_da, x, y + LABEL_CENTER_OFFSET, top,
			    &FONT_1_TOP, GFX_CENTER, GFX_CENTER, GFX_BLACK);
		}
	}
}


static void wi_general_entry_button(void *user, unsigned col, unsigned row,
    const char *label, bool second, bool enabled, bool up)
{
	struct wi_general_entry_ctx *c = user;
	struct ui_entry_input *in = c->input;
	unsigned x = BUTTON_X0 + BUTTON_X_SPACING * col;
	unsigned y = BUTTON_Y1 - BUTTON_Y_SPACING * row;
	gfx_color bg;

	if (row > 0 || col == 1) {
		bg = enabled ? up ? UP_BG : DOWN_BG : DISABLED_BG;
		base(x, y, bg);
		draw_label(x, y, label, second);
		return;
	}
	bg = enabled ? up ? SPECIAL_UP_BG : SPECIAL_DOWN_BG :
	    SPECIAL_DISABLED_BG;
	if (col == 0) {	// X
		base(x, y, bg);
		if (*in->buf || second)
			gfx_equilateral(&main_da, x, y, BUTTON_H * 0.7, -1,
			    GFX_BLACK);
		else {
			if (c->new)
				gfx_power_sym(&main_da, x, y, BUTTON_H * 0.3,
				    5, GFX_BLACK, bg);
			else
				gfx_diagonal_cross(&main_da, x, y,
				    BUTTON_H * 0.4, 4, GFX_BLACK);
		}
	} else {	// >
		if (*in->buf && !second) {
			base(x, y, bg);
			gfx_equilateral(&main_da, x, y, BUTTON_H * 0.7, 1,
			    GFX_BLACK);
		} else {
			base(x, y, GFX_BLACK);
		}
	}
}


static void wi_general_entry_clear_pad(void *user)
{
	const unsigned h = 4 * BUTTON_H + 2 * BUTTON_X_GAP;

	gfx_rect_xy(&main_da, 0, GFX_HEIGHT - h - BUTTON_BOTTOM_OFFSET,
            GFX_WIDTH, h, GFX_BLACK);
}


static bool wi_general_entry_pos(void *user, unsigned x, unsigned y,
    unsigned *col, unsigned *row)
{
	*col = x < BUTTON_X0 + BUTTON_X_SPACING / 2 ? 0 :
	    x < BUTTON_X0 + 1.5 * BUTTON_X_SPACING ? 1 : 2;
	*row = 0;
	if (y < BUTTON_Y0 - BUTTON_Y_SPACING + BUTTON_R)
		return 0;
	if (y < BUTTON_Y0)
		*row = 3;
	else if (y > BUTTON_Y1)
		*row = 0;
	else
		*row = 3 - (y - BUTTON_Y0 + BUTTON_Y_SPACING / 2) /
		    BUTTON_Y_SPACING;
	return 1;
}


/* --- API ----------------------------------------------------------------- */


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


void wi_general_entry_setup(struct wi_general_entry_ctx *c, bool new)
{
	c->new = new;
}


struct ui_entry_ops wi_general_entry_ops = {
	.init		= wi_general_entry_init,
	.input		= wi_general_entry_input,
	.pos		= wi_general_entry_pos,
	.n		= wi_general_entry_n,
	.button		= wi_general_entry_button,
	.clear_pad	= wi_general_entry_clear_pad,
};
