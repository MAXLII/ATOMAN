CC := C:/mingw64/bin/x86_64-w64-mingw32-gcc.exe

WARN := -Wall -Wextra -Werror -Wpedantic -Wconversion -Wsign-conversion \
        -Wshadow -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wundef
CFLAGS := -std=c11 $(WARN) -DPLATFORM_TESTBENCH -DSECTION_TESTBENCH_DYNAMIC_TICK \
          -I../../../code/ctrl/chb -I../../../code/app/chb \
          -I../../../code/section/baremetal -I../../../code/section -I../../../code/dbg \
          -I../../../code/lib

SOURCES := test_chb_closed_loop.c \
           ../../../code/ctrl/chb/chb_cfg.c \
           ../../../code/ctrl/chb/chb_hal.c \
           ../../../code/ctrl/chb/chb_fsm.c \
           ../../../code/ctrl/chb/chb_ctrl.c \
           ../../../code/app/chb/chb_protect.c \
           ../../../code/lib/rly_on.c \
           ../../../code/lib/notch.c \
           ../../../code/section/baremetal/section.c
TARGET := test_chb_closed_loop.exe
HEADERS := $(wildcard ../../../code/ctrl/chb/*.h ../../../code/app/chb/*.h)

.PHONY: all run clean
all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) -lm -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	-del /Q $(TARGET) 2>NUL
