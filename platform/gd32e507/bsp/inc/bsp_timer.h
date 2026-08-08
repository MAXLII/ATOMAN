// SPDX-License-Identifier: MIT
/**
 * @file    bsp_timer.h
 * @brief   GD32E507 demo timer BSP interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Expose initialization of the Perf counter
 *          - Expose initialization of the 10 kHz demo interrupt source
 *          - Keep timer selection local to the GD32E507 platform
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - TIMER1 is free-running and TIMER2 generates interrupts
 *          - Hardware access uses the GD32E50x standard peripheral library
 *
 * @author  Max.Li
 * @date    2026-08-08
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

void bsp_timer_init(void);

#endif /* BSP_TIMER_H */
