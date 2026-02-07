/**
 * @file section.c
 * @brief 段管理系统实现文件（框架层）
 */

#include "section.h"
#include "platform.h"

#include <stddef.h>
#include <string.h>

// ------------------------ 全局链表头（保留在 section.c） ------------------------
reg_task_t *p_task_first = NULL;
reg_interrupt_t *p_interrupt_first = NULL;
section_shell_t *p_shell_first = NULL;
section_link_t *p_link_first = NULL;
section_perf_record_t *p_perf_record_first = NULL;
uint32_t *p_perf_cnt = NULL;
section_com_t *p_com_first = NULL;
reg_init_t *p_init_first = NULL;
comm_route_t *p_comm_route_first = NULL;

// ------------------------ insert函数（从原 section.c 保留） ------------------------
static void task_insert(reg_task_t *task)
{
    task->time_last = SECTION_SYS_TICK;
    task->p_next = NULL;

    if (p_task_first == NULL)
    {
        p_task_first = task;
    }
    else
    {
        reg_task_t *curr = p_task_first;
        while (curr->p_next)
            curr = curr->p_next;
        curr->p_next = task;
    }
}
static void interrupt_insert(reg_interrupt_t *intr)
{
    if (!intr)
        return;

    intr->p_next = NULL;

    // 插入到链表头或按优先级顺序插入
    if (!p_interrupt_first || intr->priority < p_interrupt_first->priority)
    {
        intr->p_next = p_interrupt_first;
        p_interrupt_first = intr;
    }
    else
    {
        reg_interrupt_t *prev = p_interrupt_first;
        while (prev->p_next && prev->p_next->priority < intr->priority)
        {
            prev = prev->p_next;
        }
        intr->p_next = prev->p_next;
        prev->p_next = intr;
    }
}

uint32_t shell_data_num = 0;

static void shell_insert(section_shell_t *shell)
{
    if (!shell)
        return;

    // 防止重复插入
    for (section_shell_t *p = p_shell_first; p; p = p->p_next)
    {
        if (p == shell)
            return;
    }

    shell->p_next = NULL;
    if (p_shell_first)
    {
        shell->p_next = p_shell_first;
        p_shell_first = shell;
        shell_data_num++;
    }
    else
    {
        p_shell_first = shell;
        shell_data_num++;
    }
}
static void link_insert(section_link_t *link)
{
    link->p_next = NULL;

    if (!p_link_first)
        p_link_first = link;
    else
    {
        section_link_t *curr = p_link_first;
        while (curr->p_next)
            curr = (section_link_t *)curr->p_next;
        curr->p_next = link;
    }
}
static void perf_insert(section_perf_t *perf)
{
    if (!perf)
        return;
    section_perf_base_t *base;
    section_perf_record_t *rec;
    switch (perf->perf_type)
    {
    case SECTION_PERF_BASE:
        // 性能计数器基地址注册
        base = (section_perf_base_t *)perf->p_perf;
        if (base && base->p_cnt)
            p_perf_cnt = base->p_cnt;
        break;
    case SECTION_PERF_RECORD:
        // 性能记录注册，链表管理
        rec = (section_perf_record_t *)perf->p_perf;
        if (rec)
        {
            rec->p_cnt = &p_perf_cnt; // 赋值为一级指针的地址
            rec->p_next = NULL;
            if (!p_perf_record_first)
                p_perf_record_first = rec;
            else
            {
                section_perf_record_t *cur = p_perf_record_first;
                while (cur->p_next)
                    cur = (section_perf_record_t *)cur->p_next;
                cur->p_next = rec;
            }
        }
        break;
    default:
        break;
    }
}
static void comm_insert(section_com_t *com)
{
    if (!com)
        return;
    com->p_next = NULL;
    if (!p_com_first)
        p_com_first = com;
    else
    {
        section_com_t *curr = p_com_first;
        while (curr->p_next)
            curr = (section_com_t *)curr->p_next;
        curr->p_next = com;
    }
}
static void comm_route_insert(comm_route_t *com)
{
    if (!com)
        return;

    com->p_next = NULL;
    if (!p_comm_route_first)
    {
        p_comm_route_first = com;
    }
    else
    {
        comm_route_t *curr = p_comm_route_first;
        while (curr->p_next)
            curr = curr->p_next;
        curr->p_next = com;
    }
}
static void init_insert(reg_init_t *init)
{
    if (!init)
        return;

    init->p_next = NULL;

    // 插入到链表头或按优先级顺序插入
    if (!p_init_first || init->priority < p_init_first->priority)
    {
        init->p_next = p_init_first;
        p_init_first = init;
    }
    else
    {
        reg_init_t *prev = p_init_first;
        while (prev->p_next && prev->p_next->priority <= init->priority)
        {
            prev = prev->p_next;
        }
        init->p_next = prev->p_next;
        prev->p_next = init;
    }
}

// ------------------------ 段扫描初始化（从原 section.c 保留） ------------------------
void section_init(void)
{
    for (reg_section_t *p = (reg_section_t *)&SECTION_START;
         p < (reg_section_t *)&SECTION_STOP;
         ++p)
    {
        switch (p->section_type)
        {
        case SECTION_INIT:
            init_insert((reg_init_t *)p->p_str);
            break;
        case SECTION_TASK:
            task_insert((reg_task_t *)p->p_str);
            break;
        case SECTION_INTERRUPT:
            interrupt_insert((reg_interrupt_t *)p->p_str);
            break;
        case SECTION_SHELL:
            shell_insert((section_shell_t *)p->p_str);
            break;
        case SECTION_LINK:
            link_insert((section_link_t *)p->p_str);
            break;
        case SECTION_PERF:
            perf_insert((section_perf_t *)p->p_str);
            break;
        case SECTION_COMM:
            comm_insert((section_com_t *)p->p_str);
            break;
        case SECTION_COMM_ROUTE:
            comm_route_insert((comm_route_t *)p->p_str);
            break;
        default:
            break;
        }
    }

    for (reg_init_t *init = p_init_first; init != NULL; init = (reg_init_t *)init->p_next)
    {
        if (init->p_func)
            init->p_func();
    }
}

// ------------------------ Task系统（从原 section.c 保留） ------------------------
static float task_run_time = 0.0f;

void run_task(void)
{
    uint32_t now = SECTION_SYS_TICK;

    // 遍历任务链表，检查是否到期
    for (reg_task_t *t = p_task_first; t != NULL; t = (reg_task_t *)t->p_next)
    {
        if (now - t->time_last >= t->t_period)
        {
            if ((t->p_perf_record == NULL) ||
                (*t->p_perf_record->p_cnt == NULL)) // 如果没有性能记录或计数器
            {
                t->p_func(); // 执行任务函数
            }
            else
            {
                t->p_perf_record->start = **t->p_perf_record->p_cnt;                      // 记录开始时间
                t->p_func();                                                              // 执行任务函数
                t->p_perf_record->end = **t->p_perf_record->p_cnt;                        // 记录结束时间
                t->p_perf_record->time = t->p_perf_record->end - t->p_perf_record->start; // 计算执行时间
                if (t->p_perf_record->time > t->p_perf_record->max_time)
                    t->p_perf_record->max_time = t->p_perf_record->time; // 更新最大时间
                task_run_time += t->p_perf_record->time;
            }
            if (likely((now - t->time_last) < (t->t_period << 2)))
            {
                t->time_last += t->t_period; // 更新最后执行时间
            }
            else
            {
                t->time_last = now; // 要是上次时间与当前时间间隔太大直接更新为当前时间
            }
        }
    }
}

float task_metric = 0.0f;
float task_metric_max = 0.0f;

static void task_metric_calculate(void)
{
    task_metric = task_run_time * 500e-6f;
    if (task_metric > task_metric_max)
    {
        task_metric_max = task_metric;
    }
    task_run_time = 0.0f;
}

REG_TASK_MS(100, task_metric_calculate)

// ------------------------ 中断回调（从原 section.c 保留） ------------------------
void section_interrupt(void)
{
    for (reg_interrupt_t *p = p_interrupt_first; p != NULL; p = (reg_interrupt_t *)p->p_next)
    {
        p->p_func();
    }
}

// ------------------------ Link字节分发（从原 section.c 保留） ------------------------
static void link_process(section_link_t *link)
{
    if (!link || !link->dma_cnt || !link->handler_arr)
        return;

    uint32_t buff_size = link->buff_size;
    uint32_t dma_cnt = *link->dma_cnt;
    uint32_t cnt = buff_size - dma_cnt;

    // 处理缓冲区中的新数据
    while (link->pos != cnt)
    {
        uint8_t data = link->rx_buff[link->pos];

        // 调用所有注册的处理函数
        for (uint32_t i = 0; i < link->handler_num; ++i)
        {
            const section_link_handler_item_t *it = &link->handler_arr[i];
            if (it->func)
            {
                // 如果 ctx 是 comm_ctx_t，并且你想在路由里用 link_id，可选做一个约定：
                // 约定：当 handler 是 comm_run 时，ctx 指向 comm_ctx_t
                // 那么可以：
                // ((comm_ctx_t*)it->ctx)->link_id = link->link_id;

                it->func(data, link->my_printf, it->ctx);
            }
        }

        // 更新位置指针（环形缓冲区）
        link->pos = (link->pos + 1) % buff_size;
    }
}

void section_link_task(void)
{
    for (section_link_t *p = p_link_first; p != NULL; p = (section_link_t *)p->p_next)
    {
        link_process(p);
    }
}

// 保留你原来的任务注册
REG_TASK(10, section_link_task)

// ------------------------ FSM（若你认为属于框架，可保留） ------------------------
void section_fsm_func(reg_fsm_t *fsm)
{
    if (!fsm->p_fsm_func_table || !fsm->p_fsm_ev)
        return;

    // 查找当前状态对应的处理函数
    for (uint32_t i = 0; i < fsm->fsm_table_size; ++i)
    {
        reg_fsm_func_t *entry = &fsm->p_fsm_func_table[i];
        if (fsm->fsm_sta == entry->fsm_sta)
        {
            // 状态刚切换时执行入口函数
            if (fsm->fsm_sta_is_change)
            {
                fsm->fsm_sta_is_change = 0;
                PLECS_LOG("%s\n", entry->p_name);
                entry->func_in();
            }

            // 执行状态处理函数
            entry->func_exe();

            // 检查是否有事件需要处理
            if (*fsm->p_fsm_ev)
            {
                uint32_t next = entry->func_chk(*fsm->p_fsm_ev);

                // 状态切换
                if (next && next != entry->fsm_sta)
                {
                    PLECS_LOG("%s-chk_ev:%d\n", entry->p_name, *fsm->p_fsm_ev);
                    entry->func_out();          // 执行出口函数
                    fsm->fsm_sta = next;        // 切换状态
                    fsm->fsm_sta_is_change = 1; // 标记状态已改变
                }
                *fsm->p_fsm_ev = 0;
            }
            break;
        }
    }
}
