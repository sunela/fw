/*
 * ui_entry.c - User interface: Text entry
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>

#include "hal.h"
#include "debug.h"
#include "timer.h"
#include "gfx.h"
#include "text.h"
#include "shape.h"
#include "ui.h"
#include "ui_entry.h"


/* --- Input --------------------------------------------------------------- */

#define	INPUT_PAD_TOP		5
#define	INPUT_PAD_BOTTOM	2
#define INPUT_MAX_X		210

#define	DEFAULT_INPUT_FONT	mono34
#define	DEFAULT_TITLE_FONT	mono24

/* --- Keypad -------------------------------------------------------------- */

#define UP_BG			GFX_WHITE
#define DOWN_BG			gfx_hex(0x808080)
#define	DISABLED_BG		gfx_hex(0x606060)
#define	SPECIAL_UP_BG		GFX_YELLOW
#define	SPECIAL_DOWN_BG		gfx_hex(0x808000)
#define	SPECIAL_DISABLED_BG	gfx_hex(0x404030)

#define	BUTTON_LINGER_MS	150

#define	LABEL_MARGIN		2

/*
 * Generally approximate the golden ratio: 1:1.618 or 0.384:0.615, but increase
 * the font size since since we have only one small descender (for Q).
 */

#define	FONT_1_TOP		mono24
#define	FONT_1_BOTTOM		mono18

#define	FONT_2			mono34

#define	LABEL_TOP_OFFSET	-12
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


struct ui_entry_ctx {
	/* from ui_entry_params */
	const char *title;
	const struct ui_entry_style *style;
	char *buf;
	unsigned max_len;
	bool (*validate)(void *user, const char *s);
	void *user;

	/* run-time variables */
	unsigned input_max_height;
	const char *second;
	struct timer t_button;
	unsigned n;
};


/* --- Style --------------------------------------------------------------- */


static const struct ui_entry_style default_style = {
	.input_fg		= GFX_WHITE,
	.input_valid_bg		= GFX_HEX(0x002060),
	.input_invalid_bg	= GFX_HEX(0x800000),
	.title_fg		= GFX_WHITE,
	.title_bg		= GFX_BLACK,
};


/* --- First page ---------------------------------------------------------- */


static const char *first_map[] = {
	"0 /!?",
	"1@\"+#",
	"2ABC",
	"3DEF",
	"4GHI",
	"5JKL",
	"6MNO",
	"7PQRS",
	"8TUV",
	"9WXYZ"
};


/* --- Second pages -------------------------------------------------------- */


static const char *second_maps[] = {
	"0&!?:;/ .,",
	"1'\"`+-*@#$",
	"2ABCabc(&)",
	"3DEFdef<=>",
	"4GHIghi[\\]",
	"5JKLjkl{|}",
	"6MNOmno^_~",
	"7PQRpqrSs",
	"8TUVtuv%",
	"9WXYwxyZz"
};


/* --- Input validation ---------------------------------------------------- */


static bool valid(struct ui_entry_ctx *c)
{
	return !*c->buf || !c->validate || c->validate(c->user, c->buf);
}


/* --- Input field --------------------------------------------------------- */


/* @@@ we should just clear the bounding box */

static void clear_input(struct ui_entry_ctx *c)
{
	gfx_rect_xy(&da, 0, 0, GFX_WIDTH,
	    INPUT_PAD_TOP + c->input_max_height + INPUT_PAD_BOTTOM,
	    !*c->buf && c->title ? c->style->title_bg :
	    valid(c) ? c->style->input_valid_bg : c->style->input_invalid_bg);
}


/*
 * @@@ the "fb" allocation was
 *
 * gfx_color
 *	fb[entry_params.max_len * input_max_height * input_max_height];
 *
 * in function draw_top. This caused stack problems.
 */

static PSRAM gfx_color fb[100000];


static void draw_top(struct ui_entry_ctx *c, const char *s,
    const struct font *font, gfx_color color, gfx_color bg)
{
	struct gfx_drawable buf;
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
	assert(q.next <= c->max_len * c->input_max_height);
	assert((unsigned) q.h <=
	    INPUT_PAD_TOP + c->input_max_height + INPUT_PAD_BOTTOM);
	gfx_da_init(&buf, q.next, q.h, fb);
	gfx_clear(&buf, bg);
	text_text(&buf, 0, 0, s, font, GFX_LEFT, GFX_TOP | GFX_MAX,
	    color);
	if ((GFX_WIDTH + q.next) / 2 < INPUT_MAX_X)
		gfx_copy(&da, (GFX_WIDTH - q.next) / 2, INPUT_PAD_TOP, &buf,
		    0, 0, q.next, q.h, -1);
	else if (q.next < INPUT_MAX_X)
		gfx_copy(&da, INPUT_MAX_X - q.next, INPUT_PAD_TOP, &buf, 0, 0,
		    q.next, q.h, -1);
	else
		gfx_copy(&da, 0, INPUT_PAD_TOP, &buf, q.next - INPUT_MAX_X, 0,
		    INPUT_MAX_X, q.h, -1);
}


static void draw_input(struct ui_entry_ctx *c)
{
	if (!*c->buf && c->title)
		text_text(&da, GFX_WIDTH / 2,
		    (INPUT_PAD_TOP + c->input_max_height +
		    INPUT_PAD_BOTTOM) / 2,
		    c->title,
		    c->style->title_font ? c->style->title_font :
		    &DEFAULT_TITLE_FONT,
		    GFX_CENTER, GFX_CENTER, c->style->title_fg);
	else
		draw_top(c, c->buf,
		    c->style->input_font ? c->style->input_font :
		    &DEFAULT_INPUT_FONT,
		    c->style->input_fg,
		    valid(c) ? c->style->input_valid_bg :
		    c->style->input_invalid_bg);
}


/* --- Common for all buttons ---------------------------------------------- */


static void base(unsigned x, unsigned y, gfx_color bg)
{
	gfx_rrect_xy(&da, x - BUTTON_W / 2, y - BUTTON_H / 2,
	    BUTTON_W, BUTTON_H, BUTTON_R, bg);
}


/* --- Draw buttons (first level) ------------------------------------------ */


static void first_label(unsigned x, unsigned y, const char *s)
{
	char top[] = { *s, 0 };

	text_text(&da, x, y + LABEL_TOP_OFFSET, top, &FONT_1_TOP,
	    GFX_CENTER, GFX_CENTER, GFX_BLACK);
	text_text(&da, x, y + LABEL_BOTTOM_OFFSET, s + 1, &FONT_1_BOTTOM,
	    GFX_CENTER, GFX_CENTER, GFX_BLACK);
}


static void first_button(struct ui_entry_ctx *c, unsigned col, unsigned row,
    gfx_color bg)
{
	unsigned x = BUTTON_X0 + BUTTON_X_SPACING * col;
	unsigned y = BUTTON_Y1 - BUTTON_Y_SPACING * row;

	if (row > 0) {
		base(x, y, bg);
		first_label(x, y, first_map[1 + col + (3 - row) * 3]);
	} else if (col == 0) {	// X
		base(x, y, bg);
		if (*c->buf)
			gfx_equilateral(&da, x, y, BUTTON_H * 0.7, -1,
			    GFX_BLACK);
		else
			gfx_diagonal_cross(&da, x, y, BUTTON_H * 0.4, 4,
			    GFX_BLACK);
	} else if (col == 1) {	// "0"
		base(x, y, bg);
		first_label(x, y, first_map[0]);
	} else {	// >
		if (*c->buf) {
			base(x, y, valid(c) ? bg : SPECIAL_DISABLED_BG);
			gfx_equilateral(&da, x, y, BUTTON_H * 0.7, 1,
			    GFX_BLACK);
		} else {
			base(x, y, GFX_BLACK);
		}
	}
}


static void draw_first_text(struct ui_entry_ctx *c, bool enabled)
{
	unsigned row, col;

	for (col = 0; col != 3; col++)
		for (row = 0; row != 4; row++)
			if (row > 0 || col == 1)
				first_button(c, col, row,
				    enabled ? UP_BG : DISABLED_BG);
}


/* --- Draw buttons (second level) ----------------------------------------- */


static void second_label(unsigned x, unsigned y, char ch)
{
	char s[] = { ch, 0 };

	/*
	 * Characters that are hard to recognize if vertically centered, e.g.,
	 * minus and underscore would look the same. Some lower-case letters
	 * also look a little odd when centered, but they still are easily
	 * recognizable, so we probably don't need to do anything about them.
	 */
	bool tricky = strchr("'\"`_,.", ch);

	text_text(&da, x, y, s, &FONT_2, GFX_CENTER,
	    tricky ? GFX_CENTER | GFX_MAX : GFX_CENTER, GFX_BLACK);
}


static void second_button(const char *map, unsigned col, unsigned row,
    gfx_color bg)
{
	unsigned x = BUTTON_X0 + BUTTON_X_SPACING * col;
	unsigned y = BUTTON_Y1 - BUTTON_Y_SPACING * row;
	size_t len = strlen(map);
	unsigned n = 1 + col + (3 - row) * 3;

	if (row > 0 && n >= len)
		return;
	if (row > 0) {
		base(x, y, bg);
		second_label(x, y, map[n]);
	} else if (col == 0) {	// X
		base(x, y, SPECIAL_UP_BG);
		gfx_equilateral(&da, x, y, BUTTON_H * 0.7, -1, GFX_BLACK);
	} else if (col == 1) {	// "0"
		base(x, y, bg);
		second_label(x, y, map[0]);
	} else {	// >
		base(x, y, GFX_BLACK);
	}
}


static void draw_second(const char *map)
{
	const unsigned h = 4 * BUTTON_H + 2 * BUTTON_X_GAP;
	unsigned row, col;

	gfx_rect_xy(&da, 0, GFX_HEIGHT - h - BUTTON_BOTTOM_OFFSET,
	    GFX_WIDTH, h, GFX_BLACK);
	for (col = 0; col != 3; col++)
		for (row = 0; row != 4; row++)
			second_button(map, col, row, UP_BG);
}


/* --- Event handling ------------------------------------------------------ */


static void release_button(void *user)
{
	struct ui_entry_ctx *c = user;

	first_button(c, c->n >> 4, c->n & 15, SPECIAL_UP_BG);
	update_display(&da);
}


static void ui_entry_tap(void *ctx, unsigned x, unsigned y)
{
	struct ui_entry_ctx *c = ctx;
	unsigned col = x < BUTTON_X0 + BUTTON_X_SPACING / 2 ? 0 :
	    x < BUTTON_X0 + 1.5 * BUTTON_X_SPACING ? 1 : 2;
	unsigned row = 0;
	unsigned n;
	char *end = strchr(c->buf, 0);

	if (y < BUTTON_Y0 - BUTTON_Y_SPACING + BUTTON_R)
		return;
	if (y < BUTTON_Y0)
		row = 3;
	else if (y > BUTTON_Y1)
		row = 0;
	else
		row = 3 - (y - BUTTON_Y0 + BUTTON_Y_SPACING / 2) /
		    BUTTON_Y_SPACING;

	debug("entry_tap X %u Y %u -> col %u row %u\n", x, y, col, row);
	if (col == 0 && row == 0) { // cancel or delete
		if (c->second) {
			c->second = NULL;
			draw_first_text(c, 1);
			first_button(c, 0, 0, SPECIAL_UP_BG);
			first_button(c, 2, 0, SPECIAL_UP_BG);
			update_display(&da);
			return;
		}
		if (!*c->buf) {
			ui_return();
			return;
		}
		timer_flush(&c->t_button);
		first_button(c, 0, 0, SPECIAL_DOWN_BG);
		c->n = col << 4 | row;
		timer_set(&c->t_button, BUTTON_LINGER_MS, release_button, c);
		progress();

		end[-1] = 0;
		clear_input(c);
		draw_input(c);
		if (!*c->buf)
			first_button(c, 2, 0, SPECIAL_UP_BG);
		if (end - c->buf == (int) c->max_len)
			draw_first_text(c, 1);
		update_display(&da);
		return;
	}
	if (col == 2 && row == 0) { // enter
		if (c->second || !*c->buf)
			return;
		if (valid(c))
			ui_return();
		return;
	}
	if (end - c->buf == (int) c->max_len)
		return;
	progress();
	timer_flush(&c->t_button);
	n = row ? (3 - row) * 3 + col + 1 : 0;
	if (c->second) {
		if (n >= strlen(c->second))
			return;
		end[0] = c->second[n];
		end[1] = 0;
		c->second = NULL;
		clear_input(c);
		draw_input(c);
		first_button(c, 0, 0, SPECIAL_UP_BG);
		first_button(c, 2, 0, SPECIAL_UP_BG);
		draw_first_text(c, strlen(c->buf) != c->max_len);
	} else {
		c->second = second_maps[n];
		draw_second(c->second);
	}
	update_display(&da);
}


static void ui_entry_to(void *ctx, unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe)
{
	struct ui_entry_ctx *c = ctx;

	if (swipe == us_left) {
		*c->buf = 0;
		ui_return();
	}
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_entry_open(void *ctx, void *params)
{
	struct ui_entry_ctx *c = ctx;
	const struct ui_entry_params *prm = params;
	struct text_query q;

	assert(strlen(prm->buf) <= prm->max_len);
	c->title = prm->title;
	c->style = prm->style ? prm->style : &default_style;
	c->buf = prm->buf;
	c->max_len = prm->max_len;
	c->validate = prm->validate;
	c->user = prm->user;
	c->second = NULL;

	text_query(0, 0, "",
	    c->style->input_font ? c->style->input_font : &DEFAULT_INPUT_FONT,
	    GFX_TOP | GFX_MAX, GFX_TOP | GFX_MAX, &q);
	c->input_max_height = q.h;

	clear_input(c);
	draw_input(c);
	first_button(c, 0, 0, SPECIAL_UP_BG);
	draw_first_text(c, strlen(prm->buf) != prm->max_len);
	first_button(c, 2, 0, SPECIAL_UP_BG);
	timer_init(&c->t_button);
}


static void ui_entry_close(void *ctx)
{
	struct ui_entry_ctx *c = ctx;

	timer_cancel(&c->t_button);
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_entry_events = {
	.touch_tap	= ui_entry_tap,
	.touch_to	= ui_entry_to,
};


const struct ui ui_entry = {
	.name		= "entry",
	.ctx_size	= sizeof(struct ui_entry_ctx),
	.open		= ui_entry_open,
	.close		= ui_entry_close,
	.events		= &ui_entry_events,
};
