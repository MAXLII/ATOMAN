##########################################################################################################################
# APM32F402 demo firmware makefile
##########################################################################################################################

PROJECT_FW_TYPE = ISP
PROJECT_MCU = APM32F402
PROJECT_DEVICE_FAMILY = APM32F402xx
PROJECT_TOOLCHAIN = GCC
LD_SCRIPT = ./ldscripts/apm32f402_403xb_flash.ld

PROJECT_DEFINES = -D$(PROJECT_DEVICE_FAMILY) \
                  -DIS_$(PROJECT_MCU) \
                  -DTOOLCHAIN_$(PROJECT_TOOLCHAIN) \
                  -DFIRMWARE_TYPE_$(PROJECT_FW_TYPE)

TARGET = demo
DEBUG = 1
OPT = -O2
BUILD_DIR = build
GENERATE_LISTING ?= 0

ifeq ($(OS),Windows_NT)
MKDIR_P = if not exist "$(subst /,\,$@)" mkdir "$(subst /,\,$@)"
else
MKDIR_P = mkdir -p "$@"
endif

C_SOURCES = \
./Libraries/APM32F402_403_StdPeriphDriver/src/apm32f402_403_dma.c \
./Libraries/APM32F402_403_StdPeriphDriver/src/apm32f402_403_gpio.c \
./Libraries/APM32F402_403_StdPeriphDriver/src/apm32f402_403_misc.c \
./Libraries/APM32F402_403_StdPeriphDriver/src/apm32f402_403_rcm.c \
./Libraries/APM32F402_403_StdPeriphDriver/src/apm32f402_403_usart.c \
./Libraries/Device/Geehy/APM32F4xx/Source/system_apm32f402_403.c \
./bsp/src/bsp_can.c \
./bsp/src/bsp_gpio.c \
./bsp/src/bsp_usart.c \
./src/apm32f402_403_int.c \
./src/main.c \
./src/syscalls.c \
../../code/app/demo/demo.c \
../../code/app/demo/demo_comm.c \
../../code/app/demo/demo_interrupt.c \
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
../../code/dbg/perf_service.c \
../../code/dbg/scope.c \
../../code/dbg/scope_service.c \
../../code/dbg/sfra.c \
../../code/dbg/sfra_service.c \
../../code/dbg/trace.c \
../../code/dbg/trace_service.c \
../../code/dbg/shell.c \
../../code/dbg/shell_service.c \
../../code/interface/ac/gpio.c \
../../code/interface/ac/comm_link.c \
../../code/section/section.c \
../../code/comm/comm.c

ASM_SOURCES = \
./gcc_startup/startup_apm32f402_403.s

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

CPU = -mcpu=cortex-m4 -mthumb
FPU = -mfpu=fpv4-sp-d16
FLOAT-ABI = -mfloat-abi=hard
MCU = $(CPU) $(FPU) $(FLOAT-ABI)

AS_DEFS =
C_DEFS = $(PROJECT_DEFINES)
AS_INCLUDES =
C_INCLUDES = \
-I../../code/app \
-I../../code/app/demo \
-I../../code/lib \
-I../../code/dbg \
-I../../code/interface/ac \
-I../../code/interface \
-I../../code/interface/common \
-I../../code/section \
-I../../code/comm \
-I./Libraries/CMSIS/Include \
-I./Libraries/Device/Geehy/APM32F4xx/Include \
-I./Libraries/APM32F402_403_StdPeriphDriver/inc \
-I./src \
-I./bsp/inc

C_STANDARD_FLAGS = -std=c11
COMMON_SECTION_FLAGS = -fdata-sections -ffunction-sections
COMMON_WARN_FLAGS = -Wall -Wextra -Werror
FP_WARN_FLAGS = -Werror=double-promotion -Wdouble-promotion
WARN_FLAGS = $(COMMON_WARN_FLAGS) $(FP_WARN_FLAGS)
AS_WARN_FLAGS = $(COMMON_WARN_FLAGS)

ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) $(AS_WARN_FLAGS) $(COMMON_SECTION_FLAGS)
CFLAGS += $(C_STANDARD_FLAGS)
CFLAGS += $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) $(WARN_FLAGS) $(COMMON_SECTION_FLAGS)

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif

CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

ifeq ($(GENERATE_LISTING),1)
C_LISTING_FLAG = -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst))
else
C_LISTING_FLAG =
endif

LDSCRIPT = $(LD_SCRIPT)
LIBS = -lc -lm -lnosys
LIBDIR =
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections
LDFLAGS += -u _printf_float -u _scanf_float -u _printf_precision

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c $(lastword $(MAKEFILE_LIST)) | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $(C_LISTING_FLAG) $< -o $@

$(BUILD_DIR)/%.o: %.s $(lastword $(MAKEFILE_LIST)) | $(BUILD_DIR)
	$(AS) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(lastword $(MAKEFILE_LIST))
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

$(BUILD_DIR):
	@$(MKDIR_P)

clean:
	-rm -fR $(BUILD_DIR)

-include $(wildcard $(BUILD_DIR)/*.d)
