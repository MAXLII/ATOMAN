// SPDX-License-Identifier: MIT
/**
 * @file    demo_jitter.h
 * @brief   SRTOS interrupt jitter test interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Expose the TIMER2 ISR timestamp capture hook
 *          - Publish RAM-resident jitter capture data for debugger extraction
 *          - Keep the jitter stress test optional through compile-time switches
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR path writes fixed-size RAM records only
 *          - Hardware access is limited to the GD32 timer counter used as timestamp source
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

#ifndef __DEMO_JITTER_H__
#define __DEMO_JITTER_H__

#include <stdint.h>

#ifndef DEMO_JITTER_CAPTURE_ENABLE
#define DEMO_JITTER_CAPTURE_ENABLE 0u
#endif

#ifndef DEMO_JITTER_SAMPLE_COUNT
#define DEMO_JITTER_SAMPLE_COUNT 1024u
#endif

typedef struct
{
    uint32_t write_index;
    uint32_t wrapped_count;
    uint32_t sample_count;
    uint32_t timer2_counter_hz;
    uint32_t timer2_hz;
    uint32_t min_entry_count;
    uint32_t max_entry_count;
    uint32_t max_entry_timestamp;
    uint32_t last_entry_count;
    uint32_t timer2_period_ticks;
} demo_jitter_debug_t;

extern volatile demo_jitter_debug_t g_demo_jitter_debug;
extern volatile uint32_t g_demo_jitter_timer2_count[DEMO_JITTER_SAMPLE_COUNT];

void demo_jitter_timer2_isr_entry(void);

#endif /* __DEMO_JITTER_H__ */
