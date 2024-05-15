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
#define	INPUT_FONT		mono34
#define INPUT_MAX_X		210

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


/* --- Style --------------------------------------------------------------- */


static const struct ui_entry_style default_style = {
	.input_fg		= GFX_WHITE,
	.input_valid_bg		= GFX_HEX(0x002060),
	.input_invalid_bg	= GFX_HEX(0x800000),
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


/* ---  Variables ---------------------------------------------------------- */


static struct ui_entry_params entry_params;
static unsigned input_max_height;
static const char *second = NULL;
static struct timer t_button;


/* --- Input validation ---------------------------------------------------- */


static bool valid(void)
{
	return !*entry_params.buf || !entry_params.validate ||
	    entry_params.validate(entry_params.user, entry_params.buf);
}


/* --- Input field --------------------------------------------------------- */


/* @@@ we should just clear the bounding box */

static void clear_input(void)
{
	const struct ui_entry_style *style =
	    entry_params.style ? entry_params.style : &default_style;

	gfx_rect_xy(&da, 0, 0, GFX_WIDTH,
	    INPUT_PAD_TOP + input_max_height + INPUT_PAD_BOTTOM,
	    valid() ? style->input_valid_bg : style->input_invalid_bg);
}


static void draw_input(void)
{
	const struct ui_entry_style *style =
	    entry_params.style ? entry_params.style : &default_style;
	struct gfx_drawable buf;
	gfx_color
	    fb[entry_params.max_len * input_max_height * input_max_height];
	struct text_query q;

	if (!*entry_params.buf)
		return;
	/*
	 * For the text width, we use the position of the next character
	 * (q.next) instread of the bounding box width (q.w), so that there is
	 * visual feedback when entering a space. (A trailing space doesn't
	 * grow the bounding box. Multiple spaces would, though, which is
	 * somewhat inconsistent.)
	 */
	text_query(0, 0, entry_params.buf, &INPUT_FONT,
	    GFX_LEFT, GFX_TOP | GFX_MAX, &q);
	assert(q.next <= entry_params.max_len * input_max_height);
	assert((unsigned) q.h <=
	    INPUT_PAD_TOP + input_max_height + INPUT_PAD_BOTTOM);
	gfx_da_init(&buf, q.next, q.h, fb);
	gfx_clear(&buf,
	    valid() ? style->input_valid_bg : style->input_invalid_bg);
	text_text(&buf, 0, 0, entry_params.buf, &INPUT_FONT,
	    GFX_LEFT, GFX_TOP | GFX_MAX, style->input_fg);
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


static void first_button(unsigned col, unsigned row, gfx_color bg)
{
	unsigned x = BUTTON_X0 + BUTTON_X_SPACING * col;
	unsigned y = BUTTON_Y1 - BUTTON_Y_SPACING * row;

	if (row > 0) {
		base(x, y, bg);
		first_label(x, y, first_map[1 + col + (3 - row) * 3]);
	} else if (col == 0) {	// X
		base(x, y, bg);
		if (*entry_params.buf)
			gfx_equilateral(&da, x, y, BUTTON_H * 0.7, -1,
			    GFX_BLACK);
		else
			gfx_diagonal_cross(&da, x, y, BUTTON_H * 0.4, 4,
			    GFX_BLACK);
	} else if (col == 1) {	// "0"
		base(x, y, bg);
		first_label(x, y, first_map[0]);
	} else {	// >
		if (*entry_params.buf) {
			base(x, y, valid() ? bg : SPECIAL_DISABLED_BG);
			gfx_equilateral(&da, x, y, BUTTON_H * 0.7, 1,
			    GFX_BLACK);
		} else {
			base(x, y, GFX_BLACK);
		}
	}
}


static void draw_first_text(bool enabled)
{
	unsigned row, col;

	for (col = 0; col != 3; col++)
		for (row = 0; row != 4; row++)
			if (row > 0 || col == 1)
				first_button(col, row,
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
	unsigned n = (uintptr_t) user;

	first_button(n >> 4, n & 15, SPECIAL_UP_BG);
	update_display(&da);
}


static void ui_entry_tap(unsigned x, unsigned y)
{
	unsigned col = x < BUTTON_X0 + BUTTON_X_SPACING / 2 ? 0 :
	    x < BUTTON_X0 + 1.5 * BUTTON_X_SPACING ? 1 : 2;
	unsigned row = 0;
	unsigned n;
	char *end = strchr(entry_params.buf, 0);

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
		if (second) {
			second = NULL;
			draw_first_text(1);
			first_button(0, 0, SPECIAL_UP_BG);
			first_button(2, 0, SPECIAL_UP_BG);
			update_display(&da);
			return;
		}
		if (!*entry_params.buf) {
			ui_return();
			return;
		}
		timer_flush(&t_button);
		first_button(0, 0, SPECIAL_DOWN_BG);
		timer_set(&t_button, BUTTON_LINGER_MS, release_button,
		    (void *) (uintptr_t) (col << 4 | row));
		progress();

		end[-1] = 0;
		clear_input();
		draw_input();
		if (!*entry_params.buf)
			first_button(2, 0, SPECIAL_UP_BG);
		if (end - entry_params.buf == entry_params.max_len)
			draw_first_text(1);
		update_display(&da);
		return;
	}
	if (col == 2 && row == 0) { // enter
		if (second || !*entry_params.buf)
			return;
		if (valid())
			ui_return();
		return;
	}
	if (end - entry_params.buf == entry_params.max_len)
		return;
	progress();
	timer_flush(&t_button);
	n = row ? (3 - row) * 3 + col + 1 : 0;
	if (second) {
		if (n >= strlen(second))
			return;
		end[0] = second[n];
		end[1] = 0;
		second = NULL;
		clear_input();
		draw_input();
		first_button(0, 0, SPECIAL_UP_BG);
		first_button(2, 0, SPECIAL_UP_BG);
		draw_first_text(
		    strlen(entry_params.buf) != entry_params.max_len);
	} else {
		second = second_maps[n];
		draw_second(second);
	}
	update_display(&da);
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_entry_open(void *params)
{
	const struct ui_entry_params *prm = params;
	struct text_query q;

	assert(strlen(prm->buf) <= prm->max_len);
	entry_params = *prm;

	text_query(0, 0, "", &INPUT_FONT,
	    GFX_TOP | GFX_MAX, GFX_TOP | GFX_MAX, &q);
	input_max_height = q.h;

	clear_input();
	draw_input();
	first_button(0, 0, SPECIAL_UP_BG);
	draw_first_text(strlen(prm->buf) != prm->max_len);
	first_button(2, 0, SPECIAL_UP_BG);
	timer_init(&t_button);
}


static void ui_entry_close(void)
{
	timer_cancel(&t_button);
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_entry_events = {
	.touch_tap	= ui_entry_tap,
};


const struct ui ui_entry = {
	.open = ui_entry_open,
	.close = ui_entry_close,
	.events	= &ui_entry_events,
};
