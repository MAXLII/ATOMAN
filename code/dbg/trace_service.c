// SPDX-License-Identifier: MIT
/**
 * @file    trace_service.c
 * @brief   Execution trace service module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Provide deferred shell printing for trace records
 *          - Provide shell command handling for trace clear
 *          - Keep output and command registration outside the trace core
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "trace_service.h"

#include "section.h"
#include "shell.h"
#include "trace.h"

typedef struct
{
    uint8_t active;
    uint32_t read_index;
    DEC_MY_PRINTF;
} dbg_trace_print_ctx_t;

static dbg_trace_print_ctx_t g_dbg_trace_print_ctx = {0};

#define DBG_TRACE_PRINT_ITEM(p_link_printf, p_item)                                                                        \
    do                                                                                                                     \
    {                                                                                                                      \
        if (((p_link_printf) != NULL) && ((p_link_printf)->my_printf != NULL))                                             \
        {                                                                                                                  \
            (p_link_printf)->my_printf("%lu\t%lu \r\n", (unsigned long)((p_item)->time), (unsigned long)((p_item)->line)); \
        }                                                                                                                  \
    } while (0)

static void dbg_trace_print_start(DEC_MY_PRINTF)
{
    g_dbg_trace_print_ctx.my_printf = my_printf;
    g_dbg_trace_print_ctx.active = 1u;
    g_dbg_trace_print_ctx.read_index = 0u;
}

static void dbg_trace_clear_cmd(DEC_MY_PRINTF)
{
    (void)my_printf;
    dbg_trace_clear();
    g_dbg_trace_print_ctx.active = 0u;
    g_dbg_trace_print_ctx.read_index = 0u;
    g_dbg_trace_print_ctx.my_printf = NULL;
}

void dbg_trace_service_print_task(void)
{
    const dbg_trace_item_t *p_buffer;
    const dbg_trace_item_t *p_item;
    section_link_tx_func_t *p_link_printf = g_dbg_trace_print_ctx.my_printf;

    if (g_dbg_trace_print_ctx.active == 0u)
    {
        return;
    }

    if (g_dbg_trace_print_ctx.read_index >= dbg_trace_buffer_size_get())
    {
        g_dbg_trace_print_ctx.active = 0u;
        g_dbg_trace_print_ctx.my_printf = NULL;
        return;
    }

    p_buffer = dbg_trace_buffer_get();
    if (p_buffer == NULL)
    {
        g_dbg_trace_print_ctx.active = 0u;
        g_dbg_trace_print_ctx.my_printf = NULL;
        return;
    }

    p_item = &p_buffer[g_dbg_trace_print_ctx.read_index];
    if ((p_item->line == 0u) && (p_item->time == 0u))
    {
        g_dbg_trace_print_ctx.active = 0u;
        g_dbg_trace_print_ctx.my_printf = NULL;
        return;
    }

    DBG_TRACE_PRINT_ITEM(p_link_printf, p_item);
    ++g_dbg_trace_print_ctx.read_index;
}

REG_SHELL_CMD(dbg_trace_print, dbg_trace_print_start)
REG_SHELL_CMD(dbg_trace_clear, dbg_trace_clear_cmd)
REG_TASK_MS(50, dbg_trace_service_print_task)
