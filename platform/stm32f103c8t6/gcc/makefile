# ARM GCC port; public framework sources remain in code/.
MAKE_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
SECTION_RUNTIME ?= srtos_m
ifneq ($(filter $(SECTION_RUNTIME),baremetal srtos_m),$(SECTION_RUNTIME))
$(error SECTION_RUNTIME must be baremetal or srtos_m)
endif
BOARD_TEST ?= 0
ifneq ($(filter $(BOARD_TEST),0 1),$(BOARD_TEST))
$(error BOARD_TEST must be 0 or 1)
endif
TARGET := stm32f103c8t6_demo
BUILD_DIR := ../build_$(SECTION_RUNTIME)$(if $(filter 1,$(BOARD_TEST)),_test,)
PREFIX ?= arm-none-eabi-
CC := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
SIZE := $(PREFIX)size
MCU_FLAGS := -mcpu=cortex-m3 -mthumb -mfloat-abi=soft
DEFINES := -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER -DPLATFORM_STM32F103C8T6 -DTOOLCHAIN_GCC
INCLUDES := -I../inc -I../bsp -I../../../code/section/$(SECTION_RUNTIME) -I../../../code/section \
    -I../../../code/data_source/comm -I../../../code/dbg -I../../../code/lib \
    -isystem ../Firmware/Libraries/CMSIS/CM3/CoreSupport \
    -isystem ../Firmware/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x \
    -isystem ../Firmware/Libraries/STM32F10x_StdPeriph_Driver/inc
COMMON_CFLAGS := $(MCU_FLAGS) $(DEFINES) $(INCLUDES) -std=c11 -Os -g3 \
    -ffunction-sections -fdata-sections -Wa,--noexecstack -MMD -MP
WARN_FLAGS := -Wall -Wextra -Werror -Wpedantic -Wconversion -Wsign-conversion \
    -Wshadow -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wundef
PROJECT_CFLAGS := $(COMMON_CFLAGS) $(WARN_FLAGS)
VENDOR_CFLAGS := $(COMMON_CFLAGS) -Wall -Wextra
PROJECT_SOURCES := ../src/main.c ../src/systick.c ../src/stm32f10x_it.c ../src/syscalls.c \
    ../src/platform_init.c ../src/platform_demo.c ../src/comm_link.c ../tests/board_test.c \
    ../bsp/bsp_clk.c ../bsp/bsp_gpio.c ../bsp/bsp_usart.c ../bsp/bsp_timer.c \
    ../../../code/section/$(SECTION_RUNTIME)/section.c \
    ../../../code/data_source/comm/comm.c ../../../code/data_source/comm/comm_v1.c \
    ../../../code/dbg/shell.c ../../../code/dbg/shell_core.c ../../../code/dbg/shell_service.c \
    ../../../code/dbg/perf.c ../../../code/dbg/perf_core.c ../../../code/dbg/section_list_service.c \
    ../../../code/lib/codec_dict.c ../../../code/lib/codec_rle.c \
    ../../../code/lib/codec_lzss.c ../../../code/lib/codec_zero.c ../../../code/lib/record_dict.c
VENDOR_SOURCES := ../Firmware/Libraries/CMSIS/CM3/CoreSupport/core_cm3.c \
    ../Firmware/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x/system_stm32f10x.c \
    $(addprefix ../Firmware/Libraries/STM32F10x_StdPeriph_Driver/src/,stm32f10x_rcc.c \
    stm32f10x_gpio.c stm32f10x_usart.c stm32f10x_dma.c stm32f10x_tim.c misc.c)
PROJECT_OBJECTS := $(addprefix $(BUILD_DIR)/project/,$(notdir $(PROJECT_SOURCES:.c=.o)))
VENDOR_OBJECTS := $(addprefix $(BUILD_DIR)/vendor/,$(notdir $(VENDOR_SOURCES:.c=.o)))
OBJECTS := $(PROJECT_OBJECTS) $(VENDOR_OBJECTS) $(BUILD_DIR)/asm/startup_stm32f10x_md.o
LD_SCRIPT := stm32f103c8t6_flash.ld
LDFLAGS := $(MCU_FLAGS) -specs=nano.specs -T$(LD_SCRIPT) -Wl,--gc-sections \
    -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--print-memory-usage -Wl,--no-warn-execstack
vpath %.c $(sort $(dir $(PROJECT_SOURCES) $(VENDOR_SOURCES)))
.PHONY: all clean
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(LD_SCRIPT) makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -lc -lm -lnosys -o $@
	$(SIZE) $@
$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary -S $< $@
$(BUILD_DIR)/project/%.o: %.c makefile
	@$(call make_dir,$(@D))
	$(CC) $(PROJECT_CFLAGS) -c $< -o $@
$(BUILD_DIR)/project/board_test.o: ../tests/board_test.c makefile
	@$(call make_dir,$(@D))
	$(CC) $(PROJECT_CFLAGS) -DSTM32_BOARD_TEST=$(BOARD_TEST) -c $< -o $@
$(BUILD_DIR)/vendor/%.o: %.c makefile
	@$(call make_dir,$(@D))
	$(CC) $(VENDOR_CFLAGS) -c $< -o $@
$(BUILD_DIR)/asm/startup_stm32f10x_md.o: startup_stm32f10x_md.S
	@$(call make_dir,$(@D))
	$(CC) $(MCU_FLAGS) -x assembler-with-cpp -Wa,--noexecstack -c $< -o $@
ifeq ($(OS),Windows_NT)
define make_dir
if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
endef
clean:
	powershell -NoProfile -Command "$$p=[IO.Path]::GetFullPath('$(BUILD_DIR)'); $$root=[IO.Path]::GetFullPath('..'); if (-not $$p.StartsWith($$root+[IO.Path]::DirectorySeparatorChar)) { throw 'Invalid clean target' }; if (Test-Path -LiteralPath $$p) { Remove-Item -LiteralPath $$p -Recurse -Force }"
else
define make_dir
mkdir -p "$(1)"
endef
clean:
	@test "$(abspath $(dir $(BUILD_DIR)))" = "$(abspath ..)"
	rm -rf "$(BUILD_DIR)"
endif
-include $(OBJECTS:.o=.d)
include $(MAKE_DIR)../../gcc_float_policy.mk
