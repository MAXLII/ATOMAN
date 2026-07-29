PROJECT_ROOT := ../../..
TARGET ?= zynq7020_section_comm
BUILD_DIR ?= build
BSP_ROOT := $(BUILD_DIR)/bsp/ps7_cortexa9_0
BSP_INCLUDE := $(BSP_ROOT)/include
BSP_LIB := $(BSP_ROOT)/lib
LD_SCRIPT := $(BUILD_DIR)/lscript.ld

CC := arm-none-eabi-gcc
CP := arm-none-eabi-objcopy
SZ := arm-none-eabi-size

C_SOURCES := \
./bsp/bsp_platform.c \
./bsp/bsp_iir.c \
./bsp/bsp_interrupt.c \
./bsp/bsp_oled.c \
./bsp/bsp_timer.c \
./bsp/bsp_usart.c \
./src/main.c \
./src/platform_probe.c \
./src/zero_player_oled.c \
./src/zynq_iap_update_service.c \
$(PROJECT_ROOT)/code/section/baremetal/section.c \
$(PROJECT_ROOT)/code/comm/comm.c \
$(PROJECT_ROOT)/code/dbg/shell.c \
$(PROJECT_ROOT)/code/dbg/shell_service.c \
$(PROJECT_ROOT)/code/dbg/perf.c \
$(PROJECT_ROOT)/code/dbg/perf_service.c \
$(PROJECT_ROOT)/code/dbg/scope.c \
$(PROJECT_ROOT)/code/dbg/scope_service.c \
$(PROJECT_ROOT)/code/dbg/sfra.c \
$(PROJECT_ROOT)/code/dbg/sfra_service.c \
$(PROJECT_ROOT)/code/dbg/trace.c \
$(PROJECT_ROOT)/code/dbg/trace_service.c \
$(PROJECT_ROOT)/code/interface/ac/comm_link.c \
$(PROJECT_ROOT)/code/app/zero_player.c \
$(PROJECT_ROOT)/code/app/demo/demo.c \
$(PROJECT_ROOT)/code/app/demo/demo_comm.c \
$(PROJECT_ROOT)/code/app/demo/demo_interrupt.c \
$(PROJECT_ROOT)/code/app/demo/demo_perf.c \
$(PROJECT_ROOT)/code/app/demo/demo_scope.c \
$(PROJECT_ROOT)/code/app/demo/demo_sfra.c \
$(PROJECT_ROOT)/code/app/demo/demo_shell.c \
$(PROJECT_ROOT)/code/app/demo/demo_task.c \
$(PROJECT_ROOT)/code/app/demo/demo_trace.c \
$(PROJECT_ROOT)/code/lib/dft.c \
$(PROJECT_ROOT)/code/lib/pi_tustin.c \
$(PROJECT_ROOT)/code/lib/pr.c \
$(PROJECT_ROOT)/code/lib/record_dict.c \
$(PROJECT_ROOT)/code/lib/sogi.c

ASM_SOURCES :=

C_INCLUDES := \
-I./bsp \
-I./src \
-I$(PROJECT_ROOT)/code/section/baremetal \
-I$(PROJECT_ROOT)/code/section \
-I$(PROJECT_ROOT)/code/comm \
-I$(PROJECT_ROOT)/code/dbg \
-I$(PROJECT_ROOT)/code/interface/ac \
-I$(PROJECT_ROOT)/code/interface/common \
-I$(PROJECT_ROOT)/code/app \
-I$(PROJECT_ROOT)/code/app/demo \
-I$(PROJECT_ROOT)/code/app/bootloader/core \
-I$(PROJECT_ROOT)/code/app/bootloader/protocol \
-I$(PROJECT_ROOT)/code/app/bootloader/common \
-I$(PROJECT_ROOT)/code/app/bootloader/iap \
-I$(PROJECT_ROOT)/code/lib \
-isystem $(BSP_INCLUDE)

C_DEFS := -DIS_ZYNQ7020 -DIS_IAP -DTOOLCHAIN_GCC
CPU_FLAGS := -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard
WARN_FLAGS := -Wall -Wextra -Werror -Wpedantic \
              -Wconversion -Wsign-conversion -Wshadow -Wcast-align \
              -Wstrict-prototypes -Wmissing-prototypes -Wundef
CFLAGS := -std=c11 $(CPU_FLAGS) $(C_DEFS) $(C_INCLUDES) $(WARN_FLAGS) \
          -O2 -g3 -ffunction-sections -fdata-sections -MMD -MP
ASFLAGS := $(CPU_FLAGS) $(C_DEFS) $(C_INCLUDES) \
           -g3 -ffunction-sections -fdata-sections
LDFLAGS := $(CPU_FLAGS) -Wl,-build-id=none -specs=./Xilinx.spec \
           -Wl,-T,$(LD_SCRIPT) -Wl,-T,./zynq7020_sections.ld \
           -L$(BSP_LIB) -Wl,-Map,$(BUILD_DIR)/$(TARGET).map,--cref \
           -Wl,--gc-sections -Wl,--defsym,_SUPERVISOR_STACK_SIZE=0x2000
LIBS := -Wl,--start-group,-lxil,-lgcc,-lc,-lm,--end-group

OBJECTS := $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o))) \
           $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.S=.o)))
DEPS := $(OBJECTS:.o=.d)
vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.S $(sort $(dir $(ASM_SOURCES)))

.PHONY: all clean verify

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin verify

$(BUILD_DIR)/%.o: %.c makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -MF"$(@:.o=.d)" -MT"$@" $< -o $@

$(BUILD_DIR)/%.o: %.S makefile | $(BUILD_DIR)
	$(CC) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(LD_SCRIPT) zynq7020_sections.ld Xilinx.spec
	$(CC) $(OBJECTS) $(LDFLAGS) $(LIBS) -o $@
	$(SZ) $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(CP) -O binary -S $< $@

verify: $(BUILD_DIR)/$(TARGET).elf
	arm-none-eabi-readelf -S $< | findstr /C:"section_registry" /C:"func_ram"
	arm-none-eabi-nm -n $< | findstr /C:"__section_start" /C:"__section_end"

$(BUILD_DIR):
	@if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"

clean:
	@if exist "$(BUILD_DIR)\*.o" del /Q "$(BUILD_DIR)\*.o"
	@if exist "$(BUILD_DIR)\*.d" del /Q "$(BUILD_DIR)\*.d"
	@if exist "$(BUILD_DIR)\$(TARGET).elf" del /Q "$(BUILD_DIR)\$(TARGET).elf"
	@if exist "$(BUILD_DIR)\$(TARGET).bin" del /Q "$(BUILD_DIR)\$(TARGET).bin"
	@if exist "$(BUILD_DIR)\$(TARGET).map" del /Q "$(BUILD_DIR)\$(TARGET).map"

-include $(DEPS)
