// SPDX-License-Identifier: MIT
/**
 * @file    demo_data_pool_types.h
 * @brief   Demo data-pool value types.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Define the shared demo command snapshot
 *          - Keep protocol-independent value semantics
 *          - Provide one type to the exchange and business APIs
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Values are copied as complete task-context snapshots
 *          - This header has no Platform or framework dependency
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

#ifndef DEMO_DATA_POOL_TYPES_H
#define DEMO_DATA_POOL_TYPES_H

#include <stdint.h>

typedef struct
{
    uint32_t counter;
    uint8_t led_mask;
    int16_t temperature_x10;
} demo_data_pool_snapshot_t;

#endif /* DEMO_DATA_POOL_TYPES_H */
