// SPDX-License-Identifier: MIT
/**
 * @file bsp_timer.c
 * @brief 1 ms SECTION interrupt source.
 * @details
 *          This file is part of the digital power framework project.
 *          Module responsibilities:
 *          - Initialize TIM2 from the doubled 36 MHz APB1 timer clock
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
#include "bsp_timer.h"
#include "stm32f10x.h"
#define DWT_CTRL_REG (*(volatile uint32_t *)(uintptr_t)0xE0001000u) /* DWT control address. */
void bsp_timer_perf_resume(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT_CTRL_REG |= 1u; /* Preserve CYCCNT across debugger disconnects. */
}
void bsp_timer_init(void)
{
    TIM_TimeBaseInitTypeDef timer = {0}; /* TIM2 counts at 1 MHz, wraps at 1 ms. */
    *BSP_TIMER_PERF_COUNTER       = 0u;
    bsp_timer_perf_resume(); /* CYCCNTENA advances at the 72 MHz core clock. */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM_TimeBaseStructInit(&timer);
    timer.TIM_Prescaler = 71u;
    timer.TIM_Period    = 999u;
    TIM_TimeBaseInit(TIM2, &timer);
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    NVIC_SetPriority(TIM2_IRQn, 2u);
    NVIC_EnableIRQ(TIM2_IRQn);
    TIM_Cmd(TIM2, ENABLE);
}
