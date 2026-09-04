// SPDX-License-Identifier: MIT
/**
 * @file    frame_sfra_service.c
 * @brief   C28x-safe FRAME SFRA protocol service.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Expose registered SFRA instances and controls to FRAME
 *          - Cache and report completed frequency-response points
 *          - Serialize every protocol field as explicit eight-bit little-endian data
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Service polling and protocol handlers run outside interrupt context
 *          - Native C28x structures are never used as physical wire layouts
 *
 * @author  Max.Li
 * @date    2026-09-04
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "comm.h"
#include "f28p55_wire.h"
#include "sfra_service.h"

#define FRAME_SFRA_NAME_MAX (64u)
#define FRAME_SFRA_LIST_HEADER_SIZE (4u)
#define FRAME_SFRA_INFO_REQUEST_SIZE (4u)
#define FRAME_SFRA_INFO_ACK_SIZE (48u)
#define FRAME_SFRA_CONFIG_REQUEST_SIZE (16u)
#define FRAME_SFRA_CONTROL_REQUEST_SIZE (4u)
#define FRAME_SFRA_CONTROL_ACK_SIZE (20u)
#define FRAME_SFRA_POINT_REQUEST_SIZE (8u)
#define FRAME_SFRA_POINT_ACK_SIZE (24u)
#define FRAME_SFRA_FP32_EXPONENT_MASK (0x7F800000u)

typedef struct
{
    section_link_tx_func_t *p_output; /* Link used for asynchronous SFRA reports. */
    uint8_t active;                   /* Nonzero after a valid request captures a route. */
    uint8_t source;                   /* Device source address. */
    uint8_t dynamic_source;           /* Device dynamic source address. */
    uint8_t destination;              /* FRAME host destination address. */
    uint8_t dynamic_destination;      /* FRAME host dynamic destination address. */
} frame_sfra_route_t;

typedef struct
{
    section_item_t *p_item;           /* Next registration to report. */
    uint8_t index;                    /* Current list index. */
    uint8_t count;                    /* Valid SFRA registration count. */
    uint8_t active;                   /* Nonzero while list reports are pending. */
    frame_sfra_route_t route;         /* Request route retained for deferred ACKs. */
} frame_sfra_list_context_t;

static frame_sfra_route_t s_sfra_route = {0}; /* Active point/done report route. */
static frame_sfra_list_context_t s_sfra_list = {0}; /* Deferred list-report state. */

static uint8_t frame_sfra_name_length(const char *p_name)
{
    uint8_t length = 0u; /* Valid name characters found. */

    if (p_name == NULL)
    {
        return 0u;
    }
    while ((length < FRAME_SFRA_NAME_MAX) && (p_name[length] != '\0'))
    {
        length++;
    }
    return length;
}

static uint8_t frame_sfra_bits_are_finite(uint32_t bits)
{
    return ((bits & FRAME_SFRA_FP32_EXPONENT_MASK) != FRAME_SFRA_FP32_EXPONENT_MASK) ? 1u : 0u;
}

static void frame_sfra_route_capture(frame_sfra_route_t *p_route,
                                     const section_packform_t *p_request,
                                     DEC_MY_PRINTF)
{
    if ((p_route == NULL) || (p_request == NULL))
    {
        return;
    }

    p_route->p_output = my_printf;
    p_route->source = p_request->dst;
    p_route->dynamic_source = p_request->d_dst;
    p_route->destination = p_request->src;
    p_route->dynamic_destination = p_request->d_src;
    p_route->active = 1u;
}

static void frame_sfra_route_send(const frame_sfra_route_t *p_route,
                                  uint8_t command_word,
                                  uint8_t is_ack,
                                  wire_octet_t *p_payload,
                                  uint16_t payload_length)
{
    section_packform_t report = {0}; /* FRAME reply or asynchronous report metadata. */

    if ((p_route == NULL) || (p_route->active == 0u) ||
        (p_route->p_output == NULL))
    {
        return;
    }

    report.src = p_route->source;
    report.d_src = p_route->dynamic_source;
    report.dst = p_route->destination;
    report.d_dst = p_route->dynamic_destination;
    report.cmd_set = CMD_SET_SFRA;
    report.cmd_word = command_word;
    report.is_ack = is_ack;
    report.len = payload_length;
    report.p_data = p_payload;
    comm_send_data(&report, p_route->p_output);
}

static void frame_sfra_reply(const section_packform_t *p_request,
                             DEC_MY_PRINTF,
                             uint8_t command_word,
                             wire_octet_t *p_payload,
                             uint16_t payload_length)
{
    frame_sfra_route_t route = {0}; /* Request-scoped response route. */

    frame_sfra_route_capture(&route, p_request, my_printf);
    frame_sfra_route_send(&route, command_word, 1u, p_payload, payload_length);
}

static sfra_registration_t *frame_sfra_find(uint8_t sfra_id)
{
    section_item_t *p_item = p_sfra_first; /* Registration traversal cursor. */

    while (p_item != NULL)
    {
        sfra_registration_t *p_registration = (sfra_registration_t *)p_item->p_obj;

        if ((p_registration != NULL) && (p_registration->sfra_id == sfra_id))
        {
            return p_registration;
        }
        p_item = p_item->p_next;
    }
    return NULL;
}

static uint8_t frame_sfra_is_busy(const sfra_t *p_sfra)
{
    if (p_sfra == NULL)
    {
        return 0u;
    }
    return (uint8_t)((p_sfra->task.active != 0u) ||
                     (p_sfra->output.busy != 0u) ||
                     ((p_sfra->task.state != SFRA_STATE_IDLE) &&
                      (p_sfra->task.state != SFRA_STATE_DONE)));
}

static void frame_sfra_sweep_tag_increment(sfra_registration_t *p_registration)
{
    if (p_registration == NULL)
    {
        return;
    }
    p_registration->sweep_tag++;
    if (p_registration->sweep_tag == 0u)
    {
        p_registration->sweep_tag++;
    }
    p_registration->done_reported = 0u;
    p_registration->result_count = 0u;
}

static uint8_t frame_sfra_core_status(sfra_status_t status)
{
    switch (status)
    {
    case SFRA_STATUS_OK:
    case SFRA_STATUS_BUSY:
    case SFRA_STATUS_DONE:
        return (uint8_t)SFRA_TOOL_STATUS_OK;
    case SFRA_STATUS_INVALID_PARAM:
        return (uint8_t)SFRA_TOOL_STATUS_INVALID_PARAM;
    case SFRA_STATUS_NULL:
    default:
        return (uint8_t)SFRA_TOOL_STATUS_CORE_ERROR;
    }
}

static void frame_sfra_control_encode(wire_octet_t *p_payload,
                                      const sfra_registration_t *p_registration,
                                      uint8_t sfra_id,
                                      uint8_t status)
{
    uint16_t index; /* Payload octet cleared before encoding fields. */

    for (index = 0u; index < FRAME_SFRA_CONTROL_ACK_SIZE; index++)
    {
        p_payload[index] = 0u;
    }
    p_payload[0] = wire_octet_get(sfra_id);
    p_payload[1] = wire_octet_get(status);
    if ((p_registration == NULL) || (p_registration->p_sfra == NULL))
    {
        return;
    }

    p_payload[2] = wire_octet_get((uint16_t)p_registration->p_sfra->task.state);
    p_payload[3] = frame_sfra_is_busy(p_registration->p_sfra);
    p_payload[4] = wire_octet_get(p_registration->p_sfra->task.done);
    p_payload[5] = wire_octet_get(p_registration->data_ready);
    wire_u16_le_write(&p_payload[8], p_registration->p_sfra->task.freq_index);
    wire_u16_le_write(&p_payload[10], p_registration->p_sfra->cfg.freq_length);
    wire_u16_le_write(&p_payload[12], p_registration->result_count);
    wire_u32_le_write(&p_payload[16], p_registration->sweep_tag);
}

static void frame_sfra_info_encode(wire_octet_t *p_payload,
                                   const sfra_registration_t *p_registration,
                                   uint8_t sfra_id,
                                   uint8_t status)
{
    uint16_t index; /* Payload octet cleared before encoding fields. */

    for (index = 0u; index < FRAME_SFRA_INFO_ACK_SIZE; index++)
    {
        p_payload[index] = 0u;
    }
    p_payload[0] = wire_octet_get(sfra_id);
    p_payload[1] = wire_octet_get(status);
    if ((p_registration == NULL) || (p_registration->p_sfra == NULL))
    {
        return;
    }

    p_payload[2] = wire_octet_get((uint16_t)p_registration->p_sfra->task.state);
    p_payload[3] = frame_sfra_is_busy(p_registration->p_sfra);
    p_payload[4] = wire_octet_get(p_registration->p_sfra->task.done);
    p_payload[5] = wire_octet_get(p_registration->data_ready);
    wire_u16_le_write(&p_payload[8], p_registration->p_sfra->task.freq_index);
    wire_u16_le_write(&p_payload[10], p_registration->p_sfra->cfg.freq_length);
    wire_u16_le_write(&p_payload[12], p_registration->result_count);
    wire_u16_le_write(&p_payload[14], p_registration->p_sfra->cfg.inject_delay_tick);
    wire_u32_le_write(&p_payload[16], p_registration->sweep_tag);
    wire_f32_le_write(&p_payload[20], p_registration->p_sfra->output.current_freq_hz);
    wire_f32_le_write(&p_payload[24], p_registration->p_sfra->cfg.isr_freq_hz);
    wire_f32_le_write(&p_payload[28], p_registration->p_sfra->cfg.freq_start_hz);
    wire_f32_le_write(&p_payload[32], p_registration->p_sfra->cfg.freq_end_hz);
    wire_f32_le_write(&p_payload[36], p_registration->p_sfra->cfg.inject_amplitude);
    wire_f32_le_write(&p_payload[40], p_registration->p_sfra->cfg.settle_cycle_count);
    wire_f32_le_write(&p_payload[44], p_registration->p_sfra->cfg.collect_cycle_count);
}

static void frame_sfra_point_encode(wire_octet_t *p_payload,
                                    const sfra_registration_t *p_registration,
                                    uint8_t sfra_id,
                                    uint16_t point_index,
                                    uint8_t status)
{
    uint16_t index; /* Payload octet cleared before encoding fields. */

    for (index = 0u; index < FRAME_SFRA_POINT_ACK_SIZE; index++)
    {
        p_payload[index] = 0u;
    }
    p_payload[0] = wire_octet_get(sfra_id);
    p_payload[1] = wire_octet_get(status);
    wire_u16_le_write(&p_payload[4], point_index);
    if ((p_registration == NULL) || (p_registration->p_sfra == NULL))
    {
        return;
    }

    p_payload[2] = (uint8_t)((point_index + 1u) >= p_registration->p_sfra->cfg.freq_length);
    wire_u16_le_write(&p_payload[6], p_registration->p_sfra->cfg.freq_length);
    wire_u32_le_write(&p_payload[8], p_registration->sweep_tag);
    if ((status == (uint8_t)SFRA_TOOL_STATUS_OK) &&
        (point_index < p_registration->result_count) &&
        (point_index < SFRA_FREQ_TABLE_SIZE) &&
        (p_registration->result_cache[point_index].sweep_tag == p_registration->sweep_tag))
    {
        wire_f32_le_write(&p_payload[12], p_registration->result_cache[point_index].freq_hz);
        wire_f32_le_write(&p_payload[16], p_registration->result_cache[point_index].magnitude);
        wire_f32_le_write(&p_payload[20], p_registration->result_cache[point_index].phase_deg);
    }
}

static void frame_sfra_cache_current_point(sfra_registration_t *p_registration)
{
    uint16_t point_index; /* Completed SFRA point index. */

    if ((p_registration == NULL) || (p_registration->p_sfra == NULL))
    {
        return;
    }
    point_index = p_registration->p_sfra->output.point_index;
    if ((point_index >= p_registration->p_sfra->cfg.freq_length) ||
        (point_index >= SFRA_FREQ_TABLE_SIZE))
    {
        return;
    }

    p_registration->result_cache[point_index].sweep_tag = p_registration->sweep_tag;
    p_registration->result_cache[point_index].point_index = point_index;
    p_registration->result_cache[point_index].freq_hz = p_registration->p_sfra->output.current_freq_hz;
    p_registration->result_cache[point_index].magnitude = p_registration->p_sfra->output.mag;
    p_registration->result_cache[point_index].phase_deg = p_registration->p_sfra->output.phase;
    if (p_registration->result_count < (point_index + 1u))
    {
        p_registration->result_count = (uint16_t)(point_index + 1u);
    }
}

static void frame_sfra_service_init(void)
{
    section_item_t *p_item = p_sfra_first; /* Registration traversal cursor. */
    uint8_t count = 0u; /* Valid registered SFRA instances. */

    while (p_item != NULL)
    {
        sfra_registration_t *p_registration = (sfra_registration_t *)p_item->p_obj;

        if ((p_registration != NULL) && (p_registration->p_sfra != NULL))
        {
            p_registration->data_ready = 0u;
            p_registration->done_reported = 0u;
            p_registration->result_count = 0u;
            if (p_registration->sweep_tag == 0u)
            {
                p_registration->sweep_tag = 1u;
            }
            if (count != 0xFFu)
            {
                count++;
            }
        }
        p_item = p_item->p_next;
    }
    s_sfra_list.count = count;
}

REG_INIT(1u, frame_sfra_service_init)

static void frame_sfra_list_query_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }

    frame_sfra_route_capture(&s_sfra_route, p_pack, my_printf);
    frame_sfra_route_capture(&s_sfra_list.route, p_pack, my_printf);
    s_sfra_list.p_item = p_sfra_first;
    s_sfra_list.index = 0u;
    s_sfra_list.active = 1u;
}

REG_COMM(CMD_SET_SFRA, CMD_WORD_SFRA_LIST_QUERY, frame_sfra_list_query_act)

static void frame_sfra_list_task(void)
{
    wire_octet_t payload[FRAME_SFRA_LIST_HEADER_SIZE + FRAME_SFRA_NAME_MAX] = {0}; /* One list item. */
    sfra_registration_t *p_registration; /* Registration described by this item. */
    uint8_t name_length; /* Serialized name length. */
    uint16_t index; /* Name-copy index. */

    if (s_sfra_list.active == 0u)
    {
        return;
    }
    if (s_sfra_list.p_item == NULL)
    {
        payload[0] = 0xFFu;
        payload[1] = 1u;
        frame_sfra_route_send(&s_sfra_list.route,
                              CMD_WORD_SFRA_LIST_QUERY,
                              1u,
                              payload,
                              FRAME_SFRA_LIST_HEADER_SIZE);
        s_sfra_list.active = 0u;
        return;
    }

    p_registration = (sfra_registration_t *)s_sfra_list.p_item->p_obj;
    s_sfra_list.p_item = s_sfra_list.p_item->p_next;
    if ((p_registration == NULL) || (p_registration->p_sfra == NULL))
    {
        return;
    }

    name_length = frame_sfra_name_length(p_registration->p_name);
    payload[0] = wire_octet_get(p_registration->sfra_id);
    payload[1] = (uint8_t)((s_sfra_list.index + 1u) >= s_sfra_list.count);
    payload[2] = name_length;
    for (index = 0u; index < name_length; index++)
    {
        payload[FRAME_SFRA_LIST_HEADER_SIZE + index] =
            wire_octet_get((uint16_t)p_registration->p_name[index]);
    }

    frame_sfra_route_send(&s_sfra_list.route,
                          CMD_WORD_SFRA_LIST_QUERY,
                          1u,
                          payload,
                          (uint16_t)(FRAME_SFRA_LIST_HEADER_SIZE + name_length));
    s_sfra_list.index++;
    if (payload[1] != 0u)
    {
        s_sfra_list.active = 0u;
    }
}

REG_TASK_MS(10u, frame_sfra_list_task)

static void frame_sfra_poll_task(void)
{
    section_item_t *p_item = p_sfra_first; /* Registration traversal cursor. */

    while (p_item != NULL)
    {
        sfra_registration_t *p_registration = (sfra_registration_t *)p_item->p_obj;

        if ((p_registration != NULL) && (p_registration->p_sfra != NULL))
        {
            sfra_t *p_sfra = p_registration->p_sfra;

            if (p_sfra->output.point_done != 0u)
            {
                wire_octet_t payload[FRAME_SFRA_POINT_ACK_SIZE] = {0}; /* Point report. */

                p_registration->data_ready = 1u;
                frame_sfra_cache_current_point(p_registration);
                frame_sfra_point_encode(payload,
                                        p_registration,
                                        p_registration->sfra_id,
                                        p_sfra->output.point_index,
                                        (uint8_t)SFRA_TOOL_STATUS_OK);
                frame_sfra_route_send(&s_sfra_route,
                                      CMD_WORD_SFRA_POINT_REPORT,
                                      0u,
                                      payload,
                                      FRAME_SFRA_POINT_ACK_SIZE);
                p_sfra->output.point_done = 0u;
            }

            if ((p_registration->done_reported == 0u) && (p_sfra->task.done != 0u))
            {
                wire_octet_t payload[FRAME_SFRA_CONTROL_ACK_SIZE] = {0}; /* Sweep-done report. */

                p_registration->data_ready = 1u;
                p_registration->done_reported = 1u;
                frame_sfra_control_encode(payload,
                                          p_registration,
                                          p_registration->sfra_id,
                                          (uint8_t)SFRA_TOOL_STATUS_OK);
                frame_sfra_route_send(&s_sfra_route,
                                      CMD_WORD_SFRA_DONE_REPORT,
                                      0u,
                                      payload,
                                      FRAME_SFRA_CONTROL_ACK_SIZE);
            }
        }
        p_item = p_item->p_next;
    }
}

REG_TASK_MS(1u, frame_sfra_poll_task)

static void frame_sfra_info_query_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    wire_octet_t payload[FRAME_SFRA_INFO_ACK_SIZE] = {0}; /* Serialized SFRA state. */
    sfra_registration_t *p_registration; /* Requested SFRA instance. */
    uint8_t sfra_id = 0xFFu; /* Requested instance id. */
    uint8_t status = (uint8_t)SFRA_TOOL_STATUS_SFRA_ID_INVALID; /* Response status. */

    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }
    if ((p_pack->p_data != NULL) && (p_pack->len >= FRAME_SFRA_INFO_REQUEST_SIZE))
    {
        sfra_id = wire_octet_get(p_pack->p_data[0]);
    }

    frame_sfra_route_capture(&s_sfra_route, p_pack, my_printf);
    p_registration = frame_sfra_find(sfra_id);
    if (p_registration != NULL)
    {
        status = (uint8_t)SFRA_TOOL_STATUS_OK;
    }
    frame_sfra_info_encode(payload, p_registration, sfra_id, status);
    frame_sfra_reply(p_pack,
                     my_printf,
                     CMD_WORD_SFRA_INFO_QUERY,
                     payload,
                     FRAME_SFRA_INFO_ACK_SIZE);
}

REG_COMM(CMD_SET_SFRA, CMD_WORD_SFRA_INFO_QUERY, frame_sfra_info_query_act)

static void frame_sfra_config_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    wire_octet_t payload[FRAME_SFRA_CONTROL_ACK_SIZE] = {0}; /* Serialized config response. */
    sfra_registration_t *p_registration = NULL; /* Requested SFRA instance. */
    sfra_t *p_sfra = NULL; /* Requested SFRA core object. */
    uint8_t sfra_id = 0xFFu; /* Requested instance id. */
    uint8_t apply_mask = 0u; /* Configuration fields selected by the host. */
    uint8_t status = (uint8_t)SFRA_TOOL_STATUS_INVALID_PARAM; /* Response status. */
    uint32_t start_bits = 0u; /* Encoded start-frequency bits. */
    uint32_t end_bits = 0u; /* Encoded end-frequency bits. */
    uint32_t amplitude_bits = 0u; /* Encoded injection-amplitude bits. */
    float start_hz = 0.0f; /* Requested start frequency. */
    float end_hz = 0.0f; /* Requested end frequency. */
    float amplitude = 0.0f; /* Requested injection amplitude. */

    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }
    frame_sfra_route_capture(&s_sfra_route, p_pack, my_printf);
    if ((p_pack->p_data == NULL) || (p_pack->len < FRAME_SFRA_CONFIG_REQUEST_SIZE))
    {
        frame_sfra_control_encode(payload, NULL, sfra_id, status);
        frame_sfra_reply(p_pack, my_printf, CMD_WORD_SFRA_CFG_SET,
                         payload, FRAME_SFRA_CONTROL_ACK_SIZE);
        return;
    }

    sfra_id = wire_octet_get(p_pack->p_data[0]);
    apply_mask = wire_octet_get(p_pack->p_data[1]);
    start_bits = wire_u32_le_read(&p_pack->p_data[4]);
    end_bits = wire_u32_le_read(&p_pack->p_data[8]);
    amplitude_bits = wire_u32_le_read(&p_pack->p_data[12]);
    start_hz = wire_f32_le_read(&p_pack->p_data[4]);
    end_hz = wire_f32_le_read(&p_pack->p_data[8]);
    amplitude = wire_f32_le_read(&p_pack->p_data[12]);
    p_registration = frame_sfra_find(sfra_id);

    if (p_registration == NULL)
    {
        status = (uint8_t)SFRA_TOOL_STATUS_SFRA_ID_INVALID;
    }
    else
    {
        p_sfra = p_registration->p_sfra;
        if (frame_sfra_is_busy(p_sfra) != 0u)
        {
            status = (uint8_t)SFRA_TOOL_STATUS_BUSY;
        }
        else if ((apply_mask == 0u) ||
                 ((apply_mask & (uint8_t)~(SFRA_CFG_APPLY_FREQ |
                                           SFRA_CFG_APPLY_AMPLITUDE)) != 0u) ||
                 (frame_sfra_bits_are_finite(start_bits) == 0u) ||
                 (frame_sfra_bits_are_finite(end_bits) == 0u) ||
                 (frame_sfra_bits_are_finite(amplitude_bits) == 0u))
        {
            status = (uint8_t)SFRA_TOOL_STATUS_INVALID_PARAM;
        }
        else if (((apply_mask & SFRA_CFG_APPLY_FREQ) != 0u) &&
                 ((start_hz <= 0.0f) || (end_hz < start_hz)))
        {
            status = (uint8_t)SFRA_TOOL_STATUS_INVALID_PARAM;
        }
        else if (((apply_mask & SFRA_CFG_APPLY_AMPLITUDE) != 0u) &&
                 (amplitude <= 0.0f))
        {
            status = (uint8_t)SFRA_TOOL_STATUS_INVALID_PARAM;
        }
        else
        {
            sfra_status_t core_status = SFRA_STATUS_OK; /* Core configuration result. */

            if ((apply_mask & SFRA_CFG_APPLY_FREQ) != 0u)
            {
                core_status = sfra_set_sweep_range(p_sfra, start_hz, end_hz);
            }
            if ((core_status == SFRA_STATUS_OK) &&
                ((apply_mask & SFRA_CFG_APPLY_AMPLITUDE) != 0u))
            {
                p_sfra->cfg.inject_amplitude = amplitude;
            }
            status = frame_sfra_core_status(core_status);
            if (status == (uint8_t)SFRA_TOOL_STATUS_OK)
            {
                p_registration->data_ready = 0u;
                frame_sfra_sweep_tag_increment(p_registration);
            }
        }
    }

    frame_sfra_control_encode(payload, p_registration, sfra_id, status);
    frame_sfra_reply(p_pack,
                     my_printf,
                     CMD_WORD_SFRA_CFG_SET,
                     payload,
                     FRAME_SFRA_CONTROL_ACK_SIZE);
}

REG_COMM(CMD_SET_SFRA, CMD_WORD_SFRA_CFG_SET, frame_sfra_config_act)

static void frame_sfra_control_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    wire_octet_t payload[FRAME_SFRA_CONTROL_ACK_SIZE] = {0}; /* Serialized control response. */
    sfra_registration_t *p_registration = NULL; /* Requested SFRA instance. */
    uint8_t sfra_id = 0xFFu; /* Requested instance id. */
    uint8_t status = (uint8_t)SFRA_TOOL_STATUS_SFRA_ID_INVALID; /* Response status. */

    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }
    frame_sfra_route_capture(&s_sfra_route, p_pack, my_printf);
    if ((p_pack->p_data != NULL) && (p_pack->len >= FRAME_SFRA_CONTROL_REQUEST_SIZE))
    {
        sfra_id = wire_octet_get(p_pack->p_data[0]);
        p_registration = frame_sfra_find(sfra_id);
    }

    if ((p_registration != NULL) && (p_registration->p_sfra != NULL))
    {
        switch (wire_octet_get(p_pack->cmd_word))
        {
        case CMD_WORD_SFRA_START:
            if (frame_sfra_is_busy(p_registration->p_sfra) != 0u)
            {
                status = (uint8_t)SFRA_TOOL_STATUS_BUSY;
            }
            else
            {
                status = frame_sfra_core_status(sfra_start(p_registration->p_sfra));
                if (status == (uint8_t)SFRA_TOOL_STATUS_OK)
                {
                    p_registration->data_ready = 0u;
                    frame_sfra_sweep_tag_increment(p_registration);
                }
            }
            break;

        case CMD_WORD_SFRA_STOP:
            status = frame_sfra_core_status(sfra_stop(p_registration->p_sfra));
            p_registration->data_ready =
                (uint8_t)(p_registration->result_count > 0u);
            p_registration->done_reported = 0u;
            break;

        case CMD_WORD_SFRA_RESET:
        default:
            status = frame_sfra_core_status(sfra_reset(p_registration->p_sfra));
            p_registration->data_ready = 0u;
            frame_sfra_sweep_tag_increment(p_registration);
            break;
        }
    }

    frame_sfra_control_encode(payload, p_registration, sfra_id, status);
    frame_sfra_reply(p_pack,
                     my_printf,
                     wire_octet_get(p_pack->cmd_word),
                     payload,
                     FRAME_SFRA_CONTROL_ACK_SIZE);
}

REG_COMM(CMD_SET_SFRA, CMD_WORD_SFRA_START, frame_sfra_control_act)
REG_COMM(CMD_SET_SFRA, CMD_WORD_SFRA_STOP, frame_sfra_control_act)
REG_COMM(CMD_SET_SFRA, CMD_WORD_SFRA_RESET, frame_sfra_control_act)

static void frame_sfra_point_query_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    wire_octet_t payload[FRAME_SFRA_POINT_ACK_SIZE] = {0}; /* Serialized point response. */
    sfra_registration_t *p_registration = NULL; /* Requested SFRA instance. */
    uint8_t sfra_id = 0xFFu; /* Requested instance id. */
    uint16_t point_index = 0u; /* Requested result index. */
    uint32_t expected_sweep_tag = 0u; /* Optional host-side consistency tag. */
    uint8_t status = (uint8_t)SFRA_TOOL_STATUS_SFRA_ID_INVALID; /* Response status. */

    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }
    frame_sfra_route_capture(&s_sfra_route, p_pack, my_printf);
    if ((p_pack->p_data != NULL) && (p_pack->len >= FRAME_SFRA_POINT_REQUEST_SIZE))
    {
        sfra_id = wire_octet_get(p_pack->p_data[0]);
        point_index = wire_u16_le_read(&p_pack->p_data[2]);
        expected_sweep_tag = wire_u32_le_read(&p_pack->p_data[4]);
        p_registration = frame_sfra_find(sfra_id);
    }

    if ((p_registration != NULL) && (p_registration->p_sfra != NULL))
    {
        status = (uint8_t)SFRA_TOOL_STATUS_OK;
        if ((expected_sweep_tag != 0u) &&
            (expected_sweep_tag != p_registration->sweep_tag))
        {
            status = (uint8_t)SFRA_TOOL_STATUS_SWEEP_CHANGED;
        }
        else if (point_index >= p_registration->p_sfra->cfg.freq_length)
        {
            status = (uint8_t)SFRA_TOOL_STATUS_POINT_INDEX_INVALID;
        }
        else if ((p_registration->data_ready == 0u) ||
                 (point_index >= p_registration->result_count) ||
                 (point_index >= SFRA_FREQ_TABLE_SIZE) ||
                 (p_registration->result_cache[point_index].sweep_tag !=
                  p_registration->sweep_tag))
        {
            status = (uint8_t)SFRA_TOOL_STATUS_DATA_NOT_READY;
        }
    }

    frame_sfra_point_encode(payload,
                            p_registration,
                            sfra_id,
                            point_index,
                            status);
    frame_sfra_reply(p_pack,
                     my_printf,
                     CMD_WORD_SFRA_POINT_QUERY,
                     payload,
                     FRAME_SFRA_POINT_ACK_SIZE);
}

REG_COMM(CMD_SET_SFRA, CMD_WORD_SFRA_POINT_QUERY, frame_sfra_point_query_act)
