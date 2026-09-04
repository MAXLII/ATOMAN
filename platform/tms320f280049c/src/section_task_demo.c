// SPDX-License-Identifier: MIT
/**
 * @file    section_task_demo.c
 * @brief   TMS320F280049C SECTION registration demo.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Register one initialization callback through REG_INIT
 *          - Register one 500 ms cooperative task through REG_TASK_MS
 *          - Toggle user LED LD4 without contaminating the binary FRAME serial stream
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The demo task executes in the cooperative main-loop context
 *          - Hardware access is abstracted by the local platform interface
 *
 * @author  Max.Li
 * @date    2026-09-05
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "section.h"

volatile uint32_t g_tms320f280049c_demo_init_count = 0u;
volatile uint32_t g_tms320f280049c_demo_task_count = 0u;

static void tms320f280049c_demo_init(void)
{
    g_tms320f280049c_demo_init_count++;
}

static void tms320f280049c_demo_task(void)
{
    g_tms320f280049c_demo_task_count++;
    tms320f280049c_led_toggle();
}

REG_INIT(0, tms320f280049c_demo_init)
REG_TASK_MS(500, tms320f280049c_demo_task)
