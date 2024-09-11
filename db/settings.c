/*
 * settings.c - Settings record
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */


#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "debug.h"
#include "db.h"
#include "ui.h"	/* @@@ for main_db; should move it elsewhere */
#include "settings.h"


enum settings_type {
	st_end		= 0,	/* end of settings */
	st_flags	= 1,
};

enum settings_flag {
	sf_crosshair	= 1 << 0,
	sf_strict_rmt	= 1 << 1,
};


struct settings settings = {
	.crosshair	= 0,
	.strict_rmt	= 0,
};


static bool have_settings = 0;
static uint16_t settings_seq;


bool settings_update(void)
{
	uint8_t *p = payload_buf;
	const uint8_t *end = payload_buf + sizeof(payload_buf);
	uint8_t flags = 0;

	memset(payload_buf, 0, sizeof(payload_buf));
	if (settings.crosshair)
		flags |= sf_crosshair;
	if (settings.strict_rmt)
		flags |= sf_strict_rmt;
	if (flags) {
		if (end - p < 3)
			return 0;
		*p++ = st_flags;
		*p++ = 1;
		*p++ = flags;
	}
	settings_seq++;
	return db_update_settings(&main_db, settings_seq,
	    payload_buf, p - payload_buf);
}


static void clear_settings(struct settings *s)
{
	memset(s, 0, sizeof(*s));
}


bool settings_process(uint16_t seq, const void *payload, unsigned size)
{
	if (have_settings &&
	    ((seq + 0x10000 - settings_seq) & 0xffff) >= 0x8000)
		return 1;

	const uint8_t *end = payload + size;
	const uint8_t *p = payload;
	struct settings new_settings;

	clear_settings(&new_settings);
	while (p < end && *p) {
		enum settings_type type;
		uint8_t len;
		const uint8_t *v;

		if (p > end - 2)
			goto invalid;
		type = p[0];
		len = p[1];
		v = p + 2;
		p = v + len;
		if (p > end)
			goto invalid;
		switch (type) {
		case st_flags:
			new_settings.crosshair = *v & sf_crosshair;
			new_settings.strict_rmt = *v & sf_strict_rmt;
			break;
		default:
			goto invalid;
		}
	}

	memcpy(&settings, &new_settings, sizeof(settings));
	have_settings = 1;
	settings_seq = seq;
	return 1;

invalid:
	debug("invalid settings\n");
	return 0;
}


void settings_reset(void)
{
	have_settings = 0;
	clear_settings(&settings);
}
