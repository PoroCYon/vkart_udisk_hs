# -----------------------------------------------------------------------------
# File:        project.mk
# Description: Configuration makefile that sets up the generic Makefile
# -----------------------------------------------------------------------------
EXECUTABLE   := usb_udisk
BUILD_DIR    := ./bin
SDK_LOCATION := ./sdk
SRC_DIRS     := ./src ./src/usb $(SDK_LOCATION)/src
INC_DIRS     := ./inc $(SDK_LOCATION)/inc
STARTUP_FILE := $(SDK_LOCATION)/Startup/startup_ch32v30x_D8C.S
LIB_DIRS     := 
LIBS         := 
CXXFLAGS     :=
CFLAGS       := -Wall -msmall-data-limit=8 -msave-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common
# enable ASAN:
#CFLAGS       += -fsanitize=kernel-address -DMcuASAN_CONFIG_IS_ENABLED=1
LDFLAGS      := -static -nostartfiles -Wl,--gc-sections -Wl,--cref
TOOLCHAIN_PREFIX := riscv32-unknown-elf-
AS := $(TOOLCHAIN_PREFIX)as
CC := $(TOOLCHAIN_PREFIX)gcc
OBJCOPY := $(TOOLCHAIN_PREFIX)objcopy
OBJDUMP := $(TOOLCHAIN_PREFIX)objdump
OPENOCD := 
OCD_CFG := ./wch-riscv.cfg
WLINK := $(shell command -v wlink 2>/dev/null)
ARCH := rv32imafc
ABI  := ilp32f

-include config.mk

ifeq ($(OPENOCD),)
ifeq ($(WLINK),)
	$(error "No OpenOCD or wlink available to flash the target!")
	$(error "Please install either https://github.com/ch32-rs/wlink or https://github.com/openwch/openocd_wch")
	$(error "and set OPENOCD in the config.mk file!");
endif
endif

# TinyUSB config
include ./tinyusb/src/tinyusb.mk
ADDL_SOURCES := $(addprefix tinyusb/,$(TINYUSB_SRC_C)) ./tinyusb/src/portable/wch/dcd_ch32_usbhs.c
INC_DIRS += ./tinyusb/src
CFLAGS += -DCFG_TUSB_MCU=OPT_MCU_CH32V307 -DBOARD_TUD_MAX_SPEED=OPT_MODE_HIGH_SPEED

LINKER_DIRECTORY := $(SDK_LOCATION)/Ld

TARGET = bare_metal
