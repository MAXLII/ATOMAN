// SPDX-License-Identifier: MIT
/**
 * @file systick.h
 * @brief 100 us monotonic platform time base.
 * @details
 *          This file is part of the digital power framework project.
 *          Module responsibilities:
 *          - Supply SECTION tick and interrupt-facing increment operation
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
#ifndef SYSTICK_H
#define SYSTICK_H
#include <stdint.h>
void systick_config(void);
void delay_decrement(void);
/** @return Monotonic elapsed ticks, each representing 100 us. */
uint32_t systick_gettime_100us(void);
#endif /* SYSTICK_H */
