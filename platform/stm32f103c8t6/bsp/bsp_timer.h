// SPDX-License-Identifier: MIT
/**
 * @file bsp_timer.h
 * @brief Board interrupt timer interface.
 * @details
 *          This file is part of the digital power framework project.
 *          Module responsibilities:
 *          - Configure TIM2 to drive registered 1 ms SECTION interrupts
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
#ifndef BSP_TIMER_H
#define BSP_TIMER_H
#include <stdint.h>
/* Cortex-M3 DWT CYCCNT register address; CMSIS 1.30 omits DWT definitions. */
#define BSP_TIMER_PERF_COUNTER ((volatile uint32_t *)(uintptr_t)0xE0001004u)
void bsp_timer_init(void);
/** @brief Restore DWT gates that a debugger may clear when it disconnects. */
void bsp_timer_perf_resume(void);
#endif /* BSP_TIMER_H */
