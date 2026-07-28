// SPDX-License-Identifier: MIT
/**
 * @file    main.c
 * @brief   HC32F334 bootloader runtime entry point.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Enable the minimum HC32 peripheral register access
 *          - Initialize the system clock, SysTick, and Section runtime
 *          - Run registered Bootloader tasks from the foreground loop
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Bootloader services initialize through Section registration
 *          - Platform composition remains outside the entry point
 *
 * @author  Max.Li
 * @date    2026-07-28
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "bsp_clk.h"
#include "hc32_ll.h"
#include "section.h"
#include "systick.h"

#define HC32_BOOT_LL_PERIPH_SEL (LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | \
                                 LL_PERIPH_EFM | LL_PERIPH_SRAM)

int main(void)
{
    LL_PERIPH_WE(HC32_BOOT_LL_PERIPH_SEL);
    BSP_CLK_Init();
    systick_config();
    section_init();
    while (1)
    {
        run_task();
    }
}
