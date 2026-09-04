// SPDX-License-Identifier: MIT
/**
 * @file    jitter.c
 * @brief   Demo timer-observation Interface implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Forward jitter timer count reads to the active BSP
 *          - Forward timer clock queries to the active BSP
 *          - Forward timer period queries to the active BSP
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Read functions are used from the timer interrupt path
 *          - Register access remains inside the selected BSP
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

#include "jitter.h"

#include "bsp_timer.h"

uint32_t demo_jitter_timer_count_get(void)
{
    return bsp_timer_jitter_count_get();
}

uint32_t demo_jitter_timer_clock_hz_get(void)
{
    return bsp_timer_jitter_clock_hz_get();
}

uint32_t demo_jitter_timer_period_ticks_get(void)
{
    return bsp_timer_jitter_period_ticks_get();
}
