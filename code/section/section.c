// SPDX-License-Identifier: MIT
/**
 * @file    section.c
 * @brief   Section runtime module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Discover AUTO_REG_SECTION records emitted by the linker
 *          - Build ordered runtime lists for init, task, interrupt, link, and FSM callbacks
 *          - Execute registered callbacks and call optional instrumentation hooks around task/interrupt work
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-04-30
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "section.h"

#include <stddef.h>

static reg_task_t *p_task_first = NULL;
static reg_task_t *p_task_tail = NULL;
static reg_task_t *p_task_ready_first = NULL;
static reg_task_t *p_task_ready_tail = NULL;
static reg_task_t *p_task_unfinished_first = NULL;
static reg_task_t *p_task_unfinished_tail = NULL;
static reg_interrupt_t *p_interrupt_first = NULL;
static section_link_t *p_link_first = NULL;
static section_link_t *p_link_tail = NULL;
static reg_init_t *p_init_first = NULL;
static volatile uint8_t task_scheduler_ready = 0u;
volatile section_fault_debug_t g_section_fault_debug;

#if (SECTION_TASK_CONTEXT_SWITCH_ENABLE == 1)
typedef enum
{
    TASK_STACK_STATE_SLEEPING = 0,
    TASK_STACK_STATE_READY_NEW,
    TASK_STACK_STATE_READY_OLD,
    TASK_STACK_STATE_RUNNING,
} task_stack_state_t;

#define TASK_INITIAL_XPSR 0x01000000u
#define TASK_STACK_FILL_WORD 0xA5A5A5A5u
#define TASK_SW_CORE_FRAME_WORDS 9u
#define TASK_SW_FP_FRAME_WORDS 16u
#define TASK_HW_FP_FRAME_WORDS 18u

static reg_task_t *p_task_current = NULL;
static uint8_t task_scheduler_started = 0u;

static void task_ready_enqueue_unlocked(reg_task_t **first, reg_task_t **tail, reg_task_t *task);
static reg_task_t *task_ready_pop_unlocked(reg_task_t **first, reg_task_t **tail);
static void section_task_entry(void);
static void task_stack_init(reg_task_t *task);
static uint32_t task_stack_frame_valid(const reg_task_t *task);
static uint32_t task_stack_free_words_get(const reg_task_t *task);
static const uint32_t *task_hw_frame_get(const reg_task_t *task);
#endif

#if defined(SECTION_SENTINEL_REG_SECTION)
SECTION_REG_START_ATTR_PREFIX const reg_section_t section_reg_start = {0u, NULL};
SECTION_REG_STOP_ATTR_PREFIX const reg_section_t section_reg_stop = {0u, NULL};
#define SECTION_REG_FIRST ((const reg_section_t *)(&section_reg_start + 1))
#define SECTION_REG_LAST ((const reg_section_t *)&section_reg_stop)
#else
#define SECTION_REG_FIRST ((const reg_section_t *)&SECTION_START)
#define SECTION_REG_LAST ((const reg_section_t *)&SECTION_STOP)
#endif

#if defined(__GNUC__)
#define SECTION_WEAK __attribute__((weak))
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
#define SECTION_WEAK __weak
#else
#define SECTION_WEAK
#endif

SECTION_WEAK uint32_t section_perf_task_begin(section_perf_record_t *record)
{
    (void)record;
    return 0u;
}

SECTION_WEAK void section_perf_task_end(section_perf_record_t *record, uint32_t start_cnt)
{
    (void)record;
    (void)start_cnt;
}

SECTION_WEAK void section_perf_task_period_set(section_perf_record_t *record, uint32_t period_us)
{
    (void)record;
    (void)period_us;
}

SECTION_WEAK uint32_t FUNC_RAM section_perf_interrupt_begin(section_perf_record_t *record)
{
    (void)record;
    return 0u;
}

SECTION_WEAK void FUNC_RAM section_perf_interrupt_end(section_perf_record_t *record, uint32_t start_cnt)
{
    (void)record;
    (void)start_cnt;
}

static uint32_t section_critical_enter(void)
{
#if defined(IS_GD32) || defined(IS_HC32) || defined(IS_APM32)
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
#else
    return 0u;
#endif
}

static void section_critical_exit(uint32_t primask)
{
#if defined(IS_GD32) || defined(IS_HC32) || defined(IS_APM32)
    __set_PRIMASK(primask);
#else
    (void)primask;
#endif
}

#if (SECTION_TASK_CONTEXT_SWITCH_ENABLE == 1)
static void task_stack_init(reg_task_t *task)
{
    uint32_t *hw_frame = NULL;
    uint32_t *sp = NULL;

    if ((task == NULL) || (task->p_stack == NULL) || (task->stack_words < 33u))
    {
        return;
    }

    for (uint32_t i = 0u; i < task->stack_words; ++i)
    {
        task->p_stack[i] = TASK_STACK_FILL_WORD;
    }

    hw_frame = (uint32_t *)((uintptr_t)&task->p_stack[task->stack_words - 8u] & ~(uintptr_t)0x7u);
    sp = &hw_frame[-9];

    sp[0] = 0u;
    sp[1] = 0u;
    sp[2] = 0u;
    sp[3] = 0u;
    sp[4] = 0u;
    sp[5] = 0u;
    sp[6] = 0u;
    sp[7] = 0u;
    sp[8] = 0xFFFFFFFDu;

    hw_frame[0] = 0u;
    hw_frame[1] = 0u;
    hw_frame[2] = 0u;
    hw_frame[3] = 0u;
    hw_frame[4] = 0u;
    hw_frame[5] = 0xFFFFFFFDu;
    hw_frame[6] = ((uint32_t)(uintptr_t)section_task_entry) | 1u;
    hw_frame[7] = TASK_INITIAL_XPSR;

    task->p_sp = sp;
    task->state = (uint8_t)TASK_STACK_STATE_SLEEPING;
}

static uint32_t task_stack_frame_valid(const reg_task_t *task)
{
    uint32_t valid = 0u;
    const uint32_t *frame = task_hw_frame_get(task);

    if (frame != NULL)
    {
        const uint32_t pc = frame[6u];
        const uint32_t xpsr = frame[7u];

        if (((pc & 1u) != 0u) && ((xpsr & TASK_INITIAL_XPSR) == TASK_INITIAL_XPSR))
        {
            valid = 1u;
        }
    }

    return valid;
}

static const uint32_t *task_hw_frame_get(const reg_task_t *task)
{
    const uint32_t *frame = NULL;

    if ((task == NULL) || (task->p_sp == NULL))
    {
        return NULL;
    }

    frame = &task->p_sp[TASK_SW_CORE_FRAME_WORDS];
    if ((task->p_sp[8u] & 0x10u) == 0u)
    {
        frame = &frame[TASK_SW_FP_FRAME_WORDS + TASK_HW_FP_FRAME_WORDS];
    }

    return frame;
}

static uint32_t task_stack_free_words_get(const reg_task_t *task)
{
    uint32_t free_words = 0u;

    if ((task == NULL) || (task->p_stack == NULL))
    {
        return 0u;
    }

    while ((free_words < task->stack_words) && (task->p_stack[free_words] == TASK_STACK_FILL_WORD))
    {
        ++free_words;
    }

    return free_words;
}

static reg_task_t *task_stack_pick_next(void)
{
    reg_task_t *candidate = NULL;

    candidate = task_ready_pop_unlocked(&p_task_ready_first, &p_task_ready_tail);
    if (candidate == NULL)
    {
        candidate = task_ready_pop_unlocked(&p_task_unfinished_first, &p_task_unfinished_tail);
    }

    return candidate;
}
#endif

static void task_ready_enqueue_unlocked(reg_task_t **first, reg_task_t **tail, reg_task_t *task)
{
    if ((first == NULL) || (tail == NULL) || (task == NULL) || (task->is_ready != 0u))
    {
        return;
    }

    task->p_ready_next = NULL;
    task->is_ready = 1u;

    if (*first == NULL)
    {
        *first = task;
        *tail = task;
    }
    else
    {
        (*tail)->p_ready_next = task;
        *tail = task;
    }
}

static reg_task_t *task_ready_pop_unlocked(reg_task_t **first, reg_task_t **tail)
{
    reg_task_t *task = NULL;

    if ((first == NULL) || (tail == NULL) || (*first == NULL))
    {
        return NULL;
    }

    task = *first;
    *first = task->p_ready_next;
    if (*first == NULL)
    {
        *tail = NULL;
    }

    task->p_ready_next = NULL;
    task->is_ready = 0u;

    return task;
}

static reg_task_t *task_ready_pop(void)
{
    reg_task_t *task = NULL;
    uint32_t primask = section_critical_enter();

    task = task_ready_pop_unlocked(&p_task_ready_first, &p_task_ready_tail);
    if (task == NULL)
    {
        task = task_ready_pop_unlocked(&p_task_unfinished_first, &p_task_unfinished_tail);
    }

    if (task != NULL)
    {
        task->is_running = 1u;
    }

    section_critical_exit(primask);
    return task;
}

static void task_insert(reg_task_t *task)
{
    if (task == NULL)
    {
        return;
    }

    task->time_last = SECTION_SYS_TICK;
    task->p_next = NULL;
    task->p_ready_next = NULL;
    task->is_ready = 0u;
    task->is_running = 0u;
#if (SECTION_TASK_CONTEXT_SWITCH_ENABLE == 1)
    task_stack_init(task);
#endif
    section_perf_task_period_set(task->p_perf_record, task->t_period * SECTION_SYS_TICK_UNIT_US);

    if (p_task_first == NULL)
    {
        p_task_first = task;
        p_task_tail = task;
    }
    else
    {
        p_task_tail->p_next = task;
        p_task_tail = task;
    }
}

static void interrupt_insert(reg_interrupt_t *intr)
{
    if (intr == NULL)
    {
        return;
    }

    intr->p_next = NULL;

    if ((p_interrupt_first == NULL) || (intr->priority < p_interrupt_first->priority))
    {
        intr->p_next = p_interrupt_first;
        p_interrupt_first = intr;
    }
    else
    {
        reg_interrupt_t *prev = p_interrupt_first;
        while ((prev->p_next != NULL) && (prev->p_next->priority < intr->priority))
        {
            prev = prev->p_next;
        }
        intr->p_next = prev->p_next;
        prev->p_next = intr;
    }
}

static void link_insert(section_link_t *link)
{
    if (link == NULL)
    {
        return;
    }

    link->p_next = NULL;

    if (p_link_first == NULL)
    {
        p_link_first = link;
        p_link_tail = link;
    }
    else
    {
        p_link_tail->p_next = link;
        p_link_tail = link;
    }
}

static void init_insert(reg_init_t *init)
{
    if (init == NULL)
    {
        return;
    }

    init->p_next = NULL;

    if ((p_init_first == NULL) || (init->priority < p_init_first->priority))
    {
        init->p_next = p_init_first;
        p_init_first = init;
    }
    else
    {
        reg_init_t *prev = p_init_first;
        while ((prev->p_next != NULL) && (prev->p_next->priority <= init->priority))
        {
            prev = prev->p_next;
        }
        init->p_next = prev->p_next;
        prev->p_next = init;
    }
}

void section_init(void)
{
    task_scheduler_ready = 0u;

    for (const reg_section_t *p = SECTION_REG_FIRST;
         p < SECTION_REG_LAST;
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

    for (reg_init_t *init = p_init_first; init != NULL; init = init->p_next)
    {
        if (init->p_func != NULL)
        {
            init->p_func();
        }
    }

    task_scheduler_ready = 1u;
}

void section_runtime_reset(void)
{
    task_scheduler_ready = 0u;
    p_task_first = NULL;
    p_task_tail = NULL;
    p_task_ready_first = NULL;
    p_task_ready_tail = NULL;
    p_task_unfinished_first = NULL;
    p_task_unfinished_tail = NULL;
#if (SECTION_TASK_CONTEXT_SWITCH_ENABLE == 1)
    p_task_current = NULL;
    task_scheduler_started = 0u;
#endif
    p_interrupt_first = NULL;
    p_link_first = NULL;
    p_link_tail = NULL;
    p_init_first = NULL;
}

const section_link_t *section_link_first_get(void)
{
    return p_link_first;
}

static void task_schedule_next(reg_task_t *task, uint32_t elapsed)
{
    const uint32_t periods_elapsed = elapsed / task->t_period;

    task->time_last += periods_elapsed * task->t_period;
}

static void task_activate_if_due(reg_task_t *task, uint32_t now)
{
    uint32_t elapsed = 0u;
    uint32_t primask = 0u;

    if ((task == NULL) || ((task->p_func == NULL) && (task->p_step_func == NULL)) || (task->t_period == 0u))
    {
        return;
    }

    elapsed = (uint32_t)(now - task->time_last);
    if (elapsed < task->t_period)
    {
        return;
    }

    primask = section_critical_enter();

    elapsed = (uint32_t)(now - task->time_last);
#if (SECTION_TASK_CONTEXT_SWITCH_ENABLE == 1)
    if ((elapsed >= task->t_period) && (task->state == (uint8_t)TASK_STACK_STATE_SLEEPING))
    {
        task_schedule_next(task, elapsed);
        task->state = (uint8_t)TASK_STACK_STATE_READY_NEW;
        task_ready_enqueue_unlocked(&p_task_ready_first, &p_task_ready_tail, task);
    }
#else
    if ((elapsed >= task->t_period) && (task->is_ready == 0u) && (task->is_running == 0u))
    {
        task_schedule_next(task, elapsed);
        task_ready_enqueue_unlocked(&p_task_ready_first, &p_task_ready_tail, task);
    }
#endif

    section_critical_exit(primask);
}

void section_task_tick(void)
{
    const uint32_t now = SECTION_SYS_TICK;

    if (task_scheduler_ready == 0u)
    {
        return;
    }

    for (reg_task_t *task = p_task_first; task != NULL; task = task->p_next)
    {
        task_activate_if_due(task, now);
    }
}

static section_task_status_t task_run_step(reg_task_t *task)
{
    section_task_status_t status = SECTION_TASK_DONE;

    if (task == NULL)
    {
        return SECTION_TASK_DONE;
    }

    if (task->p_step_func != NULL)
    {
        status = task->p_step_func(task->p_ctx);
    }
    else if (task->p_func != NULL)
    {
        task->p_func();
        status = SECTION_TASK_DONE;
    }
    else
    {
        status = SECTION_TASK_DONE;
    }

    return status;
}

static void task_finish_step(reg_task_t *task, section_task_status_t status)
{
    uint32_t primask = section_critical_enter();

    if (task != NULL)
    {
        task->is_running = 0u;
        if (status == SECTION_TASK_RUNNING)
        {
            task_ready_enqueue_unlocked(&p_task_unfinished_first, &p_task_unfinished_tail, task);
        }
    }

    section_critical_exit(primask);
}

#if (SECTION_TASK_CONTEXT_SWITCH_ENABLE == 1)
uint32_t section_task_scheduler_started(void)
{
    return (uint32_t)task_scheduler_started;
}

uint32_t section_task_switch_pending(void)
{
    uint32_t pending = 0u;
    uint32_t primask = section_critical_enter();

    if ((p_task_ready_first != NULL) || (p_task_unfinished_first != NULL))
    {
        pending = 1u;
    }

    section_critical_exit(primask);
    return pending;
}

void section_task_start_request(void)
{
    task_scheduler_started = 1u;
#if defined(FPU) && (__FPU_PRESENT == 1U)
    FPU->FPCCR &= ~FPU_FPCCR_LSPEN_Msk;
#endif
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    __DSB();
    __ISB();
}

void section_task_yield(void)
{
    if (task_scheduler_started != 0u)
    {
        SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
        __DSB();
        __ISB();
    }
}

void section_task_complete_current(void)
{
    uint32_t primask = section_critical_enter();

    if (p_task_current != NULL)
    {
        p_task_current->state = (uint8_t)TASK_STACK_STATE_SLEEPING;
    }

    section_critical_exit(primask);
}

uint32_t *section_task_start_sp_get(void)
{
    reg_task_t *next = NULL;

    section_task_tick();
    next = task_stack_pick_next();
    if (next == NULL)
    {
        return NULL;
    }

    if (next->state == (uint8_t)TASK_STACK_STATE_READY_NEW)
    {
        task_stack_init(next);
    }
    next->state = (uint8_t)TASK_STACK_STATE_RUNNING;
    p_task_current = next;
    task_scheduler_started = 1u;
    return next->p_sp;
}

uint32_t *section_task_switch_sp(uint32_t *sp)
{
    reg_task_t *next = NULL;

    if (task_scheduler_started == 0u)
    {
        return sp;
    }

    if ((sp != NULL) && (p_task_current != NULL))
    {
        p_task_current->p_sp = sp;
        if (p_task_current->state == (uint8_t)TASK_STACK_STATE_RUNNING)
        {
            p_task_current->state = (uint8_t)TASK_STACK_STATE_READY_OLD;
            task_ready_enqueue_unlocked(&p_task_unfinished_first, &p_task_unfinished_tail, p_task_current);
        }
    }

    section_task_tick();
    next = task_stack_pick_next();
    if (next != NULL)
    {
        const uint32_t *frame = NULL;

        if (next->state == (uint8_t)TASK_STACK_STATE_READY_NEW)
        {
            task_stack_init(next);
        }

        next->state = (uint8_t)TASK_STACK_STATE_RUNNING;
        frame = task_hw_frame_get(next);
        p_task_current = next;
        g_section_fault_debug.task_sp = (uint32_t)(uintptr_t)next->p_sp;
        if (frame != NULL)
        {
            g_section_fault_debug.task_pc = frame[6u];
            g_section_fault_debug.task_xpsr = frame[7u];
        }
        g_section_fault_debug.task_stack_base = (uint32_t)(uintptr_t)next->p_stack;
        g_section_fault_debug.task_stack_words = next->stack_words;
        g_section_fault_debug.task_frame_valid = task_stack_frame_valid(next);
        g_section_fault_debug.task_name = (uint32_t)(uintptr_t)next->p_name;
        return next->p_sp;
    }

    if ((sp != NULL) && (p_task_current != NULL))
    {
        return p_task_current->p_sp;
    }

    return sp;
}

void section_task_start(void)
{
    if (task_scheduler_started == 0u)
    {
        section_task_tick();
        if ((p_task_ready_first != NULL) || (p_task_unfinished_first != NULL))
        {
            __ASM volatile("svc 0");
        }
    }
}

static void section_task_run_current(void)
{
    section_task_status_t status = SECTION_TASK_DONE;
    section_perf_record_t *rec = NULL;
    uint32_t perf_start = 0u;

    if (p_task_current == NULL)
    {
        return;
    }

    if (p_task_current->p_func != NULL)
    {
        rec = p_task_current->p_perf_record;
        perf_start = section_perf_task_begin(rec);
        p_task_current->p_func();
        section_perf_task_end(rec, perf_start);
    }
    else if (p_task_current->p_step_func != NULL)
    {
        do
        {
            rec = p_task_current->p_perf_record;
            perf_start = section_perf_task_begin(rec);
            status = p_task_current->p_step_func(p_task_current->p_ctx);
            section_perf_task_end(rec, perf_start);

            if (status == SECTION_TASK_RUNNING)
            {
                section_task_yield();
            }
        } while (status == SECTION_TASK_RUNNING);
    }
    else
    {
        /* Empty task descriptor. */
    }
}

static void section_task_entry(void)
{
    for (;;)
    {
        if ((p_task_current == NULL) || (p_task_current->state != (uint8_t)TASK_STACK_STATE_RUNNING))
        {
            section_task_yield();
            continue;
        }

        section_task_run_current();
        section_task_complete_current();
        section_task_yield();
    }
}

void run_task(void)
{
    section_task_tick();

    if (task_scheduler_started == 0u)
    {
        section_task_start();
    }
    else
    {
        section_task_yield();
    }
}
#else
void run_task(void)
{
    reg_task_t *task = NULL;

    section_task_tick();

    task = task_ready_pop();
    while (task != NULL)
    {
        section_perf_record_t *rec = task->p_perf_record;
        uint32_t perf_start = section_perf_task_begin(rec);
        section_task_status_t status = task_run_step(task);

        section_perf_task_end(rec, perf_start);
        task_finish_step(task, status);

        section_task_tick();
        task = task_ready_pop();
    }
}
#endif

void FUNC_RAM section_interrupt(void)
{
    for (reg_interrupt_t *p = p_interrupt_first; p != NULL; p = p->p_next)
    {
        if (p->p_func == NULL)
        {
            continue;
        }

#if (INTERRUPT_RECORD_PERF_ENABLE == 1)
        section_perf_record_t *rec = p->p_perf_record;
        uint32_t perf_start = section_perf_interrupt_begin(rec);

        p->p_func();

        section_perf_interrupt_end(rec, perf_start);
#else
        p->p_func();
#endif
    }
}

static void link_process(section_link_t *link)
{
    uint8_t data = 0u;

    if ((link == NULL) || (link->rx_get_byte == NULL) || (link->handler_arr == NULL))
    {
        return;
    }

    while (link->rx_get_byte(&data) != 0u)
    {
        for (uint32_t i = 0; i < link->handler_num; ++i)
        {
            const section_link_handler_item_t *it = &link->handler_arr[i];
            if (it->func != NULL)
            {
                it->func(data, link->my_printf, it->ctx);
            }
        }
    }
}

static void section_link_task(void)
{
    for (section_link_t *p = p_link_first; p != NULL; p = p->p_next)
    {
        link_process(p);
    }
}

REG_TASK_STACK(10, section_link_task, 256u)

void section_fsm_func(reg_fsm_t *fsm)
{
    if ((fsm == NULL) || (fsm->p_fsm_func_table == NULL) || (fsm->p_fsm_ev == NULL))
    {
        return;
    }

    for (uint32_t i = 0; i < fsm->fsm_table_size; ++i)
    {
        reg_fsm_func_t *entry = &fsm->p_fsm_func_table[i];
        if (fsm->fsm_sta == entry->fsm_sta)
        {
            if (fsm->fsm_sta_is_change != 0u)
            {
                fsm->fsm_sta_is_change = 0;
                PLECS_LOG("%s\n", entry->p_name);
                if (entry->func_in != NULL)
                {
                    entry->func_in();
                }
            }

            if (entry->func_exe != NULL)
            {
                entry->func_exe();
            }

            if (*fsm->p_fsm_ev != 0u)
            {
                uint32_t next = 0u;

                if (entry->func_chk != NULL)
                {
                    next = entry->func_chk(*fsm->p_fsm_ev);
                }

                if ((next != 0u) && (next != entry->fsm_sta))
                {
                    PLECS_LOG("%s-chk_ev:%lu\n", entry->p_name, (unsigned long)*fsm->p_fsm_ev);
                    if (entry->func_out != NULL)
                    {
                        entry->func_out();
                    }
                    fsm->fsm_sta = next;
                    fsm->fsm_sta_is_change = 1u;
                }
                *fsm->p_fsm_ev = 0u;
            }
            break;
        }
    }
}
