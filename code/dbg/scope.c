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
    int32_t end = (int32_t)trig_idx + (int32_t)(buf_siz