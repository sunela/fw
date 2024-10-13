TOP = $(shell pwd)/..
BL_SDK_BASE = $(TOP)/../bouffalo_sdk

export BL_SDK_BASE
export TOP

# --- Target-specific settings ------------------------------------------------

CHIP_m1s = bl808
BOARD_m1s = bl808dk
CPU_ID_m1s = m0
ELF_m1s = sunela_bl808_m0.elf
LD_m1s = bl808_flash_m0.ld
TARGET_CFLAGS_m1s =

#
# Note that M0P has a BL618, but the SDK uses the same build process as for the
# (very similar) BL616.
#

CHIP_m0p = bl616
BOARD_m0p = bl616dk
CPU_ID_m0p =
ELF_m1s = sunela_bl616.elf
LD_m0p = bl616_flash.ld
TARGET_CFLAGS_m0p = -DCONFIG_BOARD_FLASH_INIT_SKIP

CHIP = $(CHIP_$(TARGET))
BOARD = $(BOARD_$(TARGET))
CPU_ID = $(CPU_ID_$(TARGET))
export TARGET_CFLAGS = $(TARGET_CFLAGS_$(TARGET))

# -----------------------------------------------------------------------------

CROSS_COMPILE = riscv64-unknown-elf-

# Note: COMX may change if there are other USB-serial devices or if enumeration
# happens while something is still using ACM0 or ACM1.
COMX = /dev/ttyACM1

SDK_LD = $(BL_SDK_BASE)/bsp/board/$(BOARD)/$(LD_$(TARGET))

# add custom cmake definition
#cmake_definition+=-Dxxx=sss

include $(BL_SDK_BASE)/project.build

# Cmake always says "Built target sunela_bl808_m0.elf" (or sunela_bl616.elf),
# even if it did nothing.
# We therefore have to explicitly force linking when libsunela.a may have
# changed.

.PHONY:	redo

redo:
	rm -f build/build_out/$(ELF_$(TARGET))
	sed '/^StackSize/s/0x0400/0x2000/' $(SDK_LD) >sunela-$(TARGET).ld
	$(MAKE)
