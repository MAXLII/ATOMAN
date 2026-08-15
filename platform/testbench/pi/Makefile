CC := gcc
CXX := g++

BUILD_DIR := build
TARGET := $(BUILD_DIR)/pi_test.exe

# =================== User modification section begin ===================

# Project directories.
COMMON := ../common
CODE := ../../../code
LIB := $(CODE)/lib
DBG := $(CODE)/dbg
SECTION := $(CODE)/section
BAREMETAL := $(SECTION)/baremetal

# C source files.
C_SOURCES := pi_test.c \
             $(LIB)/pi_tustin.c \
             $(LIB)/dft.c \
             $(DBG)/sfra_core.c

# C++ source files.
CXX_SOURCES := $(COMMON)/testbench.cpp

# Header search paths.
INCLUDE_DIRS := $(COMMON) \
                $(LIB) \
                $(DBG) \
                $(SECTION) \
                $(BAREMETAL)

# ==================== User modification section end ====================

CPPFLAGS := $(addprefix -I,$(INCLUDE_DIRS)) -DIS_TESTBENCH
CFLAGS := -std=c11
CXXFLAGS := -std=c++14
LDLIBS := -lm

C_OBJECTS := $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
CXX_OBJECTS := $(addprefix $(BUILD_DIR)/,$(notdir $(CXX_SOURCES:.cpp=.o)))
OBJECTS := $(C_OBJECTS) $(CXX_OBJECTS)

vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.cpp $(sort $(dir $(CXX_SOURCES)))

.PHONY: all test prepare

all: prepare $(TARGET)

prepare: | $(BUILD_DIR)
	powershell.exe -NoProfile -Command \
		"Remove-Item -Path '$(BUILD_DIR)/*.o','$(BUILD_DIR)/*.exe' -Force -ErrorAction Ignore"

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

test: prepare $(TARGET)
	$(TARGET)
