// SPDX-License-Identifier: MIT
/**
 * @file    main.c
 * @brief   APM32 demo firmware entry.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Initialize the APM32 system clock and 100 us SysTick time base
 *          - Start section-based module initialization
 *          - Run the shared demo task scheduler loop
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "main.h"
#include "section.h"

int main(void)
{
    SystemCoreClockUpdate();
    (void)SysTick_Config(SystemCoreClock / 10000U);

    section_init();

    while (1)
    {
        run_task();
    }
}
