/*
 * debug.c - Debugging output
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdarg.h>
#include <stdint.h>

#include "hal.h"
#include "debug.h"


bool debugging = 0;


/* vdebug() is provided by the HAL */


void debug(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vdebug(fmt, ap);
	va_end(ap);
}


void hexdump(const char *s, const void *data, unsigned size)
{
	const uint8_t *d = data;
	unsigned i;

	debug("%s ", s);
	for (i = 0; i != size; i++)
		debug("%02x", d[i]);
	debug("\n");
}
