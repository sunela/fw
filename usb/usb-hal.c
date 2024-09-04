/*
 * usb-hal.c - Hardware abstraction layer for USB
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "debug.h"
#include "demo.h"
#include "ui.h"
#include "mbox.h"
#include "rmt.h"
#include "sdk-hal.h"

#include "../sdk/sdk-usb.h"


bool usb_query(uint8_t req, uint8_t **data, uint32_t *len)
{
	static char hello[] = "hello";

	debug("usb_query: req %u\r\n", req);
	switch (req) {
	case SUNELA_QUERY:
		*data = (uint8_t *) hello;
		*len = strlen(hello);
		return 1;
	case SUNELA_RMT:
		return rmt_query(&rmt_usb, data, len);
	default:
		return 0;
	}
}


/*
 * @@@ If we return false (0) if there is a problem, and ep0_handler thus
 * returns -1 to the USB stack, things seem to get messed up.
 */

bool usb_arrival(uint8_t req, const void *data, uint32_t len)
{
	unsigned i;

	for (i = 0; i != len; i++)
		debug("%02x%s", ((const uint8_t *) data)[i],
			i < len - 1 ? " ": "\r\n");
	switch (req) {
	case SUNELA_DEMO:
		mbox_deposit(&demo_mbox, data, len);
		break;
	case SUNELA_RMT:
		return rmt_arrival(&rmt_usb, data, len);
	default:
		break;
	}
	return 1;
}
