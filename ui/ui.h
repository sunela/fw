/*
 * ui.h - User interface: interfaces and screen definitions
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef UI_H
#define	UI_H

#include <stdbool.h>

#include "gfx.h"
#include "mbox.h"


#define	DEBOUNCE_MS	20

#define	IDLE_S		5	/* @@@ make dynamic */

#define	MAX_INPUT_LEN	32	/* for ui_entry */


/*
 * Possible touch sequences:
 * down-top			small movements are suppressed
 * down-long			small movements are suppressed
 * down-cancel			ambiguous gesture
 * down-(moving-...)-to
 * down-(moving-...)-cancel	movement ended near beginning
 */

enum ui_swipe {
	us_none	= 0,
	us_left,
	us_right,
	us_up,
	us_down
};

struct ui_events {
	void (*touch_down)(unsigned x, unsigned y);
	void (*touch_tap)(unsigned x, unsigned y);
	void (*touch_long)(unsigned x, unsigned y);
	void (*touch_moving)(unsigned from_x, unsigned from_y,
	    unsigned to_x, unsigned to_y);
	void (*touch_to)(unsigned from_x, unsigned from_y,
	    unsigned to_x, unsigned to_y, enum ui_swipe swipe);
	void (*touch_cancel)(void);
	void (*button_down)(void);
	void (*button_up)(void);
	void (*tick)(void);	/* called every ~10 ms */
};

struct ui {
	void (*open)(void *params);
	void (*close)(void);
	void (*resume)(void);
	const struct ui_events *events;
};


extern struct gfx_drawable da;

extern unsigned pin_cooldown; /* time when the PIN cooldown ends */
extern unsigned pin_attempts; /* number of failed PIN entries */

extern struct mbox time_mbox;

extern char ui_entry_input[MAX_INPUT_LEN + 1];
extern bool (*ui_entry_validate)(const char *s);

extern const struct ui ui_off;
extern const struct ui ui_pin;
extern const struct ui ui_fail;
extern const struct ui ui_cooldown;
extern const struct ui ui_accounts;
extern const struct ui ui_account;
extern const struct ui ui_entry;
extern const struct ui ui_time;

void progress(void);
void turn_off(void);

void show_citrine(void);

void ui_switch(const struct ui *ui, void *params);
void ui_call(const struct ui *ui, void *params);
void ui_return(void);
void ui_empty_stack(void);

#endif /* !UI_H */
