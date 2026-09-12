PROJECT_ROOT := ../../..
TARGET := build/test_svpwm_3level.exe
RUNNER_TARGET := build/svpwm_3level_testbench.exe
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
HEADERS := $(PROJECT_ROOT)/code/lib/svpwm_3level.h $(PROJECT_ROOT)/code/section/platform.h

.PHONY: all test
all: $(TARGET) $(RUNNER_TARGET)

build:
	powershell.exe -NoProfile -Command "New-Item -ItemType Directory -Force -Path build | Out-Null"

$(TARGET): $(SOURCES) $(HEADERS) | build
	$(CC) $(CFLAGS) $(SOURCES) -o $@ -lm

build/svpwm_3level.o: $(PROJECT_ROOT)/code/lib/svpwm_3level.c $(HEADERS) | build
	$(CC) $(CFLAGS) -c $< -o $@

build/svpwm_3level_testbench.o: svpwm_3level_testbench.c $(HEADERS) ../common/testbench.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/testbench.o: ../common/testbench.cpp ../common/testbench.h | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(RUNNER_TARGET): build/svpwm_3level.o build/svpwm_3level_testbench.o build/testbench.o
	$(CXX) $^ -o $@ -lm

test: $(TARGET) $(RUNNER_TARGET)
	./$(TARGET)
	./$(RUNNER_TARGET)
