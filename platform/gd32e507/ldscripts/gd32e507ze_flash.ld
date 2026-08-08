/* SPDX-License-Identifier: MIT */
/* GD32E507ZE: 512 KiB Flash and 128 KiB SRAM. */

ENTRY(Reset_Handler)

_Min_Heap_Size = 0x400;
_Min_Stack_Size = 0x800;

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
}

PHDRS
{
    flash PT_LOAD FLAGS(5);
    ram PT_LOAD FLAGS(6);
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(512);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH :flash

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
    } > FLASH :flash

    .section :
    {
        . = ALIGN(4);
        __section_start = .;
        KEEP(*(section))
        KEEP(*(section*))
        __section_end = .;
        . = ALIGN(4);
    } > FLASH :flash

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH :flash

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH :flash

    .ARM.exidx :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH :flash

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH :flash

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH :flash

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH :flash

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH :ram

    _sifunc_ram = LOADADDR(.func_ram);

    .func_ram :
    {
        . = ALIGN(4);
        _sfunc_ram = .;
        *(.func_ram)
        *(.func_ram*)
        . = ALIGN(4);
        _efunc_ram = .;
    } > RAM AT > FLASH :ram

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > RAM :NONE

    .heap_stack (NOLOAD) :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        . = . + _Min_Heap_Size;
        PROVIDE(_heap_end = .);
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > RAM :NONE

    /DISCARD/ :
    {
        *(.note.GNU-stack)
    }

    .ARM.attributes 0 : { *(.ARM.attributes) }
}

ASSERT(_estack >= ADDR(.heap_stack) + SIZEOF(.heap_stack), "GD32E507 RAM overflow")
