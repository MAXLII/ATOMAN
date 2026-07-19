// SPDX-License-Identifier: MIT
/**
 * @file    a9_section_control.c
 * @brief   Cortex-A9 control operations for the section SRTOS port.
 * @details
 *          This file is part of the Zynq-7020 platform project.
 *
 *          Module responsibilities:
 *          - Generate SVC exceptions for cooperative public-stack switching
 *          - Latch IRQ-exit context-switch requests for the assembly handler
 *          - Preserve debugger-readable counters and fatal fault information
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Request publication is ordered with a Cortex-A9 data barrier
 *          - Fatal fault handling intentionally does not return
 *
 * @author  Max.Li
 * @date    2026-07-17
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "a9_section_port.h"

#include <stdint.h>

volatile uint32_t g_a9_section_switch_requested = 0U; /* Task-switch request consumed by the IRQ return path. */
volatile a9_section_port_debug_t g_a9_section_port_debug = {
    .yield_request_count = 0U,
    .irq_switch_request_count = 0U,
    .idle_wait_count = 0U,
    .fault_reason = A9_SECTION_PORT_FAULT_NONE,
}; /* Cortex-A9 section SRTOS architecture-port diagnostics. */

void a9_section_port_yield(void)
{
    g_a9_section_port_debug.yield_request_count++;
    __asm volatile("svc 0" ::: "memory", "cc"); /* Enter the shared-stack switch path. */
}

void a9_section_port_switch_request(void)
{
    g_a9_section_port_debug.irq_switch_request_count++;
    g_a9_section_switch_requested = 1U;
    __asm volatile("dsb" ::: "memory"); /* Publish the request before the assembly IRQ return path reads it. */
}

uint32_t a9_section_port_irq_save(void)
{
    uint32_t saved_cpsr = 0U; /* IRQ mask state that must be restored on critical-section exit. */

    __asm volatile("mrs %0, cpsr\n\t"
                   "cpsid i\n\t"
                   "dsb\n\t"
                   "isb"
                   : "=r"(saved_cpsr)
                   :
                   : "memory", "cc");

    return saved_cpsr;
}

void a9_section_port_irq_restore(uint32_t saved_cpsr)
{
    __asm volatile("dsb" ::: "memory");
    if ((saved_cpsr & 0x00000080U) == 0U) /* IRQ delivery was enabled before entering the critical section. */
    {
        __asm volatile("cpsie i" ::: "memory", "cc");
    }
    __asm volatile("isb" ::: "memory");
}

void a9_section_port_wait_for_interrupt(void)
{
    g_a9_section_port_debug.idle_wait_count++;
    __asm volatile("dsb\n\t"
                   "wfi"
                   ::: "memory"); /* Avoid repeated SVC polling while waiting for the next scheduler tick. */
}

void a9_section_port_fault(uint32_t reason)
{
    g_a9_section_port_debug.fault_reason = reason;
    __asm volatile("cpsid if" ::: "memory", "cc"); /* Preserve the fault context and prevent nested exceptions. */
    __asm volatile("dsb" ::: "memory");
    __asm volatile("isb" ::: "memory");

    for (;;)
    {
        __asm volatile("wfi" ::: "memory"); /* Keep the processor observable for JTAG fault inspection. */
    }
}
