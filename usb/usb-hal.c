/*
 * usb-hal.c - Hardware abstraction layer for USB
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sdk-hal.h"


bool usb_query(uint8_t req, uint8_t **data, uint32_t *len)
{
	static char hello[] = "hello";

	printf("query\r\n");
	*data = (uint8_t *) hello;
	*len = strlen(hello);
	return 1;
}


bool usb_arrival(uint8_t req, const void *data, uint32_t len)
{
	unsigned i;

	for (i = 0; i != len; i++)
		printf("%02x%s", ((const uint8_t *) data)[i],
			i < len - 1 ? " ": "\r\n");
	return 1;
}
