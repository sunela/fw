/*
 * ui_entry.c - User interface: Text entry
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>

#include "hal.h"
#include "debug.h"
#include "timer.h"
#include "colors.h"
#include "gfx.h"
#include "text.h"
#include "shape.h"
#include "ui.h"
#include "wi_general_entry.h"
#include "ui_entry.h"


/* --- Keypad -------------------------------------------------------------- */


#define	BUTTON_LINGER_MS	150


struct ui_entry_ctx {
	/* from ui_entry_params */
	struct ui_entry_input input;
	const struct ui_entry_style *style;
	const struct ui_entry_ops *entry_ops;
	void *entry_user;
	struct wi_general_entry_ctx default_entry_ctx;
	const struct ui_entry_maps *maps;

	/* run-time variables */
	const char *second;
	struct timer t_button;
	struct {
		unsigned col;
		unsigned row;
	} down;
	uint16_t last_enabled;
};


/* --- Style --------------------------------------------------------------- */


const struct ui_entry_style ui_entry_default_style = {
	.input_fg		= INPUT_FG,
	.input_valid_bg		= INPUT_VALID_BG,
	.input_invalid_bg	= INPUT_INVALID_BG,
	.title_fg		= INPUT_TITLE_FG,
	.title_bg		= INPUT_TITLE_BG,
};


/* --- Keypad maps --------------------------------------------------------- */


const struct ui_entry_maps ui_entry_text_maps = {
	{
		"0 /!?",
		"1@\"+#",
		"2ABC",
		"3DEF",
		"4GHI",
		"5JKL",
		"6MNO",
		"7PQRS",
		"8TUV",
		"9WXYZ"
	},
	{
		"0&!?:;/ .,",
		"1'\"`+-*@#$",
		"2ABCabc(&)",
		"3DEFdef<=>",
		"4GHIghi[\\]",
		"5JKLjkl{|}",
		"6MNOmno^_~",
		"7PQRpqrSs",
		"8TUVtuv%",
		"9WXYwxyZz"
	}
};


const struct ui_entry_maps ui_entry_decimal_maps = {
	{
		"0",
		"1",
		"2",
		"3",
		"4",
		"5",
		"6",
		"7",
		"8",
		"9"
	},
	{
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
	}
};


/* --- Input validation ---------------------------------------------------- */


bool ui_entry_valid(struct ui_entry_input *in)
{
	return !*in->buf || !in->validate ||
	    in->validate(in->user, in->buf) > 0;
}


/* --- Input field --------------------------------------------------------- */


static void show_input(struct ui_entry_ctx *c)
{
	c->entry_ops->input(c->entry_user);
}


/* --- Draw buttons (first level) ------------------------------------------ */


static void draw_button(struct ui_entry_ctx *c, unsigned col, unsigned row,
    const char *label, bool enabled, bool up)
{
	c->entry_ops->button(c->entry_user, col, row, label, c->second, enabled,
	    up);
}


static void draw_first(struct ui_entry_ctx *c, bool enabled)
{
	unsigned row, col;

	for (col = 0; col != 3; col++)
		for (row = 0; row != 4; row++) {
			int n = c->entry_ops->n(c->entry_user, col, row);

			if (n >= 0)
				draw_button(c, col, row, c->maps->first[n],
				    enabled, 1);
		}
	c->last_enabled = enabled * ((1 << 11) - 1);
}


static bool draw_enabled_change(struct ui_entry_ctx *c, bool up)
{
	uint16_t set;
	unsigned row, col;

	if (!c->entry_ops->enabled_set)
		return 0;
	set = c->entry_ops->enabled_set(c->entry_user);
	for (col = 0; col != 3; col++)
		for (row = 0; row != 4; row++) {
			int n = c->entry_ops->n(c->entry_user, col, row);

			if (n < 0)
				continue;
			if (((c->last_enabled ^ set) >> n) & 1)
				draw_button(c, col, row,
				    c->maps->first[n],
				    (set >> n) & 1, up);
		}
	c->last_enabled = set;
	return 1;
}


/* --- Draw buttons (second level) ----------------------------------------- */


static void draw_second(struct ui_entry_ctx *c, const char *map)
{
	unsigned row, col;
	size_t len = strlen(map);

	c->entry_ops->clear_pad(c->entry_user);
		draw_button(c, 0, 0, NULL, 1, 1);
	for (col = 0; col != 3; col++)
		for (row = 0; row != 4; row++) {
			int n = c->entry_ops->n(c->entry_user, col, row);

			if (n < 0 || n >= (int) len)
				continue;

			char s[] = { map[n], 0 };

			draw_button(c, col, row, s, 1, 1);
		}
}


/* --- Event handling ------------------------------------------------------ */


static bool button_is_visible(struct ui_entry_ctx *c, int n)
{
	switch (n) {
	case UI_ENTRY_INVALID:
		return 0;
	case UI_ENTRY_LEFT:
		return 1;
	case UI_ENTRY_RIGHT:
		return !c->second;
	default:
		if (c->second)
			return (int) strlen(c->second) > n;
		return c->maps->first[n];
	}
}


static bool button_is_enabled(struct ui_entry_ctx *c, int n)
{
	struct ui_entry_input *in = &c->input;

	switch (n) {
	case UI_ENTRY_INVALID:
		return 0;
	case UI_ENTRY_LEFT:
		return 1;
	case UI_ENTRY_RIGHT:
		return *in->buf && ui_entry_valid(&c->input);
	default:
		if (strlen(in->buf) == in->max_len)
			return 0;
		if (c->entry_ops->enabled_set)
			return (c->last_enabled >> n) & 1;
		return 1;
	}
}


static void draw_button_by_pos(struct ui_entry_ctx *c, unsigned col,
    unsigned row, bool up)
{
	int n = c->entry_ops->n(c->entry_user, col, row);
	bool enabled = button_is_enabled(c, n);

	if (n < 0) {
		draw_button(c, col, row, NULL, enabled, up);
	} else if (c->second) {
		char tmp[] = { c->second[n], 0 };

		draw_button(c, col, row, tmp, enabled, up);
	} else {
		draw_button(c, col, row, c->maps->first[n], enabled, up);
	}
}


static void release_button(void *user)
{
	struct ui_entry_ctx *c = user;

//debug("release_button: col %u row %u\n", c->down.col, c->down.row);
	draw_button_by_pos(c, c->down.col, c->down.row, 1);
	ui_update_display();
}


static void ui_entry_down(void *ctx, unsigned x, unsigned y)
{
	struct ui_entry_ctx *c = ctx;
	unsigned col, row;
	int n;

//debug("ui_entry_down: %u %u\n", x, y);
	if (!c->entry_ops->pos(c->entry_user, x, y, &col, &row))
		return;
	n = c->entry_ops->n(c->entry_user, col, row);
	timer_flush(&c->t_button);
	if (!button_is_visible(c, n))
		return;
	if (!button_is_enabled(c, n))
		return;
	draw_button_by_pos(c, col, row, 0);
	ui_update_display();
	c->down.col = col;
	c->down.row = row;
	timer_set(&c->t_button, BUTTON_LINGER_MS, release_button, c);
}


static void ui_entry_tap(void *ctx, unsigned x, unsigned y)
{
	struct ui_entry_ctx *c = ctx;
	struct ui_entry_input *in = &c->input;
	unsigned col, row;
	int n;
	char *end = strchr(in->buf, 0);

	if (!c->entry_ops->pos(c->entry_user, x, y, &col, &row))
		return;
	n = c->entry_ops->n(c->entry_user, col, row);
	debug("entry_tap X %u Y %u -> col %u row %u (%d)\n", x, y, col, row, n);
	if (!button_is_visible(c, n))
		return;
	if (!button_is_enabled(c, n))
		return;

	if (n == UI_ENTRY_LEFT) { // cancel or delete
		if (c->second) {
			timer_cancel(&c->t_button);
			c->second = NULL;
			draw_first(c, 1);
			draw_button(c, 0, 0, NULL, 1, 1);
			draw_button(c, 2, 0, NULL, 1, 1);
			ui_update_display();
			return;
		}
		if (!*in->buf) {
			ui_return();
			return;
		}
		progress();

		end[-1] = 0;
		show_input(c);
		if (!*in->buf || !ui_entry_valid(&c->input))
			draw_button(c, 2, 0, NULL, 0, 0);
		if (!draw_enabled_change(c, 1) &&
		    end - in->buf == (int) in->max_len)
			draw_first(c, 1);
		ui_update_display();
		return;
	}
	if (n == UI_ENTRY_RIGHT) { // accept
		ui_return();
		return;
	}
	progress();
	if (c->second || !c->maps->second[n]) {
		char ch;
		int valid;

		if (c->entry_ops->key_char) {
			ch = c->entry_ops->key_char(c->entry_user, n);
		} else {
			if (c->second) {
				timer_cancel(&c->t_button);
				ch = c->second[n];
			} else {
				ch = *c->maps->first[n];
			}
		}
		end[0] = ch;
		end[1] = 0;
		c->second = NULL;
		valid = in->validate ? in->validate(in->user, in->buf) : 1;
		if (valid < 0) {
			*end = 0;
		} else {
			show_input(c);
			draw_button(c, 0, 0, NULL, 1, 1);
			draw_button(c, 2, 0, NULL, valid > 0, 1);
		}
		if (!draw_enabled_change(c, 1))
			draw_first(c, strlen(in->buf) != in->max_len);
	} else {
		timer_cancel(&c->t_button);
		c->second = c->maps->second[n];
		draw_second(c, c->second);
	}
	if (c->entry_ops->accept && c->entry_ops->accept(c->entry_user))
		ui_return();
	else
		ui_update_display();
}


static void ui_entry_to(void *ctx, unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe)
{
	struct ui_entry_ctx *c = ctx;
	struct ui_entry_input *in = &c->input;

	if (swipe == us_left) {
		*in->buf = 0;
		ui_return();
	}
}


static void ui_entry_cancel(void *ctx)
{
	struct ui_entry_ctx *c = ctx;

//debug("ui_entry_cancel\n");
	timer_flush(&c->t_button);
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_entry_open(void *ctx, void *params)
{
	struct ui_entry_ctx *c = ctx;
	const struct ui_entry_params *prm = params;

	assert(strlen(prm->input.buf) <= prm->input.max_len);
	c->input = prm->input;
	c->style = prm->style ? prm->style : &ui_entry_default_style;
	c->maps = prm->maps ? prm->maps : &ui_entry_text_maps;
	c->second = NULL;
	c->last_enabled = 0;

	if (prm->entry_ops) {
		c->entry_ops = prm->entry_ops;
		c->entry_user = prm->entry_user;
	} else {
		c->entry_ops = &wi_general_entry_ops;
		wi_general_entry_setup(&c->default_entry_ctx, 0);
		c->entry_user = &c->default_entry_ctx;
	}
	if (c->entry_ops->init)
		c->entry_ops->init(c->entry_user, &c->input, c->style);

	show_input(c);
	draw_button(c, 0, 0, NULL, 1, 1);
	draw_first(c, strlen(prm->input.buf) != prm->input.max_len);
	draw_button(c, 2, 0, NULL, 1, 1);
	timer_init(&c->t_button);
}


static void ui_entry_close(void *ctx)
{
	struct ui_entry_ctx *c = ctx;

	timer_cancel(&c->t_button);
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_entry_events = {
	.touch_down	= ui_entry_down,
	.touch_tap	= ui_entry_tap,
	.touch_to	= ui_entry_to,
	.touch_cancel	= ui_entry_cancel,
};


const struct ui ui_entry = {
	.name		= "entry",
	.ctx_size	= sizeof(struct ui_entry_ctx),
	.open		= ui_entry_open,
	.close		= ui_entry_close,
	.events		= &ui_entry_events,
};
