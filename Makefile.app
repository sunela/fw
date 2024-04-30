#
# Makefile.app - Build the application (hardware-independent)
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

CFLAGS += -g -Wall -Wextra -Wshadow -Wno-unused-parameter \
	 -Wmissing-prototypes -Wmissing-declarations \
	 -I$(shell pwd) -Isys -Ilib -Igfx -Iui -Ifont -Icrypto
OBJS = ui.o demo.o timer.o debug.o mbox.o rnd.o hmac.o hotp.o base32.o \
    fmt.o \
    basic.o poly.o long_text.o font.o text.o \
    ui_off.o ui_pin.o ui_fail.o ui_accounts.o ui_account.o ui_list.o \
    ui_entry.o shape.o accounts.o

include Makefile.c-common


vpath fmt.c lib

vpath basic.c gfx
vpath poly.c gfx
vpath long_text.c gfx
vpath font.c font
vpath text.c gfx

vpath timer.c sys
vpath debug.c sys
vpath mbox.c sys
vpath rnd.c sys
vpath hmac.c crypto
vpath hotp.c crypto
vpath base32.c crypto

vpath ui.c ui
vpath demo.c ui
vpath ui_pin.c ui
vpath ui_off.c ui
vpath ui_fail.c ui
vpath ui_accounts.c ui
vpath ui_account.c ui
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
		$(MAKE) -C font clean
