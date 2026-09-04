// SPDX-License-Identifier: MIT
/**
 * @file    demo_data_pool.c
 * @brief   Demo data-pool storage implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Own the single private demo shared-data object
 *          - Implement the exchange-side snapshot write
 *          - Implement the business-side snapshot read
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - One exchange writer publishes through an odd/even sequence counter
 *          - Readers keep their previous snapshot while a write is in progress
 *          - Static initialization establishes the initial zero snapshot
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

#include "demo_data_pool_business.h"
#include "demo_data_pool_exchange.h"

#include <stddef.h>

typedef struct
{
    volatile uint32_t sequence;
    volatile uint32_t counter;
    volatile uint8_t led_mask;
    volatile int16_t temperature_x10;
} demo_data_pool_storage_t;

static demo_data_pool_storage_t s_demo_data_pool;

void demo_data_pool_exchange_write(const demo_data_pool_snapshot_t *p_snapshot)
{
    if (p_snapshot == NULL)
    {
        return;
    }

    s_demo_data_pool.sequence++;
    s_demo_data_pool.counter = p_snapshot->counter;
    s_demo_data_pool.led_mask = p_snapshot->led_mask;
    s_demo_data_pool.temperature_x10 = p_snapshot->temperature_x10;
    s_demo_data_pool.sequence++;
}

void demo_data_pool_business_read(demo_data_pool_snapshot_t *p_snapshot)
{
    demo_data_pool_snapshot_t snapshot;
    uint32_t sequence_begin;
    uint32_t sequence_end;

    if (p_snapshot == NULL)
    {
        return;
    }

    sequence_begin = s_demo_data_pool.sequence;
    if ((sequence_begin & 1u) != 0u)
    {
        return;
    }

    snapshot.counter = s_demo_data_pool.counter;
    snapshot.led_mask = s_demo_data_pool.led_mask;
    snapshot.temperature_x10 = s_demo_data_pool.temperature_x10;
    sequence_end = s_demo_data_pool.sequence;

    if (sequence_begin == sequence_end)
    {
        *p_snapshot = snapshot;
    }
}
