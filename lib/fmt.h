/*
 * fmt/fmt.h - Number formatting
 *
 * Written 2013-2015 by Werner Almesberger
 * Copyright 2013-2015 Werner Almesberger
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
#include <stdint.h>


void add_char(void *user, char c);

uint8_t print_number(char *s, unsigned long long v, uint8_t len, uint8_t base);

void vformat(void (*out)(void *user, char c), void *user,
    const char *fmt, va_list ap);
void format(void (*out)(void *user, char c), void *user, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

#endif /* !FMT_H */
