/*
 * shared.c - Hardware abstraction layer, shared code (for now)
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "hal.h"
#include "debug.h"


/* --- gettimeofday wrapper ------------------------------------------------ */


#ifndef SDK

uint64_t time_us(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return (uint64_t) tv.tv_sec * 1000000UL + tv.tv_usec;
}

#endif /* !SDK */


/* --- Logging ------------------------------------------------------------- */


void vdebug(const char *fmt, va_list ap)
{
	static uint64_t t_start;
	static bool first = 1;
	uint64_t t = time_us();

	if (first) {
		t_start = t;
		first = 0;
	}
	t -= t_start;
	printf("[%3llu.%03llu] ",
	    (unsigned long long) t / 1000000, (unsigned long long) t % 1000000);
	vprintf(fmt, ap);
}


/* --- Delta time measurement ---------------------------------------------- */


static uint64_t t_t0;
static uint64_t t_t1;
static bool t1_cached = 0;


void t0(void)
{
	t_t0 = time_us();
	t1_cached = 0;
}


double t1(const char *fmt, ...)
{
	if (!t1_cached)
		t_t1 = time_us() - t_t0;
	if (fmt) {
		char tmp[strlen(fmt) + 1];
		va_list ap;
		char *nl;

		strcpy(tmp, fmt);
		nl = strchr(tmp , '\n');
		if (nl)
			*nl = 0;
		va_start(ap, fmt);
		vdebug(tmp, ap);
		va_end(ap);
		printf(": %3llu.%06llu s%s",
		    (unsigned long long) t_t1 / 1000000,
		    (unsigned long long) t_t1 % 1000000,
		    nl ? "\n" : "");
		t1_cached = 0;
	} else {
		t1_cached = 1;
	}
	return t_t1 * 1e-6;
}
