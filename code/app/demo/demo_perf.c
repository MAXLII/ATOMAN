// SPDX-License-Identifier: MIT
/**
 * @file    demo_perf.c
 * @brief   perf section demo.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Register named performance records with SECTION_PERF
 *          - Measure one shell-triggered code section
 *          - Measure one periodic task code section
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-18
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "perf.h"
#include "section.h"
#include "shell.h"

#include <stdint.h>

static uint32_t s_demo_perf_loop_count = 2000u;
static volatile uint32_t s_demo_perf_result = 0u;

REG_PERF_RECORD(demo_perf_shell)
REG_PERF_RECORD(demo_perf_task)

static void demo_perf_workload(uint32_t loop_count)
{
    uint32_t i;
    uint32_t acc = 0x12345678u;

    for (i = 0u; i < loop_count; ++i)
    {
        acc += (i ^ (acc >> 3)) + 0x1021u;
        acc = (acc << 5) | (acc >> 27);
    }

    s_demo_perf_result = acc;
}

static void demo_perf_cmd(DEC_MY_PRINTF)
{
    section_perf_record_t *record = P_RECORD_PERF(demo_perf_shell);

    PERF_START(demo_perf_shell);
    demo_perf_workload(s_demo_perf_loop_count);
    PERF_END(demo_perf_shell);

    if ((my_printf != NULL) && (my_printf->my_printf != NULL))
    {
        my_printf->my_printf("perf shell: loop=%lu time=%lu max=%lu result=%lu\r\n",
                             (unsigned long)s_demo_perf_loop_count,
                             (unsigned long)((record != NULL) ? record->time : 0u),
                             (unsigned long)((record != NULL) ? record->max_time : 0u),
                             (unsigned long)s_demo_perf_result);
    }
}

static void demo_perf_task_10ms(void)
{
    PERF_START(demo_perf_task);
    demo_perf_workload(200u);
    PERF_END(demo_perf_task);
}

REG_SHELL_VAR(DEMO_PERF_LOOP, s_demo_perf_loop_count, SHELL_UINT32, 200000u, 1u, NULL, SHELL_STA_NULL)
REG_SHELL_CMD(DEMO_PERF_RUN, demo_perf_cmd)
REG_TASK_MS(10, demo_perf_task_10ms)
