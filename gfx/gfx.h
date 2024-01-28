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


#define	GFX_LEFT	-1
#define	GFX_TOP		-1
#define	GFX_CENTER	0
#define	GFX_RIGHT	1
#define	GFX_BOTTOM	1


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


static inline gfx_color gfx_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	return (r & 0xf8) | (g & 0xe0) >> 5 | (g & 0x1c) << 12 |
	    (b & 0xf8) << 5;
//	return st7789_rgb(r, g, b);
}


/* Encoding: 0xrrggbb */

static inline gfx_color gfx_hex(uint32_t hex)
{
	return gfx_rgb(hex >> 16, hex >> 8, hex);
}


#define	GFX_BLACK	gfx_hex(0x000000)
#define	GFX_WHITE	gfx_hex(0xffffff)
#define	GFX_RED		gfx_hex(0xff0000)
#define	GFX_GREEN	gfx_hex(0x00ff00)
#define	GFX_BLUE	gfx_hex(0x0000ff)
#define	GFX_YELLOW	gfx_hex(0xffff00)
#define	GFX_MAGENTA	gfx_hex(0xff00ff)
#define	GFX_CYAN	gfx_hex(0x00ffff)


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

unsigned gfx_char_size(unsigned *res_w, unsigned *res_h,
    unsigned sizex, unsigned sizey, char ch);
unsigned gfx_char(struct gfx_drawable *da, int x1, int y1,
    int sizex, int sizey, char ch, gfx_color color);

void gfx_text_bbox(unsigned x, unsigned y, const char *s,
    unsigned scale, int8_t align_x, int8_t align_y, struct gfx_rect *bb);
void gfx_text(struct gfx_drawable *da, unsigned x, unsigned y, const char *s,
    unsigned scale, int8_t align_x, int8_t align_y, gfx_color color);

void gfx_reset(struct gfx_drawable *da);
void gfx_da_init(struct gfx_drawable *da, unsigned w, unsigned h,
    gfx_color *fb);

#endif /* !GFX_H */
