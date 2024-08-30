/*
 * lib/fmt.h - Number formatting
 *
 * Written 2013-2015, 2024 by Werner Almesberger
 * Copyright 2013-2015, 2024 Werner Almesberger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This work is also licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */


#ifndef FMT_H
#define	FMT_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>


void add_char(void *user, char c);

/*
 * "user", received via format/vformat, must point to "unsigned".
 */

void count_char(void *user, char c);


/*
 * print_number returns the number of characters printed/output.
 */

uint8_t print_number(char *s, unsigned long long v, uint8_t len, uint8_t base);

/*
 * format and vformat return 1 if the last character printed/output was a
 * newline, 0 if not. Note that this information is not available with
 * vformat_alloc and format_alloc.
 */

bool vformat(void (*out)(void *user, char c), void *user,
    const char *fmt, va_list ap);
bool format(void (*out)(void *user, char c), void *user, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

char *vformat_alloc(const char *fmt, va_list ap);
char *format_alloc(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

#endif /* !FMT_H */
