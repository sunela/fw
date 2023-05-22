CFLAGS = -g -Wall -Wextra -Wshadow -Wno-unused-parameter \
	 -Wmissing-prototypes -Wmissing-declarations \
	 -I$(shell pwd) -Isys -Igfx -Iui
OBJS = ui.o timer.o basic.o poly.o vfont.o text.o ui_off.o ui_pin.o

include Makefile.c-common


vpath basic.c gfx
vpath poly.c gfx
vpath vfont.c gfx
vpath text.c gfx

vpath timer.c sys

vpath ui.c ui
vpath ui_pin.c ui
vpath ui_off.c ui

vpath citrine.jpg logo


ui.o:		citrine.inc

citrine.inc:    citrine.jpg scripts/pnmtorgb.pl
		jpegtopnm $< | scripts/pnmtorgb.pl >$@ || \
		    { rm -f $@; exit 1; }

clean::
		rm -f citrine.inc
