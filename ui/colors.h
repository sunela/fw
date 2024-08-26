/*
 * colors.h - Shared color definitions
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef COLORS_H
#define	COLORS_H

#include "gfx.h"


/* --- PIN and general text entry ------------------------------------------ */

#define	UP_BG			GFX_WHITE
#define	DOWN_BG			gfx_hex(0x808080)
#define	DISABLED_BG		gfx_hex(0x606060)
#define	SPECIAL_UP_BG		GFX_YELLOW
#define	SPECIAL_DOWN_BG		gfx_hex(0x808000)
#define	SPECIAL_DISABLED_BG	gfx_hex(0x404030)

/* --- Lists --------------------------------------------------------------- */

#define	LIST_FG			GFX_WHITE
#define	EVEN_BG			GFX_BLACK
#define	ODD_BG			GFX_HEX(0x202020)

#endif /* !COLORS_H */
