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
#include "section.h"

/**
 * @brief Scope采集主函数
 * @param p_str scope_t结构体指针
 * @details
 * 1. 空闲状态下根据is_run启动采集
 * 2. 采集所有变量到缓冲区（循环缓冲）
 * 3. 检查触发标志，进入触发采集状态
 * 4. 触发后采集trig_point个点后停止
 */
__attribute__((always_inline, hot)) inline void scope_func(scope_t *restrict p_str)
{
    float *restrict buffer = (float *restrict)p_str->p_buffer;
    const uint32_t buf_size = p_str->buffer_size;
    const uint32_t var_num = p_str->var_num;

    // 状态机切换优化
    if (__builtin_expect(p_str->sta == SCOPE_IDLE, 0))
    {
        if (p_str->is_run)
        {
            p_str->sta = SCOPE_RUN;
            p_str->pos_cnt = 0;
            p_str->trig_cnt = 0;
        }
        else
        {
            return;
        }
    }

    uint32_t pos = p_str->pos_cnt;
    float **restrict p_vars = p_str->p_var;

    // 数据写入优化：指针运算，减少乘法
    float *restrict buf_base = buffer + pos;
    for (uint32_t i = 0; i < var_num; ++i)
    {
        buf_base[i * buf_size] = *(p_vars[i]);
    }

    // 环形缓冲区位置更新
    pos = (pos + 1);
    if (pos >= buf_size)
        pos = 0;
    p_str->pos_cnt = pos;

    // 状态处理优化
    if (__builtin_expect(p_str->sta == SCOPE_RUN, 1) && p_str->trig)
    {
        p_str->trig = 0;
        p_str->in_trig = 1;
        p_str->sta = SCOPE_IN_TRIG;
    }
    else if (p_str->sta == SCOPE_IN_TRIG)
    {
        if (++p_str->trig_cnt >= p_str->trig_point)
        {
            p_str->trig_cnt = 0;
            p_str->is_run = 0;
            p_str->in_trig = 0;
            p_str->sta = SCOPE_IDLE;
        }
    }
}

/**
 * @brief 启动Scope采集
 * @param scope scope_t结构体指针
 * @details
 * 仅在空闲状态下启动，复位相关计数
 */
void scope_start(scope_t *scope)
{
    if (scope && scope->sta == SCOPE_IDLE)
    {
        scope->is_run = 1;
        scope->pos_cnt = 0;
        scope->trig_cnt = 0;
        scope->in_trig = 0;
        scope->trig = 0;
    }
}

/**
 * @brief 停止Scope采集
 * @param scope scope_t结构体指针
 * @details
 * 立即停止采集并回到空闲状态
 */
void scope_stop(scope_t *scope)
{
    if (scope)
    {
        scope->is_run = 0;
        scope->sta = SCOPE_IDLE;
        scope->in_trig = 0;
    }
}

/**
 * @brief 触发Scope采集
 * @param scope scope_t结构体指针
 * @details
 * 仅在运行状态下设置触发标志
 */
void scope_trigger(scope_t *scope)
{
    if (scope && scope->sta == SCOPE_RUN)
    {
        scope->trig = 1;
    }
}

/**
 * @brief 重置Scope采集
 * @param scope scope_t结构体指针
 * @details
 * 清除所有计数和状态，回到空闲
 */
void scope_reset(scope_t *scope)
{
    if (scope)
    {
        scope->pos_cnt = 0;
        scope->trig_cnt = 0;
        scope->trig = 0;
        scope->is_run = 0;
        scope->in_trig = 0;
        scope->sta = SCOPE_IDLE;
    }
}

void scope_printf_status(scope_t *scope, DEC_MY_PRINTF)
{
    if (!scope || !my_printf)
        return;

    my_printf("Scope Status: %s\n", scope->is_run ? "Running" : "Idle");
    my_printf("Current Position: %u\n", scope->pos_cnt);
    my_printf("Trigger Count: %u\n", scope->trig_cnt);
    my_printf("In Trigger State: %s\n", scope->in_trig ? "Yes" : "No");
    my_printf("Buffer Size: %u\n", scope->buffer_size);
    my_printf("Trigger Point: %u\n", scope->trig_point);
}

void scope_printf_data(scope_t *scope, DEC_MY_PRINTF)
{
    if (!scope || !my_printf)
        return;

    const uint32_t buf_size = scope->buffer_size;
    const uint32_t var_num = scope->var_num;
    float *buffer = scope->p_buffer;
    float **var_ptrs = scope->p_var;
    const char **var_names = scope->p_var_name;
    uint32_t trig_point = scope->trig_point;
    uint32_t pos_cnt = scope->pos_cnt;

    // 打印表头
    my_printf("\t");
    for (uint32_t v = 0; v < var_num; ++v) {
        my_printf("%s\t", (var_names && var_names[v]) ? var_names[v] : "var");
    }
    my_printf("\r\n");

    // 计算触发点在环形缓冲区的位置
    uint32_t trig_pos = (pos_cnt + buf_size - trig_point) % buf_size;

    // 打印每一行数据
    for (int32_t i = -(int32_t)trig_point; i < (int32_t)(buf_size - trig_point); ++i) {
        int32_t idx = (int32_t)trig_pos + i;
        while (idx < 0) idx += buf_size;
        idx = idx % buf_size;

        my_printf("%d\t", i);

        for (uint32_t v = 0; v < var_num; ++v) {
            float val = buffer[v * buf_size + idx];
            my_printf("%f\t", val);
        }
        // 仅非最后一行打印换行
        if (i != (int32_t)(buf_size - trig_point) - 1)
            my_printf("\r\n");
    }
}
