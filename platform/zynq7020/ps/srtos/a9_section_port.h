// SPDX-License-Identifier: MIT
/**
 * @file    a9_section_port.h
 * @brief   Cortex-A9 exception port interface for the section SRTOS.
 * @details
 *          This file is part of the Zynq-7020 platform project.
 *
 *          Module responsibilities:
 *          - Expose Cortex-A9 vector installation and SVC yield operations
 *          - Publish the IRQ-exit context-switch request shared with assembly
 *          - Record port-level switch requests and fatal exception reasons
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Context save and restore are implemented in a9_section_port.S
 *          - Scheduler C code executes on the banked SVC stack
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

#ifndef A9_SECTION_PORT_H
#define A9_SECTION_PORT_H

#include <stdint.h>

typedef enum
{
    A9_SECTION_PORT_FAULT_NONE = 0U,            /* No architecture-port fault has been recorded. */
    A9_SECTION_PORT_FAULT_CONTEXT = 0xA901U,    /* The scheduler returned an invalid task context. */
    A9_SECTION_PORT_FAULT_UNDEFINED = 0xA902U,  /* The processor entered the undefined-instruction vector. */
    A9_SECTION_PORT_FAULT_PREFETCH = 0xA903U,   /* The processor entered the prefetch-abort vector. */
    A9_SECTION_PORT_FAULT_DATA_ABORT = 0xA904U, /* The processor entered the data-abort vector. */
} a9_section_port_fault_t;

typedef struct
{
    uint32_t yield_request_count;      /* Number of task-side SVC yield requests. */
    uint32_t irq_switch_request_count; /* Number of context switches requested from IRQ exit. */
    uint32_t idle_wait_count;          /* Number of low-power waits entered by the scheduler idle path. */
    uint32_t fault_reason;             /* Most recent unrecoverable architecture-port fault. */
} a9_section_port_debug_t;

extern volatile uint32_t g_a9_section_switch_requested;
extern volatile a9_section_port_debug_t g_a9_section_port_debug;

/**
 * @brief Point Cortex-A9 VBAR at the section SRTOS exception vector table.
 */
void a9_section_port_install_vector_table(void);

/**
 * @brief Yield the current shared task stack through an SVC exception.
 */
void a9_section_port_yield(void);

/**
 * @brief Request one shared-stack context switch before the current IRQ returns.
 */
void a9_section_port_switch_request(void);

/**
 * @brief Save the current IRQ mask state and disable IRQ delivery.
 * @return CPSR value captured before IRQ masking.
 */
uint32_t a9_section_port_irq_save(void);

/**
 * @brief Restore IRQ delivery to the state captured by a9_section_port_irq_save().
 * @param[in] saved_cpsr CPSR value captured before entering the critical section.
 */
void a9_section_port_irq_restore(uint32_t saved_cpsr);

/**
 * @brief Wait for the next interrupt while no section task is runnable.
 */
void a9_section_port_wait_for_interrupt(void);

/**
 * @brief Record an unrecoverable fault and disable Cortex-A9 IRQ/FIQ delivery.
 * @param[in] reason Section scheduler or architecture-port fault reason.
 */
void a9_section_port_fault(uint32_t reason) __attribute__((noreturn));

#endif /* A9_SECTION_PORT_H */
