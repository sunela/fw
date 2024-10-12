#
# Makefile - Main makefile
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

TARGETS = sim sdk
export TARGET = m1s

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

COMX = /dev/ttyACM1

#
# 16 MB (M1s) or 8 MB (M0p) Flash. We reserve the first half for the firmware
# and use the second half for data storage. Of the storage area, we use one
# partition of 2 MB.
#

FLASH_TOTAL_SIZE_m1s =	 0x01000000	# 16 MB
FLASH_STORAGE_BASE_m1s = 0x00800000	#  8 MB
FLASH_STORAGE_SIZE_m1s = 0x00200000	#  2 MB

FLASH_TOTAL_SIZE_m0p =	 0x00800000	#  8 MB
FLASH_STORAGE_BASE_m0p = 0x00400000	#  4 MB
FLASH_STORAGE_SIZE_m0p = 0x00200000	#  2 MB

export FLASH_TOTAL_SIZE = $(FLASH_TOTAL_SIZE_$(TARGET))
export FLASH_STORAGE_BASE = $(FLASH_STORAGE_BASE_$(TARGET))
export FLASH_STORAGE_SIZE = $(FLASH_STORAGE_SIZE_$(TARGET))

SDK = $(shell pwd)/../bouffalo_sdk/
FLASH_CUBE = $(SDK)/tools/bflb_tools/bouffalo_flash_cube/BLFlashCommand-ubuntu
FLASH = $(FLASH_CUBE)  --interface uart --baudrate 2000000 --port=$(COMX) \
	--chipname bl808 --cpu_id m0 --flash
REGION_ALL = --start 0x0 --len $(FLASH_TOTAL_SIZE)
REGION_DB = --start $(FLASH_STORAGE_BASE) --len $(FLASH_STORAGE_SIZE)

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

# --- BL618/BL808 console -----------------------------------------------------

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

# idem, for nm

nm:
	riscv64-unknown-elf-nm sdk/build/build_out/sunela_bl808_m0.elf

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
