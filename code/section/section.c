/**
 * @file section.c
 * @brief 自动注册段（section）管理框架实现
 *
 * 本文件是整个“段式注册框架”的运行时核心实现，负责：
 *  - 扫描链接器 section，构建各类运行时链表
 *  - 驱动 task / interrupt / link / shell / comm / perf / FSM 等子系统
 *
 * 设计定位：
 *  - 这是“框架层（framework layer）”，不包含任何具体业务逻辑
 *  - 所有具体功能均通过 section 自动注册机制接入
 *
 * 重要约定：
 *  - 本文件中所有 insert / process 函数均只操作链表和调度
 *  - 不负责协议、不负责业务、不负责硬件初始化
 */

#include "section.h"

#include <stddef.h>
#include <string.h>

/* =============================================================================
 * 全局链表头（框架私有）
 *
 * 这些指针是 section 自动注册机制的“运行时锚点”，
 * 在 section_init() 中由段扫描阶段统一构建。
 *
 * 生命周期：
 *  - 启动阶段初始化
 *  - 运行期只追加、不删除
 * =============================================================================
 */
reg_task_t *p_task_first = NULL;
reg_interrupt_t *p_interrupt_first = NULL;
section_link_t *p_link_first = NULL;
section_perf_record_t *p_perf_record_first = NULL;
uint32_t *p_perf_cnt = NULL;
reg_init_t *p_init_first = NULL;

/* =============================================================================
 * 各类注册对象的插入函数
 *
 * 职责：
 *  - 将“编译期注册对象”转换为“运行期链表”
 *  - 维持必要的顺序（如 priority）
 *
 * 注意：
 *  - 这些函数只在 section_init() 中被调用
 *  - 不考虑并发、不考虑删除
 * =============================================================================
 */

/* ------------------------ Task 插入 ------------------------ */
static void task_insert(reg_task_t *task)
{
    if (!task)
        return;

    static reg_task_t *s_task_tail = NULL;

    task->time_last = SECTION_SYS_TICK;
    task->p_next = NULL;

    if (p_task_first == NULL)
    {
        p_task_first = task;
        s_task_tail = task;
    }
    else
    {
        /* 只追加，不删除：用 tail 保证 O(1) */
        s_task_tail->p_next = task;
        s_task_tail = task;
    }
}

/* ------------------------ Interrupt 插入（按优先级） ------------------------ */
static void interrupt_insert(reg_interrupt_t *intr)
{
    if (!intr)
        return;

    intr->p_next = NULL;

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

/* ------------------------ Link 插入 ------------------------ */
static void link_insert(section_link_t *link)
{
    if (!link)
        return;

    static section_link_t *s_link_tail = NULL;

    link->p_next = NULL;

    if (!p_link_first)
    {
        p_link_first = link;
        s_link_tail = link;
    }
    else
    {
        s_link_tail->p_next = link;
        s_link_tail = link;
    }
}

/* ------------------------ Performance 插入 ------------------------ */
static void perf_insert(section_perf_t *perf)
{
    if (!perf)
        return;

    section_perf_base_t *base;
    section_perf_record_t *rec;

    static section_perf_record_t *s_perf_record_tail = NULL;

    switch (perf->perf_type)
    {
    case SECTION_PERF_BASE:
        /* 注册系统级性能计数器（如定时器） */
        base = (section_perf_base_t *)perf->p_perf;
        if (base && base->p_cnt)
            p_perf_cnt = base->p_cnt;
        break;

    case SECTION_PERF_RECORD:
        rec = (section_perf_record_t *)perf->p_perf;
        if (rec)
        {
            rec->p_cnt = &p_perf_cnt;
            rec->p_next = NULL;

            if (!p_perf_record_first)
            {
                p_perf_record_first = rec;
                s_perf_record_tail = rec;
            }
            else
            {
                s_perf_record_tail->p_next = rec;
                s_perf_record_tail = rec;
            }
        }
        break;

    default:
        break;
    }
}

/* ------------------------ Init 插入（按优先级） ------------------------ */
static void init_insert(reg_init_t *init)
{
    if (!init)
        return;

    init->p_next = NULL;

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

/* =============================================================================
 * section_init
 *
 * 框架入口函数：
 *  - 扫描链接器 section
 *  - 将所有注册对象分发到对应链表
 *  - 执行所有 INIT 函数
 *
 * 调用时机：
 *  - 系统启动早期
 *  - main() 或 OS 启动前
 * =============================================================================
 */
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
        case SECTION_LINK:
            link_insert((section_link_t *)p->p_str);
            break;
        case SECTION_PERF:
            perf_insert((section_perf_t *)p->p_str);
            break;
        default:
            break;
        }
    }

    /* 执行所有注册的初始化函数 */
    for (reg_init_t *init = p_init_first; init != NULL; init = (reg_init_t *)init->p_next)
    {
        if (init->p_func)
            init->p_func();
    }
}

/* =============================================================================
 * Task 调度系统
 * =============================================================================
 */

float task_run_time = 0.0f;

static inline uint32_t perf_cnt_read(uint32_t *const *pp_cnt)
{
    /* pp_cnt 指向全局计数器指针(p_perf_cnt)；任何一级为 NULL 都认为不可用 */
    if (!pp_cnt || !*pp_cnt)
        return 0u;
    return **pp_cnt;
}

void run_task(void)
{
    const uint32_t now = SECTION_SYS_TICK;

    for (reg_task_t *task = p_task_first; task; task = task->p_next)
    {
        if (!task->p_func)
            continue;

        const uint32_t period = task->t_period;
        if (period == 0u)
            continue;

        /* tick 回绕安全：无符号减法 */
        const uint32_t elapsed = (uint32_t)(now - task->time_last);
        if (elapsed < period)
        {
            continue;
        }

        /* --- perf start --- */
        uint32_t perf_start = 0u;
        section_perf_record_t *rec = task->p_perf_record; /* 如果你是别的字段名，改这里 */
        if (rec)
        {
            perf_start = perf_cnt_read(rec->p_cnt);
        }

        /* --- do task --- */
        task->p_func();

        /* --- perf end + update --- */
        if (rec)
        {
            const uint32_t perf_end = perf_cnt_read(rec->p_cnt);
            const uint32_t delta = (uint32_t)(perf_end - perf_start);

            /* 你原来如果是 16bit 存储，这里会截断；建议用 32bit */
            rec->time = delta;
            rec->max_time = (delta > rec->max_time) ? delta : rec->max_time;
            task_run_time += delta;
        }

        /* --- update scheduling --- */
        if (0)
        {
            /* 不追赶：避免 CPU 忙时多次连跑 */
            task->time_last = now;
        }
        else
        {
            /* 追赶/对齐：把 time_last 推进到“最近一次理论触发点” */
            const uint32_t k = elapsed / period; /* 至少 1 */
            task->time_last += k * period;

            /* 极端情况下（period 很小 + now 跳变），保证不会越界滞后太多 */
            /* 可选：若你希望强制 time_last 不超过 now */
            /* if ((uint32_t)(now - task->time_last) > period) task->time_last = now; */
        }
    }
}

void section_interrupt(void)
{
    for (reg_interrupt_t *p = p_interrupt_first; p != NULL; p = (reg_interrupt_t *)p->p_next)
    {
        p->p_func();
    }
}

/* =============================================================================
 * Link 字节分发系统
 *
 * 职责：
 *  - 从 DMA 环形缓冲中取出新字节
 *  - 按 handler_arr 顺序分发给上层处理器
 *
 * 设计约定：
 *  - link 层只处理“字节流”
 *  - 不理解协议、不缓存 payload
 * =============================================================================
 */
static void link_process(section_link_t *link)
{
    if (!link || !link->dma_cnt || !link->handler_arr)
        return;

    const uint32_t buff_size = link->buff_size;
    const uint32_t cnt = buff_size - *link->dma_cnt; /* DMA NDTR 风格：剩余计数 */

    uint32_t pos = link->pos;
    while (pos != cnt)
    {
        const uint8_t data = link->rx_buff[pos];

        for (uint32_t i = 0; i < link->handler_num; ++i)
        {
            const section_link_handler_item_t *it = &link->handler_arr[i];
            if (it->func)
                it->func(data, link->my_printf, it->ctx);
        }

        ++pos;
        if (pos >= buff_size)
            pos = 0;
    }
    link->pos = pos;
}

static void section_link_task(void)
{
    for (section_link_t *p = p_link_first; p != NULL; p = (section_link_t *)p->p_next)
    {
        link_process(p);
    }
}

/* 周期性调度 link 处理 */
REG_TASK(10, section_link_task)

/* =============================================================================
 * FSM 调度框架
 * =============================================================================
 */
void section_fsm_func(reg_fsm_t *fsm)
{
    if (!fsm->p_fsm_func_table || !fsm->p_fsm_ev)
        return;

    for (uint32_t i = 0; i < fsm->fsm_table_size; ++i)
    {
        reg_fsm_func_t *entry = &fsm->p_fsm_func_table[i];
        if (fsm->fsm_sta == entry->fsm_sta)
        {
            if (fsm->fsm_sta_is_change)
            {
                fsm->fsm_sta_is_change = 0;
                PLECS_LOG("%s\n", entry->p_name);
                entry->func_in();
            }

            entry->func_exe();

            if (*fsm->p_fsm_ev)
            {
                uint32_t next = entry->func_chk(*fsm->p_fsm_ev);
                if (next && next != entry->fsm_sta)
                {
                    PLECS_LOG("%s-chk_ev:%d\n", entry->p_name, *fsm->p_fsm_ev);
                    entry->func_out();
                    fsm->fsm_sta = next;
                    fsm->fsm_sta_is_change = 1;
                }
                *fsm->p_fsm_ev = 0;
            }
            break;
        }
    }
}
