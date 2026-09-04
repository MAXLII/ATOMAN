ROOT := ../../..
CC := C:/mingw64/bin/x86_64-w64-mingw32-gcc.exe
TARGET := test_demo_storage.exe
WARN := -Wall -Wextra -Werror -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wundef
INCLUDES := -I. -I$(ROOT)/code/lib -I$(ROOT)/code/data_pool/demo -I$(ROOT)/code/data_source/demo -I$(ROOT)/code/lib/fal -I$(ROOT)/code/interface/demo/common -I$(ROOT)/platform/gd32e507/bsp/inc
SOURCES := test_demo_storage.c fake_flash.c $(ROOT)/code/lib/bch4.c $(ROOT)/code/lib/flash_integrity.c $(ROOT)/code/data_source/demo/demo_storage_record.c $(ROOT)/code/data_source/demo/demo_storage_service.c $(ROOT)/code/data_pool/demo/demo_storage_pool.c $(ROOT)/code/lib/fal/fal_core.c $(ROOT)/code/interface/demo/common/flash_port.c
CFLAGS := -std=c11 -O2 -g $(WARN) $(INCLUDES) -I$(ROOT)/code/business/demo
SOURCES += $(ROOT)/code/business/demo/fal_cfg.c
CFG_SOURCES := test_fal_cfg.c fake_flash.c $(ROOT)/code/business/demo/fal_cfg.c $(ROOT)/code/lib/fal/fal_core.c $(ROOT)/code/interface/demo/common/flash_port.c
.PHONY: all test
all: $(TARGET) test_fal_cfg.exe
$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $@
test_fal_cfg.exe: $(CFG_SOURCES)
	$(CC) $(CFLAGS) $(CFG_SOURCES) -o $@
test: $(TARGET) test_fal_cfg.exe
	powershell.exe -NoProfile -Command "& { .\test_fal_cfg.exe 2>&1 | Tee-Object -FilePath test_fal_cfg.log; exit $$LASTEXITCODE }"
	powershell.exe -NoProfile -Command "& { .\$(TARGET) 2>&1 | Tee-Object -FilePath test_demo_storage.log; exit $$LASTEXITCODE }"
