// SPDX-License-Identifier: MIT
/**
 * @file    scope_service.c
 * @brief   Scope service module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Register scope objects for shell and communication access
 *          - Provide deferred scope data printing
 *          - Provide helpers for scope binary reporting services
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
#include "shell.h"

#include <string.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"

#define SCOPE_SERVICE_VAR_COUNT_MAX 10u

static scope_service_obj_t *g_scope_service_first = NULL;
static uint8_t g_scope_service_count = 0u;

static scope_service_obj_t *scope_service_find_by_id(uint8_t scope_id)
{
    scope_service_obj_t *p_obj = g_scope_service_first;
    while (p_obj != NULL)
    {
        if (p_obj->scope_id == scope_id)
        {
            return p_obj;
        }
        p_obj = p_obj->p_next;
    }
    return NULL;
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
    uint32_t start_index = scope_service_get_logical_start_index(p_scope, read_mode);
    if ((p_scope == NULL) || (p_scope->buffer_size == 0u))
    {
        return 0u;
    }
    return (start_index + logical_index) % p_scope->buffer_size;
}

static void scope_service_fill_ctrl_ack(scope_service_obj_t *p_obj, scope_tool_status_e status, scope_ctrl_ack_t *p_ack)
{
    memset((uint8_t *)p_ack, 0, sizeof(*p_ack));
    if (p_obj == NULL)
    {
        p_ack->status = (uint8_t)status;
        return;
    }

    p_ack->scope_id = p_obj->scope_id;
    p_ack->status = (uint8_t)status;
    p_ack->state = (uint8_t)p_obj->p_scope->state;
    p_ack->data_ready = p_obj->data_ready;
    p_ack->capture_tag = p_obj->capture_tag;
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

void scope_service_register(scope_service_obj_t *p_obj)
{
    scope_service_obj_t *p_tail = NULL;

    if ((p_obj == NULL) || (p_obj->p_scope == NULL) || (p_obj->p_name == NULL))
    {
        return;
    }

    p_obj->scope_id = g_scope_service_count;
    p_obj->capture_tag = 0u;
    p_obj->data_ready = 0u;
    p_obj->last_state = p_obj->p_scope->state;
    p_obj->p_next = NULL;

    if (g_scope_service_first == NULL)
    {
        g_scope_service_first = p_obj;
    }
    else
    {
        p_tail = g_scope_service_first;
        while (p_tail->p_next != NULL)
        {
            p_tail = p_tail->p_next;
        }
        p_tail->p_next = p_obj;
    }
    g_scope_service_count++;
}

#if SCOPE_ENABLE_PRINTF
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
        if (use_mask)
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
#endif

static void scope_service_keep_helpers(void)
{
    (void)scope_service_find_by_id;
    (void)scope_service_get_trigger_display_index;
    (void)scope_service_logical_to_physical_index;
    (void)scope_service_fill_ctrl_ack;
    (void)scope_service_reply;
    (void)SCOPE_SERVICE_VAR_COUNT_MAX;
}

REG_INIT(0, scope_service_keep_helpers)

#pragma GCC diagnostic pop
