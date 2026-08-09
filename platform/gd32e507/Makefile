TARGET := gd32e507_demo
BUILD_DIR := build

PREFIX ?= arm-none-eabi-
CC := $(PREFIX)gcc
AS := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
SIZE := $(PREFIX)size

MCU_FLAGS := -mcpu=cortex-m33 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard
PROJECT_DEFINES := -DGD32E50X -DGD32E50X_CL -DUSE_STDPERIPH_DRIVER \
    -DIS_GD32E507 -DTOOLCHAIN_GCC \
    -DENET_RXBUF_NUM=12 -DENET_TXBUF_NUM=4

INCLUDES := \
    -I../../code/app \
    -I../../code/app/demo \
    -I../../code/lib \
    -I../../code/dbg \
    -I../../code/interface/ac \
    -I../../code/interface/common \
    -I../../code/interface \
    -I../../code/section/srtos_m \
    -I../../code/section \
    -I../../code/comm \
    -I./Firmware/CMSIS \
    -I./Firmware/CMSIS/GD/GD32E50x/Include \
    -I./Firmware/GD32E50x_standard_peripheral/Include \
    -I./inc \
    -I./bsp/inc \
    -I./enet/inc \
    -I./third_party/lwip-2.2.1/src/include \
    -I./third_party/lwip-2.2.1/port/GD32E50x \
    -I./third_party/lwip-2.2.1/port/GD32E50x/Basic

COMMON_CFLAGS := $(MCU_FLAGS) $(PROJECT_DEFINES) $(INCLUDES) \
    -std=c11 -ffunction-sections -fdata-sections -Wa,--noexecstack -MMD -MP
PROJECT_WARN_FLAGS := -Wall -Wextra -Werror -Wpedantic \
    -Wconversion -Wsign-conversion -Wshadow -Wcast-align \
    -Wstrict-prototypes -Wmissing-prototypes -Wundef \
    -Werror=double-promotion -Wdouble-promotion
VENDOR_WARN_FLAGS := -Wall -Wextra
PROJECT_CFLAGS := $(COMMON_CFLAGS) $(PROJECT_WARN_FLAGS) -O2 -g3
VENDOR_CFLAGS := $(COMMON_CFLAGS) $(VENDOR_WARN_FLAGS) -O2 -g3
ASFLAGS := $(MCU_FLAGS) $(PROJECT_DEFINES) -x assembler-with-cpp -Wa,--noexecstack

LD_SCRIPT := ldscripts/gd32e507ze_flash.ld
LDFLAGS := $(MCU_FLAGS) -specs=nano.specs -T$(LD_SCRIPT) \
    -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref \
    -Wl,--print-memory-usage -Wl,--no-warn-execstack \
    -u _printf_float -u _scanf_float \
    -u _printf_precision
LIBS := -lc -lm -lnosys

PROJECT_SOURCES := \
    src/main.c \
    src/systick.c \
    src/gd32e50x_it.c \
    src/syscalls.c \
    bsp/src/bsp_gpio.c \
    bsp/src/bsp_timer.c \
    bsp/src/bsp_usart.c \
    bsp/src/bsp_enet.c \
    enet/src/enet_comm.c \
    ../../code/app/demo/demo.c \
    ../../code/app/demo/demo_comm.c \
    ../../code/app/demo/demo_interrupt.c \
    ../../code/app/demo/demo_jitter.c \
    ../../code/app/demo/demo_rtos_risk.c \
    ../../code/app/demo/demo_perf.c \
    ../../code/app/demo/demo_scope.c \
    ../../code/app/demo/demo_sfra.c \
    ../../code/app/demo/demo_shell.c \
    ../../code/app/demo/demo_task.c \
    ../../code/app/demo/demo_trace.c \
    ../../code/lib/dft.c \
    ../../code/lib/pi_tustin.c \
    ../../code/lib/pr.c \
    ../../code/lib/record_dict.c \
    ../../code/lib/sogi.c \
    ../../code/dbg/perf.c \
    ../../code/dbg/perf_core.c \
    ../../code/dbg/perf_service.c \
    ../../code/dbg/scope.c \
    ../../code/dbg/scope_core.c \
    ../../code/dbg/scope_service.c \
    ../../code/dbg/sfra.c \
    ../../code/dbg/sfra_core.c \
    ../../code/dbg/sfra_service.c \
    ../../code/dbg/trace.c \
    ../../code/dbg/trace_core.c \
    ../../code/dbg/trace_service.c \
    ../../code/dbg/shell.c \
    ../../code/dbg/shell_core.c \
    ../../code/dbg/shell_service.c \
    ../../code/dbg/section_list_service.c \
    ../../code/interface/ac/gpio.c \
    ../../code/interface/ac/comm_link.c \
    ../../code/section/srtos_m/section.c \
    ../../code/comm/comm.c

LWIP_SOURCES := \
    $(wildcard third_party/lwip-2.2.1/src/core/*.c) \
    $(wildcard third_party/lwip-2.2.1/src/core/ipv4/*.c) \
    third_party/lwip-2.2.1/src/netif/ethernet.c \
    third_party/lwip-2.2.1/port/GD32E50x/Basic/ethernetif.c

VENDOR_SOURCES := \
    Firmware/CMSIS/GD/GD32E50x/Source/system_gd32e50x.c \
    $(filter-out %/gd32e50x_sdio.c,$(wildcard Firmware/GD32E50x_standard_peripheral/Source/*.c)) \
    $(LWIP_SOURCES)
ASM_SOURCES := gcc_startup/startup_gd32e507.S

PROJECT_OBJECTS := $(addprefix $(BUILD_DIR)/project/,$(notdir $(PROJECT_SOURCES:.c=.o)))
VENDOR_OBJECTS := $(addprefix $(BUILD_DIR)/vendor/,$(notdir $(VENDOR_SOURCES:.c=.o)))
ASM_OBJECTS := $(addprefix $(BUILD_DIR)/asm/,$(notdir $(ASM_SOURCES:.S=.o)))
OBJECTS := $(PROJECT_OBJECTS) $(VENDOR_OBJECTS) $(ASM_OBJECTS)
DEPS := $(OBJECTS:.o=.d)

vpath %.c $(sort $(dir $(PROJECT_SOURCES)))
vpath %.c $(sort $(dir $(VENDOR_SOURCES)))
vpath %.S $(sort $(dir $(ASM_SOURCES)))

.PHONY: all clean

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(LD_SCRIPT)
	$(CC) $(OBJECTS) $(LDFLAGS) $(LIBS) -o $@
	$(SIZE) $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary -S $< $@

$(BUILD_DIR)/project/%.o: %.c
	@$(call make_dir,$(@D))
	$(CC) -c $(PROJECT_CFLAGS) $< -o $@

$(BUILD_DIR)/vendor/gd32e50x_can.o: Firmware/GD32E50x_standard_peripheral/Source/gd32e50x_can.c
	@$(call make_dir,$(@D))
	$(CC) -c $(VENDOR_CFLAGS) -Wno-unused-variable $< -o $@

$(BUILD_DIR)/vendor/%.o: %.c
	@$(call make_dir,$(@D))
	$(CC) -c $(VENDOR_CFLAGS) $< -o $@

$(BUILD_DIR)/asm/%.o: %.S
	@$(call make_dir,$(@D))
	$(AS) -c $(ASFLAGS) $< -o $@

ifeq ($(OS),Windows_NT)
define make_dir
if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
endef
clean:
	@if exist "$(BUILD_DIR)" rmdir /s /q "$(BUILD_DIR)"
else
define make_dir
mkdir -p "$(1)"
endef
clean:
	rm -rf "$(BUILD_DIR)"
endif

-include $(DEPS)
