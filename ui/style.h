/*
 * style.h - User interface: General style definitions
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef STYLE_H
#define	STYLE_H

/* --- Page top (title) area ----------------------------------------------- */


#define	FONT_TOP		mono18

#define	TOP_H			40	/* tall enough for easy touching */
#define	TOP_LINE_WIDTH		2	/* line under the title text */
#define	TOP_MARGIN		2	/* space between line and content */

/* If the rest of the page is a list, this is where we want it to start */

#define	LIST_Y0			(TOP_H + TOP_LINE_WIDTH + TOP_MARGIN)

#endif /* !STYLE_H */
