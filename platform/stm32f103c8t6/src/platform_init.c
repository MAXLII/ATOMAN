// SPDX-License-Identifier: MIT
/**
 * @file platform_init.c
 * @brief Board-to-framework initialization bindings.
 * @details
 *          This file is part of the digital power framework project.
 *          Module responsibilities:
 *          - Keep SECTION registration out of hardware-only BSP modules
 *          - Provide DWT CYCCNT as the 72 MHz performance clock
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
#include "bsp_gpio.h"
#include "bsp_usart.h"
#include "bsp_timer.h"
#include "stm32f10x.h"
#include "section.h"
#include "perf.h"
static void platform_init(void)
{
    bsp_gpio_init();  /* Configure DS0 and retain the SWD debugger. */
    bsp_usart_init(); /* Bind PA9/PA10 to the interrupt/DMA transport. */
    bsp_timer_init(); /* Start DWT perf timing and the registered 1 ms interrupt source. */
}
REG_INIT(-20, platform_init)
REG_PERF_BASE_CNT(BSP_TIMER_PERF_COUNTER, (1.0f / 72000000.0f))
