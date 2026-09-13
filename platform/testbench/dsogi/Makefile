# Capture this makefile before generated .d includes change MAKEFILE_LIST.
BASE_FP_MAKE_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

CC := gcc
CXX := g++
ROOT := ../../..
TARGET := build/dsogi_test.exe
WARN_FLAGS := -Wall -Wextra -Werror -Wpedantic -Wconversion -Wsign-conversion \
              -Wshadow -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wundef
CPPFLAGS := -DPLATFORM_TESTBENCH -I$(ROOT)/code/lib -I$(ROOT)/code/section -I../common
CFLAGS := -std=c11 -O2 $(WARN_FLAGS) -MMD -MP
CXXFLAGS := -std=c++14 -O2 -Wall -Wextra -Werror -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wcast-align -Wundef -MMD -MP
OBJECTS := build/dsogi_test.o build/dsogi.o build/sogi.o build/pll.o build/pi_tustin.o build/testbench.o
vpath %.c $(ROOT)/code/lib
.PHONY: all test
all: $(TARGET)
build:
	powershell.exe -NoProfile -Command "New-Item -ItemType Directory -Force -Path build | Out-Null"
build/%.o: %.c | build
	$(CC) $(CPPFLAGS) $(call testbench_cflags,$<) -c $< -o $@
build/testbench.o: ../common/testbench.cpp | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
$(TARGET): $(OBJECTS)
	$(CXX) $^ -o $@ -lm
test: $(TARGET)
	./$(TARGET)
-include $(OBJECTS:.o=.d)

# Test sources may use double; production sources retain single-precision diagnostics.
include $(BASE_FP_MAKE_DIR)../common/float_policy.mk
