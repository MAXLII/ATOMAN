// SPDX-License-Identifier: MIT
/**
 * @file    demo_data_pool_exchange.h
 * @brief   Demo data-pool exchange interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Accept validated snapshots from protocol and data-source modules
 *          - Publish one complete command snapshot atomically at task level
 *          - Hide the private data-pool object from exchange clients
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - One exchange writer publishes snapshots in task context
 *          - The data pool does not access Platform resources
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

#ifndef DEMO_DATA_POOL_EXCHANGE_H
#define DEMO_DATA_POOL_EXCHANGE_H

#include "demo_data_pool_types.h"

void demo_data_pool_exchange_write(const demo_data_pool_snapshot_t *p_snapshot);

#endif /* DEMO_DATA_POOL_EXCHANGE_H */
