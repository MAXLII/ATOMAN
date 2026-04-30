// SPDX-License-Identifier: MIT
/**
 * @file    time_share.c
 * @brief   time_share library module.
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
#include "time_share.h"
#include "stddef.h"

void time_share_init(time_share_t *p_str,
                     time_share_func_table_t *p_func_table,
                     uint32_t table_size)
{
    p_str->p_func_table = p_func_table;
    p_str->table_size = table_size;
    p_str->idle_dn_cnt = 0;
    p_str->table_index = 0;
}

void time_share_func(time_share_t *p_str)
{
    if ((p_str == NULL) ||
        (p_str->p_func_table == NULL))
    {
        return;
    }

    for (uint32_t i = 0; i < p_str->table_size; i++)
    {
        p_str->p_func_table[i].cnt++;
    }

    if (p_str->idle_dn_cnt == 0)
    {
        for (uint32_t i = 0; i < p_str->table_size; i++)
        {
            p_str->table_index = (p_str->table_index + 1) % p_str->table_size;
            if (p_str->p_func_table[p_str->table_index].cnt >= p_str->p_func_table[p_str->table_index].period)
            {
                p_str->p_func_table[p_str->table_index].cnt = 0;
                p_str->p_func_table[p_str->table_index].func();
                p_str->idle_dn_cnt = 9;
                break;
            }
        }
    }
    else
    {
        if (p_str->idle_dn_cnt)
        {
            p_str->idle_dn_cnt--;
        }
    }
}
