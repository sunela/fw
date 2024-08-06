/*
 * ui_off.c - User interface: sleep/standby
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include "hal.h"
#include "ui.h"


static void ui_off_open(void *ctx, void *params)
{
	/*
	 * Erase the frame buffer, so that one cannot see possibly secret
	 * information from the controller chip or just by illuminating the
	 * panel.
	 */
	/*
	 * @@@ This causes the display to get erased, from top to bottom,
	 * slowly enough that one can see the progress.
	 *
	 * For a better visual effect, we should turn it off at once. To do
	 * this, we should first turn off displaying (e.g., backlight, maybe
	 * the display refresh part of the controller as well), only then erase
	 * the frame buffer, and finally do any additional shutdown that might
	 * interfere with frame buffer updates.
	 */
	gfx_clear(&main_da, GFX_BLACK);
	ui_update_display();
	display_on(0);
}


static void ui_off_close(void *ctx)
{
	display_on(1);
}


const struct ui ui_off = {
	.name	= "off",
	.open	= ui_off_open,
	.close	= ui_off_close,
};
