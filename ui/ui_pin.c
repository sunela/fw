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
#include "gfx.h"
#include "ui.h"


#define	MIN_PIN_LEN		4
#define	MAX_PIN_LEN		8


/* --- Keypad -------------------------------------------------------------- */

#define UP_BG			GFX_WHITE
#define DOWN_BG			gfx_hex(0x808080)
#define	SPECIAL_BG		GFX_YELLOW
#define	DISABLED_BG		gfx_hex(0x606060)

#define	BUTTON_LINGER_MS	150
#define	FONT_SIZE		46

#define	BUTTON_BOTTOM_OFFSET	3
#define	BUTTON_R		25
#define	BUTTON_X_GAP		20
#define	BUTTON_Y_GAP		10	
#define	BUTTON_X_SPACING	(2 * BUTTON_R + BUTTON_X_GAP)
#define	BUTTON_Y_SPACING	(2 * BUTTON_R + BUTTON_Y_GAP)
#define	BUTTON_X0		(GFX_WIDTH / 2 - BUTTON_X_SPACING)
#define	BUTTON_Y1		(GFX_HEIGHT - BUTTON_R - BUTTON_BOTTOM_OFFSET)
#define	BUTTON_Y0		(BUTTON_Y1 - 3 * BUTTON_Y_SPACING)
#define	X_ADJUST(ch)		((ch) != '4')
#define	Y_ADJUST(ch)		1

/* --- Input/progress indicators ------------------------------------------- */

#define	IND_COLOR		gfx_hex(0x80c0ff)

#define	IND_TOP_OFFSET		10
#define	IND_R			8
#define	IND_GAP			10
#define	IND_SPACING		(2 * IND_R + IND_GAP)
#define	IND_CX			(GFX_WIDTH / 2)
#define	IND_CY			(IND_TOP_OFFSET + IND_R)


static uint32_t pin = 0xffffffff;
static unsigned pin_len = 0;
static struct timer t_button;


/* --- Draw input indicators ----------------------------------------------- */


static void clear_indicators(unsigned n)
{
	unsigned side = (IND_SPACING * (n - 1) + 1) / 2 + IND_R;

	if (n)
		gfx_rect_xy(&da, IND_CX - side, IND_CY - IND_R,
		    2 * side + 1, 2 * IND_R + 1, GFX_BLACK);
}


static void draw_indicators(unsigned n)
{
	unsigned x = IND_CX - (IND_SPACING * (n - 1) / 2);
	unsigned i;

	for (i = 0; i != n; i++)
		gfx_disc(&da, x + IND_SPACING * i, IND_CY, IND_R, IND_COLOR);
}


/* --- Draw buttons -------------------------------------------------------- */


static void clear_button(unsigned col, unsigned row)
{
	unsigned x = BUTTON_X0 + BUTTON_X_SPACING * col;
        unsigned y = BUTTON_Y1 - BUTTON_Y_SPACING * row;
	
	gfx_rect_xy(&da, x - BUTTON_R, y - BUTTON_R,
	    2 * BUTTON_R + 1, 2 * BUTTON_R + 1 , GFX_BLACK);
}


static void pin_char(unsigned x, unsigned y, char ch)
{
	char s[] = { ch, 0 };

	gfx_text(&da, x + X_ADJUST(ch), y + Y_ADJUST(ch), s, FONT_SIZE,
	    GFX_CENTER, GFX_CENTER, GFX_BLACK);
}


static void cross(unsigned x, unsigned y, unsigned r, unsigned w)
{
	unsigned d0 = (r - w) / 1.414;
	unsigned d1 = (r + w) / 1.414;
	short v1[] = {
		x + d0, y - d1,
		x + d1, y - d0,
		x - d0, y + d1,
		x - d1, y + d0
	};
	short v2[] = {
		x - d0, y - d1,
		x - d1, y - d0,
		x + d0, y + d1,
		x + d1, y + d0
	};

	gfx_poly(&da, 4, v1, GFX_BLACK);
	gfx_poly(&da, 4, v2, GFX_BLACK);
}


static void equilateral(unsigned x, unsigned y, unsigned a)
{
	/* R = a / sqrt(3); R = 2 r */
	unsigned R = a / 1.732;
	unsigned r = R / 2;
	short v[] = {
		x - r, y - a / 2,
		x - r, y + a / 2,
		x + R, y
	};

	gfx_poly(&da, 3, v, GFX_BLACK);
}


static void pin_button(unsigned col, unsigned row, gfx_color bg)
{
	unsigned x = BUTTON_X0 + BUTTON_X_SPACING * col;
        unsigned y = BUTTON_Y1 - BUTTON_Y_SPACING * row;

	gfx_disc(&da, x, y, BUTTON_R, bg);
	if (row > 0) {
		pin_char(x, y, '1' + (col + (3 - row) * 3));
	} else if (col == 0) {	// X
		cross(x, y, BUTTON_R * 0.8, 4);
	} else if (col == 1) {	// "0"
		pin_char(x, y, '0');
	} else {	// >
		equilateral(x, y, BUTTON_R * 1.4);
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
	update_display(&da);
}


static void ui_pin_tap(unsigned x, unsigned y)
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
		timer_flush(&t_button);
		clear_indicators(pin_len);
		clear_button(0, 0);
		if (pin_len >= MIN_PIN_LEN)
			clear_button(2, 0);
		if (pin_len == MAX_PIN_LEN)
			draw_digits(UP_BG);
		pin_len = 0;
		pin = 0xffffffff;
		update_display(&da);
		return;
	}
	if (col == 2 && row == 0) { // enter
		if (pin_len < MIN_PIN_LEN)
			return;
		printf("%08x\n", pin);
		return;
	}
	if (pin_len == MAX_PIN_LEN)
		return;
	timer_flush(&t_button);
	timer_set(&t_button, BUTTON_LINGER_MS, release_button,
	    (void *) (uintptr_t) (col << 4 | row));
	pin_button(col, row, DOWN_BG);
	n = row ? (3 - row) * 3 + col + 1 : 0;
	pin = pin << 4 | n;
	pin_len++;
	if (pin_len == 1)
		pin_button(0, 0, SPECIAL_BG);
	if (pin_len == MIN_PIN_LEN)
		pin_button(2, 0, SPECIAL_BG);
	if (pin_len == MAX_PIN_LEN)
		draw_digits(DISABLED_BG);
	clear_indicators(pin_len - 1);
	draw_indicators(pin_len);
	update_display(&da);
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_pin_open(void)
{
	unsigned row, col;

	for (col = 0; col != 3; col++)
		for (row = 0; row != 4; row++)
			if (row > 0 || col == 1)
				draw_digits(UP_BG);
	timer_init(&t_button);
}


static void ui_pin_close(void)
{
	timer_cancel(&t_button);
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_pin_events = {
	.touch_tap	= ui_pin_tap,
};


const struct ui ui_pin = {
	.open = ui_pin_open,
	.close = ui_pin_close,
	.events	= &ui_pin_events,
};
