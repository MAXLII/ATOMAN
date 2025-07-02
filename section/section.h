#ifndef __SECTION_H_
#define __SECTION_H_

#include "stdint.h"
#include "stddef.h"

// 注册类型
typedef enum
{
    SECTION_INIT,
    SECTION_TASK,
    SECTION_INTERRUPT,
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

#define REG_TASK(period, func)                                 \
    reg_task_t reg_task_##func = {                             \
        .t_period = period,                                    \
        .p_func = func,                                        \
        .time_last = 0,                                        \
        .p_next = NULL,                                        \
    };                                                         \
    reg_section_t reg_section_task_##func AUTO_REG_SECTION = { \
        .section_type = SECTION_TASK,                          \
        .p_str = (void *)&reg_task_##func,                     \
                                                               \
    };
void run_task(void);

// 初始化注册
typedef struct
{
    void (*p_func)(void);
} reg_init_t;

#define REG_INIT(func)                                    \
    reg_init_t reg_init_##func = {                        \
        .p_func = func,                                   \
    };                                                    \
    reg_section_t reg_section_##func AUTO_REG_SECTION = { \
        .section_type = SECTION_INIT,                     \
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

#define REG_INTERRUPT(priority_num, func)                           \
    reg_interrupt_t reg_interrupt_##func = {                        \
        .priority = priority_num,                                   \
        .p_func = func,                                             \
        .p_next = NULL,                                             \
    };                                                              \
    reg_section_t reg_section_interrupt_##func AUTO_REG_SECTION = { \
        .section_type = SECTION_INTERRUPT,                          \
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

#endif
