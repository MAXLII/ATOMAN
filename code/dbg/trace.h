/**
 * Copyright (c) 2026
 *
 * @file trace.h
 * @brief Execution trace interface for recording source line numbers with system time.
 * @author Max.Li
 * @date 2026-03-28
 *
 * @par Revision History
 * 2026-03-28 Max.Li
 * - Created the execution trace module with time binding, trace mark, and clear APIs.
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
