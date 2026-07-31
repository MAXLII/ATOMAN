CC := C:/mingw64/bin/x86_64-w64-mingw32-gcc.exe

CFLAGS := -std=c11 -Wall -Wextra -Werror -Wpedantic \
          -Wconversion -Wsign-conversion -Wshadow \
          -I../../../code/lib

TARGET := test_inv_i32.exe
SOURCES := test_inv_i32.c \
           ../../../code/lib/apf_i32.c \
           ../../../code/lib/resonant_i32.c \
           ../../../code/lib/trig_i32.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -lm -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	-del /Q $(TARGET) 2>NUL
