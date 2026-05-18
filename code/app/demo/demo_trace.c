// SPDX-License-Identifier: MIT
/**
 * @file    demo_trace.c
 * @brief   trace section demo.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Emit trace marks from periodic tasks
 *          - Demonstrate DBG_TRACE_MARK usage
 *          - Keep trace mark sources local to this demo file
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

static void demo_trace_mark_100ms(void)
{
    DBG_TRACE_MARK();
}

static void demo_trace_mark_500ms(void)
{
    DBG_TRACE_MARK();
}

REG_TASK_MS(100, demo_trace_mark_100ms)
REG_TASK_MS(500, demo_trace_mark_500ms)
