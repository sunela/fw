/*
 * secrets.c - Master secret and related secrets
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "pin.h"
#include "secrets.h"


uint8_t device_secret[MASTER_SECRET_BYTES];
uint8_t master_secret[MASTER_SECRET_BYTES];

static uint8_t master_pattern[MASTER_SECRET_BYTES];


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


bool secrets_init(void)
{
	/* @@@ obtain from protected persistent storage */
	memset(device_secret, 0, sizeof(device_secret));
	memset(master_secret, 0, sizeof(master_secret));

	/* @@@ the master secret we want to obtain, for now, is all-zero */
	memset(master_pattern, 0, sizeof(master_pattern));
	pin_xor(master_pattern, DUMMY_PIN);
	return 1;
}
