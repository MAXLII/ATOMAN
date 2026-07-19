// SPDX-License-Identifier: MIT
/**
 * @file    section.c
 * @brief   Bare-metal section runtime module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Discover AUTO_REG_SECTION records emitted by the linker
 *          - Build ordered runtime lists for init, task, interrupt, link, and FSM callbacks
 *          - Execute periodic tasks cooperatively without an exception-based context switcher
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-07-17
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
#include <string.h>

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
volatile section_critical_race_debug_t g_section_critical_race_debug;

#if (SECTION_CRITICAL_RACE_PROBE_ENABLE == 1u)
static volatile uint32_t section_race_probe_depth = 0u;

static void section_race_probe_delay(void)
{
    volatile uint32_t spin = 0u;

    for (spin = 0u; spin < SECTION_CRITICAL_RACE_PROBE_SPIN; ++spin)
    {
    }
}

static void section_race_probe_invariant(reg_task_t *first, reg_task_t *tail)
{
    if (((first == NULL) && (tail != NULL)) ||
        ((first != NULL) && (tail == NULL)))
    {
        g_section_critical_race_debug.probe_invariant_fail_count++;
    }
}

static void section_race_probe_begin(uint32_t tag)
{
    uint32_t depth = 0u;

    g_section_critical_race_debug.probe_enter_count++;
    g_section_critical_race_debug.probe_last_tag = tag;

    depth = section_race_probe_depth;
    if (depth != 0u)
    {
        g_section_critical_race_debug.probe_reentry_count++;
    }

    depth++;
    section_race_probe_depth = depth;
    if (depth > g_section_critical_race_debug.probe_max_depth)
    {
        g_section_critical_race_debug.probe_max_depth = depth;
    }

    section_race_probe_delay();
}

static void section_race_probe_end(void)
{
    section_race_probe_delay();
    if (section_race_probe_depth != 0u)
    {
        section_race_probe_depth--;
    }
}
#else
#define section_race_probe_delay() \
    do                             \
    {                              \
    } while (0)
#define section_race_probe_invariant(first, tail) \
    do                                           \
    {                                            \
        (void)(first);                           \
        (void)(tail);                            \
    } while (0)
#define section_race_probe_begin(tag) \
    do                                \
    {                                 \
        (void)(tag);                  \
    } while (0)
#define section_race_probe_end() \
    do                           \
    {                            \
    } while (0)
#endif

#if (PERF_ENABLE)
#define SECTION_TASK_PERF_LOCALS()     \
    section_perf_record_t *rec = NULL; \
    uint32_t perf_start = 0u
#define SECTION_TASK_PERF_BEGIN(task)              \
    do                                             \
    {                                              \
        rec = (task)->p_perf_record;               \
        perf_start = section_perf_task_begin(rec); \
    } while (0)
#define SECTION_TASK_PERF_END()                 \
    do                                          \
    {                                           \
        section_perf_task_end(rec, perf_start); \
    } while (0)
#define SECTION_TASK_PERF_PERIOD_SET(task)                                         \
    do                                                                             \
    {                                                                              \
        section_perf_task_period_set((task)->p_perf_record,                        \
                                     (task)->t_period * SECTION_SYS_TICK_UNIT_US); \
    } while (0)
#if (PERF_INTERRUPT_ENABLE == 1u)
#define SECTION_INTERRUPT_PERF_RUN(item)                         \
    do                                                           \
    {                                                            \
        section_perf_record_t *rec = (item)->p_perf_record;      \
        uint32_t perf_start = section_perf_interrupt_begin(rec); \
        (item)->p_func();                                        \
        section_perf_interrupt_end(rec, perf_start);             \
    } while (0)
#else
#define SECTION_INTERRUPT_PERF_RUN(item) \
    do                                   \
    {                                    \
        (item)->p_func();                \
    } while (0)
#endif
#else
#define SECTION_TASK_PERF_LOCALS()
#define SECTION_TASK_PERF_BEGIN(task) \
    do                                \
    {                                 \
        (void)(task);                 \
    } while (0)
#define SECTION_TASK_PERF_END() \
    do                          \
    {                           \
    } while (0)
#define SECTION_TASK_PERF_PERIOD_SET(task) \
    do                                     \
    {                                      \
        (void)(task);                      \
    } while (0)
#define SECTION_INTERRUPT_PERF_RUN(item) \
    do                                   \
    {                                    \
        (item)->p_func();                \
    } while (0)
#endif

section_runtime_kind_t section_runtime_kind_get(void)
{
    return SECTION_RUNTIME_BAREMETAL;
}

const char *section_runtime_name_get(void)
{
    return "baremetal";
}

uint32_t section_runtime_preemptive_get(void)
{
    return SECTION_RUNTIME_PREEMPTIVE;
}

void section_port_init(void)
{
}

void section_task_irq_exit_request(void)
{
}

void section_task_start(void)
{
}

void section_task_yield(void)
{
}

void section_task_complete_current(void)
{
}

void section_task_start_request(void)
{
}

uint32_t section_task_scheduler_started(void)
{
    return 0u;
}

uint32_t section_task_switch_pending(void)
{
    return 0u;
}

uint32_t section_task_slice_elapsed(void)
{
    return 0u;
}

uint32_t *section_task_start_sp_get(void)
{
    return NULL;
}

/**
 * @param[in] sp Stack pointer for the active exception context.
 * @return The original stack pointer because bare-metal mode does not switch context.
 */
uint32_t *section_task_switch_sp(uint32_t *sp)
{
    return sp;
}

static void task_ready_enqueue_unlocked(reg_task_t **first, reg_task_t **tail, reg_task_t *task);
static reg_task_t *task_ready_pop_unlocked(reg_task_t **first, reg_task_t **tail);

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

#if (PERF_ENABLE)
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
#endif

static uint32_t section_critical_enter(void)
{
#if (SECTION_CRITICAL_USE_PRIMASK == 1u)
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    __DSB();
    __ISB();
#if (SECTION_CRITICAL_RACE_PROBE_ENABLE == 1u)
    g_section_critical_race_debug.critical_enter_count++;
#endif
    return primask;
#else
#if (SECTION_CRITICAL_RACE_PROBE_ENABLE == 1u)
    g_section_critical_race_debug.critical_enter_count++;
#endif
    return 0u;
#endif
}

static void section_critical_exit(uint32_t primask)
{
#if (SECTION_CRITICAL_USE_PRIMASK == 1u)
#if (SECTION_CRITICAL_RACE_PROBE_ENABLE == 1u)
    g_section_critical_race_debug.critical_exit_count++;
#endif
    __set_PRIMASK(primask);
    __DSB();
    __ISB();
#else
    (void)primask;
#if (SECTION_CRITICAL_RACE_PROBE_ENABLE == 1u)
    g_section_critical_race_debug.critical_exit_count++;
#endif
#endif
}

static void task_ready_enqueue_unlocked(reg_task_t **first, reg_task_t **tail, reg_task_t *task)
{
    section_race_probe_begin(0x4252454Eu);
    if ((first == NULL) || (tail == NULL) || (task == NULL) || (task->is_ready != 0u))
    {
        section_race_probe_end();
        return;
    }

    section_race_probe_invariant(*first, *tail);
    task->p_ready_next = NULL;
    task->is_ready = 1u;
    section_race_probe_delay();

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
    section_race_probe_invariant(*first, *tail);
    section_race_probe_end();
}

static reg_task_t *task_ready_pop_unlocked(reg_task_t **first, reg_task_t **tail)
{
    reg_task_t *task = NULL;

    section_race_probe_begin(0x4252504Fu);
    if ((first == NULL) || (tail == NULL) || (*first == NULL))
    {
        section_race_probe_end();
        return NULL;
    }

    section_race_probe_invariant(*first, *tail);
    task = *first;
    *first = task->p_ready_next;
    section_race_probe_delay();
    if (*first == NULL)
    {
        *tail = NULL;
    }

    task->p_ready_next = NULL;
    task->is_ready = 0u;
    section_race_probe_invariant(*first, *tail);
    section_race_probe_end();

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
    SECTION_TASK_PERF_PERIOD_SET(task);

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
    (void)memset((void *)&g_section_critical_race_debug, 0, sizeof(g_section_critical_race_debug));
    p_task_first = NULL;
    p_task_tail = NULL;
    p_task_ready_first = NULL;
    p_task_ready_tail = NULL;
    p_task_unfinished_first = NULL;
    p_task_unfinished_tail = NULL;
    (void)memset((void *)&g_section_fault_debug, 0, sizeof(g_section_fault_debug));
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
    if ((elapsed >= task->t_period) && (task->is_ready == 0u) && (task->is_running == 0u))
    {
        task_schedule_next(task, elapsed);
        task_ready_enqueue_unlocked(&p_task_ready_first, &p_task_ready_tail, task);
    }

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

void run_task(void)
{
    reg_task_t *task = NULL;

    section_task_tick();

    task = task_ready_pop();
    while (task != NULL)
    {
        SECTION_TASK_PERF_LOCALS();
        SECTION_TASK_PERF_BEGIN(task);
        section_task_status_t status = task_run_step(task);

        SECTION_TASK_PERF_END();
        task_finish_step(task, status);

        section_task_tick();
        task = task_ready_pop();
    }
}

void FUNC_RAM section_interrupt(void)
{
    for (reg_interrupt_t *p = p_interrupt_first; p != NULL; p = p->p_next)
    {
        if (p->p_func == NULL)
        {
            continue;
        }

        SECTION_INTERRUPT_PERF_RUN(p);
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

REG_TASK(10, section_link_task)

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
