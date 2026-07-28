PROJECT_ROOT := ../../../..
TARGET := zynq7020_bootloader
BUILD_DIR := build
BSP_ROOT := $(BUILD_DIR)/bsp/ps7_cortexa9_0
BSP_INCLUDE := $(BSP_ROOT)/include
BSP_LIB := $(BSP_ROOT)/lib
LD_SCRIPT := $(BUILD_DIR)/lscript.ld

CC := arm-none-eabi-gcc
CP := arm-none-eabi-objcopy
SZ := arm-none-eabi-size

C_SOURCES := \
../bsp/bsp_platform.c \
../bsp/bsp_interrupt.c \
../bsp/bsp_timer.c \
../bsp/bsp_usart.c \
../bsp/bsp_qspi_flash.c \
main.c \
bootloader_comm_link.c \
bootloader_fal_adapter.c \
zynq_boot_platform.c \
../src/zynq_boot_handoff.c \
fal_cfg.c \
$(PROJECT_ROOT)/code/section/baremetal/section.c \
$(PROJECT_ROOT)/code/comm/comm.c \
$(PROJECT_ROOT)/code/interface/fal/fal_core.c \
$(PROJECT_ROOT)/code/app/bootloader/bootloader_core.c \
$(PROJECT_ROOT)/code/app/bootloader/bootloader_metadata.c \
$(PROJECT_ROOT)/code/app/bootloader/bootloader_protocol.c

C_INCLUDES := \
-I. \
-I../bsp \
-I../src \
-I$(PROJECT_ROOT)/code/section/baremetal \
-I$(PROJECT_ROOT)/code/section \
-I$(PROJECT_ROOT)/code/comm \
-I$(PROJECT_ROOT)/code/dbg \
-I$(PROJECT_ROOT)/code/interface/fal \
-I$(PROJECT_ROOT)/code/interface/common \
-I$(PROJECT_ROOT)/code/app/bootloader \
-isystem $(BSP_INCLUDE)

C_DEFS := -DIS_ZYNQ7020 -DTOOLCHAIN_GCC -DBOOTLOADER_PROTOCOL_FRAME_SERVICE_ENABLE \
          -DHYP_GUEST=0 -DEL1_NONSECURE=0 -DXEN_USE_PV_CONSOLE=0
CPU_FLAGS := -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard
WARN_FLAGS := -Wall -Wextra -Werror -Wpedantic \
              -Wconversion -Wsign-conversion -Wshadow -Wcast-align \
              -Wstrict-prototypes -Wmissing-prototypes -Wundef
CFLAGS := -std=c11 $(CPU_FLAGS) $(C_DEFS) $(C_INCLUDES) $(WARN_FLAGS) \
          -O2 -g3 -ffunction-sections -fdata-sections -MMD -MP
LDFLAGS := $(CPU_FLAGS) -Wl,-build-id=none -specs=../Xilinx.spec \
           -Wl,-T,$(LD_SCRIPT) -Wl,-T,../zynq7020_sections.ld \
           -L$(BSP_LIB) -Wl,-Map,$(BUILD_DIR)/$(TARGET).map,--cref \
           -Wl,--gc-sections -Wl,--defsym,_SUPERVISOR_STACK_SIZE=0x2000
LIBS := -Wl,--start-group,-lxil,-lgcc,-lc,-lm,--end-group

OBJECTS := $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
DEPS := $(OBJECTS:.o=.d)
vpath %.c $(sort $(dir $(C_SOURCES)))

.PHONY: all clean verify

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin verify

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -MF"$(@:.o=.d)" -MT"$@" $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(LD_SCRIPT) ../zynq7020_sections.ld ../Xilinx.spec
	$(CC) $(OBJECTS) $(LDFLAGS) $(LIBS) -o $@
	$(SZ) $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(CP) -O binary -S $< $@

verify: $(BUILD_DIR)/$(TARGET).elf
	arm-none-eabi-readelf -h $< | findstr /C:"Entry point address:               0x4000000"
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
