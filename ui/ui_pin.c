/*
 * ui_pin.c - User interface: PIN input
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#include "hal.h"
#include "debug.h"
#include "timer.h"
#include "rnd.h"
#include "gfx.h"
#include "text.h"
#include "db.h"
#include "pin.h"
#include "shape.h"
#include "ui.h"


/* --- Keypad -------------------------------------------------------------- */

#define UP_BG			GFX_WHITE
#define DOWN_BG			gfx_hex(0x808080)
#define	SPECIAL_BG		GFX_YELLOW
#define	DISABLED_BG		gfx_hex(0x606060)

#define	BUTTON_LINGER_MS	150
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
#define	X_ADJUST(ch)		(-(ch == '4'))
#define	Y_ADJUST(ch)		1

/* --- Input/progress indicators ------------------------------------------- */

#define	IND_COLOR		gfx_hex(0x80c0ff)

#define	IND_TOP_OFFSET		10
#define	IND_R			8
#define	IND_GAP			10
#define	IND_SPACING		(2 * IND_R + IND_GAP)
#define	IND_CX			(GFX_WIDTH / 2)
#define	IND_CY			(IND_TOP_OFFSET + IND_R)

/* --- Database opening progress ------------------------------------------- */

#define	PROGRESS_TOTAL_COLOR	GFX_HEX(0x202020)
#define	PROGRESS_DONE_COLOR	GFX_GREEN

#define	PROGRESS_W		180
#define	PROGRESS_H		20
#define	PROGRESS_X0		((GFX_WIDTH - PROGRESS_W) / 2)
#define	PROGRESS_Y0		((GFX_HEIGHT - PROGRESS_H) / 2)


static uint32_t pin = 0xffffffff;
static unsigned pin_len = 0;
static struct timer t_button;
static uint8_t shuffle[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };


/* --- Draw input indicators ----------------------------------------------- */


static void clear_indicators(unsigned n)
{
	unsigned side = (IND_SPACING * (n - 1) + 1) / 2 + IND_R;

	if (n)
		gfx_rect_xy(&main_da, IND_CX - side, IND_CY - IND_R,
		    2 * side + 1, 2 * IND_R + 1, GFX_BLACK);
}


static void draw_indicators(unsigned n)
{
	unsigned x = IND_CX - (IND_SPACING * (n - 1) / 2);
	unsigned i;

	for (i = 0; i != n; i++)
		gfx_disc(&main_da, x + IND_SPACING * i, IND_CY, IND_R,
		    IND_COLOR);
}


/* --- Draw buttons -------------------------------------------------------- */


static void clear_button(unsigned col, unsigned row)
{
	unsigned x = BUTTON_X0 + BUTTON_X_SPACING * col;
	unsigned y = BUTTON_Y1 - BUTTON_Y_SPACING * row;

	gfx_rect_xy(&main_da, x - BUTTON_R, y - BUTTON_R,
	    2 * BUTTON_R + 1, 2 * BUTTON_R + 1 , GFX_BLACK);
}


static void pin_char(unsigned x, unsigned y, char ch)
{
	char s[] = { ch, 0 };

	text_text(&main_da, x + X_ADJUST(ch), y + Y_ADJUST(ch), s, &FONT,
	    GFX_CENTER, GFX_CENTER, GFX_BLACK);
}


static void pin_digit(unsigned x, unsigned y, uint8_t digit)
{
	pin_char(x, y, '0' + shuffle[digit]);
}


static void pin_button(unsigned col, unsigned row, gfx_color bg)
{
	unsigned x = BUTTON_X0 + BUTTON_X_SPACING * col;
	unsigned y = BUTTON_Y1 - BUTTON_Y_SPACING * row;

	gfx_disc(&main_da, x, y, BUTTON_R, bg);
	if (row > 0) {
		pin_digit(x, y, 1 + col + (3 - row) * 3);
	} else if (col == 0) {	// X
		gfx_diagonal_cross(&main_da, x, y, BUTTON_R * 0.8, 4,
		    GFX_BLACK);
	} else if (col == 1) {	// "0"
		pin_digit(x, y, 0);
	} else {	// >
		gfx_equilateral(&main_da, x, y, BUTTON_R * 1.4, 1, GFX_BLACK);
	}
}


static void draw_digits(gfx_color bg)
{
	unsigned row, col;

	for (col = 0; col != 3; col++)
		for (row = 0; row != 4; row++)
			if (row > 0 || col == 1)
				pin_button(col, row, bg);
}


/* --- Event handling ------------------------------------------------------ */


static void release_button(void *user)
{
	unsigned n = (uintptr_t) user;

debug("release_button 0x%x\n", n);
	pin_button(n >> 4, n & 15,
	    pin_len < MAX_PIN_LEN ? UP_BG : DISABLED_BG);
	ui_update_display();
}


static void open_progress(void *user, unsigned i, unsigned n)
{
	unsigned *progress = user;
	unsigned w = n ? i * PROGRESS_W / n : 0;

	if (*progress == w)
		return;
	gfx_rect_xy(&main_da, PROGRESS_X0 + *progress, PROGRESS_Y0,
	    w - *progress, PROGRESS_H, PROGRESS_DONE_COLOR);
	ui_update_display();
	*progress = w;
}


static bool accept_pin(void)
{
	struct db_stats s;
	unsigned progress = 0;

	if (pin != DUMMY_PIN)
		return 0;
	gfx_clear(&main_da, GFX_BLACK);
	gfx_rect_xy(&main_da, PROGRESS_X0, PROGRESS_Y0, PROGRESS_W, PROGRESS_H,
	    PROGRESS_TOTAL_COLOR);
	ui_update_display();	/* give immediate visual feedback */
	if (!db_open_progress(&main_db, NULL, open_progress, &progress))
		return 0;
	db_stats(&main_db, &s);
	/*
	 * @@@ also let us in if there Flash has been erased. This is for
	 * development - in the end, an erased Flash should bypass the PIN
	 * dialog and go through a setup procedure, e.g., asking for a new PIN,
	 * and writing some record (configuration ?), to "pin" the PIN.
	 */
	return s.data || s.empty || (s.erased == s.total);
}


static void ui_pin_tap(void *ctx, unsigned x, unsigned y)
{
	unsigned col = x < BUTTON_X0 + BUTTON_X_SPACING / 2 ? 0 :
	    x < BUTTON_X0 + 1.5 * BUTTON_X_SPACING ? 1 : 2;
	unsigned row = 0;
	unsigned n;

	if (y < BUTTON_Y0 - BUTTON_Y_SPACING + BUTTON_R)
		return;
	if (y < BUTTON_Y0)
		row = 3;
	else if (y > BUTTON_Y1)
		row = 0;
	else
		row = 3 - (y - BUTTON_Y0 + BUTTON_Y_SPACING / 2) /
		    BUTTON_Y_SPACING;

	debug("pin_tap X %u Y %u -> col %u row %u\n", x, y, col, row);
	if (col == 0 && row == 0) { // cancel
		if (pin_len == 0)
			return;
		progress();
		timer_flush(&t_button);
		clear_indicators(pin_len);
		clear_button(0, 0);
		if (pin_len >= MIN_PIN_LEN)
			clear_button(2, 0);
		if (pin_len == MAX_PIN_LEN)
			draw_digits(UP_BG);
		pin_len = 0;
		pin = 0xffffffff;
		ui_update_display();
		return;
	}
	if (col == 2 && row == 0) { // enter
		if (pin_len < MIN_PIN_LEN)
			return;
debug("%08lx\n", (unsigned long) pin);
		progress();
		if (accept_pin()) {
			pin_attempts = 0;
			pin_cooldown = 0;
			ui_switch(&ui_accounts, NULL);
		} else {
			ui_switch(&ui_fail, NULL);
		}
		return;
	}
	if (pin_len == MAX_PIN_LEN)
		return;
	progress();
	timer_flush(&t_button);
	timer_set(&t_button, BUTTON_LINGER_MS, release_button,
	    (void *) (uintptr_t) (col << 4 | row));
	pin_button(col, row, DOWN_BG);
	n = row ? (3 - row) * 3 + col + 1 : 0;
	pin = pin << 4 | shuffle[n];
	pin_len++;
	if (pin_len == 1)
		pin_button(0, 0, SPECIAL_BG);
	if (pin_len == MIN_PIN_LEN)
		pin_button(2, 0, SPECIAL_BG);
	if (pin_len == MAX_PIN_LEN)
		draw_digits(DISABLED_BG);
	clear_indicators(pin_len - 1);
	draw_indicators(pin_len);
	ui_update_display();
}


/* --- Shuffle the digit buttons ------------------------------------------- */


static inline void swap(uint8_t *a, uint8_t *b)
{
	uint8_t tmp = *a;

	*a = *b;
	*b = tmp;
}


void pin_shuffle_pad(void)
{
	uint8_t i;

	/*
	 * @@@ use better algorithm
	 */
	for (i = 0; i != 10; i++)
		swap(shuffle + i, shuffle + rnd(10));
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_pin_open(void *ctx, void *params)
{
	unsigned row, col;

	pin = 0xffffffff;
	pin_len = 0;
	for (col = 0; col != 3; col++)
		for (row = 0; row != 4; row++)
			if (row > 0 || col == 1)
				draw_digits(UP_BG);
	timer_init(&t_button);
	set_idle(IDLE_PIN_S);
}


static void ui_pin_close(void *ctx)
{
	timer_cancel(&t_button);
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_pin_events = {
	.touch_tap	= ui_pin_tap,
};


const struct ui ui_pin = {
	.name		= "pin",
	.open		= ui_pin_open,
	.close		= ui_pin_close,
	.events		= &ui_pin_events,
};
