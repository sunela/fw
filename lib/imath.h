/*
 * imath.h - Integer math functions
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef IMATH_H
#define	IMATH_H

#include <stdint.h>


uint32_t isqrt(uint32_t n);

/*
 * isin returns f * sin(a), for 0 <= a <= 45 degrees.
 */

unsigned isin(unsigned a, unsigned f);

#endif /* !IMATH_H */
