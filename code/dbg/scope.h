/**
 * @file scope.h
 * @brief 软件示波器（Scope）模块头文件
 * @author Max.Li
 * @version 1.0
 * @date 2025-07-09
 *
 * @details
 * 提供多变量采集、触发、循环缓冲等功能，适用于嵌入式信号采集与调试。
 */

#ifndef __SCOPE_H
#define __SCOPE_H

#include "stdint.h"
#include "section.h"

//================= 类型定义 =================

/**
 * @brief Scope状态枚举
 */
typedef enum
{
    SCOPE_STATE_IDLE,      ///< 空闲状态
    SCOPE_STATE_RUNNING,   ///< 采集运行状态
    SCOPE_STATE_TRIGGERED, ///< 触发后采集状态
} scope_state_e;

/**
 * @brief Scope主控制结构体
 */
typedef struct
{
    uint32_t write_index;      ///< 当前写入位置
    uint32_t trigger_index;    ///< 触发点在缓冲区的索引
    uint8_t is_triggered;      ///< 触发标志
    uint8_t is_running;        ///< 运行标志
    uint32_t buffer_size;      ///< 缓冲区长度
    uint32_t trigger_post_cnt; ///< 触发后采集点数
    uint8_t var_count;         ///< 采集变量数量
    uint32_t trigger_counter;  ///< 触发后采集计数
    uint8_t in_trigger;        ///< 处于触发采集状态标志
    float *buffer;             ///< 采集缓冲区指针
    float **var_ptrs;          ///< 采集变量指针数组
    const char **var_names;    ///< 采集变量名指针数组
    scope_state_e state;       ///< 当前状态
} scope_t;

//================= 宏工具 =================

// 采集变量地址宏
#define SCOPE_ADDR(x) (&x)

// 变参宏展开工具
#define SCOPE_EXPAND(...) __VA_ARGS__

// FOR_EACH宏族
#define SCOPE_FOR_EACH_1(m, a) m(a)
#define SCOPE_FOR_EACH_2(m, a, ...) m(a), SCOPE_FOR_EACH_1(m, __VA_ARGS__)
#define SCOPE_FOR_EACH_3(m, a, ...) m(a), SCOPE_FOR_EACH_2(m, __VA_ARGS__)
#define SCOPE_FOR_EACH_4(m, a, ...) m(a), SCOPE_FOR_EACH_3(m, __VA_ARGS__)
#define SCOPE_FOR_EACH_5(m, a, ...) m(a), SCOPE_FOR_EACH_4(m, __VA_ARGS__)
#define SCOPE_FOR_EACH_6(m, a, ...) m(a), SCOPE_FOR_EACH_5(m, __VA_ARGS__)
#define SCOPE_FOR_EACH_7(m, a, ...) m(a), SCOPE_FOR_EACH_6(m, __VA_ARGS__)
#define SCOPE_FOR_EACH_8(m, a, ...) m(a), SCOPE_FOR_EACH_7(m, __VA_ARGS__)
#define SCOPE_FOR_EACH_9(m, a, ...) m(a), SCOPE_FOR_EACH_8(m, __VA_ARGS__)
#define SCOPE_FOR_EACH_10(m, a, ...) m(a), SCOPE_FOR_EACH_9(m, __VA_ARGS__)
#define SCOPE_FOR_EACH_N( \
    _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) SCOPE_FOR_EACH_##N
#define SCOPE_FOR_EACH(m, ...) \
    SCOPE_EXPAND(SCOPE_FOR_EACH_N(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)(m, __VA_ARGS__))

// 统计变参数量宏
#define SCOPE_COUNT_ARGS_N(_, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N
#define SCOPE_COUNT_ARGS(...) \
    SCOPE_EXPAND(SCOPE_COUNT_ARGS_N(_, __VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))

// 辅助宏：变量名转字符串
#define SCOPE_STR(x) #x

//================= Shell命令注册宏 =================

#define REG_SCOPE_STATUS_CMD(name)                     \
    static void scope_status_##name(DEC_MY_PRINTF)     \
    {                                                  \
        scope_printf_status(&scope_##name, my_printf); \
    }                                                  \
    REG_SHELL_CMD(scope_status_##name, scope_status_##name)

#define REG_SCOPE_START_CMD(name)                 \
    static void scope_start_##name(DEC_MY_PRINTF) \
    {                                             \
        scope_start(&scope_##name);               \
        my_printf("Scope started\r\n");           \
    }                                             \
    REG_SHELL_CMD(scope_start_##name, scope_start_##name)

#define SCOPE_DATA_STEP_START(name, my_printf) scope_printf_data_start(&scope_##name, my_printf)

#define REG_SCOPE_DATA_STEP_CMD(name)                                                             \
    static void scope_data_step_##name(DEC_MY_PRINTF) { SCOPE_DATA_STEP_START(name, my_printf); } \
    REG_SHELL_CMD(scope_data_step_##name, scope_data_step_##name)

//================= Scope对象注册宏 =================

/**
 * @brief 注册Scope采集对象
 * @param name 采集对象名称
 * @param buf_size 缓冲区长度
 * @param trig_post_cnt 触发后采集点数
 * @param ... 采集变量列表（float类型变量名）
 * @note 自动注册Scope对象及其状态、启动、分时打印Shell命令
 */
#define REG_SCOPE(name, buf_size, trig_post_cnt, ...)                                                               \
    float scope_##name##_buffer[SCOPE_COUNT_ARGS(__VA_ARGS__)][buf_size];                                           \
    float __VA_ARGS__;                                                                                              \
    float *scope_##name##_var_ptrs[SCOPE_COUNT_ARGS(__VA_ARGS__)] = {SCOPE_FOR_EACH(SCOPE_ADDR, __VA_ARGS__)};      \
    const char *scope_##name##_var_names[SCOPE_COUNT_ARGS(__VA_ARGS__)] = {SCOPE_FOR_EACH(SCOPE_STR, __VA_ARGS__)}; \
    scope_t scope_##name = {                                                                                        \
        .write_index = 0,                                                                                           \
        .trigger_index = 0,                                                                                         \
        .is_triggered = 0,                                                                                          \
        .is_running = 0,                                                                                            \
        .buffer_size = buf_size,                                                                                    \
        .var_count = SCOPE_COUNT_ARGS(__VA_ARGS__),                                                                 \
        .trigger_post_cnt = trig_post_cnt,                                                                          \
        .var_ptrs = &scope_##name##_var_ptrs[0],                                                                    \
        .buffer = &scope_##name##_buffer[0][0],                                                                     \
        .var_names = &scope_##name##_var_names[0],                                                                  \
        .trigger_counter = 0,                                                                                       \
        .in_trigger = 0,                                                                                            \
        .state = SCOPE_STATE_IDLE,                                                                                  \
    };                                                                                                              \
    REG_SCOPE_STATUS_CMD(name)                                                                                      \
    REG_SCOPE_START_CMD(name)                                                                                       \
    REG_SCOPE_DATA_STEP_CMD(name)

//================= Scope接口函数声明 =================

void scope_run(scope_t *scope);
void scope_start(scope_t *scope);
void scope_stop(scope_t *scope);
void scope_trigger(scope_t *scope);
void scope_reset(scope_t *scope);
void scope_printf_status(scope_t *scope, DEC_MY_PRINTF);
void scope_printf_data_start(scope_t *scope, DEC_MY_PRINTF);
int scope_printf_data_step(void);
int scope_printf_data_is_active(void);
void scope_func(scope_t *restrict p_str);
//================= Scope接口宏 =================

#define SCOPE_RUN(name) scope_run(&scope_##name)
#define SCOPE_TRIGGER(name) scope_trigger(&scope_##name)
#define SCOPE(name) scope_func(&scope_##name)
#define SCOPE_GET_BUFFER(name) (scope_##name.p_buffer)
#define SCOPE_GET_BUFFER_SIZE(name) (scope_##name.buffer_size)
#define SCOPE_GET_VAR_NUM(name) (scope_##name.var_num)
#define SCOPE_GET_VAR_PTRS(name) (scope_##name.p_var)
#define SCOPE_TRIGGER(name) scope_trigger(&scope_##name)

/**
 * @brief 分时打印Scope数据（宏，每次调用打印一行）
 */
#define SCOPE_DATA_STEP_RUN()              \
    do                                     \
    {                                      \
        if (scope_printf_data_is_active()) \
        {                                  \
            scope_printf_data_step();      \
        }                                  \
    } while (0)
/*
 * @brief 运行Scope采集对象
 * @param name 采集对象名称
 * @note 等价于 scope_func(&scope_##name)
 */
#define SCOPE(name) scope_func(&scope_##name)
/**
 * @brief 触发Scope采集（宏）
 * @param name 采集对象名称
 * @note 等价于 scope_trigger(&scope_##name)
 */
#define SCOPE_TRIGGER(name) scope_trigger(&scope_##name)

/**
 * @brief 分时打印Scope数据（宏，每次调用打印一行）
 */
#define SCOPE_DATA_STEP_RUN()              \
    do                                     \
    {                                      \
        if (scope_printf_data_is_active()) \
        {                                  \
            scope_printf_data_step();      \
        }                                  \
    } while (0)

#endif
