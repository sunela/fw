/*
 * ui.c - User interface: setup and event distribution
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "debug.h"
#include "alloc.h"
#include "hal.h"
#include "timer.h"
#include "gfx.h"
#include "wi_list.h"
#include "db.h"
#include "pin.h"
#include "demo.h"
#include "ui.h"


#define	CROSSHAIR	0

#define	UI_STACK_SIZE	5
#define	UI_TIMERS	3


struct gfx_drawable da;
struct db main_db;

unsigned pin_cooldown = 0;
unsigned pin_attempts = 0;

static PSRAM gfx_color fb[GFX_WIDTH * GFX_HEIGHT];


struct stack {
	const struct ui *ui;
	void *ctx;
};


static struct stack stack[UI_STACK_SIZE] = { { NULL, NULL }, };
static unsigned sp = 0;
static struct timer idle_timer;
static struct timer long_timer; /* for long touch screen press */
static unsigned idle_s;


/* --- Helper functions ---------------------------------------------------- */


static inline const struct ui *current_ui(void)
{
	return stack[sp].ui;
}


static inline const struct ui_events *current_events(void)
{
	return stack[sp].ui && stack[sp].ui->events ?
	    stack[sp].ui->events : NULL;
}


static inline void *current_ctx(void)
{
	return stack[sp].ctx;
}


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


bool is_on = 0;


static void idle_off(void *user)
{
	turn_off();
}


void progress(void)
{
	timer_set(&idle_timer, 1000 * idle_s, idle_off, NULL);
}


void set_idle(unsigned seconds)
{
	idle_s = seconds;
	progress();
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
	timer_cancel(&idle_timer);
	timer_cancel(&long_timer);
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


/* --- Swipe classifier ---------------------------------------------------- */


#define	SWIPE_R		(GFX_WIDTH / 3)	/* minimum swipe distance */
#define	SWIPE_F		2		/* minimum directional dominance */


static enum ui_swipe classify_swipe(unsigned ax, unsigned ay,
    unsigned bx, unsigned by)
{
	int dx = (int) bx - (int) ax;
	int dy = (int) by - (int) ay;

	if (dx * dx + dy * dy < SWIPE_R * SWIPE_R)
		return us_none;
	if (dx * dx > SWIPE_F * SWIPE_F * dy * dy)
		return dx < 0 ? us_left : us_right;
	if (dy * dy > SWIPE_F * SWIPE_F * dx * dx)
		return dy < 0 ? us_up : us_down;
	return us_none;
}


/* --- Touch screen / mouse ------------------------------------------------ */


#define	DRAG_R		10	/* minimum distance to indicate dragging */
#define	LONG_MS		400

static unsigned touch_start_ms = 0;
static unsigned touch_start_x, touch_start_y;
static unsigned touch_last_x, touch_last_y;
static bool touch_dragging = 0;
static bool touch_is_long = 0;


static void touch_long(void *user)
{
	const struct ui_events *e = current_events();

	if (e && e->touch_long)
		e->touch_long(current_ctx(), touch_start_x, touch_start_y);
	touch_is_long  = 1;
}


void touch_down_event(unsigned x, unsigned y)
{
	const struct ui_events *e = current_events();

//	debug("mouse down %u %u\n", x, y);
	if (x >= GFX_WIDTH || y >= GFX_HEIGHT)
		return;
	if (e && e->touch_down)
		e->touch_down(current_ctx(), x, y);
	touch_start_ms = now;
	touch_start_x = touch_last_x = x;
	touch_start_y = touch_last_y = y;
	touch_dragging = 0;
	touch_is_long = 0;
	timer_set(&long_timer, LONG_MS, touch_long, NULL);
	crosshair_show(x, y);
}


static void moving_event(unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y)
{
	const struct ui_events *e = current_events();
	enum ui_swipe swipe = classify_swipe(from_x, from_y, to_x, to_y);
	unsigned i;

	if (!e)
		return;
	for (i = 0; i != e->n_lists; i++)
		if (e->lists[i] && wi_list_moving(e->lists[i],
		    from_x, from_y, to_x, to_y, swipe))
			return;
	if (e->touch_moving)
		e->touch_moving(current_ctx(), from_x, from_y, to_x, to_y,
		    swipe);
}


static void to_event(unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y)
{
	const struct ui_events *e = current_events();
	enum ui_swipe swipe = classify_swipe(from_x, from_y, to_x, to_y);
	unsigned i;

	if (!e)
		return;
	for (i = 0; i != e->n_lists; i++)
		if (e->lists[i] && wi_list_to(e->lists[i],
		    from_x, from_y, to_x, to_y, swipe))
			return;
	if (e->touch_to)
		e->touch_to(current_ctx(), from_x, from_y, to_x, to_y, swipe);
}


static void cancel_event(void)
{
	const struct ui_events *e = current_events();
	unsigned i;

	if (!e)
		return;
	for (i = 0; i != e->n_lists; i++)
		if (e->lists[i])
			wi_list_cancel(e->lists[i]);
	if (e->touch_cancel)
		e->touch_cancel(current_ctx());
}


void touch_move_event(unsigned x, unsigned y)
{
	int dx = (int) x - (int) touch_start_x;
	int dy = (int) y - (int) touch_start_y;

//	debug("mouse move %u %u\n", x, y);
	if (x >= GFX_WIDTH || y >= GFX_HEIGHT)
		return;
	touch_last_x = x;
	touch_last_y = y;
	if (dx * dx + dy * dy >= DRAG_R * DRAG_R && !touch_dragging) {
		cancel_event();
		timer_cancel(&long_timer);
		touch_is_long = 0;
		touch_dragging = 1;
	}
	if (touch_dragging)
		moving_event(touch_start_x, touch_start_y, x, y);
	crosshair_show(x, y);
}


void touch_up_event(void)
{
	const struct ui_events *e = current_events();

//	debug("mouse up\n");
	timer_cancel(&long_timer);
	if (touch_is_long) {
		assert(!touch_dragging);
		touch_is_long = 0;
		crosshair_remove();
		return;
	}

	int dx = (int) touch_last_x - (int) touch_start_x;
	int dy = (int) touch_last_y - (int) touch_start_y;

	if (dx * dx + dy * dy >= DRAG_R * DRAG_R) {
		/*
		 * @@@ the way how timers are currently implemented, some time,
		 * including for timers, moves more slowly when there is
		 * interaction or other system activity. This means that we
		 * don't timeout easily when scrolling. However, this will
		 * change.
		 *
		 * For now, just the "progress" call here should be enough to
		 * prevent overly aggressive timeouts. The more general problem
		 * is to make sure accidental touches can't defer an idle
		 * timeout indefinitely, and we will need a better progress
		 * indicator.
		 */
		progress();
		to_event(touch_start_x, touch_start_y,
		    touch_last_x, touch_last_y);
	} else 	{
		if (touch_dragging) {
			cancel_event();
		} else {
			if (e && e->touch_tap)
				e->touch_tap(current_ctx(),
				    touch_start_x, touch_start_y);
		}
	}
	crosshair_remove();
}


/* --- Timer ticks --------------------------------------------------------- */


void tick_event(void)
{
	const struct ui_events *e = current_events();

	if (e && e->tick)
		e->tick(current_ctx());
	poll_demo_mbox();
}


/* --- UI page selection --------------------------------------------------- */


void ui_switch(const struct ui *ui, void *params)
{
	debug("ui_switch %u:%s(%p) -> %u:%s(%p)\n",
	    sp, current_ui() ? current_ui()->name : "", current_ui(),
	    sp, ui->name, ui);
	if (current_ui()) {
		if (current_ui()->close)
			current_ui()->close(current_ctx());
		memset(current_ctx(), 0, current_ui()->ctx_size);
	}
	free(current_ctx());
	gfx_clear(&da, GFX_BLACK);
	stack[sp].ui = ui;
	stack[sp].ctx = ui->ctx_size ? alloc_size(ui->ctx_size) : NULL;
	memset(current_ctx(), 0, ui->ctx_size);
	if (ui->open)
		ui->open(current_ctx(), params);
	update_display(&da);
}


void ui_call(const struct ui *ui, void *params)
{
	debug("ui_call %u:%s(%p) -> %u:%s(%p)\n",
	    sp, current_ui() ? current_ui()->name : "", current_ui(),
	    sp + 1, ui->name, ui);
	if (current_ui() && current_ui()->close)
		current_ui()->close(current_ctx());
	assert(sp < UI_STACK_SIZE);
	gfx_clear(&da, GFX_BLACK);
	sp++;
	stack[sp].ui = ui;
	stack[sp].ctx = ui->ctx_size ? alloc_size(ui->ctx_size) : NULL;
	memset(current_ctx(), 0, ui->ctx_size);
	if (ui->open)
		ui->open(current_ctx(), params);
	update_display(&da);
}


void ui_return(void)
{
	debug("ui_return %u:%s(%p) -> %d:%s(%p)\n",
	    sp, current_ui()->name, current_ui,
	    (int) sp - 1,
	    sp ? stack[sp - 1].ui ? stack[sp -1].ui->name : "" : "?",
	    sp ? stack[sp - 1].ui : NULL);
	assert(sp);
	if (current_ui()) {
		if (current_ui()->close)
			current_ui()->close(current_ctx());
		memset(current_ctx(), 0, current_ui()->ctx_size);
	}
	free(current_ctx());
	gfx_clear(&da, GFX_BLACK);
	sp--;
	assert(current_ui()->resume);
	current_ui()->resume(current_ctx());
	update_display(&da);
}


void ui_empty_stack(void)
{
	if (!sp)
		return;
	while (sp) {
		const struct ui *ui;

		debug("empty_stack %u:%s{%p) -> %d:%s(%p)\n",
		    sp, current_ui() ? current_ui()->name : "", current_ui(),
		    (int) sp - 1,
		    sp ? stack[sp - 1].ui ? stack[sp -1].ui->name : "" : "?",
		    sp ? stack[sp - 1].ui : NULL);
		ui = current_ui();
		if (ui->close)
			ui->close(current_ctx());
		free(current_ctx());
		sp--;
	}
	assert(current_ui()->resume);
	current_ui()->resume(current_ctx());
}


/* --- Initialization ------------------------------------------------------ */


bool app_init(char **args, unsigned n_args)
{
	gfx_da_init(&da, GFX_WIDTH, GFX_HEIGHT, fb);
	gfx_clear(&da, gfx_hex(0));

	timer_init(&idle_timer);
	timer_init(&long_timer);

	demo_init();

	if (n_args) {
		is_on = 1;
		idle_s = 600;
		demo(args, n_args);
	} else {
		ui_switch(&ui_off, NULL);
	}

	update_display(&da);
	return 1;
}
