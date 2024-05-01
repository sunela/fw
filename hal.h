/*
 * hal.h - Hardware abstraction layer
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef HAL_H
#define	HAL_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "gfx.h"


#define GFX_WIDTH	240
#define GFX_HEIGHT	280


void console_char(char c);

void vdebug(const char *fmt, va_list ap);
void t0(void);
double t1(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

void button_event(bool down);
void touch_down_event(unsigned x, unsigned y);
void touch_move_event(unsigned x, unsigned y);
void touch_up_event(void);
void tick_event(void);	/* call every ~10 ms */

void mdelay(unsigned ms);
void msleep(unsigned ms);

uint64_t time_us(void);

void update_display(struct gfx_drawable *da);
void display_on(bool on);

bool app_init(char *const *args, unsigned n_args);

#endif /* !HAL_H */
