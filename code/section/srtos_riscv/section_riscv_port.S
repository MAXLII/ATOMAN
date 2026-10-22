/* SPDX-License-Identifier: MIT
 * RV32 shared-stack execution boundary. Integer-only; FPU/coprocessor ports
 * must extend the context before enabling those instructions in tasks.
 * Copyright (c) 2026 Max.Li. Licensed under the MIT License.
 */
#if !defined(__riscv_xlen) || (__riscv_xlen != 32)
#error "This shared-stack context requires RV32."
#endif
#if defined(__riscv_flen) || defined(__riscv_vector)
#error "Extend the context before enabling floating-point or vector registers."
#endif

    .section .bss
    .balign 16
dispatch_context:
    .space 64

    .section .text
    .balign 4
    .global section_riscv_enter
    .type section_riscv_enter, @function
section_riscv_enter:
    la t0, dispatch_context
    sw ra, 0(t0)
    sw sp, 4(t0)
    sw s0, 8(t0)
    sw s1, 12(t0)
    sw s2, 16(t0)
    sw s3, 20(t0)
    sw s4, 24(t0)
    sw s5, 28(t0)
    sw s6, 32(t0)
    sw s7, 36(t0)
    sw s8, 40(t0)
    sw s9, 44(t0)
    sw s10, 48(t0)
    sw s11, 52(t0)
    csrrci t1, mstatus, 8
    sw t1, 56(t0)
    lw t1, 4(a0)
    csrw mcause, t1
    lw t1, 0(a0)
    csrw mstatus, t1
    addi sp, a0, 16
    lw t0, 0(sp)
    csrw mepc, t0
    lw ra, 4(sp)
    /* gp is immutable for all contexts in the same firmware. */
    lw tp, 16(sp)
    lw t0, 20(sp)
    lw t1, 24(sp)
    lw t2, 28(sp)
    lw s0, 32(sp)
    lw s1, 36(sp)
    lw a0, 40(sp)
    lw a1, 44(sp)
    lw a2, 48(sp)
    lw a3, 52(sp)
    lw a4, 56(sp)
    lw a5, 60(sp)
    lw a6, 64(sp)
    lw a7, 68(sp)
    lw s2, 72(sp)
    lw s3, 76(sp)
    lw s4, 80(sp)
    lw s5, 84(sp)
    lw s6, 88(sp)
    lw s7, 92(sp)
    lw s8, 96(sp)
    lw s9, 100(sp)
    lw s10, 104(sp)
    lw s11, 108(sp)
    lw t3, 112(sp)
    lw t4, 116(sp)
    lw t5, 120(sp)
    lw t6, 124(sp)
    addi sp, sp, 128
    mret
    .size section_riscv_enter, .-section_riscv_enter

    .balign 4
    .global section_riscv_dispatch_return
    .type section_riscv_dispatch_return, @function
section_riscv_dispatch_return:
    csrci mstatus, 8
    la t0, dispatch_context
    lw ra, 0(t0)
    lw sp, 4(t0)
    lw s0, 8(t0)
    lw s1, 12(t0)
    lw s2, 16(t0)
    lw s3, 20(t0)
    lw s4, 24(t0)
    lw s5, 28(t0)
    lw s6, 32(t0)
    lw s7, 36(t0)
    lw s8, 40(t0)
    lw s9, 44(t0)
    lw s10, 48(t0)
    lw s11, 52(t0)
    lw t1, 56(t0)
    csrw mstatus, t1
    ret
    .size section_riscv_dispatch_return, .-section_riscv_dispatch_return

    .balign 4
    .global section_riscv_yield
    .type section_riscv_yield, @function
section_riscv_yield:
    addi sp, sp, -144
    sw ra, 16(sp)
    sw ra, 20(sp)
    sw gp, 28(sp)
    sw tp, 32(sp)
    sw t0, 36(sp)
    sw t1, 40(sp)
    sw t2, 44(sp)
    sw s0, 48(sp)
    sw s1, 52(sp)
    sw a0, 56(sp)
    sw a1, 60(sp)
    sw a2, 64(sp)
    sw a3, 68(sp)
    sw a4, 72(sp)
    sw a5, 76(sp)
    sw a6, 80(sp)
    sw a7, 84(sp)
    sw s2, 88(sp)
    sw s3, 92(sp)
    sw s4, 96(sp)
    sw s5, 100(sp)
    sw s6, 104(sp)
    sw s7, 108(sp)
    sw s8, 112(sp)
    sw s9, 116(sp)
    sw s10, 120(sp)
    sw s11, 124(sp)
    sw t3, 128(sp)
    sw t4, 132(sp)
    sw t5, 136(sp)
    sw t6, 140(sp)
    addi t0, sp, 144
    sw t0, 24(sp)
    csrrci t0, mstatus, 8
    /* Encode an M-mode mret context, retaining the caller's interrupt enable. */
    li t1, 0x1888
    not t1, t1
    and t1, t0, t1
    andi t0, t0, 8
    slli t0, t0, 4
    or t1, t1, t0
    li t0, 0x1800
    or t1, t1, t0
    sw t1, 0(sp)
    sw zero, 8(sp)
    sw zero, 12(sp)
    mv a0, sp
    call section_riscv_yield_commit
    tail section_riscv_dispatch_return
    .size section_riscv_yield, .-section_riscv_yield
