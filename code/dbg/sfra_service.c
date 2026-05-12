// SPDX-License-Identifier: MIT
/**
 * @file    sfra_service.c
 * @brief   SFRA service module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Build the g_sfra_first linked list from SECTION_SFRA entries at init
 *          - Translate SFRA protocol commands into core sfra_t operations
 *          - Poll registered SFRA instances and actively report new points and sweep completion
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-12
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "sfra_service.h"

#include "comm.h"

#include <stddef.h>
#include <string.h>

#define SFRA_SERVICE_NAME_LEN_MAX 64u
sfra_t *g_sfra_first = NULL;
static uint8_t g_sfra_service_count = 0u;

typedef struct
{
    DEC_MY_PRINTF;
    uint8_t active;
    uint8_t src;
    uint8_t d_src;
    uint8_t dst;
    uint8_t d_dst;
} sfra_report_ctx_t;

static sfra_report_ctx_t s_sfra_report_ctx = {0};

void sfra_service_init(void)
{
    sfra_t **p_tail = &g_sfra_first;
    uint8_t id = 0u;

    g_sfra_first = NULL;
    g_sfra_service_count = 0u;

    for (reg_section_t *p = (reg_section_t *)&SECTION_START;
         p < (reg_section_t *)&SECTION_STOP;
         ++p)
    {
        if (p->section_type == SECTION_SFRA)
        {
            sfra_t *s = (sfra_t *)p->p_str;
            s->sfra_id = id++;
            s->data_ready = 0u;
            s->done_reported = 0u;
            s->result_count = 0u;
            if (s->sweep_tag == 0u)
            {
                s->sweep_tag = 1u;
            }
            s->p_next = NULL;
            *p_tail = s;
            p_tail = &s->p_next;
            g_sfra_service_count++;
        }
    }
}

static uint16_t sfra_service_min_len(uint16_t a, uint16_t b)
{
    return (a < b) ? a : b;
}

static uint8_t sfra_service_strnlen(const char *str, uint8_t max_len)
{
    uint8_t len = 0u;

    if (str != NULL)
    {
        while ((len < max_len) && (str[len] != '\0'))
        {
            len++;
        }
    }

    return len;
}

static void sfra_service_copy_payload(void *p_dst,
                                      uint16_t dst_size,
                                      const section_packform_t *p_pack)
{
    if ((p_dst == NULL) || (dst_size == 0u))
    {
        return;
    }

    (void)memset(p_dst, 0, dst_size);
    if ((p_pack != NULL) && (p_pack->p_data != NULL) && (p_pack->len > 0u))
    {
        (void)memcpy(p_dst,
                     p_pack->p_data,
                     sfra_service_min_len(dst_size, p_pack->len));
    }
}

static void sfra_service_reply(section_packform_t *p_req_pack,
                               DEC_MY_PRINTF,
                               uint8_t cmd_word,
                               uint8_t is_ack,
                               uint8_t *p_data,
                               uint16_t len)
{
    section_packform_t packform = {0};

    packform.cmd_set = CMD_SET_SFRA;
    packform.cmd_word = cmd_word;
    packform.dst = p_req_pack->src;
    packform.d_dst = p_req_pack->d_src;
    packform.src = p_req_pack->dst;
    packform.d_src = p_req_pack->d_dst;
    packform.is_ack = is_ack;
    packform.len = len;
    packform.p_data = p_data;
    comm_send_data(&packform, my_printf);
}

static void sfra_service_capture_route(section_packform_t *p_req_pack, DEC_MY_PRINTF)
{
    if (p_req_pack == NULL)
    {
        return;
    }

    s_sfra_report_ctx.my_printf = my_printf;
    s_sfra_report_ctx.src = p_req_pack->dst;
    s_sfra_report_ctx.d_src = p_req_pack->d_dst;
    s_sfra_report_ctx.dst = p_req_pack->src;
    s_sfra_report_ctx.d_dst = p_req_pack->d_src;
    s_sfra_report_ctx.active = 1u;
}

static void sfra_service_send_report(uint8_t cmd_word, uint8_t *p_data, uint16_t len)
{
    section_packform_t packform = {0};

    if ((s_sfra_report_ctx.active == 0u) || (s_sfra_report_ctx.my_printf == NULL))
    {
        return;
    }

    packform.cmd_set = CMD_SET_SFRA;
    packform.cmd_word = cmd_word;
    packform.src = s_sfra_report_ctx.src;
    packform.d_src = s_sfra_report_ctx.d_src;
    packform.dst = s_sfra_report_ctx.dst;
    packform.d_dst = s_sfra_report_ctx.d_dst;
    packform.is_ack = 0u;
    packform.len = len;
    packform.p_data = p_data;
    comm_send_data(&packform, s_sfra_report_ctx.my_printf);
}

static sfra_t *sfra_service_find_by_id(uint8_t sfra_id)
{
    sfra_t *s = g_sfra_first;

    while (s != NULL)
    {
        if (s->sfra_id == sfra_id)
        {
            return s;
        }
        s = s->p_next;
    }

    return NULL;
}

static uint8_t sfra_service_is_busy(const sfra_t *sfra)
{
    if (sfra == NULL)
    {
        return 0u;
    }

    return (uint8_t)((sfra->task.active != 0u) ||
                     (sfra->output.busy != 0u) ||
                     ((sfra->task.state != SFRA_STATE_IDLE) &&
                      (sfra->task.state != SFRA_STATE_DONE)));
}

static void sfra_service_sweep_tag_inc(sfra_t *sfra)
{
    if (sfra == NULL)
    {
        return;
    }

    ++sfra->sweep_tag;
    if (sfra->sweep_tag == 0u)
    {
        ++sfra->sweep_tag;
    }
    sfra->done_reported = 0u;
    sfra->result_count = 0u;
}

static uint8_t sfra_service_core_status_to_tool(sfra_status_t status)
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

static void sfra_service_fill_ctrl_ack(sfra_ctrl_ack_t *p_ack,
                                       sfra_t *sfra,
                                       uint8_t sfra_id,
                                       uint8_t status)
{
    if (p_ack == NULL)
    {
        return;
    }

    (void)memset(p_ack, 0, sizeof(*p_ack));
    p_ack->sfra_id = sfra_id;
    p_ack->status = status;
    if (sfra == NULL)
    {
        return;
    }

    p_ack->state = (uint8_t)sfra->task.state;
    p_ack->busy = sfra_service_is_busy(sfra);
    p_ack->done = sfra->task.done;
    p_ack->data_ready = sfra->data_ready;
    p_ack->freq_index = sfra->task.freq_index;
    p_ack->freq_length = sfra->cfg.freq_length;
    p_ack->table_length = sfra->result_count;
    p_ack->sweep_tag = sfra->sweep_tag;
}

static void sfra_service_fill_info_ack(sfra_info_ack_t *p_ack,
                                       sfra_t *sfra,
                                       uint8_t sfra_id,
                                       uint8_t status)
{
    if (p_ack == NULL)
    {
        return;
    }

    (void)memset(p_ack, 0, sizeof(*p_ack));
    p_ack->sfra_id = sfra_id;
    p_ack->status = status;
    if (sfra == NULL)
    {
        return;
    }

    p_ack->state = (uint8_t)sfra->task.state;
    p_ack->busy = sfra_service_is_busy(sfra);
    p_ack->done = sfra->task.done;
    p_ack->data_ready = sfra->data_ready;
    p_ack->freq_index = sfra->task.freq_index;
    p_ack->freq_length = sfra->cfg.freq_length;
    p_ack->table_length = sfra->result_count;
    p_ack->inject_delay_tick = sfra->cfg.inject_delay_tick;
    p_ack->sweep_tag = sfra->sweep_tag;
    p_ack->current_freq_hz = sfra->output.current_freq_hz;
    p_ack->isr_freq_hz = sfra->cfg.isr_freq_hz;
    p_ack->freq_start_hz = sfra->cfg.freq_start_hz;
    p_ack->freq_end_hz = sfra->cfg.freq_end_hz;
    p_ack->inject_amplitude = sfra->cfg.inject_amplitude;
    p_ack->settle_cycle_count = sfra->cfg.settle_cycle_count;
    p_ack->collect_cycle_count = sfra->cfg.collect_cycle_count;
}

static void sfra_service_fill_point_ack(sfra_point_ack_t *p_ack,
                                        sfra_t *sfra,
                                        uint16_t point_index,
                                        uint8_t status)
{
    if (p_ack == NULL)
    {
        return;
    }

    (void)memset(p_ack, 0, sizeof(*p_ack));
    p_ack->status = status;
    p_ack->point_index = point_index;
    if (sfra == NULL)
    {
        p_ack->sfra_id = 0xFFu;
        return;
    }

    p_ack->sfra_id = sfra->sfra_id;
    p_ack->point_count = sfra->cfg.freq_length;
    p_ack->sweep_tag = sfra->sweep_tag;
    p_ack->is_last = (uint8_t)((point_index + 1u) >= sfra->cfg.freq_length);

    if ((status == (uint8_t)SFRA_TOOL_STATUS_OK) &&
        (point_index < sfra->result_count) &&
        (point_index < SFRA_FREQ_TABLE_SIZE) &&
        (sfra->result_cache[point_index].sweep_tag == sfra->sweep_tag))
    {
        p_ack->freq_hz = sfra->result_cache[point_index].freq_hz;
        p_ack->magnitude = sfra->result_cache[point_index].magnitude;
        p_ack->magnitude_db = sfra->result_cache[point_index].magnitude_db;
        p_ack->phase_deg = sfra->result_cache[point_index].phase_deg;
    }
}

static void sfra_service_cache_current_point(sfra_t *sfra)
{
    uint16_t point_index;
    if (sfra == NULL)
    {
        return;
    }

    point_index = sfra->output.point_index;
    if ((point_index >= sfra->cfg.freq_length) ||
        (point_index >= SFRA_FREQ_TABLE_SIZE))
    {
        return;
    }

    sfra->result_cache[point_index].sweep_tag = sfra->sweep_tag;
    sfra->result_cache[point_index].point_index = point_index;
    sfra->result_cache[point_index].freq_hz = sfra->output.current_freq_hz;
    sfra->result_cache[point_index].magnitude = sfra->output.mag;
    sfra->result_cache[point_index].magnitude_db = sfra->output.mag_db;
    sfra->result_cache[point_index].phase_deg = sfra->output.phase;

    if (sfra->result_count < (point_index + 1u))
    {
        sfra->result_count = point_index + 1u;
    }
}

static void sfra_service_send_empty_list(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    sfra_list_item_t item = {0};

    item.sfra_id = 0xFFu;
    item.is_last = 1u;
    sfra_service_reply(p_pack,
                       my_printf,
                       CMD_WORD_SFRA_LIST_QUERY,
                       1u,
                       (uint8_t *)&item,
                       (uint16_t)sizeof(item));
}

static void sfra_service_poll_task(void)
{
    sfra_t *s = g_sfra_first;

    while (s != NULL)
    {
        if (s->output.point_done != 0u)
        {
            sfra_point_report_t report;

            s->data_ready = 1u;
            sfra_service_cache_current_point(s);
            sfra_service_fill_point_ack(&report,
                                        s,
                                        s->output.point_index,
                                        (uint8_t)SFRA_TOOL_STATUS_OK);
            sfra_service_send_report(CMD_WORD_SFRA_POINT_REPORT,
                                     (uint8_t *)&report,
                                     (uint16_t)sizeof(report));
            s->output.point_done = 0u;
        }

        if ((s->done_reported == 0u) && (s->task.done != 0u))
        {
            sfra_ctrl_ack_t report;

            s->data_ready = 1u;
            s->done_reported = 1u;
            sfra_service_fill_ctrl_ack(&report, s, s->sfra_id, (uint8_t)SFRA_TOOL_STATUS_OK);
            sfra_service_send_report(CMD_WORD_SFRA_DONE_REPORT,
                                     (uint8_t *)&report,
                                     (uint16_t)sizeof(report));
        }

        s = s->p_next;
    }
}

static void sfra_list_query_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    sfra_t *s = g_sfra_first;
    uint8_t index = 0u;

    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }

    sfra_service_capture_route(p_pack, my_printf);
    if (s == NULL)
    {
        sfra_service_send_empty_list(p_pack, my_printf);
        return;
    }

    while (s != NULL)
    {
        uint8_t name_len = sfra_service_strnlen(s->p_name, SFRA_SERVICE_NAME_LEN_MAX);
        uint8_t payload[sizeof(sfra_list_item_t) + SFRA_SERVICE_NAME_LEN_MAX];
        sfra_list_item_t item = {0};

        item.sfra_id = s->sfra_id;
        item.is_last = (uint8_t)((index + 1u) >= g_sfra_service_count);
        item.name_len = name_len;

        (void)memcpy(payload, &item, sizeof(item));
        if (name_len > 0u)
        {
            (void)memcpy(&payload[sizeof(item)], s->p_name, name_len);
        }

        sfra_service_reply(p_pack,
                           my_printf,
                           CMD_WORD_SFRA_LIST_QUERY,
                           1u,
                           payload,
                           (uint16_t)(sizeof(item) + name_len));

        s = s->p_next;
        index++;
    }
}

static void sfra_info_query_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    sfra_info_query_t query;
    sfra_info_ack_t ack;
    sfra_t *s;

    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }

    sfra_service_capture_route(p_pack, my_printf);
    sfra_service_copy_payload(&query, (uint16_t)sizeof(query), p_pack);
    s = sfra_service_find_by_id(query.sfra_id);
    sfra_service_fill_info_ack(&ack,
                               s,
                               query.sfra_id,
                               (s == NULL) ? (uint8_t)SFRA_TOOL_STATUS_SFRA_ID_INVALID
                                           : (uint8_t)SFRA_TOOL_STATUS_OK);
    sfra_service_reply(p_pack,
                       my_printf,
                       CMD_WORD_SFRA_INFO_QUERY,
                       1u,
                       (uint8_t *)&ack,
                       (uint16_t)sizeof(ack));
}

static void sfra_cfg_set_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    sfra_cfg_set_t cfg;
    sfra_ctrl_ack_t ack;
    sfra_t *s;
    uint8_t status = (uint8_t)SFRA_TOOL_STATUS_OK;

    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }

    sfra_service_capture_route(p_pack, my_printf);
    sfra_service_copy_payload(&cfg, (uint16_t)sizeof(cfg), p_pack);
    s = sfra_service_find_by_id(cfg.sfra_id);
    if (s == NULL)
    {
        sfra_service_fill_ctrl_ack(&ack, NULL, cfg.sfra_id, (uint8_t)SFRA_TOOL_STATUS_SFRA_ID_INVALID);
        sfra_service_reply(p_pack, my_printf, CMD_WORD_SFRA_CFG_SET, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack));
        return;
    }

    if (sfra_service_is_busy(s) != 0u)
    {
        status = (uint8_t)SFRA_TOOL_STATUS_BUSY;
    }
    else if ((cfg.apply_mask == 0u) ||
             ((cfg.apply_mask & (uint8_t)~(SFRA_CFG_APPLY_FREQ | SFRA_CFG_APPLY_AMPLITUDE)) != 0u))
    {
        status = (uint8_t)SFRA_TOOL_STATUS_INVALID_PARAM;
    }
    else if (((cfg.apply_mask & SFRA_CFG_APPLY_FREQ) != 0u) &&
             ((cfg.freq_start_hz <= 0.0f) || (cfg.freq_end_hz < cfg.freq_start_hz)))
    {
        status = (uint8_t)SFRA_TOOL_STATUS_INVALID_PARAM;
    }
    else if (((cfg.apply_mask & SFRA_CFG_APPLY_AMPLITUDE) != 0u) &&
             (cfg.inject_amplitude <= 0.0f))
    {
        status = (uint8_t)SFRA_TOOL_STATUS_INVALID_PARAM;
    }
    else
    {
        sfra_status_t core_status = SFRA_STATUS_OK;

        if ((cfg.apply_mask & SFRA_CFG_APPLY_FREQ) != 0u)
        {
            core_status = sfra_set_sweep_range(s, cfg.freq_start_hz, cfg.freq_end_hz);
        }

        if ((core_status == SFRA_STATUS_OK) &&
            ((cfg.apply_mask & SFRA_CFG_APPLY_AMPLITUDE) != 0u))
        {
            s->cfg.inject_amplitude = cfg.inject_amplitude;
        }

        status = sfra_service_core_status_to_tool(core_status);
        if (status == (uint8_t)SFRA_TOOL_STATUS_OK)
        {
            s->data_ready = 0u;
            sfra_service_sweep_tag_inc(s);
        }
    }

    sfra_service_fill_ctrl_ack(&ack, s, cfg.sfra_id, status);
    sfra_service_reply(p_pack,
                       my_printf,
                       CMD_WORD_SFRA_CFG_SET,
                       1u,
                       (uint8_t *)&ack,
                       (uint16_t)sizeof(ack));
}

static void sfra_start_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    sfra_info_query_t query;
    sfra_ctrl_ack_t ack;
    sfra_t *s;
    uint8_t status;

    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }

    sfra_service_capture_route(p_pack, my_printf);
    sfra_service_copy_payload(&query, (uint16_t)sizeof(query), p_pack);
    s = sfra_service_find_by_id(query.sfra_id);
    if (s == NULL)
    {
        sfra_service_fill_ctrl_ack(&ack, NULL, query.sfra_id, (uint8_t)SFRA_TOOL_STATUS_SFRA_ID_INVALID);
        sfra_service_reply(p_pack, my_printf, CMD_WORD_SFRA_START, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack));
        return;
    }

    if (sfra_service_is_busy(s) != 0u)
    {
        status = (uint8_t)SFRA_TOOL_STATUS_BUSY;
    }
    else
    {
        status = sfra_service_core_status_to_tool(sfra_start(s));
        if (status == (uint8_t)SFRA_TOOL_STATUS_OK)
        {
            s->data_ready = 0u;
            sfra_service_sweep_tag_inc(s);
        }
    }

    sfra_service_fill_ctrl_ack(&ack, s, query.sfra_id, status);
    sfra_service_reply(p_pack, my_printf, CMD_WORD_SFRA_START, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack));
}

static void sfra_stop_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    sfra_info_query_t query;
    sfra_ctrl_ack_t ack;
    sfra_t *s;
    uint8_t status;

    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }

    sfra_service_capture_route(p_pack, my_printf);
    sfra_service_copy_payload(&query, (uint16_t)sizeof(query), p_pack);
    s = sfra_service_find_by_id(query.sfra_id);
    if (s == NULL)
    {
        sfra_service_fill_ctrl_ack(&ack, NULL, query.sfra_id, (uint8_t)SFRA_TOOL_STATUS_SFRA_ID_INVALID);
        sfra_service_reply(p_pack, my_printf, CMD_WORD_SFRA_STOP, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack));
        return;
    }

    status = sfra_service_core_status_to_tool(sfra_stop(s));
    s->data_ready = (uint8_t)(s->result_count > 0u);
    s->done_reported = 0u;
    sfra_service_fill_ctrl_ack(&ack, s, query.sfra_id, status);
    sfra_service_reply(p_pack, my_printf, CMD_WORD_SFRA_STOP, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack));
}

static void sfra_reset_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    sfra_info_query_t query;
    sfra_ctrl_ack_t ack;
    sfra_t *s;
    uint8_t status;

    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }

    sfra_service_capture_route(p_pack, my_printf);
    sfra_service_copy_payload(&query, (uint16_t)sizeof(query), p_pack);
    s = sfra_service_find_by_id(query.sfra_id);
    if (s == NULL)
    {
        sfra_service_fill_ctrl_ack(&ack, NULL, query.sfra_id, (uint8_t)SFRA_TOOL_STATUS_SFRA_ID_INVALID);
        sfra_service_reply(p_pack, my_printf, CMD_WORD_SFRA_RESET, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack));
        return;
    }

    status = sfra_service_core_status_to_tool(sfra_reset(s));
    s->data_ready = 0u;
    sfra_service_sweep_tag_inc(s);
    sfra_service_fill_ctrl_ack(&ack, s, query.sfra_id, status);
    sfra_service_reply(p_pack, my_printf, CMD_WORD_SFRA_RESET, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack));
}

static void sfra_point_query_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    sfra_point_query_t query;
    sfra_point_ack_t ack;
    sfra_t *s;
    uint8_t status = (uint8_t)SFRA_TOOL_STATUS_OK;

    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }

    sfra_service_capture_route(p_pack, my_printf);
    sfra_service_copy_payload(&query, (uint16_t)sizeof(query), p_pack);
    s = sfra_service_find_by_id(query.sfra_id);
    if (s == NULL)
    {
        sfra_service_fill_point_ack(&ack, NULL, query.point_index, (uint8_t)SFRA_TOOL_STATUS_SFRA_ID_INVALID);
        ack.sfra_id = query.sfra_id;
    }
    else
    {
        if ((query.expected_sweep_tag != 0u) && (query.expected_sweep_tag != s->sweep_tag))
        {
            status = (uint8_t)SFRA_TOOL_STATUS_SWEEP_CHANGED;
        }
        else if (query.point_index >= s->cfg.freq_length)
        {
            status = (uint8_t)SFRA_TOOL_STATUS_POINT_INDEX_INVALID;
        }
        else if ((s->data_ready == 0u) ||
                 (query.point_index >= s->result_count) ||
                 (query.point_index >= SFRA_FREQ_TABLE_SIZE) ||
                 (s->result_cache[query.point_index].sweep_tag != s->sweep_tag))
        {
            status = (uint8_t)SFRA_TOOL_STATUS_DATA_NOT_READY;
        }

        sfra_service_fill_point_ack(&ack, s, query.point_index, status);
    }

    sfra_service_reply(p_pack,
                       my_printf,
                       CMD_WORD_SFRA_POINT_QUERY,
                       1u,
                       (uint8_t *)&ack,
                       (uint16_t)sizeof(ack));
}

REG_INIT(0, sfra_service_init)
REG_TASK_MS(1, sfra_service_poll_task)
REG_COMM(CMD_SET_SFRA, CMD_WORD_SFRA_LIST_QUERY, sfra_list_query_act)
REG_COMM(CMD_SET_SFRA, CMD_WORD_SFRA_INFO_QUERY, sfra_info_query_act)
REG_COMM(CMD_SET_SFRA, CMD_WORD_SFRA_CFG_SET, sfra_cfg_set_act)
REG_COMM(CMD_SET_SFRA, CMD_WORD_SFRA_START, sfra_start_act)
REG_COMM(CMD_SET_SFRA, CMD_WORD_SFRA_STOP, sfra_stop_act)
REG_COMM(CMD_SET_SFRA, CMD_WORD_SFRA_RESET, sfra_reset_act)
REG_COMM(CMD_SET_SFRA, CMD_WORD_SFRA_POINT_QUERY, sfra_point_query_act)
