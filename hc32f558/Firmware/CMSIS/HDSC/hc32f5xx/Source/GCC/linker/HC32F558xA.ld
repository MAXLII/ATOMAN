/******************************************************************************
 * Copyright (C) 2022-2026, Xiaohua Semiconductor Co., Ltd. All rights reserved.
 *
 * This software component is licensed by XHSC under BSD 3-Clause license
 * (the "License"); You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                    opensource.org/licenses/BSD-3-Clause
 *
 */
/*****************************************************************************/
/*  File        HC32F558xA.ld                                                */
/*  Abstract    Linker script for HC32F558 Device with                       */
/*              128KByte FLASH, 128KByte RAM                                 */
/*  Version     V1.0                                                         */
/*  Date        2026-04-16                                                   */
/*****************************************************************************/
/* OTP section(data sections are not flash multiplexed region) implementation.
   You need to pay attention to the size of the specified OTP block.
   Take two OTP blocks for example. */
__OTP_DATA_BASE = 0x03000000;
__OTP_LOCK_BASE = 0x03001800;
/* OTP block 16 */
__OTP_DATA_B16_START  = 0x03000000;
__OTP_LOCK_B16_START  = 0x03001900;
__OTP_DATA_B16_OFFSET = __OTP_DATA_B16_START - __OTP_DATA_BASE;
__OTP_LOCK_B16_OFFSET = __OTP_LOCK_B16_START - __OTP_LOCK_BASE;
/* OTP block 17 */
__OTP_DATA_B17_START  = 0x03000010;
__OTP_LOCK_B17_START  = 0x03001910;
__OTP_DATA_B17_OFFSET = __OTP_DATA_B17_START - __OTP_DATA_BASE;
__OTP_LOCK_B17_OFFSET = __OTP_LOCK_B17_START - __OTP_LOCK_BASE;

/* Use contiguous memory regions for simple. */
MEMORY
{
    FLASH       (rx): ORIGIN = 0x00000000, LENGTH = 128K
    OTP_DATA    (rx): ORIGIN = 0x03000000, LENGTH = 6K
    OTP_LOCK    (rx): ORIGIN = 0x03001800, LENGTH = 1920
    ICG          (r): ORIGIN = 0x0300E000, LENGTH = 8K
    RAM        (rwx): ORIGIN = 0x1FFF8000, LENGTH = 128K
}

ENTRY(Reset_Handler)

SECTIONS
{
    .vectors :
    {
        . = ALIGN(4);
        KEEP(*(.vectors))
        . = ALIGN(4);
    } >FLASH

    .icg_sec 0x0300E000 :
    {
        KEEP(*(.icg_sec))
    } >ICG

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)

        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
    } >FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } >FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } >FLASH

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } >FLASH
    __exidx_end = .;

    .preinit_array :
    {
        . = ALIGN(4);
        /* preinit data */
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);
        . = ALIGN(4);
    } >FLASH

    .init_array :
    {
        . = ALIGN(4);
        /* init data */
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);
        . = ALIGN(4);
    } >FLASH

    .fini_array :
    {
        . = ALIGN(4);
        /* finit data */
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN (__fini_array_end = .);
        . = ALIGN(4);
    } >FLASH

    __etext = ALIGN(4);

    .otp_data :
    {
        . = ALIGN(4);
        . = ORIGIN(OTP_DATA) + __OTP_DATA_B16_OFFSET;
        KEEP(*(.otp_b16_data*))
        . = ORIGIN(OTP_DATA) + __OTP_DATA_B17_OFFSET;
        KEEP(*(.otp_b17_data*))
        . = ALIGN(4);
    } >OTP_DATA

    .otp_lock :
    {
        . = ALIGN(4);
        . = ORIGIN(OTP_LOCK) + __OTP_LOCK_B16_OFFSET;
        KEEP(*(.otp_b16_lock*))
        . = ORIGIN(OTP_LOCK) + __OTP_LOCK_B17_OFFSET;
        KEEP(*(.otp_b17_lock*))
        . = ALIGN(4);
    } >OTP_LOCK

    .data : AT (__etext)
    {
        . = ALIGN(4);
        __data_start__ = .;
        *(vtable)
        *(.data)
        *(.data*)
        . = ALIGN(4);
        *(.ramfunc)
        *(.ramfunc*)
        . = ALIGN(4);
        __data_end__ = .;
    } >RAM

    .bss (NOLOAD):
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
        . = ALIGN(4);
        *(.noinit*)
        . = ALIGN(4);
    } >RAM

    .heap_stack (COPY) :
    {
        . = ALIGN(8);
        __end__ = .;
        PROVIDE(end = .);
        PROVIDE(_end = .);
        *(.heap*)
        . = ALIGN(8);
        __HeapLimit = .;

        __StackLimit = .;
        *(.stack*)
        . = ALIGN(8);
        __StackTop = .;
    } >RAM

    /DISCARD/ :
    {
        libc.a (*)
        libm.a (*)
        libgcc.a (*)
    }

    .ARM.attributes 0 : { *(.ARM.attributes) }

    PROVIDE(_stack = __StackTop);
    PROVIDE(_Min_Heap_Size = __HeapLimit - __HeapBase);
    PROVIDE(_Min_Stack_Size = __StackTop - __StackLimit);

    __RamEnd = ORIGIN(RAM) + LENGTH(RAM);
    ASSERT(__StackTop <= __RamEnd, "region RAM overflowed with stack")
}
