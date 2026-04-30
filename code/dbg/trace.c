// SPDX-License-Identifier: MIT
/**
 * @file    trace.c
 * @brief   Execution trace module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Record source-line trace marks with timestamps from a bound system-time counter
 *          - Store trace records in a fixed FIFO-style circular buffer with monotonic read/write counters
 *          - Expose clear and one-record-at-a-time readback APIs for trace_service.c
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-04-30
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "trace.h"

#include <string.h>

typedef struct
{
    volatile uint32_t *p_system_time; /* External time counter used for trace timestamps. */
    uint32_t write_count;             /* Total number of records written. */
    uint32_t read_count;              /* Total number of records consumed by the read API. */
} dbg_trace_ctx_t;

static dbg_trace_item_t g_dbg_trace_buffer[DBG_TRACE_BUFFER_SIZE] = {0}; /* Fixed trace storage buffer. */
static dbg_trace_ctx_t g_dbg_trace_ctx = {0};                            /* Runtime state for trace recording. */

void dbg_trace_bind_time(volatile uint32_t *p_system_time)
{
    g_dbg_trace_ctx.p_system_time = p_system_time;
}

void dbg_trace_record(uint32_t line)
{
    dbg_trace_item_t *p_item = NULL; /* Target record slot for the current trace mark. */
    uint32_t write_index;

    if (g_dbg_trace_ctx.p_system_time == NULL)
    {
        return;
    }

    write_index = g_dbg_trace_ctx.write_count % DBG_TRACE_BUFFER_SIZE;
    p_item = &g_dbg_trace_buffer[write_index];
    p_item->line = line;
    p_item->time = *(g_dbg_trace_ctx.p_system_time);

    g_dbg_trace_ctx.write_count++;

    if ((g_dbg_trace_ctx.write_count - g_dbg_trace_ctx.read_count) > DBG_TRACE_BUFFER_SIZE)
    {
        g_dbg_trace_ctx.read_count = g_dbg_trace_ctx.write_count - DBG_TRACE_BUFFER_SIZE;
    }
}

void dbg_trace_clear(void)
{
    memset(g_dbg_trace_buffer, 0, sizeof(g_dbg_trace_buffer));
    g_dbg_trace_ctx.write_count = 0U;
    g_dbg_trace_ctx.read_count = 0U;
}

const dbg_trace_item_t *dbg_trace_buffer_get(void)
{
    return g_dbg_trace_buffer;
}

uint32_t dbg_trace_buffer_size_get(void)
{
    return DBG_TRACE_BUFFER_SIZE;
}

uint32_t dbg_trace_record_count_get(void)
{
    return g_dbg_trace_ctx.write_count - g_dbg_trace_ctx.read_count;
}

const dbg_trace_item_t *dbg_trace_item_get(uint32_t index)
{
    uint32_t buffer_index;
    uint32_t record_count = dbg_trace_record_count_get();

    if (index >= record_count)
    {
        return NULL;
    }

    buffer_index = (g_dbg_trace_ctx.read_count + index) % DBG_TRACE_BUFFER_SIZE;

    return &g_dbg_trace_buffer[buffer_index];
}

uint8_t dbg_trace_read(uint32_t *p_time, uint32_t *p_line)
{
    const dbg_trace_item_t *p_item;
    uint32_t read_index;

    if ((p_time == NULL) || (p_line == NULL))
    {
        return 0U;
    }

    if (g_dbg_trace_ctx.read_count == g_dbg_trace_ctx.write_count)
    {
        return 0U;
    }

    if ((g_dbg_trace_ctx.write_count - g_dbg_trace_ctx.read_count) > DBG_TRACE_BUFFER_SIZE)
    {
        g_dbg_trace_ctx.read_count = g_dbg_trace_ctx.write_count - DBG_TRACE_BUFFER_SIZE;
    }

    read_index = g_dbg_trace_ctx.read_count % DBG_TRACE_BUFFER_SIZE;
    p_item = &g_dbg_trace_buffer[read_index];
    *p_time = p_item->time;
    *p_line = p_item->line;
    g_dbg_trace_ctx.read_count++;

    return 1U;
}
