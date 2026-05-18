// SPDX-License-Identifier: MIT
/**
 * @file    demo_scope.c
 * @brief   scope section demo.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Register scope variables with SECTION_SCOPE
 *          - Update scope sample values from one periodic task
 *          - Run the scope capture state machine for the registered scope
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

#include "scope.h"
#include "section.h"

#include <math.h>
#include <stdint.h>

static uint32_t s_demo_scope_tick = 0u;

REG_SCOPE_EX(demo_scope_wave, 100, 50, 10000u,
             demo_scope_sin,
             demo_scope_cos)

static void demo_scope_task_10ms(void)
{
    float theta;

    theta = 2.0f * 3.1415926f * (float)(s_demo_scope_tick % 100u) / 100.0f;
    demo_scope_sin = sinf(theta);
    demo_scope_cos = cosf(theta);

    SCOPE_RUN(demo_scope_wave);
    s_demo_scope_tick++;
}

REG_TASK_MS(10, demo_scope_task_10ms)
