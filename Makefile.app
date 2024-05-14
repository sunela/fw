#
# Makefile.app - Build the application (hardware-independent)
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

CFLAGS += -g -Wall -Wextra -Wshadow -Wno-unused-parameter \
	 -Wmissing-prototypes -Wmissing-declarations \
	 -Wno-address-of-packed-member \
	 -I$(shell pwd) -Isys -Ilib -Igfx -Iui -Ifont -Icrypto -Idb
OBJS = ui.o demo.o timer.o debug.o mbox.o rnd.o hmac.o hotp.o base32.o \
    fmt.o imath.o \
    basic.o poly.o shape.o long_text.o font.o text.o \
    dbcrypt.o block.o db.o \
    ui_off.o ui_pin.o ui_fail.o ui_accounts.o ui_account.o wi_list.o \
    ui_entry.o ui_time.o ui_overlay.o ui_setup.o ui_storage.o

include Makefile.c-common


vpath fmt.c lib
vpath imath.c lib

vpath basic.c gfx
vpath poly.c gfx
vpath long_text.c gfx
vpath font.c font
vpath text.c gfx
vpath shape.c gfx

vpath timer.c sys
vpath debug.c sys
vpath mbox.c sys
vpath rnd.c sys

vpath hmac.c crypto
vpath hotp.c crypto
vpath base32.c crypto

vpath dbcrypt.c db
vpath block.c db
vpath db.c db

vpath ui.c ui
vpath ui_pin.c ui
vpath ui_off.c ui
vpath ui_fail.c ui
vpath ui_accounts.c ui
vpath ui_account.c ui
vpath ui_entry.c ui
vpath ui_time.c ui
vpath ui_overlay.c ui
vpath ui_setup.c ui
vpath ui_storage.c ui

vpath wi_list.c ui
vpath demo.c ui

vpath citrine.jpg logo
vpath mksintab.pl lib


ui.o:		citrine.inc
lib/imath.c:	sin.inc

citrine.inc:    citrine.jpg scripts/pnmtorgb.pl
		jpegtopnm $< | scripts/pnmtorgb.pl >$@ || \
		    { rm -f $@; exit 1; }

sin.inc:	mksintab.pl
		$(BUILD) perl $< >$@ || { rm -f $@; exit 1; }

clean::
		rm -f citrine.inc
		rm -f sin.inc
		$(MAKE) -C font clean
