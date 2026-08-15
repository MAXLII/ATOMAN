CC := gcc
CXX := g++

COMMON_WARN_FLAGS := -Wall -Wextra -Werror -Wpedantic \
                     -Wconversion -Wsign-conversion \
                     -Wshadow -Wcast-align -Wundef
C_WARN_FLAGS := $(COMMON_WARN_FLAGS) \
                -Wstrict-prototypes -Wmissing-prototypes
CXX_WARN_FLAGS := $(COMMON_WARN_FLAGS) \
                  -Wold-style-cast

INCLUDE_FLAGS := -I../common \
                 -I../../../code/lib \
                 -I../../../code/section
CFLAGS := -std=c11 $(C_WARN_FLAGS) -DIS_TESTBENCH $(INCLUDE_FLAGS)
CXXFLAGS := -std=c++14 $(CXX_WARN_FLAGS) -fno-exceptions -fno-rtti \
            -DIS_TESTBENCH $(INCLUDE_FLAGS)

BUILD_DIR := build
TARGET := $(BUILD_DIR)/ac_loss_det_test.exe
TEST_LOG := $(BUILD_DIR)/ac_loss_det_test.log
OBJECTS := $(BUILD_DIR)/ac_loss_det_test.o \
           $(BUILD_DIR)/testbench.o \
           $(BUILD_DIR)/ac_loss_det.o

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -lm -o $(TARGET)

$(BUILD_DIR)/ac_loss_det_test.o: ac_loss_det_test.c ac_loss_det_test.h ../common/testbench.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c ac_loss_det_test.c -o $@

$(BUILD_DIR)/testbench.o: ../common/testbench.cpp ../common/testbench.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c ../common/testbench.cpp -o $@

$(BUILD_DIR)/ac_loss_det.o: ../../../code/lib/ac_loss_det.c \
                            ../../../code/lib/ac_loss_det.h \
                            ../../../code/section/my_math.h \
                            ../../../code/section/timing.h \
                            ../../../code/section/platform.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c ../../../code/lib/ac_loss_det.c -o $@

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

test: $(TARGET)
	powershell.exe -NoProfile -Command \
		"& './$(TARGET)' 2>&1 | Tee-Object -FilePath '$(TEST_LOG)'; exit $$LASTEXITCODE"

clean:
	$(RM) -r $(BUILD_DIR)
