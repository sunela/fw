/*
 * ui_bip39.c - User interface: Input a set of bIP39 words
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "debug.h"
#include "fmt.h"
#include "text.h"
#include "colors.h"
#include "gfx.h"
#include "ui.h"
#include "bip39in.h"
#include "wi_bip39_entry.h"
#include "wi_list.h"
//#include "ui_notice.h"
#include "ui_entry.h"
#include "ui_bip39.h"


enum state {
	S_INPUT		= 0,
	S_CHOICES	= 1, /* showing remaining choices (bip39_2) */
};

struct ui_bip39_2_ctx {
	struct ui_bip39_ctx *ctx;
};

struct ui_bip39_ctx {
	enum state state;
	struct wi_list list;
	struct ui_bip39_params params;
	char buf[BIP39_MAX_SETS + 1];
	char title_buf[10 + 1];
	struct wi_bip39_entry_ctx bip39_entry_ctx;
};


static const struct wi_list_style list_style = {
	.y0	= 0,
	.y1	= GFX_HEIGHT - 1,
	.entry = {
		.fg	= { GFX_WHITE, GFX_WHITE },
		/*
		 * We normally use black-grey-... since most lists have a
		 * title that visually limits the first entry. In this case,
		 * there is no title and we may have just a single entry. Thus
		 * it is better to start with grey.
		 */
		.bg	= { GFX_HEX(0x202020), GFX_BLACK },
		.min_h	= 50,
	}
};

static void ui_bip39_2_tap(void *ctx, unsigned x, unsigned y);
static void ui_bip39_2_open(void *ctx, void *params);


static struct wi_list *lists[1];

static const struct ui_events ui_bip39_2_events = {
	.touch_tap	= ui_bip39_2_tap,
	.touch_to	= swipe_back,
	.lists		= lists,
	.n_lists	= 1,
};

static const struct ui ui_bip39_2 = {
	.name		= "bip39 2",
	.ctx_size	= sizeof(struct ui_bip39_ctx),
	.open		= ui_bip39_2_open,
	.events		= &ui_bip39_2_events,
};


/* --- Helper functions ---------------------------------------------------- */


static int validate(void *user, const char *s)
{
	unsigned len = strlen(s);
	unsigned n, i;

	if (len == BIP39_MAX_SETS)
		return 1;
	n = bip39_match(s, NULL, 0);
	for (i = 0; i != n; i++)
		if (strlen(bip39_words[bip39_matches[i]]) == len)
			return 1;
	return 0;
}


/* --- Event handling ------------------------------------------------------ */


static void ui_bip39_2_tap(void *ctx, unsigned x, unsigned y)
{
	struct ui_bip39_2_ctx *c2 = ctx;
	struct ui_bip39_ctx *c = c2->ctx;
	const struct wi_list_entry *entry;
	const uint16_t *choice;

	entry = wi_list_pick(&c->list, x, y);
	if (!entry)
		return;
	choice = wi_list_user(entry);
	if (!choice)
		return;
	c->params.result[*c->params.num_words] = *choice;
	ui_return();
}


/* --- Entry --------------------------------------------------------------- */


static void entry(struct ui_bip39_ctx *c, const char *prev)
{
	static struct ui_entry_style style;
	static struct ui_entry_maps bip39_maps = { { NULL, }, { NULL } };
	struct ui_entry_params params = {
		.input = {
			.title		= c->title_buf,
			.buf		= c->buf,
			.max_len	= BIP39_MAX_SETS,
			.validate	= validate,
			.user		= c,
		},
		.style = &style,
		.maps = &bip39_maps,
		.entry_ops = &wi_bip39_entry_ops,
		.entry_user = &c->bip39_entry_ctx,
	};
	char *p = c->title_buf;
	unsigned i;

	for (i = 0; i != 10; i++)
		bip39_maps.first[i] = bip39_sets[i];
	format(add_char, &p, "Word %u/%u",
	    *c->params.num_words + 1, c->params.max_words);
	assert(p < c->title_buf + sizeof(c->title_buf));

	c->state = S_INPUT;
	style = ui_entry_default_style;
	style.title_font = &mono18;
	style.input_invalid_bg = INPUT_NOTYET_BG;
	if (prev)
{
		bip39_word_to_sets(prev, c->buf, BIP39_MAX_SETS);
debug("prev \"%s\" -> \"%s\"\n", prev, c->buf);
}
	else
		memset(c->buf, 0, sizeof(c->buf));
	wi_bip39_entry_setup(&c->bip39_entry_ctx, !*c->params.num_words,
	    *c->params.num_words);
	progress();
	ui_call(&ui_entry, &params);
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_bip39_open(void *ctx, void *params)
{
	struct ui_bip39_ctx *c = ctx;
	struct ui_bip39_params *prm = params;

	memset(c, 0, sizeof(*c));
	c->params = *prm;
	*c->params.num_words = 0;
	entry(c, NULL);
}


static bool is_terminal(const struct ui_bip39_ctx *c, uint16_t word)
{
	unsigned in_len = strlen(c->buf);
	unsigned w_len = strlen(bip39_words[word]);

	if (w_len == in_len)
		return 1;
	if (in_len == BIP39_MAX_SETS && w_len > in_len)
		return 1;
	/*
	 * The caller also needs to keep any matches that return a single
	 * word.
	 */
	return 0;
}


static void ui_bip39_2_open(void *ctx, void *params)
{
	struct ui_bip39_2_ctx *c2 = ctx;
	struct ui_bip39_ctx *c = params;
	unsigned list_len = 0;
	unsigned n, i;
	unsigned h;

	c2->ctx = c;
	lists[0] = &c->list;

	n = bip39_match(c->buf, NULL, 0);
	wi_list_begin(&c->list, &list_style);
	for (i = 0; i != n; i++) {
		uint16_t *m = bip39_matches + i;

debug("\tadding %u: %s (%s)\n", i + 1, bip39_words[*m], c->buf);
		if (is_terminal(c, *m) || n == 1) {
			wi_list_add(&c->list, bip39_words[*m], NULL, m);
			list_len++;
		}
	}
	h = wi_list_end(&c->list);
	assert(list_len > 0 && list_len <= BIP39_MAX_FINAL_CHOICES);
	/*
	 * @@@ ugly: we draw the list just to obtain its height, then erase
	 * everything and draw it again. Could:
	 * 1) have a "dry run" mode in wi_list.c (but that would touch a lot of
	 *    code), or
	 * 2) just scroll the list in place
	 * Instead of gfx_clear, we could also just clear the area above the
	 * new y0, but that may be unreliable.
	 */
	if (h < GFX_HEIGHT) {
		gfx_clear(&main_da, GFX_BLACK);
		wi_list_y0(&c->list, (GFX_HEIGHT - h) / 2);
	}
}


static void ui_bip39_resume(void *ctx)
{
	struct ui_bip39_ctx *c = ctx;

/*
 * @@@ what if we want to go back to the previous word ?
 */
	if (!*c->buf) {
		if (*c->params.num_words) {
			const uint16_t word =
			    c->params.result[*c->params.num_words - 1];

			(*c->params.num_words)--;
			entry(c, bip39_words[word]);
		} else {
			ui_return();
		}
		return;
	}

	unsigned n;

	switch (c->state) {
	case S_INPUT:
		n = bip39_match(c->buf, NULL, 0);
debug("ui_bip39_resume: %u\n", n);
		c->state = S_CHOICES;
		ui_call(&ui_bip39_2, c);
		return;
//		c->params.result[*c->params.num_words] = bip39_matches[0];
		/* fall through */

/*
 * @@@ we need to provide at least a means to accept lists shorther than
 * max_words, e.g., if entering an existing master key from some other device.
 * Plan B: let user choose the length before entering words.
 *
 * @@@ we should show the final list and ask to cancel/edit/accept
 */
	case S_CHOICES:
		c->state = S_INPUT;
		(*c->params.num_words)++;
		if (*c->params.num_words == c->params.max_words)
			ui_return();
		else
			entry(c, NULL);
		return;
	default:
		ABORT();
	}
}


/* --- Interface ----------------------------------------------------------- */


const struct ui ui_bip39 = {
	.name		= "bip39",
	.ctx_size	= sizeof(struct ui_bip39_ctx),
	.open		= ui_bip39_open,
	.resume		= ui_bip39_resume,
};
