#
# Makefile - Main makefile
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

TARGETS = sim sdk

FONTS = mono14.font mono18.font mono24.font mono34.font mono36.font mono58.font

.PHONY:	all sim sdk fonts db empty-db gdb nm clean spotless test tests
.PHONY:	flash picocom upload download download-all erase

all:	fonts $(TARGETS) dummy.db

sim:
	$(MAKE) -f Makefile.sim

sdk:
	$(MAKE) -f Makefile.sdk
	$(MAKE) -C sdk redo

# --- Generated files ---------------------------------------------------------

fonts:	$(FONTS:%=font/%)

$(FONTS:%=font/%): font/Makefile font/cvtfont.py
	$(MAKE) -C font

# dd if=/dev/zero bs=32 count=1 | base32
PK = AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA====

db:
	rm -f dummy.db && make dummy.db

dummy.db: tools/accenc.py accounts.json
	$(BUILD) $^ >$@ $(PK) || { rm -f $@; exit 1; }

empty-db:
	echo "[]" | tools/accenc.py /dev/stdin >dummy.db || \
	    { rm -f dummy.db; exit 1; }

# --- Flashing ----------------------------------------------------------------

flash upload download download-all erase:
	$(MAKE) -C sdk $@

# --- BL808 console -----------------------------------------------------------

CONSOLE = /dev/ttyACM1

# For convenience: invoke picocom with flashing on ^A^S (+ Enter)

picocom:
	picocom --send-cmd 'sh -c "make flash 1>&2"' --receive-cmd '' \
	    -b 2000000 $(CONSOLE)

# --- Debugging ---------------------------------------------------------------

# For convenience: invoke gdb on the target (SDK) executable
# E.g., for  info line *0x...

gdb nm:
	$(MAKE) -C sdk $@

# --- Testing -----------------------------------------------------------------

test tests:
	$(MAKE) -C tests

# --- Cleanup -----------------------------------------------------------------

clean:
	for n in $(TARGETS); do $(MAKE) -f Makefile.$$n clean; done
	$(MAKE) -C sdk clean

spotless:
	for n in $(TARGETS); do $(MAKE) -f Makefile.$$n spotless; done
	rm -f dummy.db
