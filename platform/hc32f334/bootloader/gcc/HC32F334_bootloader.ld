ENTRY(Reset_Handler)

HC32F334_IAP_BASE = 0x00004000;
_Min_Heap_Size = 0;
_Min_Stack_Size = 0x0800;

MEMORY
{
    FLASH       (rx)  : ORIGIN = 0x00000000, LENGTH = HC32F334_IAP_BASE
    RAM_NOINIT  (rwx) : ORIGIN = 0x1FFFC000, LENGTH = 16
    RAM         (rwx) : ORIGIN = 0x1FFFC010, LENGTH = 0x7FF0
    RAMB        (rwx) : ORIGIN = 0x200F0000, LENGTH = 4K
}

SECTIONS
{
    .vectors :
    {
        . = ALIGN(4);
        KEEP(*(.vectors))
        . = ALIGN(4);
    } > FLASH

    .icg_sec 0x00000400 :
    {
        KEEP(*(.icg_sec))
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text .text*)
        *(.glue_7 .glue_7t)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata .rodata*)
        . = ALIGN(4);
    } > FLASH

    .section_registry :
    {
        . = ALIGN(4);
        __section_start = .;
        KEEP(*(section))
        __section_end = .;
        . = ALIGN(4);
    } > FLASH

    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
    __exidx_start = .;
    .ARM.exidx : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) } > FLASH
    __exidx_end = .;

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH
    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH
    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    .data :
    {
        . = ALIGN(4);
        __data_start__ = .;
        *(.data .data*)
        *(.ramfunc .ramfunc*)
        *(RAMCODE)
        *(.func_ram .func_ram*)
        . = ALIGN(4);
        __data_end__ = .;
    } > RAM AT > FLASH
    __data_load__ = LOADADDR(.data);

    .noinit (NOLOAD) :
    {
        . = ALIGN(4);
        *(.noinit .noinit*)
        *(.bss.noinit .bss.noinit*)
        . = ALIGN(4);
    } > RAM_NOINIT

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss .bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > RAM

    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        __HeapBase = .;
        . += _Min_Heap_Size;
        __HeapLimit = .;
    } > RAM

    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        __StackLimit = .;
        . += _Min_Stack_Size;
        __StackTop = .;
    } > RAMB

    .ARM.attributes 0 : { *(.ARM.attributes) }
    /DISCARD/ : { *(.note.GNU-stack) }

    PROVIDE(end = __HeapBase);
    PROVIDE(_end = __HeapBase);
    PROVIDE(_stack = __StackTop);

    ASSERT(SIZEOF(.vectors) <= 0x400, "HC32F334 vector table overlaps ICG data")
    ASSERT(SIZEOF(.noinit) <= LENGTH(RAM_NOINIT), "HC32F334 retained area exceeds 16 bytes")
    ASSERT(__HeapLimit <= ORIGIN(RAM) + LENGTH(RAM), "HC32F334 bootloader RAM overflow")
    ASSERT(__StackTop <= ORIGIN(RAMB) + LENGTH(RAMB), "HC32F334 bootloader stack overflow")
}
