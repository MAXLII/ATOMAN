/* SPDX-License-Identifier: MIT
 * Preserve the SDK's original 128-byte IRQ frame before rtos_int_enter moves
 * execution onto the ISR stack. The real SDK entry/exit remain authoritative.
 * Copyright (c) 2026 Max.Li. Licensed under the MIT License.
 */
    .section .iram1,"ax"
    .balign 4
    .global __wrap_rtos_int_enter
    .type __wrap_rtos_int_enter, @function
__wrap_rtos_int_enter:
    addi sp, sp, -16
    sw ra, 0(sp)
    addi a0, sp, 16
    csrr a1, mstatus
    csrr a2, mcause
    call bsp_srtos_irq_enter
    lw ra, 0(sp)
    addi sp, sp, 16
    tail __real_rtos_int_enter
    .size __wrap_rtos_int_enter, .-__wrap_rtos_int_enter

    .balign 4
    .global __wrap_rtos_int_exit
    .type __wrap_rtos_int_exit, @function
__wrap_rtos_int_exit:
    addi sp, sp, -16
    sw ra, 0(sp)
    sw a0, 4(sp)
    sw a1, 8(sp)
    call bsp_srtos_irq_exit
    lw a1, 8(sp)
    lw a0, 4(sp)
    lw ra, 0(sp)
    addi sp, sp, 16
    tail __real_rtos_int_exit
    .size __wrap_rtos_int_exit, .-__wrap_rtos_int_exit
