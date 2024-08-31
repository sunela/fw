/*
 * rmt-db.c - Remote control interface: Database access
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "debug.h"
#include "db.h"
#include "settings.h"
#include "ui.h"	/* for main_db */
#include "rmt.h"
#include "rmt-db.h"


#if 0
#define DEBUG(...) debug(__VA_ARGS__)
#else
#define DEBUG(...) do {} while (0)
#endif


#define	MAX_REQ_LEN	256


enum rdb_state {
	RDS_IDLE,	/* before request */
	RDS_REQ,	/* in request */
	RDS_RES,	/* before or in response */
	RDS_END,
};


static enum rdb_state state = RDS_IDLE;
static enum rdb_op op;
static PSRAM_NOINIT uint8_t buf[MAX_REQ_LEN];
static uint64_t generation;
static const struct db_entry *de;
static const struct db_field *f;
static volatile bool async_reset = 0;


static bool rmt_db_reset(void)
{
	debug("rmt_db_reset: from state %u (%s)\n",
	    state, settings.strict_rmt ? "disabled" : "enabled");
	if (settings.strict_rmt)
		return 0;
	async_reset = 1;
	return 1;
}


void rmt_db_poll(void)
{
	int got;

	if (!rmt_poll(&rmt_usb))
		return;
	if (async_reset) {
		async_reset = 0;
		state = RDS_IDLE;
	}
	if (state)
		DEBUG("rmt_db_poll state %u op %u\n", state, op);
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
		generation = main_db.generation;
		DEBUG("\tRDS_IDLE: %u\n", op);
		switch (*buf) {
		case RDOP_LS:
			de = main_db.entries;
			break;
		case RDOP_SHOW:
			f = NULL;
			for (de = main_db.entries; de; de = de->next)
				if (strlen(de->name) == (size_t) got - 1 &&
				    !strncmp(de->name, (const char *) buf + 1,
				    got -1))
					break;
			if (!de) {
				op = RDOP_NOT_FOUND;
				break;
			}
			f = de->fields;
			break;
		default:
			op = RDOP_INVALID;
			return;
		}
		return;
	case RDS_REQ:
		DEBUG("\tRDS_REQ: %u\n", op);
		got = rmt_request(&rmt_usb, buf, MAX_REQ_LEN);
		if (!got)
			return;
		if (got > 0) {
			op = RDOP_INVALID;
			return;
		}
		state = RDS_RES;
		switch (op) {
		case RDOP_LS:
			if (!de) {
				state = RDS_END;
				return;
			}
			break;
		case RDOP_SHOW:
			if (!f) {
				state = RDS_END;
				return;
			}
			break;
		default:
			break;
		}
		/* fall through */
	case RDS_RES:
		DEBUG("\tRDS_RES: %u\n", op);
		switch (op) {
		case RDOP_LS:
			if (generation != main_db.generation) {
				if (!rmt_response(&rmt_usb,
				    "\000DB changed", 10))
					return;
				state = RDS_END;
			}
			DEBUG("\"%s\"\n", de->name);
			if (!rmt_response(&rmt_usb, de->name, strlen(de->name)))
				return;
			de = de->next;
			if (!de)
				state = RDS_END;
			break;
		case RDOP_SHOW:
			if (generation != main_db.generation) {
				if (!rmt_response(&rmt_usb,
				    "\000DB changed", 10))
					return;
				state = RDS_END;
			}
			while (f) {
				switch (f->type) {
				case ft_end:
				case ft_id:
				case ft_prev:
					break;
				case ft_user:
				case ft_email:
				case ft_hotp_counter:
				case ft_comment:
					if (!rmt_responsev(&rmt_usb,
					    &f->type, 1, f->data, f->len, NULL))
						return;
					break;
				case ft_pw:
				case ft_pw2:
				case ft_hotp_secret:
				case ft_totp_secret:
					if (!rmt_response(&rmt_usb,
					    &f->type, 1))
						return;
					break;
				default:
					abort();
				}
				f = f->next;
			}
			if (!f)
				state = RDS_END;
			break;
		case RDOP_INVALID:
			if (!rmt_response(&rmt_usb, "\000Bad request", 12))
				return;
			state = RDS_END;
			break;
		case RDOP_NOT_FOUND:
			if (!rmt_response(&rmt_usb, "\000Not found", 12))
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
	async_reset = 0;
	rmt_set_reset(&rmt_usb, rmt_db_reset);
}
