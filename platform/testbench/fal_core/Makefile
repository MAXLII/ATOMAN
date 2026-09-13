# Capture this makefile before generated .d includes change MAKEFILE_LIST.
BASE_FP_MAKE_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

PROJECT_ROOT := ../../..
TARGET := test_fal_core.exe
CC := C:/mingw64/bin/x86_64-w64-mingw32-gcc.exe

WARN_FLAGS := -Wall -Wextra -Werror -Wpedantic \
              -Wconversion -Wsign-conversion -Wshadow -Wcast-align \
              -Wstrict-prototypes -Wmissing-prototypes -Wundef
CFLAGS := -std=c11 -O2 -g3 $(WARN_FLAGS) \
          -I. -I$(PROJECT_ROOT)/code/lib/fal

SOURCES := test_fal_core.c fake_flash.c fake_fal_cfg.c \
           $(PROJECT_ROOT)/code/lib/fal/fal_core.c

OBJECTS := $(notdir $(SOURCES:.c=.o))
vpath %.c $(sort $(dir $(SOURCES)))

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $^ -o $@

%.o: %.c
	$(CC) $(call testbench_cflags,$<) -c $< -o $@

test: $(TARGET)
	powershell.exe -NoProfile -Command "& { .\$(TARGET) 2>&1 | Tee-Object -FilePath test_fal_core.log; exit $$LASTEXITCODE }"

clean:
	@if exist "$(TARGET)" del /Q "$(TARGET)"
	@if exist "*.o" del /Q "*.o"

# Test sources may use double; production sources retain single-precision diagnostics.
include $(BASE_FP_MAKE_DIR)../common/float_policy.mk
