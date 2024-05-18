#
# Makefile - Main makefile
#
# This work is licensed under the terms of the MIT License.
# A copy of the license can be found in the file LICENSE.MIT
#

TARGETS = sim fw sdk

FONTS = mono18.font mono24.font mono34.font mono36.font mono58.font

.PHONY:	all sim fw sdk fonts gdb clean spotless
.PHONY:	flash picocom upload download erase

all:	fonts $(TARGETS) dummy.db

sim:
	$(MAKE) -f Makefile.sim

fw:
	$(MAKE) -f Makefile.fw

CONSOLE = /dev/ttyACM1

sdk:
	$(MAKE) -f Makefile.sdk
	$(MAKE) -C sdk redo

flash:
	$(MAKE) -C sdk flash COMX=$(CONSOLE)

SDK = $(shell pwd)/../bouffalo_sdk/
FLASH = $(SDK)/tools/bflb_tools/bouffalo_flash_cube/BLFlashCommand-ubuntu
COMX = /dev/ttyACM1

upload:	$(shell pwd)/dummy.db
	$(FLASH) --interface uart --baudrate 2000000 --port=$(COMX) \
	    --chipname bl808 --cpu_id m0 \
	    --flash --write --start 0x800000 --len 0x200000 --file $<
# --config=sdk/flash_prog_cfg.ini \

download:	
	$(FLASH) --interface uart --baudrate 2000000 --port=$(COMX) \
	    --chipname bl808 --cpu_id m0 \
	    --flash --read --start 0x0 --len 0x1000000 \
	    --file $(shell pwd)/foo

erase:
	$(FLASH) --interface uart --baudrate 2000000 --port=$(COMX) \
	    --chipname bl808 --cpu_id m0 \
	    --flash --erase --whole_chip

# For convenience: invoke picocom with flashing on ^A^S (+ Enter)

picocom:
	picocom --send-cmd 'sh -c "make flash 1>&2"' --receive-cmd '' \
	    -b 2000000 $(CONSOLE)

# For convenience: invoke gdb on the target (SDK) executable
# E.g., for  info line *0x...

gdb:
	riscv64-unknown-elf-gdb sdk/build/build_out/sunela_bl808_m0.elf

fonts:	$(FONTS:%=font/%)

$(FONTS:%=font/%): font/Makefile font/cvtfont.py
	$(MAKE) -C font

dummy.db: tools/accenc.py accounts.json
	$(BUILD) $^ >$@ || { rm -f $@; exit 1; }

clean:
	for n in $(TARGETS); do $(MAKE) -f Makefile.$$n clean; done
	$(MAKE) -C sdk clean

spotless:
	for n in $(TARGETS); do $(MAKE) -f Makefile.$$n spotless; done
	rm -f dummy.db
