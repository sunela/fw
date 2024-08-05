/*
 * debug.c - Debugging output
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdarg.h>

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

