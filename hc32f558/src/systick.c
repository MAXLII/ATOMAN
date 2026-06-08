// SPDX-License-Identifier: MIT
/**
 * @file    systick.c
 * @brief   HC32F558 SysTick timing module.
 * @details
 *          This file is part of the HC32F558 project.
 *
 *          Module responsibilities:
 *          - Configure SysTick as the 100 us scheduler time base
 *          - Maintain a monotonic 100 us tick counter
 *          - Provide a blocking millisecond delay helper
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - delay_decrement() runs in SysTick ISR context
 *          - Hardware access is abstracted through CMSIS SysTick APIs
 *
 * @author  Max.Li
 * @date    2026-06-06
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "systick.h"
#include "hc32f5xx.h"

static volatile uint32_t s_delay_tick;
volatile uint32_t sys_tick_100us;

void systick_config(void)
{
    SystemCoreClockUpdate();

    if (0UL != SysTick_Config(SystemCoreClock / 10000UL)) {
        for (;;) {
        }
    }

    NVIC_SetPriority(SysTick_IRQn, 0U);
}

void delay_1ms(uint32_t count)
{
    s_delay_tick = count * 10UL;

    while (0UL != s_delay_tick) {
    }
}

void delay_decrement(void)
{
    sys_tick_100us++;

    if (0UL != s_delay_tick) {
        s_delay_tick--;
    }
}

uint32_t systick_gettime_100us(void)
{
    return sys_tick_100us;
}
