/*
 * demo.c - Demos/tests
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>

#include "hal.h"
#include "debug.h"
#include "util.h"
#include "mbox.h"
#include "gfx.h"
#include "shape.h"
#include "long_text.h"
#include "text.h"
#include "pin.h"
#include "sha.h"
#include "hmac.h"
#include "hotp.h"
#include "base32.h"
#include "secrets.h"
#include "dbcrypt.h"
#include "db.h"
#include "ui_overlay.h"
#include "ui_entry.h"
#include "ui.h"
#include "demo.h"

#ifndef SIM
#include "st7789.h"
#endif /* !SIM */


struct mbox demo_mbox;


/* Simple rectangles and colors. */

static bool demo_rgb(char *const *args, unsigned n_args)
{
	if (n_args)
		return 0;

	/* red, green, blue rectangle */
	gfx_rect_xy(&main_da, 50, 50, 150, 50, gfx_hex(0xff0000));
	gfx_rect_xy(&main_da, 50, 130, 150, 50, gfx_hex(0x00ff00));
	gfx_rect_xy(&main_da, 50, 210, 150, 50, gfx_hex(0x0000ff));

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
	gfx_copy(&main_da, (GFX_WIDTH - w) / 2, (GFX_HEIGHT - h) / 2,
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
	gfx_copy(&main_da, 0, 0, &da2, 0, 0, GFX_WIDTH, GFX_HEIGHT, GFX_BLACK);
	gfx_copy(&main_da, 10, 150, &da3, 0, 0, GFX_WIDTH - 30, 10, -1);
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

	gfx_poly(&main_da, 3, v, GFX_MAGENTA);
	gfx_poly(&main_da, 3, v2, GFX_YELLOW);

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
		text_char(&main_da, 100, 50 + 50 * i, fonts[i], '5', GFX_WHITE);

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

	long_text_setup(&lt, &main_da, 0, (GFX_HEIGHT - DEMO_LONG_HEIGHT) / 2,
	    GFX_WIDTH, DEMO_LONG_HEIGHT, "THIS IS A SCROLLING TEXT.",
	    &DEMO_LONG_FONT, GFX_WHITE, GFX_BLUE);
	while (i) {
		bool hold;

		t0();
		hold = long_text_scroll(&lt, &main_da, -DEMO_LONG_STEP);
		ui_update_display();
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

	gfx_rect_xy(&main_da, 0, 0, GFX_WIDTH, DEMO_VSCROLL_TOP, GFX_GREEN);
	gfx_rect_xy(&main_da, 0, DEMO_VSCROLL_TOP + DEMO_VSCROLL_MIDDLE,
	    GFX_WIDTH, DEMO_VSCROLL_BOTTOM, GFX_CYAN);
	for (x = 0; x != DEMO_VSCROLL_TOP; x++)
		if (x & 1)
			gfx_rect_xy(&main_da, x, x, DEMO_VSCROLL_MARK, 1,
			    GFX_BLACK);
	for (x = 0; x != DEMO_VSCROLL_MIDDLE; x++)
		if (x & 1)
			gfx_rect_xy(&main_da, x, DEMO_VSCROLL_TOP + x,
			    DEMO_VSCROLL_MARK, 1, GFX_WHITE);
	for (x = 0; x != DEMO_VSCROLL_BOTTOM; x++)
		if (x & 1)
			gfx_rect_xy(&main_da, x,
			    DEMO_VSCROLL_TOP + DEMO_VSCROLL_MIDDLE + x,
			    DEMO_VSCROLL_MARK, 1, GFX_BLACK);
	gfx_rect_xy(&main_da, 0, DEMO_VSCROLL_TOP + DEMO_VSCROLL_SCROLL,
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
		ABORT();
	}
#endif /* !SIM */

	return 1;
}


/* Text entry */

static int demo_entry_validate(void *user, const char *s)
{
	return !strchr(s, 'x');
}


static bool demo_entry(char *const *args, unsigned n_args)
{
	static char buf[MAX_INPUT_LEN + 1] = "";
	struct ui_entry_params params = {
		.input = {
			.buf		= buf,
			.max_len	= sizeof(buf) - 1,
			.validate	= demo_entry_validate,
		},
	};

	if (n_args)
		return 0;

	ui_switch(&ui_entry, &params);

	return 1;
}


/* SHA1, hardware accelerator or gcrypt */

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
		debug("%02x%c", res[i],
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
		debug("%02x%c", res[i],
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
		debug("bad counter \"%s\"\n", c);
		exit(1);
	}
	debug("%06lu\n", hotp64(k, strlen(k), count) % 1000000UL);

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
		debug("ERROR\n");
	else
		debug("%d/%u \"%s\"\n", (int) got, (unsigned) res_size, res);

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
		debug("ERROR\n");
	} else {
		debug("%d/%u", (int) got, (unsigned) res_size);
		for (i = 0; i != got; i++)
			debug(" %02x", res[i]);
		debug("\n");
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
		debug("bad counter \"%s\"\n", c);
		exit(1);
	}
	got = base32_decode(key, key_size, s);
	if (got < 0) {
		debug("ERROR\n");
	} else {
		ssize_t i;

		debug("%d/%u", (int) got, (unsigned) key_size);
		for (i = 0; i != got; i++)
			debug(" %02x", key[i]);
		debug("\n");
	}
	debug("%06lu\n", hotp64(key, got, count) % 1000000UL);

	return 1;
}


/* Account list */

static bool demo_acc(char *const *args, unsigned n_args)
{
	if (n_args)
		return 0;

	ui_switch(&ui_accounts, NULL);

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

	text_bbox(ox, oy, s, &DEMO_ALIGN_FONT, ax, ay, &bb);

	/*
	 * Draw text once for the next character position, which we want to
	 * draw below everything else, then draw it again later.
	 */
	next = text_text(&main_da, ox, oy, s, &DEMO_ALIGN_FONT,
	    ax, ay, DEMO_ALIGN_TEXT_FG);
	gfx_rect_xy(&main_da, next, bb.y - DEMO_ALIGN_NEXT_H,
	    DEMO_ALIGN_NEXT_W, bb.h + 2 * DEMO_ALIGN_NEXT_H,
	    DEMO_ALIGN_NEXT_FG);

	gfx_rect(&main_da, &bb, DEMO_ALIGN_BG);

	for (i = 0; i != DEMO_ALIGN_ORIGIN_R; i++) {
		gfx_rect_xy(&main_da, ox + i, oy + i, 1, 1,
		    DEMO_ALIGN_ORIGIN_FG);
		gfx_rect_xy(&main_da, ox + i, oy - i, 1, 1,
		    DEMO_ALIGN_ORIGIN_FG);
		gfx_rect_xy(&main_da, ox - i, oy + i, 1, 1,
		    DEMO_ALIGN_ORIGIN_FG);
		gfx_rect_xy(&main_da, ox - i, oy - i, 1, 1,
		    DEMO_ALIGN_ORIGIN_FG);
	}

	text_text(&main_da, ox, oy, s, &DEMO_ALIGN_FONT,
	    ax, ay, DEMO_ALIGN_TEXT_FG);

	return 1;
}


/* Set the time */

static bool demo_time(char *const *args, unsigned n_args)
{
	if (n_args)
		return 0;

	ui_switch(&ui_time, NULL);

	return 1;
}


/* Draw a filled arc */

static bool demo_arc(char *const *args, unsigned n_args)
{
	if (n_args != 2)
		return 0;

	gfx_arc(&main_da, GFX_WIDTH / 2, GFX_HEIGHT / 2, GFX_WIDTH * .4,
	    atoi(args[0]), atoi(args[1]), GFX_WHITE, GFX_BLUE);

	return 1;
}


/* Draw a power symbol */

static bool demo_powersym(char *const *args, unsigned n_args)
{
	unsigned r = 50;
	unsigned lw = 15;

	switch (n_args) {
	case 0:
		break;
	case 2:
		lw = atoi(args[1]);
		/* fall through */
	case 1:
		r = atoi(args[0]);
		break;
	default:
		return 0;
	}

	gfx_power_sym(&main_da, GFX_WIDTH / 2, GFX_HEIGHT / 2, r, lw,
	    GFX_WHITE, GFX_BLACK);
	return 1;
}


/* Draw a gear symbol */

static bool demo_gearsym(char *const *args, unsigned n_args)
{
	// Big gear (which looks much better): 60 30 36 20 16
	unsigned ro = 12;
	unsigned ri = 6;
	unsigned tb = 10;
	unsigned tt = 6;
	unsigned th = 4;
	
	switch (n_args) {
	case 0:
		break;
	case 5:
		ro = atoi(args[0]);
		ri = atoi(args[1]);
		tb = atoi(args[2]);
		tt = atoi(args[3]);
		th = atoi(args[4]);
		break;
	default:
		return 0;
	}

	gfx_gear_sym(&main_da, GFX_WIDTH / 2, GFX_HEIGHT / 2,
	    ro, ri, tb, tt, th,
	    GFX_WHITE, GFX_BLACK);
	return 1;
}


/* Show a button overlay */

static bool demo_overlay(char *const *args, unsigned n_args)
{
	static const struct ui_overlay_button buttons[] = {
		{ ui_overlay_sym_power,		NULL, NULL },
		{ ui_overlay_sym_delete,	NULL, NULL },
		{ ui_overlay_sym_add,		NULL, NULL },
		{ ui_overlay_sym_back,		NULL, NULL },
		{ ui_overlay_sym_next,		NULL, NULL },
		{ ui_overlay_sym_edit,		NULL, NULL },
		{ ui_overlay_sym_setup,		NULL, NULL },
		{ ui_overlay_sym_pc_comm,	NULL, NULL, },
		{ NULL }
	};
	struct ui_overlay_params prm = {
		.buttons	= buttons,
		.n_buttons	= 8,
	};

	switch (n_args) {
	case 0:
		break;
	case 1:
		prm.n_buttons = atoi(args[0]);
		break;
	default:
		return 0;
	}

	ui_switch(&ui_overlay, &prm);
	return 1;
}


static bool demo_overlay2(char *const *args, unsigned n_args)
{
	static const struct ui_overlay_button buttons[] = {
		{ ui_overlay_sym_move_from,	NULL, NULL },
		{ ui_overlay_sym_move_to,	NULL, NULL },
		{ ui_overlay_sym_move_cancel,	NULL, NULL },
		{ NULL, },
		{ NULL, },
		{ NULL, },
		{ NULL, },
		{ NULL, },
		{ NULL }
	};
	struct ui_overlay_params prm = {
		.buttons	= buttons,
		.n_buttons	= 3,
	};

	switch (n_args) {
	case 0:
		break;
	case 1:
		prm.n_buttons = atoi(args[0]);
		break;
	default:
		return 0;
	}

	ui_switch(&ui_overlay, &prm);
	return 1;
}


/* Draw a movement symbol */

static bool demo_movesym(char *const *args, unsigned n_args)
{
	// Big gear (which looks much better): 60 30 36 20 16
	unsigned bs = 50;
	unsigned br = 10;
	unsigned lw = 8;
	bool from = 1;
	int to = 0;
	
	switch (n_args) {
	case 0:
		break;
	case 5:
		bs = atoi(args[2]);
		br = atoi(args[3]);
		lw = atoi(args[4]);
		/* fall through */
	case 2:
		from = atoi(args[0]);
		to = atoi(args[1]);
		break;
	default:
		return 0;
	}

	gfx_move_sym(&main_da, GFX_WIDTH / 2, GFX_HEIGHT / 2,
	    bs, br, lw, from, to,
	    GFX_WHITE, GFX_BLACK);
	return 1;
}


/* Draw a checkbox */

static bool demo_checkbox(char *const *args, unsigned n_args)
{
	unsigned w = 20;
	unsigned lw = 2;
	bool on;

	switch (n_args) {
	case 1:
		break;
	case 3:
		lw = atoi(args[1]);
		/* fall through */
	case 2:
		w = atoi(args[0]);
		break;
	default:
		return 0;
	}
	on = strcmp(args[n_args - 1], "0");

	gfx_checkbox(&main_da, GFX_WIDTH / 2, GFX_HEIGHT / 2,
	    w, lw, on,
	    GFX_WHITE, GFX_BLACK);
	return 1;
}


/* Draw a PC communication symbol */

static bool demo_pccomm(char *const *args, unsigned n_args)
{
	unsigned side = 50;

	switch (n_args) {
	case 0:
		break;
	case 1:
		side = atoi(args[0]);
		break;
	default:
		return 0;
	}

	gfx_pc_comm_sym(&main_da, GFX_WIDTH / 2, GFX_HEIGHT / 2, side,
	    GFX_WHITE, GFX_BLACK);
	return 1;
}


/* Show a formatted text */

static bool demo_format(char *const *args, unsigned n_args)
{
	unsigned w = GFX_WIDTH;
	unsigned h = GFX_HEIGHT;
	unsigned offset = 0;
	unsigned x, y;
	int8_t align = GFX_LEFT;

	switch (n_args) {
	case 1:
		break;
	case 5:
		switch (*args[4]) {
		case 'l':
			align = GFX_LEFT;
			break;
		case 'c':
			align = GFX_CENTER;
			break;
		case 'r':
			align = GFX_RIGHT;
			break;
		default:
			return 0;
		}
		/* fall through */
	case 4:
		offset = atoi(args[3]);
		/* fall through */
	case 3:
		h = atoi(args[2]);
		/* fall through */
	case 2:
		w = atoi(args[1]);
		break;
	default:
		return 0;
	}

	x = (GFX_WIDTH - w) / 2;
	y = (GFX_HEIGHT - h) / 2;
	gfx_rect_xy(&main_da, x, y, w, h, GFX_RED);
	text_format(&main_da, x, y, w, h, offset, args[0], &mono18, align,
	    GFX_WHITE);
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
	{ NULL,		NULL,		NULL },		// 8
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
	{ "time",	demo_time,	"" },
	{ "arc",	demo_arc,	"from to" },
	{ "powersym",	demo_powersym,	"[r [lw]]"  },
	{ "gearsym",	demo_gearsym,	"[ro ri tb tt th]" },
	{ "overlay",	demo_overlay,	"[n]" },
	{ "movesym",	demo_movesym,	"[from to [bs br lw]]" },
	{ "overlay2",	demo_overlay2,	"[n]" },
	{ "checkbox",	demo_checkbox,	"[w [lw]] 0|1" },
	{ "pccomm",	demo_pccomm,	"[side]" },
	{ "format",	demo_format,	"string [w [h [offset [l|r|c]]]]" },
};


static void help(const struct demo *d)
{
	debug("%s%s%s\n", d->name, *d->help ? " " : "", d->help);
}


void demo(char **args, unsigned n_args)
{
	struct dbcrypt *c;
	unsigned n, i;
	char *end;

	secrets_init();
 	c = dbcrypt_init(master_secret, sizeof(master_secret));
	db_open(&main_db, c);
	display_on(1);
	n = strtoul(args[0], &end, 0);
	if (*end)
		n = 0;
	for (i = 0; i != ARRAY_ENTRIES(demos); i++) {
		const struct demo *d = demos + i;

		if (i + 1 == n || (d->name && !strcmp(d->name, args[0]))) {
			display_on(1);
			if (d->fn(args + 1, n_args - 1))
				return;
			help(d);
			exit(1);
		}
	}
	for (i = 0; i != ARRAY_ENTRIES(demos); i++)
		if (demos[i].name)
			help(demos + i);
	exit(1);
}


void poll_demo_mbox(void)
{
	char buf[256];
	ssize_t got;
	char *args[10];
	unsigned n_args = 0;
	char *p = buf;

	got = mbox_retrieve(&demo_mbox, buf, sizeof(buf));
	if (got < 0)
		return;
	if (!got) {
		debug("poll_demo_mbox: empty message\n");
		return;
	}
	if (buf[got - 1]) {
		debug("poll_demo_mbox: no NUL\n");
		return;
	}
	while (p != buf + got) {
		if (n_args == ARRAY_ENTRIES(args)) {
			debug("poll_demo_mbox: too many arguments\n");
			return;
		}
		args[n_args++] = p;
		p = strchr(p, 0) + 1;
	}
	assert(n_args);
	demo(args, n_args);
}


void demo_init(void)
{
	static char demo_buf[256];

	mbox_init(&demo_mbox, demo_buf, sizeof(demo_buf));
	mbox_enable(&demo_mbox);
}

