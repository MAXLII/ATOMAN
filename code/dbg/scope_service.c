// SPDX-License-Identifier: MIT
/**
 * @file    scope_service.c
 * @brief   Scope service module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Build the g_scope_first linked list from SECTION_SCOPE entries at init
 *          - Assign scope ids and translate scope state into shell or binary service responses
 *          - Own deferred scope data printing and communication helper code outside the capture core
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
#include "scope_service.h"

#include "comm.h"

#include <stddef.h>
#include <string.h>

#define SCOPE_SERVICE_VAR_COUNT_MAX 10u
#define SCOPE_SERVICE_NAME_LEN_MAX 64u

/* Linked list head — built at init time from SECTION_SCOPE entries */
scope_t *g_scope_first = NULL;
static uint8_t g_scope_service_count = 0u;

typedef struct
{
    scope_t *p_cur;
    DEC_MY_PRINTF;
    uint8_t active;
    uint8_t index;
    uint8_t src;
    uint8_t d_src;
    uint8_t dst;
    uint8_t d_dst;
} scope_list_ctx_t;

static scope_list_ctx_t s_scope_list_ctx = {0};

/* Init — traverse .section segment, build g_scope_first linked list */
void scope_service_init(void)
{
    scope_t **p_tail = &g_scope_first;
    uint8_t id = 0u;

    for (reg_section_t *p = (reg_section_t *)&SECTION_START;
         p < (reg_section_t *)&SECTION_STOP;
         ++p)
    {
        if (p->section_type == SECTION_SCOPE)
        {
            scope_t *s = (scope_t *)p->p_str;
            s->scope_id = id++;
            s->p_next = NULL;
            *p_tail = s;
            p_tail = &s->p_next;
            g_scope_service_count++;
        }
    }
}

static void scope_service_reply(section_packform_t *p_req_pack,
                                DEC_MY_PRINTF,
                                uint8_t cmd_word,
                                uint8_t is_ack,
                                uint8_t *p_data,
                                uint16_t len)
{
    section_packform_t packform = {0};

    packform.cmd_set = CMD_SET_SCOPE;
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

/* Command handlers */
static uint8_t scope_service_query_scope_id(section_packform_t *p_pack)
{
    if ((p_pack == NULL) || (p_pack->p_data == NULL) || (p_pack->len == 0u))
    {
        return 0xFFu;
    }
    return p_pack->p_data[0];
}

/* Helpers */
static scope_t *scope_service_find_by_id(uint8_t scope_id)
{
    scope_t *s = g_scope_first;
    while (s != NULL)
    {
        if (s->scope_id == scope_id)
        {
            return s;
        }
        s = s->p_next;
    }
    return NULL;
}

static void scope_service_capture_tag_inc(scope_t *p_scope)
{
    if (p_scope == NULL)
    {
        return;
    }

    ++p_scope->capture_tag;
    if (p_scope->capture_tag == 0u)
    {
        ++p_scope->capture_tag;
    }
}

static uint8_t scope_service_strnlen(const char *str, uint8_t max_len)
{
    uint8_t len = 0u;
    if (str != NULL)
    {
        while (len < max_len && str[len] != '\0')
            len++;
    }
    return len;
}

static uint32_t scope_service_get_trigger_display_index(scope_t *p_scope)
{
    if ((p_scope == NULL) || (p_scope->buffer_size == 0u) || (p_scope->trigger_post_cnt >= p_scope->buffer_size))
    {
        return 0u;
    }
    return p_scope->buffer_size - p_scope->trigger_post_cnt - 1u;
}

static uint32_t scope_service_get_logical_start_index(scope_t *p_scope, uint8_t read_mode)
{
    if ((p_scope == NULL) || (p_scope->buffer_size == 0u))
    {
        return 0u;
    }

    if ((read_mode == SCOPE_READ_MODE_FORCE) &&
        (p_scope->state == SCOPE_STATE_RUNNING) &&
        (p_scope->in_trigger == 0u))
    {
        return p_scope->write_index % p_scope->buffer_size;
    }

    return (p_scope->trigger_index + p_scope->trigger_post_cnt + 1u) % p_scope->buffer_size;
}

static uint32_t scope_service_logical_to_physical_index(scope_t *p_scope, uint8_t read_mode, uint32_t logical_index)
{
    if ((p_scope == NULL) || (p_scope->buffer_size == 0u))
    {
        return 0u;
    }
    uint32_t start_index = scope_service_get_logical_start_index(p_scope, read_mode);
    return (start_index + logical_index) % p_scope->buffer_size;
}

static void scope_service_capture_route(scope_list_ctx_t *p_ctx, section_packform_t *p_req_pack, DEC_MY_PRINTF)
{
    if ((p_ctx == NULL) || (p_req_pack == NULL))
    {
        return;
    }

    p_ctx->my_printf = my_printf;
    p_ctx->src = p_req_pack->dst;
    p_ctx->d_src = p_req_pack->d_dst;
    p_ctx->dst = p_req_pack->src;
    p_ctx->d_dst = p_req_pack->d_src;
}

static void scope_service_send_active(scope_list_ctx_t *p_ctx, uint8_t cmd_word, uint8_t is_ack, uint8_t *p_data, uint16_t len)
{
    section_packform_t packform = {0};

    if (p_ctx == NULL)
    {
        return;
    }

    packform.cmd_set = CMD_SET_SCOPE;
    packform.cmd_word = cmd_word;
    packform.src = p_ctx->src;
    packform.d_src = p_ctx->d_src;
    packform.dst = p_ctx->dst;
    packform.d_dst = p_ctx->d_dst;
    packform.is_ack = is_ack;
    packform.len = len;
    packform.p_data = p_data;
    comm_send_data(&packform, p_ctx->my_printf);
}

/* Poll tasks — walk g_scope_first linked list */
static void scope_service_poll_state(void)
{
    scope_t *s = g_scope_first;
    while (s != NULL)
    {
        if ((s->last_state == SCOPE_STATE_TRIGGERED) && (s->state == SCOPE_STATE_IDLE))
        {
            s->data_ready = 1u;
            scope_service_capture_tag_inc(s);
        }
        s->last_state = s->state;
        s = s->p_next;
    }
}

static void scope_service_poll_list(void)
{
    if (s_scope_list_ctx.active == 0u)
    {
        return;
    }

    scope_t *s = s_scope_list_ctx.p_cur;
    if (s == NULL)
    {
        s_scope_list_ctx.active = 0u;
        return;
    }

    uint8_t name_len = scope_service_strnlen(s->p_name, SCOPE_SERVICE_NAME_LEN_MAX);
    uint8_t payload[sizeof(scope_list_item_t) + SCOPE_SERVICE_NAME_LEN_MAX];
    scope_list_item_t item;

    item.scope_id = s->scope_id;
    item.is_last = (uint8_t)((s_scope_list_ctx.index + 1u) >= g_scope_service_count);
    item.name_len = name_len;
    item.reserved = 0u;

    (void)memcpy(payload, &item, sizeof(item));
    if (name_len > 0u)
    {
        (void)memcpy(&payload[sizeof(item)], s->p_name, name_len);
    }

    scope_service_send_active(&s_scope_list_ctx,
                              CMD_WORD_SCOPE_LIST_QUERY,
                              1u,
                              payload,
                              (uint16_t)(sizeof(item) + name_len));

    s_scope_list_ctx.p_cur = s->p_next;
    ++s_scope_list_ctx.index;
    if (item.is_last != 0u)
    {
        s_scope_list_ctx.active = 0u;
    }
}

static void scope_service_poll_task(void)
{
    scope_service_poll_state();
    scope_service_poll_list();
}


static void scope_service_send_empty_list(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    scope_list_item_t item = {0};

    item.scope_id = 0xFFu;
    item.is_last = 1u;
    item.name_len = 0u;
    scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_LIST_QUERY, 1u, (uint8_t *)&item, (uint16_t)sizeof(item));
}

static void scope_list_query_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }

    if (g_scope_first == NULL)
    {
        scope_service_send_empty_list(p_pack, my_printf);
        return;
    }

    s_scope_list_ctx.p_cur = g_scope_first;
    s_scope_list_ctx.index = 0u;
    s_scope_list_ctx.active = 1u;
    scope_service_capture_route(&s_scope_list_ctx, p_pack, my_printf);
    scope_service_poll_list();
}

static void scope_info_query_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }

    scope_info_ack_t ack = {0};
    uint8_t scope_id = scope_service_query_scope_id(p_pack);
    scope_t *p_scope = scope_service_find_by_id(scope_id);

    ack.scope_id = scope_id;
    if (p_scope == NULL)
    {
        ack.status = (uint8_t)SCOPE_TOOL_STATUS_SCOPE_ID_INVALID;
    }
    else
    {
        ack.status = (uint8_t)SCOPE_TOOL_STATUS_OK;
        ack.state = (uint8_t)p_scope->state;
        ack.data_ready = p_scope->data_ready;
        ack.var_count = p_scope->var_count;
        ack.sample_count = p_scope->buffer_size;
        ack.write_index = p_scope->write_index;
        ack.trigger_index = p_scope->trigger_index;
        ack.trigger_post_cnt = p_scope->trigger_post_cnt;
        ack.trigger_display_index = scope_service_get_trigger_display_index(p_scope);
        ack.sample_period_us = p_scope->sample_period_us;
        ack.capture_tag = p_scope->capture_tag;
    }

    scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_INFO_QUERY, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack));
}

static void scope_var_query_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    uint8_t scope_id = 0xFFu;
    uint8_t var_index = 0xFFu;

    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }

    if ((p_pack->p_data != NULL) && (p_pack->len >= 2u))
    {
        scope_id = p_pack->p_data[0];
        var_index = p_pack->p_data[1];
    }

    scope_t *p_scope = scope_service_find_by_id(scope_id);
    scope_var_ack_t ack = {0};
    ack.scope_id = scope_id;
    ack.var_index = var_index;

    if (p_scope == NULL)
    {
        ack.status = (uint8_t)SCOPE_TOOL_STATUS_SCOPE_ID_INVALID;
        ack.is_last = 1u;
        scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_VAR_QUERY, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack));
        return;
    }

    if (var_index >= p_scope->var_count)
    {
        ack.status = (uint8_t)SCOPE_TOOL_STATUS_VAR_INDEX_INVALID;
        ack.is_last = 1u;
        scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_VAR_QUERY, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack));
        return;
    }

    const char *p_name = p_scope->var_names[var_index];
    uint8_t name_len = scope_service_strnlen(p_name, SCOPE_SERVICE_NAME_LEN_MAX);

    ack.status = (uint8_t)SCOPE_TOOL_STATUS_OK;
    ack.is_last = (uint8_t)((var_index + 1u) >= p_scope->var_count);
    ack.name_len = name_len;

    uint8_t payload[sizeof(scope_var_ack_t) + SCOPE_SERVICE_NAME_LEN_MAX];
    (void)memcpy(payload, &ack, sizeof(ack));
    if (name_len > 0u)
    {
        (void)memcpy(&payload[sizeof(ack)], p_name, name_len);
    }
    scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_VAR_QUERY, 1u, payload, (uint16_t)(sizeof(ack) + name_len));
}

static void scope_start_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    uint8_t scope_id = scope_service_query_scope_id(p_pack);
    scope_t *p_scope = scope_service_find_by_id(scope_id);
    scope_ctrl_ack_t ack;

    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }

    if (p_scope == NULL)
    {
        ack.scope_id = scope_id;
        ack.status = (uint8_t)SCOPE_TOOL_STATUS_SCOPE_ID_INVALID;
        ack.state = 0u;
        ack.data_ready = 0u;
        ack.capture_tag = 0u;
        scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_START, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack));
        return;
    }

    if (p_scope->state != SCOPE_STATE_IDLE)
    {
        ack.scope_id = p_scope->scope_id;
        ack.status = (uint8_t)SCOPE_TOOL_STATUS_RUNNING_DENIED;
        ack.state = (uint8_t)p_scope->state;
        ack.data_ready = p_scope->data_ready;
        ack.capture_tag = p_scope->capture_tag;
    }
    else
    {
        p_scope->data_ready = 0u;
        scope_service_capture_tag_inc(p_scope);
        scope_start(p_scope);
        p_scope->last_state = p_scope->state;
        ack.scope_id = p_scope->scope_id;
        ack.status = (uint8_t)SCOPE_TOOL_STATUS_OK;
        ack.state = (uint8_t)p_scope->state;
        ack.data_ready = p_scope->data_ready;
        ack.capture_tag = p_scope->capture_tag;
    }

    scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_START, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack));
}

static void scope_trigger_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    uint8_t scope_id = scope_service_query_scope_id(p_pack);
    scope_t *p_scope = scope_service_find_by_id(scope_id);
    scope_ctrl_ack_t ack;

    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }

    if (p_scope == NULL)
    {
        ack.scope_id = scope_id;
        ack.status = (uint8_t)SCOPE_TOOL_STATUS_SCOPE_ID_INVALID;
        ack.state = 0u;
        ack.data_ready = 0u;
        ack.capture_tag = 0u;
        scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_TRIGGER, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack));
        return;
    }

    if (p_scope->state != SCOPE_STATE_RUNNING)
    {
        ack.scope_id = p_scope->scope_id;
        ack.status = (uint8_t)SCOPE_TOOL_STATUS_RUNNING_DENIED;
        ack.state = (uint8_t)p_scope->state;
        ack.data_ready = p_scope->data_ready;
        ack.capture_tag = p_scope->capture_tag;
    }
    else
    {
        scope_trigger(p_scope);
        ack.scope_id = p_scope->scope_id;
        ack.status = (uint8_t)SCOPE_TOOL_STATUS_OK;
        ack.state = (uint8_t)p_scope->state;
        ack.data_ready = p_scope->data_ready;
        ack.capture_tag = p_scope->capture_tag;
    }

    scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_TRIGGER, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack));
}

static void scope_stop_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    uint8_t scope_id = scope_service_query_scope_id(p_pack);
    scope_t *p_scope = scope_service_find_by_id(scope_id);
    scope_ctrl_ack_t ack;

    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }

    if (p_scope == NULL)
    {
        ack.scope_id = scope_id;
        ack.status = (uint8_t)SCOPE_TOOL_STATUS_SCOPE_ID_INVALID;
        ack.state = 0u;
        ack.data_ready = 0u;
        ack.capture_tag = 0u;
        scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_STOP, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack));
        return;
    }

    scope_stop(p_scope);
    p_scope->data_ready = 1u;
    p_scope->last_state = p_scope->state;
    ack.scope_id = p_scope->scope_id;
    ack.status = (uint8_t)SCOPE_TOOL_STATUS_OK;
    ack.state = (uint8_t)p_scope->state;
    ack.data_ready = p_scope->data_ready;
    ack.capture_tag = p_scope->capture_tag;
    scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_STOP, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack));
}

static void scope_reset_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    uint8_t scope_id = scope_service_query_scope_id(p_pack);
    scope_t *p_scope = scope_service_find_by_id(scope_id);
    scope_ctrl_ack_t ack;

    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }

    if (p_scope == NULL)
    {
        ack.scope_id = scope_id;
        ack.status = (uint8_t)SCOPE_TOOL_STATUS_SCOPE_ID_INVALID;
        ack.state = 0u;
        ack.data_ready = 0u;
        ack.capture_tag = 0u;
        scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_RESET, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack));
        return;
    }

    scope_reset(p_scope);
    p_scope->data_ready = 0u;
    scope_service_capture_tag_inc(p_scope);
    p_scope->last_state = p_scope->state;
    ack.scope_id = p_scope->scope_id;
    ack.status = (uint8_t)SCOPE_TOOL_STATUS_OK;
    ack.state = (uint8_t)p_scope->state;
    ack.data_ready = p_scope->data_ready;
    ack.capture_tag = p_scope->capture_tag;
    scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_RESET, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack));
}

static void scope_sample_query_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    uint8_t scope_id = 0xFFu;
    uint8_t read_mode = SCOPE_READ_MODE_NORMAL;
    uint32_t sample_index = 0u;
    uint32_t expected_capture_tag = 0u;

    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }

    if ((p_pack->p_data != NULL) && (p_pack->len >= sizeof(scope_sample_query_t)))
    {
        const scope_sample_query_t *p_query = (const scope_sample_query_t *)p_pack->p_data;
        scope_id = p_query->scope_id;
        read_mode = p_query->read_mode;
        sample_index = p_query->sample_index;
        expected_capture_tag = p_query->expected_capture_tag;
    }
    else if ((p_pack->p_data != NULL) && (p_pack->len >= 2u))
    {
        scope_id = p_pack->p_data[0];
        read_mode = p_pack->p_data[1];
    }

    scope_t *p_scope = scope_service_find_by_id(scope_id);
    scope_sample_ack_t ack = {0};
    ack.scope_id = scope_id;
    ack.read_mode = read_mode;
    ack.sample_index = sample_index;

    if (p_scope == NULL)
    {
        ack.status = (uint8_t)SCOPE_TOOL_STATUS_SCOPE_ID_INVALID;
        scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_SAMPLE_QUERY, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack));
        return;
    }

    ack.capture_tag = p_scope->capture_tag;
    if ((read_mode != SCOPE_READ_MODE_NORMAL) && (read_mode != SCOPE_READ_MODE_FORCE))
    {
        ack.status = (uint8_t)SCOPE_TOOL_STATUS_SAMPLE_INDEX_INVALID;
    }
    else if ((read_mode == SCOPE_READ_MODE_NORMAL) && (p_scope->state != SCOPE_STATE_IDLE))
    {
        ack.status = (uint8_t)SCOPE_TOOL_STATUS_RUNNING_DENIED;
    }
    else if ((expected_capture_tag != 0u) && (expected_capture_tag != p_scope->capture_tag))
    {
        ack.status = (uint8_t)SCOPE_TOOL_STATUS_CAPTURE_CHANGED;
    }
    else if ((p_scope->data_ready == 0u) && (read_mode != SCOPE_READ_MODE_FORCE))
    {
        ack.status = (uint8_t)SCOPE_TOOL_STATUS_DATA_NOT_READY;
    }
    else if ((p_scope->buffer_size == 0u) || (sample_index >= p_scope->buffer_size))
    {
        ack.status = (uint8_t)SCOPE_TOOL_STATUS_SAMPLE_INDEX_INVALID;
    }
    else
    {
        uint8_t payload[sizeof(scope_sample_ack_t) + (SCOPE_SERVICE_VAR_COUNT_MAX * sizeof(float))];
        uint32_t physical_index = scope_service_logical_to_physical_index(p_scope, read_mode, sample_index);
        uint8_t var_count = p_scope->var_count;

        if (var_count > SCOPE_SERVICE_VAR_COUNT_MAX)
        {
            var_count = SCOPE_SERVICE_VAR_COUNT_MAX;
        }

        ack.status = (uint8_t)SCOPE_TOOL_STATUS_OK;
        ack.var_count = var_count;
        ack.is_last_sample = (uint8_t)((sample_index + 1u) >= p_scope->buffer_size);

        for (uint8_t i = 0u; i < var_count; ++i)
        {
            float value = p_scope->buffer[physical_index + ((uint32_t)i * p_scope->buffer_size)];
            (void)memcpy(&payload[sizeof(ack) + ((uint32_t)i * sizeof(float))], &value, sizeof(value));
        }

        uint16_t len = (uint16_t)(sizeof(ack) + ((uint16_t)var_count * (uint16_t)sizeof(float)));
        (void)memcpy(payload, &ack, sizeof(ack));
        scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_SAMPLE_QUERY, 1u, payload, len);
        return;
    }

    scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_SAMPLE_QUERY, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack));
}

/* Printf helpers (gated by SCOPE_ENABLE_PRINTF) */
#if SCOPE_ENABLE_PRINTF == 1

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"

void scope_printf_status(scope_t *scope, DEC_MY_PRINTF)
{
    if ((scope == NULL) || (my_printf == NULL))
    {
        return;
    }

    my_printf->my_printf("Scope Status: %s\n", scope->is_running ? "Running" : "Idle");
    my_printf->my_printf("Current Write Index: %lu\n", (unsigned long)scope->write_index);
    my_printf->my_printf("Trigger Index: %lu\n", (unsigned long)scope->trigger_index);
    my_printf->my_printf("Trigger Counter: %lu\n", (unsigned long)scope->trigger_counter);
    my_printf->my_printf("In Trigger State: %s\n", scope->in_trigger ? "Yes" : "No");
    my_printf->my_printf("Buffer Size: %lu\n", (unsigned long)scope->buffer_size);
    my_printf->my_printf("Trigger Post Count: %lu\n", (unsigned long)scope->trigger_post_cnt);
}

void scope_printf_data(scope_t *scope, DEC_MY_PRINTF)
{
    uint32_t mask = 0u;

    if ((scope == NULL) || (my_printf == NULL))
    {
        return;
    }

    const uint32_t buf_size = scope->buffer_size;
    const uint32_t var_count = scope->var_count;
    float *buffer = scope->buffer;
    const char **var_names = scope->var_names;
    uint32_t trig_post_cnt = scope->trigger_post_cnt;
    uint32_t trig_idx = scope->trigger_index;
    int32_t start = (int32_t)trig_idx - (int32_t)trig_post_cnt;
    int32_t end = (int32_t)trig_idx + (int32_t)(buf_size - trig_post_cnt);

    my_printf->my_printf("\t");
    for (uint32_t v = 0u; v < var_count; ++v)
    {
        my_printf->my_printf("%s\t", (var_names && var_names[v]) ? var_names[v] : "var");
    }
    my_printf->my_printf("\r\n");

    for (uint32_t t = buf_size; t != 0u; t >>= 1)
    {
        mask = (mask << 1) | 1u;
    }
    int use_mask = ((mask + 1u) == buf_size);

    for (int32_t i = start; i < end; ++i)
    {
        uint32_t idx;
        if (use_mask != 0u)
        {
            idx = (uint32_t)i & mask;
        }
        else
        {
            idx = (uint32_t)(((i % (int32_t)buf_size) + (int32_t)buf_size) % (int32_t)buf_size);
        }

        float *row = buffer + idx;
        for (uint32_t v = 0u; v < var_count; ++v)
        {
            my_printf->my_printf("%s=%f,", var_names[v], (double)row[v * buf_size]);
        }
        if (i != (end - 1))
        {
            my_printf->my_printf("\r\n");
        }
    }
    my_printf->my_printf("\r\n");
}

typedef struct
{
    scope_t *scope;
    DEC_MY_PRINTF;
    int32_t cur;
    int32_t start;
    uint8_t active;
} scope_print_ctx_t;

static scope_print_ctx_t g_scope_print_ctx = {0};

void scope_printf_data_start(scope_t *scope, DEC_MY_PRINTF)
{
    if ((scope == NULL) || (my_printf == NULL) || (g_scope_print_ctx.active != 0u) || (scope->buffer_size == 0u))
    {
        return;
    }

    const uint32_t buf_size = scope->buffer_size;
    const uint32_t trig_post_cnt = scope->trigger_post_cnt;
    const uint32_t trig_idx = scope->trigger_index;

    g_scope_print_ctx.scope = scope;
    g_scope_print_ctx.my_printf = my_printf;
    g_scope_print_ctx.start = ((int32_t)trig_idx + (int32_t)trig_post_cnt + 1) % (int32_t)buf_size;
    g_scope_print_ctx.cur = g_scope_print_ctx.start;
    g_scope_print_ctx.active = 1u;
}

int scope_printf_data_step(void)
{
    if (g_scope_print_ctx.active == 0u)
    {
        return 0;
    }

    scope_t *scope = g_scope_print_ctx.scope;
    DEC_MY_PRINTF = g_scope_print_ctx.my_printf;
    const uint32_t buf_size = scope->buffer_size;
    const uint32_t var_count = scope->var_count;
    float *buffer = scope->buffer;
    uint32_t idx = (uint32_t)g_scope_print_ctx.cur % buf_size;
    float *row = buffer + idx;

    for (uint32_t v = 0u; v < var_count; ++v)
    {
        if (v == (var_count - 1u))
        {
            my_printf->my_printf("%f\n", (double)row[v * buf_size]);
        }
        else
        {
            my_printf->my_printf("%f,", (double)row[v * buf_size]);
        }
    }

    g_scope_print_ctx.cur = (g_scope_print_ctx.cur + 1) % (int32_t)buf_size;
    if (g_scope_print_ctx.cur == g_scope_print_ctx.start)
    {
        g_scope_print_ctx.active = 0u;
        return 0;
    }
    return 1;
}

int scope_printf_data_is_active(void)
{
    return g_scope_print_ctx.active;
}

static void scope_print_data(void)
{
    SCOPE_DATA_STEP_RUN();
}

REG_TASK_MS(1, scope_print_data)

#pragma GCC diagnostic pop
#else
void scope_printf_status(scope_t *scope, DEC_MY_PRINTF)
{
    (void)scope;
    (void)my_printf;
}

void scope_printf_data_start(scope_t *scope, DEC_MY_PRINTF)
{
    (void)scope;
    (void)my_printf;
}

int scope_printf_data_step(void)
{
    return 0;
}

int scope_printf_data_is_active(void)
{
    return 0;
}

void scope_printf_data(scope_t *scope, DEC_MY_PRINTF)
{
    (void)scope;
    (void)my_printf;
}
#endif

/* Registration */
REG_INIT(0, scope_service_init)
REG_TASK_MS(1, scope_service_poll_task)
REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_LIST_QUERY, scope_list_query_act)
REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_INFO_QUERY, scope_info_query_act)
REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_VAR_QUERY, scope_var_query_act)
REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_START, scope_start_act)
REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_TRIGGER, scope_trigger_act)
REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_STOP, scope_stop_act)
REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_RESET, scope_reset_act)
REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_SAMPLE_QUERY, scope_sample_query_act)
