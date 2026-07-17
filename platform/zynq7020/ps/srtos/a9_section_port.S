/* SPDX-License-Identifier: MIT */
/**
 * @file    a9_section_port.S
 * @brief   Cortex-A9 exception and public-stack context-switch port.
 * @details Saves ARM core state, VFPv3 state, return PC, and CPSR on the
 *          shared task runtime stack. The saved stack image is copied by C
 *          while executing on the banked SVC stack, so restoring another
 *          task cannot overwrite the active scheduler call frame.
 */

    .syntax unified
    .cpu cortex-a9
    .fpu vfpv3
    .arm

    .equ SYS_MODE, 0x1f
    .equ SVC_MODE, 0x13
    .equ IRQ_MODE, 0x12
    .equ PORT_FAULT_CONTEXT, 0xA901
    .equ PORT_FAULT_UNDEFINED, 0xA902
    .equ PORT_FAULT_PREFETCH, 0xA903
    .equ PORT_FAULT_DATA_ABORT, 0xA904

    .extern _boot
    .extern IRQInterrupt
    .extern FIQInterrupt
    .extern section_task_scheduler_started
    .extern section_task_start_sp_get
    .extern section_task_switch_sp
    .extern g_a9_section_switch_requested
    .extern a9_section_port_fault

    .macro A9_SECTION_SAVE_CONTEXT
        srsdb   sp!, #SYS_MODE
        cps     #SYS_MODE
        push    {r0-r12, r14}
        vpush   {d0-d15}
        vpush   {d16-d31}
        vmrs    r0, FPSCR
        push    {r0}
        vmrs    r0, FPEXC
        push    {r0}
    .endm

    .macro A9_SECTION_RESTORE_CONTEXT
        pop     {r0}
        vmsr    FPEXC, r0
        pop     {r0}
        vmsr    FPSCR, r0
        vpop    {d16-d31}
        vpop    {d0-d15}
        pop     {r0-r12, r14}
        rfeia   sp!
    .endm

    .section .vectors, "ax"
    .align 5
    .global _vector_table
_vector_table:
    b       _boot
    b       a9_section_undefined_handler
    b       a9_section_svc_handler
    b       a9_section_prefetch_abort_handler
    b       a9_section_data_abort_handler
    nop
    b       a9_section_irq_handler
    b       a9_section_fiq_handler

    .section .text.a9_section_port, "ax"
    .align 4
    .global a9_section_port_install_vector_table
    .type a9_section_port_install_vector_table, %function
a9_section_port_install_vector_table:
    ldr     r0, =_vector_table
    mcr     p15, 0, r0, c12, c0, 0
    dsb
    isb
    bx      lr

    .align 4
    .type a9_section_svc_handler, %function
a9_section_svc_handler:
    cpsid   i
    push    {r0-r5, r12, lr}
    bl      section_task_scheduler_started
    cmp     r0, #0
    pop     {r0-r5, r12, lr}
    beq     a9_section_start_first_task

    A9_SECTION_SAVE_CONTEXT
    mov     r0, sp
    cps     #SVC_MODE
    bl      section_task_switch_sp
    cmp     r0, #0
    beq     a9_section_context_fault
    cps     #SYS_MODE
    mov     sp, r0
    A9_SECTION_RESTORE_CONTEXT

a9_section_start_first_task:
    push    {r4, lr}
    bl      section_task_start_sp_get
    cmp     r0, #0
    beq     a9_section_no_task_ready
    add     sp, sp, #8
    cps     #SYS_MODE
    mov     sp, r0
    A9_SECTION_RESTORE_CONTEXT

a9_section_no_task_ready:
    pop     {r4, lr}
    movs    pc, lr

    .align 4
    .type a9_section_irq_handler, %function
a9_section_irq_handler:
    sub     lr, lr, #4
    push    {lr}
    mrs     lr, SPSR
    push    {lr}
    cps     #SVC_MODE

    push    {r0-r5, r12, lr}
    vpush   {d0-d7}
    vpush   {d16-d31}
    vmrs    r0, FPSCR
    push    {r0}
    vmrs    r0, FPEXC
    push    {r0}

    bl      IRQInterrupt

    pop     {r0}
    vmsr    FPEXC, r0
    pop     {r0}
    vmsr    FPSCR, r0
    vpop    {d16-d31}
    vpop    {d0-d7}

    ldr     r0, =g_a9_section_switch_requested
    ldr     r1, [r0]
    cmp     r1, #0
    beq     a9_section_irq_return
    mov     r1, #0
    str     r1, [r0]
    dsb

    pop     {r0-r5, r12, lr}
    cps     #IRQ_MODE
    pop     {lr}
    msr     SPSR_cxsf, lr
    pop     {lr}

    A9_SECTION_SAVE_CONTEXT
    mov     r0, sp
    cps     #SVC_MODE
    bl      section_task_switch_sp
    cmp     r0, #0
    beq     a9_section_context_fault
    cps     #SYS_MODE
    mov     sp, r0
    A9_SECTION_RESTORE_CONTEXT

a9_section_irq_return:
    pop     {r0-r5, r12, lr}
    cps     #IRQ_MODE
    pop     {lr}
    msr     SPSR_cxsf, lr
    pop     {lr}
    movs    pc, lr

a9_section_context_fault:
    ldr     r0, =PORT_FAULT_CONTEXT
    b       a9_section_port_fault

    .align 4
a9_section_fiq_handler:
    push    {r0-r3, r12, lr}
    bl      FIQInterrupt
    pop     {r0-r3, r12, lr}
    subs    pc, lr, #4

a9_section_undefined_handler:
    ldr     r0, =PORT_FAULT_UNDEFINED
    b       a9_section_port_fault

a9_section_prefetch_abort_handler:
    ldr     r0, =PORT_FAULT_PREFETCH
    b       a9_section_port_fault

a9_section_data_abort_handler:
    ldr     r0, =PORT_FAULT_DATA_ABORT
    b       a9_section_port_fault

    .end
