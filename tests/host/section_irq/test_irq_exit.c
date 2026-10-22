// SPDX-License-Identifier: MIT
/**
 * @file test_irq_exit.c
 * @brief Reproduce a tick interrupting an active Cortex-M context switch.
 * @details Executes the production SRTOS IRQ-exit and slice checks. Only the
 *          SCB register, tick and PendSV request are replaced by host values.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License; see LICENSE.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef struct test_scb
{
    uint32_t SHCSR;
} test_scb_t;
static test_scb_t scb;
#define SCB                     (&scb)
#define SCB_SHCSR_PENDSVACT_Msk (UINT32_C(1) << 10u)
#include "platform.h"

static uint32_t requests;
static uint8_t task_scheduler_started;
static uint32_t task_last_switch_tick;
uint32_t section_testbench_time_100us;
#define SECTION_TASK_SLICE_TICKS 10u
#undef SECTION_PORT_CONTEXT_SWITCH_REQUEST
#define SECTION_PORT_CONTEXT_SWITCH_REQUEST() (++requests)
uint32_t section_task_scheduler_started(void);
uint32_t section_task_slice_elapsed(void);
void section_task_irq_exit_request(void);
#include "irq_exit_production.inc"

int main(void)
{
    section_testbench_time_100us = 100u;
    task_last_switch_tick        = 90u;
    section_task_irq_exit_request();
    assert(requests == 0u);
    task_scheduler_started = 1u;
    /* SysTick interrupts PendSV while the outgoing slice is still expired. */
    scb.SHCSR = SCB_SHCSR_PENDSVACT_Msk;
    section_task_irq_exit_request();
    assert(requests == 0u);
    /* A new task gets its complete slice after exception return. */
    task_last_switch_tick        = 100u;
    scb.SHCSR                    = 0u;
    section_testbench_time_100us = 109u;
    section_task_irq_exit_request();
    assert(requests == 0u);
    section_testbench_time_100us = 110u;
    section_task_irq_exit_request();
    assert(requests == 1u);
    /* Other active-exception bits do not suppress a legitimate request. */
    scb.SHCSR = UINT32_C(1) << 11u;
    section_task_irq_exit_request();
    assert(requests == 2u);
    /* The tick and slice subtraction must also tolerate wraparound. */
    task_last_switch_tick        = UINT32_MAX - 4u;
    section_testbench_time_100us = 4u;
    section_task_irq_exit_request();
    assert(requests == 2u);
    section_testbench_time_100us = 5u;
    section_task_irq_exit_request();
    assert(requests == 3u);
    (void)puts("SRTOS IRQ exit: active PendSV, fresh slice, expiry and wrap passed");
    return 0;
}
