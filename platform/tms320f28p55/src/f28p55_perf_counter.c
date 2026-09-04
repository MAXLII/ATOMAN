// SPDX-License-Identifier: MIT
/**
 * @file    f28p55_perf_counter.c
 * @brief   F28P55 performance-counter registration.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Bind Perf to ERAD Counter1 current-count storage
 *          - Describe the counter as one 150 MHz CPU cycle per count
 *          - Register the hardware time base through SECTION_PERF
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ERAD Counter1 is configured before SECTION initialization
 *          - Hardware setup remains in the platform port
 *
 * @author  Max.Li
 * @date    2026-09-04
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "perf.h"

#define F28P55_PERF_COUNTER_PERIOD_S (0.0000000066666667f)
#define F28P55_PERF_COUNTER_ADDRESS \
    ((volatile uint32_t *)(ERAD_COUNTER1_BASE + ERAD_O_CTM_COUNT))

REG_PERF_BASE_CNT(F28P55_PERF_COUNTER_ADDRESS, F28P55_PERF_COUNTER_PERIOD_S)
