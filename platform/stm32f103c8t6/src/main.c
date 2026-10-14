// SPDX-License-Identifier: MIT
/**
 * @file main.c
 * @brief STM32F103C8T6 framework entry point.
 * @details
 *          This file is part of the digital power framework project.
 *          Module responsibilities:
 *          - Verify clocks, start the tick, and initialize registered modules
 *          - Dispatch whichever SECTION runtime the include/source paths select
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
#include "systick.h"
#include "section.h"
/** @return This firmware entry point does not return. */
int main(void)
{
    bsp_clk_init();   /* Check vendor HSE clock setup against the schematic. */
    systick_config(); /* Start the common 100 us scheduler clock. */
    section_init();   /* Discover and initialize BSP bindings and services. */

    for (;;)
    {
        run_task(); /* Dispatch baremetal or Cortex-M SRTOS tasks. */
    }
}
