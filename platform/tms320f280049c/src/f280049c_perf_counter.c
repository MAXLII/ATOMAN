// SPDX-License-Identifier: MIT
/**
 * @file    f280049c_perf_counter.c
 * @brief   F280049C performance-counter registration.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Bind Perf to ERAD Counter1 current-count storage
 *          - Describe the counter as one 100 MHz CPU cycle per count
 *          - Register the hardware time base through SECTION_PERF
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ERAD Counter1 is configured before SECTION initialization
 *          - Hardware setup remains in the platform port
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

#include "perf.h"

#define F280049C_PERF_COUNTER_PERIOD_S (0.00000001f)
#define F280049C_PERF_COUNTER_ADDRESS \
    ((volatile uint32_t *)(ERAD_COUNTER1_BASE + ERAD_O_CTM_COUNT))

REG_PERF_BASE_CNT(F280049C_PERF_COUNTER_ADDRESS, F280049C_PERF_COUNTER_PERIOD_S)
