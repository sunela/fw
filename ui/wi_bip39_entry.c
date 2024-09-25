/*
 * wi_bip39_entry.c - User interface: general text entry
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <string.h>
#include <assert.h>

#include "debug.h"
#include "hal.h"
#include "fmt.h"
#include "bip39in.h"
#include "gfx.h"
#include "shape.h"
#include "text.h"
#include "ui.h"
#include "colors.h"
#include "ui_entry.h"
#include "wi_bip39_entry.h"


/* --- Input --------------------------------------------------------------- */

#define	INPUT_PAD_TOP		5
#define	INPUT_PAD_BOTTOM	2
#define	INPUT_MAX_X		210

#define	DEFAULT_INPUT_FONT	mono24
#define	DEFAULT_TITLE_FONT	mono24


/* --- Keypad -------------------------------------------------------------- */

/*
 * Generally approximate the golden ratio: 1:1.618 or 0.384:0.615, but increase
 * the font size since since we have only one small descender (for Q).
 */

#define	LABEL_FONT		mono18
#define	LABEL_OFFSET		0

#define	BUTTON_BOTTOM_OFFSET	8

#define	BUTTON_R		12
#define	BUTTON_W		70
#define	BUTTON_H		50
#define	BUTTON_X_GAP		8
#define	BUTTON_Y_GAP		5
#define	BUTTON_X_SPACING	(BUTTON_W + BUTTON_X_GAP)
#define	BUTTON_Y_SPACING	(BUTTON_H + BUTTON_Y_GAP)
#define	BUTTON_X0		(GFX_WIDTH / 2 - BUTTON_X_SPACING)
#define	BUTTON_Y1		(GFX_HEIGHT - BUTTON_H / 2 - \
				    BUTTON_BOTTOM_OFFSET)
#define	BUTTON_Y0		(BUTTON_Y1 - 3 * BUTTON_Y_SPACING)


/* --- Input --------------------------------------------------------------- */

/* @@@ we should just clear the bounding box */

static void clear_input(struct wi_bip39_entry_ctx *c)
{
	struct ui_entry_input *in = c->input;
	const struct ui_entry_style *style = c->style;

	gfx_rect_xy(&main_da, 0, 0, GFX_WIDTH,
	    INPUT_PAD_TOP + c->input_max_height + INPUT_PAD_BOTTOM,
	    ui_entry_valid(in) ? style->input_valid_bg :
            style->input_invalid_bg);
}


static void sort(char *buf, unsigned n)
{
	unsigned i, j;

	for (i = 0; i + 1 < n; i++)
		for (j = i + 1; j != n; j++)
			if (buf[i] > buf[j]) {
				char tmp = buf[i];

				buf[i] = buf[j];
				buf[j] = tmp;
			}
}


static void draw_top(struct wi_bip39_entry_ctx *c, const char *s,
    const struct font *font, gfx_color color, gfx_color bg)
{
//	struct ui_entry_input *in = c->input;
	struct text_query q;

	/*
	 * For the text width, we use the position of the next character
	 * (q.next) instread of the bounding box width (q.w), so that there is
	 * visual feedback when entering a space. (A trailing space doesn't
	 * grow the bounding box. Multiple spaces would, though, which is
	 * somewhat inconsistent.)
	 */
	text_query(0, 0, s, font, GFX_LEFT, GFX_TOP | GFX_MAX, &q);
//	assert(q.next <= (int) (in->max_len * c->input_max_height));

	gfx_rect_xy(&main_da, 0, INPUT_PAD_TOP, GFX_WIDTH, q.h, bg);
	gfx_clip_xy(&main_da, 0, INPUT_PAD_TOP, GFX_WIDTH, q.h);
	if ((GFX_WIDTH + q.next) / 2 < INPUT_MAX_X)
		text_text(&main_da, (GFX_WIDTH - q.next) / 2, INPUT_PAD_TOP,
		    s, font, GFX_LEFT, GFX_TOP | GFX_MAX, color);
	else
		text_text(&main_da, INPUT_MAX_X - q.next, INPUT_PAD_TOP,
		    s, font, GFX_LEFT, GFX_TOP | GFX_MAX, color);
	gfx_clip(&main_da, NULL);
}


static void input_pattern(char *buf, const char *s)
{
	char *p = buf;
	unsigned n, i, j;

	/*
	 * The longest set descriptors are "vwxyz" minus one of the middle
	 * letters, plus the square bracket ([]).
	 */
	n = bip39_match(s, NULL, 0);
	for (j = 0; j != strlen(s); j++) {
		char *r = p, *t;

		for (i = 0; i != n; i++) {
			char ch;

			ch = bip39_words[bip39_matches[i]][j];
			if (r == p || !memchr(p, ch, r - p))
				*r++ = ch;
		}
		sort(p, r - p);
*r = 0;
debug("s \"%s\" buf \"%s\" p \"%s\"\n", s, buf, p);
		switch (r - p) {
		case 0:
			ABORT();
			break;
		case 1:
			p = r;
			break;
		default:
			for (t = p + 1; t < r; t++)
				if (t[-1] + 1!= t[0])
					break;
			if (t == r) {
				p[1] = p[0];
				p[3] = r[-1];
				p[0] = '[';
				p[2] = '-';
				p[4] = ']';
				p += 5;
				break;
			}
			/* fall through */
		case 2:
		case 3:
			memmove(p + 1, p, r - p);
			p[0] = '[';
			r[1] = ']';
			p = r + 2;
			break;
		}
	}
	*p = 0;
}


static void draw_input(struct wi_bip39_entry_ctx *c)
{
	struct ui_entry_input *in = c->input;
	const struct ui_entry_style *style = c->style;
	char tmp[BIP39_MAX_SETS * (4 + 2) + 1];

	if (!*in->buf && in->title) {
		text_text(&main_da, GFX_WIDTH / 2,
		    (INPUT_PAD_TOP + c->input_max_height +
		    INPUT_PAD_BOTTOM) / 2,
		    in->title, style->title_font ? style->title_font :
		    &DEFAULT_TITLE_FONT,
		    GFX_CENTER, GFX_CENTER, style->title_fg);
		return;
	}

	input_pattern(tmp, in->buf);
	draw_top(c, tmp,
	    style->input_font ? style->input_font : &DEFAULT_INPUT_FONT,
	    style->input_fg,
	    ui_entry_valid(in) ? style->input_valid_bg :
	    style->input_invalid_bg);
}


static void wi_bip39_entry_input(void *user)
{
	struct wi_bip39_entry_ctx *c = user;

	clear_input(c);
	draw_input(c);
}


/* --- Pad layout ---------------------------------------------------------- */


static void base(unsigned x, unsigned y, gfx_color bg)
{
        gfx_rrect_xy(&main_da, x - BUTTON_W / 2, y - BUTTON_H / 2,
            BUTTON_W, BUTTON_H, BUTTON_R, bg);
}


static int wi_bip39_entry_n(void *user, unsigned col, unsigned row)
{
	if (row) {
		int n = col + (3 - row) * 3;

		return n  < 0 || n > 8 ? UI_ENTRY_INVALID : n;
	} else {
		switch (col) {
		case 0:
			return UI_ENTRY_LEFT;
		case 1:
			return 9;
		case 2:
			return UI_ENTRY_RIGHT;
		default:
			return UI_ENTRY_INVALID;
		}
	}
}


static void draw_label(unsigned x, unsigned y, const char *s)
{
	unsigned len = strlen(s);
	char tmp[3 + 1];

	if (len <= 3) {
		strcpy(tmp, s);
	} else {
		tmp[0] = *s;
		tmp[1] = '-';
		tmp[2] = s[len - 1];
		tmp[3] = 0;
	}
	text_text(&main_da, x, y + LABEL_OFFSET, tmp,
	    &LABEL_FONT, GFX_CENTER, GFX_CENTER, GFX_BLACK);
}


static uint16_t wi_bip39_enabled_set(void *user)
{
	struct wi_bip39_entry_ctx *c = user;
	struct ui_entry_input *in = c->input;
	uint16_t set = 0;
	const char *p;
	char next[10 + 1];

	/*
	 * We get length == 4 if we got here by going back from a previous
	 * input.
	 */
	if (strlen(in->buf) == BIP39_MAX_SETS)
		return 0;
	bip39_match(in->buf, next, sizeof(next));
	for (p = next; *p; p++)
		set |= 1 << (*p - '0');
	return set;
		
}


static void wi_bip39_entry_button(void *user, unsigned col, unsigned row,
    const char *label, bool second, bool enabled, bool up)
{
	struct wi_bip39_entry_ctx *c = user;
	struct ui_entry_input *in = c->input;
	unsigned x = BUTTON_X0 + BUTTON_X_SPACING * col;
	unsigned y = BUTTON_Y1 - BUTTON_Y_SPACING * row;
	gfx_color bg;

	assert(!second);
	if (row > 0 || col == 1) {
		char next[10 + 1];
		char ch;

		ch = wi_bip39_entry_n(c, col, row) + '0';
		bip39_match(in->buf, next, sizeof(next));
debug("buf \"%s\" next \"%s\" label \"%s\"\n", in->buf, next, label);
		enabled = strchr(next, ch);
		bg = enabled ? up ? UP_BG : DOWN_BG : DISABLED_BG;
		base(x, y, bg);
		draw_label(x, y, label);
		return;
	}
	bg = enabled ? up ? SPECIAL_UP_BG : SPECIAL_DOWN_BG :
	    SPECIAL_DISABLED_BG;
	if (col == 0) {	// X
		base(x, y, bg);
		if (c->cancel && !*in->buf)
			gfx_diagonal_cross(&main_da, x, y,
				    BUTTON_H * 0.4, 4, GFX_BLACK);
		else
			gfx_equilateral(&main_da, x, y, BUTTON_H * 0.7, -1,
			    GFX_BLACK);
	} else {	// >
		if (*in->buf) {
			base(x, y, bg);
			gfx_equilateral(&main_da, x, y, BUTTON_H * 0.7, 1,
			    GFX_BLACK);
		} else {
			base(x, y, GFX_BLACK);
		}
	}
}


static void wi_bip39_entry_clear_pad(void *user)
{
	const unsigned h = 4 * BUTTON_H + 2 * BUTTON_X_GAP;

	gfx_rect_xy(&main_da, 0, GFX_HEIGHT - h - BUTTON_BOTTOM_OFFSET,
            GFX_WIDTH, h, GFX_BLACK);
}


#include "debug.h"
static bool wi_bip39_entry_pos(void *user, unsigned x, unsigned y,
    unsigned *col, unsigned *row)
{
	*col = x < BUTTON_X0 + BUTTON_X_SPACING / 2 ? 0 :
	    x < BUTTON_X0 + 1.5 * BUTTON_X_SPACING ? 1 : 2;
	*row = 0;
	if (y < BUTTON_Y0 - BUTTON_Y_SPACING + BUTTON_R)
		return 0;
	if (y < BUTTON_Y0)
		*row = 3;
	else if (y > BUTTON_Y1)
		*row = 0;
	else
		*row = 3 - (y - BUTTON_Y0 + BUTTON_Y_SPACING / 2) /
		    BUTTON_Y_SPACING;
	return 1;
}


static char wi_bip39_key_char(void *user, unsigned n)
{
	assert(n < 10);
	return '0' + n;
}


static bool wi_bip39_accept(void *user)
{
	struct wi_bip39_entry_ctx *c = user;
	struct ui_entry_input *in = c->input;

	if (strlen(in->buf) == BIP39_MAX_SETS)
		return 1;
	return bip39_match(in->buf, NULL, 0) == 1;
}


/* --- API ----------------------------------------------------------------- */


static void wi_bip39_entry_init(void *ctx, struct ui_entry_input *input,
    const struct ui_entry_style *style)
{
	struct wi_bip39_entry_ctx *c = ctx;
	struct text_query q;

	c->input = input;
	c->style = style;
	text_query(0, 0, "",
            style->input_font ? style->input_font : &DEFAULT_INPUT_FONT,
            GFX_TOP | GFX_MAX, GFX_TOP | GFX_MAX, &q);
        c->input_max_height = q.h;
}


void wi_bip39_entry_setup(struct wi_bip39_entry_ctx *c, bool cancel,
    unsigned word)
{
	c->cancel = cancel;
	c->word = word;
}


struct ui_entry_ops wi_bip39_entry_ops = {
	.init		= wi_bip39_entry_init,
	.input		= wi_bip39_entry_input,
	.pos		= wi_bip39_entry_pos,
	.n		= wi_bip39_entry_n,
	.button		= wi_bip39_entry_button,
	.clear_pad	= wi_bip39_entry_clear_pad,
	.key_char	= wi_bip39_key_char,
	.enabled_set	= wi_bip39_enabled_set,
	.accept		= wi_bip39_accept,
};
