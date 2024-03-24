/*
 * fw.c - Hardware abstraction layer for running on, for now, the M1s Dock
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>

#include "hw/board.h"
#include "hw/bl808/delay.h"
#include "hw/bl808/mmio.h"
#include "hw/bl808/gpio.h"
#include "hw/bl808/spi.h"
#include "hw/bl808/i2c.h"
#include "hw/st7789.h"
#include "hw/cst816.h"
#include "hw/bl.h"

#include "hal.h"
#include "debug.h"
#include "timer.h"
#include "gfx.h"


/* --- Delays and sleeping ------------------------------------------------- */


void msleep(unsigned ms)
{
	mdelay(ms);
}


/* --- Event loop ---------------------------------------------------------- */


static void process_touch(void)
{
	struct cst816_touch t;
	const struct cst816_event *e = t.event;
	static bool down = 0;

printf("TOUCH (%u)\n", down);
mdelay(1);
	cst816_read(&t);
mdelay(1);
if (t.events)
printf("\t%u %u %u\n", e->action, e->x, e->y);
else
printf("\tUP\n");
	if (t.events && e->action != cst816_up) {
		if (down)
			return;
		if (e->x >= GFX_WIDTH || e->y >= GFX_HEIGHT)
			return;
		touch_down_event(GFX_WIDTH - 1 - e->x, GFX_HEIGHT - 1 - e->y);
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
	unsigned uptime = 1;
	int last_touch = 0;
	bool button_down = 0;

	while (1) {
		bool on;

		if (button_down != !gpio_in(BUTTON_R)) {
			button_down = !button_down;
printf("BUTTON (%u)\n", button_down);
			button_event(button_down);
		}

		on = !cst816_poll();
		if (on != last_touch)
//		if (on && !last_touch)
//if (on)
			process_touch();
		last_touch = on;

		timer_tick(uptime);
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
#if 1
	st7789_update(da->fb, da->damage.x, da->damage.y,
	    da->damage.x + da->damage.w - 1, da->damage.y + da->damage.h - 1);
#else
	st7789_update(da->fb, 0, 0, GFX_WIDTH - 1, GFX_HEIGHT - 1);
#endif
}


/* --- Display on/off ------------------------------------------------------ */


void display_on(bool on)
{
	/* @@@ also switch the LCD */
	bl_on(on);
}


/* --- Command-line processing --------------------------------------------- */


static void usage(const char *name)
{
	fprintf(stderr, "usage: %s [demo-number]\n", name);
	exit(1);
}


int main(int argc, char **argv)
{
	int param = 0;
	char *end;
	int c;

	while ((c = getopt(argc, argv, "")) != EOF)
		switch (c) {
		default:
			usage(*argv);
		}
	switch (argc - optind) {
	default:
		case 0:
			break;
		case 1:
			param = strtoul(argv[optind], &end, 0);
			if (*end)
				usage(*argv);
			break;
		usage(*argv);
	}

	mmio_init();
	gpio_cfg_in(BUTTON_R, GPIO_PULL_UP);

	spi_init(LCD_MOSI, LCD_SCLK, LCD_CS, 15);
	i2c_init(0, I2C0_SDA, I2C0_SCL, 100);
	bl_init(LCD_BL);

	// @@@ no on-off control yet
	st7789_on();
	st7789_init(LCD_SPI, LCD_RST, LCD_DnC, GFX_WIDTH, GFX_HEIGHT, 0, 20);
	st7789_on();

	cst816_init(TOUCH_I2C, TOUCH_I2C_ADDR, TOUCH_INT);

	app_init(param);
	event_loop();

	return 0;
}
