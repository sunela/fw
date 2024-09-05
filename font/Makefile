#
# fonts/Makefile - Makefile for selecting and converting a TTF font
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

#
# See https://fontconfig.pages.freedesktop.org/fontconfig/fontconfig-user.html
# for how font patterns are formed:
#
# fc-list -b is also useful for showing font properties.
#

FONT_PATTERN = Liberation Mono:bold:slant=0
FONT_FILE = $(shell fc-list '$(FONT_PATTERN)' file | grep /liberation2/ | \
    sed 's/:.*//')

FONTS = mono14.font mono18.font mono24.font mono34.font mono36.font mono58.font

CFLAGS = -Wall -Wextra -g
OBJS = font.o

include ../Makefile.c-common

.PHONY:	all clean spotless

all::	show

show:	show.c $(FONTS) $(OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(OBJS)

mono14.font: cvtfont.py Makefile PATCHES
	$(BUILD) ./cvtfont.py $(FONT_FILE) 14 mono14 32-126 PATCHES >$@ || \
	    { rm -f $@; exit 1; }

mono18.font: cvtfont.py Makefile PATCHES
	$(BUILD) ./cvtfont.py $(FONT_FILE) 18 mono18 32-126 PATCHES >$@ || \
	    { rm -f $@; exit 1; }

mono24.font: cvtfont.py Makefile PATCHES
	$(BUILD) ./cvtfont.py $(FONT_FILE) 24 mono24 32-126 PATCHES >$@ || \
	    { rm -f $@; exit 1; }

mono34.font: cvtfont.py Makefile PATCHES
	$(BUILD) ./cvtfont.py $(FONT_FILE) 34 mono34 32-126 PATCHES >$@ || \
	    { rm -f $@; exit 1; }

# only decimal digits
mono36.font: cvtfont.py Makefile PATCHES
	$(BUILD) ./cvtfont.py $(FONT_FILE) 36 mono36 48-57 PATCHES >$@ || \
	    { rm -f $@; exit 1; }

# only decimal digits
mono58.font: cvtfont.py Makefile PATCHES
	$(BUILD) ./cvtfont.py $(FONT_FILE) 58 mono58 48-57 PATCHES >$@ || \
	    { rm -f $@; exit 1; }

clean::
	rm -f $(FONTS)

spotless::
	rm -f show
