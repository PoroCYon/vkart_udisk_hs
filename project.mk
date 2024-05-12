# -----------------------------------------------------------------------------
# File:        project.mk
# Description: Configuration makefile that sets up the generic Makefile
# -----------------------------------------------------------------------------
EXECUTABLE   := usb_udisk
BUILD_DIR    := ./bin
SDK_LOCATION := ./sdk
SRC_DIRS     := ./src $(SDK_LOCATION)/src 
INC_DIRS     := ./inc $(SDK_LOCATION)/inc
STARTUP_FILE := $(SDK_LOCATION)/Startup/startup_ch32v30x_D8C.S
LIB_DIRS     := 
LIBS         := 
ADDL_SOURCES := 
CFLAGS       :=
CXXFLAGS     :=
CFLAGS       := -Wall -nostartfiles  -msmall-data-limit=8 -msave-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Xlinker --gc-sections 
LDFLAGS      := -static
TOOLCHAIN_PREFIX := riscv32-unknown-elf-
AS := $(TOOLCHAIN_PREFIX)as
CC := $(TOOLCHAIN_PREFIX)gcc
OBJCOPY := $(TOOLCHAIN_PREFIX)objcopy
OBJDUMP := $(TOOLCHAIN_PREFIX)objdump
OPENOCD := $(HOME)/src/pico/_ch522/openocd_wchlink/riscv-openocd/inst/bin/openocd
OCD_CFG := $(HOME)/src/pico/_ch522/openocd_wchlink/riscv-openocd/inst/bin/wch-riscv.cfg
ARCH := rv32imafc
ABI  := ilp32


LINKER_DIRECTORY := $(SDK_LOCATION)/Ld

TARGET = bare_metal
