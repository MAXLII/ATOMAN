// SPDX-License-Identifier: MIT
/**
 * @file    trace.c
 * @brief   Execution trace compatibility adapter implementation.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Preserve existing execution-trace function symbols
 *          - Forward trace storage operations to trace_core.c
 *          - Keep communication reporting outside the core implementation
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The portable implementation is available through trace_core.h
 *          - Protocol handling belongs to trace_service.c
 *
 * @author  Max.Li
 * @date    2026-08-02
 * @version 2.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "trace.h"

void dbg_trace_bind_time(volatile uint32_t *p_system_time)
{
    dbg_trace_core_bind_time(p_system_time);
}

void dbg_trace_record(uint32_t line)
{
    dbg_trace_core_record(line);
}

void dbg_trace_clear(void)
{
    dbg_trace_core_clear();
}

const dbg_trace_item_t *dbg_trace_buffer_get(void)
{
    return dbg_trace_core_buffer_get();
}

uint32_t dbg_trace_buffer_size_get(void)
{
    return dbg_trace_core_buffer_size_get();
}

uint32_t dbg_trace_record_count_get(void)
{
    return dbg_trace_core_record_count_get();
}

const dbg_trace_item_t *dbg_trace_item_get(uint32_t index)
{
    return dbg_trace_core_item_get(index);
}

uint8_t dbg_trace_read(uint32_t *p_time, uint32_t *p_line)
{
    return dbg_trace_core_read(p_time, p_line);
}
