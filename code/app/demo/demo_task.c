// SPDX-License-Identifier: MIT
/**
 * @file    demo_task.c
 * @brief   task section demo.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Register periodic callbacks with SECTION_TASK
 *          - Demonstrate millisecond and scheduler-tick task periods
 *          - Keep task counters local to this demo file
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

#include <stdint.h>

static uint32_t s_demo_task_10ms_count = 0u;
static uint32_t s_demo_task_tick_count = 0u;

static void demo_task_10ms(void)
{
    s_demo_task_10ms_count++;
}

static void demo_task_100us(void)
{
    s_demo_task_tick_count++;
}

REG_TASK_MS(10, demo_task_10ms)
REG_TASK(1, demo_task_100us)
