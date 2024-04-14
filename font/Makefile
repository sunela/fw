FONT = /usr/share/fonts/truetype/liberation/LiberationMono-Bold.ttf
FONTS = mono18.font mono28.font mono34.font mono38.font mono58.font

CFLAGS = -Wall -Wextra -g
OBJS = font.o

include ../Makefile.c-common

.PHONY:	all clean spotless

all::	show

show:	show.c $(FONTS) $(OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(OBJS)

mono18.font: cvtfont.py Makefile
	./cvtfont.py $(FONT) 18 mono18 >$@ || { rm -f $@; exit 1; }

mono28.font: cvtfont.py Makefile
	./cvtfont.py $(FONT) 28 mono28 >$@ || { rm -f $@; exit 1; }

mono34.font: cvtfont.py Makefile
	./cvtfont.py $(FONT) 34 mono34 >$@ || { rm -f $@; exit 1; }

# only decimal digits
mono38.font: cvtfont.py Makefile
	./cvtfont.py $(FONT) 38 mono38 48-57 >$@ || { rm -f $@; exit 1; }

# only decimal digits
mono58.font: cvtfont.py Makefile
	./cvtfont.py $(FONT) 58 mono58 48-57 >$@ || { rm -f $@; exit 1; }

clean::
	rm -r $(FONTS)

spotless::
	rm -r show
