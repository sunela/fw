/*
 * ui.c - User interface: setup and event distribution
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdbool.h>
#include <assert.h>

#include "debug.h"
#include "hal.h"
#include "timer.h"
#include "gfx.h"
#include "pin.h"
#include "demo.h"
#include "ui.h"


#define	CROSSHAIR	0

#define	UI_TIMERS	3


struct gfx_drawable da;

unsigned pin_cooldown = 0;
unsigned pin_attempts = 0;

static gfx_color fb[GFX_WIDTH * GFX_HEIGHT];
static const struct ui *current_ui = NULL;
static struct timer idle_timer;


/* --- Crosshair ----------------------------------------------------------- */


#if CROSSHAIR
static bool crosshair_shown = 0;
static unsigned crosshair_x = 0;
static unsigned crosshair_y = 0;
static gfx_color backing_fb_x[GFX_WIDTH];
static gfx_color backing_fb_y[GFX_HEIGHT];
static struct gfx_drawable backing_da_x;
static struct gfx_drawable backing_da_y;
#endif /* CROSSHAIR */


static void crosshair_remove(void)
{
#if CROSSHAIR
	if (crosshair_shown) {
		gfx_copy(&da, 0, crosshair_y, &backing_da_x, 0, 0,
		    GFX_WIDTH, 1, -1);
		gfx_copy(&da, crosshair_x, 0, &backing_da_y, 0, 0,
		    1, GFX_HEIGHT, -1);
		update_display(&da);
		crosshair_shown = 0;
	}
#endif /* CROSSHAIR */
}


static void crosshair_show(unsigned x, unsigned y)
{
#if CROSSHAIR
	if (crosshair_shown)
		crosshair_remove();
	gfx_da_init(&backing_da_x, GFX_WIDTH, 1, backing_fb_x);
	gfx_da_init(&backing_da_y, 1, GFX_HEIGHT, backing_fb_y);
	gfx_copy(&backing_da_x, 0, 0, &da, 0, y, GFX_WIDTH, 1, -1);
	gfx_copy(&backing_da_y, 0, 0, &da, x, 0, 1, GFX_HEIGHT -1, -1);
	crosshair_x = x;
	crosshair_y = y;
	gfx_rect_xy(&da, x, 0, 1, GFX_HEIGHT - 1, GFX_RED);
	gfx_rect_xy(&da, 0, y, GFX_WIDTH - 1, 1, GFX_RED);
	update_display(&da);
	crosshair_shown = 1;
#endif /* CROSSHAIR */
}


/* --- On/off button ------------------------------------------------------- */


static bool is_on = 0;


static void idle_off(void *user)
{
	turn_off();
}


void progress(void)
{
	timer_set(&idle_timer, 1000 * IDLE_S, idle_off, NULL);
}


static void turn_on(void)
{
	if (is_on)
		return;
	is_on = 1;
	// @@@ hal_...
	pin_shuffle_pad();
	progress();
	if (now < pin_cooldown)
		ui_switch(&ui_cooldown, NULL);
	else
		ui_switch(&ui_pin, NULL);
}


void turn_off(void)
{
	if (!is_on)
		return;
	is_on = 0;
	// @@@ hal_...
	ui_empty_stack();
	ui_switch(&ui_off, NULL);
}


static void toggle_on_off(void)
{
	debug("toggle_on_off %u\n", is_on);
	if (is_on)
		turn_off();
	else
		turn_on();
}


void button_event(bool down)
{
	static unsigned debounce  = 0;

	debug("button %u (%u < %u)\n", down, debounce, (unsigned) now);
	if (now < debounce)
		return;
	if (!down) {
		debounce = 0;
		return;
	}
	if (debounce) {
		debounce = 0;
	} else {
		toggle_on_off();
		debounce = now + DEBOUNCE_MS;
	}
}


/* --- Touch screen / mouse ------------------------------------------------ */


//static unsigned touch_down_start = 0;


void touch_down_event(unsigned x, unsigned y)
{
	debug("mouse down %u %u\n", x, y);
	if (x >= GFX_WIDTH || y >= GFX_HEIGHT)
		return;
	if (current_ui && current_ui->events && current_ui->events->touch_tap)
		current_ui->events->touch_tap(x, y);
	crosshair_show(x, y);
}


void touch_move_event(unsigned x, unsigned y)
{
//	debug("mouse move %u %u\n", x, y);
	if (x >= GFX_WIDTH || y >= GFX_HEIGHT)
		return;
	crosshair_show(x, y);
}


void touch_up_event(void)
{
	debug("mouse up\n");
	crosshair_remove();
}


/* --- Timer ticks --------------------------------------------------------- */


void tick_event(void)
{
	if (current_ui && current_ui->events && current_ui->events->tick)
		current_ui->events->tick();
	poll_demo_mbox();
}


/* --- UI page selection --------------------------------------------------- */


#define	UI_STACK_SIZE	5


struct stack {
	const struct ui *ui;
};


static struct stack stack[UI_STACK_SIZE];
static unsigned sp = 0;


void ui_switch(const struct ui *ui, void *params)
{
	debug("ui_switch (%p -> %p)\n", current_ui, ui);
	if (current_ui && current_ui->close)
		current_ui->close();
	gfx_clear(&da, GFX_BLACK);
	current_ui = ui;
	if (ui->open)
		ui->open(params);
	update_display(&da);
}


void ui_call(const struct ui *ui, void *params)
{
	debug("ui_call(%p -> %p)\n", current_ui, ui);
	assert(sp != UI_STACK_SIZE);
	stack[sp].ui = current_ui;
	sp++;
	gfx_clear(&da, GFX_BLACK);
	current_ui = ui;
	if (ui->open)
		ui->open(params);
	update_display(&da);
}


void ui_return(void)
{
	assert(sp);
	if (current_ui && current_ui->close)
		current_ui->close();
	gfx_clear(&da, GFX_BLACK);
	sp--;
	current_ui = stack[sp].ui;
	assert(current_ui->resume);
	current_ui->resume();
	update_display(&da);
}


void ui_empty_stack(void)
{
	if (!sp)
		return;
	while (sp) {
		const struct ui *ui;

		sp--;
		ui = stack[sp].ui;
		if (ui->close)
			ui->close();
		current_ui = ui;
	}
	assert(current_ui->resume);
	current_ui->resume();
}


/* --- Initialization ------------------------------------------------------ */


bool app_init(char **args, unsigned n_args)
{
	gfx_da_init(&da, GFX_WIDTH, GFX_HEIGHT, fb);
	gfx_clear(&da, gfx_hex(0));

	timer_init(&idle_timer);
	demo_init();

	if (n_args)
		demo(args, n_args);
	else
		ui_switch(&ui_off, NULL);

	update_display(&da);
	return 1;
}
