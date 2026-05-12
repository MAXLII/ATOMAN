# SPDX-License-Identifier: MIT
# @file    Makefile
# @brief   Makefile for GD32G553 using ARM GCC
#
# @details
#          Build system for GD32G553RE (Cortex-M4) using arm-none-eabi-gcc.
#          Usage: make [-jN] [clean] [all]
#
# @author  Generated
# @date    2026-05-11
# @version 1.0.0

# ---------------------------------------------------------------------------
# Project Configuration
# ---------------------------------------------------------------------------
PROJECT_NAME    := gd32g553
TARGET          := isp_firmware

# Build output directory
BUILD_DIR       := build

# ---------------------------------------------------------------------------
# Toolchain - ARM GCC (arm-none-eabi-*)
# ---------------------------------------------------------------------------
PREFIX          := arm-none-eabi-

CC              := $(PREFIX)gcc
AS              := $(PREFIX)gcc -x assembler-with-cpp
CP              := $(PREFIX)objcopy
SZ              := $(PREFIX)size
OD              := $(PREFIX)objdump
HEX             := $(CP) -O ihex
BIN             := $(CP) -O binary -S

# ---------------------------------------------------------------------------
# Compilation Flags
# ---------------------------------------------------------------------------
CPU             := -mcpu=cortex-m4 -mthumb
FPU             := -mfpu=fpv4-sp-d16
FLOAT-ABI       := -mfloat-abi=hard
MCU             := $(CPU) $(FPU) $(FLOAT-ABI)

# C standard
CSTD            := -std=c11 -pedantic

# Optimization
OPT             := -O2

# Debug info
DEBUG           := 1

# Preprocessor definitions
DEFINES         := -DGD32G5X3 -DFIRMWARE_TYPE_ISP -DAPP_START_ADDR=0x08000000

# Linker script
LDSCRIPT        := ldscripts/gd32g5x3_flash.ld

# Libraries
LIBS            := -lc -lm -lnosys
LDFLAGS          := $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBS) \
                   -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections
LDFLAGS          += -u _printf_float -u _scanf_float -u _printf_precision

# ASM defines
AS_DEFS         :=

# ---------------------------------------------------------------------------
# Flags
# ---------------------------------------------------------------------------
ASFLAGS         := $(MCU) $(AS_DEFS) $(OPT) -Wall -Wextra -Wpedantic -fdata-sections -ffunction-sections
CFLAGS          := $(MCU) $(DEFINES) $(C_INCLUDES) $(OPT) -Wall -Wextra -Wpedantic -fdata-sections -ffunction-sections
CFLAGS          += $(CSTD)
ifeq ($(DEBUG), 1)
CFLAGS          += -g -gdwarf-2
endif
CFLAGS          += -MMD -MP -MF"$(@:%.o=%.d)"

# ---------------------------------------------------------------------------
# Source Files (absolute paths)
# ---------------------------------------------------------------------------
CODEDIR         := ../code
FW_DIR          := .

ASM_SOURCES     := $(FW_DIR)/gcc_startup/startup_gd32g5x3.s

C_SOURCES       := \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_adc.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_can.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_cau_aes.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_cau_des.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_cau_tdes.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_cau.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_cla.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_cmp.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_cpdm.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_crc.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_dac.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_dbg.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_dma.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_exmc.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_exti.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_fac.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_fft.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_fmc_ramfunc.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_fmc.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_fwdgt.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_gpio.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_hpdf.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_hrtimer.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_i2c.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_lptimer.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_misc.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_pmu.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_qspi.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_rcu.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_rtc.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_spi.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_syscfg.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_timer.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_tmu.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_trigsel.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_trng.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_usart.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_vref.c \
$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_wwdgt.c \
$(FW_DIR)/Firmware/CMSIS/GD/GD32G5x3/Source/system_gd32g5x3.c \
$(FW_DIR)/bsp/src/bsp_can.c \
$(FW_DIR)/bsp/src/bsp_gpio.c \
$(FW_DIR)/bsp/src/bsp_init.c \
$(FW_DIR)/bsp/src/bsp_timer.c \
$(FW_DIR)/bsp/src/bsp_usart.c \
$(FW_DIR)/src/gd32g5x3_it.c \
$(FW_DIR)/src/main.c \
$(FW_DIR)/src/systick.c \
$(CODEDIR)/app/demo/demo.c \
$(CODEDIR)/lib/dft.c \
$(CODEDIR)/lib/pi_tustin.c \
$(CODEDIR)/lib/pr.c \
$(CODEDIR)/lib/record_dict.c \
$(CODEDIR)/dbg/perf.c \
$(CODEDIR)/dbg/perf_service.c \
$(CODEDIR)/dbg/scope.c \
$(CODEDIR)/dbg/scope_service.c \
$(CODEDIR)/dbg/sfra.c \
$(CODEDIR)/dbg/sfra_service.c \
$(CODEDIR)/dbg/trace.c \
$(CODEDIR)/dbg/trace_service.c \
$(CODEDIR)/dbg/shell.c \
$(CODEDIR)/dbg/shell_service.c \
$(CODEDIR)/interface/ac/gpio.c \
$(CODEDIR)/interface/ac/comm_link.c \
$(CODEDIR)/section/section.c \
$(CODEDIR)/comm/comm.c

# ---------------------------------------------------------------------------
# Include paths
# ---------------------------------------------------------------------------
C_INCLUDES      := \
-I$(CODEDIR)/app \
-I$(CODEDIR)/app/demo \
-I$(CODEDIR)/lib \
-I$(CODEDIR)/dbg \
-I$(CODEDIR)/interface/ac \
-I$(CODEDIR)/interface \
-I$(CODEDIR)/interface/common \
-I$(CODEDIR)/section \
-I$(CODEDIR)/comm \
-I$(FW_DIR)/Firmware/CMSIS \
-I$(FW_DIR)/Firmware/CMSIS/GD/GD32G5x3/Include \
-I$(FW_DIR)/Firmware/GD32G5x3_standard_peripheral/Include \
-I$(FW_DIR)/inc \
-I$(FW_DIR)/bsp/inc

# ---------------------------------------------------------------------------
# Object files
# ---------------------------------------------------------------------------
OBJECTS         := $(C_SOURCES:%=$(BUILD_DIR)/%.o)
OBJECTS         := $(OBJECTS:%.c=$(BUILD_DIR)/%.o)
OBJECTS         := $(patsubst %.s,$(BUILD_DIR)/%.o,$(OBJECTS))
OBJECTS         := $(patsubst %.c,$(BUILD_DIR)/%.o,$(OBJECTS))

# ---------------------------------------------------------------------------
# Build Rules
# ---------------------------------------------------------------------------
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$(BIN) $< $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean
