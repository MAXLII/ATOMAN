/**
 * Copyright (c) 2026
 *
 * @file trace.c
 * @brief Execution trace implementation for recording, step-printing, and clearing trace data.
 * @author Max.Li
 * @date 2026-03-28
 *
 * @par Revision History
 * 2026-03-28 Max.Li
 * - Created the execution trace module and added record, print, and clear support.
 */

#include "trace.h"

#include <string.h>

#include "section.h"
#include "shell.h"
#include "usart.h"

EXT_LINK(USART0_LINK);

typedef struct
{
    volatile uint32_t *p_system_time; /* External time counter used for trace timestamps. */
    uint32_t write_index;             /* Next buffer index to write. */
    uint8_t is_full;                  /* Recording stop flag after the buffer is filled once. */
} dbg_trace_ctx_t;

typedef struct
{
    uint8_t active;      /* Deferred print enable flag. */
    uint32_t read_index; /* Next trace record index to print. */
} dbg_trace_print_ctx_t;

static dbg_trace_item_t g_dbg_trace_buffer[DBG_TRACE_BUFFER_SIZE] = {0}; /* Fixed trace storage buffer. */
static dbg_trace_ctx_t g_dbg_trace_ctx = {0};                            /* Runtime state for trace recording. */
static dbg_trace_print_ctx_t g_dbg_trace_print_ctx = {0};                /* Runtime state for deferred printing. */

/* Print one trace item in the required "time\tline" format. */
#define DBG_TRACE_PRINT_ITEM(p_link_printf, p_item)                                                                        \
    do                                                                                                                     \
    {                                                                                                                      \
        if (((p_link_printf) != NULL) && ((p_link_printf)->my_printf != NULL))                                             \
        {                                                                                                                  \
            (p_link_printf)->my_printf("%lu\t%lu \r\n", (unsigned long)((p_item)->time), (unsigned long)((p_item)->line)); \
        }                                                                                                                  \
    } while (0)

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
    /* Clearing the buffer also stops any ongoing deferred print session. */
    memset(g_dbg_trace_buffer, 0, sizeof(g_dbg_trace_buffer));
    g_dbg_trace_ctx.write_index = 0U;
    g_dbg_trace_ctx.is_full = 0U;
    g_dbg_trace_print_ctx.active = 0U;
    g_dbg_trace_print_ctx.read_index = 0U;
}

static void dbg_trace_print_start(DEC_MY_PRINTF)
{
    (void)my_printf;
    /* The actual output is handled by the periodic task to avoid burst printing. */
    g_dbg_trace_print_ctx.active = 1U;
    g_dbg_trace_print_ctx.read_index = 0U;
}

static void dbg_trace_clear_cmd(DEC_MY_PRINTF)
{
    (void)my_printf;
    dbg_trace_clear();
}

static void dbg_trace_print_task(void)
{
    dbg_trace_item_t *p_item = NULL;                                  /* Current record to print. */
    section_link_tx_func_t *p_link_printf = LINK_PRINTF(USART0_LINK); /* UART print interface used for shell output. */

    if (g_dbg_trace_print_ctx.active == 0U)
    {
        return;
    }

    if (g_dbg_trace_print_ctx.read_index >= DBG_TRACE_BUFFER_SIZE)
    {
        g_dbg_trace_print_ctx.active = 0U;
        return;
    }

    p_item = &g_dbg_trace_buffer[g_dbg_trace_print_ctx.read_index];
    /* A zeroed record is treated as the end of valid trace data. */
    if ((p_item->line == 0U) && (p_item->time == 0U))
    {
        g_dbg_trace_print_ctx.active = 0U;
        return;
    }

    DBG_TRACE_PRINT_ITEM(p_link_printf, p_item);
    g_dbg_trace_print_ctx.read_index++;
}

/* Shell command: start deferred trace printing. */
REG_SHELL_CMD(dbg_trace_print, dbg_trace_print_start)
/* Shell command: clear all trace records. */
REG_SHELL_CMD(dbg_trace_clear, dbg_trace_clear_cmd)
/* Print one trace record every 50 ms after the shell command is triggered. */
REG_TASK_MS(50, dbg_trace_print_task)
