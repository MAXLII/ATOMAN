# Capture this makefile before generated .d includes change MAKEFILE_LIST.
BASE_FP_MAKE_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

PROJECT_ROOT := ../../..
TARGET := test_comm_v1.exe
CC := C:/mingw64/bin/x86_64-w64-mingw32-gcc.exe

WARN_FLAGS := -Wall -Wextra -Werror -Wpedantic \
              -Wconversion -Wsign-conversion -Wshadow -Wcast-align \
              -Wstrict-prototypes -Wmissing-prototypes -Wundef
CFLAGS := -std=c11 -O2 -g3 $(WARN_FLAGS) -DPLATFORM_TESTBENCH \
          -I. -I$(PROJECT_ROOT)/code/lib \
          -I$(PROJECT_ROOT)/code/data_source/comm \
          -I$(PROJECT_ROOT)/code/section \
          -I$(PROJECT_ROOT)/code/section/baremetal \
          -I$(PROJECT_ROOT)/code/dbg

SOURCES := test_comm_v1.c \
           $(PROJECT_ROOT)/code/lib/codec_rle.c \
           $(PROJECT_ROOT)/code/lib/codec_dict.c \
           $(PROJECT_ROOT)/code/lib/codec_lzss.c \
           $(PROJECT_ROOT)/code/lib/codec_zero.c \
           $(PROJECT_ROOT)/code/data_source/comm/comm_v1.c

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $@

test: $(TARGET)
	.\$(TARGET)

clean:
	@if exist "$(TARGET)" del /Q "$(TARGET)"
	@if exist "*.o" del /Q "*.o"

# Test sources may use double; production sources retain single-precision diagnostics.
include $(BASE_FP_MAKE_DIR)../common/float_policy.mk
