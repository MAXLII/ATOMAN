/**
 * @file scope.c
 * @brief 软件示波器（Scope）模块实现文件
 * @author Max.Li
 * @version 1.0
 * @date 2025-07-10
 *
 * @details
 * 实现多变量采集、触发、循环缓冲等功能。
 */

#include "scope.h"
#include "comm.h"
#include "section.h"
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

/**
 * @brief Scope采集主函数，周期性调用
 * @param scope Scope结构体指针
 * @details
 * - 空闲状态下根据is_running启动采集
 * - 采集所有变量到缓冲区（循环缓冲）
 * - 检查触发标志，进入触发采集状态
 * - 触发后采集trigger_post_cnt个点后停止
 */
__attribute__((always_inline, hot)) inline void scope_run(scope_t *scope)
{
    float *buffer = scope->buffer;
    const uint32_t buf_size = scope->buffer_size;
    const uint32_t var_count = scope->var_count;

    // 状态机切换：空闲->运行
    if (__builtin_expect(scope->state == SCOPE_STATE_IDLE, 0))
    {
        if (scope->is_running)
        {
            scope->state = SCOPE_STATE_RUNNING;
            scope->write_index = 0;
            scope->trigger_counter = 0;
        }
        else
        {
            return;
        }
    }

    uint32_t write_idx = scope->write_index;
    float **var_ptrs = scope->var_ptrs;

    // 采集所有变量到缓冲区（每变量一列，循环缓冲）
    float *buf_base = buffer + write_idx;
    for (uint32_t i = 0; i < var_count; ++i)
    {
        buf_base[i * buf_size] = *(var_ptrs[i]);
    }

    // 触发处理
    if (__builtin_expect(scope->state == SCOPE_STATE_RUNNING, 1) && scope->is_triggered)
    {
        // 记录触发点索引，进入触发态
        scope->trigger_index = write_idx;
        scope->is_triggered = 0;
        scope->in_trigger = 1;
        scope->state = SCOPE_STATE_TRIGGERED;
    }
    else if (scope->state == SCOPE_STATE_TRIGGERED)
    {
        // 触发后采集trigger_post_cnt个点后停止
        if (++scope->trigger_counter >= scope->trigger_post_cnt)
        {
            scope->trigger_counter = 0;
            scope->is_running = 0;
            scope->in_trigger = 0;
            scope->state = SCOPE_STATE_IDLE;
        }
    }

    // 环形缓冲区写指针更新
    write_idx = (write_idx + 1);
    if (write_idx >= buf_size)
        write_idx = 0;
    scope->write_index = write_idx;
}

/**
 * @brief 启动Scope采集
 * @param scope Scope结构体指针
 * @details 仅在空闲状态下启动，复位相关计数
 */
void scope_start(scope_t *scope)
{
    if (scope && scope->state == SCOPE_STATE_IDLE)
    {
        scope->is_running = 1;
        scope->write_index = 0;
        scope->trigger_counter = 0;
        scope->in_trigger = 0;
        scope->is_triggered = 0;
    }
}

/**
 * @brief 停止Scope采集
 * @param scope Scope结构体指针
 * @details 立即停止采集并回到空闲状态
 */
void scope_stop(scope_t *scope)
{
    if (scope)
    {
        scope->is_running = 0;
        scope->state = SCOPE_STATE_IDLE;
        scope->in_trigger = 0;
    }
}

/**
 * @brief 触发Scope采集
 * @param scope Scope结构体指针
 * @details 仅在运行状态下设置触发标志
 */
void scope_trigger(scope_t *scope)
{
    if (scope && scope->state == SCOPE_STATE_RUNNING)
    {
        scope->is_triggered = 1;
    }
}

/**
 * @brief 重置Scope采集
 * @param scope Scope结构体指针
 * @details 清除所有计数和状态，回到空闲
 */
void scope_reset(scope_t *scope)
{
    if (scope)
    {
        scope->write_index = 0;
        scope->trigger_counter = 0;
        scope->is_triggered = 0;
        scope->is_running = 0;
        scope->in_trigger = 0;
        scope->state = SCOPE_STATE_IDLE;
    }
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
/**
 * @brief 打印Scope状态信息
 * @param scope Scope结构体指针
 * @param my_printf 打印函数
 */
void scope_printf_status(scope_t *scope, DEC_MY_PRINTF)
{
    if (!scope || !my_printf)
        return;

    my_printf->my_printf("Scope Status: %s\n", scope->is_running ? "Running" : "Idle");
    my_printf->my_printf("Current Write Index: %u\n", scope->write_index);
    my_printf->my_printf("Trigger Index: %u\n", scope->trigger_index);
    my_printf->my_printf("Trigger Counter: %u\n", scope->trigger_counter);
    my_printf->my_printf("In Trigger State: %s\n", scope->in_trigger ? "Yes" : "No");
    my_printf->my_printf("Buffer Size: %u\n", scope->buffer_size);
    my_printf->my_printf("Trigger Post Count: %u\n", scope->trigger_post_cnt);
}

/**
 * @brief 打印Scope采集数据（高性能实现，减少除法和取模运算）
 * @param scope Scope结构体指针
 * @param my_printf 打印函数
 * @details 以trigger_index为原点，原点行为0，前后数据对齐
 */
void scope_printf_data(scope_t *scope, DEC_MY_PRINTF)
{
    if (!scope || !my_printf)
        return;

    const uint32_t buf_size = scope->buffer_size;
    const uint32_t var_count = scope->var_count;
    float *buffer = scope->buffer;
    const char **var_names = scope->var_names;
    uint32_t trig_post_cnt = scope->trigger_post_cnt;
    uint32_t trig_idx = scope->trigger_index;

    my_printf->my_printf("\t");
    for (uint32_t v = 0; v < var_count; ++v)
        my_printf->my_printf("%s\t", (var_names && var_names[v]) ? var_names[v] : "var");
    my_printf->my_printf("\r\n");

    int32_t start = (int32_t)trig_idx - (int32_t)trig_post_cnt;
    int32_t end = (int32_t)trig_idx + (int32_t)(buf_size - trig_post_cnt);

    uint32_t mask = 0;
    for (uint32_t t = buf_size; t; t >>= 1)
        mask = (mask << 1) | 1;
    int use_mask = ((mask + 1) == buf_size);

    for (int32_t i = start; i < end; ++i)
    {
        uint32_t idx;
        if (use_mask)
            idx = (uint32_t)i & mask;
        else
            idx = ((i % (int32_t)buf_size) + buf_size) % buf_size;

        float *row = buffer + idx;
        for (uint32_t v = 0; v < var_count; ++v)
            my_printf->my_printf("%s=%f,", var_names[v], row[v * buf_size]);
        if (i != end - 1)
            my_printf->my_printf("\r\n");
    }
    my_printf->my_printf("\r\n");
}

typedef struct
{
    scope_t *scope;
    DEC_MY_PRINTF;
    int32_t cur;
    int32_t start;
    int32_t end;
    uint8_t active;
} scope_print_ctx_t;

static scope_print_ctx_t g_scope_print_ctx = {0};

void scope_printf_data_start(scope_t *scope, DEC_MY_PRINTF)
{
    if (!scope || !my_printf || g_scope_print_ctx.active)
        return;

    const uint32_t buf_size = scope->buffer_size;
    const uint32_t trig_post_cnt = scope->trigger_post_cnt;
    const uint32_t trig_idx = scope->trigger_index;

    g_scope_print_ctx.scope = scope;
    g_scope_print_ctx.my_printf = my_printf;
    g_scope_print_ctx.start = ((int32_t)trig_idx + (int32_t)trig_post_cnt + 1) % buf_size;
    g_scope_print_ctx.end = ((int32_t)trig_idx + (int32_t)trig_post_cnt) % buf_size;
    g_scope_print_ctx.cur = g_scope_print_ctx.start;
    g_scope_print_ctx.active = 1;
}

int scope_printf_data_step(void)
{
    if (!g_scope_print_ctx.active)
        return 0;

    scope_t *scope = g_scope_print_ctx.scope;
    DEC_MY_PRINTF = g_scope_print_ctx.my_printf;
    const uint32_t buf_size = scope->buffer_size;
    const uint32_t var_count = scope->var_count;
    float *buffer = scope->buffer;
    uint32_t idx = g_scope_print_ctx.cur % buf_size;

    float *row = buffer + idx;
    for (uint32_t v = 0; v < var_count; ++v)
        if (v == var_count - 1)
            my_printf->my_printf("%f\n", row[v * buf_size]);
        else
            my_printf->my_printf("%f,", row[v * buf_size]);

    g_scope_print_ctx.cur = (g_scope_print_ctx.cur + 1) % buf_size;
    if (g_scope_print_ctx.cur == g_scope_print_ctx.start)
    {
        g_scope_print_ctx.active = 0;
        return 0;
    }
    return 1;
}

int scope_printf_data_is_active(void)
{
    return g_scope_print_ctx.active;
}

void scope_print_data(void)
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

static void scope_service_list_query_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    scope_service_obj_t *p_obj = g_scope_service_first;

    if ((p_pack == NULL) || (p_pack->is_ack == 1u))
    {
        return;
    }

    while (p_obj != NULL)
    {
        const uint8_t name_len = (uint8_t)strlen(p_obj->p_name);
        uint8_t tx_buffer[sizeof(scope_list_item_t) + name_len];
        scope_list_item_t item = {0};

        item.scope_id = p_obj->scope_id;
        item.is_last = (p_obj->p_next == NULL) ? 1u : 0u;
        item.name_len = name_len;
        memcpy((uint8_t *)tx_buffer, (uint8_t *)&item, sizeof(item));
        memcpy((uint8_t *)&tx_buffer[sizeof(item)], (const uint8_t *)p_obj->p_name, name_len);
        scope_service_reply(p_pack,
                            my_printf,
                            CMD_WORD_SCOPE_LIST_QUERY,
                            0u,
                            (uint8_t *)tx_buffer,
                            (uint16_t)(sizeof(item) + name_len));
        p_obj = p_obj->p_next;
    }

    if (g_scope_service_first == NULL)
    {
        scope_list_item_t item = {0};

        item.scope_id = 0xFFu;
        item.is_last = 1u;
        item.name_len = 0u;
        scope_service_reply(p_pack,
                            my_printf,
                            CMD_WORD_SCOPE_LIST_QUERY,
                            0u,
                            (uint8_t *)&item,
                            sizeof(item));
    }
}

REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_LIST_QUERY, scope_service_list_query_act)

static void scope_service_info_query_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    scope_info_ack_t ack = {0};
    scope_info_query_t query = {0};
    scope_service_obj_t *p_obj = NULL;

    if ((p_pack == NULL) || (p_pack->is_ack == 1u))
    {
        return;
    }

    if (p_pack->len == sizeof(query))
    {
        memcpy((uint8_t *)&query, p_pack->p_data, sizeof(query));
        p_obj = scope_service_find_by_id(query.scope_id);
    }

    if (p_obj == NULL)
    {
        ack.status = SCOPE_TOOL_STATUS_SCOPE_ID_INVALID;
    }
    else
    {
        ack.scope_id = p_obj->scope_id;
        ack.status = SCOPE_TOOL_STATUS_OK;
        ack.state = (uint8_t)p_obj->p_scope->state;
        ack.data_ready = p_obj->data_ready;
        ack.var_count = p_obj->p_scope->var_count;
        ack.sample_count = p_obj->p_scope->buffer_size;
        ack.write_index = p_obj->p_scope->write_index;
        ack.trigger_index = p_obj->p_scope->trigger_index;
        ack.trigger_post_cnt = p_obj->p_scope->trigger_post_cnt;
        ack.trigger_display_index = scope_service_get_trigger_display_index(p_obj->p_scope);
        ack.sample_period_us = p_obj->sample_period_us;
        ack.capture_tag = p_obj->capture_tag;
    }

    scope_service_reply(p_pack,
                        my_printf,
                        CMD_WORD_SCOPE_INFO_QUERY,
                        1u,
                        (uint8_t *)&ack,
                        sizeof(ack));
}

REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_INFO_QUERY, scope_service_info_query_act)

static void scope_service_var_query_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    scope_var_ack_t ack = {0};
    scope_var_query_t query = {0};
    scope_service_obj_t *p_obj = NULL;
    uint8_t tx_buffer[sizeof(scope_var_ack_t) + SHELL_STR_SIZE_MAX] = {0};

    if ((p_pack == NULL) || (p_pack->is_ack == 1u))
    {
        return;
    }

    if (p_pack->len == sizeof(query))
    {
        memcpy((uint8_t *)&query, p_pack->p_data, sizeof(query));
        p_obj = scope_service_find_by_id(query.scope_id);
    }

    if (p_obj == NULL)
    {
        ack.status = SCOPE_TOOL_STATUS_SCOPE_ID_INVALID;
        memcpy((uint8_t *)tx_buffer, (uint8_t *)&ack, sizeof(ack));
        scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_VAR_QUERY, 1u, (uint8_t *)tx_buffer, sizeof(ack));
        return;
    }

    ack.scope_id = p_obj->scope_id;
    ack.var_index = query.var_index;
    if (query.var_index >= p_obj->p_scope->var_count)
    {
        ack.status = SCOPE_TOOL_STATUS_VAR_INDEX_INVALID;
        memcpy((uint8_t *)tx_buffer, (uint8_t *)&ack, sizeof(ack));
        scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_VAR_QUERY, 1u, (uint8_t *)tx_buffer, sizeof(ack));
        return;
    }

    ack.status = SCOPE_TOOL_STATUS_OK;
    ack.is_last = (query.var_index >= (uint8_t)(p_obj->p_scope->var_count - 1u)) ? 1u : 0u;
    ack.name_len = (uint8_t)strlen(p_obj->p_scope->var_names[query.var_index]);
    memcpy((uint8_t *)tx_buffer, (uint8_t *)&ack, sizeof(ack));
    memcpy((uint8_t *)&tx_buffer[sizeof(ack)],
           (const uint8_t *)p_obj->p_scope->var_names[query.var_index],
           ack.name_len);
    scope_service_reply(p_pack,
                        my_printf,
                        CMD_WORD_SCOPE_VAR_QUERY,
                        1u,
                        (uint8_t *)tx_buffer,
                        (uint16_t)(sizeof(ack) + ack.name_len));
}

REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_VAR_QUERY, scope_service_var_query_act)

static void scope_service_start_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    scope_ctrl_ack_t ack = {0};
    scope_info_query_t query = {0};
    scope_service_obj_t *p_obj = NULL;
    scope_tool_status_e status = SCOPE_TOOL_STATUS_SCOPE_ID_INVALID;

    if ((p_pack == NULL) || (p_pack->is_ack == 1u))
    {
        return;
    }

    if (p_pack->len == sizeof(query))
    {
        memcpy((uint8_t *)&query, p_pack->p_data, sizeof(query));
        p_obj = scope_service_find_by_id(query.scope_id);
    }

    if (p_obj != NULL)
    {
        if (p_obj->p_scope->state == SCOPE_STATE_IDLE)
        {
            scope_start(p_obj->p_scope);
            p_obj->data_ready = 0u;
            p_obj->capture_tag++;
            p_obj->last_state = p_obj->p_scope->state;
            status = SCOPE_TOOL_STATUS_OK;
        }
        else
        {
            status = SCOPE_TOOL_STATUS_BUSY;
        }
    }

    scope_service_fill_ctrl_ack(p_obj, status, &ack);
    scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_START, 1u, (uint8_t *)&ack, sizeof(ack));
}

REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_START, scope_service_start_act)

static void scope_service_trigger_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    scope_ctrl_ack_t ack = {0};
    scope_info_query_t query = {0};
    scope_service_obj_t *p_obj = NULL;
    scope_tool_status_e status = SCOPE_TOOL_STATUS_SCOPE_ID_INVALID;

    if ((p_pack == NULL) || (p_pack->is_ack == 1u))
    {
        return;
    }

    if (p_pack->len == sizeof(query))
    {
        memcpy((uint8_t *)&query, p_pack->p_data, sizeof(query));
        p_obj = scope_service_find_by_id(query.scope_id);
    }

    if (p_obj != NULL)
    {
        if (p_obj->p_scope->state == SCOPE_STATE_RUNNING)
        {
            scope_trigger(p_obj->p_scope);
            p_obj->last_state = p_obj->p_scope->state;
            status = SCOPE_TOOL_STATUS_OK;
        }
        else
        {
            status = SCOPE_TOOL_STATUS_BUSY;
        }
    }

    scope_service_fill_ctrl_ack(p_obj, status, &ack);
    scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_TRIGGER, 1u, (uint8_t *)&ack, sizeof(ack));
}

REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_TRIGGER, scope_service_trigger_act)

static void scope_service_stop_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    scope_ctrl_ack_t ack = {0};
    scope_info_query_t query = {0};
    scope_service_obj_t *p_obj = NULL;
    scope_tool_status_e status = SCOPE_TOOL_STATUS_SCOPE_ID_INVALID;

    if ((p_pack == NULL) || (p_pack->is_ack == 1u))
    {
        return;
    }

    if (p_pack->len == sizeof(query))
    {
        memcpy((uint8_t *)&query, p_pack->p_data, sizeof(query));
        p_obj = scope_service_find_by_id(query.scope_id);
    }

    if (p_obj != NULL)
    {
        scope_stop(p_obj->p_scope);
        p_obj->data_ready = 1u;
        p_obj->last_state = p_obj->p_scope->state;
        status = SCOPE_TOOL_STATUS_OK;
    }

    scope_service_fill_ctrl_ack(p_obj, status, &ack);
    scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_STOP, 1u, (uint8_t *)&ack, sizeof(ack));
}

REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_STOP, scope_service_stop_act)

static void scope_service_reset_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    scope_ctrl_ack_t ack = {0};
    scope_info_query_t query = {0};
    scope_service_obj_t *p_obj = NULL;
    scope_tool_status_e status = SCOPE_TOOL_STATUS_SCOPE_ID_INVALID;

    if ((p_pack == NULL) || (p_pack->is_ack == 1u))
    {
        return;
    }

    if (p_pack->len == sizeof(query))
    {
        memcpy((uint8_t *)&query, p_pack->p_data, sizeof(query));
        p_obj = scope_service_find_by_id(query.scope_id);
    }

    if (p_obj != NULL)
    {
        scope_reset(p_obj->p_scope);
        p_obj->data_ready = 0u;
        p_obj->last_state = p_obj->p_scope->state;
        status = SCOPE_TOOL_STATUS_OK;
    }

    scope_service_fill_ctrl_ack(p_obj, status, &ack);
    scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_RESET, 1u, (uint8_t *)&ack, sizeof(ack));
}

REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_RESET, scope_service_reset_act)

static void scope_service_sample_query_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    scope_sample_query_t query = {0};
    scope_sample_ack_t ack = {0};
    scope_service_obj_t *p_obj = NULL;
    uint8_t tx_buffer[sizeof(scope_sample_ack_t) + SCOPE_SERVICE_VAR_COUNT_MAX * sizeof(float)] = {0};

    if ((p_pack == NULL) || (p_pack->is_ack == 1u))
    {
        return;
    }

    if (p_pack->len == sizeof(query))
    {
        memcpy((uint8_t *)&query, p_pack->p_data, sizeof(query));
        p_obj = scope_service_find_by_id(query.scope_id);
    }

    if (p_obj == NULL)
    {
        ack.status = SCOPE_TOOL_STATUS_SCOPE_ID_INVALID;
        memcpy((uint8_t *)tx_buffer, (uint8_t *)&ack, sizeof(ack));
        scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_SAMPLE_QUERY, 1u, (uint8_t *)tx_buffer, sizeof(ack));
        return;
    }

    ack.scope_id = p_obj->scope_id;
    ack.read_mode = query.read_mode;
    ack.var_count = p_obj->p_scope->var_count;
    ack.sample_index = query.sample_index;
    ack.capture_tag = p_obj->capture_tag;
    if (query.sample_index >= p_obj->p_scope->buffer_size)
    {
        ack.status = SCOPE_TOOL_STATUS_SAMPLE_INDEX_INVALID;
        memcpy((uint8_t *)tx_buffer, (uint8_t *)&ack, sizeof(ack));
        scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_SAMPLE_QUERY, 1u, (uint8_t *)tx_buffer, sizeof(ack));
        return;
    }

    if ((query.read_mode == SCOPE_READ_MODE_NORMAL) && (p_obj->p_scope->state != SCOPE_STATE_IDLE))
    {
        ack.status = SCOPE_TOOL_STATUS_RUNNING_DENIED;
        memcpy((uint8_t *)tx_buffer, (uint8_t *)&ack, sizeof(ack));
        scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_SAMPLE_QUERY, 1u, (uint8_t *)tx_buffer, sizeof(ack));
        return;
    }

    if ((query.read_mode == SCOPE_READ_MODE_NORMAL) && (p_obj->data_ready == 0u))
    {
        ack.status = SCOPE_TOOL_STATUS_DATA_NOT_READY;
        memcpy((uint8_t *)tx_buffer, (uint8_t *)&ack, sizeof(ack));
        scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_SAMPLE_QUERY, 1u, (uint8_t *)tx_buffer, sizeof(ack));
        return;
    }

    if ((query.expected_capture_tag != 0u) && (query.expected_capture_tag != p_obj->capture_tag))
    {
        ack.status = SCOPE_TOOL_STATUS_CAPTURE_CHANGED;
        memcpy((uint8_t *)tx_buffer, (uint8_t *)&ack, sizeof(ack));
        scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_SAMPLE_QUERY, 1u, (uint8_t *)tx_buffer, sizeof(ack));
        return;
    }

    if (p_obj->p_scope->var_count > SCOPE_SERVICE_VAR_COUNT_MAX)
    {
        ack.status = SCOPE_TOOL_STATUS_BUSY;
        memcpy((uint8_t *)tx_buffer, (uint8_t *)&ack, sizeof(ack));
        scope_service_reply(p_pack, my_printf, CMD_WORD_SCOPE_SAMPLE_QUERY, 1u, (uint8_t *)tx_buffer, sizeof(ack));
        return;
    }

    ack.status = SCOPE_TOOL_STATUS_OK;
    ack.is_last_sample = (query.sample_index >= (p_obj->p_scope->buffer_size - 1u)) ? 1u : 0u;
    memcpy((uint8_t *)tx_buffer, (uint8_t *)&ack, sizeof(ack));
    if (p_obj->p_scope->var_count > 0u)
    {
        uint32_t physical_index = scope_service_logical_to_physical_index(p_obj->p_scope,
                                                                          query.read_mode,
                                                                          query.sample_index);
        for (uint8_t var_index = 0u; var_index < p_obj->p_scope->var_count; var_index++)
        {
            float value = p_obj->p_scope->buffer[var_index * p_obj->p_scope->buffer_size + physical_index];
            memcpy((uint8_t *)&tx_buffer[sizeof(ack) + var_index * sizeof(float)],
                   (uint8_t *)&value,
                   sizeof(float));
        }
    }
    scope_service_reply(p_pack,
                        my_printf,
                        CMD_WORD_SCOPE_SAMPLE_QUERY,
                        1u,
                        (uint8_t *)tx_buffer,
                        (uint16_t)(sizeof(ack) + p_obj->p_scope->var_count * sizeof(float)));
}

REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_SAMPLE_QUERY, scope_service_sample_query_act)

static void scope_service_state_task(void)
{
    scope_service_obj_t *p_obj = g_scope_service_first;

    while (p_obj != NULL)
    {
        if ((p_obj->last_state != SCOPE_STATE_IDLE) &&
            (p_obj->p_scope->state == SCOPE_STATE_IDLE))
        {
            p_obj->data_ready = 1u;
        }
        else if (p_obj->p_scope->state != SCOPE_STATE_IDLE)
        {
            p_obj->data_ready = 0u;
        }
        p_obj->last_state = p_obj->p_scope->state;
        p_obj = p_obj->p_next;
    }
}

REG_TASK_MS(1, scope_service_state_task)

#pragma GCC diagnostic pop
