// SPDX-License-Identifier: MIT
/**
 * @file    time_share.h
 * @brief   time_share library public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Provide cooperative time-sharing for a table of periodic callbacks
 *          - Track per-callback counters and execute due functions in round-robin order
 *          - Throttle callback dispatch with an idle countdown to spread workload
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __TIME_SHARE_H
#define __TIME_SHARE_H

#include "stdint.h"

#define TIME_SHARE_REG_FUNC(_func, _period) { \
    .func = _func,                            \
    .period = _period,                        \
    .cnt = 0,                                 \
}

typedef struct
{
    void (*func)(void);
    uint32_t period;
    uint32_t cnt;
} time_share_func_table_t;

typedef struct
{
    time_share_func_table_t *p_func_table;
    uint32_t table_size;
    uint32_t table_index;
    uint32_t idle_dn_cnt;
} time_share_t;

void time_share_init(time_share_t *p_str,
                     time_share_func_table_t *p_func_table,
                     uint32_t table_size);

void time_share_func(time_share_t *p_str);

#endif
