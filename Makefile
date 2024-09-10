#
# Makefile - Main makefile
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

TARGETS = sim sdk

FONTS = mono14.font mono18.font mono24.font mono34.font mono36.font mono58.font

.PHONY:	all sim sdk fonts gdb clean spotless
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

dummy.db: tools/accenc.py accounts.json
	$(BUILD) $^ >$@ $(PK) || { rm -f $@; exit 1; }

# --- Flashing ----------------------------------------------------------------

COMX = /dev/ttyACM1

SDK = $(shell pwd)/../bouffalo_sdk/
FLASH_CUBE = $(SDK)/tools/bflb_tools/bouffalo_flash_cube/BLFlashCommand-ubuntu
FLASH = $(FLASH_CUBE)  --interface uart --baudrate 2000000 --port=$(COMX) \
	--chipname bl808 --cpu_id m0 --flash
REGION_ALL = --start 0x0 --len 0x1000000
REGION_DB = --start 0x800000 --len 0x200000

flash:
	$(MAKE) -C sdk flash COMX=$(COMX)

upload:	$(shell pwd)/dummy.db
	$(FLASH) --write $(REGION_DB) --file $<
# --config=sdk/flash_prog_cfg.ini \

download:
	$(FLASH) --read $(REGION_DB) --file $(shell pwd)/flash.db

download-all:
	$(FLASH) --read $(REGION_ALL) --file $(shell pwd)/all.bin

erase:
	$(FLASH) --erase --whole_chip

# --- BL808 console -----------------------------------------------------------

CONSOLE = /dev/ttyACM1

# For convenience: invoke picocom with flashing on ^A^S (+ Enter)

picocom:
	picocom --send-cmd 'sh -c "make flash 1>&2"' --receive-cmd '' \
	    -b 2000000 $(CONSOLE)

# --- Debugging ---------------------------------------------------------------

# For convenience: invoke gdb on the target (SDK) executable
# E.g., for  info line *0x...

gdb:
	riscv64-unknown-elf-gdb sdk/build/build_out/sunela_bl808_m0.elf

# --- Cleanup -----------------------------------------------------------------

clean:
	for n in $(TARGETS); do $(MAKE) -f Makefile.$$n clean; done
	$(MAKE) -C sdk clean

spotless:
	for n in $(TARGETS); do $(MAKE) -f Makefile.$$n spotless; done
	rm -f dummy.db
