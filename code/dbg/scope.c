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
 * @brief Scope采集主函数，周期性调用
 * @param scope Scope结构体指针
 * @details
 * - 空闲状态下根据is_running启动采集
 * - 采集所有变量到缓冲区（循环缓冲）
 * - 检查触发标志，进入触发采集状态
 * - 触发后采集trigger_post_cnt个点后停止
 */
__attribute__((always_inline, hot)) inline void scope_func(scope_t *scope)
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

/**
 * @brief 打印Scope状态信息
 * @param scope Scope结构体指针
 * @param my_printf 打印函数
 */
void scope_printf_status(scope_t *scope, DEC_MY_PRINTF)
{
    if (!scope || !my_printf)
        return;

    my_printf("Scope Status: %s\n", scope->is_running ? "Running" : "Idle");
    my_printf("Current Write Index: %u\n", scope->write_index);
    my_printf("Trigger Index: %u\n", scope->trigger_index);
    my_printf("Trigger Counter: %u\n", scope->trigger_counter);
    my_printf("In Trigger State: %s\n", scope->in_trigger ? "Yes" : "No");
    my_printf("Buffer Size: %u\n", scope->buffer_size);
    my_printf("Trigger Post Count: %u\n", scope->trigger_post_cnt);
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

    // 打印表头
    my_printf("\t");
    for (uint32_t v = 0; v < var_count; ++v)
        my_printf("%s\t", (var_names && var_names[v]) ? var_names[v] : "var");
    my_printf("\r\n");

    // 预计算起始索引，避免循环内重复计算
    int32_t start = (int32_t)trig_idx - (int32_t)trig_post_cnt;
    int32_t end = (int32_t)trig_idx + (int32_t)(buf_size - trig_post_cnt);

    // 优化：提前计算一次buf_size的掩码（假设buf_size为2的幂，否则用取模）
    uint32_t mask = 0;
    for (uint32_t t = buf_size; t; t >>= 1)
        mask = (mask << 1) | 1;
    int use_mask = ((mask + 1) == buf_size); // 判断是否2的幂

    for (int32_t i = start; i < end; ++i)
    {
        int32_t rel = i - (int32_t)trig_idx;
        uint32_t idx;
        if (use_mask)
            idx = (uint32_t)i & mask;
        else
            idx = ((i % (int32_t)buf_size) + buf_size) % buf_size;

        my_printf("%d\t", rel);

        // 优化：减少乘法，提前计算每变量的基址
        float *row = buffer + idx;
        for (uint32_t v = 0; v < var_count; ++v)
            my_printf("%f\t", row[v * buf_size]);
        if (i != end - 1)
            my_printf("\r\n");
    }
    my_printf("\r\n");
}

/**
 * @brief Scope分时打印数据上下文
 */
typedef struct
{
    scope_t *scope;
    void (*my_printf)(const char *__format, ...);
    int32_t cur;
    int32_t start;
    int32_t end;
    uint8_t active;
} scope_print_ctx_t;

static scope_print_ctx_t g_scope_print_ctx = {0};

/**
 * @brief 启动分时打印Scope数据
 * @param scope Scope结构体指针
 * @param my_printf 打印函数
 * @note 每次调用scope_printf_data_step打印一行，直到全部打印完成
 */
void scope_printf_data_start(scope_t *scope, DEC_MY_PRINTF)
{
    if (!scope || !my_printf || g_scope_print_ctx.active)
        return;

    const uint32_t buf_size = scope->buffer_size;
    const uint32_t trig_post_cnt = scope->trigger_post_cnt;
    const uint32_t trig_idx = scope->trigger_index;

    g_scope_print_ctx.scope = scope;
    g_scope_print_ctx.my_printf = my_printf;
    g_scope_print_ctx.start = (int32_t)trig_idx - (int32_t)trig_post_cnt;
    g_scope_print_ctx.end = (int32_t)trig_idx + (int32_t)(buf_size - trig_post_cnt);
    g_scope_print_ctx.cur = g_scope_print_ctx.start;
    g_scope_print_ctx.active = 1;

    // 打印表头
    my_printf("\t");
    for (uint32_t v = 0; v < scope->var_count; ++v)
        my_printf("%s\t", (scope->var_names && scope->var_names[v]) ? scope->var_names[v] : "var");
    my_printf("\r\n");
}

/**
 * @brief 分时打印Scope数据，每次调用打印一行
 * @return 1: 打印未完成，0: 打印已完成
 */
int scope_printf_data_step(void)
{
    if (!g_scope_print_ctx.active)
        return 0;

    scope_t *scope = g_scope_print_ctx.scope;
    void (*my_printf)(const char *__format, ...) = g_scope_print_ctx.my_printf;
    const uint32_t buf_size = scope->buffer_size;
    const uint32_t var_count = scope->var_count;
    float *buffer = scope->buffer;

    int32_t i = g_scope_print_ctx.cur;
    int32_t rel = i - (int32_t)scope->trigger_index;
    uint32_t idx;
    // 优化：掩码加速
    uint32_t mask = 0;
    for (uint32_t t = buf_size; t; t >>= 1)
        mask = (mask << 1) | 1;
    int use_mask = ((mask + 1) == buf_size);
    if (use_mask)
        idx = (uint32_t)i & mask;
    else
        idx = ((i % (int32_t)buf_size) + buf_size) % buf_size;

    my_printf("%d\t", rel);
    float *row = buffer + idx;
    for (uint32_t v = 0; v < var_count; ++v)
        my_printf("%f\t", row[v * buf_size]);
    my_printf("\r\n");

    g_scope_print_ctx.cur++;
    if (g_scope_print_ctx.cur >= g_scope_print_ctx.end)
    {
        g_scope_print_ctx.active = 0;
        return 0; // 打印完成
    }
    return 1; // 还有未打印的行
}

/**
 * @brief 判断Scope分时打印是否正在进行
 * @return 1: 正在打印，0: 空闲
 */
int scope_printf_data_is_active(void)
{
    return g_scope_print_ctx.active;
}
