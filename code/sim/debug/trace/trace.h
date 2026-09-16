// SPDX-License-Identifier: MIT
/**
 * @file    trace.h
 * @brief   Execution trace compatibility adapter.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Preserve existing execution-trace APIs and instrumentation macros
 *          - Forward trace storage operations to the portable Trace core
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
#ifndef __TRACE_H__
#define __TRACE_H__

#include "trace_core.h"

void dbg_trace_bind_time(volatile uint32_t *p_system_time);
void dbg_trace_record(uint32_t line);
void dbg_trace_clear(void);
const dbg_trace_item_t *dbg_trace_buffer_get(void);
uint32_t dbg_trace_buffer_size_get(void);
uint32_t dbg_trace_record_count_get(void);
const dbg_trace_item_t *dbg_trace_item_get(uint32_t index);
uint8_t dbg_trace_read(uint32_t *p_time, uint32_t *p_line);

#if (TRACE_ENABLE == 1u)
#define DBG_TRACE_BIND_TIME(p_system_time) \
    dbg_trace_bind_time((volatile uint32_t *)(p_system_time))

#define DBG_TRACE_MARK()            \
    do                              \
    {                               \
        dbg_trace_record(__LINE__); \
    } while (0)
#else
#define DBG_TRACE_BIND_TIME(p_system_time) ((void)0)
#define DBG_TRACE_MARK() ((void)0)
#endif

#endif /* __TRACE_H__ */
