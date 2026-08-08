// SPDX-License-Identifier: MIT
/**
 * @file    systick.c
 * @brief   GD32E507 SysTick time-base implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Configure SysTick for a 100 us interrupt period
 *          - Maintain the monotonic framework time base
 *          - Maintain the platform blocking-delay countdown
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Tick state is shared between interrupt and background contexts
 *          - Hardware access uses CMSIS SysTick and NVIC interfaces
 *
 * @author  Max.Li
 * @date    2026-08-08
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "systick.h"

#include "gd32e50x.h"

static volatile uint32_t delay_count_100us = 0u; /* Remaining delay in 100 us ticks. */
volatile uint32_t sys_tick_100us = 0u;           /* Monotonic framework time in 100 us ticks. */

void systick_config(void)
{
    if (SysTick_Config(SystemCoreClock / 10000u) != 0u)
    {
        for (;;)
        {
        }
    }

    NVIC_SetPriority(SysTick_IRQn, 0u);
    NVIC_SetPriority(PendSV_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL);
}

void delay_1ms(uint32_t count)
{
    delay_count_100us = count * 10u;
    while (delay_count_100us != 0u)
    {
    }
}

void delay_decrement(void)
{
    sys_tick_100us++;
    if (delay_count_100us != 0u)
    {
        delay_count_100us--;
    }
}

uint32_t systick_gettime_100us(void)
{
    return sys_tick_100us;
}
