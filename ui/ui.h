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
#include "db.h"


#define	DEBOUNCE_MS	20

#define	IDLE_PIN_S	5	/* very short, in case of pocket activation */
#define	IDLE_HOLD_S	5	/* not much to look at. Also, pocket ... */
#define	IDLE_ACCOUNTS_S	15	/* keep it short */
#define	IDLE_ACCOUNT_S	60	/* give people enough time to interact */
#define	IDLE_OVERLAY_S	10	/* note: this is "back", not "off" */
#define	IDLE_SETUP_S	15	/* keep it short, in case we got there by
				   accident */
#define	IDLE_SET_TIME_S	30	/* long - the page is complex */
#define	IDLE_CONFIRM_S	10	/* keep it short, but not rushing */

#define	MAX_INPUT_LEN	32	/* for ui_entry */


/*
 * Possible touch sequences:
 * down-tap			small movements are suppressed
 * down-long			small movements are suppressed
 * down-long-cancel
 * down-long-cancel-moving-...-to
 * down-long-cancel-moving-...-cancel
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

struct wi_list;

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

	/* automatically handle list events */
	struct wi_list **lists;
	unsigned n_lists;
};

struct ui {
	void (*open)(void *params);
	void (*close)(void);
	void (*resume)(void);
	const struct ui_events *events;
};


extern struct gfx_drawable da;
extern struct db main_db;

extern unsigned pin_cooldown; /* time when the PIN cooldown ends */
extern unsigned pin_attempts; /* number of failed PIN entries */

extern struct mbox time_mbox;

/* User interface pages */

extern const struct ui ui_off;
extern const struct ui ui_pin;
extern const struct ui ui_fail;
extern const struct ui ui_cooldown;
extern const struct ui ui_accounts;
extern const struct ui ui_account;
extern const struct ui ui_field_add;
extern const struct ui ui_field_edit;
extern const struct ui ui_entry;
extern const struct ui ui_overlay;
extern const struct ui ui_confirm;
extern const struct ui ui_setup;
extern const struct ui ui_time;
extern const struct ui ui_storage;

void progress(void);
void set_idle(unsigned seconds);
void turn_off(void);

void show_citrine(void);

void ui_switch(const struct ui *ui, void *params);
void ui_call(const struct ui *ui, void *params);
void ui_return(void);
void ui_empty_stack(void);

#endif /* !UI_H */
