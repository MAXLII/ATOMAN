// SPDX-License-Identifier: MIT
/**
 * @file    section.c
 * @brief   Cortex-M section SRTOS runtime module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Discover AUTO_REG_SECTION records emitted by the linker
 *          - Run registered tasks on one shared runtime stack
 *          - Snapshot suspended Cortex-M task contexts into the fixed context pool
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

section_item_t task_list;
section_item_t interrupt_list;
section_item_t link_list;
section_item_t init_list;

REG_DBG_LIST(init, init_list)
REG_DBG_LIST(task, task_list)
REG_DBG_LIST(interrupt, interrupt_list)
REG_DBG_LIST(link, link_list)
static uint8_t section_list_contains(const section_item_t *p_head,
                                     const section_item_t *p_item)
{
    const section_item_t *p_cursor = NULL;

    if ((p_head == NULL) || (p_item == NULL))
    {
        return 0u;
    }

    for (p_cursor = p_head->p_next; p_cursor != NULL; p_cursor = p_cursor->p_next)
    {
        if (p_cursor == p_item)
        {
            return 1u;
        }
    }

    return 0u;
}

void section_list_init(section_item_t *p_head)
{
    if (p_head == NULL)
    {
        return;
    }

    p_head->p_obj = NULL;
    p_head->p_next = NULL;
}

uint32_t section_list_count(const section_item_t *p_head)
{
    const section_item_t *p_cursor = NULL;
    uint32_t count = 0u;

    if (p_head == NULL)
    {
        return 0u;
    }

    for (p_cursor = p_head->p_next;
         (p_cursor != NULL) && (count < UINT32_MAX);
         p_cursor = p_cursor->p_next)
    {
        count++;
    }
    return count;
}

void section_list_push_front(section_item_t *p_head, section_item_t *p_item)
{
    if ((p_head == NULL) || (p_item == NULL) ||
        (section_list_contains(p_head, p_item) == 1u))
    {
        return;
    }

    p_item->p_next = p_head->p_next;
    p_head->p_next = p_item;
}

void section_list_push_back(section_item_t *p_head, section_item_t *p_item)
{
    section_item_t *p_tail = NULL;

    if ((p_head == NULL) || (p_item == NULL) ||
        (section_list_contains(p_head, p_item) == 1u))
    {
        return;
    }

    p_item->p_next = NULL;
    p_tail = p_head;
    while (p_tail->p_next != NULL)
    {
        p_tail = p_tail->p_next;
    }
    p_tail->p_next = p_item;
}

void section_list_insert_after(section_item_t *p_head,
                               section_item_t *p_prev,
                               section_item_t *p_item)
{
    if ((p_head == NULL) || (p_item == NULL) ||
        (section_list_contains(p_head, p_item) == 1u))
    {
        return;
    }

    if (p_prev == NULL)
    {
        section_list_push_front(p_head, p_item);
        return;
    }

    if (section_list_contains(p_head, p_prev) == 0u)
    {
        return;
    }

    p_item->p_next = p_prev->p_next;
    p_prev->p_next = p_item;
}

static void section_list_remove(section_item_t *p_head, section_item_t *p_item)
{
    section_item_t *p_prev = NULL;
    section_item_t *p_cursor = NULL;

    if ((p_head == NULL) || (p_item == NULL))
    {
        return;
    }

    p_prev = p_head;
    for (p_cursor = p_head->p_next; p_cursor != NULL; p_cursor = p_cursor->p_next)
    {
        if (p_cursor == p_item)
        {
            break;
        }
        p_prev = p_cursor;
    }

    if (p_cursor == NULL)
    {
        return;
    }

    p_prev->p_next = p_cursor->p_next;
    p_cursor->p_next = NULL;
}

void section_list_move_to_front(section_item_t *p_head, section_item_t *p_item)
{
    if ((p_head == NULL) || (p_item == NULL) || (p_head->p_next == p_item))
    {
        return;
    }

    if (section_list_contains(p_head, p_item) == 0u)
    {
        return;
    }

    section_list_remove(p_head, p_item);
    section_list_push_front(p_head, p_item);
}

section_item_t *section_list_at(const section_item_t *p_head, uint32_t index)
{
    section_item_t *p_item = NULL;
    uint32_t current_index = 0u;

    if (p_head == NULL)
    {
        return NULL;
    }

    p_item = p_head->p_next;
    while ((p_item != NULL) && (current_index < index))
    {
        p_item = p_item->p_next;
        current_index++;
    }

    return p_item;
}

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
    return SECTION_RUNTIME_SRTOS_M;
}

const char *section_runtime_name_get(void)
{
    return "srtos-m";
}

uint32_t section_runtime_preemptive_get(void)
{
    return SECTION_RUNTIME_PREEMPTIVE;
}

void section_port_init(void)
{
}

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

#if (SECTION_TASK_CONTEXT_POOL_FULL_POLICY != SECTION_TASK_CONTEXT_POOL_FAULT) &&       \
    (SECTION_TASK_CONTEXT_POOL_FULL_POLICY != SECTION_TASK_CONTEXT_POOL_KEEP_RUNNING)
#error "Invalid SECTION_TASK_CONTEXT_POOL_FULL_POLICY."
#endif

static reg_task_t *p_srtos_task_ready_first = NULL;
static reg_task_t *p_srtos_task_ready_tail = NULL;
static reg_task_t *p_srtos_task_unfinished_first = NULL;
static reg_task_t *p_srtos_task_unfinished_tail = NULL;
static reg_task_t *p_task_current = NULL;
static uint8_t task_scheduler_started = 0u;
static uint32_t task_runtime_stack[SECTION_TASK_RUNTIME_STACK_WORDS] SECTION_TASK_STACK_ATTR;
static uint32_t task_context_pool[SECTION_TASK_CONTEXT_POOL_WORDS] SECTION_TASK_STACK_ATTR;
static uint32_t task_context_pool_head = 0u;
static uint32_t task_context_pool_tail = 0u;
static uint32_t task_context_pool_used = 0u;
static uint32_t task_context_pool_gap_start = 0u;
static uint32_t task_context_pool_gap_words = 0u;
static uint32_t task_last_switch_tick = 0u;
static uint8_t task_fault_active = 0u;

static void srtos_task_ready_enqueue_unlocked(reg_task_t **first, reg_task_t **tail, reg_task_t *task);
static reg_task_t *srtos_task_ready_pop_unlocked(reg_task_t **first, reg_task_t **tail);
static void section_task_entry(void);
static void srtos_task_schedule_next(reg_task_t *task, uint32_t elapsed);
static uint32_t task_context_alloc(reg_task_t *task, uint32_t required_words);
static void task_context_release(reg_task_t *task);
static void task_context_release_gap_if_head(void);
static uint32_t section_critical_enter(void);
static void section_critical_exit(uint32_t primask);
static uint32_t *task_runtime_stack_low_get(void);
static uint32_t *task_runtime_stack_top_get(void);
static void task_stack_prepare_initial(reg_task_t *task);
static uint32_t task_stack_save(reg_task_t *task, uint32_t *sp);
static uint32_t *task_stack_restore(reg_task_t *task);
static uint32_t task_stack_frame_valid(const reg_task_t *task);
static uint32_t task_stack_free_words_get(const reg_task_t *task);
static const uint32_t *task_hw_frame_get(const reg_task_t *task);
static reg_task_t *task_stack_pick_next(void);
static void task_slice_reset(void);
static void task_debug_context_pool_update(void);
static uint32_t task_runtime_stack_used_words_get(uint32_t *sp);
static void task_fault_set(uint32_t reason, const reg_task_t *task, uint32_t *sp, uint32_t required_words);
static section_task_status_t section_task_run_current(void);
static void section_task_continue_current(void);

static void srtos_task_insert_init(reg_task_t *task)
{
    if (task == NULL)
    {
        return;
    }

    task->p_sp = NULL;
    task->p_stack = task_runtime_stack_low_get();
    task->p_snapshot = NULL;
    task->snapshot_words = 0u;
    task->snapshot_capacity_words = 0u;
    task->state = (uint8_t)TASK_STACK_STATE_SLEEPING;
}

static void srtos_task_runtime_reset(void)
{
    p_srtos_task_ready_first = NULL;
    p_srtos_task_ready_tail = NULL;
    p_srtos_task_unfinished_first = NULL;
    p_srtos_task_unfinished_tail = NULL;
    p_task_current = NULL;
    task_scheduler_started = 0u;
    task_context_pool_head = 0u;
    task_context_pool_tail = 0u;
    task_context_pool_used = 0u;
    task_context_pool_gap_start = 0u;
    task_context_pool_gap_words = 0u;
    task_last_switch_tick = 0u;
    task_fault_active = 0u;
    (void)memset((void *)&g_section_fault_debug, 0, sizeof(g_section_fault_debug));
    g_section_fault_debug.task_fault_policy = SECTION_TASK_CONTEXT_POOL_FULL_POLICY;
}

static uint32_t srtos_task_activate_if_due(reg_task_t *task, uint32_t elapsed)
{
    if ((task == NULL) || (elapsed < task->t_period) || (task->state != (uint8_t)TASK_STACK_STATE_SLEEPING))
    {
        return 0u;
    }

    srtos_task_schedule_next(task, elapsed);
    task->state = (uint8_t)TASK_STACK_STATE_READY_NEW;
    srtos_task_ready_enqueue_unlocked(&p_srtos_task_ready_first, &p_srtos_task_ready_tail, task);

    return 1u;
}

static uint32_t *task_runtime_stack_low_get(void)
{
    return &task_runtime_stack[0];
}

static uint32_t *task_runtime_stack_top_get(void)
{
    return (uint32_t *)((uintptr_t)&task_runtime_stack[SECTION_TASK_RUNTIME_STACK_WORDS] & ~(uintptr_t)0x7u);
}

static uint32_t task_runtime_stack_used_words_get(uint32_t *sp)
{
    const uint32_t *low = task_runtime_stack_low_get();
    const uint32_t *top = task_runtime_stack_top_get();
    uint32_t used_words = 0u;

    if ((sp >= low) && (sp <= top))
    {
        used_words = (uint32_t)(top - sp);
    }

    return used_words;
}

static void task_fault_set(uint32_t reason, const reg_task_t *task, uint32_t *sp, uint32_t required_words)
{
    g_section_fault_debug.task_fault_reason = reason;
    g_section_fault_debug.task_fault_policy = SECTION_TASK_CONTEXT_POOL_FULL_POLICY;
    g_section_fault_debug.task_context_required_words = required_words;
    g_section_fault_debug.task_runtime_stack_used_words = task_runtime_stack_used_words_get(sp);
    g_section_fault_debug.task_sp = (uint32_t)(uintptr_t)sp;
    g_section_fault_debug.task_stack_base = (uint32_t)(uintptr_t)task_runtime_stack_low_get();
    g_section_fault_debug.task_stack_words = SECTION_TASK_RUNTIME_STACK_WORDS;
    g_section_fault_debug.task_stack_free_words = task_stack_free_words_get(task);

    if (task != NULL)
    {
        const uint32_t *frame = task_hw_frame_get(task);

        g_section_fault_debug.task_name = (uint32_t)(uintptr_t)task->p_name;
        g_section_fault_debug.task_frame_valid = task_stack_frame_valid(task);
        if (frame != NULL)
        {
            g_section_fault_debug.task_pc = frame[6u];
            g_section_fault_debug.task_xpsr = frame[7u];
        }
    }

    task_debug_context_pool_update();
    task_fault_active = 1u;
    task_scheduler_started = 0u;
    SECTION_PORT_FAULT_HOOK(reason);
}

static void srtos_task_schedule_next(reg_task_t *task, uint32_t elapsed)
{
    const uint32_t periods_elapsed = elapsed / task->t_period;

    task->time_last += periods_elapsed * task->t_period;
}

static uint32_t task_context_alloc(reg_task_t *task, uint32_t required_words)
{
    uint32_t offset = 0u;
    uint32_t free_words = 0u;

    if (task == NULL)
    {
        return 0u;
    }

    if (required_words == 0u)
    {
        return 1u;
    }

    if (required_words > SECTION_TASK_RUNTIME_STACK_WORDS)
    {
        return 0u;
    }

    if (task->p_snapshot != NULL)
    {
        return (task->snapshot_capacity_words >= required_words) ? 1u : 0u;
    }

    if (task_context_pool_used >= SECTION_TASK_CONTEXT_POOL_WORDS)
    {
        return 0u;
    }

    free_words = SECTION_TASK_CONTEXT_POOL_WORDS - task_context_pool_used;
    if (required_words > free_words)
    {
        return 0u;
    }

    if (task_context_pool_tail >= task_context_pool_head)
    {
        if ((task_context_pool_tail + required_words) <= SECTION_TASK_CONTEXT_POOL_WORDS)
        {
            offset = task_context_pool_tail;
            task_context_pool_tail += required_words;
            if (task_context_pool_tail >= SECTION_TASK_CONTEXT_POOL_WORDS)
            {
                task_context_pool_tail = 0u;
            }
        }
        else
        {
            const uint32_t gap_words = SECTION_TASK_CONTEXT_POOL_WORDS - task_context_pool_tail;

            if ((required_words > task_context_pool_head) ||
                ((task_context_pool_used + gap_words + required_words) > SECTION_TASK_CONTEXT_POOL_WORDS))
            {
                return 0u;
            }

            task_context_pool_gap_start = task_context_pool_tail;
            task_context_pool_gap_words = gap_words;
            task_context_pool_used += gap_words;
            task_context_pool_tail = 0u;

            offset = 0u;
            task_context_pool_tail = required_words;
        }
    }
    else
    {
        if ((task_context_pool_tail + required_words) > task_context_pool_head)
        {
            return 0u;
        }

        offset = task_context_pool_tail;
        task_context_pool_tail += required_words;
    }

    task->p_snapshot = &task_context_pool[offset];
    task->snapshot_capacity_words = required_words;
    task_context_pool_used += required_words;
    task_debug_context_pool_update();

    return 1u;
}

static void task_context_release(reg_task_t *task)
{
    uint32_t offset = 0u;

    if ((task == NULL) || (task->p_snapshot == NULL) || (task->snapshot_capacity_words == 0u))
    {
        return;
    }

    offset = (uint32_t)(task->p_snapshot - task_context_pool);
    task_context_release_gap_if_head();
    if (offset != task_context_pool_head)
    {
        g_section_fault_debug.task_context_release_fail_count++;
        task_fault_set(SECTION_TASK_FAULT_CONTEXT_RELEASE_ORDER, task, task->p_sp, task->snapshot_capacity_words);
        return;
    }

    task_context_pool_head += task->snapshot_capacity_words;
    task_context_pool_used -= task->snapshot_capacity_words;
    if (task_context_pool_head >= SECTION_TASK_CONTEXT_POOL_WORDS)
    {
        task_context_pool_head = 0u;
    }

    task_context_release_gap_if_head();

    task->p_snapshot = NULL;
    task->snapshot_words = 0u;
    task->snapshot_capacity_words = 0u;
    task_debug_context_pool_update();
}

static void task_context_release_gap_if_head(void)
{
    if ((task_context_pool_gap_words != 0u) &&
        (task_context_pool_head == task_context_pool_gap_start))
    {
        task_context_pool_head = 0u;
        task_context_pool_used -= task_context_pool_gap_words;
        task_context_pool_gap_start = 0u;
        task_context_pool_gap_words = 0u;
    }
}

static void task_stack_prepare_initial(reg_task_t *task)
{
    uint32_t *hw_frame = NULL;
    uint32_t *sp = NULL;
    uint32_t *top = task_runtime_stack_top_get();

    if (task == NULL)
    {
        return;
    }

    if (SECTION_TASK_RUNTIME_STACK_WORDS < 33u)
    {
        task_fault_set(SECTION_TASK_FAULT_RUNTIME_STACK_TOO_SMALL, task, NULL, 33u);
        return;
    }

    for (uint32_t i = 0u; i < SECTION_TASK_RUNTIME_STACK_WORDS; ++i)
    {
        task_runtime_stack[i] = TASK_STACK_FILL_WORD;
    }

    hw_frame = &top[-8];
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
    task->p_stack = task_runtime_stack_low_get();
}

static uint32_t task_stack_save(reg_task_t *task, uint32_t *sp)
{
    uint32_t *low = task_runtime_stack_low_get();
    uint32_t *top = task_runtime_stack_top_get();
    uint32_t used_words = 0u;

    if ((task == NULL) || (sp == NULL))
    {
        return 0u;
    }

    if ((sp < low) || (sp > top))
    {
        task_fault_set(SECTION_TASK_FAULT_PSP_OVERFLOW, task, sp, 0u);
        return 0u;
    }

    used_words = (uint32_t)(top - sp);
    if ((used_words == 0u) || (task_context_alloc(task, used_words) == 0u))
    {
        g_section_fault_debug.task_context_save_fail_count++;
        g_section_fault_debug.task_context_required_words = used_words;
        task_debug_context_pool_update();
#if (SECTION_TASK_CONTEXT_POOL_FULL_POLICY == SECTION_TASK_CONTEXT_POOL_FAULT)
        task_fault_set(SECTION_TASK_FAULT_CONTEXT_POOL_FULL, task, sp, used_words);
#endif
        return 0u;
    }

    (void)memcpy(task->p_snapshot, sp, used_words * sizeof(uint32_t));
    task->snapshot_words = used_words;
    task->p_sp = top - used_words;
    task->p_stack = low;

    return 1u;
}

static uint32_t *task_stack_restore(reg_task_t *task)
{
    uint32_t *low = task_runtime_stack_low_get();
    uint32_t *top = task_runtime_stack_top_get();
    uint32_t *sp = NULL;

    if (task == NULL)
    {
        return NULL;
    }

    if (task->state == (uint8_t)TASK_STACK_STATE_READY_NEW)
    {
        task_stack_prepare_initial(task);
        return task->p_sp;
    }

    if ((task->p_snapshot == NULL) ||
        (task->snapshot_words == 0u) ||
        (task->snapshot_words > SECTION_TASK_RUNTIME_STACK_WORDS))
    {
        task_fault_set(SECTION_TASK_FAULT_CONTEXT_RESTORE_OVERFLOW, task, task->p_sp, task->snapshot_words);
        return NULL;
    }

    sp = top - task->snapshot_words;
    if ((sp < low) || (sp > top))
    {
        task_fault_set(SECTION_TASK_FAULT_CONTEXT_RESTORE_OVERFLOW, task, sp, task->snapshot_words);
        return NULL;
    }

    (void)memcpy(sp, task->p_snapshot, task->snapshot_words * sizeof(uint32_t));
    task->p_sp = sp;
    task->p_stack = task_runtime_stack_low_get();
    task_context_release(task);

    return sp;
}

static uint32_t task_stack_frame_valid(const reg_task_t *task)
{
    uint32_t valid = 0u;
    const uint32_t *frame = task_hw_frame_get(task);

    if (frame != NULL)
    {
        const uint32_t pc = frame[6u];
        const uint32_t xpsr = frame[7u];

        if ((pc != 0u) && ((xpsr & TASK_INITIAL_XPSR) == TASK_INITIAL_XPSR))
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

static void task_slice_reset(void)
{
    task_last_switch_tick = SECTION_SYS_TICK;
}

static void task_debug_context_pool_update(void)
{
    g_section_fault_debug.task_context_pool_words = SECTION_TASK_CONTEXT_POOL_WORDS;
    g_section_fault_debug.task_context_pool_used = task_context_pool_used;
    g_section_fault_debug.task_context_pool_head = task_context_pool_head;
    g_section_fault_debug.task_context_pool_tail = task_context_pool_tail;
    g_section_fault_debug.task_fault_policy = SECTION_TASK_CONTEXT_POOL_FULL_POLICY;
}

static uint32_t task_stack_free_words_get(const reg_task_t *task)
{
    uint32_t free_words = 0u;

    if (task == NULL)
    {
        return 0u;
    }

    while ((free_words < SECTION_TASK_RUNTIME_STACK_WORDS) &&
           (task_runtime_stack[free_words] == TASK_STACK_FILL_WORD))
    {
        ++free_words;
    }

    return free_words;
}

static void srtos_task_ready_enqueue_unlocked(reg_task_t **first, reg_task_t **tail, reg_task_t *task)
{
#if (SECTION_TASK_QUEUE_INTERNAL_CRITICAL == 1u)
    uint32_t primask = section_critical_enter();
#endif
    section_race_probe_begin(0x5352454Eu);
    if ((first == NULL) || (tail == NULL) || (task == NULL) || (task->is_ready != 0u))
    {
        section_race_probe_end();
#if (SECTION_TASK_QUEUE_INTERNAL_CRITICAL == 1u)
        section_critical_exit(primask);
#endif
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
#if (SECTION_TASK_QUEUE_INTERNAL_CRITICAL == 1u)
    section_critical_exit(primask);
#endif
}

static reg_task_t *srtos_task_ready_pop_unlocked(reg_task_t **first, reg_task_t **tail)
{
    reg_task_t *task = NULL;
#if (SECTION_TASK_QUEUE_INTERNAL_CRITICAL == 1u)
    uint32_t primask = section_critical_enter();
#endif

    section_race_probe_begin(0x5352504Fu);
    if ((first == NULL) || (tail == NULL) || (*first == NULL))
    {
        section_race_probe_end();
#if (SECTION_TASK_QUEUE_INTERNAL_CRITICAL == 1u)
        section_critical_exit(primask);
#endif
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
#if (SECTION_TASK_QUEUE_INTERNAL_CRITICAL == 1u)
    section_critical_exit(primask);
#endif

    return task;
}

static reg_task_t *task_stack_pick_next(void)
{
    reg_task_t *candidate = NULL;
    static uint32_t ready_pick_count = 0u;

    if ((p_srtos_task_unfinished_first != NULL) &&
        ((p_srtos_task_ready_first == NULL) || (ready_pick_count >= SECTION_TASK_READY_BURST_MAX)))
    {
        candidate = srtos_task_ready_pop_unlocked(&p_srtos_task_unfinished_first, &p_srtos_task_unfinished_tail);
        ready_pick_count = 0u;
    }
    else
    {
        candidate = srtos_task_ready_pop_unlocked(&p_srtos_task_ready_first, &p_srtos_task_ready_tail);
        if (candidate != NULL)
        {
            ready_pick_count++;
        }
        else
        {
            candidate = srtos_task_ready_pop_unlocked(&p_srtos_task_unfinished_first, &p_srtos_task_unfinished_tail);
            ready_pick_count = 0u;
        }
    }

    if (candidate != NULL)
    {
        candidate->is_running = 1u;
    }

    return candidate;
}

uint32_t section_task_scheduler_started(void)
{
    return (uint32_t)task_scheduler_started;
}

uint32_t section_task_switch_pending(void)
{
    uint32_t pending = 0u;

    if ((p_srtos_task_ready_first != NULL) || (p_srtos_task_unfinished_first != NULL))
    {
        pending = 1u;
    }

    return pending;
}

uint32_t section_task_slice_elapsed(void)
{
    const uint32_t now = SECTION_SYS_TICK;
    uint32_t elapsed = 0u;

    if ((uint32_t)(now - task_last_switch_tick) >= SECTION_TASK_SLICE_TICKS)
    {
        elapsed = 1u;
    }

    return elapsed;
}

void section_task_irq_exit_request(void)
{
    if ((section_task_scheduler_started() != 0u) && /* The shared-stack scheduler owns task context. */
        (section_task_slice_elapsed() != 0u))       /* The running task has consumed its current time slice. */
    {
        SECTION_PORT_CONTEXT_SWITCH_REQUEST(); /* Defer the PendSV switch until exception return. */
    }
}

void section_task_start_request(void)
{
    if (task_fault_active != 0u)
    {
        return;
    }

    task_scheduler_started = 1u;
    SECTION_PORT_FPU_LAZY_STACKING_DISABLE();
    SECTION_PORT_CONTEXT_SWITCH_REQUEST();
}

void section_task_yield(void)
{
    if ((task_scheduler_started != 0u) && (task_fault_active == 0u))
    {
        SECTION_PORT_CONTEXT_SWITCH_REQUEST();
    }
}

void section_task_complete_current(void)
{
    if (p_task_current != NULL)
    {
        p_task_current->state = (uint8_t)TASK_STACK_STATE_SLEEPING;
        p_task_current->is_running = 0u;
    }
}

static void section_task_continue_current(void)
{
    if (p_task_current != NULL)
    {
        p_task_current->state = (uint8_t)TASK_STACK_STATE_READY_NEW;
        p_task_current->is_running = 0u;
        reg_task_t **first = &p_srtos_task_unfinished_first;
        reg_task_t **tail = &p_srtos_task_unfinished_tail;

        srtos_task_ready_enqueue_unlocked(first, tail, p_task_current);
    }
}

uint32_t *section_task_start_sp_get(void)
{
    reg_task_t *next = NULL;
    uint32_t *next_sp = NULL;

    if (task_fault_active != 0u)
    {
        return NULL;
    }

    section_task_tick();
    next = task_stack_pick_next();
    if (next == NULL)
    {
        return NULL;
    }

    next_sp = task_stack_restore(next);
    if (next_sp == NULL)
    {
        return NULL;
    }

    next->state = (uint8_t)TASK_STACK_STATE_RUNNING;
    p_task_current = next;
    task_scheduler_started = 1u;
    task_slice_reset();
    return next_sp;
}

uint32_t *section_task_switch_sp(uint32_t *sp)
{
    reg_task_t *next = NULL;
    uint32_t *next_sp = NULL;
    uint32_t has_switch_target = 0u;

    if (task_scheduler_started == 0u)
    {
        return sp;
    }

    if (task_fault_active != 0u)
    {
        return sp;
    }

    section_task_tick();

    if ((p_srtos_task_ready_first != NULL) || (p_srtos_task_unfinished_first != NULL))
    {
        has_switch_target = 1u;
    }

    if (has_switch_target == 0u)
    {
        if ((sp != NULL) && (p_task_current != NULL))
        {
            if ((sp < task_runtime_stack_low_get()) || (sp > task_runtime_stack_top_get()))
            {
                task_fault_set(SECTION_TASK_FAULT_PSP_OVERFLOW, p_task_current, sp, 0u);
                return sp;
            }
            p_task_current->p_sp = sp;
        }
        return sp;
    }

    if ((sp != NULL) && (p_task_current != NULL))
    {
        if (p_task_current->state == (uint8_t)TASK_STACK_STATE_RUNNING)
        {
            if (task_stack_save(p_task_current, sp) == 0u)
            {
                return sp;
            }
            p_task_current->state = (uint8_t)TASK_STACK_STATE_READY_OLD;
            p_task_current->is_running = 0u;
            reg_task_t **first = &p_srtos_task_unfinished_first;
            reg_task_t **tail = &p_srtos_task_unfinished_tail;

            srtos_task_ready_enqueue_unlocked(first, tail, p_task_current);
        }
        else
        {
            p_task_current->p_sp = sp;
        }
    }

    next = task_stack_pick_next();
    if (next != NULL)
    {
        const uint32_t *frame = NULL;

        next_sp = task_stack_restore(next);
        if (next_sp == NULL)
        {
            return sp;
        }

        next->state = (uint8_t)TASK_STACK_STATE_RUNNING;
        frame = task_hw_frame_get(next);
        p_task_current = next;
        g_section_fault_debug.task_sp = (uint32_t)(uintptr_t)next_sp;
        if (frame != NULL)
        {
            g_section_fault_debug.task_pc = frame[6u];
            g_section_fault_debug.task_xpsr = frame[7u];
        }
        g_section_fault_debug.task_stack_base = (uint32_t)(uintptr_t)next->p_stack;
        g_section_fault_debug.task_stack_words = SECTION_TASK_RUNTIME_STACK_WORDS;
        g_section_fault_debug.task_frame_valid = task_stack_frame_valid(next);
        g_section_fault_debug.task_name = (uint32_t)(uintptr_t)next->p_name;
        g_section_fault_debug.task_stack_free_words = task_stack_free_words_get(next);
        task_debug_context_pool_update();
        task_slice_reset();
        return next_sp;
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
        if ((p_srtos_task_ready_first != NULL) || (p_srtos_task_unfinished_first != NULL))
        {
            __ASM volatile("svc 0");
        }
    }
}

static section_task_status_t section_task_run_current(void)
{
    section_task_status_t status = SECTION_TASK_DONE;
    SECTION_TASK_PERF_LOCALS();

    if (p_task_current == NULL)
    {
        return SECTION_TASK_DONE;
    }

    if (p_task_current->p_func != NULL)
    {
        SECTION_TASK_PERF_BEGIN(p_task_current);
        p_task_current->p_func();
        SECTION_TASK_PERF_END();
        status = SECTION_TASK_DONE;
    }
    else if (p_task_current->p_step_func != NULL)
    {
        SECTION_TASK_PERF_BEGIN(p_task_current);
        status = p_task_current->p_step_func(p_task_current->p_ctx);
        SECTION_TASK_PERF_END();
    }
    else
    {
        status = SECTION_TASK_DONE;
    }

    return status;
}

static void section_task_entry(void)
{
    for (;;)
    {
        section_task_status_t status = SECTION_TASK_DONE;

        if ((p_task_current == NULL) || (p_task_current->state != (uint8_t)TASK_STACK_STATE_RUNNING))
        {
            section_task_yield();
            continue;
        }

        status = section_task_run_current();
        if (status == SECTION_TASK_RUNNING)
        {
            section_task_continue_current();
        }
        else
        {
            section_task_complete_current();
        }
        section_task_yield();
    }
}

static void srtos_task_run(void)
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
    p_task->p_ready_next = NULL;
    p_task->is_ready = 0u;
    p_task->is_running = 0u;
    srtos_task_insert_init(p_task);
    SECTION_TASK_PERF_PERIOD_SET(p_task);
    section_list_push_back(&task_list, p_item);
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

    if ((interrupt_list.p_next == NULL) ||
        (p_interrupt->priority < ((reg_interrupt_t *)interrupt_list.p_next->p_obj)->priority))
    {
        section_list_push_front(&interrupt_list, p_item);
    }
    else
    {
        p_prev = interrupt_list.p_next;
        while ((p_prev->p_next != NULL) &&
               (((reg_interrupt_t *)p_prev->p_next->p_obj)->priority < p_interrupt->priority))
        {
            p_prev = p_prev->p_next;
        }
        section_list_insert_after(&interrupt_list, p_prev, p_item);
    }
}

static void link_insert(section_item_t *p_item)
{
    if ((p_item == NULL) || (p_item->p_obj == NULL))
    {
        return;
    }

    section_list_push_back(&link_list, p_item);
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

    if ((init_list.p_next == NULL) ||
        (p_init->priority < ((reg_init_t *)init_list.p_next->p_obj)->priority))
    {
        section_list_push_front(&init_list, p_item);
    }
    else
    {
        p_prev = init_list.p_next;
        while ((p_prev->p_next != NULL) &&
               (((reg_init_t *)p_prev->p_next->p_obj)->priority <= p_init->priority))
        {
            p_prev = p_prev->p_next;
        }
        section_list_insert_after(&init_list, p_prev, p_item);
    }
}

void section_init(void)
{
    task_scheduler_ready = 0u;
    section_list_init(&init_list);
    section_list_init(&task_list);
    section_list_init(&interrupt_list);
    section_list_init(&link_list);

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

    for (section_item_t *p_item = init_list.p_next; p_item != NULL; p_item = p_item->p_next)
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
    section_list_init(&task_list);
    srtos_task_runtime_reset();
    section_list_init(&interrupt_list);
    section_list_init(&link_list);
    section_list_init(&init_list);
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
    (void)srtos_task_activate_if_due(task, elapsed);
    section_critical_exit(primask);
}

void section_task_tick(void)
{
    const uint32_t now = SECTION_SYS_TICK;

    if (task_scheduler_ready == 0u)
    {
        return;
    }

    for (section_item_t *p_item = task_list.p_next; p_item != NULL; p_item = p_item->p_next)
    {
        task_activate_if_due((reg_task_t *)p_item->p_obj, now);
    }
}

void run_task(void)
{
    srtos_task_run();
}

void FUNC_RAM section_interrupt(void)
{
    for (section_item_t *p_item = interrupt_list.p_next; p_item != NULL; p_item = p_item->p_next)
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
    for (section_item_t *p_item = link_list.p_next; p_item != NULL; p_item = p_item->p_next)
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
