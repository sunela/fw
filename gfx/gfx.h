/*
 * gfx.h - Graphics library
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef GFX_H
#define	GFX_H

#include <stdbool.h>
#include <stdint.h>

//#include "st7789.h"


#define	GFX_ALIGN_MASK		3
#define		GFX_LEFT	0
#define		GFX_TOP		0
#define		GFX_CENTER	1
#define		GFX_RIGHT	2
#define		GFX_BOTTOM	2


typedef uint16_t gfx_color;


struct gfx_rect {
	unsigned x, y;
	unsigned w, h;
};

struct gfx_drawable {
	unsigned w, h;
	gfx_color *fb;
	bool changed;
	struct gfx_rect damage;
};


/*
 * GFX_RGB and GFX_HEX macros are for constant expressions in static
 * initializers.
 * Note that some of the arguments are evaluated more than once.
 */

#define	GFX_RGB(r, g, b) \
	(((r) & 0xf8) | ((g) & 0xe0) >> 5 | ((g) & 0x1c) << 11 | \
	    ((b) & 0xf8) << 5)

static inline gfx_color gfx_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	return (r & 0xf8) | (g & 0xe0) >> 5 | (g & 0x1c) << 11 |
	    (b & 0xf8) << 5;
//	return st7789_rgb(r, g, b);
}


/* Encoding: 0xrrggbb */

#define	GFX_HEX(hex) GFX_RGB((hex) >> 16, (hex) >> 8, (hex))

static inline gfx_color gfx_hex(uint32_t hex)
{
	return gfx_rgb(hex >> 16, hex >> 8, hex);
}


#define	GFX_BLACK	GFX_HEX(0x000000)
#define	GFX_WHITE	GFX_HEX(0xffffff)
#define	GFX_RED		GFX_HEX(0xff0000)
#define	GFX_GREEN	GFX_HEX(0x00ff00)
#define	GFX_BLUE	GFX_HEX(0x0000ff)
#define	GFX_YELLOW	GFX_HEX(0xffff00)
#define	GFX_MAGENTA	GFX_HEX(0xff00ff)
#define	GFX_CYAN	GFX_HEX(0x00ffff)


void gfx_rect(struct gfx_drawable *da, const struct gfx_rect *r,
    gfx_color color);
void gfx_rect_xy(struct gfx_drawable *da, unsigned x, unsigned y, unsigned w,
    unsigned h, gfx_color color);
void gfx_disc(struct gfx_drawable *da, unsigned x, unsigned y, unsigned r, 
    gfx_color color);
void gfx_clear(struct gfx_drawable *da, gfx_color bg);

void gfx_copy(struct gfx_drawable *to, unsigned xt, unsigned yt,
    const struct gfx_drawable *from, unsigned xf, unsigned yf,
    unsigned w, unsigned h, int transparent_color);

void gfx_poly(struct gfx_drawable *da, int points, const short *v,
    gfx_color color);

void gfx_hscroll(struct gfx_drawable *da, unsigned x, unsigned y, unsigned w,
    unsigned h, int dx);
void gfx_vscroll(struct gfx_drawable *da, unsigned x, unsigned y, unsigned w,
    unsigned h, int dy);

void gfx_reset(struct gfx_drawable *da);
void gfx_da_init(struct gfx_drawable *da, unsigned w, unsigned h,
    gfx_color *fb);

#endif /* !GFX_H */
