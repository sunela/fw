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


#define	DEBOUNCE_MS	20
//#define	LONG_MS		

struct ui_events {
	void (*touch_tap)(unsigned x, unsigned y);
	void (*touch_long)(unsigned x, unsigned y);
	void (*touch_from)(unsigned x, unsigned y);
	void (*touch_moving)(unsigned x, unsigned y);
	void (*touch_to)(unsigned x, unsigned y);
	void (*button_down)(void);
	void (*button_up)(void);
};

struct ui {
	void (*open)(void);
	void (*close)(void);
	const struct ui_events *events;
};


extern struct gfx_drawable da;

extern const struct ui ui_off;
extern const struct ui ui_pin;


void show_citrine(void);

void ui_switch(const struct ui *ui);

#endif /* !UI_H */
