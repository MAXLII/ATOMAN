// SPDX-License-Identifier: MIT
/**
 * @file    trace_core.h
 * @brief   Execution trace public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define trace record layout and trace buffer capacity
 *          - Expose APIs for time binding, trace insertion, clearing, and readback
 *          - Provide lightweight macros for binding a time source and writing into the FIFO trace buffer
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
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
#ifndef __TRACE_CORE_H__
#define __TRACE_CORE_H__

#include <stdint.h>

#define TRACE_ENABLE 1u

#if ((TRACE_ENABLE != 0u) && (TRACE_ENABLE != 1u))
#error "TRACE_ENABLE must be 0 or 1."
#endif

typedef struct
{
    uint32_t line; /* Source line number captured at the trace point. */
    uint32_t time; /* Snapshot of the bound system time counter. */
} dbg_trace_item_t;

#define DBG_TRACE_BUFFER_SIZE 64u

void dbg_trace_core_bind_time(volatile uint32_t *p_system_time);
void dbg_trace_core_record(uint32_t line);
void dbg_trace_core_clear(void);
const dbg_trace_item_t *dbg_trace_core_buffer_get(void);
uint32_t dbg_trace_core_buffer_size_get(void);
uint32_t dbg_trace_core_record_count_get(void);
const dbg_trace_item_t *dbg_trace_core_item_get(uint32_t index);
uint8_t dbg_trace_core_read(uint32_t *p_time, uint32_t *p_line);

#endif /* __TRACE_CORE_H__ */
