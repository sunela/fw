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

//#define	LONG_MS		

#define	MAX_INPUT_LEN	32	/* for ui_entry */


struct ui_events {
	void (*touch_tap)(unsigned x, unsigned y);
	void (*touch_long)(unsigned x, unsigned y);
	void (*touch_from)(unsigned x, unsigned y);
	void (*touch_moving)(unsigned x, unsigned y);
	void (*touch_to)(unsigned x, unsigned y);
	void (*button_down)(void);
	void (*button_up)(void);
	void (*tick)(void);	/* called every ~10 ms */
};

struct ui {
	void (*open)(void);
	void (*close)(void);
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

void ui_switch(const struct ui *ui);

#endif /* !UI_H */
