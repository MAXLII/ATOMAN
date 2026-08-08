// SPDX-License-Identifier: MIT
/**
 * @file    systick.h
 * @brief   GD32E507 SysTick time-base interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Configure a 100 us SysTick interrupt
 *          - Expose the monotonic scheduler tick
 *          - Provide a blocking millisecond delay for platform initialization
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The tick counter is updated in SysTick_Handler
 *          - Hardware access uses CMSIS SysTick services
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

#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

void systick_config(void);
void delay_1ms(uint32_t count);
void delay_decrement(void);
uint32_t systick_gettime_100us(void);

#endif /* SYSTICK_H */
