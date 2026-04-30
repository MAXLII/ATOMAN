// SPDX-License-Identifier: MIT
/**
 * @file    trace.h
 * @brief   Execution trace public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define trace record data structures
 *          - Expose APIs for time binding, record insertion, clearing, and readback
 *          - Provide macros for binding time and recording source line marks
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
#ifndef __TRACE_H__
#define __TRACE_H__

#include <stdint.h>

typedef struct
{
    uint32_t line; /* Source line number captured at the trace point. */
    uint32_t time; /* Snapshot of the bound system time counter. */
} dbg_trace_item_t;

#define DBG_TRACE_BUFFER_SIZE 500u

void dbg_trace_bind_time(volatile uint32_t *p_system_time);
void dbg_trace_record(uint32_t line);
void dbg_trace_clear(void);
const dbg_trace_item_t *dbg_trace_buffer_get(void);
uint32_t dbg_trace_buffer_size_get(void);

/* Bind the external system time counter used by all trace records. */
#define DBG_TRACE_BIND_TIME(p_system_time) \
    dbg_trace_bind_time((volatile uint32_t *)(p_system_time))

/* Record the current source line into the internal trace buffer. */
#define DBG_TRACE_MARK()            \
    do                              \
    {                               \
        dbg_trace_record(__LINE__); \
    } while (0)

#endif
