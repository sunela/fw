#
# Makefile.app - Build the application (hardware-independent)
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

CFLAGS += -g -Wall -Wextra -Wshadow -Wno-unused-parameter \
	 -Wmissing-prototypes -Wmissing-declarations \
	 -Wno-address-of-packed-member \
	 -I$(shell pwd) -Isys -Ilib -Igfx -Iui -Ifont -Icrypto -Idb -Imain \
	 -Irmt -Ilib/bip39
OBJS = ui.o demo.o timer.o debug.o mbox.o rnd.o hmac.o hotp.o base32.o \
    tweetnacl.o \
    fmt.o imath.o bip39.o version.o rmt.o rmt-db.o \
    basic.o poly.o shape.o long_text.o font.o text.o \
    dbcrypt.o block.o span.o db.o settings.o pin.o secrets.o \
    ui_off.o ui_pin.o ui_fail.o ui_accounts.o ui_account.o ui_field.o \
    wi_list.o ui_entry.o wi_general_entry.o ui_time.o ui_overlay.o \
    ui_confirm.o ui_setup.o ui_storage.o ui_version.o ui_rd.o ui_notice.o \
    ui_pin_change.o wi_pin_entry.o ui_rmt.o ui_new.o ui_choices.o \
    ui_show_master.o

include Makefile.c-common


vpath fmt.c lib
vpath imath.c lib
vpath bip39.c lib/bip39

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
vpath tweetnacl.c crypto

vpath rmt.c rmt
vpath rmt-db.c rmt

vpath dbcrypt.c db
vpath block.c db
vpath span.c db
vpath db.c db
vpath settings.c db
vpath pin.c db
vpath secrets.c db

vpath ui.c ui
vpath ui_pin.c ui
vpath ui_off.c ui
vpath ui_fail.c ui
vpath ui_accounts.c ui
vpath ui_account.c ui
vpath ui_field.c ui
vpath ui_entry.c ui
vpath ui_time.c ui
vpath ui_overlay.c ui
vpath ui_confirm.c ui
vpath ui_setup.c ui
vpath ui_storage.c ui
vpath ui_version.c ui
vpath ui_rd.c ui
vpath ui_notice.c ui
vpath ui_pin_change.c ui
vpath ui_rmt.c ui
vpath ui_new.c ui
vpath ui_choices.c ui
vpath ui_show_master.c ui

vpath wi_list.c ui
vpath wi_general_entry.c ui
vpath wi_pin_entry.c ui
vpath demo.c ui

vpath citrine.jpg logo
vpath mksintab.pl lib

# --- Generated files ---------------------------------------------------------

ui.o:		citrine.inc
lib/imath.c:	sin.inc

citrine.inc:    citrine.jpg scripts/pnmtorgb.pl
		jpegtopnm $< | scripts/pnmtorgb.pl >$@ || \
		    { rm -f $@; exit 1; }

sin.inc:	mksintab.pl
		$(BUILD) perl $< >$@ || { rm -f $@; exit 1; }

bip39.c:	lib/bip39/english.inc

lib/bip39/english.inc: lib/bip39/english.txt Makefile
		sed 's/.*/"&",/' <$< > $@ || { rm -f $@; exit 1; }

# --- Build version -----------------------------------------------------------

.PHONY:		main/version.c

BUILD_DATE = $(shell date +'%Y%m%d-%H:%M')
BUILD_HASH = $(shell git rev-parse HEAD | cut -c 1-7)
BUILD_DIRTY = $(shell [ -z "`git status -s -uno`" ]; echo $$?)

$(OBJDIR)version$(OBJ_SUFFIX): main/version.c | generated_headers
		$(CC) $(CFLAGS) $(CFLAGS_CC) \
		    -DBUILD_DATE="\"$(BUILD_DATE)\"" \
		    -DBUILD_HASH=0x$(BUILD_HASH) \
		    -DBUILD_DIRTY=$(BUILD_DIRTY) \
		    -o $@ -c $<

# --- Cleanup -----------------------------------------------------------------

clean::
		rm -f citrine.inc
		rm -f sin.inc lib/bip3/english.inc
		$(MAKE) -C font clean
