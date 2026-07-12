// SPDX-License-Identifier: MIT
/**
 * @file    systick.h
 * @brief   HC32F558 SysTick timing interface.
 * @details
 *          This file is part of the HC32F558 project.
 *
 *          Module responsibilities:
 *          - Declare the 100 us system tick configuration interface
 *          - Declare delay counter maintenance for SysTick_Handler
 *          - Expose the framework tick counter used by section scheduling
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - delay_decrement() is called from SysTick ISR context
 *          - Hardware access is abstracted through CMSIS SysTick APIs
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

#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

void systick_config(void);
void delay_1ms(uint32_t count);
void delay_decrement(void);
uint32_t systick_gettime_100us(void);

#endif /* SYSTICK_H */

