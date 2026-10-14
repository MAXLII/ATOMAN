// SPDX-License-Identifier: MIT
/**
 * @file bsp_clk.c
 * @brief MINI STM32F103C8T6 clock verification.
 * @details
 *          This file is part of the digital power framework project.
 *          Module responsibilities:
 *          - Check vendor SystemInit selected the board's external oscillator
 *          - Stop startup if the clock tree cannot satisfy the UART time base
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
#include "bsp_clk.h"
#include "stm32f10x.h"

void bsp_clk_init(void)
{
    RCC_ClocksTypeDef clocks = {0}; /* Actual bus clocks after vendor startup. */
    SystemCoreClockUpdate();
    RCC_GetClocksFreq(&clocks);

    if (    (clocks.SYSCLK_Frequency != 72000000u) /* HSE 8 MHz x9 must lock. */
         || (clocks.HCLK_Frequency != 72000000u)   /* Scheduler clock is undivided. */
         || (clocks.PCLK1_Frequency != 36000000u)  /* APB1 stays within its limit. */
         || (clocks.PCLK2_Frequency != 72000000u)) /* USART1 clock is APB2. */
    {
        __disable_irq();

        for (;;)
        {
        }
    }
}
