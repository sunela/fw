/*
 * hotp.h - HMAC-Based One-Time Password (HOTP)
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef	HOTP_H
#define	HOTP_H

#include <stdint.h>
#include <sys/types.h>


uint32_t hotp(const void *k, size_t k_size, const void *c, size_t c_size);

#endif	/* !HOTP_H */

