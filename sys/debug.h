/*
 * debug.h - Debugging output
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */


#ifndef DEBUG_H
#define	DEBUG_H

#include <stdbool.h>


/*
 * "debugging" can be used by any code currently under development to activate
 * additional debugging operations. It has no direct effect on debug().
 */

extern bool debugging;


void debug(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));
void hexdump(const char *s, const void *data, unsigned size);

#endif /* !DEBUG_H */
