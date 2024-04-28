BL_SDK_BASE ?= $(shell pwd)/../../bouffalo_sdk

export BL_SDK_BASE

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
