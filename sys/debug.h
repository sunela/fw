/*
 * debug.h - Debugging output
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */


#ifndef DEBUG_H
#define	DEBUG_H

void debug(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

#endif /* !DEBUG_H */
