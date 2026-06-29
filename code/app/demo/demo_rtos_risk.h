// SPDX-License-Identifier: MIT
/**
 * @file    demo_rtos_risk.h
 * @brief   SRTOS risk-oriented test interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Expose RAM-resident counters for SRTOS risk tests
 *          - Keep risk tests disabled by default through compile-time switches
 *          - Provide debugger-readable evidence without changing SRTOS internals
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-30
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef __DEMO_RTOS_RISK_H__
#define __DEMO_RTOS_RISK_H__

#include <stdint.h>

#ifndef DEMO_RTOS_RISK_TEST_ENABLE
#define DEMO_RTOS_RISK_TEST_ENABLE 0u
#endif

#define DEMO_RTOS_RISK_ORDER_LOG_SIZE 64u

typedef struct
{
    uint32_t fast_1tick_count;
    uint32_t fast_2tick_count;
    uint32_t fast_3tick_count;
    uint32_t same_period_count[8];
    uint32_t same_period_order_error;
    uint32_t same_period_epoch;
    uint32_t variable_enter_count;
    uint32_t variable_short_count;
    uint32_t variable_long_count;
    uint32_t variable_error_count;
    uint32_t variable_max_run_ticks;
    uint32_t long_enter_count;
    uint32_t long_complete_count;
    uint32_t long_error_count;
    uint32_t long_max_run_ticks;
    uint32_t nested_count;
    uint32_t nested_error_count;
    uint32_t stack_count;
    uint32_t stack_error_count;
    uint32_t float_count;
    uint32_t float_error_count;
    uint32_t float_bits;
    uint32_t monitor_count;
    uint32_t min_fast_delta;
    uint32_t max_fast_delta;
    uint32_t ready_fairness_error;
    uint32_t start_tick;
    uint32_t last_tick;
    uint32_t order_index;
    uint32_t order_log[DEMO_RTOS_RISK_ORDER_LOG_SIZE];
} demo_rtos_risk_debug_t;

extern volatile demo_rtos_risk_debug_t g_demo_rtos_risk_debug;

#endif /* __DEMO_RTOS_RISK_H__ */
