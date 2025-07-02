#include "section.h"
#include "stddef.h"

#ifdef IS_PLECS

#include "plecs.h"
extern uint32_t plecs_time_100us;
#define SECTION_SYS_TICK plecs_time_100us

extern size_t __start_section;
extern size_t __stop_section;

#define SECTION_START __start_section
#define SECTION_STOP __stop_section

#else

#include "systick.h"

#define SECTION_SYS_TICK systick_gettime_100us()

/* 使用链接脚本中定义的符号 */
extern uint32_t __section_start;
extern uint32_t __section_end;

#define SECTION_START __section_start
#define SECTION_STOP __section_end

#endif

reg_task_t *p_task_first = NULL;
reg_interrupt_t *p_interrupt_first = NULL;

void run_task(void)
{
    uint32_t sys_tisk = SECTION_SYS_TICK; // 获取当前系统时间
    reg_task_t *p_task = p_task_first;    // 获取任务链表的第一个任务

    if (p_task == NULL)
    {
        return; // 如果没有任务，直接返回
    }

    while (p_task != NULL)
    { // 遍历链表，直到链表末尾
        // 检查任务是否到达执行时间
        if (sys_tisk - p_task->time_last >= p_task->t_period)
        {
            p_task->p_func();             // 执行任务
            p_task->time_last = sys_tisk; // 更新任务的上次执行时间
        }

        p_task = (reg_task_t *)p_task->p_next; // 移动到下一个任务
    }
}

void section_init(void)
{
    reg_section_t *p_section = (reg_section_t *)&SECTION_START; // 获取段的起始地址
    reg_task_t *p_task_last = NULL;                             // 上一个任务的指针
    reg_task_t *p_task_now = NULL;                              // 当前任务的指针
    reg_interrupt_t *p_interrupt_now = NULL;                    // 当前中断的指针

    for (; p_section < (reg_section_t *)&SECTION_STOP; p_section++)
    {
        if (p_section->section_type == SECTION_INIT)
        {
            // 如果是初始化段，调用初始化函数
            reg_init_t *p_init = (reg_init_t *)p_section->p_str;
            p_init->p_func();
        }
        else if (p_section->section_type == SECTION_TASK)
        {
            // 如果是任务段，将任务添加到链表中
            p_task_now = (reg_task_t *)p_section->p_str;
            p_task_now->time_last = SECTION_SYS_TICK; // 初始化任务的上次执行时间
            p_task_now->p_next = NULL;                // 新任务的 p_next 初始化为 NULL

            if (p_task_last != NULL)
            {
                p_task_last->p_next = p_task_now; // 将上一个任务的 p_next 指向当前任务
            }
            else
            {
                p_task_first = p_task_now; // 如果是第一个任务，设置为链表头
            }
            p_task_last = p_task_now; // 更新上一个任务的指针
        }
        else if (p_section->section_type == SECTION_INTERRUPT)
        {
            // 如果是中断段，按优先级插入链表
            p_interrupt_now = (reg_interrupt_t *)p_section->p_str;
            p_interrupt_now->p_next = NULL;

            if (p_interrupt_first == NULL || p_interrupt_now->priority < p_interrupt_first->priority)
            {
                p_interrupt_now->p_next = p_interrupt_first;
                p_interrupt_first = p_interrupt_now;
            }
            else
            {
                reg_interrupt_t *p_prev = p_interrupt_first;
                while (p_prev->p_next != NULL && p_prev->p_next->priority <= p_interrupt_now->priority)
                {
                    p_prev = p_prev->p_next;
                }
                p_interrupt_now->p_next = p_prev->p_next;
                p_prev->p_next = p_interrupt_now;
            }
        }
    }
}

void section_interrupt(void)
{
    reg_interrupt_t *p_interrupt = p_interrupt_first;
    while (p_interrupt != NULL)
    {
        p_interrupt->p_func();
        p_interrupt = (reg_interrupt_t *)p_interrupt->p_next;
    }
}

void section_fsm_func(reg_fsm_t *str)
{
    if ((str->p_fsm_func_table == NULL) ||
        (str->p_fsm_ev == NULL))
    {
        return; // Exit the function if the table pointer or fsm_event is NULL
    }
    for (uint32_t i = 0; i < str->fsm_table_size; i++)
    {
        reg_fsm_func_t *p_func = &str->p_fsm_func_table[i];
        if (str->fsm_sta == p_func->fsm_sta)
        {
            if (str->fsm_sta_is_change != 0)
            {
                str->fsm_sta_is_change = 0; // 状态机状态改变标志清零
                p_func->func_in();          // 进入状态函数
            }
            p_func->func_exe(); // 执行状态函数
            if (*str->p_fsm_ev != 0)
            {
                uint32_t next_state = p_func->func_chk(*str->p_fsm_ev); // 检查状态函数
                *str->p_fsm_ev = 0;
                if ((next_state != p_func->fsm_sta) &&
                    (next_state != 0))
                {
                    p_func->func_out();         // 退出状态函数
                    str->fsm_sta = next_state;  // 更新状态机状态
                    str->fsm_sta_is_change = 1; // 状态机状态改变标志置位
                }
            }
            break;
        }
    }
}
