// SPDX-License-Identifier: MIT
/**
 * @file    frame_trace_service.c
 * @brief   C28x-safe FRAME execution-trace protocol service.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Control execution-trace reporting through FRAME
 *          - Bind trace timestamps directly to the 100 us hardware tick
 *          - Serialize trace records as explicit eight-bit little-endian fields
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Trace records are emitted outside interrupt context
 *          - Native C28x structures are never used as physical wire layouts
 *
 * @author  Max.Li
 * @date    2026-09-05
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "comm.h"
#include "f280049c_wire.h"
#include "section.h"
#include "trace_service.h"

#define FRAME_TRACE_CONTROL_REQUEST_SIZE (1u)
#define FRAME_TRACE_CONTROL_ACK_SIZE (4u)
#define FRAME_TRACE_RECORD_SIZE (6u)
#define FRAME_TRACE_REPORTS_PER_TASK (1u)

typedef struct
{
    uint8_t running;                   /* Nonzero while binary reports are enabled. */
    section_link_tx_func_t *p_output;  /* Link selected by the latest control request. */
    uint8_t source;                    /* Device source address. */
    uint8_t dynamic_source;            /* Device dynamic source address. */
    uint8_t destination;               /* FRAME host destination address. */
    uint8_t dynamic_destination;       /* FRAME host dynamic destination address. */
} frame_trace_context_t;

static frame_trace_context_t s_trace_context = {0}; /* Active binary-report route. */

static void frame_trace_send(uint8_t command_word,
                             uint8_t is_ack,
                             wire_octet_t *p_payload,
                             uint16_t payload_length)
{
    section_packform_t report = {0}; /* FRAME response or asynchronous report. */

    if (s_trace_context.p_output == NULL)
    {
        return;
    }

    report.src = s_trace_context.source;
    report.d_src = s_trace_context.dynamic_source;
    report.dst = s_trace_context.destination;
    report.d_dst = s_trace_context.dynamic_destination;
    report.cmd_set = TRACE_SERVICE_CMD_SET;
    report.cmd_word = command_word;
    report.is_ack = is_ack;
    report.len = payload_length;
    report.p_data = p_payload;
    comm_send_data(&report, s_trace_context.p_output);
}

static void frame_trace_control_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    wire_octet_t payload[FRAME_TRACE_CONTROL_ACK_SIZE] = {0}; /* Control acknowledgement. */

    if ((p_pack == NULL) || (p_pack->is_ack != 0u) ||
        (p_pack->p_data == NULL) ||
        (p_pack->len < FRAME_TRACE_CONTROL_REQUEST_SIZE))
    {
        return;
    }

    s_trace_context.p_output = my_printf;
    s_trace_context.source = p_pack->dst;
    s_trace_context.dynamic_source = p_pack->d_dst;
    s_trace_context.destination = p_pack->src;
    s_trace_context.dynamic_destination = p_pack->d_src;
    s_trace_context.running = (wire_octet_get(p_pack->p_data[0]) != 0u) ? 1u : 0u;

    payload[0] = 1u;
    payload[1] = s_trace_context.running;
    wire_u16_le_write(&payload[2], TRACE_SERVICE_TIME_UNIT_US);
    frame_trace_send(TRACE_SERVICE_CMD_CONTROL,
                     1u,
                     payload,
                     FRAME_TRACE_CONTROL_ACK_SIZE);
}

REG_COMM(TRACE_SERVICE_CMD_SET, TRACE_SERVICE_CMD_CONTROL, frame_trace_control_act)

static void frame_trace_report_task(void)
{
    wire_octet_t payload[FRAME_TRACE_RECORD_SIZE] = {0}; /* One serialized trace record. */
    uint32_t time_value = 0u; /* Captured Trace timestamp. */
    uint32_t line_value = 0u; /* Captured source line number. */
    uint16_t report_count = 0u; /* Reports emitted during this task activation. */

    if ((s_trace_context.running == 0u) || (s_trace_context.p_output == NULL))
    {
        return;
    }

    while ((report_count < FRAME_TRACE_REPORTS_PER_TASK) &&
           (dbg_trace_read(&time_value, &line_value) != 0u))
    {
        wire_u32_le_write(payload, time_value);
        wire_u16_le_write(&payload[4], (uint16_t)line_value);
        frame_trace_send(TRACE_SERVICE_CMD_RECORD_REPORT,
                         0u,
                         payload,
                         FRAME_TRACE_RECORD_SIZE);
        report_count++;
    }
}

REG_TASK_MS(1u, frame_trace_report_task)

static void frame_trace_init(void)
{
    dbg_trace_bind_time(tms320f280049c_section_tick_address_get());
    dbg_trace_clear();
}

REG_INIT(1u, frame_trace_init)
