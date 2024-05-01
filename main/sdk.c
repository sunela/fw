/*
 * sdk.c - Hardware abstraction layer for the M1s Dock on the Bouffalo SDK
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#include "hw/board.h"
#include "hw/bl808/mmio.h"
#include "hw/bl808/gpio.h"
#include "hw/bl808/spi.h"
#include "hw/bl808/i2c.h"
#include "hw/st7789.h"
#include "hw/cst816.h"
#include "hw/backlight.h"

#include "hal.h"
#include "sdk-hal.h"
#include "debug.h"
#include "timer.h"
#include "gfx.h"


/* --- Event loop ---------------------------------------------------------- */


static void process_touch(void)
{
	struct cst816_touch t;
	const struct cst816_event *e = t.event;
	static bool down = 0;

	debug("TOUCH (%u)\n", down);
	mdelay(1);
	cst816_read(&t);
	mdelay(1);
	if (t.events)
		debug("\t%u %u %u\n", e->action, e->x, e->y);
	else
		debug("\tUP\n");
	if (t.events && e->action != cst816_up) {
		unsigned x = GFX_WIDTH - 1 - e->x;
		unsigned y = GFX_HEIGHT - 1 - e->y;

		if (e->x >= GFX_WIDTH || e->y >= GFX_HEIGHT)
			return;
		if (down)
			 touch_move_event(x, y);
		else
			touch_down_event(x, y);
		down = 1;
	} else {
		if (down)
			touch_up_event();
		down = 0;
	}
}


/*
 * @@@ why do we need to sample on both interrupt edges ?
 */

static void event_loop(void)
{
	static unsigned uptime = 1;
	static int last_touch = 0;
	static bool button_down = 0;

	while (1) {
		bool on;

		if (button_down != !gpio_in(BUTTON_R)) {
			button_down = !button_down;
			debug("BUTTON (%u)\n", button_down);
			button_event(button_down);
		}

		on = !cst816_poll();
		if (on != last_touch)
			process_touch();
		last_touch = on;

		timer_tick(uptime);
		if (!(uptime & 7))
			tick_event();
		msleep(1);
		uptime++;
	}
}


/* --- Display update ------------------------------------------------------ */


void update_display(struct gfx_drawable *da)
{
	if (!da->changed)
		return;
	gfx_reset(da);
	assert(da->damage.w);
	assert(da->damage.h);

	unsigned n_pix = da->damage.w * da->damage.h;
	double dt;

	t0();
	st7789_update(da->fb, da->damage.x, da->damage.y,
	    da->damage.x + da->damage.w - 1, da->damage.y + da->damage.h - 1);
	dt = t1(NULL);
	t1("D %u %u + %u %u: %u px (%.3f Mbps)\n",
	    da->damage.x, da->damage.y, da->damage.w, da->damage.h,
	     n_pix, n_pix / dt * 16e-6);
}


/* --- Display on/off ------------------------------------------------------ */


void display_on(bool on)
{
	/* @@@ also switch the LCD */
	backlight_on(on);
}


/* --- Command-line processing --------------------------------------------- */


void sdk_main(void)
{
	mmio_init();

	gpio_cfg_in(BUTTON_R, GPIO_PULL_UP);

	spi_init(LCD_MOSI, LCD_SCLK, LCD_CS, 15);
	i2c_init(0, I2C0_SDA, I2C0_SCL, 100);
	backlight_init(LCD_BL);

	// @@@ no on-off control yet
	st7789_on();
	st7789_init(LCD_SPI, LCD_RST, LCD_DnC, GFX_WIDTH, GFX_HEIGHT, 0, 20);
	st7789_on();

	cst816_init(TOUCH_I2C, TOUCH_I2C_ADDR, TOUCH_INT);

	app_init(NULL, 0);
	event_loop();
}
