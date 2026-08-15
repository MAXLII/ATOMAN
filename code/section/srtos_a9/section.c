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
volatile section_scheduler_debug_t g_section_scheduler_debug;

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
static uint32_t task_runtime_stack[SECTION_TASK_RUNTIME_STACK_WORDS] SECTION_TASK_STACK_ATTR;
static uint32_t task_context_pool[SECTION_TASK_CONTEXT_POOL_WORDS] SECTION_TASK_STACK_ATTR;
static uint32_t task_context_pool_head = 0u;
static uint32_t task_context_pool_tail = 0u;
static uint32_t task_context_pool_used = 0u;
static uint32_t task_context_pool_gap_start = 0u;
static uint32_t task_context_pool_gap_words = 0u;
static uint32_t task_last_switch_tick = 0u;
static uint32_t task_ready_pick_burst_count = 0u;
static uint8_t task_fault_active = 0u;

static void srtos_task_ready_enqueue_unlocked(reg_task_t **first, reg_task_t **tail, reg_task_t *task);
static reg_task_t *srtos_task_ready_pop_unlocked(reg_task_t **first, reg_task_t **tail);
static void section_task_entry(void);
static void srtos_task_schedule_next(reg_task_t *task, uint32_t elapsed);
static uint32_t task_context_alloc(reg_task_t *p_task, uint32_t required_words);
static void task_context_release(reg_task_t *p_task);
static void task_context_release_gap_if_head(void);
static uint32_t section_critical_enter(void);
static void section_critical_exit(uint32_t saved_cpsr);
static uint32_t *task_runtime_stack_low_get(void);
static uint32_t *task_runtime_stack_top_get(void);
static void task_stack_prepare_initial(reg_task_t *p_task);
static uint32_t task_stack_save(reg_task_t *p_task, uint32_t *p_sp);
static uint32_t *task_stack_restore(reg_task_t *p_task);
static uint32_t task_stack_frame_valid(const reg_task_t *p_task);
static uint32_t task_stack_free_words_get(const reg_task_t *p_task);
static const uint32_t *task_hw_frame_get(const reg_task_t *p_task);
static reg_task_t *task_stack_pick_next(void);
static void task_slice_reset(void);
static void task_debug_context_pool_update(void);
static void task_debug_stack_min_update(const reg_task_t *p_task);
static uint32_t task_context_pool_invariant_valid(void);
static uint32_t task_runtime_stack_used_words_get(uint32_t *p_sp);
static void task_fault_set(uint32_t reason,
                           const reg_task_t *p_task,
                           uint32_t *p_sp,
                           uint32_t required_words);
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
    task_ready_pick_burst_count = 0u;
    task_fault_active = 0u;
    (void)memset((void *)&g_section_fault_debug, 0, sizeof(g_section_fault_debug));
    (void)memset((void *)&g_section_scheduler_debug, 0, sizeof(g_section_scheduler_debug));
    g_section_fault_debug.task_fault_policy = SECTION_TASK_CONTEXT_POOL_FULL_POLICY;
    g_section_scheduler_debug.runtime_stack_min_free_words = SECTION_TASK_RUNTIME_STACK_WORDS;
    task_debug_context_pool_update();
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

static uint32_t task_runtime_stack_used_words_get(uint32_t *p_sp)
{
    const uintptr_t stack_low = (uintptr_t)task_runtime_stack_low_get(); /* Inclusive shared-stack lower address. */
    const uintptr_t stack_top = (uintptr_t)task_runtime_stack_top_get(); /* Exclusive shared-stack upper address. */
    const uintptr_t stack_pointer = (uintptr_t)p_sp; /* Stack pointer being measured. */
    uint32_t used_words = 0u;

    if ((stack_pointer >= stack_low) && /* Stack pointer is not below the shared stack. */
        (stack_pointer <= stack_top))   /* Stack pointer is not above the aligned stack top. */
    {
        used_words = (uint32_t)((stack_top - stack_pointer) / sizeof(uint32_t));
    }

    return used_words;
}

static void task_fault_set(uint32_t reason,
                           const reg_task_t *p_task,
                           uint32_t *p_sp,
                           uint32_t required_words)
{
    g_section_fault_debug.task_fault_reason = reason;
    g_section_fault_debug.task_fault_policy = SECTION_TASK_CONTEXT_POOL_FULL_POLICY;
    g_section_fault_debug.task_context_required_words = required_words;
    g_section_fault_debug.task_runtime_stack_used_words = task_runtime_stack_used_words_get(p_sp);
    g_section_fault_debug.task_sp = (uint32_t)(uintptr_t)p_sp;
    g_section_fault_debug.task_stack_base = (uint32_t)(uintptr_t)task_runtime_stack_low_get();
    g_section_fault_debug.task_stack_words = SECTION_TASK_RUNTIME_STACK_WORDS;
    g_section_fault_debug.task_stack_free_words = task_stack_free_words_get(p_task);

    if (p_task != NULL)
    {
        const uint32_t *p_frame = task_hw_frame_get(p_task); /* Return frame associated with the failing task. */

        g_section_fault_debug.task_name = (uint32_t)(uintptr_t)p_task->p_name;
        g_section_fault_debug.task_frame_valid = task_stack_frame_valid(p_task);
        if (p_frame != NULL)
        {
            g_section_fault_debug.task_pc = p_frame[TASK_FRAME_PC_INDEX];
            g_section_fault_debug.task_xpsr = p_frame[TASK_FRAME_STATUS_INDEX];
        }
    }

    task_debug_context_pool_update();
    task_fault_active = 1u;
    task_scheduler_started = 0u;
    a9_section_port_fault(reason);
}

static void srtos_task_schedule_next(reg_task_t *task, uint32_t elapsed)
{
    const uint32_t periods_elapsed = elapsed / task->t_period;

    task->time_last += periods_elapsed * task->t_period;
}

static uint32_t task_context_alloc(reg_task_t *p_task, uint32_t required_words)
{
    uint32_t offset = 0u;     /* First context-pool word reserved for this snapshot. */
    uint32_t free_words = 0u; /* Unreserved words, including space on both sides of a wrap. */

    if (p_task == NULL)
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

    if (p_task->p_snapshot != NULL)
    {
        return (p_task->snapshot_capacity_words >= required_words) ? 1u : 0u;
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
                g_section_scheduler_debug.context_pool_wrap_count++;
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
            g_section_scheduler_debug.context_pool_wrap_count++;

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

    p_task->p_snapshot = &task_context_pool[offset];
    p_task->snapshot_capacity_words = required_words;
    task_context_pool_used += required_words;
    g_section_scheduler_debug.context_alloc_count++;
    task_debug_context_pool_update();
    if (task_context_pool_invariant_valid() == 0u)
    {
        g_section_scheduler_debug.invariant_fail_count++;
        task_fault_set(SECTION_TASK_FAULT_CONTEXT_POOL_CORRUPT, p_task, p_task->p_sp, required_words);
        return 0u;
    }

    return 1u;
}

static void task_context_release(reg_task_t *p_task)
{
    const uintptr_t pool_start = (uintptr_t)&task_context_pool[0]; /* Inclusive context-pool lower address. */
    const uintptr_t pool_end = (uintptr_t)&task_context_pool[SECTION_TASK_CONTEXT_POOL_WORDS];
    uintptr_t snapshot_address = 0u; /* Integer form used to validate a potentially damaged snapshot pointer. */
    uint32_t offset = 0u;            /* Snapshot offset from the context-pool base, in words. */

    if ((p_task == NULL) ||             /* No task owns a snapshot. */
        (p_task->p_snapshot == NULL) ||  /* The task has no allocated context image. */
        (p_task->snapshot_capacity_words == 0u)) /* A zero-capacity allocation cannot be released. */
    {
        return;
    }

    snapshot_address = (uintptr_t)p_task->p_snapshot;
    if ((snapshot_address < pool_start) || /* Snapshot begins below the context pool. */
        (snapshot_address >= pool_end) ||  /* Snapshot begins at or above the pool end. */
        (((snapshot_address - pool_start) % sizeof(uint32_t)) != 0u)) /* Snapshot is not word aligned. */
    {
        g_section_fault_debug.task_context_release_fail_count++;
        g_section_scheduler_debug.invariant_fail_count++;
        task_fault_set(SECTION_TASK_FAULT_CONTEXT_POOL_CORRUPT,
                       p_task,
                       p_task->p_sp,
                       p_task->snapshot_capacity_words);
        return;
    }

    offset = (uint32_t)((snapshot_address - pool_start) / sizeof(uint32_t));
    if ((p_task->snapshot_capacity_words > SECTION_TASK_CONTEXT_POOL_WORDS) ||
        (offset > (SECTION_TASK_CONTEXT_POOL_WORDS - p_task->snapshot_capacity_words)))
    {
        g_section_fault_debug.task_context_release_fail_count++;
        g_section_scheduler_debug.invariant_fail_count++;
        task_fault_set(SECTION_TASK_FAULT_CONTEXT_POOL_CORRUPT,
                       p_task,
                       p_task->p_sp,
                       p_task->snapshot_capacity_words);
        return;
    }

    task_context_release_gap_if_head();
    if (offset != task_context_pool_head)
    {
        g_section_fault_debug.task_context_release_fail_count++;
        task_fault_set(SECTION_TASK_FAULT_CONTEXT_RELEASE_ORDER,
                       p_task,
                       p_task->p_sp,
                       p_task->snapshot_capacity_words);
        return;
    }

    if (p_task->snapshot_capacity_words > task_context_pool_used)
    {
        g_section_fault_debug.task_context_release_fail_count++;
        g_section_scheduler_debug.invariant_fail_count++;
        task_fault_set(SECTION_TASK_FAULT_CONTEXT_POOL_CORRUPT,
                       p_task,
                       p_task->p_sp,
                       p_task->snapshot_capacity_words);
        return;
    }

    task_context_pool_head += p_task->snapshot_capacity_words;
    task_context_pool_used -= p_task->snapshot_capacity_words;
    if (task_context_pool_head >= SECTION_TASK_CONTEXT_POOL_WORDS)
    {
        task_context_pool_head = 0u;
    }

    task_context_release_gap_if_head();

    p_task->p_snapshot = NULL;
    p_task->snapshot_words = 0u;
    p_task->snapshot_capacity_words = 0u;
    g_section_scheduler_debug.context_release_count++;
    task_debug_context_pool_update();
    if (task_context_pool_invariant_valid() == 0u)
    {
        g_section_scheduler_debug.invariant_fail_count++;
        task_fault_set(SECTION_TASK_FAULT_CONTEXT_POOL_CORRUPT, p_task, p_task->p_sp, 0u);
    }
}

static void task_context_release_gap_if_head(void)
{
    if ((task_context_pool_gap_words != 0u) &&
        (task_context_pool_head == task_context_pool_gap_start))
    {
        if (task_context_pool_gap_words > task_context_pool_used)
        {
            g_section_scheduler_debug.invariant_fail_count++;
            task_fault_set(SECTION_TASK_FAULT_CONTEXT_POOL_CORRUPT,
                           NULL,
                           NULL,
                           task_context_pool_gap_words);
            return;
        }

        task_context_pool_head = 0u;
        task_context_pool_used -= task_context_pool_gap_words;
        task_context_pool_gap_start = 0u;
        task_context_pool_gap_words = 0u;
    }
}

static uint32_t task_context_pool_invariant_valid(void)
{
    uint32_t valid = 1u; /* Aggregated validity of context-pool indexes and accounting. */

    if ((task_context_pool_head >= SECTION_TASK_CONTEXT_POOL_WORDS) ||
        (task_context_pool_tail >= SECTION_TASK_CONTEXT_POOL_WORDS) ||
        (task_context_pool_used > SECTION_TASK_CONTEXT_POOL_WORDS))
    {
        valid = 0u;
    }

    if (task_context_pool_gap_words == 0u)
    {
        if (task_context_pool_gap_start != 0u)
        {
            valid = 0u;
        }
    }
    else if ((task_context_pool_gap_start >= SECTION_TASK_CONTEXT_POOL_WORDS) ||
             (task_context_pool_gap_words > task_context_pool_used) ||
             ((task_context_pool_gap_start + task_context_pool_gap_words) !=
              SECTION_TASK_CONTEXT_POOL_WORDS) ||
             (task_context_pool_tail > task_context_pool_head) ||
             ((task_context_pool_tail == task_context_pool_head) &&
              (task_context_pool_used != SECTION_TASK_CONTEXT_POOL_WORDS)))
    {
        valid = 0u;
    }
    else
    {
    }

    return valid;
}

static void task_stack_prepare_initial(reg_task_t *p_task)
{
    uint32_t *p_sp = NULL;                              /* Initial A9 context image on the shared stack. */
    uint32_t *p_stack_top = task_runtime_stack_top_get(); /* Aligned upper boundary of the shared stack. */

    if (p_task == NULL)
    {
        return;
    }

    if (SECTION_TASK_RUNTIME_STACK_WORDS < TASK_A9_CONTEXT_WORDS)
    {
        task_fault_set(SECTION_TASK_FAULT_RUNTIME_STACK_TOO_SMALL,
                       p_task,
                       NULL,
                       TASK_A9_CONTEXT_WORDS);
        return;
    }

    p_sp = &p_stack_top[-(int32_t)TASK_A9_CONTEXT_WORDS];
    for (uint32_t i = 0u; i < TASK_A9_CONTEXT_WORDS; ++i)
    {
        p_sp[i] = 0u;
    }

    p_sp[TASK_A9_FPEXC_INDEX] = 0x40000000u;
    p_sp[TASK_A9_RETURN_PC_INDEX] = (uint32_t)(uintptr_t)section_task_entry;
    p_sp[TASK_A9_RETURN_STATUS_INDEX] = TASK_INITIAL_STATUS;

    p_task->p_sp = p_sp;
    p_task->p_stack = task_runtime_stack_low_get();
    task_debug_stack_min_update(p_task);
}

static uint32_t task_stack_save(reg_task_t *p_task, uint32_t *p_sp)
{
    uint32_t *p_stack_top = task_runtime_stack_top_get(); /* Aligned upper boundary used for snapshot sizing. */
    const uintptr_t stack_low = (uintptr_t)task_runtime_stack_low_get(); /* Inclusive shared-stack lower address. */
    const uintptr_t stack_top = (uintptr_t)p_stack_top; /* Exclusive shared-stack upper address. */
    const uintptr_t stack_pointer = (uintptr_t)p_sp;    /* Context stack pointer supplied by the A9 port. */
    uint32_t used_words = 0u;                           /* Context image size copied into the pool. */

    if ((p_task == NULL) || /* No task owns the supplied context. */
        (p_sp == NULL))     /* The architecture port did not supply a stack pointer. */
    {
        return 0u;
    }

    if ((stack_pointer < stack_low) || /* Context begins below the shared stack. */
        (stack_pointer > stack_top) || /* Context begins above the shared stack. */
        (((stack_top - stack_pointer) % sizeof(uint32_t)) != 0u)) /* Context is not word aligned. */
    {
        task_fault_set(SECTION_TASK_FAULT_PSP_OVERFLOW, p_task, p_sp, 0u);
        return 0u;
    }

    used_words = (uint32_t)((stack_top - stack_pointer) / sizeof(uint32_t));
    if ((used_words == 0u) || /* A valid saved A9 context always contains the fixed exception frame. */
        (task_context_alloc(p_task, used_words) == 0u)) /* The shared context pool cannot hold the snapshot. */
    {
        g_section_fault_debug.task_context_save_fail_count++;
        g_section_fault_debug.task_context_required_words = used_words;
        task_debug_context_pool_update();
#if (SECTION_TASK_CONTEXT_POOL_FULL_POLICY == SECTION_TASK_CONTEXT_POOL_FAULT)
        task_fault_set(SECTION_TASK_FAULT_CONTEXT_POOL_FULL, p_task, p_sp, used_words);
#endif
        return 0u;
    }

    (void)memcpy(p_task->p_snapshot, p_sp, used_words * sizeof(uint32_t));
    p_task->snapshot_words = used_words;
    p_task->p_sp = p_stack_top - used_words;
    p_task->p_stack = task_runtime_stack_low_get();
    g_section_scheduler_debug.context_save_count++;
    task_debug_stack_min_update(p_task);

    return 1u;
}

static uint32_t *task_stack_restore(reg_task_t *p_task)
{
    uint32_t *p_stack_low = task_runtime_stack_low_get(); /* Inclusive shared-stack lower boundary. */
    uint32_t *p_stack_top = task_runtime_stack_top_get(); /* Aligned shared-stack upper boundary. */
    uint32_t *p_sp = NULL;                                /* Context stack pointer returned to the A9 port. */

    if (p_task == NULL)
    {
        return NULL;
    }

    if (p_task->state == (uint8_t)TASK_STACK_STATE_READY_NEW)
    {
        task_stack_prepare_initial(p_task);
        return p_task->p_sp;
    }

    if ((p_task->p_snapshot == NULL) ||      /* A suspended task must own a pool snapshot. */
        (p_task->snapshot_words == 0u) ||    /* A valid A9 context cannot have zero words. */
        (p_task->snapshot_words > SECTION_TASK_RUNTIME_STACK_WORDS)) /* Snapshot must fit the shared stack. */
    {
        task_fault_set(SECTION_TASK_FAULT_CONTEXT_RESTORE_OVERFLOW,
                       p_task,
                       p_task->p_sp,
                       p_task->snapshot_words);
        return NULL;
    }

    p_sp = p_stack_top - p_task->snapshot_words;
    if ((p_sp < p_stack_low) || /* Restored context would begin below the shared stack. */
        (p_sp > p_stack_top))   /* Restored context would begin above the shared stack. */
    {
        task_fault_set(SECTION_TASK_FAULT_CONTEXT_RESTORE_OVERFLOW,
                       p_task,
                       p_sp,
                       p_task->snapshot_words);
        return NULL;
    }

    (void)memcpy(p_sp, p_task->p_snapshot, p_task->snapshot_words * sizeof(uint32_t));
    p_task->p_sp = p_sp;
    p_task->p_stack = task_runtime_stack_low_get();
    g_section_scheduler_debug.context_restore_count++;
    task_debug_stack_min_update(p_task);
    task_context_release(p_task);

    return p_sp;
}

static uint32_t task_stack_frame_valid(const reg_task_t *p_task)
{
    uint32_t valid = 0u;                                   /* Whether the saved A9 return frame is plausible. */
    const uint32_t *p_frame = task_hw_frame_get(p_task); /* Return PC and CPSR words for the selected task. */

    if (p_frame != NULL)
    {
        const uint32_t pc = p_frame[TASK_FRAME_PC_INDEX];         /* Exception return program counter. */
        const uint32_t cpsr = p_frame[TASK_FRAME_STATUS_INDEX];   /* Exception return processor status. */

        if ((pc != 0u) && /* Return target is not the null address. */
            ((cpsr & 0x1Fu) == TASK_INITIAL_STATUS)) /* Task returns to the expected A9 System mode. */
        {
            valid = 1u;
        }
    }

    return valid;
}

static const uint32_t *task_hw_frame_get(const reg_task_t *p_task)
{
    const uintptr_t pool_start = (uintptr_t)&task_context_pool[0]; /* Inclusive context-pool lower address. */
    const uintptr_t pool_end = (uintptr_t)&task_context_pool[SECTION_TASK_CONTEXT_POOL_WORDS];
    const uintptr_t stack_low = (uintptr_t)task_runtime_stack_low_get(); /* Inclusive shared-stack lower address. */
    const uintptr_t stack_top = (uintptr_t)task_runtime_stack_top_get(); /* Exclusive shared-stack upper address. */
    uintptr_t context_address = 0u; /* Integer form of the candidate task context address. */

    if (p_task == NULL)
    {
        return NULL;
    }

    if ((p_task->p_snapshot != NULL) && /* Suspended task context currently resides in the pool. */
        (p_task->snapshot_words >= TASK_A9_CONTEXT_WORDS)) /* Snapshot contains the fixed A9 return frame. */
    {
        context_address = (uintptr_t)p_task->p_snapshot;
        if ((context_address >= pool_start) && /* Snapshot begins within the context pool. */
            (context_address <= (pool_end - (TASK_A9_CONTEXT_WORDS * sizeof(uint32_t)))))
        {
            return &p_task->p_snapshot[TASK_A9_RETURN_PC_INDEX];
        }
    }

    if (p_task->p_sp == NULL)
    {
        return NULL;
    }

    context_address = (uintptr_t)p_task->p_sp;
    if ((context_address < stack_low) || /* Context begins below the shared stack. */
        (context_address > (stack_top - (TASK_A9_CONTEXT_WORDS * sizeof(uint32_t)))))
    {
        return NULL;
    }

    return &p_task->p_sp[TASK_A9_RETURN_PC_INDEX];
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
    if (task_context_pool_used > g_section_scheduler_debug.context_pool_high_water_words)
    {
        g_section_scheduler_debug.context_pool_high_water_words = task_context_pool_used;
    }
}

static void task_debug_stack_min_update(const reg_task_t *p_task)
{
    const uint32_t free_words = task_stack_free_words_get(p_task);
    /* Conservative free-stack estimate for the selected task. */

    if (free_words < g_section_scheduler_debug.runtime_stack_min_free_words)
    {
        g_section_scheduler_debug.runtime_stack_min_free_words = free_words;
        g_section_scheduler_debug.runtime_stack_peak_used_words = SECTION_TASK_RUNTIME_STACK_WORDS - free_words;
        g_section_scheduler_debug.runtime_stack_peak_task_name = (uint32_t)(uintptr_t)p_task->p_name;
    }
}

static uint32_t task_stack_free_words_get(const reg_task_t *p_task)
{
    const uintptr_t stack_low = (uintptr_t)task_runtime_stack_low_get(); /* Inclusive shared-stack lower address. */
    const uintptr_t stack_top = (uintptr_t)task_runtime_stack_top_get(); /* Exclusive shared-stack upper address. */
    uintptr_t stack_pointer = 0u; /* Integer form of the task's most recently captured stack pointer. */

    if (p_task == NULL)
    {
        return 0u;
    }

    if ((p_task->p_snapshot != NULL) && /* A suspended task has an exact copied context size. */
        (p_task->snapshot_words <= SECTION_TASK_RUNTIME_STACK_WORDS)) /* Snapshot size is safe to subtract. */
    {
        return SECTION_TASK_RUNTIME_STACK_WORDS - p_task->snapshot_words;
    }

    if (p_task->p_sp == NULL)
    {
        return 0u;
    }

    stack_pointer = (uintptr_t)p_task->p_sp;
    if ((stack_pointer < stack_low) || /* Captured stack pointer is below the shared stack. */
        (stack_pointer > stack_top))   /* Captured stack pointer is above the shared stack. */
    {
        return 0u;
    }

    return (uint32_t)((stack_pointer - stack_low) / sizeof(uint32_t));
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
    reg_task_t *p_candidate = NULL; /* Runnable task selected from one of the 2 scheduler queues. */

    if ((p_srtos_task_unfinished_first != NULL) &&
        ((p_srtos_task_ready_first == NULL) ||
         (task_ready_pick_burst_count >= SECTION_TASK_READY_BURST_MAX)))
    {
        p_candidate = srtos_task_ready_pop_unlocked(&p_srtos_task_unfinished_first,
                                                    &p_srtos_task_unfinished_tail);
        task_ready_pick_burst_count = 0u;
        if (p_candidate != NULL)
        {
            g_section_scheduler_debug.unfinished_task_pick_count++;
        }
    }
    else
    {
        p_candidate = srtos_task_ready_pop_unlocked(&p_srtos_task_ready_first, &p_srtos_task_ready_tail);
        if (p_candidate != NULL)
        {
            task_ready_pick_burst_count++;
            g_section_scheduler_debug.ready_task_pick_count++;
        }
        else
        {
            p_candidate = srtos_task_ready_pop_unlocked(&p_srtos_task_unfinished_first,
                                                        &p_srtos_task_unfinished_tail);
            task_ready_pick_burst_count = 0u;
            if (p_candidate != NULL)
            {
                g_section_scheduler_debug.unfinished_task_pick_count++;
            }
        }
    }

    if (p_candidate != NULL)
    {
        p_candidate->is_running = 1u;
    }

    return p_candidate;
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
    if ((section_task_scheduler_started() != 0u) && /* The A9 shared-stack scheduler owns task context. */
        (section_task_slice_elapsed() != 0u))       /* The running task has consumed its current time slice. */
    {
        a9_section_port_switch_request(); /* Defer the switch until registered IRQ callbacks have completed. */
    }
}

void section_task_start_request(void)
{
    if (task_fault_active != 0u)
    {
        return;
    }

    task_scheduler_started = 1u;
    a9_section_port_yield();
}

void section_task_yield(void)
{
    if ((task_scheduler_started != 0u) && (task_fault_active == 0u))
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
    reg_task_t *p_next = NULL; /* First runnable task selected during scheduler startup. */
    uint32_t *p_next_sp = NULL; /* Initial A9 context returned to the SVC assembly path. */

    if (task_fault_active != 0u)
    {
        return NULL;
    }

    section_task_tick();
    p_next = task_stack_pick_next();
    if (p_next == NULL)
    {
        return NULL;
    }

    p_next_sp = task_stack_restore(p_next);
    if (p_next_sp == NULL)
    {
        return NULL;
    }

    p_next->state = (uint8_t)TASK_STACK_STATE_RUNNING;
    p_task_current = p_next;
    task_scheduler_started = 1u;
    g_section_scheduler_debug.task_switch_count++;
    task_slice_reset();
    return p_next_sp;
}

uint32_t *section_task_switch_sp(uint32_t *p_sp)
{
    reg_task_t *p_next = NULL; /* Runnable task selected after saving the current context. */
    uint32_t *p_next_sp = NULL; /* Restored A9 context returned to the exception assembly path. */
    uint32_t has_switch_target = 0u; /* Whether either scheduler queue contains runnable work. */

    if (task_scheduler_started == 0u)
    {
        return p_sp;
    }

    if (task_fault_active != 0u)
    {
        return p_sp;
    }

    section_task_tick();

    if ((p_srtos_task_ready_first != NULL) || (p_srtos_task_unfinished_first != NULL))
    {
        has_switch_target = 1u;
    }

    if (has_switch_target == 0u)
    {
        if ((p_sp != NULL) && (p_task_current != NULL))
        {
            const uintptr_t stack_pointer = (uintptr_t)p_sp; /* Context pointer supplied by the exception port. */
            const uintptr_t stack_low = (uintptr_t)task_runtime_stack_low_get(); /* Inclusive stack lower address. */
            const uintptr_t stack_top = (uintptr_t)task_runtime_stack_top_get(); /* Exclusive stack upper address. */

            if ((stack_pointer < stack_low) || /* Context begins below the shared stack. */
                (stack_pointer > stack_top))   /* Context begins above the shared stack. */
            {
                task_fault_set(SECTION_TASK_FAULT_PSP_OVERFLOW, p_task_current, p_sp, 0u);
                return p_sp;
            }
            p_task_current->p_sp = p_sp;
            task_debug_stack_min_update(p_task_current);
        }
        return p_sp;
    }

    if ((p_sp != NULL) && (p_task_current != NULL))
    {
        if (p_task_current->state == (uint8_t)TASK_STACK_STATE_RUNNING)
        {
            if (task_stack_save(p_task_current, p_sp) == 0u)
            {
                return p_sp;
            }
            p_task_current->state = (uint8_t)TASK_STACK_STATE_READY_OLD;
            p_task_current->is_running = 0u;
            reg_task_t **first = &p_srtos_task_unfinished_first;
            reg_task_t **tail = &p_srtos_task_unfinished_tail;

            srtos_task_ready_enqueue_unlocked(first, tail, p_task_current);
        }
        else
        {
            p_task_current->p_sp = p_sp;
        }
    }

    p_next = task_stack_pick_next();
    if (p_next != NULL)
    {
        const uint32_t *p_frame = NULL; /* Return frame used for debugger-readable switch diagnostics. */

        p_next_sp = task_stack_restore(p_next);
        if (p_next_sp == NULL)
        {
            return p_sp;
        }

        p_next->state = (uint8_t)TASK_STACK_STATE_RUNNING;
        p_frame = task_hw_frame_get(p_next);
        p_task_current = p_next;
        g_section_fault_debug.task_sp = (uint32_t)(uintptr_t)p_next_sp;
        if (p_frame != NULL)
        {
            g_section_fault_debug.task_pc = p_frame[TASK_FRAME_PC_INDEX];
            g_section_fault_debug.task_xpsr = p_frame[TASK_FRAME_STATUS_INDEX];
        }
        g_section_fault_debug.task_stack_base = (uint32_t)(uintptr_t)p_next->p_stack;
        g_section_fault_debug.task_stack_words = SECTION_TASK_RUNTIME_STACK_WORDS;
        g_section_fault_debug.task_frame_valid = task_stack_frame_valid(p_next);
        g_section_fault_debug.task_name = (uint32_t)(uintptr_t)p_next->p_name;
        g_section_fault_debug.task_stack_free_words = task_stack_free_words_get(p_next);
        task_debug_stack_min_update(p_next);
        task_debug_context_pool_update();
        g_section_scheduler_debug.task_switch_count++;
        task_slice_reset();
        return p_next_sp;
    }

    if ((p_sp != NULL) && (p_task_current != NULL))
    {
        return p_task_current->p_sp;
    }

    return p_sp;
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
            g_section_scheduler_debug.idle_wait_count++;
            a9_section_port_wait_for_interrupt();
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
    const uint32_t saved_cpsr = a9_section_port_irq_save(); /* IRQ state restored when the critical section exits. */
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
    a9_section_port_irq_restore(saved_cpsr);
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
    p_item->p_next = NULL;
    if (p_task_first == NULL)
    {
        p_task_first = p_item;
    }
    else
    {
        p_task_tail->p_next = p_item;
    }
    p_task_tail = p_item;
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
    }
    else
    {
        p_link_tail->p_next = p_item;
    }
    p_link_tail = p_item;
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
    section_runtime_reset();

    for (const reg_section_t *p_entry = SECTION_REG_FIRST;
         p_entry < SECTION_REG_LAST;
         ++p_entry)
    {
        switch (p_entry->section_type)
        {
        case SECTION_INIT:
            init_insert((section_item_t *)p_entry->p_str);
            break;
        case SECTION_TASK:
            task_insert((section_item_t *)p_entry->p_str);
            break;
        case SECTION_INTERRUPT:
            interrupt_insert((section_item_t *)p_entry->p_str);
            break;
        case SECTION_LINK:
            link_insert((section_item_t *)p_entry->p_str);
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
    srtos_task_runtime_reset();
    p_interrupt_first = NULL;
    p_link_first = NULL;
    p_link_tail = NULL;
    p_init_first = NULL;
}

static void task_activate_if_due(reg_task_t *task, uint32_t now)
{
    uint32_t elapsed = 0u;
    uint32_t saved_cpsr = 0u;

    if ((task == NULL) || ((task->p_func == NULL) && (task->p_step_func == NULL)) || (task->t_period == 0u))
    {
        return;
    }

    elapsed = (uint32_t)(now - task->time_last);
    if (elapsed < task->t_period)
    {
        return;
    }

    saved_cpsr = section_critical_enter();

    elapsed = (uint32_t)(now - task->time_last);
    (void)srtos_task_activate_if_due(task, elapsed);
    section_critical_exit(saved_cpsr);
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

void run_task(void)
{
    srtos_task_run();
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

static void link_process(section_link_t *p_link)
{
    uint8_t data = 0u;                  /* Byte dispatched during the current Link round. */
    uint32_t processed_byte_count = 0u; /* Bytes consumed from this Link during the current round. */
    uint32_t handler_index = 0u;        /* Handler receiving the current byte. */

    if ((p_link == NULL) ||                     /* No Link descriptor is available. */
        (p_link->rx_get_byte == NULL) ||        /* The Link cannot provide received bytes. */
        (p_link->handler_arr == NULL))          /* The Link has no byte consumers. */
    {
        return;
    }

    while (processed_byte_count < SECTION_LINK_RX_BYTE_BUDGET)
    {
        if (p_link->rx_get_byte(&data) == 0u)
        {
            break;
        }

        for (handler_index = 0u; handler_index < p_link->handler_num; ++handler_index)
        {
            const section_link_handler_item_t *p_handler =
                &p_link->handler_arr[handler_index]; /* Handler bound to the current Link. */
            if (p_handler->func != NULL)
            {
                p_handler->func(data, p_link->my_printf, p_handler->ctx);
            }
        }
        processed_byte_count++;
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
