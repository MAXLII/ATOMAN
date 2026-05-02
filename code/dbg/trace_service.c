// SPDX-License-Identifier: MIT
/**
 * @file    trace_service.c
 * @brief   Execution trace service module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Register shell commands for deferred trace printing and trace buffer clearing
 *          - Handle Trace binary control commands and report FIFO records in bounded batches
 *          - Keep shell, section task registration, and output formatting outside trace.c
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

#include "comm.h"
#include "section.h"
#include "shell.h"
#include "trace.h"

#include <string.h>

#pragma pack(push, 1)
typedef struct
{
    uint8_t enable;
} dbg_trace_control_req_t;

typedef struct
{
    uint8_t success;
    uint8_t running;
    uint16_t time_unit_us;
} dbg_trace_control_ack_t;

typedef struct
{
    uint32_t time;
    uint16_t line;
} dbg_trace_record_report_t;
#pragma pack(pop)

/* Binary transport context — always available */
typedef struct
{
    uint8_t running;
    DEC_MY_PRINTF;
    uint8_t src;
    uint8_t d_src;
    uint8_t dst;
    uint8_t d_dst;
} dbg_trace_binary_ctx_t;

static dbg_trace_binary_ctx_t g_dbg_trace_binary_ctx = {0};

#define DBG_TRACE_BINARY_MAX_REPORT_PER_TASK 3u

/* Printf helpers (gated by TRACE_SERVICE_PRINTF) */
#if TRACE_SERVICE_PRINTF == 1

typedef struct
{
    uint8_t active;
    DEC_MY_PRINTF;
} dbg_trace_print_ctx_t;

static dbg_trace_print_ctx_t g_dbg_trace_print_ctx = {0};

#define DBG_TRACE_PRINT_VALUE(p_link_printf, time_value, line_value)                                               \
    do                                                                                                            \
    {                                                                                                             \
        if (((p_link_printf) != NULL) && ((p_link_printf)->my_printf != NULL))                                    \
        {                                                                                                         \
            (p_link_printf)->my_printf("%lu\t%lu \r\n", (unsigned long)(time_value), (unsigned long)(line_value)); \
        }                                                                                                         \
    } while (0)

static void dbg_trace_print_start(DEC_MY_PRINTF)
{
    g_dbg_trace_print_ctx.my_printf = my_printf;
    g_dbg_trace_print_ctx.active = 1u;
}

static void dbg_trace_clear_cmd(DEC_MY_PRINTF)
{
    (void)my_printf;
    dbg_trace_clear();
    g_dbg_trace_print_ctx.active = 0u;
    g_dbg_trace_print_ctx.my_printf = NULL;
}

void dbg_trace_service_print_task(void)
{
    uint32_t time_value = 0u;
    uint32_t line_value = 0u;
    section_link_tx_func_t *p_link_printf = g_dbg_trace_print_ctx.my_printf;

    if (g_dbg_trace_print_ctx.active == 0u)
    {
        return;
    }

    if (dbg_trace_read(&time_value, &line_value) == 0u)
    {
        g_dbg_trace_print_ctx.active = 0u;
        g_dbg_trace_print_ctx.my_printf = NULL;
        return;
    }

    DBG_TRACE_PRINT_VALUE(p_link_printf, time_value, line_value);
}

REG_SHELL_CMD(dbg_trace_print, dbg_trace_print_start)
REG_SHELL_CMD(dbg_trace_clear, dbg_trace_clear_cmd)
REG_TASK_MS(50, dbg_trace_service_print_task)

#else

void dbg_trace_service_print_task(void)
{
}

#endif /* TRACE_SERVICE_PRINTF */

/* Binary protocol handlers */
static void dbg_trace_binary_capture_route(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    if (p_pack == NULL)
    {
        return;
    }

    g_dbg_trace_binary_ctx.my_printf = my_printf;
    g_dbg_trace_binary_ctx.src = p_pack->dst;
    g_dbg_trace_binary_ctx.d_src = p_pack->d_dst;
    g_dbg_trace_binary_ctx.dst = p_pack->src;
    g_dbg_trace_binary_ctx.d_dst = p_pack->d_src;
}

static void dbg_trace_binary_send(uint8_t cmd_word, uint8_t is_ack, uint8_t *p_data, uint16_t len)
{
    section_packform_t pack = {0};

    pack.src = g_dbg_trace_binary_ctx.src;
    pack.d_src = g_dbg_trace_binary_ctx.d_src;
    pack.dst = g_dbg_trace_binary_ctx.dst;
    pack.d_dst = g_dbg_trace_binary_ctx.d_dst;
    pack.cmd_set = TRACE_SERVICE_CMD_SET;
    pack.cmd_word = cmd_word;
    pack.is_ack = is_ack;
    pack.len = len;
    pack.p_data = p_data;

    comm_send_data(&pack, g_dbg_trace_binary_ctx.my_printf);
}

static void dbg_trace_control_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    dbg_trace_control_req_t req = {0};
    dbg_trace_control_ack_t ack = {0};
    uint16_t copy_len;

    if (p_pack == NULL)
    {
        return;
    }

    copy_len = (p_pack->len < (uint16_t)sizeof(req)) ? p_pack->len : (uint16_t)sizeof(req);
    if ((copy_len != 0u) && (p_pack->p_data != NULL))
    {
        memcpy((uint8_t *)&req, p_pack->p_data, copy_len);
    }

    dbg_trace_binary_capture_route(p_pack, my_printf);

    if (req.enable != 0u)
    {
        g_dbg_trace_binary_ctx.running = 1u;
    }
    else
    {
        g_dbg_trace_binary_ctx.running = 0u;
    }

    ack.success = 1u;
    ack.running = g_dbg_trace_binary_ctx.running;
    ack.time_unit_us = TRACE_SERVICE_TIME_UNIT_US;
    dbg_trace_binary_send(TRACE_SERVICE_CMD_CONTROL, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack));
}

void dbg_trace_service_binary_task(void)
{
    dbg_trace_record_report_t report = {0};
    uint32_t line_value = 0u;
    uint16_t report_count = 0u;

    if (g_dbg_trace_binary_ctx.running == 0u)
    {
        return;
    }

    while ((report_count < DBG_TRACE_BINARY_MAX_REPORT_PER_TASK) &&
           (dbg_trace_read(&report.time, &line_value) != 0u))
    {
        report.line = (uint16_t)line_value;
        dbg_trace_binary_send(TRACE_SERVICE_CMD_RECORD_REPORT, 0u, (uint8_t *)&report, (uint16_t)sizeof(report));
        report_count++;
    }
}

/* Registrations */
REG_TASK_MS(1, dbg_trace_service_binary_task)
REG_COMM(TRACE_SERVICE_CMD_SET, TRACE_SERVICE_CMD_CONTROL, dbg_trace_control_act)
