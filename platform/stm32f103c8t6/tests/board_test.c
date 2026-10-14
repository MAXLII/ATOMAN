// SPDX-License-Identifier: MIT
/**
 * @file board_test.c
 * @brief Optional SECTION/SRTOS physical-board acceptance workload.
 * @details
 *          This file is part of the digital power framework project.
 *          Module responsibilities:
 *          - Verify task/interrupt progress and local stack values across preemption
 *          - Expose test evidence without enabling the stress workload in normal firmware
 *          Design notes:
 *          - C11 compatible; no dynamic allocation in platform drivers
 *          - Hardware access belongs to the BSP; framework bindings belong to src
 * @author Max.Li
 * @date 2026-10-14
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li.
 *          All rights reserved.
 *          This file is licensed under the MIT License.
 *          See the LICENSE file in the project root for full license text.
 */
#include "section.h"
#include "systick.h"
#include "shell.h"
#ifndef STM32_BOARD_TEST
#define STM32_BOARD_TEST 0u /* Explicit opt-in for the bounded preemption workload. */
#endif
#if (STM32_BOARD_TEST == 1u)
static volatile uint32_t fast_count     = 0u; /* Registered 1 ms task execution count. */
static volatile uint32_t context_checks = 0u; /* Completed 4 ms stack checks. */
static volatile uint32_t context_errors = 0u; /* Corrupted local sentinel words. */
static volatile uint32_t preemptions    = 0u; /* Long activations interrupted by the 1 ms task. */
static void fast_probe(void)
{
    fast_count++;
}
static void context_probe(void)
{
    volatile uint32_t words[16] = {0}; /* PSP locals must survive nested task snapshots. */
    uint32_t start              = systick_gettime_100us(); /* Unsigned subtraction tolerates tick wrap. */
    uint32_t before             = fast_count; /* Task progress before occupying the CPU. */
    uint32_t index              = 0u;         /* Sentinel slot in the fixed test frame. */

    for (index = 0u; index < 16u; index++)
    {
        words[index] = 0x13572468u ^ index;
    }

    while ((systick_gettime_100us() - start) < 40u)
    {
        __NOP();
    }

    if (fast_count > before)
    {
        preemptions++;
    }

    for (index = 0u; index < 16u; index++)
    {
        if (words[index] != (0x13572468u ^ index))
        {
            context_errors++;
        }
    }
    context_checks++;
}
static void test(shell_core_io_t *p_io)
{
    uint32_t fault = 0u; /* Runtime fault is absent in cooperative mode. */
#if (SECTION_RUNTIME_PREEMPTIVE == 1u)
    fault = g_section_fault_debug.task_fault_reason;
#endif

    if (    (p_io == NULL)             /* Require a bound output object. */
         || (p_io->my_printf == NULL)) /* Require a printable Link. */
    {
        return;
    }
    p_io->my_printf("fast=%u checks=%u errors=%u preemptions=%u fault=%u\r\n",
                    (unsigned)fast_count,
                    (unsigned)context_checks,
                    (unsigned)context_errors,
                    (unsigned)preemptions,
                    (unsigned)fault);
}
REG_TASK_MS(1u, fast_probe)
REG_TASK_MS(100u, context_probe)
REG_SHELL_CMD(test, test)
#endif /* STM32_BOARD_TEST */
