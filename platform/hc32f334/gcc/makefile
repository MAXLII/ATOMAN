PROJECT_MCU = HC32F334
PROJECT_TOOLCHAIN = GCC
PROJECT_MAKEFILE := $(lastword $(MAKEFILE_LIST))
BUILD_TARGET ?= isp
IS_IAP = 0

ifeq ($(BUILD_TARGET),iap)
IS_IAP = 1
LD_SCRIPT = ./HC32F334xA.ld
else ifeq ($(BUILD_TARGET),isp)
LD_SCRIPT = ./HC32F334_isp.ld
else
$(error Unsupported BUILD_TARGET '$(BUILD_TARGET)'; use isp or iap)
endif

ifeq ($(IS_IAP),1)
PROJECT_TARGET_DEFINES = -DIS_IAP
IAP_SOURCES = ../../../code/app/bootloader/iap_update_service.c
else
PROJECT_TARGET_DEFINES = -DFIRMWARE_TYPE_ISP
IAP_SOURCES =
endif

PROJECT_DEFINES = -D$(PROJECT_MCU) \
                  -DIS_$(PROJECT_MCU) \
                  -DTOOLCHAIN_$(PROJECT_TOOLCHAIN) \
                  -DUSE_DDL_DRIVER \
                  -DIS_AC \
                  $(PROJECT_TARGET_DEFINES)

TARGET = hc32f334_ac_$(BUILD_TARGET)
DEBUG ?= 1
OPT ?= -O2
BUILD_ROOT ?= build
BUILD_DIR = $(BUILD_ROOT)/$(BUILD_TARGET)
HOST_CC ?= gcc
FW_INFO_SOURCE = ../../gd32g553c/tools/fw_info/fw_info.c
FW_INFO_HEADER = ../../gd32g553c/tools/fw_info/fw_info.h
FW_INFO_TOOL = $(BUILD_DIR)/hc32_fw_info.exe

PROJECT_SOURCES = \
../src/main.c \
../bsp/bsp_clk.c \
../bsp/bsp_gpio.c \
../bsp/bsp_pwm.c \
../bsp/bsp_usart.c \
../bsp/bsp_can.c \
../bsp/bsp_timer.c \
../src/hc32f334_it.c \
../src/syscalls.c \
../src/systick.c \
../../../code/app/demo/demo.c \
../../../code/app/demo/demo_comm.c \
../../../code/app/demo/demo_interrupt.c \
../../../code/app/demo/demo_perf.c \
../../../code/app/demo/demo_scope.c \
../../../code/app/demo/demo_sfra.c \
../../../code/app/demo/demo_shell.c \
../../../code/app/demo/demo_task.c \
../../../code/app/demo/demo_trace.c \
../../../code/interface/ac/gpio.c \
../../../code/interface/ac/pwm.c \
../../../code/interface/ac/comm_link.c \
../../../code/section/srtos_m/section.c \
../../../code/comm/comm.c \
../../../code/lib/record_dict.c \
../../../code/lib/pi_tustin.c \
../../../code/lib/dft.c \
../../../code/dbg/perf.c \
../../../code/dbg/perf_service.c \
../../../code/dbg/scope.c \
../../../code/dbg/scope_service.c \
../../../code/dbg/shell.c \
../../../code/dbg/shell_service.c \
../../../code/dbg/sfra.c \
../../../code/dbg/sfra_service.c \
../../../code/dbg/trace.c \
../../../code/dbg/trace_service.c

PROJECT_SOURCES += $(IAP_SOURCES)

VENDOR_SOURCES = \
../Firmware/CMSIS/HDSC/hc32f3xx/Source/system_hc32f334.c \
$(wildcard ../Firmware/HC32_LL_Driver/src/*.c)

ASM_SOURCES = startup_hc32f334.S

PREFIX = arm-none-eabi-
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
endif

HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

MCU_FLAGS = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

C_INCLUDES = \
-I../src \
-I../bsp \
-I../inc \
-I../../src \
-I../Firmware/CMSIS/Include \
-I../Firmware/CMSIS/HDSC/hc32f3xx/Include \
-I../Firmware/HC32_LL_Driver/inc \
-I../../../code/app \
-I../../../code/app/demo \
-I../../../code/app/bootloader \
-I../../../code/section/srtos_m \
-I../../../code/section \
-I../../../code/comm \
-I../../../code/interface \
-I../../../code/interface/ac \
-I../../../code/interface/common \
-I../../../code/lib \
-I../../../code/dbg

COMMON_FLAGS = $(MCU_FLAGS) $(PROJECT_DEFINES) $(C_INCLUDES) $(OPT) \
               -std=c11 -ffunction-sections -fdata-sections
PROJECT_WARN_FLAGS = -Wall -Wextra -Werror -Wpedantic \
                     -Wdouble-promotion -Werror=double-promotion
VENDOR_WARN_FLAGS = -Wall -Wextra -Wno-unused-parameter

ifeq ($(DEBUG),1)
COMMON_FLAGS += -g3 -gdwarf-2
endif

PROJECT_CFLAGS = $(COMMON_FLAGS) $(PROJECT_WARN_FLAGS)
VENDOR_CFLAGS = $(COMMON_FLAGS) $(VENDOR_WARN_FLAGS)
ASFLAGS = $(MCU_FLAGS) $(PROJECT_DEFINES) -ffunction-sections -fdata-sections

LIBS = -lc -lm -lnosys
LDFLAGS = $(MCU_FLAGS) -specs=nano.specs -T$(LD_SCRIPT) $(LIBS) \
          -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref,--gc-sections \
          -Wl,--print-memory-usage,--no-warn-rwx-segments,-z,noexecstack \
          -u _printf_float -u _scanf_float -u _printf_precision

PROJECT_OBJECTS = $(addprefix $(BUILD_DIR)/project/,$(notdir $(PROJECT_SOURCES:.c=.o)))
VENDOR_OBJECTS = $(addprefix $(BUILD_DIR)/vendor/,$(notdir $(VENDOR_SOURCES:.c=.o)))
ASM_OBJECTS = $(addprefix $(BUILD_DIR)/startup/,$(notdir $(ASM_SOURCES:.S=.o)))
OBJECTS = $(PROJECT_OBJECTS) $(VENDOR_OBJECTS) $(ASM_OBJECTS)

vpath %.c $(sort $(dir $(PROJECT_SOURCES)))
vpath %.c $(sort $(dir $(VENDOR_SOURCES)))
vpath %.S $(sort $(dir $(ASM_SOURCES)))

TARGET_OUTPUTS = $(BUILD_DIR)/$(TARGET).elf \
                 $(BUILD_DIR)/$(TARGET).hex \
                 $(BUILD_DIR)/$(TARGET).bin
ifeq ($(BUILD_TARGET),iap)
TARGET_OUTPUTS += $(BUILD_DIR)/hc32f334_ac_update_iap.bin
endif

all: $(TARGET_OUTPUTS)

# The vendor DDL assert macro compiles out its only reads of these local values.
$(BUILD_DIR)/project/bsp_usart.o: PROJECT_CFLAGS += -Wno-unused-but-set-variable
# Registration macros in this integration unit include their own terminators.
$(BUILD_DIR)/project/comm_link.o: PROJECT_CFLAGS += -Wno-pedantic

$(BUILD_DIR)/project/%.o: %.c $(PROJECT_MAKEFILE) | $(BUILD_DIR)/project
	$(CC) -c $(PROJECT_CFLAGS) -MMD -MP -MF $(@:.o=.d) $< -o $@

$(BUILD_DIR)/vendor/%.o: %.c $(PROJECT_MAKEFILE) | $(BUILD_DIR)/vendor
	$(CC) -c $(VENDOR_CFLAGS) -MMD -MP -MF $(@:.o=.d) $< -o $@

$(BUILD_DIR)/startup/%.o: %.S $(PROJECT_MAKEFILE) | $(BUILD_DIR)/startup
	$(AS) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(LD_SCRIPT) | $(BUILD_DIR)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$(BIN) $< $@

$(FW_INFO_TOOL): $(FW_INFO_SOURCE) $(FW_INFO_HEADER) | $(BUILD_DIR)
	$(HOST_CC) -std=gnu11 -DIS_LLC -DIS_IAP $< -o $@

$(BUILD_DIR)/hc32f334_ac_update_iap.bin: $(BUILD_DIR)/$(TARGET).bin $(FW_INFO_TOOL)
	$(FW_INFO_TOOL) $< $@

$(BUILD_DIR) $(BUILD_DIR)/project $(BUILD_DIR)/vendor $(BUILD_DIR)/startup:
ifeq ($(OS),Windows_NT)
	@if not exist "$(subst /,\,$@)" mkdir "$(subst /,\,$@)"
else
	@mkdir -p "$@"
endif

clean:
ifeq ($(OS),Windows_NT)
	@if exist "$(subst /,\,$(BUILD_DIR))" rmdir /s /q "$(subst /,\,$(BUILD_DIR))"
else
	@rm -rf "$(BUILD_DIR)"
endif

-include $(wildcard $(BUILD_DIR)/project/*.d)
-include $(wildcard $(BUILD_DIR)/vendor/*.d)

.PHONY: all clean
