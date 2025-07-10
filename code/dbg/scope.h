/**
 * @file scope.h
 * @brief 软件示波器（Scope）模块头文件
 * @author
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

/**
 * @brief Scope状态枚举
 */
typedef enum
{
    SCOPE_IDLE,     ///< 空闲状态
    SCOPE_RUN,      ///< 采集运行状态
    SCOPE_IN_TRIG,  ///< 触发后采集状态
} SCOPE_E;

/**
 * @brief Scope主控制结构体
 * @details
 * 采集变量指针、缓冲区、状态、触发点等信息。
 */
typedef struct
{
    uint32_t pos_cnt;      ///< 当前采集位置
    uint32_t trig;         ///< 触发标志
    uint8_t is_run;        ///< 运行标志
    uint32_t buffer_size;  ///< 缓冲区长度
    uint32_t trig_point;   ///< 触发后采集点数
    uint8_t var_num;       ///< 采集变量数量
    uint32_t trig_cnt;     ///< 触发后采集计数
    uint32_t in_trig;      ///< 处于触发采集状态标志
    float *p_buffer;       ///< 采集缓冲区指针
    float **p_var;         ///< 采集变量指针数组
    const char **p_var_name; ///< 采集变量名指针数组
    uint8_t sta;           ///< 当前状态（SCOPE_E）
} scope_t;

/**
 * @brief 采集变量地址宏
 */
#define ADDR(x) &x

/**
 * @brief 变参宏展开工具
 */
#define EXPAND(...) __VA_ARGS__

/**
 * @brief FOR_EACH宏族：对每个参数应用宏m
 * @note 用于生成变量指针数组
 */
#define FOR_EACH_1(m, a) m(a)
#define FOR_EACH_2(m, a, ...) m(a), FOR_EACH_1(m, __VA_ARGS__)
#define FOR_EACH_3(m, a, ...) m(a), FOR_EACH_2(m, __VA_ARGS__)
#define FOR_EACH_4(m, a, ...) m(a), FOR_EACH_3(m, __VA_ARGS__)
#define FOR_EACH_5(m, a, ...) m(a), FOR_EACH_4(m, __VA_ARGS__)
#define FOR_EACH_6(m, a, ...) m(a), FOR_EACH_5(m, __VA_ARGS__)
#define FOR_EACH_7(m, a, ...) m(a), FOR_EACH_6(m, __VA_ARGS__)
#define FOR_EACH_8(m, a, ...) m(a), FOR_EACH_7(m, __VA_ARGS__)
#define FOR_EACH_9(m, a, ...) m(a), FOR_EACH_8(m, __VA_ARGS__)
#define FOR_EACH_10(m, a, ...) m(a), FOR_EACH_9(m, __VA_ARGS__)
#define FOR_EACH_11(m, a, ...) m(a), FOR_EACH_10(m, __VA_ARGS__)
#define FOR_EACH_12(m, a, ...) m(a), FOR_EACH_11(m, __VA_ARGS__)
#define FOR_EACH_13(m, a, ...) m(a), FOR_EACH_12(m, __VA_ARGS__)
#define FOR_EACH_14(m, a, ...) m(a), FOR_EACH_13(m, __VA_ARGS__)
#define FOR_EACH_15(m, a, ...) m(a), FOR_EACH_14(m, __VA_ARGS__)
#define FOR_EACH_N(          \
    _1, _2, _3, _4, _5,      \
    _6, _7, _8, _9, _10,     \
    _11, _12, _13, _14, _15, \
    N, ...) FOR_EACH_##N
#define FOR_EACH(m, ...)                  \
    EXPAND(FOR_EACH_N(__VA_ARGS__,        \
                      15, 14, 13, 12, 11, \
                      10, 9, 8, 7, 6,     \
                      5, 4, 3, 2, 1)(m, __VA_ARGS__))

/**
 * @brief 统计变参数量宏
 */
#define COUNT_ARGS_N(                      \
    _, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
    _10, _11, _12, _13, _14, _15, _16, N, ...) N

#define COUNT_ARGS(...)                         \
    EXPAND(COUNT_ARGS_N(_,                      \
                        __VA_ARGS__,            \
                        16, 15, 14, 13, 12, 11, \
                        10, 9, 8, 7, 6,         \
                        5, 4, 3, 2, 1, 0))

// 辅助宏：将变量名转为字符串
#define STR(x) #x

/**
 * @brief 注册Scope采集对象
 * @param name 采集对象名称
 * @param _buffer_size 缓冲区长度
 * @param _trig_point 触发后采集点数
 * @param ... 采集变量列表（float类型变量名）
 * @note 自动生成缓冲区、变量指针数组、变量名数组和scope_t对象
 * @example
 * REG_SCOPE(my_scope, 256, 64, var1, var2, var3)
 */
#define REG_SCOPE(name, _buffer_size, _trig_point, ...)                                \
    float scope_buffer_##name[EXPAND(COUNT_ARGS(__VA_ARGS__))][_buffer_size];          \
    float __VA_ARGS__;                                                                 \
    float *scope_var_##name[EXPAND(COUNT_ARGS(__VA_ARGS__))] = {FOR_EACH(ADDR, __VA_ARGS__)}; \
    const char *scope_name_##name[EXPAND(COUNT_ARGS(__VA_ARGS__))] = {FOR_EACH(STR, __VA_ARGS__)}; \
    scope_t scope_##name = {                                                           \
        .pos_cnt = 0,                                                                  \
        .trig = 0,                                                                     \
        .is_run = 0,                                                                   \
        .buffer_size = _buffer_size,                                                   \
        .var_num = EXPAND(COUNT_ARGS(__VA_ARGS__)),                                    \
        .trig_point = _trig_point,                                                     \
        .p_var = &scope_var_##name[0],                                                 \
        .p_buffer = &scope_buffer_##name[0][0],                                        \
        .p_var_name = &scope_name_##name[0],                                           \
        .trig_cnt = 0,                                                                 \
        .in_trig = 0,                                                                  \
        .sta = SCOPE_IDLE,                                                             \
    };

/**
 * @brief 获取Scope缓冲区指针
 * @param name 采集对象名称
 */
#define SCOPE_GET_BUFFER(name) (scope_##name.p_buffer)

/**
 * @brief 获取Scope缓冲区长度
 * @param name 采集对象名称
 */
#define SCOPE_GET_BUFFER_SIZE(name) (scope_##name.buffer_size)

/**
 * @brief 获取Scope采集变量数量
 * @param name 采集对象名称
 */
#define SCOPE_GET_VAR_NUM(name) (scope_##name.var_num)

/**
 * @brief 获取Scope变量指针数组
 * @param name 采集对象名称
 */
#define SCOPE_GET_VAR_PTRS(name) (scope_##name.p_var)

/**
 * @brief 运行Scope采集对象
 * @param name 采集对象名称
 * @note 等价于 scope_func(&scope_##name)
 */
#define SCOPE(name) scope_func(&scope_##name)

/**
 * @brief 打印Scope状态信息
 * @param name 采集对象名称
 * @param my_printf 打印函数
 */
#define SCOPE_PRINTF_STATUS(name, my_printf) scope_printf_status(&scope_##name, my_printf)

/**
 * @brief 打印Scope采集数据
 * @param name 采集对象名称
 * @param my_printf 打印函数
 */
#define SCOPE_PRINTF_DATA(name, my_printf) scope_printf_data(&scope_##name, my_printf)

/**
 * @brief Scope采集主函数
 * @param p_str scope_t结构体指针
 * @note 需周期性调用，实现变量采集与触发处理
 */
void scope_func(scope_t *restrict p_str);

/**
 * @brief 启动Scope采集
 * @param scope scope_t结构体指针
 */
void scope_start(scope_t *scope);

/**
 * @brief 停止Scope采集
 * @param scope scope_t结构体指针
 */
void scope_stop(scope_t *scope);

/**
 * @brief 触发Scope采集
 * @param scope scope_t结构体指针
 */
void scope_trigger(scope_t *scope);

/**
 * @brief 重置Scope采集
 * @param scope scope_t结构体指针
 */
void scope_reset(scope_t *scope);

void scope_printf_status(scope_t *scope, DEC_MY_PRINTF);
void scope_printf_data(scope_t *scope, DEC_MY_PRINTF);

#endif
