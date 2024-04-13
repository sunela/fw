#
# Makefile.app - Build the application (hardware-independent)
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

CFLAGS += -g -Wall -Wextra -Wshadow -Wno-unused-parameter \
	 -Wmissing-prototypes -Wmissing-declarations \
	 -I$(shell pwd) -Isys -Igfx -Iui
OBJS = ui.o timer.o debug.o rnd.o basic.o poly.o vfont.o text.o long_text.o \
    ui_off.o ui_pin.o ui_fail.o ui_accounts.o ui_list.o ui_entry.o \
    shape.o accounts.o

include Makefile.c-common


vpath basic.c gfx
vpath poly.c gfx
vpath vfont.c gfx
vpath text.c gfx
vpath long_text.c gfx

vpath timer.c sys
vpath debug.c sys
vpath rnd.c sys

vpath ui.c ui
vpath ui_pin.c ui
vpath ui_off.c ui
vpath ui_fail.c ui
vpath ui_accounts.c ui
vpath ui_list.c ui
vpath ui_entry.c ui
vpath shape.c ui
vpath accounts.c ui

vpath citrine.jpg logo


ui.o:		citrine.inc

citrine.inc:    citrine.jpg scripts/pnmtorgb.pl
		jpegtopnm $< | scripts/pnmtorgb.pl >$@ || \
		    { rm -f $@; exit 1; }

clean::
		rm -f citrine.inc
