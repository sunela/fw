#include <stdarg.h>

#include "hal.h"
#include "debug.h"


/* vdebug() is provided by the HAL */


void debug(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vdebug(fmt, ap);
	va_end(ap);
}

