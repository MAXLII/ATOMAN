// SPDX-License-Identifier: MIT
/**
 * @file    trace.c
 * @brief   Execution trace module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Record source-line trace marks with timestamps from a bound system-time counter
 *          - Store trace records in a fixed static buffer and stop when the buffer is full
 *          - Expose clear and read-only buffer access APIs for trace_service.c
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
    uint32_t write_index;             /* Next buffer index to write. */
    uint8_t is_full;                  /* Recording stop flag after the buffer is filled once. */
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

    /* Stop recording when the buffer is full or time source is not bound. */
    if ((g_dbg_trace_ctx.p_system_time == NULL) ||
        (g_dbg_trace_ctx.is_full != 0U) ||
        (g_dbg_trace_ctx.write_index >= DBG_TRACE_BUFFER_SIZE))
    {
        return;
    }

    p_item = &g_dbg_trace_buffer[g_dbg_trace_ctx.write_index];
    p_item->line = line;
    p_item->time = *(g_dbg_trace_ctx.p_system_time);

    g_dbg_trace_ctx.write_index++;
    /* Once the last slot is written, lock the recorder to keep existing data intact. */
    if (g_dbg_trace_ctx.write_index >= DBG_TRACE_BUFFER_SIZE)
    {
        g_dbg_trace_ctx.is_full = 1U;
    }
}

void dbg_trace_clear(void)
{
    memset(g_dbg_trace_buffer, 0, sizeof(g_dbg_trace_buffer));
    g_dbg_trace_ctx.write_index = 0U;
    g_dbg_trace_ctx.is_full = 0U;
}

const dbg_trace_item_t *dbg_trace_buffer_get(void)
{
    return g_dbg_trace_buffer;
}

uint32_t dbg_trace_buffer_size_get(void)
{
    return DBG_TRACE_BUFFER_SIZE;
}
