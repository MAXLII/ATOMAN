# Capture this makefile before generated .d includes change MAKEFILE_LIST.
BASE_FP_MAKE_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

PROJECT_ROOT := ../../..
TARGET := build/test_svpwm_3level.exe
RUNNER_TARGET := build/svpwm_3level_testbench.exe
BALANCE_TARGET := build/test_midpoint_balance.exe
CC := gcc
CXX := g++
WARN_FLAGS := -Wall -Wextra -Werror -Wpedantic \
              -Wconversion -Wsign-conversion -Wshadow -Wcast-align \
              -Wstrict-prototypes -Wmissing-prototypes -Wundef
CFLAGS := -std=c11 -O2 $(WARN_FLAGS) -DPLATFORM_TESTBENCH \
          -I$(PROJECT_ROOT)/code/lib -I$(PROJECT_ROOT)/code/section -I../common
CXXFLAGS := -std=c++14 -O2 -Wall -Wextra -Werror -Wpedantic \
            -Wconversion -Wsign-conversion -Wshadow -Wcast-align -Wundef -I../common
SOURCES := test_svpwm_3level.c $(PROJECT_ROOT)/code/lib/svpwm_3level.c
HEADERS := $(PROJECT_ROOT)/code/lib/svpwm_3level.h $(PROJECT_ROOT)/code/section/platform.h \
           $(PROJECT_ROOT)/code/section/my_math.h $(PROJECT_ROOT)/code/section/timing.h

.PHONY: all test
all: $(TARGET) $(RUNNER_TARGET) $(BALANCE_TARGET)

build:
	powershell.exe -NoProfile -Command "New-Item -ItemType Directory -Force -Path build | Out-Null"

$(TARGET): build/test_svpwm_3level.o build/svpwm_3level.o
	$(CC) $^ -o $@ -lm

build/test_svpwm_3level.o: test_svpwm_3level.c $(HEADERS) | build
	$(CC) $(call testbench_cflags,$<) -c $< -o $@

build/svpwm_3level.o: $(PROJECT_ROOT)/code/lib/svpwm_3level.c $(HEADERS) | build
	$(CC) $(call testbench_cflags,$<) -c $< -o $@

build/svpwm_3level_testbench.o: svpwm_3level_testbench.c $(HEADERS) ../common/testbench.h | build
	$(CC) $(call testbench_cflags,$<) -c $< -o $@

build/testbench.o: ../common/testbench.cpp ../common/testbench.h | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(RUNNER_TARGET): build/svpwm_3level.o build/svpwm_3level_testbench.o build/testbench.o
	$(CXX) $^ -o $@ -lm

build/test_midpoint_balance.o: test_midpoint_balance.c $(HEADERS) | build
	$(CC) $(call testbench_cflags,$<) -c $< -o $@

$(BALANCE_TARGET): build/test_midpoint_balance.o build/svpwm_3level.o
	$(CC) $^ -o $@ -lm

test: $(TARGET) $(RUNNER_TARGET) $(BALANCE_TARGET)
	./$(TARGET)
	./$(RUNNER_TARGET)
	./$(BALANCE_TARGET)

# Test sources may use double; production sources retain single-precision diagnostics.
include $(BASE_FP_MAKE_DIR)../common/float_policy.mk
