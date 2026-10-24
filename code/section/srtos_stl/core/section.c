// SPDX-License-Identifier: MIT
/**
 * @file section.c
 * @brief Resident single-core task groups, shared-stack snapshots and cancellation.
 * @details Adapted from the srtos_m scheduling algorithm at base 680f1a6.
 *          Entity: resident metadata owns identity and allocation history.
 *          Prior: no business descriptor is read after registration/init returns.
 *          Time: port serializes all state transitions; overwrite requires native cleanup.
 * Copyright (c) 2026 Max.Li. Licensed under the MIT License.
 */
#include "section.h"
#include <string.h>

#define STL_NONE UINT32_MAX /* No snapshot allocation. */
#define STL_HALF UINT32_C(0x80000000) /* Maximum unambiguous tick delta. */
#define STL_GUARD UINT32_C(0x73A54C91) /* Shared stack overflow witness. */
#define STL_GUARD_WORDS 2u /* Excluded from valid execution frames. */

_Static_assert(SECTION_STL_MAX_GROUPS > 0u, "group capacity must be positive");
_Static_assert(SECTION_STL_MAX_TASKS > 0u, "task capacity must be positive");
_Static_assert(SECTION_STL_MAX_RECORDS > 0u, "record capacity must be positive");
_Static_assert(SECTION_STL_POOL_WORDS > 0u, "pool capacity must be positive");
_Static_assert(SECTION_STL_STACK_WORDS >= 32u, "task stack too small");
_Static_assert((SECTION_STL_STACK_WORDS % 2u) == 0u, "stack top must be 8-byte aligned");
_Static_assert(SECTION_STL_SLICE_TICKS > 0u, "slice must be positive");
_Static_assert(SECTION_STL_SLICE_TICKS < STL_HALF, "slice must fit modular time");

typedef enum
{
    TASK_FREE = 0, /* Resident slot available. */
    TASK_SLEEPING, /* Awaiting a periodic release. */
    TASK_NEW, /* Ready callback without saved stack. */
    TASK_OLD, /* Ready continuation with copied stack. */
    TASK_RUNNING /* Currently executing on PSP. */
} TASK_STATE_E;

typedef struct resident_task
{
    section_stl_handle_t owner; /* Copied group identity, never an application node. */
    uint32_t quantum_left; /* Remaining task quantum across native maintenance. */
    uint32_t period; /* Activation interval in ticks. */
    uint32_t last; /* Phase anchor in the single core timebase. */
    void (*p_func)(void); /* Business entry, called only after resident checks. */
    section_task_status_t (*p_step_func)(void *p_ctx); /* Optional one-step callback. */
    void *p_ctx; /* Opaque business argument, never inspected by core. */
    struct resident_task *p_group_next; /* Resident per-group period-sorted list. */
    struct resident_task *p_ready_next; /* Resident global FIFO link. */
    uint32_t record; /* Allocation slot, or STL_NONE. */
    uint32_t queued; /* Normalized FIFO membership. */
    TASK_STATE_E state; /* Resident execution state. */
} resident_task_t;

typedef struct resident_group
{
    section_stl_handle_t handle; /* Incarnation protected from address reuse. */
    struct resident_group *p_next; /* Resident global group list. */
    resident_task_t *p_tasks; /* Resident task list. */
    SECTION_STL_GROUP_STATE_E state; /* Admission and cancellation state. */
    uint32_t task_count; /* Copied descriptors. */
    uint32_t references; /* Published external references, released exactly once. */
    uint32_t init_busy; /* Synchronous init still holds business function pointers. */
    uint32_t initialized; /* Initialization completed successfully. */
    uint32_t quiescent; /* Native attestation of external cleanup. */
} resident_group_t;

typedef struct snapshot_record
{
    section_stl_handle_t owner; /* Self-contained identity even after task-slot reuse. */
    uint32_t task_index; /* Resident task index when allocated. */
    uint32_t offset; /* Snapshot first word, excluding charged wrap gap. */
    uint32_t words; /* Copied active stack length. */
    uint32_t charge; /* Reserved words including any preceding wrap gap. */
    uint32_t consumed; /* Tombstone/restore completed; physical reclaim is ordered. */
} snapshot_record_t;

static resident_group_t groups[SECTION_STL_MAX_GROUPS]; /* Resident group storage. */
static resident_task_t tasks[SECTION_STL_MAX_TASKS]; /* Resident task storage. */
static snapshot_record_t records[SECTION_STL_MAX_RECORDS]; /* FIFO allocation metadata. */
static reg_init_t init_callbacks[SECTION_STL_MAX_REGISTRATIONS]; /* One resident managed-init transaction. */
static uint32_t pool[SECTION_STL_POOL_WORDS]; /* Snapshot bytes are never executed here. */
static _Alignas(8) uint32_t runtime_stack[SECTION_STL_STACK_WORDS]; /* Single executable PSP stack. */
static resident_group_t *p_group_first; /* Head of global registered group list. */
static resident_task_t *p_ready_first; /* Ready FIFO head. */
static resident_task_t *p_ready_tail; /* Ready FIFO tail. */
static resident_task_t *p_current; /* Task whose CPU ownership ends only at a safe boundary. */
static uint32_t initialized; /* Cold-start guard; never reset by a business group. */
static uint32_t native_pending; /* Sticky request, consumed only at a safe native return. */
static uint32_t generation; /* Monotonic load generation, never rolled back. */
static uint32_t tick_now; /* Global monotonic modulo-32-bit tick. */
static uint32_t slice_start; /* Current task CPU-segment start in platform ticks. */
static uint32_t pool_head; /* First charged allocation position. */
static uint32_t pool_tail; /* Next allocation position. */
static uint32_t record_head; /* Oldest unretired allocation record. */
static uint32_t record_tail; /* Next free metadata slot. */
static section_stl_stats_t stats; /* Resident observability and sticky fault. */

/** @param a First identity. @param b Second identity. @return 1 for the same incarnation. */
static uint32_t identity_equal(section_stl_handle_t a, section_stl_handle_t b)
{
    return ((a.group_id == b.group_id) && /* Identity must match independently of storage address. */
            (a.generation == b.generation)) ? 1u : 0u; /* Generation rejects stale loads. */
}

/** @return 1 only when native Thread can make loader lifecycle assertions. */
static uint32_t native_context(void)
{
    return ((section_stl_port_in_handler() == 0u) && /* ISR cannot certify external cleanup. */
            (section_stl_port_active() == 0u)) ? 1u : 0u; /* No business PSP frame may still return. */
}

/** @param handle Exact identity. @return Resident group or NULL; caller holds port lock. */
static resident_group_t *group_find(section_stl_handle_t handle)
{
    resident_group_t *p_group = p_group_first; /* Resident list cursor. */
    while (p_group != NULL)
    {
        if (identity_equal(p_group->handle, handle) == 1u)
        {
            return p_group;
        }
        p_group = p_group->p_next;
    }
    return NULL;
}

/** @param p_group Resident owner. @return 1 while executing CPU or initialization code. */
static uint32_t group_executing(const resident_group_t *p_group)
{
    if (p_group->init_busy == 1u)
    {
        return 1u;
    }
    if (p_current != NULL)
    {
        return identity_equal(p_current->owner, p_group->handle);
    }
    return 0u;
}

/** @param handle Record identity. @return Count includes consumed records behind a live head. */
static uint32_t group_records(section_stl_handle_t handle)
{
    uint32_t count = 0u; /* Matching allocations still occupying FIFO metadata. */
    uint32_t index = record_head; /* Allocation FIFO cursor. */
    for (uint32_t remaining = stats.record_count; remaining > 0u; --remaining) /* Bounded record scan. */
    {
        if (identity_equal(records[index].owner, handle) == 1u)
        {
            ++count;
        }
        index = (index + 1u) % SECTION_STL_MAX_RECORDS;
    }
    return count;
}

/** Reclaim only the consumed prefix; no business pointer is consulted. */
static void records_reap(void)
{
    while (stats.record_count > 0u)
    {
        snapshot_record_t *p_record = &records[record_head]; /* Oldest reserved span. */
        if (p_record->consumed == 0u)
        {
            break;
        }
        if ((p_record->charge > stats.pool_used) || /* Reject accounting underflow. */
            (p_record->words > SECTION_STL_POOL_WORDS) || /* Bound subtraction below. */
            (p_record->offset > SECTION_STL_POOL_WORDS - p_record->words)) /* Span must fit the pool. */
        {
            stats.fault = SECTION_STL_CORRUPT;
            return;
        }
        pool_head = (p_record->offset + p_record->words) % SECTION_STL_POOL_WORDS;
        stats.pool_used -= p_record->charge;
        (void)memset(p_record, 0, sizeof(*p_record));
        record_head = (record_head + 1u) % SECTION_STL_MAX_RECORDS;
        --stats.record_count;
    }
    if (stats.record_count == 0u)
    {
        pool_head = 0u;
        pool_tail = 0u;
        if (stats.pool_used != 0u)
        {
            stats.fault = SECTION_STL_CORRUPT;
        }
    }
}

/** @param p_task Resident task; caller prevents duplicate admission. */
static void ready_push(resident_task_t *p_task)
{
    if (p_task->queued == 1u)
    {
        return;
    }
    p_task->p_ready_next = NULL;
    p_task->queued = 1u;
    if (p_ready_tail != NULL)
    {
        p_ready_tail->p_ready_next = p_task;
    }
    else
    {
        p_ready_first = p_task;
    }
    p_ready_tail = p_task;
    ++stats.ready_count;
}

/** Restore a native-preempted continuation before choosing another task turn. */
static void ready_push_front(resident_task_t *p_task)
{
    if (p_task->queued == 1u) { return; }
    p_task->queued = 1u;
    p_task->p_ready_next = p_ready_first;
    p_ready_first = p_task;
    if (p_ready_tail == NULL) { p_ready_tail = p_task; }
    ++stats.ready_count;
}

/** @return Oldest ready task, removed atomically under the caller's lock. */
static resident_task_t *ready_pop(void)
{
    resident_task_t *p_task = p_ready_first; /* Removed FIFO head. */
    if (p_task != NULL)
    {
        p_ready_first = p_task->p_ready_next;
        if (p_ready_first == NULL)
        {
            p_ready_tail = NULL;
        }
        p_task->p_ready_next = NULL;
        p_task->queued = 0u;
        --stats.ready_count;
    }
    return p_task;
}

/** @param handle Owner whose queued entries are removed. @param old_too 1 also cancels saved work. */
static void ready_remove(section_stl_handle_t handle, uint32_t old_too)
{
    resident_task_t *p_task = p_ready_first; /* Current queue node. */
    resident_task_t *p_previous = NULL; /* Last retained node. */
    while (p_task != NULL)
    {
        resident_task_t *p_next = p_task->p_ready_next; /* Safe resident successor. */
        if ((identity_equal(p_task->owner, handle) == 1u) && /* Cancel only this incarnation. */
            ((old_too == 1u) || (p_task->state == TASK_NEW))) /* Disable retains saved continuations. */
        {
            if (p_previous == NULL)
            {
                p_ready_first = p_next;
            }
            else
            {
                p_previous->p_ready_next = p_next;
            }
            if (p_ready_tail == p_task)
            {
                p_ready_tail = p_previous;
            }
            p_task->p_ready_next = NULL;
            p_task->queued = 0u;
            p_task->state = TASK_SLEEPING;
            --stats.ready_count;
        }
        else
        {
            p_previous = p_task;
        }
        p_task = p_next;
    }
}

/** @param p_group Group with closed admission. Invalidates records before any reclamation. */
static void group_cancel(resident_group_t *p_group)
{
    uint32_t index = record_head; /* Self-contained allocation cursor. */
    ready_remove(p_group->handle, 1u);
    for (uint32_t remaining = stats.record_count; remaining > 0u; --remaining) /* Bounded metadata scan. */
    {
        snapshot_record_t *p_record = &records[index]; /* Record can outlive its task slot. */
        if ((identity_equal(p_record->owner, p_group->handle) == 1u) && /* Exact revoked load. */
            (p_record->consumed == 0u)) /* Count cancellation only once. */
        {
            p_record->consumed = 1u;
            ++stats.discarded;
        }
        index = (index + 1u) % SECTION_STL_MAX_RECORDS;
    }
    for (resident_task_t *p_task = p_group->p_tasks; p_task != NULL; p_task = p_task->p_group_next) /* Resident tasks. */
    {
        p_task->record = STL_NONE;
    }
    records_reap();
    if (group_executing(p_group) == 0u)
    {
        p_group->state = SECTION_STL_REVOKED;
    }
}

/** @param p_task Task whose active frame is validated. @param p_sp Saved frame.
 * @return Word count, or 0 after latching corruption. */
static uint32_t frame_words(const resident_task_t *p_task, const uint32_t *p_sp)
{
    const uintptr_t low = (uintptr_t)&runtime_stack[STL_GUARD_WORDS]; /* Lowest legal software frame. */
    const uintptr_t top = (uintptr_t)&runtime_stack[SECTION_STL_STACK_WORDS]; /* Fixed common stack top. */
    const uintptr_t address = (uintptr_t)p_sp; /* Validate before subtracting unrelated pointers. */
    uint32_t words = 0u; /* Bounded number of active words. */
    (void)p_task;
    if ((runtime_stack[0] != STL_GUARD) || /* Detect downward overflow. */
        (runtime_stack[1] != ~STL_GUARD) || /* Complement makes zero fill observable. */
        (address < low) || /* Reject foreign stacks. */
        (address >= top) || /* Require a nonempty frame. */
        ((address % sizeof(uint32_t)) != 0u)) /* Aligned uint32_t access only. */
    {
        stats.fault = SECTION_STL_CORRUPT;
        return 0u;
    }
    words = (uint32_t)((top - address) / sizeof(uint32_t));
    if (section_stl_port_frame_valid(p_sp, words) == 0u)
    {
        stats.fault = SECTION_STL_CORRUPT;
        return 0u;
    }
    return words;
}

/** @param p_task Running resident task. @param p_sp Validated common-stack frame.
 * @param words Active words. @return 1 after atomic allocation/copy, 0 on bounded exhaustion. */
static uint32_t snapshot_save(resident_task_t *p_task, const uint32_t *p_sp, uint32_t words)
{
    uint32_t gap = 0u; /* Tail padding charged to this allocation. */
    uint32_t offset = pool_tail; /* Snapshot beginning. */
    snapshot_record_t *p_record = &records[record_tail]; /* Unpublished record until all checks pass. */
    records_reap();
    if (stats.fault != SECTION_STL_OK)
    {
        return 0u;
    }
    offset = pool_tail;
    if ((stats.record_count == SECTION_STL_MAX_RECORDS) || /* Preserve metadata ordering. */
        (words > SECTION_STL_POOL_WORDS - stats.pool_used)) /* No partial reservation. */
    {
        ++stats.pool_full;
        return 0u;
    }
    if (pool_tail >= pool_head)
    {
        if (words > SECTION_STL_POOL_WORDS - pool_tail)
        {
            gap = SECTION_STL_POOL_WORDS - pool_tail;
            offset = 0u;
            if ((words > pool_head) || /* Wrapped snapshot must not cross live head. */
                (gap > SECTION_STL_POOL_WORDS - stats.pool_used - words)) /* Charge stranded tail too. */
            {
                ++stats.pool_full;
                return 0u;
            }
        }
    }
    else if (words > pool_head - pool_tail)
    {
        ++stats.pool_full;
        return 0u;
    }
    *p_record = (snapshot_record_t){.owner = p_task->owner,
                                   .task_index = (uint32_t)(p_task - tasks),
                                   .offset = offset, .words = words, .charge = words + gap, .consumed = 0u};
    (void)memcpy(&pool[offset], p_sp, (size_t)words * sizeof(uint32_t));
    p_task->record = record_tail;
    record_tail = (record_tail + 1u) % SECTION_STL_MAX_RECORDS;
    pool_tail = (offset + words) % SECTION_STL_POOL_WORDS;
    stats.pool_used += words + gap;
    ++stats.record_count;
    ++stats.saves;
    return 1u;
}

/** @param p_task Selected resident task. @return Frame only after identity and frame validation. */
static uint32_t *snapshot_restore(resident_task_t *p_task)
{
    snapshot_record_t *p_record = NULL; /* Resident metadata; never a business object. */
    uint32_t *p_sp = NULL; /* Destination on the shared runtime stack. */
    if (p_task->record >= SECTION_STL_MAX_RECORDS)
    {
        stats.fault = SECTION_STL_CORRUPT;
        return NULL;
    }
    p_record = &records[p_task->record];
    if ((p_record->consumed == 1u) || /* Tombstones can never become CPU frames. */
        (identity_equal(p_record->owner, p_task->owner) == 0u) || /* Reused task slot is a different owner. */
        (p_record->task_index != (uint32_t)(p_task - tasks)) || /* Reject wrong task association. */
        (p_record->words > SECTION_STL_STACK_WORDS - STL_GUARD_WORDS) || /* Bound destination. */
        (p_record->words > SECTION_STL_POOL_WORDS) || /* Bound source subtraction. */
        (p_record->offset > SECTION_STL_POOL_WORDS - p_record->words)) /* Source must fit. */
    {
        stats.fault = SECTION_STL_CORRUPT;
        return NULL;
    }
    if (section_stl_port_frame_valid(&pool[p_record->offset], p_record->words) == 0u)
    {
        stats.fault = SECTION_STL_CORRUPT;
        return NULL;
    }
    p_sp = &runtime_stack[SECTION_STL_STACK_WORDS - p_record->words];
    (void)memcpy(p_sp, &pool[p_record->offset], (size_t)p_record->words * sizeof(uint32_t));
    p_record->consumed = 1u;
    p_task->record = STL_NONE;
    ++stats.restores;
    records_reap();
    if (stats.fault != SECTION_STL_OK)
    {
        return NULL;
    }
    return p_sp;
}

/** Activate sleeping tasks once, coalescing overdue releases without shifting phase. */
static void activate_due(void)
{
    for (resident_group_t *p_group = p_group_first; p_group != NULL; p_group = p_group->p_next) /* Group order. */
    {
        if (p_group->state != SECTION_STL_ENABLED)
        {
            continue;
        }
        for (resident_task_t *p_task = p_group->p_tasks; p_task != NULL; p_task = p_task->p_group_next) /* Period order. */
        {
            const uint32_t elapsed = tick_now - p_task->last; /* Modulo elapsed ticks. */
            if ((p_task->state == TASK_SLEEPING) && /* No overlapping activations. */
                (elapsed >= p_task->period)) /* Release only when due. */
            {
                p_task->last += (elapsed / p_task->period) * p_task->period;
                p_task->state = TASK_NEW;
                ready_push(p_task);
            }
        }
    }
}

SECTION_STL_STATUS_E section_stl_init(uint32_t now)
{
    if (section_stl_port_api_allowed() == 0u) /* Fast IRQ cannot reenter resident tables. */
    {
        return SECTION_STL_BAD_CONTEXT;
    }
    uint32_t saved = 0u; /* Previous interrupt mask. */
    if (native_context() == 0u)
    {
        return SECTION_STL_BAD_CONTEXT;
    }
    saved = section_stl_port_lock();
    if (initialized == 1u)
    {
        section_stl_port_unlock(saved);
        return SECTION_STL_BAD_STATE;
    }
    tick_now = now;
    slice_start = now;
    runtime_stack[0] = STL_GUARD;
    runtime_stack[1] = ~STL_GUARD;
    initialized = 1u;
    section_stl_port_unlock(saved);
    return SECTION_STL_OK;
}

SECTION_STL_STATUS_E section_stl_group_register(uint32_t group_id, const reg_section_t *p_entries,
                                               uint32_t count, section_stl_handle_t *p_handle)
{
    if (section_stl_port_api_allowed() == 0u) /* Fast IRQ cannot reenter resident tables. */
    {
        return SECTION_STL_BAD_CONTEXT;
    }
    uint32_t saved = 0u; /* Port transaction token. */
    uint32_t needed = 0u; /* Number of task descriptors. */
    uint32_t free_tasks = 0u; /* Available resident slots. */
    resident_group_t *p_group = NULL; /* Candidate empty group slot. */
    SECTION_STL_STATUS_E result = SECTION_STL_OK; /* Validated before any publication. */
    if (native_context() == 0u)
    {
        return SECTION_STL_BAD_CONTEXT;
    }
    if ((group_id == 0u) || /* Zero identity is reserved for native context. */
        (p_handle == NULL) || /* Caller must receive the generation. */
        (count > SECTION_STL_MAX_REGISTRATIONS) || /* Bound all table walks. */
        ((count > 0u) && (p_entries == NULL))) /* Nonempty table must be readable. */
    {
        return SECTION_STL_BAD_ARGUMENT;
    }
    saved = section_stl_port_lock();
    if ((initialized == 0u) || /* Global initialization is native and once-only. */
        (stats.fault != SECTION_STL_OK)) /* Faulted scheduler cannot admit new work. */
    {
        result = SECTION_STL_BAD_STATE;
    }
    for (uint32_t index = 0u; index < SECTION_STL_MAX_GROUPS; ++index) /* Fixed group slots. */
    {
        if (groups[index].handle.group_id == group_id)
        {
            result = SECTION_STL_BAD_STATE;
        }
        if (groups[index].handle.group_id == 0u)
        {
            p_group = &groups[index];
        }
    }
    for (uint32_t index = 0u; index < SECTION_STL_MAX_TASKS; ++index) /* Fixed task quota. */
    {
        if (tasks[index].state == TASK_FREE)
        {
            ++free_tasks;
        }
    }
    for (uint32_t index = 0u; index < count; ++index) /* Caller table is stable during native collection. */
    {
        if (p_entries[index].section_type == (uint32_t)SECTION_TASK)
        {
            const section_item_t *p_item = p_entries[index].p_str; /* Centralized generic wrapper recovery. */
            const reg_task_t *p_task = NULL; /* Business descriptor, temporary only. */
            if (p_item == NULL)
            {
                result = SECTION_STL_BAD_ARGUMENT;
                break;
            }
            p_task = p_item->p_obj;
            if (p_task == NULL)
            {
                result = SECTION_STL_BAD_ARGUMENT;
                break;
            }
            if ((p_task->t_period == 0u) || /* Zero-period scheduling is undefined. */
                (p_task->t_period >= STL_HALF) || /* Modular deadlines require half-range bound. */
                ((p_task->p_func == NULL) == (p_task->p_step_func == NULL))) /* Exactly one callback kind. */
            {
                result = SECTION_STL_BAD_ARGUMENT;
                break;
            }
            ++needed;
            for (uint32_t prior = 0u; prior < index; ++prior) /* Reject duplicate task wrapper registration. */
            {
                if ((p_entries[prior].section_type == (uint32_t)SECTION_TASK) && /* Compare only task wrappers. */
                    (p_entries[prior].p_str == p_entries[index].p_str)) /* Duplicate activation would be unsafe. */
                {
                    result = SECTION_STL_BAD_ARGUMENT;
                }
            }
        }
    }
    if ((result == SECTION_STL_OK) && /* Preserve earlier input/duplicate errors. */
        ((p_group == NULL) || (needed > free_tasks))) /* Registration must be all-or-nothing. */
    {
        result = SECTION_STL_NO_CAPACITY;
    }
    if ((result == SECTION_STL_OK) && /* Do not obscure the actual validation failure. */
        (generation == UINT32_MAX)) /* Never reuse a generation. */
    {
        result = SECTION_STL_GENERATION_EXHAUSTED;
    }
    if (result == SECTION_STL_OK)
    {
        ++generation;
        *p_group = (resident_group_t){.handle = {.group_id = group_id, .generation = generation},
                                      .state = SECTION_STL_DISABLED};
        for (uint32_t index = 0u; index < count; ++index) /* Commit copied descriptors. */
        {
            if (p_entries[index].section_type == (uint32_t)SECTION_TASK)
            {
                const section_item_t *p_item = p_entries[index].p_str; /* Validated wrapper. */
                const reg_task_t *p_source = p_item->p_obj; /* Read once; no pointer retained. */
                resident_task_t **pp_place = &p_group->p_tasks; /* Period-sorted resident insertion point. */
                for (uint32_t slot = 0u; slot < SECTION_STL_MAX_TASKS; ++slot) /* Find one free task. */
                {
                    if (tasks[slot].state == TASK_FREE)
                    {
                        resident_task_t *p_task = &tasks[slot]; /* Claimed resident task slot. */
                        *p_task = (resident_task_t){.owner = p_group->handle, .period = p_source->t_period,
                            .last = tick_now, .p_func = p_source->p_func, .p_step_func = p_source->p_step_func,
                            .p_ctx = p_source->p_ctx, .record = STL_NONE, .state = TASK_SLEEPING};
                        while (*pp_place != NULL)
                        {
                            if ((*pp_place)->period > p_task->period)
                            {
                                break;
                            }
                            pp_place = &(*pp_place)->p_group_next;
                        }
                        p_task->p_group_next = *pp_place;
                        *pp_place = p_task;
                        ++p_group->task_count;
                        break;
                    }
                }
            }
        }
        p_group->p_next = p_group_first;
        p_group_first = p_group;
        ++stats.group_count;
        *p_handle = p_group->handle;
    }
    section_stl_port_unlock(saved);
    return result;
}

SECTION_STL_STATUS_E section_stl_group_initialize(section_stl_handle_t handle,
                                                 const reg_section_t *p_entries, uint32_t count)
{
    if (section_stl_port_api_allowed() == 0u) /* Fast IRQ cannot reenter resident tables. */
    {
        return SECTION_STL_BAD_CONTEXT;
    }
    uint32_t callback_count = 0u; /* Number of validated initialization callbacks. */
    uint32_t owns_callbacks = 0u; /* Only the admitted transaction may clear the shared init buffer. */
    uint32_t saved = 0u; /* Critical-section token. */
    resident_group_t *p_group = NULL; /* Resident owner during synchronous callbacks. */
    SECTION_STL_STATUS_E result = SECTION_STL_OK; /* Initialization admission result. */
    if (native_context() == 0u)
    {
        return SECTION_STL_BAD_CONTEXT;
    }
    if ((count > SECTION_STL_MAX_REGISTRATIONS) || /* Bound native stack use and sorting. */
        ((count > 0u) && (p_entries == NULL))) /* Empty init table is valid. */
    {
        return SECTION_STL_BAD_ARGUMENT;
    }
    saved = section_stl_port_lock();
    p_group = group_find(handle);
    if (p_group == NULL)
    {
        result = SECTION_STL_STALE;
    }
    else if ((p_group->state != SECTION_STL_DISABLED) || /* Init cannot race scheduling. */
             (p_group->init_busy == 1u) || /* Reject recursive initialization. */
             (p_group->initialized == 1u)) /* Never initialize a live incarnation twice. */
    {
        result = SECTION_STL_BAD_STATE;
    }
    if (stats.fault != SECTION_STL_OK)
    {
        result = SECTION_STL_BAD_STATE;
    }
    for (resident_group_t *p_other = p_group_first; p_other != NULL; p_other = p_other->p_next) /* One init buffer. */
    {
        if (p_other->init_busy == 1u)
        {
            result = SECTION_STL_BAD_STATE;
        }
    }
    if (result == SECTION_STL_OK)
    {
        owns_callbacks = 1u;
        for (uint32_t index = 0u; index < count; ++index) /* Copy callbacks before releasing admission lock. */
        {
            if (p_entries[index].section_type == (uint32_t)SECTION_INIT)
            {
                const section_item_t *p_item = p_entries[index].p_str; /* Validated generic wrapper boundary. */
                const reg_init_t *p_init = NULL; /* Business initialization descriptor. */
                uint32_t place = callback_count; /* Stable insertion-sort position. */
                if (p_item == NULL)
                {
                    result = SECTION_STL_BAD_ARGUMENT;
                    break;
                }
                p_init = p_item->p_obj;
                if (p_init == NULL)
                {
                    result = SECTION_STL_BAD_ARGUMENT;
                    break;
                }
                if (p_init->p_func == NULL)
                {
                    result = SECTION_STL_BAD_ARGUMENT;
                    break;
                }
                while (place > 0u)
                {
                    if (init_callbacks[place - 1u].priority <= p_init->priority)
                    {
                        break;
                    }
                    init_callbacks[place] = init_callbacks[place - 1u];
                    --place;
                }
                init_callbacks[place] = *p_init;
                ++callback_count;
            }
        }
        if (result == SECTION_STL_OK)
        {
            p_group->init_busy = 1u;
        }
    }
    if ((result != SECTION_STL_OK) && /* Validation failed before callback execution. */
        (owns_callbacks == 1u)) /* Never erase another active init transaction. */
    {
        (void)memset(init_callbacks, 0, sizeof(init_callbacks));
    }
    section_stl_port_unlock(saved);
    if (result != SECTION_STL_OK)
    {
        return result;
    }
    for (uint32_t index = 0u; index < callback_count; ++index) /* Synchronous managed initialization. */
    {
        saved = section_stl_port_lock();
        if (p_group->state >= SECTION_STL_REVOKING)
        {
            result = SECTION_STL_BAD_STATE;
        }
        section_stl_port_unlock(saved);
        if (result != SECTION_STL_OK)
        {
            break;
        }
        init_callbacks[index].p_func(); /* init_busy prevents overwrite across the unlocked call. */
    }
    saved = section_stl_port_lock();
    (void)memset(init_callbacks, 0, sizeof(init_callbacks)); /* Remove all transient business function pointers. */
    p_group->init_busy = 0u;
    if (p_group->state >= SECTION_STL_REVOKING)
    {
        p_group->state = SECTION_STL_REVOKED;
        result = SECTION_STL_BAD_STATE;
    }
    else
    {
        p_group->initialized = 1u;
    }
    section_stl_port_unlock(saved);
    return result;
}

SECTION_STL_STATUS_E section_stl_group_enable(section_stl_handle_t handle)
{
    if (section_stl_port_api_allowed() == 0u) /* Fast IRQ cannot reenter resident tables. */
    {
        return SECTION_STL_BAD_CONTEXT;
    }
    const uint32_t saved = section_stl_port_lock(); /* Admission transaction. */
    resident_group_t *p_group = group_find(handle); /* Exact resident incarnation. */
    SECTION_STL_STATUS_E result = SECTION_STL_STALE; /* Default for stale handles. */
    if (p_group != NULL)
    {
        result = SECTION_STL_BAD_STATE;
        if ((p_group->initialized == 1u) && /* Managed init must have returned. */
            (p_group->state < SECTION_STL_REVOKING) && /* Revocation is irreversible. */
            (stats.fault == SECTION_STL_OK)) /* No new work after corruption. */
        {
            if (p_group->state != SECTION_STL_ENABLED)
            {
                for (resident_task_t *p_task = p_group->p_tasks; p_task != NULL; p_task = p_task->p_group_next) /* Rebase. */
                {
                    p_task->last = tick_now;
                }
            }
            p_group->state = SECTION_STL_ENABLED;
            result = SECTION_STL_OK;
        }
    }
    section_stl_port_unlock(saved);
    return result;
}

SECTION_STL_STATUS_E section_stl_group_disable(section_stl_handle_t handle)
{
    if (section_stl_port_api_allowed() == 0u) /* Fast IRQ cannot reenter resident tables. */
    {
        return SECTION_STL_BAD_CONTEXT;
    }
    const uint32_t saved = section_stl_port_lock(); /* Close activation atomically. */
    resident_group_t *p_group = group_find(handle); /* Exact resident owner. */
    SECTION_STL_STATUS_E result = SECTION_STL_STALE; /* Missing incarnation. */
    if (p_group != NULL)
    {
        result = SECTION_STL_BAD_STATE;
        if (p_group->state < SECTION_STL_REVOKING)
        {
            p_group->state = SECTION_STL_DISABLED;
            ready_remove(handle, 0u);
            result = SECTION_STL_OK;
        }
    }
    section_stl_port_unlock(saved);
    return result;
}

SECTION_STL_STATUS_E section_stl_group_pause(section_stl_handle_t handle)
{
    if (section_stl_port_api_allowed() == 0u) /* Fast IRQ cannot reenter resident tables. */
    {
        return SECTION_STL_BAD_CONTEXT;
    }
    const uint32_t saved = section_stl_port_lock(); /* Check and transition in one transaction. */
    resident_group_t *p_group = group_find(handle); /* Exact resident owner. */
    SECTION_STL_STATUS_E result = SECTION_STL_STALE; /* Missing incarnation. */
    if (p_group != NULL)
    {
        result = SECTION_STL_BAD_STATE;
        if (p_group->state < SECTION_STL_REVOKING)
        {
            result = SECTION_STL_BUSY;
            if ((group_executing(p_group) == 0u) && /* Never freeze an active callback mid-call. */
                (group_records(handle) == 0u) && /* Never pin FIFO snapshots behind a paused head. */
                (p_group->references == 0u)) /* No asynchronous caller may be paused invisibly. */
            {
                p_group->state = SECTION_STL_PAUSED;
                ready_remove(handle, 0u);
                result = SECTION_STL_OK;
            }
        }
    }
    section_stl_port_unlock(saved);
    return result;
}

SECTION_STL_STATUS_E section_stl_group_revoke(section_stl_handle_t handle)
{
    if (section_stl_port_api_allowed() == 0u) /* Fast IRQ cannot reenter resident tables. */
    {
        return SECTION_STL_BAD_CONTEXT;
    }
    const uint32_t saved = section_stl_port_lock(); /* Close ingress before invalidating snapshots. */
    resident_group_t *p_group = group_find(handle); /* Exact incarnation. */
    SECTION_STL_STATUS_E result = SECTION_STL_STALE; /* Missing identity. */
    if (p_group != NULL)
    {
        p_group->state = SECTION_STL_REVOKING;
        group_cancel(p_group);
        result = SECTION_STL_OK;
        if (group_executing(p_group) == 1u)
        {
            result = SECTION_STL_BUSY;
            if (p_current != NULL)
            {
                if (identity_equal(p_current->owner, handle) == 1u)
                {
                    section_stl_port_request_switch(); /* Tail-chain if revoke races exception-return commit. */
                }
            }
        }
    }
    section_stl_port_unlock(saved);
    return result;
}

SECTION_STL_STATUS_E section_stl_group_acquire(section_stl_handle_t handle)
{
    if (section_stl_port_api_allowed() == 0u) /* Fast IRQ cannot reenter resident tables. */
    {
        return SECTION_STL_BAD_CONTEXT;
    }
    const uint32_t saved = section_stl_port_lock(); /* Atomic admission and reference increment. */
    resident_group_t *p_group = group_find(handle); /* Exact owner of the published external pointer. */
    SECTION_STL_STATUS_E result = SECTION_STL_STALE; /* Stale callbacks cannot reacquire. */
    if (p_group != NULL)
    {
        result = SECTION_STL_BAD_STATE;
        if ((p_group->state < SECTION_STL_PAUSED) && /* Disable drains existing owners; pause/revoke blocks new ones. */
            (p_group->references < UINT32_MAX)) /* Never wrap ownership accounting. */
        {
            ++p_group->references;
            result = SECTION_STL_OK;
        }
    }
    section_stl_port_unlock(saved);
    return result;
}

SECTION_STL_STATUS_E section_stl_group_release(section_stl_handle_t handle)
{
    if (section_stl_port_api_allowed() == 0u) /* Fast IRQ cannot reenter resident tables. */
    {
        return SECTION_STL_BAD_CONTEXT;
    }
    const uint32_t saved = section_stl_port_lock(); /* Exact-once release accounting. */
    resident_group_t *p_group = group_find(handle); /* Original generation only. */
    SECTION_STL_STATUS_E result = SECTION_STL_STALE; /* Late old releases are harmless. */
    if (p_group != NULL)
    {
        result = SECTION_STL_BAD_STATE;
        if (p_group->references > 0u)
        {
            --p_group->references;
            result = SECTION_STL_OK;
        }
    }
    section_stl_port_unlock(saved);
    return result;
}

uint32_t SECTION_WEAK section_stl_host_switch_allowed(void)
{
    return 1u; /* Standalone runtime has no external servicing transaction. */
}

uint32_t SECTION_WEAK section_stl_host_owner_commit(section_stl_handle_t owner)
{
    (void)owner;
    return 1u; /* Shuttle overrides with a resident, bounded ownership transaction. */
}

/** @param owner Exact next resident identity, or all-zero for native. @return Commit success. */
static uint32_t host_commit(section_stl_handle_t owner)
{
    if (section_stl_host_owner_commit(owner) == 0u)
    {
        stats.fault = SECTION_STL_CORRUPT; /* No unchecked owner may execute business code. */
        return 0u;
    }
    return 1u;
}

/** @return NULL after committing native ownership, or a sticky isolation fault on failure. */
static uint32_t *return_native(void)
{
    if (host_commit((section_stl_handle_t){0}) == 1u)
    {
        native_pending = 0u;
    }
    return NULL;
}

/** @param p_group Resident group. @return 1 only after all component-owned references are gone. */
static uint32_t group_retired(const resident_group_t *p_group)
{
    return ((p_group->state == SECTION_STL_REVOKED) && /* Irreversible entry closure. */
            (group_executing(p_group) == 0u) && /* No return path remains live. */
            (p_group->references == 0u) && /* External users explicitly released ownership. */
            (group_records(p_group->handle) == 0u)) ? 1u : 0u; /* FIFO allocation history fully retired. */
}

SECTION_STL_STATUS_E section_stl_group_quiescent(section_stl_handle_t handle)
{
    if (section_stl_port_api_allowed() == 0u) /* Fast IRQ cannot reenter resident tables. */
    {
        return SECTION_STL_BAD_CONTEXT;
    }
    uint32_t saved = 0u; /* Native finalization lock. */
    resident_group_t *p_group = NULL; /* Resident lifecycle authority. */
    SECTION_STL_STATUS_E result = SECTION_STL_STALE; /* Missing incarnation. */
    if (native_context() == 0u)
    {
        return SECTION_STL_BAD_CONTEXT;
    }
    saved = section_stl_port_lock();
    records_reap();
    p_group = group_find(handle);
    if (p_group != NULL)
    {
        result = SECTION_STL_BUSY;
        if ((group_retired(p_group) == 1u) && /* Core evidence must be complete. */
            (stats.fault == SECTION_STL_OK)) /* Corruption invalidates overwrite proof. */
        {
            p_group->quiescent = 1u;
            result = SECTION_STL_OK;
        }
    }
    section_stl_port_unlock(saved);
    return result;
}

SECTION_STL_STATUS_E section_stl_group_forget(section_stl_handle_t handle)
{
    if (section_stl_port_api_allowed() == 0u) /* Fast IRQ cannot reenter resident tables. */
    {
        return SECTION_STL_BAD_CONTEXT;
    }
    uint32_t saved = 0u; /* Native unlink transaction. */
    resident_group_t *p_group = NULL; /* Group being retired. */
    resident_group_t **pp_link = &p_group_first; /* Resident list unlink location. */
    SECTION_STL_STATUS_E result = SECTION_STL_STALE; /* Missing or already forgotten incarnation. */
    if (native_context() == 0u)
    {
        return SECTION_STL_BAD_CONTEXT;
    }
    saved = section_stl_port_lock();
    p_group = group_find(handle);
    if (p_group != NULL)
    {
        result = SECTION_STL_BUSY;
        if ((group_retired(p_group) == 1u) && /* Recheck after native attestation. */
            (p_group->quiescent == 1u) && /* Explicit IRQ/DMA/resource cleanup is mandatory. */
            (stats.fault == SECTION_STL_OK)) /* Never certify a corrupted runtime. */
        {
            resident_task_t *p_task = p_group->p_tasks; /* Resident task list to clear. */
            while (p_task != NULL)
            {
                resident_task_t *p_next = p_task->p_group_next; /* Save before clearing the resident slot. */
                (void)memset(p_task, 0, sizeof(*p_task));
                p_task = p_next;
            }
            while (*pp_link != p_group)
            {
                pp_link = &(*pp_link)->p_next;
            }
            *pp_link = p_group->p_next;
            (void)memset(p_group, 0, sizeof(*p_group));
            --stats.group_count;
            result = SECTION_STL_OK;
        }
    }
    section_stl_port_unlock(saved);
    return result;
}

SECTION_STL_STATUS_E section_stl_group_query(section_stl_handle_t handle, section_stl_group_info_t *p_info)
{
    if (section_stl_port_api_allowed() == 0u) /* Fast IRQ cannot reenter resident tables. */
    {
        return SECTION_STL_BAD_CONTEXT;
    }
    uint32_t saved = 0u; /* Snapshot lock. */
    resident_group_t *p_group = NULL; /* Exact resident owner. */
    SECTION_STL_STATUS_E result = SECTION_STL_STALE; /* Missing incarnation. */
    if (p_info == NULL)
    {
        return SECTION_STL_BAD_ARGUMENT;
    }
    saved = section_stl_port_lock();
    p_group = group_find(handle);
    if (p_group != NULL)
    {
        *p_info = (section_stl_group_info_t){.state = p_group->state, .task_count = p_group->task_count,
            .records = group_records(handle), .references = p_group->references,
            .executing = group_executing(p_group), .initialized = p_group->initialized,
            .quiescent = p_group->quiescent};
        result = SECTION_STL_OK;
    }
    section_stl_port_unlock(saved);
    return result;
}

SECTION_STL_STATUS_E section_stl_tick(uint32_t now)
{
    if (section_stl_port_api_allowed() == 0u) /* Fast IRQ cannot reenter resident tables. */
    {
        return SECTION_STL_BAD_CONTEXT;
    }
    const uint32_t saved = section_stl_port_lock(); /* Time publication serializes with switching. */
    SECTION_STL_STATUS_E result = SECTION_STL_BAD_STATE; /* Not initialized or faulted. */
    uint32_t request = 0u; /* Deferred PendSV request outside state mutation. */
    if ((initialized == 1u) && /* Native timebase established. */
        (stats.fault == SECTION_STL_OK)) /* Corrupt scheduler stays closed. */
    {
        result = SECTION_STL_BAD_ARGUMENT;
        if (now - tick_now < STL_HALF)
        {
            tick_now = now;
            if (p_current == NULL) /* Native idle has no interrupted task to queue first. */
            {
                activate_due();
            }
            /* Otherwise switch queues the interrupted task before newly due work. */
            result = SECTION_STL_OK;
            if ((p_current != NULL) && /* A PSP task owns CPU execution. */
                (section_stl_port_switch_active() == 0u) && /* Active PendSV already consumes this boundary. */
                (tick_now - slice_start >= p_current->quantum_left)) /* Current task exhausted its remaining quantum. */
            {
                request = 1u;
            }
        }
    }
    if (request == 1u)
    {
        section_stl_port_request_switch(); /* PendSV remains masked until state is committed. */
    }
    section_stl_port_unlock(saved);
    return result;
}

SECTION_STL_STATUS_E section_stl_run(void)
{
    if (section_stl_port_api_allowed() == 0u) /* Fast IRQ cannot reenter resident tables. */
    {
        return SECTION_STL_BAD_CONTEXT;
    }
    uint32_t saved = 0u; /* Native dispatch admission lock. */
    if ((initialized == 0u) || /* Startup must precede native dispatch. */
        (stats.fault != SECTION_STL_OK)) /* Faulted contexts are never resumed. */
    {
        return SECTION_STL_BAD_STATE;
    }
    saved = section_stl_port_lock();
    for (resident_group_t *p_group = p_group_first; p_group != NULL; p_group = p_group->p_next) /* Init barrier. */
    {
        if (p_group->init_busy == 1u)
        {
            section_stl_port_unlock(saved);
            return SECTION_STL_BUSY;
        }
    }
    section_stl_port_unlock(saved);
    if (section_stl_port_run() == 0u)
    {
        return SECTION_STL_BAD_CONTEXT;
    }
    return stats.fault;
}

SECTION_STL_STATUS_E section_stl_yield(void)
{
    if (section_stl_port_api_allowed() == 0u) /* Fast IRQ cannot reenter resident tables. */
    {
        return SECTION_STL_BAD_CONTEXT;
    }
    const uint32_t saved = section_stl_port_lock(); /* Validate current ownership with request. */
    SECTION_STL_STATUS_E result = SECTION_STL_BAD_CONTEXT; /* Native yield is unsupported. */
    if (p_current != NULL)
    {
        section_stl_port_request_switch();
        result = SECTION_STL_OK;
    }
    section_stl_port_unlock(saved);
    return result;
}

SECTION_STL_STATUS_E section_stl_request_native(void)
{
    uint32_t saved = 0u; /* Protect the sticky request against switch consumption. */
    SECTION_STL_STATUS_E result = SECTION_STL_BAD_CONTEXT; /* Native caller already owns MSP. */
    if (section_stl_port_api_allowed() == 0u)
    {
        return SECTION_STL_BAD_CONTEXT;
    }
    saved = section_stl_port_lock();
    if (initialized == 0u)
    {
        result = SECTION_STL_BAD_STATE;
    }
    else if (stats.fault != SECTION_STL_OK)
    {
        result = stats.fault;
    }
    else if (section_stl_port_active() == 1u)
    {
        native_pending = 1u;
        section_stl_port_request_switch();
        result = SECTION_STL_OK;
    }
    section_stl_port_unlock(saved);
    return result;
}

void section_stl_stats_get(section_stl_stats_t *p_stats)
{
    if (section_stl_port_api_allowed() == 0u) /* Do not inspect shared diagnostics in a fast IRQ. */
    {
        if (p_stats != NULL)
        {
            *p_stats = (section_stl_stats_t){.fault = SECTION_STL_BAD_CONTEXT};
        }
        return;
    }
    uint32_t saved = 0u; /* Atomic diagnostic read. */
    if (p_stats != NULL)
    {
        saved = section_stl_port_lock();
        *p_stats = stats;
        p_stats->current = (section_stl_handle_t){0};
        if (p_current != NULL)
        {
            p_stats->current = p_current->owner;
        }
        section_stl_port_unlock(saved);
    }
}

uint32_t *section_stl_switch(uint32_t *p_saved_sp)
{
    resident_group_t *p_group = NULL; /* Resident owner of the departing/arriving frame. */
    resident_task_t *p_next = NULL; /* Selected FIFO task. */
    uint32_t *p_next_sp = NULL; /* Validated M frame to restore. */
    const uint32_t elapsed = tick_now - slice_start; /* Elapsed current CPU segment, excluding native pauses. */
    if (stats.fault != SECTION_STL_OK)
    {
        p_current = NULL;
        return return_native();
    }
    if (section_stl_host_switch_allowed() == 0u)
    {
        return p_saved_sp; /* No state/slice/queue change while host service owns the old stack. */
    }
    if (p_current != NULL)
    {
        p_group = group_find(p_current->owner);
        if ((p_group == NULL) || /* A missing owner cannot be resumed. */
            (p_group->state >= SECTION_STL_REVOKING)) /* Cancel before touching its old CPU frame. */
        {
            p_current->state = TASK_SLEEPING;
            p_current = NULL;
            if (p_group != NULL)
            {
                group_cancel(p_group);
            }
        }
        else if (p_current->state == TASK_RUNNING)
        {
            const uint32_t words = frame_words(p_current, p_saved_sp); /* Validate PSP before copying. */
            if (words == 0u)
            {
                p_current = NULL;
                return return_native();
            }
            if (snapshot_save(p_current, p_saved_sp, words) == 0u)
            {
                if (stats.fault != SECTION_STL_OK)
                {
                    p_current = NULL;
                    return return_native();
                }
                return p_saved_sp; /* Existing current identity remains valid; keep running on exhaustion. */
            }
            p_current->state = TASK_OLD;
            const uint32_t charged = elapsed != 0u ? elapsed : 1u;
            if (native_pending == 1u && charged < p_current->quantum_left)
            {
                p_current->quantum_left -= charged;
                ready_push_front(p_current);
            }
            else
            {
                p_current->quantum_left = SECTION_STL_SLICE_TICKS;
                ready_push(p_current);
            }
            p_current = NULL;
        }
        else
        {
            p_current = NULL; /* Completed callback no longer owns a resumable stack. */
        }
    }
    if ((initialized == 0u) || /* Reject accidental pre-init SVC. */
        (stats.fault != SECTION_STL_OK)) /* Corruption returns native, never an unchecked CPU frame. */
    {
        return return_native();
    }
    if (native_pending == 1u)
    {
        return return_native(); /* Current frame is safely copied/queued, or callback has completed. */
    }
    activate_due();
    records_reap();
    if (stats.fault != SECTION_STL_OK)
    {
        return return_native();
    }
    p_next = ready_pop();
    while (p_next != NULL)
    {
        p_group = group_find(p_next->owner);
        if ((p_group == NULL) || /* Generation must still exist in resident group list. */
            (p_group->state >= SECTION_STL_PAUSED) || /* Pause/revoke never restores a frame. */
            ((p_group->state == SECTION_STL_DISABLED) && (p_next->state != TASK_OLD))) /* Disable drains only old work. */
        {
            if (p_group != NULL)
            {
                if (p_group->state >= SECTION_STL_REVOKING)
                {
                    group_cancel(p_group);
                    if (stats.fault != SECTION_STL_OK)
                    {
                        return return_native();
                    }
                }
            }
            p_next = ready_pop();
            continue;
        }
        if (p_next->state == TASK_OLD)
        {
            p_next_sp = snapshot_restore(p_next);
        }
        else
        {
            p_next->quantum_left = SECTION_STL_SLICE_TICKS;
            p_next_sp = section_stl_port_prepare(&runtime_stack[STL_GUARD_WORDS],
                                                 &runtime_stack[SECTION_STL_STACK_WORDS], section_stl_entry);
        }
        if ((stats.fault != SECTION_STL_OK) || /* Reclamation failure cannot escape through a fresh frame. */
            (p_next_sp == NULL) || /* Failed construction/restore must not become an exception return. */
            (frame_words(p_next, p_next_sp) == 0u)) /* Validate complete selected frame. */
        {
            stats.fault = SECTION_STL_CORRUPT;
            return return_native();
        }
        if (host_commit(p_next->owner) == 0u)
        {
            p_current = NULL;
            return return_native(); /* Fail closed, including an attempted owner clear. */
        }
        slice_start = tick_now; /* New CPU segment; native pauses do not consume task credit. */
        p_next->state = TASK_RUNNING;
        p_current = p_next;
        return p_next_sp; /* Port holds the kernel priority ceiling until exception-return commit. */
    }
    return return_native();
}

void section_stl_entry(void)
{
    for (;;)
    {
        resident_task_t *p_task = p_current; /* Selected resident descriptor; port committed this owner. */
        resident_group_t *p_group = NULL; /* Lifecycle checked after callback before requeue. */
        section_task_status_t outcome = SECTION_TASK_DONE; /* Plain callbacks complete one activation. */
        uint32_t saved = 0u; /* Completion transaction token. */
        uint32_t inline_ok = 0u; /* CPU-qualified fast path sampled before acquiring the kernel ceiling. */
        if (p_task == NULL)
        {
            section_stl_port_request_switch();
            continue;
        }
        if (p_task->p_func != NULL)
        {
            p_task->p_func();
        }
        else if (p_task->p_step_func != NULL)
        {
            outcome = p_task->p_step_func(p_task->p_ctx);
        }
        inline_ok = section_stl_port_inline_allowed();
        saved = section_stl_port_lock();
        p_group = group_find(p_task->owner);
        p_task->state = TASK_SLEEPING;
        if ((p_group != NULL) && /* Owner survived until callback completion. */
            (p_group->state == SECTION_STL_ENABLED) && /* Disable/revoke prevents new steps. */
            (outcome == SECTION_TASK_RUNNING)) /* Step continuation requires no CPU stack snapshot. */
        {
            p_task->state = TASK_NEW;
            ready_push(p_task);
        }
        activate_due(); /* Preserve completion order before choosing the FIFO head. */
        if ((inline_ok == 1u) && /* M port confirms compatible Thread/PSP execution. */
            (native_pending == 0u) && /* Never starve a pending native-service request. */
            (section_stl_host_switch_allowed() == 1u) && /* Host servicing must be finished. */
            (tick_now - slice_start < p_task->quantum_left) && /* Do not extend the departing task quantum. */
            (p_ready_first != NULL) && /* Fast path has actual work. */
            (p_ready_first->state == TASK_NEW)) /* Never pass a pending old snapshot. */
        {
            resident_group_t *p_next_group = group_find(p_ready_first->owner); /* Resident admission check. */
            if ((p_next_group != NULL) && /* Exact next incarnation remains loaded. */
                (p_next_group->state == SECTION_STL_ENABLED) && /* No callback after admission closes. */
                (host_commit(p_ready_first->owner) == 1u)) /* Inline is also a real owner transition. */
            {
                p_current = ready_pop();
                p_current->quantum_left = SECTION_STL_SLICE_TICKS;
                slice_start = tick_now;
                p_current->state = TASK_RUNNING;
                if (p_group != NULL)
                {
                    if (p_group->state == SECTION_STL_REVOKING)
                    {
                        group_cancel(p_group);
                    }
                }
                section_stl_port_unlock(saved);
                continue;
            }
        }
        section_stl_port_request_switch(); /* Completion is committed before PendSV can run. */
        section_stl_port_unlock(saved);
        for (;;)
        {
            /* PendSV must transfer execution; a completed callback must never run again here. */
        }
    }
}
