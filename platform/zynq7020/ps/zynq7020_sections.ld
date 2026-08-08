/* Zynq-7020 additions to the Xilinx SDK generated Cortex-A9 linker script. */
SECTIONS
{
    .section_registry ALIGN(4) :
    {
        __section_start = .;
        KEEP(*(section))
        KEEP(*(section.*))
        __section_end = .;
    }

    .func_ram ALIGN(4) :
    {
        KEEP(*(.func_ram))
        KEEP(*(.func_ram.*))
    }
} INSERT BEFORE .bss;

ASSERT(_end <= 0x1FF00000,
       "Firmware overlaps reserved PL UART DMA DDR window at 0x1FF00000")
