TOP = $(shell pwd)/..
BL_SDK_BASE = $(TOP)/../bouffalo_sdk

export BL_SDK_BASE
export TOP

CHIP = bl808
BOARD = bl808dk
CPU_ID = m0
CROSS_COMPILE = riscv64-unknown-elf-

# Note: COMX may change if there are other USB-serial devices or if enumeration
# happens while something is still using ACM0 or ACM1.
COMX = /dev/ttyACM1

# add custom cmake definition
#cmake_definition+=-Dxxx=sss

include $(BL_SDK_BASE)/project.build

# Cmake always says "Built target sunela_bl808_m0.elf", even if it did nothing.
# We therefore have to explicitly force linking when libsunela.a may have
# changed.

.PHONY:	redo

redo:
	rm -f build/build_out/sunela_bl808_m0.elf
	$(MAKE)
