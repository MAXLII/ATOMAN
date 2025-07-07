#ifndef __SECTION_H_
#define __SECTION_H_

#include "stdint.h"
#include "stddef.h"
#include "stdio.h"

// 注册类型
typedef enum
{
    SECTION_INIT,
    SECTION_TASK,
    SECTION_INTERRUPT,
    SECTION_SHELL,
    SECTION_LINK,
} SECTION_E;

// 公共属性
typedef struct
{
    uint32_t section_type; // 注册类型
    void *p_str;           // 属性指针
} reg_section_t;

// 自动注册到段
#ifdef IS_PLECS
#define AUTO_REG_SECTION __attribute__((__section__("section")))
#else
#define AUTO_REG_SECTION __attribute__((used, __section__("section")))
#endif

// 任务注册
typedef struct
{
    uint32_t t_period;
    uint32_t time_last;
    void (*p_func)(void);
    void *p_next;
} reg_task_t;

#define REG_TASK(period, func)                                       \
    reg_task_t reg_task_##func = {                                   \
        .t_period = period,                                          \
        .p_func = func,                                              \
        .time_last = 0,                                              \
        .p_next = NULL,                                              \
    };                                                               \
    const reg_section_t reg_section_task_##func AUTO_REG_SECTION = { \
        .section_type = SECTION_TASK,                                \
        .p_str = (void *)&reg_task_##func,                           \
                                                                     \
    };

#define REG_TASK_MS(period, func) REG_TASK(period * 10, func)

void run_task(void);

// 初始化注册
typedef struct
{
    void (*p_func)(void);
} reg_init_t;

#define REG_INIT(func)                                          \
    reg_init_t reg_init_##func = {                              \
        .p_func = func,                                         \
    };                                                          \
    const reg_section_t reg_section_##func AUTO_REG_SECTION = { \
        .section_type = SECTION_INIT,                           \
        .p_str = (void *)&reg_init_##func};

void section_init(void);

// 中断注册

#define PRIORITY_NUM_MAX 16

typedef struct reg_interrupt
{
    uint8_t priority;
    void (*p_func)(void);
    struct reg_interrupt *p_next; // 添加这一行
} reg_interrupt_t;

#define REG_INTERRUPT(priority_num, func)                                 \
    reg_interrupt_t reg_interrupt_##func = {                              \
        .priority = priority_num,                                         \
        .p_func = func,                                                   \
        .p_next = NULL,                                                   \
    };                                                                    \
    const reg_section_t reg_section_interrupt_##func AUTO_REG_SECTION = { \
        .section_type = SECTION_INTERRUPT,                                \
        .p_str = (void *)&reg_interrupt_##func};

void section_interrupt(void);

typedef struct
{
    uint32_t fsm_sta; // 状态机状态
    void (*func_in)(void);
    void (*func_exe)(void);
    uint32_t (*func_chk)(uint32_t);
    void (*func_out)(void);
} reg_fsm_func_t;

typedef struct
{
    uint32_t fsm_sta;
    reg_fsm_func_t *p_fsm_func_table;
    uint32_t fsm_table_size;
    uint8_t fsm_sta_is_change;
    uint32_t *p_fsm_ev;
} reg_fsm_t;

// 宏定义，用于定义状态表项
#define FSM_ENTRY(sta, in, exe, chk, out) \
    {                                     \
        .fsm_sta = sta,                   \
        .func_in = in,                    \
        .func_exe = exe,                  \
        .func_chk = chk,                  \
        .func_out = out,                  \
    }

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

#define FSM_GET_STATE(name) reg_fsm_##name.fsm_sta

void section_fsm_func(reg_fsm_t *str);

void shell_run(char data, void (*my_printf)(const char *__format, ...));

typedef enum
{
    SHELL_INT8,
    SHELL_UINT8,
    SHELL_INT16,
    SHELL_UINT16,
    SHELL_INT32,
    SHELL_UINT32,
    SHELL_FP32,
    SHELL_CMD,
} SHELL_TYPE_E;

typedef struct section_shell_t
{
    const char *p_name;
    uint32_t p_name_size;
    void *p_var;
    uint32_t type;
    void (*func)(void);
    struct section_shell_t *p_next; // 可修改的指针
} section_shell_t;

#define REG_SHELL_VAR(_name, _var, _type, _func)                       \
    section_shell_t section_shell_##_name = {                    \
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

#define REG_SHELL_CMD(_name, _func)                                    \
    section_shell_t section_shell_##_name = {                    \
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

typedef struct
{
    uint8_t (*p_buff)[];
    uint32_t buff_size;
    uint32_t pos;
    uint32_t *dma_cnt;
    void (*my_printf)(const char *__format, ...);
    void(*p_next);
    void (**func_arr)(char,
                      void (*my_printf)(const char *__format, ...)); // 新增：函数指针数组
    uint32_t func_num;                                               // 新增：函数数量
} section_link_t;

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

#define GET_LINK_BUFF(link) link##_rx_buff
#define GET_LINK_SIZE(link) section_link_##link.buff_size

#endif
