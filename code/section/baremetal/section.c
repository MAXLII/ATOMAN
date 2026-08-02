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

section_item_t *p_task_first = NULL;
static section_item_t *p_task_tail = NULL;
section_item_t *p_interrupt_first = NULL;
section_item_t *p_link_first = NULL;
static section_item_t *p_link_tail = NULL;
section_item_t *p_init_first = NULL;

REG_DBG_LIST(init, p_init_first)
REG_DBG_LIST(task, p_task_first)
REG_DBG_LIST(interrupt, p_interrupt_first)
REG_DBG_LIST(link, p_link_first)
static volatile uint8_t task_scheduler_ready = 0u;
volatile section_fault_debug_t g_section_fault_debug;
volatile section_critical_race_debug_t g_section_critical_race_debug;

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

static void task_insert(section_item_t *p_item)
{
    reg_task_t *p_task = NULL;

    if ((p_item == NULL) || (p_item->p_obj == NULL))
    {
        return;
    }

    p_task = (reg_task_t *)p_item->p_obj;
    p_task->time_last = SECTION_SYS_TICK;
    p_task->is_ready = 0u;
    SECTION_TASK_PERF_PERIOD_SET(p_task);
    p_item->p_next = NULL;

    if (p_task_first == NULL)
    {
        p_task_first = p_item;
        p_task_tail = p_item;
    }
    else
    {
        p_task_tail->p_next = p_item;
        p_task_tail = p_item;
    }
}

static void interrupt_insert(section_item_t *p_item)
{
    reg_interrupt_t *p_interrupt = NULL;
    section_item_t *p_prev = NULL;

    if ((p_item == NULL) || (p_item->p_obj == NULL))
    {
        return;
    }

    p_interrupt = (reg_interrupt_t *)p_item->p_obj;

    if ((p_interrupt_first == NULL) ||
        (p_interrupt->priority < ((reg_interrupt_t *)p_interrupt_first->p_obj)->priority))
    {
        p_item->p_next = p_interrupt_first;
        p_interrupt_first = p_item;
    }
    else
    {
        p_prev = p_interrupt_first;
        while ((p_prev->p_next != NULL) &&
               (((reg_interrupt_t *)p_prev->p_next->p_obj)->priority < p_interrupt->priority))
        {
            p_prev = p_prev->p_next;
        }
        p_item->p_next = p_prev->p_next;
        p_prev->p_next = p_item;
    }
}

static void link_insert(section_item_t *p_item)
{
    if ((p_item == NULL) || (p_item->p_obj == NULL))
    {
        return;
    }

    p_item->p_next = NULL;
    if (p_link_first == NULL)
    {
        p_link_first = p_item;
        p_link_tail = p_item;
    }
    else
    {
        p_link_tail->p_next = p_item;
        p_link_tail = p_item;
    }
}

static void init_insert(section_item_t *p_item)
{
    reg_init_t *p_init = NULL;
    section_item_t *p_prev = NULL;

    if ((p_item == NULL) || (p_item->p_obj == NULL))
    {
        return;
    }

    p_init = (reg_init_t *)p_item->p_obj;

    if ((p_init_first == NULL) ||
        (p_init->priority < ((reg_init_t *)p_init_first->p_obj)->priority))
    {
        p_item->p_next = p_init_first;
        p_init_first = p_item;
    }
    else
    {
        p_prev = p_init_first;
        while ((p_prev->p_next != NULL) &&
               (((reg_init_t *)p_prev->p_next->p_obj)->priority <= p_init->priority))
        {
            p_prev = p_prev->p_next;
        }
        p_item->p_next = p_prev->p_next;
        p_prev->p_next = p_item;
    }
}

void section_init(void)
{
    task_scheduler_ready = 0u;
    p_init_first = NULL;
    p_task_first = NULL;
    p_task_tail = NULL;
    p_interrupt_first = NULL;
    p_link_first = NULL;
    p_link_tail = NULL;

    for (const reg_section_t *p = SECTION_REG_FIRST;
         p < SECTION_REG_LAST;
         ++p)
    {
        switch (p->section_type)
        {
        case SECTION_INIT:
            init_insert((section_item_t *)p->p_str);
            break;
        case SECTION_TASK:
            task_insert((section_item_t *)p->p_str);
            break;
        case SECTION_INTERRUPT:
            interrupt_insert((section_item_t *)p->p_str);
            break;
        case SECTION_LINK:
            link_insert((section_item_t *)p->p_str);
            break;
        default:
            break;
        }
    }

    for (section_item_t *p_item = p_init_first; p_item != NULL; p_item = p_item->p_next)
    {
        reg_init_t *p_init = (reg_init_t *)p_item->p_obj;
        if (p_init->p_func != NULL)
        {
            p_init->p_func();
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
    (void)memset((void *)&g_section_fault_debug, 0, sizeof(g_section_fault_debug));
    p_interrupt_first = NULL;
    p_link_first = NULL;
    p_link_tail = NULL;
    p_init_first = NULL;
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

    if ((task == NULL) || (task->p_func == NULL) || (task->t_period == 0u))
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
    if ((elapsed >= task->t_period) && (task->is_ready == 0u))
    {
        task_schedule_next(task, elapsed);
        task->is_ready = 1u;
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

    for (section_item_t *p_item = p_task_first; p_item != NULL; p_item = p_item->p_next)
    {
        task_activate_if_due((reg_task_t *)p_item->p_obj, now);
    }
}

static uint8_t task_claim(reg_task_t *p_task)
{
    uint32_t primask = section_critical_enter();
    uint8_t claimed = 0u;

    if ((p_task != NULL) && (p_task->is_ready != 0u))
    {
        p_task->is_ready = 0u;
        claimed = 1u;
    }

    section_critical_exit(primask);
    return claimed;
}

void run_task(void)
{
    section_task_tick();

    for (section_item_t *p_item = p_task_first; p_item != NULL; p_item = p_item->p_next)
    {
        reg_task_t *p_task = (reg_task_t *)p_item->p_obj;

        if (task_claim(p_task) == 0u)
        {
            continue;
        }

        SECTION_TASK_PERF_LOCALS();
        SECTION_TASK_PERF_BEGIN(p_task);
        p_task->p_func();
        SECTION_TASK_PERF_END();
    }
}

void FUNC_RAM section_interrupt(void)
{
    for (section_item_t *p_item = p_interrupt_first; p_item != NULL; p_item = p_item->p_next)
    {
        reg_interrupt_t *p = (reg_interrupt_t *)p_item->p_obj;
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
    for (section_item_t *p_item = p_link_first; p_item != NULL; p_item = p_item->p_next)
    {
        link_process((section_link_t *)p_item->p_obj);
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
