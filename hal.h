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

#ifndef SDK_MAIN
#include "gfx.h"
#endif


#define GFX_WIDTH	240
#define GFX_HEIGHT	280

#ifdef SDK
#define	PSRAM	__attribute__((__section__(".psram_noinit")))
#else
#define	PSRAM
#endif

#define	CPU_ID_LENGTH	20


extern bool quiet;
extern int64_t time_offset;
extern int64_t time_override;


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

#ifndef SDK_MAIN
void update_display_partial(struct gfx_drawable *da, unsigned x, unsigned y);
void update_display(struct gfx_drawable *da);
void display_on(bool on);
#endif /* !SDK_MAIN */

void read_cpu_id(char *buf);	/* CPU_ID_LENGTH */

bool app_init(char **args, unsigned n_args);

#endif /* !HAL_H */
