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
#include "storage.h"
#include "block.h"
#include "pin.h"
#include "secrets.h"


#if 0
#define	debug(...)	do {} while (0)
#define	t0()		do {} while (0)
#define	t1(...)		do {} while (0)
#endif


/*
 * device_secret is a secret stored in the device such that it cannot be
 * retrieved by an attacker and that it does not get leaked.
 *
 * master_pattern is a secret pattern obtained by hashing the device secret 
 * with the PIN.
 *
 * master_key is a secret key obtained by XOR-ing the master pattern with a
 * (non-secret) pad. This pad is recalculated each time the PIN - and thus the
 * master secred - changes, so that the master key remains the same.
 */


uint8_t device_secret[MASTER_SECRET_BYTES];
uint8_t master_secret[MASTER_SECRET_BYTES];

static uint8_t pad_id[MASTER_SECRET_BYTES];
static uint8_t master_pattern[MASTER_SECRET_BYTES];


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
		hexdump("\tIN =", p, size);
		sha256_hash(p, size);
	}
	sha256_end(out);
	hexdump("HASH =", out, MASTER_SECRET_BYTES);
}


static void mult(void *out, const void *n, const void *p)
{
	assert(crypto_scalarmult_BYTES == MASTER_SECRET_BYTES);
	assert(crypto_scalarmult_SCALARBYTES == MASTER_SECRET_BYTES);
	assert(crypto_scalarmult_BYTES == MASTER_SECRET_BYTES);
	hexdump("\tN =", n, MASTER_SECRET_BYTES);
	hexdump("\tP =", p, MASTER_SECRET_BYTES);
//	crypto_scalarmult(out, n, p);
	/*
	 * crypto_box_beforenm is hsalsa20(n * p)
	 * which is the same as bytes(Box(PrivateKey(n), PublicKey(p)))
	 * in PyNaCl. We use this instead of plain crypto_scalarmult() for
	 * easier compatibility with Python.
	 */
	crypto_box_beforenm(out, p, n);
	hexdump("N * P =", out, MASTER_SECRET_BYTES);
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


/* --- Adapt master key ---------------------------------------------------- */


static bool have_pad = 0;
static uint16_t pad_seq;
static int pad_block = -1;


static bool apply_pad(uint8_t *secret, int last_seq, uint16_t seq,
    const uint8_t *pads, unsigned size, const uint8_t *id)
    
{
	const uint8_t *end = pads + size;
	const uint8_t *p;

	if (last_seq >= 0 && ((seq + 0x10000 - last_seq) & 0xffff) >= 0x8000)
		return 0;
	for (p = pads; p + 2 * MASTER_SECRET_BYTES <= end;
	    p += 2 * MASTER_SECRET_BYTES) {
		unsigned i;

		if (memcmp(p, id, MASTER_SECRET_BYTES))
			continue;
		for (i = 0; i != MASTER_SECRET_BYTES; i++)
			secret[i] =
			    master_pattern[i] ^ p[MASTER_SECRET_BYTES + i];
hexdump("ID", p, MASTER_SECRET_BYTES);
hexdump("pad", p + MASTER_SECRET_BYTES, MASTER_SECRET_BYTES);
		return 1;
	}
	return 0;
}


/* --- Write a new pad ----------------------------------------------------- */


static bool change_pad(uint8_t *buf, unsigned size,
    const uint8_t *old_id, const uint8_t *new_id, const uint8_t *new_pad)
{
	const uint8_t *end = buf + size;
	uint8_t *p;
	uint8_t *erased = NULL;

	for (p = buf; p + 2 * MASTER_SECRET_BYTES <= end;
	    p += 2 * MASTER_SECRET_BYTES) {
		if (!memcmp(p, old_id, MASTER_SECRET_BYTES))
			goto found;
		if (!erased) {
			unsigned i;

			for (i = 0; i != 2 * MASTER_SECRET_BYTES; i++)
				if (p[i] != 0xff)
					goto next;
			erased = p;
		}
next: ;
	}
	if (!erased)
		return 0;
	p = erased;
found:
	memcpy(p, new_id, MASTER_SECRET_BYTES);
	memcpy(p + MASTER_SECRET_BYTES, new_pad, MASTER_SECRET_BYTES);
	return 1;
}


/* --- Management of secrets ----------------------------------------------- */


bool secrets_change(uint32_t old_pin, uint32_t new_pin)
{
	unsigned n;
	int best_seq = -1;
	unsigned new_block;
	uint16_t *seq = (void *) io_buf;

	assert(have_pad);
	assert(pad_block > -1);

	for (n = 0; n < PAD_BLOCKS; n += storage_erase_size())
		if (n != (unsigned) pad_block &&
		    storage_read_block(io_buf, pad_block)) {
			unsigned i;

			for (i = 0; i != STORAGE_BLOCK_SIZE; i++)
				if (io_buf[i] != 0xff)
					break;
			if (i == STORAGE_BLOCK_SIZE) {
				new_block = n;
				goto erased;
			}
			if (best_seq < 0 || *seq < best_seq) {
				best_seq = *seq;
				new_block = n;
			}
		}
	if (best_seq < 0) {
		debug("could not find block for pad");
		return 0;
	}
	if (!storage_erase_blocks(new_block, storage_erase_size())) {
		debug("could not erase new block %u\n", new_block);
		return 0;
	}
erased:
	if (!storage_read_block(io_buf, pad_block)) {
		debug("could not read pad block %u\n", pad_block);
		return 0;
	}

	uint8_t new_pad[MASTER_SECRET_BYTES];
	uint8_t old_id[MASTER_SECRET_BYTES];
	unsigned i;
	bool ok;

	master_hash(master_pattern, new_pin);
	id_hash(old_id, old_pin);
	id_hash(pad_id, new_pin);

	for (i = 0; i != MASTER_SECRET_BYTES; i++)
		new_pad[i] = master_pattern[i] ^ master_secret[i];

	memset(io_buf, 0xff, MASTER_SECRET_BYTES);
	*seq = pad_seq + 1;

	ok = change_pad(io_buf + MASTER_SECRET_BYTES,
	    STORAGE_BLOCK_SIZE - MASTER_SECRET_BYTES,
	    old_id, pad_id, new_pad);

	memset(pad_id, 0, sizeof(pad_id));
	memset(master_pattern, 0, sizeof(master_pattern));

	if (!ok) {
		debug("change_pad failed\n");
		return 0;
	}

	if (!storage_write_block(io_buf, new_block)) {
		debug("storage_write_block failed\n");
		return 0;
	}
	/*
	 * We need to erase the old block. Else, both the old and the new PIN
	 * will be valid to produce the same secret key.
	 */
	if (!storage_erase_blocks(pad_block, storage_erase_size())) {
		debug("could not erase old block %u\n", new_block);
		return 0;
	}

	pad_block = new_block;
	pad_seq++;

	debug("secrets_change: block %u, seq %u\n", pad_block, pad_seq);

	return 1;
}


bool secrets_setup(uint8_t *secret, int *block, uint32_t pin)
{
	bool found = 0;
	unsigned n;

	master_hash(master_pattern, pin);
	id_hash(pad_id, pin);

	for (n = 0; n < PAD_BLOCKS; n += storage_erase_size()) {
		/* block layout */

		const uint16_t *seq = (const void *) io_buf;
		const uint8_t *pads = io_buf + MASTER_SECRET_BYTES;
		unsigned pads_size = STORAGE_BLOCK_SIZE - MASTER_SECRET_BYTES;

		if (!storage_read_block(io_buf, n))
			continue;
		if (apply_pad(secret, found ? pad_seq : -1, *seq,
		    pads, pads_size, pad_id)) {
			pad_seq = *seq;
			if (block)
				*block = n;
			found = 1;
		}
	}
debug("PAD: found %u seq %u\n", found, pad_seq);
	memset(pad_id, 0, sizeof(pad_id));
	memset(master_pattern, 0, sizeof(master_pattern));
	
	return found;
}


bool secrets_setup_master(uint32_t pin)
{
	pad_block = -1;
	have_pad = secrets_setup(master_secret, &pad_block, pin);
	return have_pad;
}


bool secrets_new(uint32_t pin)
{
	/* @@@ obtain from protected persistent storage */
	memset(device_secret, 0, sizeof(device_secret));
	memcpy(master_pattern, device_secret, MASTER_SECRET_BYTES);
	pin_xor(master_pattern, pin);
	return 1;
}


void secrets_test_pad(void)
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
	/* @@@ obtain from protected persistent storage */
	memset(device_secret, 0, sizeof(device_secret));

	/* we don't have a master secret yet */
	memset(master_secret, 0, sizeof(master_secret));

	/* @@@ the master secret we want to obtain, for now, is all-zero */
	memset(master_pattern, 0, sizeof(master_pattern));
	pin_xor(master_pattern, DUMMY_PIN);

	have_pad = 0;

	return 1;
}
