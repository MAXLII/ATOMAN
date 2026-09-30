CC := C:/mingw64/bin/x86_64-w64-mingw32-gcc.exe

CFLAGS := -std=c11 -Wall -Wextra -Werror -Wpedantic \
          -Wconversion -Wsign-conversion -Wshadow \
          -I../../../code/section

TARGET := test_section_list.exe
SOURCES := test_section_list.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	-del /Q $(TARGET) 2>NUL
