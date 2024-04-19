/*
 * demo.c - Demos/tests
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "hal.h"
#include "gfx.h"
#include "long_text.h"
#include "ntext.h"
#include "pin.h"
#include "sha.h"
#include "hmac.h"
#include "hotp.h"
#include "base32.h"
#include "ui.h"
#include "demo.h"

#ifndef SIM
#include "st7789.h"
#endif /* !SIM */


bool use_ntext = 0;


/* Simple rectangles and colors. */

static void demo_1(void)
{
	/* red, green, blue rectangle */
	gfx_rect_xy(&da, 50, 50, 150, 50, gfx_hex(0xff0000));
	gfx_rect_xy(&da, 50, 130, 150, 50, gfx_hex(0x00ff00));
	gfx_rect_xy(&da, 50, 210, 150, 50, gfx_hex(0x0000ff));
}


/* Compositing */

void show_citrine(void)
{
#if 0
	static const uint16_t img[] = {
		#include "citrine.inc"
	};
	struct gfx_drawable da_img;
	unsigned w, h;

	w = img[0];
	h = img[1];

	gfx_da_init(&da_img, w, h, (uint16_t *) img + 2);
	gfx_copy(&da, (GFX_WIDTH - w) / 2, (GFX_HEIGHT - h) / 2,
	    &da_img, 0, 0, w, h, -1);
#endif
}


static void demo_2(void)
{
	gfx_color fb2[GFX_WIDTH * GFX_HEIGHT];
	struct gfx_drawable da2;

	gfx_da_init(&da2, GFX_WIDTH, GFX_HEIGHT, fb2);
	gfx_clear(&da2, GFX_BLACK);
	gfx_rect_xy(&da2, 100, 0, 50, GFX_HEIGHT, GFX_WHITE);

	gfx_color fb3[(GFX_WIDTH - 30) * 10];

	struct gfx_drawable da3;

	gfx_da_init(&da3, GFX_WIDTH - 30, 10, fb3);
	gfx_clear(&da3, GFX_CYAN);

	show_citrine();

#if 1
	gfx_copy(&da, 0, 0, &da2, 0, 0, GFX_WIDTH, GFX_HEIGHT, GFX_BLACK);
	gfx_copy(&da, 10, 150, &da3, 0, 0, GFX_WIDTH - 30, 10, -1);
#endif
}


/* Polygons */

static void demo_3(void)
{
	const short v[] = { 10, 10, 110, 10, 10, 210 };
	const short v2[] = { 100, 270, 230, 250, 150, 100 };

	gfx_poly(&da, 3, v, GFX_MAGENTA);
	gfx_poly(&da, 3, v2, GFX_YELLOW);
}


/* Text */

static void demo_4(bool new)
{
	const struct font *fonts[] = {
		&mono18,
		&mono24,
		&mono36,
	};
	unsigned i;

	for (i = 0; i != 3; i++)
		if (new)
			ntext_char(&da, 100, 50 + 50 * i, fonts[i], '5',
			    GFX_WHITE);
		else
			gfx_char(&da, 100, 50 + 50 * i, 16 << i, 16 << i, '@',
			    GFX_WHITE);
}


/* Horizontally scrolling text */


#define	DEMO_5_HEIGHT	32
#define	DEMO_5_HOLD_MS	500
#define	DEMO_5_DELAY_MS	10
#define	DEMO_5_STEP	2


static void demo_5(void)
{
	struct long_text lt;
	unsigned i = 10;

	long_text_setup(&lt, &da, 0, (GFX_HEIGHT - DEMO_5_HEIGHT) / 2,
	    GFX_WIDTH, DEMO_5_HEIGHT, "THIS IS A SCROLLING TEXT.",
	    DEMO_5_HEIGHT, GFX_WHITE, GFX_BLUE);
	while (i) {
		bool hold;

		t0();
		hold = long_text_scroll(&lt, &da, -DEMO_5_STEP);
		update_display(&da);
		t1("scroll & update");
		if (hold) {
			msleep(DEMO_5_HOLD_MS - DEMO_5_DELAY_MS);
			i--;
		}
		msleep(DEMO_5_DELAY_MS);
	}
}


/* Vertical scrolling (only on real hw) */


#define	DEMO_6_TOP	50
#define	DEMO_6_BOTTOM	80
#define	DEMO_6_MIDDLE	(GFX_HEIGHT - DEMO_6_TOP - DEMO_6_BOTTOM)
#define	DEMO_6_SCROLL	10
#define	DEMO_6_MARK	10


static void demo_6_pattern(void)
{
	unsigned x;

	gfx_rect_xy(&da, 0, 0, GFX_WIDTH, DEMO_6_TOP, GFX_GREEN);
	gfx_rect_xy(&da, 0, DEMO_6_TOP + DEMO_6_MIDDLE, GFX_WIDTH,
	    DEMO_6_BOTTOM, GFX_CYAN);
	for (x = 0; x != DEMO_6_TOP; x++)
		if (x & 1)
			gfx_rect_xy(&da, x, x, DEMO_6_MARK, 1, GFX_BLACK);
	for (x = 0; x != DEMO_6_MIDDLE; x++)
		if (x & 1)
			gfx_rect_xy(&da, x, DEMO_6_TOP + x, DEMO_6_MARK, 1,
			    GFX_WHITE);
	for (x = 0; x != DEMO_6_BOTTOM; x++)
		if (x & 1)
			gfx_rect_xy(&da, x, DEMO_6_TOP + DEMO_6_MIDDLE + x,
			    DEMO_6_MARK, 1, GFX_BLACK);
	gfx_rect_xy(&da, 0, DEMO_6_TOP + DEMO_6_SCROLL, GFX_WIDTH, 1,
	    GFX_RED);
}


static void demo_6(void)
{
	demo_6_pattern();
#ifndef SIM
	st7789_vscroll(DEMO_6_TOP, DEMO_6_TOP + DEMO_6_MIDDLE - 1,
	    DEMO_6_TOP + DEMO_6_SCROLL);
#endif /* !SIM */
}


static void demo_6a(unsigned y0, unsigned y1, unsigned ytop)
{
	demo_6_pattern();
#ifndef SIM
	st7789_vscroll(y0, y1, ytop);
#endif /* !SIM */
}


static void demo_6b(unsigned tfa, unsigned vsa, unsigned bfa, unsigned vsp)
{
	demo_6_pattern();
#ifndef SIM
	st7789_vscroll_raw(tfa, vsa, bfa, vsp);
#endif /* !SIM */
}


/* Text entry */


static bool demo_7_validate(const char *s)
{
	return !strchr(s, 'x');
}


/* SHA1,  hardware accelerator or gcrypt */


static void demo_9(const char *s)
{
	uint8_t res[SHA1_HASH_BYTES];
	unsigned i;

	sha1_begin();
	sha1_hash((const void *) s, strlen(s));
	sha1_end(res);

	for (i = 0; i != SHA1_HASH_BYTES; i++)
		printf("%02x%c", res[i],
		    i == SHA1_HASH_BYTES - 1 ? '\n' : ' ');
}


/* HMAC-SHA1 */


static void demo_10(const char *k, const char *c)
{
	uint8_t res[HMAC_SHA1_BYTES];
	unsigned i;

	hmac_sha1(res, k, strlen(k), c, strlen(c));
	for (i = 0; i != HMAC_SHA1_BYTES; i++)
		printf("%02x%c", res[i],
		    i == HMAC_SHA1_BYTES - 1 ? '\n' : ' ');
}


/* HOTP */


#define	HOTP_COUNT_BYTES	8


static void demo_11(const char *k, const char *c)
{
	char *end;
	unsigned long count;
	uint8_t c_bytes[HOTP_COUNT_BYTES];
	unsigned i;

	count = strtoul(c, &end, 10);
	if (*end) {
		fprintf(stderr, "bad counter \"%s\"\n", c);
		exit(1);
	}
	for (i = 0; i != HOTP_COUNT_BYTES; i++)
		c_bytes[i] = count >> 8 * (HOTP_COUNT_BYTES - i - 1);
	printf("%06u\n",
	    hotp(k, strlen(k), c_bytes, HOTP_COUNT_BYTES) % 1000000);
}


/* Base32 encoding */


static void demo_12(const char *s)
{
	size_t res_size = base32_encode_size(strlen(s));
	char res[res_size];
	ssize_t got;

	got = base32_encode(res, res_size, s, strlen(s));
	if (got < 0)
		printf("ERROR\n");
	else
		printf("%d/%u \"%s\"\n", (int) got, (unsigned) res_size, res);
}


/* Base32 decoding */


static void demo_13(const char *s)
{
	size_t res_size = base32_decode_size(s);
	uint8_t res[res_size];
	ssize_t got, i;

	got = base32_decode(res, res_size, s);
	if (got < 0) {
		printf("ERROR\n");
	} else {
		printf("%d/%u", (int) got, (unsigned) res_size);
		for (i = 0; i != got; i++)
			printf(" %02x", res[i]);
		printf("\n");
	}
}


/* HOTP with base32-encoded secret */


static void demo_14(const char *s, const char *c)
{
	size_t key_size = base32_decode_size(s);
	uint8_t key[key_size];
	ssize_t got, i;
	char *end;
	unsigned long count;
	uint8_t c_bytes[HOTP_COUNT_BYTES];

	count = strtoul(c, &end, 10);
	if (*end) {
		fprintf(stderr, "bad counter \"%s\"\n", c);
		exit(1);
	}
	got = base32_decode(key, key_size, s);
	if (got < 0) {
		printf("ERROR\n");
	} else {
		printf("%d/%u", (int) got, (unsigned) key_size);
		for (i = 0; i != got; i++)
			printf(" %02x", key[i]);
		printf("\n");
	}
	for (i = 0; i != HOTP_COUNT_BYTES; i++)
		c_bytes[i] = count >> 8 * (HOTP_COUNT_BYTES - i - 1);
	printf("%06u\n", hotp(key, got, c_bytes, HOTP_COUNT_BYTES) % 1000000);

}


/* --- Initialization ------------------------------------------------------ */


void demo(unsigned param, char *const *args, unsigned n_args)
{
	switch (param) {
	case 0:
		ui_switch(&ui_off);
		break;
	case 1:
		demo_1();
		break;
	case 2:
		demo_2();
		break;
	case 3:
		demo_3();
		break;
	case 4:
		demo_4(n_args);
		break;
	case 5:
		demo_5();
		break;
	case 6:
		switch (n_args) {
		case 0:
			demo_6();
			break;
		case 3:
			demo_6a(atoi(args[0]), atoi(args[1]), atoi(args[2]));
			break;
		case 4:
			demo_6b(atoi(args[0]), atoi(args[1]), atoi(args[2]),
			    atoi(args[3]));
			break;
		default:
			return;
		}
		break;
	case 7:
		ui_entry_validate = demo_7_validate;
		use_ntext = n_args;
		ui_switch(&ui_entry);
		break;
	case 8:
		use_ntext = 1;
		break;
	case 9:
		if (n_args)
			demo_9(args[0]);
		exit(1);
	case 10:
		if (n_args > 1)
			demo_10(args[0], args[1]);
		exit(1);
	case 11:
		if (n_args > 1)
			demo_11(args[0], args[1]);
		exit(1);
	case 12:
		if (n_args)
			demo_12(args[0]);
		exit(1);
	case 13:
		if (n_args)
			demo_13(args[0]);
		exit(1);
	case 14:
		if (n_args > 1)
			demo_14(args[0], args[1]);
		exit(1);
	default:
		break;
	}
}
