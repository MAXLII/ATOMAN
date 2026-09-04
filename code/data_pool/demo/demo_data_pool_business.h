// SPDX-License-Identifier: MIT
/**
 * @file    demo_data_pool_business.h
 * @brief   Demo data-pool business interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Provide typed snapshots to demo business modules
 *          - Preserve read-only access to the private data-pool object
 *          - Keep business code independent from protocol storage
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Reads publish only a stable task-context snapshot
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

#ifndef DEMO_DATA_POOL_BUSINESS_H
#define DEMO_DATA_POOL_BUSINESS_H

#include "demo_data_pool_types.h"

void demo_data_pool_business_read(demo_data_pool_snapshot_t *p_snapshot);

#endif /* DEMO_DATA_POOL_BUSINESS_H */
