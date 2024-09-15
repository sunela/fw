/*
 * secrets.c - Master secret and related secrets
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "debug.h"
#include "hal.h"
#include "sha.h"
#include "tweetnacl.h"
#include "pin.h"
#include "secrets.h"


#if 0
#define	debug(...)	do {} while (0)
#define	t0()		do {} while (0)
#define	t1(...)		do {} while (0)
#endif


uint8_t device_secret[MASTER_SECRET_BYTES];
uint8_t master_secret[MASTER_SECRET_BYTES];

static uint8_t master_pattern[MASTER_SECRET_BYTES];


/* --- Debug output -------------------------------------------------------- */


static void hex(const char *s, const uint8_t *data, unsigned size)
{
	unsigned i;

	debug("%s ", s);
	for (i = 0; i != size; i++)
		debug("%02x", data[i]);
	debug("\n");
}


/* --- Hash functions f(dev_sec, pin) -> the master secret and ID hash ----- */


/*
 * Variables and operations:
 *
 * pin	 	the PIN, as little-endian 32-bit integer, padded with 0xff
 * device_secret
 *		secret 32-byte string
 * a + b	concatenation of a and b
 * hash(x)	SHA256(x) where x is a byte string
 * n * p	multiplication of scalar n with curve element p
 *
 * Note that none of the operations are commutative:
 * H(a + b) != H(b + a)
 * n * p != p * n
 */

static void hash(uint8_t *out, ...)
{
	va_list ap;

	assert(SHA256_HASH_BYTES == MASTER_SECRET_BYTES);
	va_start(ap, out);
	sha256_begin();
	while (1) {
		const void *p = va_arg(ap, const void *);
		unsigned size;

		if (!p)
			break;
		size = va_arg(ap, unsigned);	
		hex("\tIN =", p, size);
		sha256_hash(p, size);
	}
	sha256_end(out);
	hex("HASH =", out, MASTER_SECRET_BYTES);
}


static void mult(void *out, const void *n, const void *p)
{
	assert(crypto_scalarmult_BYTES == MASTER_SECRET_BYTES);
	assert(crypto_scalarmult_SCALARBYTES == MASTER_SECRET_BYTES);
	assert(crypto_scalarmult_BYTES == MASTER_SECRET_BYTES);
	hex("\tN =", n, MASTER_SECRET_BYTES);
	hex("\tP =", p, MASTER_SECRET_BYTES);
	crypto_scalarmult(out, n, p);
	hex("N * P =", out, MASTER_SECRET_BYTES);
}


static void master_hash(void *out, uint32_t pin)
{
	uint8_t a[MASTER_SECRET_BYTES];
	uint8_t b[MASTER_SECRET_BYTES];
	uint8_t c[MASTER_SECRET_BYTES];

	// A = hash(pin)
	// B = hash(device_secret + A)
	// C = hash(A + device_secret)
	// A = B * C
	// master = hash(A)

	// Run-time cost:
	//  4 hash operations
	//  4 + 2 * 64 + 32 = 164 bytes hashed
	//  1 scalar product

	hash(a, &pin, sizeof(pin), NULL);
	hash(b, device_secret, MASTER_SECRET_BYTES, a, MASTER_SECRET_BYTES,
	    NULL);
	hash(c, a, MASTER_SECRET_BYTES, device_secret, MASTER_SECRET_BYTES,
	    NULL);
	mult(a, b, c);
	hash(out, a, MASTER_SECRET_BYTES, NULL);

	memset(a, 0, sizeof(a));
	memset(b, 0, sizeof(b));
	memset(c, 0, sizeof(c));
}


static void id_hash(void *out, uint32_t pin)
{
	uint8_t a[MASTER_SECRET_BYTES];
	uint8_t b[MASTER_SECRET_BYTES];
	uint8_t c[MASTER_SECRET_BYTES];
	uint8_t d[MASTER_SECRET_BYTES];

	// A = hash(pin)
	// B = hash(device_secret + pin)
	// C = A * B
	// D = B * A
	// A = hash(C + D)
	// id = hash(A + C)

	// Run-time cost:
	//  4 hash operations
	//  4 + 36 + 64 + 64 = 168 bytes hashed
	//  2 scalar products

	hash(a, &pin, sizeof(pin), NULL);
	hash(b, device_secret, MASTER_SECRET_BYTES, &pin, sizeof(pin), NULL);
	mult(c, a, b);
	mult(d, b, a);
	hash(a, c, MASTER_SECRET_BYTES, d, MASTER_SECRET_BYTES, NULL);
	hash(out, a, MASTER_SECRET_BYTES, c, MASTER_SECRET_BYTES, NULL);

	memset(a, 0, sizeof(a));
	memset(b, 0, sizeof(b));
	memset(c, 0, sizeof(c));
	memset(d, 0, sizeof(d));
}


/* --- Management of secrets --- */


bool secrets_change(uint32_t old_pin, uint32_t new_pin)
{
	pin_xor(master_pattern, old_pin);
	pin_xor(master_pattern, new_pin);
	return 1;
}


bool secrets_setup(uint32_t pin)
{
	memcpy(master_secret, master_pattern, MASTER_SECRET_BYTES);
	pin_xor(master_secret, pin);
	return 1;
}


bool secrets_new(uint32_t pin)
{
	/* @@@ obtain from protected persistent storage */
	memset(device_secret, 0, sizeof(device_secret));
	memcpy(master_pattern, device_secret, MASTER_SECRET_BYTES);
	pin_xor(master_pattern, pin);
	return 1;
}


static void try(void)
{
	uint8_t res[MASTER_SECRET_BYTES];
	uint32_t pin = 0xffff1234;

	t0();
	debug("--- Master ---\n");
	master_hash(&res, pin);
	t1("master\n");

	t0();
	debug("--- ID ---\n");
	id_hash(&res, pin);
	debug("--- End ---\n");
	t1("id\n");
}


bool secrets_init(void)
{
	try();

	/* @@@ obtain from protected persistent storage */
	memset(device_secret, 0, sizeof(device_secret));

	/* we don't have a master secret yet */
	memset(master_secret, 0, sizeof(master_secret));

	/* @@@ the master secret we want to obtain, for now, is all-zero */
	memset(master_pattern, 0, sizeof(master_pattern));
	pin_xor(master_pattern, DUMMY_PIN);
	return 1;
}
