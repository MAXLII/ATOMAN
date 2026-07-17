// SPDX-License-Identifier: MIT
/**
 * @file    bsp_timer.h
 * @brief   Zynq-7020 section time-base interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Initialize the Cortex-A9 global-timer time base
 *          - Expose a monotonic 100 us tick for section scheduling
 *          - Keep Xilinx timer types out of shared framework headers
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The read path is safe in task and IRQ context
 *          - Hardware access is isolated in the Zynq BSP
 *
 * @author  Max.Li
 * @date    2026-07-17
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef BSP_TIMER_H
#define BSP_TIMER_H

#include <stdint.h>

void bsp_timer_init(void);
uint32_t bsp_timer_gettime_100us(void);
int32_t bsp_timer_interrupt_start(uint32_t frequency_hz);

#endif /* BSP_TIMER_H */
