/*
 * rmt-db.c - Remote control interface: Database access
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "hal.h"
#include "debug.h"
#include "db.h"
#include "ui.h"	/* for main_db */
#include "rmt.h"
#include "rmt-db.h"


#define	MAX_REQ_LEN	1


enum rdb_state {
	RDS_IDLE,	/* before request */
	RDS_REQ,	/* in request */
	RDS_RES,	/* before or in response */
	RDS_END,
};



static enum rdb_state state = RDS_IDLE;
static enum rdb_op op;
static PSRAM uint8_t buf[MAX_REQ_LEN];
static uint64_t generation;
static const struct db_entry *de;


void rmt_db_poll(void)
{
	int got;

	if (!rmt_poll(&rmt_usb))
		return;
//debug("rmt_db_poll state %u op %u\n", state, op);
	switch (state) {
	case RDS_IDLE:
		got = rmt_request(&rmt_usb, buf, MAX_REQ_LEN);
		if (got < 0)
			return;
		state = RDS_REQ;
		if (!got) {
			op = RDOP_NULL;
			return;
		}
		op = *buf;
debug("\tRDS_IDLE: %u\n", op);
		switch (*buf) {
		case RDOP_LS:
			break;
		default:
			op = RDOP_INVALID;
			return;
		}
		return;
	case RDS_REQ:
debug("\tRDS_REQ: %u\n", op);
		got = rmt_request(&rmt_usb, buf, MAX_REQ_LEN);
		if (!got)
			return;
		state = RDS_RES;
		switch (op) {
		case RDOP_LS:
			generation = main_db.generation;
			de = main_db.entries;
			if (!de) {
				state = RDS_END;
				return;
			}
			break;
		default:
			break;
		}
		/* fall through */
	case RDS_RES:
debug("\tRDS_RES: %u\n", op);
		switch (op) {
		case RDOP_LS:
			if (generation != main_db.generation) {
				if (!rmt_response(&rmt_usb,
				    "\000DB changed", 10))
					return;
				state = RDS_END;
			}
debug("de %p de->name %p\n", de, de->name);
debug("\"%s\"\n", de->name);
			if (!rmt_response(&rmt_usb, de->name, strlen(de->name)))
				return;
			de = de->next;
			if (!de)
				state = RDS_END;
			break;
		case RDOP_INVALID:
			if (!rmt_response(&rmt_usb, "\000Bad request", 12))
				return;
			state = RDS_END;
			break;
		case RDOP_NULL:
			state = RDS_END;
			break;
		default:
			abort();
		}
		break;
	case RDS_END:
		if (!rmt_end(&rmt_usb))
			return;
		state = RDS_IDLE;
		break;	
	}
}


void rmt_db_init(void)
{
	rmt_open(&rmt_usb);
}
