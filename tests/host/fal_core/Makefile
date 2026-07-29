PROJECT_ROOT := ../../..
TARGET := test_fal_core.exe
CC := C:/mingw64/bin/x86_64-w64-mingw32-gcc.exe

WARN_FLAGS := -Wall -Wextra -Werror -Wpedantic \
              -Wconversion -Wsign-conversion -Wshadow -Wcast-align \
              -Wstrict-prototypes -Wmissing-prototypes -Wundef
CFLAGS := -std=c11 -O2 -g3 $(WARN_FLAGS) \
          -I. -I$(PROJECT_ROOT)/code/interface/fal

SOURCES := test_fal_core.c fake_flash.c fake_fal_cfg.c \
           $(PROJECT_ROOT)/code/interface/fal/fal_core.c

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $@

test: $(TARGET)
	powershell.exe -NoProfile -Command "& { .\$(TARGET) 2>&1 | Tee-Object -FilePath test_fal_core.log; exit $$LASTEXITCODE }"

clean:
	@if exist "$(TARGET)" del /Q "$(TARGET)"
	@if exist "*.o" del /Q "*.o"
