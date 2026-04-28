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
#include "my_math.h"

#ifndef SCOPE_ENABLE_PRINTF
#define SCOPE_ENABLE_PRINTF 0
#endif

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

typedef struct scope_service_obj_t
{
    uint8_t scope_id;
    const char *p_name;
    scope_t *p_scope;
    uint32_t sample_period_us;
    uint32_t capture_tag;
    uint8_t data_ready;
    scope_state_e last_state;
    struct scope_service_obj_t *p_next;
} scope_service_obj_t;

#define CMD_SET_SCOPE 0x01

#define CMD_WORD_SCOPE_LIST_QUERY 0x18
#define CMD_WORD_SCOPE_INFO_QUERY 0x19
#define CMD_WORD_SCOPE_VAR_QUERY 0x1A
#define CMD_WORD_SCOPE_START 0x1B
#define CMD_WORD_SCOPE_TRIGGER 0x1C
#define CMD_WORD_SCOPE_STOP 0x1D
#define CMD_WORD_SCOPE_RESET 0x1E
#define CMD_WORD_SCOPE_SAMPLE_QUERY 0x1F

typedef enum
{
    SCOPE_READ_MODE_NORMAL = 0,
    SCOPE_READ_MODE_FORCE = 1,
} scope_read_mode_e;

typedef enum
{
    SCOPE_TOOL_STATUS_OK = 0,
    SCOPE_TOOL_STATUS_SCOPE_ID_INVALID = 1,
    SCOPE_TOOL_STATUS_VAR_INDEX_INVALID = 2,
    SCOPE_TOOL_STATUS_SAMPLE_INDEX_INVALID = 3,
    SCOPE_TOOL_STATUS_RUNNING_DENIED = 4,
    SCOPE_TOOL_STATUS_DATA_NOT_READY = 5,
    SCOPE_TOOL_STATUS_BUSY = 6,
    SCOPE_TOOL_STATUS_CAPTURE_CHANGED = 7,
} scope_tool_status_e;

#pragma pack(1)
typedef struct
{
    uint8_t reserved;
} scope_list_query_t;

typedef struct
{
    uint8_t scope_id;
    uint8_t is_last;
    uint8_t name_len;
    uint8_t reserved;
} scope_list_item_t;

typedef struct
{
    uint8_t scope_id;
    uint8_t reserved[3];
} scope_info_query_t;

typedef struct
{
    uint8_t scope_id;
    uint8_t status;
    uint8_t state;
    uint8_t data_ready;
    uint8_t var_count;
    uint8_t reserved[3];
    uint32_t sample_count;
    uint32_t write_index;
    uint32_t trigger_index;
    uint32_t trigger_post_cnt;
    uint32_t trigger_display_index;
    uint32_t sample_period_us;
    uint32_t capture_tag;
} scope_info_ack_t;

typedef struct
{
    uint8_t scope_id;
    uint8_t var_index;
    uint8_t reserved[2];
} scope_var_query_t;

typedef struct
{
    uint8_t scope_id;
    uint8_t status;
    uint8_t var_index;
    uint8_t is_last;
    uint8_t name_len;
    uint8_t reserved[3];
} scope_var_ack_t;

typedef struct
{
    uint8_t scope_id;
    uint8_t status;
    uint8_t state;
    uint8_t data_ready;
    uint32_t capture_tag;
} scope_ctrl_ack_t;

typedef struct
{
    uint8_t scope_id;
    uint8_t read_mode;
    uint8_t reserved[2];
    uint32_t sample_index;
    uint32_t expected_capture_tag;
} scope_sample_query_t;

typedef struct
{
    uint8_t scope_id;
    uint8_t status;
    uint8_t read_mode;
    uint8_t var_count;
    uint32_t sample_index;
    uint32_t capture_tag;
    uint8_t is_last_sample;
    uint8_t reserved[3];
} scope_sample_ack_t;
#pragma pack()

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

#if SCOPE_ENABLE_PRINTF
#define REG_SCOPE_STATUS_CMD(name)                     \
    static void scope_status_##name(DEC_MY_PRINTF)     \
    {                                                  \
        scope_printf_status(&scope_##name, my_printf); \
    }                                                  \
    REG_SHELL_CMD(scp_sta_##name, scope_status_##name)

#define REG_SCOPE_START_CMD(name)                  \
    static void scope_start_##name(DEC_MY_PRINTF)  \
    {                                              \
        scope_start(&scope_##name);                \
        my_printf->my_printf("Scope started\r\n"); \
    }                                              \
    REG_SHELL_CMD(scp_start_##name, scope_start_##name)

#define SCOPE_DATA_STEP_START(name, my_printf) scope_printf_data_start(&scope_##name, my_printf)

#define REG_SCOPE_DATA_STEP_CMD(name)                                                             \
    static void scope_data_step_##name(DEC_MY_PRINTF) { SCOPE_DATA_STEP_START(name, my_printf); } \
    REG_SHELL_CMD(scp_pf_##name, scope_data_step_##name)
#else
#define REG_SCOPE_STATUS_CMD(name)
#define REG_SCOPE_START_CMD(name)
#define SCOPE_DATA_STEP_START(name, my_printf) ((void)0)
#define REG_SCOPE_DATA_STEP_CMD(name)
#endif

//================= Scope对象注册宏 =================

/**
 * @brief 注册Scope采集对象
 * @param name 采集对象名称
 * @param buf_size 缓冲区长度
 * @param trig_post_cnt 触发后采集点数
 * @param ... 采集变量列表（float类型变量名）
 * @note 自动注册Scope对象及其状态、启动、分时打印Shell命令
 */
#define REG_SCOPE_EX(name, buf_size, trig_post_cnt, _sample_period_us, ...)                                         \
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
        .var_names = &scope_##name##_var_names[0],           