// SPDX-License-Identifier: MIT
/**
 * @file systick.c
 * @brief Cortex-M3 scheduler time base.
 * @details
 *          This file is part of the digital power framework project.
 *          Module responsibilities:
 *          - Configure 100 us SysTick and lowest-priority PendSV
 *          - Maintain a 32-bit monotonic tick shared with task context
 *          Design notes:
 *          - C11 compatible; no dynamic allocation in platform drivers
 *          - Hardware access belongs to the BSP; framework bindings belong to src
 * @author Max.Li
 * @date 2026-10-14
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li.
 *          All rights reserved.
 *          This file is licensed under the MIT License.
 *          See the LICENSE file in the project root for full license text.
 */
#include "systick.h"
#include "stm32f10x.h"
static volatile uint32_t sys_tick_100us = 0u; /* ISR-owned scheduler clock. */
void systick_config(void)
{
    if (SysTick_Config(SystemCoreClock / 10000u) != 0u)
    {
        for (;;)
        {
        }
    }
    NVIC_SetPriority(SysTick_IRQn, 0u);
    NVIC_SetPriority(PendSV_IRQn, (1u << __NVIC_PRIO_BITS) - 1u);
}
void delay_decrement(void)
{
    sys_tick_100us++;
}
uint32_t systick_gettime_100us(void)
{
    return sys_tick_100us;
}
