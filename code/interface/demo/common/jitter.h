// SPDX-License-Identifier: MIT
/**
 * @file    jitter.h
 * @brief   Demo timer-observation Interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Expose the timer count sampled by the demo jitter probe
 *          - Expose the timer clock and programmed period
 *          - Keep MCU timer registers outside demo business code
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Read functions are used from the timer interrupt path
 *          - Hardware access is implemented by the selected Platform Interface
 *
 * @author  Max.Li
 * @date    2026-09-05
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef DEMO_JITTER_INTERFACE_H
#define DEMO_JITTER_INTERFACE_H

#include <stdint.h>

uint32_t demo_jitter_timer_count_get(void);
uint32_t demo_jitter_timer_clock_hz_get(void);
uint32_t demo_jitter_timer_period_ticks_get(void);

#endif /* DEMO_JITTER_INTERFACE_H */
