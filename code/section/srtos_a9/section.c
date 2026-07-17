// SPDX-License-Identifier: MIT
/**
 * @file    section.c
 * @brief   Cortex-A9 section SRTOS runtime module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Discover section registration records without changing the public REG_* interface
 *          - Run A9 tasks on one shared runtime stack and snapshot suspended contexts
 *          - Select new and suspended tasks while preserving periodic section scheduling
 *          - Dispatch registered initialization, interrupt, communication link, and FSM callbacks
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - IRQ-sensitive queues are protected through the Cortex-A9 CPSR I bit
 *          - SRTOS context copy and task selection execute on the banked SVC stack
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
#include "a9_section_port.h"

#include <stddef.h>
#include <string.h>

#define TASK_A9_CPSR_IRQ_MASK 0x00000080u

static reg_task_t *p_task_first = NULL;
static reg_task_t *p_task_tail = NULL;
static reg_interrupt_t *p_interrupt_first = NULL;
static section_link_t *p_link_first = NULL;
static section_link_t *p_link_tail = NULL;
static reg_init_t *p_init_first = NULL;
static volatile uint8_t task_scheduler_ready = 0u;
volatile section_fault_debug_t g_section_fault_debug;
volatile section_critical_race_debug_t g_section_critical_race_debug;

#if (SECTION_CRITICAL_RACE_PROBE_ENABLE == 1u)
static volatile uint32_t s_section_race_probe_depth = 0u;

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

    depth = s_section_race_probe_depth;
    if (depth != 0u)
    {
        g_section_critical_race_debug.probe_reentry_count++;
    }

    depth++;
    s_section_race_probe_depth = depth;
    if (depth > g_section_critical_race_debug.probe_max_depth)
    {
        g_section_critical_race_debug.probe_max_depth = depth;
    }

    section_race_probe_delay();
}

static void section_race_probe_end(void)
{
    section_race_probe_delay();
    if (s_section_race_probe_depth != 0u)
    {
        s_section_race_probe_depth--;
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
    return SECTION_RUNTIME_SRTOS_A9;
}

const char *section_runtime_name_get(void)
{
    return "srtos-a9";
}

uint32_t section_runtime_preemptive_get(void)
{
    return SECTION_RUNTIME_PREEMPTIVE;
}

void section_port_init(void)
{
    a9_section_port_install_vector_table();
}

typedef enum
{
    TASK_STACK_STATE_SLEEPING = 0,
    TASK_STACK_STATE_READY_NEW,
    TASK_STACK_STATE_READY_OLD,
    TASK_STACK_STATE_RUNNING,
} task_stack_state_t;

#define TASK_STACK_FILL_WORD 0xA5A5A5A5u
#define TASK_INITIAL_STATUS 0x0000001Fu
#define TASK_A9_CONTEXT_WORDS 82u
#define TASK_A9_FPEXC_INDEX 0u
#define TASK_A9_RETURN_PC_INDEX 80u
#define TASK_A9_RETURN_STATUS_INDEX 81u
#define TASK_FRAME_PC_INDEX 0u
#define TASK_FRAME_STATUS_INDEX 1u
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
static uint32_t s_task_runtime_stack[SECTION_TASK_RUNTIME_STACK_WORDS] SECTION_TASK_STACK_ATTR;
static uint32_t s_task_context_pool[SECTION_TASK_CONTEXT_POOL_WORDS] SECTION_TASK_STACK_ATTR;
static uint32_t s_task_context_pool_head = 0u;
static uint32_t s_task_context_pool_tail = 0u;
static uint32_t s_task_context_pool_used = 0u;
static uint32_t s_task_context_pool_gap_start = 0u;
static uint32_t s_task_context_pool_gap_words = 0u;
static uint32_t s_task_last_switch_tick = 0u;
static uint8_t s_task_fault_active = 0u;

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
    s_task_context_pool_head = 0u;
    s_task_context_pool_tail = 0u;
    s_task_context_pool_used = 0u;
    s_task_context_pool_gap_start = 0u;
    s_task_context_pool_gap_words = 0u;
    s_task_last_switch_tick = 0u;
    s_task_fault_active = 0u;
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
    return &s_task_runtime_stack[0];
}

static uint32_t *task_runtime_stack_top_get(void)
{
    return (uint32_t *)((uintptr_t)&s_task_runtime_stack[SECTION_TASK_RUNTIME_STACK_WORDS] & ~(uintptr_t)0x7u);
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
            g_section_fault_debug.task_pc = frame[TASK_FRAME_PC_INDEX];
            g_section_fault_debug.task_xpsr = frame[TASK_FRAME_STATUS_INDEX];
        }
    }

    task_debug_context_pool_update();
    s_task_fault_active = 1u;
    task_scheduler_started = 0u;
    a9_section_port_fault(reason);
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

    if (s_task_context_pool_used >= SECTION_TASK_CONTEXT_POOL_WORDS)
    {
        return 0u;
    }

    free_words = SECTION_TASK_CONTEXT_POOL_WORDS - s_task_context_pool_used;
    if (required_words > free_words)
    {
        return 0u;
    }

    if (s_task_context_pool_tail >= s_task_context_pool_head)
    {
        if ((s_task_context_pool_tail + required_words) <= SECTION_TASK_CONTEXT_POOL_WORDS)
        {
            offset = s_task_context_pool_tail;
            s_task_context_pool_tail += required_words;
            if (s_task_context_pool_tail >= SECTION_TASK_CONTEXT_POOL_WORDS)
            {
                s_task_context_pool_tail = 0u;
            }
        }
        else
        {
            const uint32_t gap_words = SECTION_TASK_CONTEXT_POOL_WORDS - s_task_context_pool_tail;

            if ((required_words > s_task_context_pool_head) ||
                ((s_task_context_pool_used + gap_words + required_words) > SECTION_TASK_CONTEXT_POOL_WORDS))
            {
                return 0u;
            }

            s_task_context_pool_gap_start = s_task_context_pool_tail;
            s_task_context_pool_gap_words = gap_words;
            s_task_context_pool_used += gap_words;
            s_task_context_pool_tail = 0u;

            offset = 0u;
            s_task_context_pool_tail = required_words;
        }
    }
    else
    {
        if ((s_task_context_pool_tail + required_words) > s_task_context_pool_head)
        {
            return 0u;
        }

        offset = s_task_context_pool_tail;
        s_task_context_pool_tail += required_words;
    }

    task->p_snapshot = &s_task_context_pool[offset];
    task->snapshot_capacity_words = required_words;
    s_task_context_pool_used += required_words;
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

    offset = (uint32_t)(task->p_snapshot - s_task_context_pool);
    task_context_release_gap_if_head();
    if (offset != s_task_context_pool_head)
    {
        g_section_fault_debug.task_context_release_fail_count++;
        task_fault_set(SECTION_TASK_FAULT_CONTEXT_RELEASE_ORDER, task, task->p_sp, task->snapshot_capacity_words);
        return;
    }

    s_task_context_pool_head += task->snapshot_capacity_words;
    s_task_context_pool_used -= task->snapshot_capacity_words;
    if (s_task_context_pool_head >= SECTION_TASK_CONTEXT_POOL_WORDS)
    {
        s_task_context_pool_head = 0u;
    }

    task_context_release_gap_if_head();

    task->p_snapshot = NULL;
    task->snapshot_words = 0u;
    task->snapshot_capacity_words = 0u;
    task_debug_context_pool_update();
}

static void task_context_release_gap_if_head(void)
{
    if ((s_task_context_pool_gap_words != 0u) &&
        (s_task_context_pool_head == s_task_context_pool_gap_start))
    {
        s_task_context_pool_head = 0u;
        s_task_context_pool_used -= s_task_context_pool_gap_words;
        s_task_context_pool_gap_start = 0u;
        s_task_context_pool_gap_words = 0u;
    }
}

static void task_stack_prepare_initial(reg_task_t *task)
{
    uint32_t *sp = NULL;
    uint32_t *top = task_runtime_stack_top_get();

    if (task == NULL)
    {
        return;
    }

    if (SECTION_TASK_RUNTIME_STACK_WORDS < TASK_A9_CONTEXT_WORDS)
    {
        task_fault_set(SECTION_TASK_FAULT_RUNTIME_STACK_TOO_SMALL,
                       task,
                       NULL,
        TASK_A9_CONTEXT_WORDS);
        return;
    }

    for (uint32_t i = 0u; i < SECTION_TASK_RUNTIME_STACK_WORDS; ++i)
    {
        s_task_runtime_stack[i] = TASK_STACK_FILL_WORD;
    }

    sp = &top[-(int32_t)TASK_A9_CONTEXT_WORDS];
    for (uint32_t i = 0u; i < TASK_A9_CONTEXT_WORDS; ++i)
    {
        sp[i] = 0u;
    }

    sp[TASK_A9_FPEXC_INDEX] = 0x40000000u;
    sp[TASK_A9_RETURN_PC_INDEX] = (uint32_t)(uintptr_t)section_task_entry;
    sp[TASK_A9_RETURN_STATUS_INDEX] = TASK_INITIAL_STATUS;

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
        const uint32_t pc = frame[TASK_FRAME_PC_INDEX];
        const uint32_t xpsr = frame[TASK_FRAME_STATUS_INDEX];

        if ((pc != 0u) && ((xpsr & 0x1Fu) == TASK_INITIAL_STATUS))
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

    frame = &task->p_sp[TASK_A9_RETURN_PC_INDEX];

    return frame;
}

static void task_slice_reset(void)
{
    s_task_last_switch_tick = SECTION_SYS_TICK;
}

static void task_debug_context_pool_update(void)
{
    g_section_fault_debug.task_context_pool_words = SECTION_TASK_CONTEXT_POOL_WORDS;
    g_section_fault_debug.task_context_pool_used = s_task_context_pool_used;
    g_section_fault_debug.task_context_pool_head = s_task_context_pool_head;
    g_section_fault_debug.task_context_pool_tail = s_task_context_pool_tail;
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
           (s_task_runtime_stack[free_words] == TASK_STACK_FILL_WORD))
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

    if ((uint32_t)(now - s_task_last_switch_tick) >= SECTION_TASK_SLICE_TICKS)
    {
        elapsed = 1u;
    }

    return elapsed;
}

void section_task_irq_exit_request(void)
{
    if ((section_task_scheduler_started() != 0u) && /* A9 共享栈调度器已经接管任务现场。 */
        (section_task_slice_elapsed() != 0u))       /* 当前任务已经耗尽本次时间片。 */
    {
        a9_section_port_switch_request(); /* 请求自定义 IRQ 返回路径切换任务。 */
    }
}

void section_task_start_request(void)
{
    if (s_task_fault_active != 0u)
    {
        return;
    }

    task_scheduler_started = 1u;
    a9_section_port_yield();
}

void section_task_yield(void)
{
    if ((task_scheduler_started != 0u) && (s_task_fault_active == 0u))
    {
        a9_section_port_yield();
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

    if (s_task_fault_active != 0u)
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

    if (s_task_fault_active != 0u)
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
            g_section_fault_debug.task_pc = frame[TASK_FRAME_PC_INDEX];
            g_section_fault_debug.task_xpsr = frame[TASK_FRAME_STATUS_INDEX];
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
            a9_section_port_yield();
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
    uint32_t saved_cpsr = 0u; /* 进入临界区前的 Cortex-A9 CPSR。 */

    __asm volatile("mrs %0, cpsr\n\t"
                   "cpsid i\n\t"
                   "dsb\n\t"
                   "isb"
                   : "=r"(saved_cpsr)
                   :
                   : "memory", "cc");
#if (SECTION_CRITICAL_RACE_PROBE_ENABLE == 1u)
    g_section_critical_race_debug.critical_enter_count++;
#endif
    return saved_cpsr;
}

static void section_critical_exit(uint32_t saved_cpsr)
{
#if (SECTION_CRITICAL_RACE_PROBE_ENABLE == 1u)
    g_section_critical_race_debug.critical_exit_count++;
#endif
    __asm volatile("dsb" ::: "memory");
    if ((saved_cpsr & TASK_A9_CPSR_IRQ_MASK) == 0u)
    {
        __asm volatile("cpsie i" ::: "memory", "cc");
    }
    __asm volatile("isb" ::: "memory");
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
    srtos_task_insert_init(task);
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
    srtos_task_runtime_reset();
    p_interrupt_first = NULL;
    p_link_first = NULL;
    p_link_tail = NULL;
    p_init_first = NULL;
}

const section_link_t *section_link_first_get(void)
{
    return p_link_first;
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

    for (reg_task_t *task = p_task_first; task != NULL; task = task->p_next)
    {
        task_activate_if_due(task, now);
    }
}

void run_task(void)
{
    srtos_task_run();
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
