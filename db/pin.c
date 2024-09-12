/*
 * pin.c - PIN operations
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "rnd.h"
#include "sha.h"
#include "timer.h"
#include "secrets.h"
#include "pin.h"


#define	DUMMY_PIN	0xffff1234


uint8_t pin_shuffle[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

static uint32_t secret_pin = DUMMY_PIN;
static unsigned pin_cooldown; /* time when the PIN cooldown ends */
static unsigned pin_attempts; /* number of failed PIN entries */



uint32_t pin_encode(const char *s)
{
	uint32_t pin = 0xffffffff;

	while (*s) {
		pin = pin << 4 | (*s - '0');
		s++;
	}
	return pin;
}


void pin_success(void)
{
	pin_attempts = 0;
	pin_cooldown = 0;
}


void pin_fail(void)
{
	pin_attempts++;
	if (pin_attempts >= PIN_FREE_ATTEMPTS)
		pin_cooldown = now + PIN_WAIT_S(pin_attempts) * 1000;
}


bool pin_revalidate(uint32_t pin)
{
	/* @@@ */
	if (pin == secret_pin) {
		pin_success();
		return 1;
	} else {
		pin_fail();
		return 0;
	}
}


unsigned pin_cooldown_ms(void)
{
	if (pin_cooldown <= now)
		return 0;
	return pin_cooldown - now;
}


int pin_change(uint32_t old_pin, uint32_t new_pin)
{
	if (!pin_revalidate(old_pin))
		return 0;
	if (old_pin == new_pin)
		return 0;
	secret_pin = new_pin;
	return 1;
}


bool pin_set(uint32_t new_pin)
{
	secrets_init();
	if (!secrets_new(new_pin))
		return 0;
	secret_pin = new_pin;
	return 1;
}


void pin_xor(uint8_t secret[MASTER_SECRET_BYTES], uint32_t pin)
{
	/*
	 * @@@ SHA1(device_secret + PIN) is a poor KDF. Pick something better.
	 */
	uint8_t hash[MASTER_SECRET_BYTES];
	unsigned i;

	assert(SHA1_HASH_BYTES <= MASTER_SECRET_BYTES);
	memset(hash, 0, sizeof(hash));
	sha1_begin();
	sha1_hash(device_secret, sizeof(device_secret));
	sha1_hash((const void *) &pin, sizeof(pin));
	sha1_end(hash);

	for (i = 0; i != MASTER_SECRET_BYTES; i++)
		secret[i] ^= hash[i];
	memset(hash, 0, sizeof(hash));
}


static inline void swap(uint8_t *a, uint8_t *b)
{
	uint8_t tmp = *a;

	*a = *b;
	*b = tmp;
}


void pin_shuffle_pad(void)
{
	uint8_t i;

	/*
	 * @@@ use better algorithm
	 */
	for (i = 0; i != 10; i++)
		swap(pin_shuffle + i, pin_shuffle + rnd(10));
}
