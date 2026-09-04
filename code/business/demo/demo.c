// SPDX-License-Identifier: MIT
/**
 * @file    demo.c
 * @brief   init section demo.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Register startup callbacks with SECTION_INIT
 *          - Demonstrate init callback execution order
 *          - Bind the trace time base used by the trace demo
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

#include "section.h"
#include "trace.h"

#include <stdint.h>

static uint32_t s_demo_init_count = 0u;

extern volatile uint32_t sys_tick_100us;

static void demo_init_trace_time(void)
{
    DBG_TRACE_BIND_TIME(&sys_tick_100us);
    s_demo_init_count++;
}

static void demo_init_runtime_state(void)
{
    s_demo_init_count++;
}

REG_INIT(-10, demo_init_trace_time)
REG_INIT(10, demo_init_runtime_state)
