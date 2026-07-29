PROJECT_ROOT := ../../..
TARGET := test_bootloader_core.exe
CC := C:/mingw64/bin/x86_64-w64-mingw32-gcc.exe

WARN_FLAGS := -Wall -Wextra -Werror -Wpedantic \
              -Wconversion -Wsign-conversion -Wshadow -Wcast-align \
              -Wstrict-prototypes -Wmissing-prototypes -Wundef
CFLAGS := -std=c11 -O2 -g3 $(WARN_FLAGS) \
          -I$(PROJECT_ROOT)/code/app/bootloader/core \
          -I$(PROJECT_ROOT)/code/app/bootloader/protocol \
          -I$(PROJECT_ROOT)/code/app/bootloader/common \
          -I$(PROJECT_ROOT)/code/interface/fal

SOURCES := test_bootloader_core.c \
           $(PROJECT_ROOT)/code/app/bootloader/core/bootloader_core.c \
           $(PROJECT_ROOT)/code/app/bootloader/protocol/bootloader_protocol.c \
           $(PROJECT_ROOT)/code/app/bootloader/core/bootloader_metadata.c

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $@

test: $(TARGET)
	powershell.exe -NoProfile -Command "& { .\$(TARGET) 2>&1 | Tee-Object -FilePath test_bootloader_core.log; exit $$LASTEXITCODE }"

clean:
	@if exist "$(TARGET)" del /Q "$(TARGET)"
	@if exist "*.o" del /Q "*.o"
