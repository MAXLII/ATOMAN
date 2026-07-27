PROJECT_ROOT := ../../../..
PLATFORM_ROOT := ../..
TARGET := hc32f334_bootloader
BUILD_DIR ?= build
UPGRADE_MODE ?= staged
W25Q_SELF_TEST ?= 0
EXTRA_DEFS ?=
OPT ?= -Os
LTO_FLAGS ?= -flto
LD_SCRIPT := HC32F334_bootloader.ld

PREFIX := arm-none-eabi-
CC := $(PREFIX)gcc
AS := $(PREFIX)gcc -x assembler-with-cpp
CP := $(PREFIX)objcopy
SZ := $(PREFIX)size

PROJECT_SOURCES := \
../main.c \
../boot_interrupt.c \
../boot_uart.c \
../boot_comm_link.c \
../hc32_boot_platform.c \
../fal_cfg.c \
$(PLATFORM_ROOT)/src/hc32_boot_handoff.c \
$(PLATFORM_ROOT)/bsp/bsp_clk.c \
$(PLATFORM_ROOT)/bsp/bsp_efm_flash.c \
$(PLATFORM_ROOT)/bsp/bsp_w25q64.c \
$(PLATFORM_ROOT)/src/systick.c \
$(PROJECT_ROOT)/code/section/baremetal/section.c \
$(PROJECT_ROOT)/code/comm/comm.c \
$(PROJECT_ROOT)/code/interface/fal/fal_core.c \
$(PROJECT_ROOT)/code/app/bootloader/bootloader_core.c \
$(PROJECT_ROOT)/code/app/bootloader/bootloader_fal_adapter.c \
$(PROJECT_ROOT)/code/app/bootloader/bootloader_metadata.c \
$(PROJECT_ROOT)/code/app/bootloader/bootloader_protocol.c \
$(PROJECT_ROOT)/code/app/bootloader/bootloader_section_service.c

VENDOR_SOURCES := \
$(PLATFORM_ROOT)/Firmware/CMSIS/HDSC/hc32f3xx/Source/system_hc32f334.c \
$(PLATFORM_ROOT)/Firmware/HC32_LL_Driver/src/hc32_ll.c \
$(PLATFORM_ROOT)/Firmware/HC32_LL_Driver/src/hc32_ll_aos.c \
$(PLATFORM_ROOT)/Firmware/HC32_LL_Driver/src/hc32_ll_clk.c \
$(PLATFORM_ROOT)/Firmware/HC32_LL_Driver/src/hc32_ll_dma.c \
$(PLATFORM_ROOT)/Firmware/HC32_LL_Driver/src/hc32_ll_efm.c \
$(PLATFORM_ROOT)/Firmware/HC32_LL_Driver/src/hc32_ll_fcg.c \
$(PLATFORM_ROOT)/Firmware/HC32_LL_Driver/src/hc32_ll_gpio.c \
$(PLATFORM_ROOT)/Firmware/HC32_LL_Driver/src/hc32_ll_icg.c \
$(PLATFORM_ROOT)/Firmware/HC32_LL_Driver/src/hc32_ll_spi.c \
$(PLATFORM_ROOT)/Firmware/HC32_LL_Driver/src/hc32_ll_usart.c \
$(PLATFORM_ROOT)/Firmware/HC32_LL_Driver/src/hc32_ll_utility.c

ASM_SOURCES := $(PLATFORM_ROOT)/gcc/startup_hc32f334.S

MCU_FLAGS := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
C_DEFS := -DHC32F334 -DIS_HC32F334 -DTOOLCHAIN_GCC -DUSE_DDL_DRIVER \
          -DFIRMWARE_TYPE_BOOTLOADER -DAPP_START_ADDR=0 -DVECT_TAB_OFFSET=0 \
          -DHC32F334_IAP_BASE=0x00004000UL
ifeq ($(UPGRADE_MODE),direct)
C_DEFS += -DHC32_BOOTLOADER_DEFAULT_MODE=BOOTLOADER_UPGRADE_MODE_DIRECT
endif
C_DEFS += -DHC32_BOOT_W25Q64_SELF_TEST=$(W25Q_SELF_TEST)
C_DEFS += $(EXTRA_DEFS)
C_INCLUDES := \
-I.. \
-I$(PLATFORM_ROOT)/bsp \
-I$(PLATFORM_ROOT)/inc \
-I$(PLATFORM_ROOT)/src \
-I$(PLATFORM_ROOT)/Firmware/CMSIS/Include \
-I$(PLATFORM_ROOT)/Firmware/CMSIS/HDSC/hc32f3xx/Include \
-I$(PLATFORM_ROOT)/Firmware/HC32_LL_Driver/inc \
-I$(PROJECT_ROOT)/code/section/baremetal \
-I$(PROJECT_ROOT)/code/section \
-I$(PROJECT_ROOT)/code/comm \
-I$(PROJECT_ROOT)/code/dbg \
-I$(PROJECT_ROOT)/code/interface/fal \
-I$(PROJECT_ROOT)/code/interface/common \
-I$(PROJECT_ROOT)/code/app/bootloader

COMMON_FLAGS := $(MCU_FLAGS) $(C_DEFS) $(C_INCLUDES) -std=c11 $(OPT) $(LTO_FLAGS) -g3 \
                -ffunction-sections -fdata-sections -MMD -MP
PROJECT_CFLAGS := $(COMMON_FLAGS) -Wall -Wextra -Werror -Wpedantic \
                  -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes \
                  -Wmissing-prototypes -Wundef
VENDOR_CFLAGS := $(COMMON_FLAGS) -Wall -Wextra -Wno-unused-parameter
ASFLAGS := $(MCU_FLAGS) $(C_DEFS) -ffunction-sections -fdata-sections
LDFLAGS := $(MCU_FLAGS) -specs=nano.specs -T$(LD_SCRIPT) -lc -lm -lnosys \
           $(LTO_FLAGS) \
           -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref,--gc-sections \
           -Wl,--print-memory-usage,--no-warn-rwx-segments,-z,noexecstack

PROJECT_OBJECTS := $(addprefix $(BUILD_DIR)/project/,$(notdir $(PROJECT_SOURCES:.c=.o)))
VENDOR_OBJECTS := $(addprefix $(BUILD_DIR)/vendor/,$(notdir $(VENDOR_SOURCES:.c=.o)))
ASM_OBJECTS := $(addprefix $(BUILD_DIR)/startup/,$(notdir $(ASM_SOURCES:.S=.o)))
OBJECTS := $(PROJECT_OBJECTS) $(VENDOR_OBJECTS) $(ASM_OBJECTS)
DEPS := $(OBJECTS:.o=.d)

vpath %.c $(sort $(dir $(PROJECT_SOURCES)))
vpath %.c $(sort $(dir $(VENDOR_SOURCES)))
vpath %.S $(sort $(dir $(ASM_SOURCES)))

.PHONY: all clean
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/project/boot_comm_link.o: PROJECT_CFLAGS += -Wno-pedantic

$(BUILD_DIR)/project/%.o: %.c Makefile | $(BUILD_DIR)/project
	$(CC) -c $(PROJECT_CFLAGS) -MF$(@:.o=.d) $< -o $@

$(BUILD_DIR)/vendor/%.o: %.c Makefile | $(BUILD_DIR)/vendor
	$(CC) -c $(VENDOR_CFLAGS) -MF$(@:.o=.d) $< -o $@

$(BUILD_DIR)/startup/%.o: %.S Makefile | $(BUILD_DIR)/startup
	$(AS) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(LD_SCRIPT) | $(BUILD_DIR)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	$(CP) -O ihex $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$(CP) -O binary -S $< $@

$(BUILD_DIR) $(BUILD_DIR)/project $(BUILD_DIR)/vendor $(BUILD_DIR)/startup:
	@if not exist "$(subst /,\,$@)" mkdir "$(subst /,\,$@)"

clean:
	@if exist "$(subst /,\,$(BUILD_DIR))" rmdir /s /q "$(subst /,\,$(BUILD_DIR))"

-include $(DEPS)
