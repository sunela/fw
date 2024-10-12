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
#include "gpio.h"
#include "timer.h"
#include "db.h"
#include "gfx.h"


#define	DEBUG	0


/* scripting is only available in the simulator */
bool scripting = 0;


/* --- Event loop ---------------------------------------------------------- */


static void process_touch(void)
{
	struct cst816_touch t;
	const struct cst816_event *e = t.event;
	static bool down = 0;

#if DEBUG
	debug("TOUCH (%u)\n", down);
#endif
	mdelay(1);
	cst816_read(&t);
	mdelay(1);
#if DEBUG
	if (t.events)
		debug("\t(%u) %u %u %u\n", t.events, e->action, e->x, e->y);
	else
		debug("\tUP\n");
#endif /* DEBUG */
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


static void event_loop(void)
{
	static unsigned uptime = 1;
	static bool button_down = 0;

	while (1) {
		if (button_down != !gpio_in(BUTTON_R)) {
			button_down = !button_down;
			debug("BUTTON (%u)\n", button_down);
			button_event(button_down);
		}

		if (gpio_int_stat(TOUCH_INT)) {
			gpio_int_clr(TOUCH_INT);
			process_touch();
		}

		timer_tick(uptime);
		if (!(uptime & 7))
			tick_event();

		msleep(1);
		uptime++;
	}
}


/* --- Display update ------------------------------------------------------ */


void update_display_partial(struct gfx_drawable *da, unsigned x, unsigned y)
{
	st7789_update_partial(da->fb, 0, 0, x, y, da->w, da->h, da->w);
}


void update_display(struct gfx_drawable *da)
{
	if (!da->changed)
		return;
	gfx_reset(da);
	assert(da->damage.w);
	assert(da->damage.h);

#if DEBUG
	unsigned n_pix = da->damage.w * da->damage.h;
	double dt;

	t0();
#endif /* DEBUG */

	st7789_update(da->fb, da->damage.x, da->damage.y,
	    da->damage.x + da->damage.w - 1, da->damage.y + da->damage.h - 1);

#if DEBUG
	dt = t1(NULL);
	t1("D %u %u + %u %u: %u px (%.3f Mbps)\n",
	    da->damage.x, da->damage.y, da->damage.w, da->damage.h,
	    n_pix, n_pix / dt * 16e-6);
#endif /* DEBUG */
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
	backlight_init(LCD_BL, LCD_BL_INVERTED);

	// @@@ no on-off control yet
	st7789_on();
	st7789_init(LCD_SPI, LCD_RST, LCD_DnC, GFX_WIDTH, GFX_HEIGHT, 0, 20);
	st7789_on();

	/*
	 * INT_STAT is only set if INT_MASK is zero.
	 */
	gpio_cfg_int(TOUCH_INT, GPIO_INT_MODE_FALL);
	cst816_init(TOUCH_I2C, TOUCH_I2C_ADDR, TOUCH_INT);

	db_init();
	app_init(NULL, 0);
	event_loop();
}
