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
FONT_FILE = $(shell fc-list '$(FONT_PATTERN)' file | grep /liberation/ | \
    sed 's/:.*//')

FONTS = mono18.font mono28.font mono34.font mono38.font mono58.font

CFLAGS = -Wall -Wextra -g
OBJS = font.o

include ../Makefile.c-common

.PHONY:	all clean spotless

all::	show

show:	show.c $(FONTS) $(OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(OBJS)

mono18.font: cvtfont.py Makefile
	./cvtfont.py $(FONT_FILE) 18 mono18 32-126 >$@ || { rm -f $@; exit 1; }

mono28.font: cvtfont.py Makefile
	./cvtfont.py $(FONT_FILE) 28 mono28 32-126 >$@ || { rm -f $@; exit 1; }

mono34.font: cvtfont.py Makefile
	./cvtfont.py $(FONT_FILE) 34 mono34 32-126 >$@ || { rm -f $@; exit 1; }

# only decimal digits
mono38.font: cvtfont.py Makefile
	./cvtfont.py $(FONT_FILE) 38 mono38 48-57 >$@ || { rm -f $@; exit 1; }

# only decimal digits
mono58.font: cvtfont.py Makefile
	./cvtfont.py $(FONT_FILE) 58 mono58 48-57 >$@ || { rm -f $@; exit 1; }

clean::
	rm -f $(FONTS)

spotless::
	rm -f show
