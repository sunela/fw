/*
 * base32.h - Base32 (RFC4648) encoder/decoder
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef	BASE32_H
#define	BASE32_H

#include <stddef.h>
#include <sys/types.h>


/*
 * base32_encode_size returns the size of the result buffer, including the
 * terminating NUL.
 */

static inline size_t base32_encode_size(size_t size)
{
	return (size + 4) / 5 * 8 + 1;
}


/*
 * base32_encode returns the length of the resulting string, without the
 * terminating NUL. It does store the NUL, though, and fails (returning -1) if
 * the result buffer is too small to store it.
 */

ssize_t base32_encode(char *res, size_t res_size, const void *data,
    size_t size);

ssize_t base32_decode(void *res, size_t res_size, const char *s);


static inline ssize_t base32_decode_size(const char *s)
{
	return base32_decode(NULL, 0, s);
}

#endif	/* !BASE32_H */
