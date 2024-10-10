/*
 * gpio.c - Driver for BL808 General-purpose IO
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdbool.h>

#include "gpio.h"


void gpio_cfg_off(unsigned pin)
{
	uint32_t cfg;

	cfg = GPIO_CFG(pin);
	cfg &= GPIO_DEL(FN) & GPIO_DEL(OE) & GPIO_DEL(IE) & GPIO_DEL(PULL);
	GPIO_CFG(pin) = cfg;
}


void gpio_cfg_in(unsigned pin, enum GPIO_PULL pull)
{
	uint32_t cfg;

	cfg = GPIO_CFG(pin);
	cfg &= GPIO_DEL(FN) & GPIO_DEL(OE) & GPIO_DEL(PULL);
	cfg |= GPIO_ADD(FN, GPIO_FN_GPIO) | GPIO_ADD(IE, 1) |
	    GPIO_ADD(PULL, pull);
	GPIO_CFG(pin) = cfg;
}


void gpio_cfg_out(unsigned pin, bool on, int drive)
{
	uint32_t cfg;

	cfg = GPIO_CFG(pin);
	cfg &= GPIO_DEL(MODE) & GPIO_DEL(FN) & GPIO_DEL(IE) & GPIO_DEL(O) &
	    GPIO_DEL(PULL) & GPIO_DEL(DRV);
	cfg |= GPIO_ADD(MODE, GPIO_MODE_SET) | GPIO_ADD(OE, 1) |
	    GPIO_ADD(O, on) | GPIO_ADD(FN, GPIO_FN_GPIO) |
	    GPIO_ADD(DRV, drive & 3);
	GPIO_CFG(pin) = cfg;
}


void gpio_cfg_fn(unsigned pin, enum GPIO_FN fn)
{
	uint32_t cfg;

	cfg = GPIO_CFG(pin);
	cfg &= GPIO_DEL(FN);
	cfg |= GPIO_ADD(FN, fn);
	GPIO_CFG(pin) = cfg;
}


void gpio_cfg_int(unsigned pin, enum GPIO_INT_MODE mode)
{
	uint32_t cfg;

	cfg = GPIO_CFG(pin);
	if (mode < 0) {
		cfg |= GPIO_ADD(INT_MASK, 1);
	} else {
		cfg &= GPIO_DEL(INT_MASK) & GPIO_DEL(INT_MODE);
		cfg |= GPIO_ADD(INT_MODE, mode);
	}
	GPIO_CFG(pin) = cfg;
}


void gpio_int_clr(unsigned pin)
{
	uint32_t cfg;

	cfg = GPIO_CFG(pin);
	cfg &= GPIO_DEL(INT_CLR);
	GPIO_CFG(pin) = cfg;
	cfg |= GPIO_ADD(INT_CLR, 1);
	GPIO_CFG(pin) = cfg;
	cfg &= GPIO_DEL(INT_CLR);
	GPIO_CFG(pin) = cfg;
}
