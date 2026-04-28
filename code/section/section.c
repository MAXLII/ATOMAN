/**
 * @file section.c
 * @brief Runtime implementation of the section-based auto-registration framework.
 */

#include "section.h"

#include <stddef.h>
#include <string.h>

reg_task_t *p_task_first = NULL;
reg_interrupt_t *p_interrupt_first = NULL;
section_link_t *p_link_first = NULL;
reg_init_t *p_init_first = NULL;
static volatile section_perf_record_t *s_running_task_perf_record = NULL;
static volatile uint32_t s_running_task_interrupt_time = 0u;

static void task_insert(reg_task_t *task)
{
    static reg_task_t *s_task_tail = NULL;

    if (!task)
        return;

    task->time_last = SECTION_SYS_TICK;
    task->p_next = NULL;

    if (p_task_first == NULL)
    {
        p_task_first = task;
        s_task_tail = task;
    }
    else
    {
        s_task_tail->p_next = task;
        s_task_tail = task;
    }
}

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

static void link_insert(section_link_t *link)
{
    static section_link_t *s_link_tail = NULL;

    if (!link)
        return;

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

static inline uint32_t perf_cnt_read(uint32_t *const *pp_cnt)
{
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

        const uint32_t elapsed = (uint32_t)(now - task->time_last);
        if (elapsed < period)
        {
            continue;
        }

        uint32_t perf_start = 0u;
        section_perf_record_t *rec = task->p_perf_record;
        if (rec)
        {
            perf_start = perf_cnt_read(rec->p_cnt);
#if (INTERRUPT_RECORD_PERF_ENABLE == 1)
            s_running_task_interrupt_time = 0u;
            s_running_task_perf_record = rec;
#endif
        }

        task->p_func();

        if (rec)
        {
            const uint32_t perf_end = perf_cnt_read(rec->p_cnt);
            uint32_t delta = (uint32_t)(perf_end - perf_start);
#if (INTERRUPT_RECORD_PERF_ENABLE == 1)
            uint32_t interrupt_time = s_running_task_interrupt_time;

            s_running_task_perf_record = NULL;
            s_running_task_interrupt_time = 0u;
            if (delta > interrupt_time)
            {
                delta -= interrupt_time;
            }
            else
            {
                delta = 0u;
            }
#endif

            rec->time = delta;
            rec->max_time = (delta > rec->max_time) ? delta : rec->max_time;
            rec->run_time += delta;
        }

        if (0)
        {
            task->time_last = now;
        }
        else
        {
            const uint32_t k = elapsed / period;
            task->time_last += k * period;
        }
    }
}

void section_interrupt(void)
{
    for (reg_interrupt_t *p = p_interrupt_first; p != NULL; p = (reg_interrupt_t *)p->p_next)
    {
#if (INTERRUPT_RECORD_PERF_ENABLE == 1)
        uint32_t perf_start = 0u;
        section_perf_record_t *rec = p->p_perf_record;

        if (rec)
        {
            perf_start = perf_cnt_read(rec->p_cnt);
        }

        p->p_func();

        if (rec)
        {
            const uint32_t perf_end = perf_cnt_read(rec->p_cnt);
            const uint32_t delta = (uint32_t)(perf_end - perf_start);

            rec->time = delta;
            rec->max_time = (delta > rec->max_time) ? delta : rec->max_time;
            rec->run_time += delta;
            if (s_running_task_perf_record != NULL)
            {
                s_running_task_interrupt_time += delta;
            }
        }
#else
        p->p_func();
#endif
    }
}

static void link_process(section_link_t *link)
{
    uint8_t data = 0u;

    if (!link || !link->rx_get_byte || !link->handler_arr)
        return;

    while (link->rx_get_byte(&data) != 0u)
    {
        for (uint32_t i = 0; i < link->handler_num; ++i)
        {
            const section_link_handler_item_t *it = &link->handler_arr[i];
            if (it->func)
                it->func(data, link->my_printf, it->ctx);
        }
    }
}

static void section_link_task(void)
{
    for (section_link_t *p = p_link_first; p != NULL; p = (section_link_t *)p->p_next)
    {
        link_process(p);
    }
}

REG_TASK(10, section_link_task)

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
