/* SPDX-License-Identifier: MIT
 * @file    gd32g553_gcc.ld
 * @brief   Linker script for GD32G553 with GCC
 *
 * @details
 *          Scatter file equivalent for ARM GCC linker.
 *          Memory layout:
 *          - Flash: 0x08000000, 512KB
 *          - SRAM:  0x20000000, 96KB
 *          - SRAM2: 0x10000000, 32KB
 *
 * @author  Generated for GD32G553RE
 * @date    2026-05-11
 * @version 1.0.0
 */

ENTRY(Reset_Handler)

/* Memory Regions */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 0x80000
    RAM (rwx)   : ORIGIN = 0x20000000, LENGTH = 0x18000
    RAM2 (rwx)  : ORIGIN = 0x10000000, LENGTH = 0x8000
}

/* Section Definitions */
SECTIONS
{
    .text :
    {
        . = ALIGN(4);
        _stext = .;

        *(.vectors)
        *(.text*)
        *(.rodata*)

        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .ARM.exidx :
    {
        *(.ARM.exidx*)
        *(.gnu.linkonce.armexidx.*)
    } > FLASH

    /* .func_ram section for functions that need to run from RAM */
    .func_ram :
    {
        . = ALIGN(4);
        _sfunc_ram = .;
        *(.func_ram*)
        *(.Func_Ram*)
        . = ALIGN(4);
        _efunc_ram = .;
    } > RAM

    /* Initialized data */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;

        _sidata = LOADADDR(.data);
        *(.data*)
        *(.ramfunc*)

        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    /* Zero-initialized data */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)

        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* Stack and Heap */
    ._user_stack_heap :
    {
        . = ALIGN(8);
        _end = .;

        . = ALIGN(8);
        __StackTop = .;
        __StackLimit = __StackTop - 0x400;  /* 1KB stack */
    } > RAM

    /DISCARD/ :
    {
        libc.a (*)
        libm.a (*)
        libgcc.a (*)
    }
}

/* Symbol for startup */
_sp = __StackTop;
