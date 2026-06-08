// SPDX-License-Identifier: MIT
/**
 * @file    bsp_timer.h
 * @brief   HC32F558 timer BSP interface.
 * @details
 *          This file is part of the HC32F558 project.
 *
 *          Module responsibilities:
 *          - Declare the 32-bit Timer6 perf counter initialization interface
 *          - Expose a direct read helper for the registered perf counter
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe reads use the hardware 32-bit counter register
 *          - Hardware access is abstracted through HC32 LL timer APIs
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

#ifndef BSP_TIMER_H
#define BSP_TIMER_H

#include <stdint.h>

int32_t bsp_timer_init(void);
uint32_t bsp_timer_get_perf_cnt(void);

#endif /* BSP_TIMER_H */
