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
#include "text.h"
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


/* Simple rectangles and colors. */

static bool demo_rgb(char *const *args, unsigned n_args)
{
	if (n_args)
		return 0;

	/* red, green, blue rectangle */
	gfx_rect_xy(&da, 50, 50, 150, 50, gfx_hex(0xff0000));
	gfx_rect_xy(&da, 50, 130, 150, 50, gfx_hex(0x00ff00));
	gfx_rect_xy(&da, 50, 210, 150, 50, gfx_hex(0x0000ff));

	return 1;
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


static bool demo_composit(char *const *args, unsigned n_args)
{
	gfx_color fb2[GFX_WIDTH * GFX_HEIGHT];
	struct gfx_drawable da2;

	if (n_args)
		return 0;

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
	return 1;
}


/* Polygons */

static bool demo_poly(char *const *args, unsigned n_args)
{
	const short v[] = { 10, 10, 110, 10, 10, 210 };
	const short v2[] = { 100, 270, 230, 250, 150, 100 };

	if (n_args)
		return 0;

	gfx_poly(&da, 3, v, GFX_MAGENTA);
	gfx_poly(&da, 3, v2, GFX_YELLOW);

	return 1;
}


/* Text */

static bool demo_text(char *const *args, unsigned n_args)
{
	const struct font *fonts[] = {
		&mono18,
		&mono24,
		&mono36,
	};
	unsigned i;

	if (n_args)
		return 0;

	for (i = 0; i != 3; i++)
		text_char(&da, 100, 50 + 50 * i, fonts[i], '5', GFX_WHITE);

	return 1;
}


/* Horizontally scrolling text */

#define	DEMO_LONG_FONT	mono34
#define	DEMO_LONG_HEIGHT	32
#define	DEMO_LONG_HOLD_MS	500
#define	DEMO_LONG_DELAY_MS	10
#define	DEMO_LONG_STEP	2


static bool demo_long(char *const *args, unsigned n_args)
{
	struct long_text lt;
	unsigned i = 10;

	if (n_args)
		return 0;

	long_text_setup(&lt, &da, 0, (GFX_HEIGHT - DEMO_LONG_HEIGHT) / 2,
	    GFX_WIDTH, DEMO_LONG_HEIGHT, "THIS IS A SCROLLING TEXT.",
	    &DEMO_LONG_FONT, GFX_WHITE, GFX_BLUE);
	while (i) {
		bool hold;

		t0();
		hold = long_text_scroll(&lt, &da, -DEMO_LONG_STEP);
		update_display(&da);
		t1("scroll & update");
		if (hold) {
			msleep(DEMO_LONG_HOLD_MS - DEMO_LONG_DELAY_MS);
			i--;
		}
		msleep(DEMO_LONG_DELAY_MS);
	}

	return 1;
}


/* Vertical scrolling (only on real hw) */

#define	DEMO_VSCROLL_TOP	50
#define	DEMO_VSCROLL_BOTTOM	80
#define	DEMO_VSCROLL_MIDDLE	(GFX_HEIGHT - DEMO_VSCROLL_TOP - \
				DEMO_VSCROLL_BOTTOM)
#define	DEMO_VSCROLL_SCROLL	10
#define	DEMO_VSCROLL_MARK	10


static void demo_vscroll_pattern(void)
{
	unsigned x;

	gfx_rect_xy(&da, 0, 0, GFX_WIDTH, DEMO_VSCROLL_TOP, GFX_GREEN);
	gfx_rect_xy(&da, 0, DEMO_VSCROLL_TOP + DEMO_VSCROLL_MIDDLE, GFX_WIDTH,
	    DEMO_VSCROLL_BOTTOM, GFX_CYAN);
	for (x = 0; x != DEMO_VSCROLL_TOP; x++)
		if (x & 1)
			gfx_rect_xy(&da, x, x, DEMO_VSCROLL_MARK, 1, GFX_BLACK);
	for (x = 0; x != DEMO_VSCROLL_MIDDLE; x++)
		if (x & 1)
			gfx_rect_xy(&da, x, DEMO_VSCROLL_TOP + x,
			    DEMO_VSCROLL_MARK, 1, GFX_WHITE);
	for (x = 0; x != DEMO_VSCROLL_BOTTOM; x++)
		if (x & 1)
			gfx_rect_xy(&da, x,
			    DEMO_VSCROLL_TOP + DEMO_VSCROLL_MIDDLE + x,
			    DEMO_VSCROLL_MARK, 1, GFX_BLACK);
	gfx_rect_xy(&da, 0, DEMO_VSCROLL_TOP + DEMO_VSCROLL_SCROLL,
	    GFX_WIDTH, 1, GFX_RED);
}


static bool demo_vscroll(char *const *args, unsigned n_args)
{
	if (n_args && (n_args != 3 && n_args != 4))
		return 0;

	demo_vscroll_pattern();
#ifndef SIM
	switch (n_args) {
	case 0:
		st7789_vscroll(DEMO_VSCROLL_TOP,
		    DEMO_VSCROLL_TOP + DEMO_VSCROLL_MIDDLE - 1,
		    DEMO_VSCROLL_TOP + DEMO_VSCROLL_SCROLL);
		break;
	case 3:
		st7789_vscroll(atoi(args[0]), atoi(args[1]), atoi(args[2]));
		break;
	case 4:
		st7789_vscroll_raw(atoi(args[0]), atoi(args[1]), atoi(args[2]),
		    atoi(args[3]));
		break;
	default:
		abort();
	}
#endif /* !SIM */

	return 1;
}


/* Text entry */

static bool demo_entry_validate(const char *s)
{
	return !strchr(s, 'x');
}


static bool demo_entry(char *const *args, unsigned n_args)
{
	if (n_args)
		return 0;

	ui_entry_validate = demo_entry_validate;
	ui_switch(&ui_entry);

	return 1;
}


/* SHA1,  hardware accelerator or gcrypt */

static bool demo_sha1(char *const *args, unsigned n_args)
{
	if (n_args != 1)
		return 0;

	const char *s = args[0];
	uint8_t res[SHA1_HASH_BYTES];
	unsigned i;

	sha1_begin();
	sha1_hash((const void *) s, strlen(s));
	sha1_end(res);

	for (i = 0; i != SHA1_HASH_BYTES; i++)
		printf("%02x%c", res[i],
		    i == SHA1_HASH_BYTES - 1 ? '\n' : ' ');

	return 1;
}


/* HMAC-SHA1 */

static bool demo_hmac(char *const *args, unsigned n_args)
{
	if (n_args != 2)
		return 0;

	const char *k = args[0];
	const char *m = args[1];
	uint8_t res[HMAC_SHA1_BYTES];
	unsigned i;

	hmac_sha1(res, k, strlen(k), m, strlen(m));
	for (i = 0; i != HMAC_SHA1_BYTES; i++)
		printf("%02x%c", res[i],
		    i == HMAC_SHA1_BYTES - 1 ? '\n' : ' ');

	return 1;
}


/* HOTP */

static bool demo_hotp(char *const *args, unsigned n_args)
{
	if (n_args != 2)
		return 0;

	const char *k = args[0];
	const char *c = args[1];
	char *end;
	unsigned long count;

	count = strtoul(c, &end, 10);
	if (*end) {
		fprintf(stderr, "bad counter \"%s\"\n", c);
		exit(1);
	}
	printf("%06u\n", hotp64(k, strlen(k), count) % 1000000);

	return 1;
}


/* Base32 encoding */

static bool demo_b32enc(char *const *args, unsigned n_args)
{
	if (n_args != 1)
		return 0;

	const char *s = args[0];
	size_t res_size = base32_encode_size(strlen(s));
	char res[res_size];
	ssize_t got;

	got = base32_encode(res, res_size, s, strlen(s));
	if (got < 0)
		printf("ERROR\n");
	else
		printf("%d/%u \"%s\"\n", (int) got, (unsigned) res_size, res);

	return 1;
}


/* Base32 decoding */

static bool demo_b32dec(char *const *args, unsigned n_args)
{
	if (n_args != 1)
		return 0;

	const char *s = args[0];
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

	return 1;
}


/* HOTP with base32-encoded secret */

static bool demo_hotp2(char *const *args, unsigned n_args)
{

	if (n_args != 2)
		return 0;

	const char *s = args[0];
	const char *c = args[1];
	size_t key_size = base32_decode_size(s);
	uint8_t key[key_size];
	ssize_t got;
	char *end;
	unsigned long count;

	count = strtoul(c, &end, 10);
	if (*end) {
		fprintf(stderr, "bad counter \"%s\"\n", c);
		exit(1);
	}
	got = base32_decode(key, key_size, s);
	if (got < 0) {
		printf("ERROR\n");
	} else {
		ssize_t i;

		printf("%d/%u", (int) got, (unsigned) key_size);
		for (i = 0; i != got; i++)
			printf(" %02x", key[i]);
		printf("\n");
	}
	printf("%06u\n", hotp64(key, got, count) % 1000000);

	return 1;
}


/* Account list */

static bool demo_acc(char *const *args, unsigned n_args)
{
	if (n_args)
		return 0;

	ui_switch(&ui_accounts);

	return 1;
}


/* Text alignment */

#define	DEMO_ALIGN_FONT		mono34

#define	DEMO_ALIGN_BG		gfx_hex(0x404040)
#define	DEMO_ALIGN_TEXT_FG	GFX_WHITE
#define	DEMO_ALIGN_ORIGIN_FG	GFX_YELLOW
#define	DEMO_ALIGN_HEADROOM_FG	GFX_RED
#define	DEMO_ALIGN_NEXT_FG	GFX_MAGENTA

#define	DEMO_ALIGN_ORIGIN_R	7
#define	DEMO_ALIGN_HEADROOM_W	5
#define	DEMO_ALIGN_NEXT_W	2
#define	DEMO_ALIGN_NEXT_H	5


static bool demo_align(char *const *args, unsigned n_args)
{
	if (n_args != 3)
		return 0;

	const char *s = args[0];
	char *end;
	const unsigned ax = strtoul(args[1], &end, 0);

	if (*end)
		return 0;

	const unsigned ay = strtoul(args[2], &end, 0);

	if (*end)
		return 0;

	const unsigned ox = GFX_WIDTH / 2;
	const unsigned oy = GFX_HEIGHT / 2;
	struct gfx_rect bb;
	unsigned i, next;

	text_text_bbox(ox, oy, s, &DEMO_ALIGN_FONT, ax, ay, &bb);

	/*
	 * Draw text once for the next character position, which we want to
	 * draw below everything else, then draw it again later.
	 */
	next = text_text(&da, ox, oy, s, &DEMO_ALIGN_FONT,
	    ax, ay, DEMO_ALIGN_TEXT_FG);
	gfx_rect_xy(&da, next, bb.y - DEMO_ALIGN_NEXT_H,
	    DEMO_ALIGN_NEXT_W, bb.h + 2 * DEMO_ALIGN_NEXT_H,
	    DEMO_ALIGN_NEXT_FG);

	gfx_rect(&da, &bb, DEMO_ALIGN_BG);

	for (i = 0; i != DEMO_ALIGN_ORIGIN_R; i++) {
		gfx_rect_xy(&da, ox + i, oy + i, 1, 1, DEMO_ALIGN_ORIGIN_FG);
		gfx_rect_xy(&da, ox + i, oy - i, 1, 1, DEMO_ALIGN_ORIGIN_FG);
		gfx_rect_xy(&da, ox - i, oy + i, 1, 1, DEMO_ALIGN_ORIGIN_FG);
		gfx_rect_xy(&da, ox - i, oy - i, 1, 1, DEMO_ALIGN_ORIGIN_FG);
	}

	text_text(&da, ox, oy, s, &DEMO_ALIGN_FONT,
	    ax, ay, DEMO_ALIGN_TEXT_FG);

	return 1;
}


/* --- Initialization ------------------------------------------------------ */


static const struct demo {
	const char *name;	/* NULL if absent */
	bool (*fn)(char *const *args, unsigned n_args);
	const char *help;
} demos[] = {
	{ "rgb",	demo_rgb,	"" },		// 1
	{ "composit",	demo_composit,	"" },		// 2
	{ "poly",	demo_poly,	"" },		// 3
	{ "text",	demo_text,	"" },		// 4
	{ "long",	demo_long,	"" },		// 5
	{ "vscroll",	demo_vscroll,	"[y0 y1 ytop | tda vsa bfa vsp]" },
							// 6
	{ "entry",	demo_entry,	"" },		// 7
	{ NULL, 	NULL,		NULL },		// 8
		// was used to switch from vector fonts to ntext
	{ "sha1",	demo_sha1,	"string" },	// 9
	{ "hmac",	demo_hmac,	"key msg" },	// 10
	{ "hotp",	demo_hotp,	"key counter" },// 11
	{ "b32enc",	demo_b32enc,	"string" },	// 12
	{ "b32dec",	demo_b32dec,	"string" },	// 13
	{ "hotp2",	demo_hotp2,	"key-base32 counter" },
							// 14
	{ "acc",	demo_acc,	"" },		// 15
	{ "align",	demo_align,	"text x-align y-align" },
};


static void help(const struct demo *d)
{
	fprintf(stderr, "%s%s%s\n", d->name, *d->help ? " " : "", d->help);
}


void demo(char *const *args, unsigned n_args)
{
	unsigned n, i;
	char *end;

	n = strtoul(args[0], &end, 0);
	if (*end)
		n = 0;
	for (i = 0; i != sizeof(demos) / sizeof(*demos); i++) {
		const struct demo *d = demos + i;

		if (i + 1 == n || (d->name && !strcmp(d->name, args[0]))) {
			display_on(1);
			if (d->fn(args + 1, n_args - 1))
				return;
			help(d);
			exit(1);
		}
	}
	for (i = 0; i != sizeof(demos) / sizeof(*demos); i++)
		if (demos[i].name)
			help(demos + i);
	exit(1);
}
