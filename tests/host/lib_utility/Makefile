PROJECT_ROOT := ../../..
TARGET := test_lib_utility.exe
CC := C:/mingw64/bin/x86_64-w64-mingw32-gcc.exe

WARN_FLAGS := -Wall -Wextra -Werror -Wpedantic \
              -Wconversion -Wsign-conversion -Wshadow -Wcast-align \
              -Wstrict-prototypes -Wmissing-prototypes -Wundef
CFLAGS := -std=c11 -O2 -g3 $(WARN_FLAGS) -I$(PROJECT_ROOT)/code/lib

SOURCES := test_lib_utility.c \
           $(PROJECT_ROOT)/code/lib/alive_monitor.c \
           $(PROJECT_ROOT)/code/lib/key_event.c \
           $(PROJECT_ROOT)/code/lib/status_bitmap.c

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $@

test: $(TARGET)
	.\$(TARGET)

clean:
	@if exist "$(TARGET)" del /Q "$(TARGET)"
	@if exist "*.o" del /Q "*.o"
