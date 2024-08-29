/*
 * wi_pin_entry.c - User interface: PIN entry
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <string.h> //
#include <assert.h>

#include "hal.h"
#include "gfx.h"
#include "shape.h"
#include "colors.h"
#include "text.h"
#include "ui.h"
#include "ui_entry.h"
#include "wi_pin_entry.h"


/* --- Input/progress indicators ------------------------------------------- */

#define	IND_COLOR		gfx_hex(0x80c0ff)

#define	IND_TOP_OFFSET		10
#define	IND_R			8
#define	IND_GAP			10
#define	IND_SPACING		(2 * IND_R + IND_GAP)
#define	IND_CX			(GFX_WIDTH / 2)
#define	IND_CY			(IND_TOP_OFFSET + IND_R)

#define	DEFAULT_TITLE_FONT	mono18

/* --- Keypad -------------------------------------------------------------- */

#define	FONT			mono36

#define	BUTTON_BOTTOM_OFFSET	3
#define	BUTTON_R		25
#define	BUTTON_X_GAP		20
#define	BUTTON_Y_GAP		10
#define	BUTTON_X_SPACING	(2 * BUTTON_R + BUTTON_X_GAP)
#define	BUTTON_Y_SPACING	(2 * BUTTON_R + BUTTON_Y_GAP)
#define	BUTTON_X0		(GFX_WIDTH / 2 - BUTTON_X_SPACING)
#define	BUTTON_Y1		(GFX_HEIGHT - BUTTON_R - BUTTON_BOTTOM_OFFSET)
#define	BUTTON_Y0		(BUTTON_Y1 - 3 * BUTTON_Y_SPACING)
#define	X_ADJUST(n)		(-((n) == 4))
#define	Y_ADJUST(n)		1


/* --- Input indicators ---------------------------------------------------- */


static void clear_indicators(void)
{
	gfx_rect_xy(&main_da, 0, IND_CY - IND_R, GFX_WIDTH,
	    2 * IND_R + 1, GFX_BLACK);
}


static void draw_indicators(unsigned n)
{
	unsigned x = IND_CX - (IND_SPACING * (n - 1) / 2);
	unsigned i;

	for (i = 0; i != n; i++)
		gfx_disc(&main_da, x + IND_SPACING * i, IND_CY, IND_R,
		    IND_COLOR);
}


static void draw_title(struct wi_pin_entry_ctx *c)
{
	struct ui_entry_input *in = c->input;
	const struct ui_entry_style *style = c->style;

	text_text(&main_da, GFX_WIDTH / 2, IND_CY, in->title,
	    style->title_font ? style->title_font : &DEFAULT_TITLE_FONT,
	    GFX_CENTER, GFX_CENTER, style->title_fg);
}


static void wi_pin_entry_input(void *user)
{
	struct wi_pin_entry_ctx *c = user;
	struct ui_entry_input *in = c->input;

	clear_indicators();
	if (*in->buf || !in->title)
		draw_indicators(strlen(c->input->buf));
	else
		draw_title(c);
}


/* --- Pad layout ---------------------------------------------------------- */


static void base(unsigned x, unsigned y, gfx_color bg)
{
	gfx_disc(&main_da, x, y, BUTTON_R, bg);
}


static int wi_pin_entry_n(void *user, unsigned col, unsigned row)
{
	struct wi_pin_entry_ctx *c = user;
	int n = 1 + col + (3 - row) * 3;

	if (row == 0) {
		if (col == 1)
			return c->shuffle[0];
		return -1;
	}
	return n  < 0 || n > 9 ? -1 : c->shuffle[n];
}


static void pin_char(unsigned x, unsigned y, unsigned n)
{
	char s[] = { '0' + n, 0 };

	text_text(&main_da, x + X_ADJUST(n), y + Y_ADJUST(n), s, &FONT,
	    GFX_CENTER, GFX_CENTER, GFX_BLACK);
}


static void wi_pin_entry_button(void *user, unsigned col, unsigned row,
    const char *label, bool second, bool enabled, bool up)
{
	struct wi_pin_entry_ctx *c = user;
	struct ui_entry_input *in = c->input;
	unsigned x = BUTTON_X0 + BUTTON_X_SPACING * col;
	unsigned y = BUTTON_Y1 - BUTTON_Y_SPACING * row;
	gfx_color bg;

	assert(!second);
	if (row > 0 || col == 1) {
		unsigned n = wi_pin_entry_n(c, col, row);

		bg = enabled ? up ? UP_BG : DOWN_BG : DISABLED_BG;
		base(x, y, bg);
		pin_char(x, y, n);
		return;
        }

	bg = enabled ? up ? SPECIAL_UP_BG : SPECIAL_DOWN_BG :
	    SPECIAL_DISABLED_BG;
	if (col == 0) {  // X
		base(x, y, bg); 
		if (*in->buf)
			gfx_equilateral(&main_da, x, y, BUTTON_R * 1.4, -1,
			    GFX_BLACK);
		else
			if (c->login)
				gfx_power_sym(&main_da, x, y, BUTTON_R * 0.6,
				    5, GFX_BLACK, bg);
			else
		                gfx_diagonal_cross(&main_da, x, y,
				    BUTTON_R * 0.8, 4, GFX_BLACK);
	} else {        // >
		if (*in->buf) {
			base(x, y, bg);
			gfx_equilateral(&main_da, x, y, BUTTON_R * 1.4, 1,
			    GFX_BLACK);
		} else {
			base(x, y, GFX_BLACK);
		}
	}
}


static void wi_pin_entry_clear_pad(void *user)
{
	const unsigned h = 3 * BUTTON_Y_SPACING + 2 * BUTTON_R;

	gfx_rect_xy(&main_da, 0, GFX_HEIGHT - h - BUTTON_BOTTOM_OFFSET,
            GFX_WIDTH, h, GFX_BLACK);
}


static bool wi_pin_entry_pos(void *user, unsigned x, unsigned y,
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


static void wi_pin_entry_init(void *ctx, struct ui_entry_input *input,
    const struct ui_entry_style *style)
{
	struct wi_pin_entry_ctx *c = ctx;

	c->input = input;
	c->style = style;
}


void wi_pin_entry_setup(struct wi_pin_entry_ctx *c, bool login,
    const uint8_t *shuffle)
{
	int i;

	c->login = login;
	for (i = 0; i != 10; i++)
		c->shuffle[i] = shuffle ? shuffle[i] : i;
}

struct ui_entry_ops wi_pin_entry_ops = {
	.init		= wi_pin_entry_init,
	.input		= wi_pin_entry_input,
	.pos		= wi_pin_entry_pos,
	.n		= wi_pin_entry_n,
	.button		= wi_pin_entry_button,
	.clear_pad	= wi_pin_entry_clear_pad,
};
