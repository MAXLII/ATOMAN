CGT_ROOT ?= C:/ti/ccs2100/ccs/tools/compiler/ti-cgt-c2000_25.11.1.LTS
C2000WARE_ROOT ?= C:/ti/C2000Ware_5_04_00_00

CL2000 := $(CGT_ROOT)/bin/cl2000.exe
PLATFORM_DIR := platform/tms320f28p55
BUILD_DIR := $(PLATFORM_DIR)/build
DRIVERLIB_DIR := $(C2000WARE_ROOT)/driverlib/f28p55x/driverlib
DRIVERLIB_LIB := $(DRIVERLIB_DIR)/ccs/Debug/driverlib.lib
LINKER_CMD := $(PLATFORM_DIR)/linker/f28p55x_ram_lnk.cmd
OUT_FILE := $(BUILD_DIR)/section_task_demo.out
MAP_FILE := $(BUILD_DIR)/section_task_demo.map

INCLUDES := \
	--include_path="$(CGT_ROOT)/include" \
	--include_path="$(PLATFORM_DIR)/inc" \
	--include_path="$(PLATFORM_DIR)/device" \
	--include_path="$(DRIVERLIB_DIR)" \
	--include_path="code/section" \
	--include_path="code/section/baremetal" \
	--include_path="code/data_source/comm" \
	--include_path="code/dbg" \
	--include_path="code/lib" \
	--include_path="code/business/demo"

CFLAGS := -v28 -ml -mt --abi=eabi --c11 --float_support=fpu32 \
	--tmu_support=tmu1 --gen_func_subsections=on --display_error_number \
	--diag_warning=225 --diag_suppress=303 --define=DEBUG --define=RAM \
	--define=REV_ID=1 --define=DEMO_SFRA_ISR_DIVIDER=1 \
	--define=PLATFORM_TMS320F28P55 --define=TOOLCHAIN_TI_C2000 \
	--preinclude=platform.h \
	--define=_LAUNCHXL_F28P55X $(INCLUDES)

OBJECTS := \
	$(BUILD_DIR)/main.obj \
	$(BUILD_DIR)/platform_port.obj \
	$(BUILD_DIR)/comm_link.obj \
	$(BUILD_DIR)/f28p55_shell_adapter.obj \
	$(BUILD_DIR)/frame_parameter_service.obj \
	$(BUILD_DIR)/frame_scope_service.obj \
	$(BUILD_DIR)/frame_perf_service.obj \
	$(BUILD_DIR)/frame_trace_service.obj \
	$(BUILD_DIR)/frame_sfra_service.obj \
	$(BUILD_DIR)/f28p55_perf_counter.obj \
	$(BUILD_DIR)/section_task_demo.obj \
	$(BUILD_DIR)/section.obj \
	$(BUILD_DIR)/comm.obj \
	$(BUILD_DIR)/section_list_service.obj \
	$(BUILD_DIR)/scope_core.obj \
	$(BUILD_DIR)/scope.obj \
	$(BUILD_DIR)/perf_core.obj \
	$(BUILD_DIR)/perf.obj \
	$(BUILD_DIR)/trace_core.obj \
	$(BUILD_DIR)/trace.obj \
	$(BUILD_DIR)/sfra_core.obj \
	$(BUILD_DIR)/sfra.obj \
	$(BUILD_DIR)/record_dict.obj \
	$(BUILD_DIR)/dft.obj \
	$(BUILD_DIR)/pi_tustin.obj \
	$(BUILD_DIR)/demo_shell.obj \
	$(BUILD_DIR)/demo_scope.obj \
	$(BUILD_DIR)/demo_perf.obj \
	$(BUILD_DIR)/demo_trace.obj \
	$(BUILD_DIR)/demo_sfra.obj \
	$(BUILD_DIR)/device.obj \
	$(BUILD_DIR)/f28p55x_codestartbranch.obj

.PHONY: all clean

all: $(OUT_FILE)

$(BUILD_DIR):
	@if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"

$(BUILD_DIR)/main.obj: $(PLATFORM_DIR)/src/main.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/platform_port.obj: $(PLATFORM_DIR)/src/platform_port.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/comm_link.obj: $(PLATFORM_DIR)/src/comm_link.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/f28p55_shell_adapter.obj: $(PLATFORM_DIR)/src/f28p55_shell_adapter.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/frame_parameter_service.obj: $(PLATFORM_DIR)/src/frame_parameter_service.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/frame_scope_service.obj: $(PLATFORM_DIR)/src/frame_scope_service.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/frame_perf_service.obj: $(PLATFORM_DIR)/src/frame_perf_service.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/frame_trace_service.obj: $(PLATFORM_DIR)/src/frame_trace_service.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/frame_sfra_service.obj: $(PLATFORM_DIR)/src/frame_sfra_service.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/f28p55_perf_counter.obj: $(PLATFORM_DIR)/src/f28p55_perf_counter.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/section_task_demo.obj: $(PLATFORM_DIR)/src/section_task_demo.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/section.obj: code/section/baremetal/section.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/comm.obj: code/data_source/comm/comm.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/section_list_service.obj: code/dbg/section_list_service.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/scope_core.obj: code/dbg/scope_core.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/scope.obj: code/dbg/scope.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/perf_core.obj: code/dbg/perf_core.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/perf.obj: code/dbg/perf.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/trace_core.obj: code/dbg/trace_core.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/trace.obj: code/dbg/trace.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/sfra_core.obj: code/dbg/sfra_core.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/sfra.obj: code/dbg/sfra.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/record_dict.obj: code/lib/record_dict.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/dft.obj: code/lib/dft.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/pi_tustin.obj: code/lib/pi_tustin.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/demo_shell.obj: code/business/demo/demo_shell.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/demo_scope.obj: code/business/demo/demo_scope.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/demo_perf.obj: code/business/demo/demo_perf.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/demo_trace.obj: code/business/demo/demo_trace.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/demo_sfra.obj: code/business/demo/demo_sfra.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/device.obj: $(PLATFORM_DIR)/device/device.c | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(BUILD_DIR)/f28p55x_codestartbranch.obj: $(PLATFORM_DIR)/device/f28p55x_codestartbranch.asm | $(BUILD_DIR)
	"$(CL2000)" $(CFLAGS) --compile_only --output_file="$@" "$<"

$(OUT_FILE): $(OBJECTS) $(LINKER_CMD)
	"$(CL2000)" -v28 -ml -mt --abi=eabi --float_support=fpu32 --tmu_support=tmu1 \
		--run_linker $(OBJECTS) "$(DRIVERLIB_LIB)" "$(LINKER_CMD)" \
		--search_path="$(CGT_ROOT)/lib" --library=rts2800_fpu32_eabi.lib \
		--entry_point=code_start --stack_size=0x3F8 --heap_size=0x200 \
		--warn_sections --reread_libs --rom_model \
		--map_file="$(MAP_FILE)" --output_file="$(OUT_FILE)"

clean:
	@if exist "$(BUILD_DIR)" rmdir /s /q "$(BUILD_DIR)"
