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
