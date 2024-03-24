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


/* --- Logging ------------------------------------------------------------- */


void vdebug(const char *fmt, va_list ap)
{
	static struct timeval t0;
	static bool first = 1;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	if (first) {
		t0 = tv;
		first = 0;
	}
	tv.tv_sec -= t0.tv_sec;
	tv.tv_usec -= t0.tv_usec;
	if (tv.tv_usec < 0) {
		tv.tv_sec--;
		tv.tv_usec += 1000 * 1000;
	}
	printf("[%3u.%03u] ",
	    (unsigned) tv.tv_sec, (unsigned) tv.tv_usec / 1000);
	vprintf(fmt, ap);
}
