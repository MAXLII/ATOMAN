/**
 * @file section.h
 * @brief 段管理系统头文件
 * @author WeiXin.Li
 * @version 1.0
 * @date 2025-07-09
 * @copyright Copyright (c) 2025
 *
 * @details
 * 本文件定义了基于段的自动注册管理系统，提供以下功能：
 * - 任务管理：定时任务的自动注册和管理
 * - 初始化管理：初始化函数的自动注册
 * - 中断管理：中断处理函数的优先级管理
 * - Shell命令：交互式命令行系统
 * - 状态机：FSM状态机框架
 * - 链路管理：数据链路处理框架
 *
 * @note 使用GNU编译器的section特性实现自动注册
 */

#ifndef __SECTION_H_
#define __SECTION_H_

#include "stdint.h"
#include "stddef.h"
#include "stdio.h"

/**
 * @brief 注册类型枚举
 * @note 定义了所有支持的自动注册类型
 */
typedef enum
{
    SECTION_INIT,      ///< 初始化函数注册
    SECTION_TASK,      ///< 定时任务注册
    SECTION_INTERRUPT, ///< 中断处理函数注册
    SECTION_SHELL,     ///< Shell命令注册
    SECTION_LINK,      ///< 链路处理注册
    SECTION_PERF,      ///< 代码运行时间统计注册
    SECTION_COMM,      ///< 通信
} SECTION_E;

/**
 * @brief 段注册公共结构
 * @note 所有注册项的统一格式，用于段遍历
 */
typedef struct
{
    uint32_t section_type; ///< 注册类型，来自SECTION_E枚举
    void *p_str;           ///< 指向具体注册结构的指针
} reg_section_t;

/**
 * @brief 自动注册到段的属性宏
 * @note 根据平台选择合适的section属性
 */
#ifdef IS_PLECS
#define AUTO_REG_SECTION __attribute__((__section__("section")))
#else
#define AUTO_REG_SECTION __attribute__((used, __section__("section")))
#endif

/*============================================================================
 * 任务管理相关定义
 *============================================================================*/

typedef enum
{
    SECTION_PERF_RECORD, ///< 记录代码运行时间统计注册
    SECTION_PERF_BASE,   ///< 基础代码运行时间统计注册
} SECTION_PERF_E;

typedef struct
{
    uint32_t *p_cnt; ///< 计数器一级指针
} section_perf_base_t;

typedef struct
{
    uint32_t perf_type; ///< 性能类型标识
    void *p_perf;       ///< 指向具体性能数据的指针
} section_perf_t;

#define REG_PERF_BASE_CNT(timer_cnt)                                     \
    section_perf_base_t section_perf_base_timer = {                      \
        .p_cnt = (uint32_t *)&(timer_cnt),                               \
    };                                                                   \
    section_perf_t section_timer_cnt_perf = {                            \
        .perf_type = SECTION_PERF_BASE,                                  \
        .p_perf = (void *)&section_perf_base_timer,                      \
    };                                                                   \
    const reg_section_t reg_section_perf_base_timer AUTO_REG_SECTION = { \
        .section_type = SECTION_PERF,                                    \
        .p_str = (void *)&section_timer_cnt_perf,                        \
    };

typedef struct
{
    char *p_name;      /// 计时名称
    uint16_t start;    ///< 计时开始点
    uint16_t end;      ///< 计时结束点
    uint16_t time;     ///< 计时时间（100us单位）
    uint16_t reserved; ///< 保留字段，便于对齐
    uint32_t max_time; ///< 最大计时值
    uint32_t **p_cnt;  ///< 计数器二级指针
    void *p_next;      ///< 链表下一个节点指针
} section_perf_record_t;

#define PERF_START(name) section_perf_record_##name.start = **section_perf_record_##name.p_cnt
#define PERF_END(name)                                                                                   \
    section_perf_record_##name.end = **section_perf_record_##name.p_cnt;                                 \
    section_perf_record_##name.time = section_perf_record_##name.end - section_perf_record_##name.start; \
    if (section_perf_record_##name.time > section_perf_record_##name.max_time)                           \
        section_perf_record_##name.max_time = section_perf_record_##name.time;

#define PERF_RECORD_ENABLE 1 ///< 是否启用性能记录功能

#if (PERF_RECORD_ENABLE == 1)
#define P_RECORD_PERF(name) (section_perf_record_t *)&section_perf_record_##name
#define REG_PERF_RECORD(name)                                               \
    section_perf_record_t section_perf_record_##name = {                    \
        .p_name = #name,                                                    \
        .start = 0,                                                         \
        .end = 0,                                                           \
        .max_time = 0,                                                      \
        .p_cnt = NULL,                                                      \
        .p_next = NULL,                                                     \
    };                                                                      \
    section_perf_t section_perf_record_##name##_perf = {                    \
        .perf_type = SECTION_PERF_RECORD,                                   \
        .p_perf = (void *)&section_perf_record_##name,                      \
    };                                                                      \
    const reg_section_t reg_section_perf_record_##name AUTO_REG_SECTION = { \
        .section_type = SECTION_PERF,                                       \
        .p_str = (void *)&section_perf_record_##name##_perf,                \
    };
#else
#define P_RECORD_PERF(name) NULL
#define REG_PERF_RECORD(name)
#endif

/**
 * @brief 任务注册结构
 * @note 定义了定时任务的所有属性
 */
typedef struct
{
    uint32_t t_period;                    ///< 任务执行周期（100us为单位）
    uint32_t time_last;                   ///< 上次执行时间戳
    void (*p_func)(void);                 ///< 任务函数指针
    section_perf_record_t *p_perf_record; ///< 性能计数器指针（可选）
    void *p_next;                         ///< 链表下一个节点指针
} reg_task_t;

/**
 * @brief 注册定时任务
 * @param period 任务执行周期（100us为单位）
 * @param func 任务函数名
 * @note 使用示例：REG_TASK(100, my_task_func); // 每10ms执行一次
 */
#define REG_TASK(period, func)                                       \
    REG_PERF_RECORD(func)                                            \
    reg_task_t reg_task_##func = {                                   \
        .t_period = period,                                          \
        .p_func = func,                                              \
        .time_last = 0,                                              \
        .p_perf_record = P_RECORD_PERF(func),                        \
        .p_next = NULL,                                              \
    };                                                               \
    const reg_section_t reg_section_task_##func AUTO_REG_SECTION = { \
        .section_type = SECTION_TASK,                                \
        .p_str = (void *)&reg_task_##func,                           \
                                                                     \
    };

/**
 * @brief 注册定时任务（毫秒单位）
 * @param period 任务执行周期（毫秒为单位）
 * @param func 任务函数名
 * @note 使用示例：REG_TASK_MS(10, my_task_func); // 每10ms执行一次
 */
#define REG_TASK_MS(period, func) REG_TASK((period) * 10, func)

/**
 * @brief 运行所有注册的定时任务
 * @note 需要在主循环中定期调用
 */
void run_task(void);

/*============================================================================
 * 初始化管理相关定义
 *============================================================================*/

/**
 * @brief 初始化函数注册结构
 */
typedef struct reg_init
{
    uint8_t priority;
    void (*p_func)(void);    ///< 初始化函数指针
    struct reg_init *p_next; ///< 链表下一个节点指针
} reg_init_t;

/**
 * @brief 注册初始化函数
 * @param func 初始化函数名
 * @note 使用示例：REG_INIT(num, my_init_func);
 * @note 注册的函数会在section_init()中自动调用
 */
#define REG_INIT(prio, func)                                    \
    reg_init_t reg_init_##func = {                              \
        .priority = prio,                                       \
        .p_func = func,                                         \
        .p_next = NULL,                                         \
    };                                                          \
    const reg_section_t reg_section_##func AUTO_REG_SECTION = { \
        .section_type = SECTION_INIT,                           \
        .p_str = (void *)&reg_init_##func};

/**
 * @brief 执行所有注册的初始化函数
 * @note 应在系统启动时调用
 */
void section_init(void);

/*============================================================================
 * 中断管理相关定义
 *============================================================================*/

#define PRIORITY_NUM_MAX 16 ///< 最大优先级数量

/**
 * @brief 中断处理函数注册结构
 */
typedef struct reg_interrupt
{
    uint8_t priority;             ///< 中断优先级（数值越小优先级越高）
    void (*p_func)(void);         ///< 中断处理函数指针
    struct reg_interrupt *p_next; ///< 链表下一个节点指针
} reg_interrupt_t;

/**
 * @brief 注册中断处理函数
 * @param priority_num 优先级（0-15，数值越小优先级越高）
 * @param func 中断处理函数名
 * @note 使用示例：REG_INTERRUPT(1, my_irq_handler);
 */
#define REG_INTERRUPT(priority_num, func)                                 \
    reg_interrupt_t reg_interrupt_##func = {                              \
        .priority = priority_num,                                         \
        .p_func = func,                                                   \
        .p_next = NULL,                                                   \
    };                                                                    \
    const reg_section_t reg_section_interrupt_##func AUTO_REG_SECTION = { \
        .section_type = SECTION_INTERRUPT,                                \
        .p_str = (void *)&reg_interrupt_##func};

/**
 * @brief 按优先级执行所有注册的中断处理函数
 * @note 应在中断服务程序中调用
 */
void section_interrupt(void);

/*============================================================================
 * 状态机相关定义
 *============================================================================*/

/**
 * @brief 状态机函数表项结构
 */
typedef struct
{
    uint32_t fsm_sta;               ///< 状态值
    void (*func_in)(void);          ///< 状态入口函数
    void (*func_exe)(void);         ///< 状态执行函数
    uint32_t (*func_chk)(uint32_t); ///< 状态检查函数，返回下一状态
    void (*func_out)(void);         ///< 状态出口函数
} reg_fsm_func_t;

/**
 * @brief 状态机控制结构
 */
typedef struct
{
    uint32_t fsm_sta;                 ///< 当前状态
    reg_fsm_func_t *p_fsm_func_table; ///< 状态函数表指针
    uint32_t fsm_table_size;          ///< 状态函数表大小
    uint8_t fsm_sta_is_change;        ///< 状态改变标志
    uint32_t *p_fsm_ev;               ///< 事件指针
} reg_fsm_t;

/**
 * @brief 状态机表项定义宏
 * @param sta 状态值
 * @param in 入口函数
 * @param exe 执行函数
 * @param chk 检查函数
 * @param out 出口函数
 * @note 使用示例：FSM_ENTRY(STATE_IDLE, idle_in, idle_exe, idle_chk, idle_out)
 */
#define FSM_ENTRY(sta, in, exe, chk, out) \
    {                                     \
        .fsm_sta = sta,                   \
        .func_in = in,                    \
        .func_exe = exe,                  \
        .func_chk = chk,                  \
        .func_out = out,                  \
    }

/**
 * @brief 注册状态机
 * @param name 状态机名称
 * @param init_sta 初始状态
 * @param fsm_ev 事件变量
 * @param ... 状态表项列表（使用FSM_ENTRY宏定义）
 * @note 使用示例：
 * @code
 * uint32_t my_event = 0;
 * REG_FSM(my_fsm, STATE_INIT, my_event,
 *     FSM_ENTRY(STATE_INIT, init_in, init_exe, init_chk, init_out),
 *     FSM_ENTRY(STATE_RUN, run_in, run_exe, run_chk, run_out)
 * );
 * @endcode
 */
#define REG_FSM(name, init_sta, fsm_ev, ...)                                            \
    static reg_fsm_func_t reg_fsm_func_##name##_table[] = {__VA_ARGS__};                \
    static reg_fsm_t reg_fsm_##name = {                                                 \
        .fsm_sta = init_sta,                                                            \
        .p_fsm_func_table = reg_fsm_func_##name##_table,                                \
        .fsm_table_size = sizeof(reg_fsm_func_##name##_table) / sizeof(reg_fsm_func_t), \
        .fsm_sta_is_change = 1,                                                         \
        .p_fsm_ev = &fsm_ev,                                                            \
    };                                                                                  \
                                                                                        \
    static void fsm_##name##_run(void)                                                  \
    {                                                                                   \
        section_fsm_func(&reg_fsm_##name);                                              \
    }                                                                                   \
    REG_TASK(1, fsm_##name##_run)

/**
 * @brief 获取状态机当前状态
 * @param name 状态机名称
 * @return 当前状态值
 */
#define FSM_GET_STATE(name) reg_fsm_##name.fsm_sta

/**
 * @brief 状态机运行函数
 * @param str 状态机控制结构指针
 */
void section_fsm_func(reg_fsm_t *str);

/*============================================================================
 * Shell命令系统相关定义
 *============================================================================*/

/**
 * @brief Printf函数声明宏
 * @note 用于统一Shell系统中的打印函数接口
 */
#define DEC_MY_PRINTF void (*my_printf)(const char *__format, ...)

/**
 * @brief Shell命令处理函数
 * @param data 接收到的字符
 * @param my_printf 打印函数指针
 * @note 处理交互式命令行输入
 */
void shell_run(uint8_t data, DEC_MY_PRINTF);

/**
 * @brief Shell变量类型枚举
 */
typedef enum
{
    SHELL_INT8,   ///< 8位有符号整数
    SHELL_UINT8,  ///< 8位无符号整数
    SHELL_INT16,  ///< 16位有符号整数
    SHELL_UINT16, ///< 16位无符号整数
    SHELL_INT32,  ///< 32位有符号整数
    SHELL_UINT32, ///< 32位无符号整数
    SHELL_FP32,   ///< 32位浮点数
    SHELL_CMD,    ///< 命令（无变量）
} SHELL_TYPE_E;

/**
 * @brief Shell命令/变量注册结构
 */
typedef struct section_shell_t
{
    const char *p_name;             ///< 命令/变量名称
    uint32_t p_name_size;           ///< 名称长度
    void *p_var;                    ///< 变量指针（命令为NULL）
    uint32_t type;                  ///< 类型（来自SHELL_TYPE_E）
    void (*func)(DEC_MY_PRINTF);    ///< 回调函数（可选）
    uint32_t status;                ///< 状态（可选）
    DEC_MY_PRINTF;                  ///< 打印函数指针
    struct section_shell_t *p_next; ///< 链表下一个节点指针
} section_shell_t;

/**
 * @brief 注册Shell变量
 * @param _name 变量名称
 * @param _var 变量
 * @param _type 变量类型（来自SHELL_TYPE_E）
 * @param _func 回调函数（可为NULL）
 * @note 使用示例：
 * @code
 * int32_t my_var = 100;
 * REG_SHELL_VAR(my_var, my_var, SHELL_INT32, NULL);
 * @endcode
 */
#define REG_SHELL_VAR(_name, _var, _type, _func)                       \
    section_shell_t section_shell_##_name = {                          \
        .p_name = #_name,                                              \
        .p_name_size = (sizeof(#_name) - 1),                           \
        .p_var = (void *)&_var,                                        \
        .type = _type,                                                 \
        .func = _func,                                                 \
        .p_next = NULL,                                                \
    };                                                                 \
    const reg_section_t reg_section_shell_##_name AUTO_REG_SECTION = { \
        .section_type = SECTION_SHELL,                                 \
        .p_str = (void *)&section_shell_##_name,                       \
    };

/**
 * @brief 注册Shell命令
 * @param _name 命令名称
 * @param _func 命令处理函数
 * @note 使用示例：
 * @code
 * void my_command(DEC_MY_PRINTF) {
 *     my_printf("Hello World!\r\n");
 * }
 * REG_SHELL_CMD(hello, my_command);
 * @endcode
 */
#define REG_SHELL_CMD(_name, _func)                                    \
    section_shell_t section_shell_##_name = {                          \
        .p_name = #_name,                                              \
        .p_name_size = sizeof(#_name) - 1,                             \
        .p_var = NULL,                                                 \
        .type = SHELL_CMD,                                             \
        .func = _func,                                                 \
        .p_next = NULL,                                                \
    };                                                                 \
    const reg_section_t reg_section_shell_##_name AUTO_REG_SECTION = { \
        .section_type = SECTION_SHELL,                                 \
        .p_str = (void *)&section_shell_##_name,                       \
    };

/*============================================================================
 * 链路管理相关定义
 *============================================================================*/

/**
 * @brief 链路处理结构
 * @note 用于管理DMA接收缓冲区和数据处理
 */
typedef struct
{
    uint8_t (*p_buff)[];                       ///< 接收缓冲区指针
    uint32_t buff_size;                        ///< 缓冲区大小
    uint32_t pos;                              ///< 当前处理位置
    uint32_t *dma_cnt;                         ///< DMA计数器指针
    DEC_MY_PRINTF;                             ///< 打印函数指针
    void(*p_next);                             ///< 链表下一个节点指针
    void (**func_arr)(uint8_t, DEC_MY_PRINTF); ///< 数据处理函数数组
    uint32_t func_num;                         ///< 处理函数数量
} section_link_t;

/**
 * @brief 注册链路处理
 * @param link 链路名称
 * @param size 缓冲区大小
 * @param print 打印函数
 * @param _dma_cnt DMA计数器指针
 * @param _func_arr 处理函数数组
 * @param _func_num 处理函数数量
 * @note 使用示例：
 * @code
 * void uart_handler(char data, DEC_MY_PRINTF) {
 *     // 处理接收到的数据
 * }
 * void (*uart_handlers[])(char, DEC_MY_PRINTF) = {uart_handler};
 * extern uint32_t uart_dma_cnt;
 * REG_LINK(uart, 256, printf, &uart_dma_cnt, uart_handlers, 1);
 * @endcode
 */
#define REG_LINK(link, size, print, _dma_cnt, _func_arr, _func_num)  \
    uint8_t link##_rx_buff[size];                                    \
    section_link_t section_link_##link = {                           \
        .p_buff = &link##_rx_buff,                                   \
        .buff_size = size,                                           \
        .pos = 0,                                                    \
        .dma_cnt = _dma_cnt,                                         \
        .my_printf = print,                                          \
        .p_next = NULL,                                              \
        .func_arr = _func_arr,                                       \
        .func_num = _func_num,                                       \
    };                                                               \
    const reg_section_t reg_section_link_##link AUTO_REG_SECTION = { \
        .section_type = SECTION_LINK,                                \
        .p_str = (void *)&section_link_##link,                       \
    };

/**
 * @brief 获取链路缓冲区
 * @param link 链路名称
 * @return 缓冲区数组名
 */
#define GET_LINK_BUFF(link) link##_rx_buff

/**
 * @brief 获取链路缓冲区大小
 * @param link 链路名称
 * @return 缓冲区大小
 */
#define GET_LINK_SIZE(link) section_link_##link.buff_size

typedef enum
{
    SECTION_PACKFORM_STA_SOP,
    SECTION_PACKFORM_STA_CMD,
    SECTION_PACKFORM_STA_LEN,
    SECTION_PACKFORM_STA_DATA,
    SECTION_PACKFORM_STA_CRC,
    SECTION_PACKFORM_STA_EOP,
} SECTION_PACKFORM_STA_E;

typedef struct
{
    uint8_t sop;     ///< 起始符 固定0xE8
    uint8_t cmd;     ///< 命令字
    uint16_t len;    ///< 数据长度
    uint8_t *p_data; ///< 数据指针
    uint16_t sum;    ///< 校验和
    uint16_t eop;    ///< 结束符 固定0x0A0D
} section_packform_t;

typedef struct
{
    uint8_t data_buffer[512]; ///< 通信缓冲区
    uint8_t index;            ///< 缓冲区索引
    uint8_t status;           ///< 通信状态
    uint16_t sum;             ///< 校验和
    section_packform_t pack;  ///< 当前包指针
    void (*func)(section_packform_t *p_pack, DEC_MY_PRINTF);
    uint16_t len;
    uint8_t len_flag; ///< 长度标志位
    uint8_t eop_flag; ///< 结束符标志位
} comm_ctx_t;

typedef struct section_com_t
{
    uint8_t cmd;
    void (*func)(section_packform_t *p_pack, DEC_MY_PRINTF);
    struct section_com_t *p_next; ///< 链表下一个节点指针
} section_com_t;

#define REG_COMM(_cmd, _func)                                       \
    section_com_t section_com_##_cmd = {                            \
        .cmd = _cmd,                                                \
        .func = _func,                                              \
        .p_next = NULL,                                             \
    };                                                              \
    const reg_section_t reg_section_com_##_cmd AUTO_REG_SECTION = { \
        .section_type = SECTION_COMM,                               \
        .p_str = (void *)&section_com_##_cmd,                       \
    };

void comm_run(uint8_t data, DEC_MY_PRINTF);

void comm_send_data(section_packform_t *p_pack, DEC_MY_PRINTF);

#endif
