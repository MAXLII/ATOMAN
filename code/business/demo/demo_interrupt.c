// SPDX-License-Identifier: MIT
/**
 * @file    demo_interrupt.c
 * @brief   interrupt section demo.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Register callbacks with SECTION_INTERRUPT
 *          - Demonstrate interrupt callback priority ordering
 *          - Keep interrupt callback state local to this demo file
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

static uint32_t s_demo_interrupt_fast_count = 0u;
static uint32_t s_demo_interrupt_slow_count = 0u;

static void demo_interrupt_fast(void)
{
    s_demo_interrupt_fast_count++;
}

static void demo_interrupt_slow(void)
{
    s_demo_interrupt_slow_count++;
}

REG_INTERRUPT(5, demo_interrupt_fast)
REG_INTERRUPT(10, demo_interrupt_slow)
