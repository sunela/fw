/*
 * shared.c - Hardware abstraction layer, shared code (for now)
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>

#include "hal.h"
#include "debug.h"


/* --- Subtract time ------------------------------------------------------- */


static void t_sub(struct timeval *a, const struct timeval *b)
{
	a->tv_sec -= b->tv_sec;
	a->tv_usec -= b->tv_usec;
	if (a->tv_usec < 0) {
		a->tv_sec--;
		a->tv_usec += 1000 * 1000;
	}
}


/* --- Logging ------------------------------------------------------------- */


void vdebug(const char *fmt, va_list ap)
{
	static struct timeval t_start;
	static bool first = 1;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	if (first) {
		t_start = tv;
		first = 0;
	}
	t_sub(&tv, &t_start);
	printf("[%3u.%03u] ",
	    (unsigned) tv.tv_sec, (unsigned) tv.tv_usec / 1000);
	vprintf(fmt, ap);
}


/* --- Delta time measurement ---------------------------------------------- */


static struct timeval t0_tv;


void t0(void)
{
	gettimeofday(&t0_tv, NULL);
}


void t1(const char *s)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	t_sub(&tv, &t0_tv);
	debug("%s: %3u.%06u s\n", s, tv.tv_sec, tv.tv_usec);
}
