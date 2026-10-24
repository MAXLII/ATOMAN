// SPDX-License-Identifier: MIT
/**
 * @file section.h
 * @brief Resident task groups and section registration for Shuttle SRTOS.
 * @details Fixed-capacity, single-core C11 runtime. Descriptors are copied;
 *          cancellation never follows a pointer into an unloaded descriptor.
 *          Source registration compatibility is not a cross-image binary ABI.
 * Copyright (c) 2026 Max.Li. Licensed under the MIT License.
 */
#ifndef SECTION_STL_SECTION_H
#define SECTION_STL_SECTION_H

#include <stddef.h>
#include <stdint.h>
#include "platform.h"

#ifndef SECTION_STL_PLATFORM_CONTRACT
#error "SRTOS_STL requires an explicit framework platform.h contract"
#elif SECTION_STL_PLATFORM_CONTRACT != 1
#error "Unsupported SRTOS_STL platform contract version"
#endif
#if !defined(SECTION_REG_ATTR_PREFIX) || !defined(SECTION_REG_ATTR_SUFFIX)
#error "SRTOS_STL platform contract must define both registration attributes"
#endif
#ifndef SECTION_WEAK
#error "SRTOS_STL platform contract must define an explicit weak-symbol attribute"
#endif
#ifndef SECTION_SYS_TICK_UNIT_US
#error "SRTOS_STL platform contract must define SECTION_SYS_TICK_UNIT_US"
#elif (SECTION_SYS_TICK_UNIT_US < 1) || (SECTION_SYS_TICK_UNIT_US > UINT32_MAX)
#error "SRTOS_STL tick unit must be a positive uint32 microsecond value"
#endif

#ifndef SECTION_STL_MAX_GROUPS
#define SECTION_STL_MAX_GROUPS 8u /* Resident group slots. */
#endif
#ifndef SECTION_STL_MAX_TASKS
#define SECTION_STL_MAX_TASKS 32u /* Resident task slots. */
#endif
#ifndef SECTION_STL_MAX_RECORDS
#define SECTION_STL_MAX_RECORDS 32u /* Independent FIFO allocation records. */
#endif
#ifndef SECTION_STL_POOL_WORDS
#define SECTION_STL_POOL_WORDS 1024u /* Snapshot storage in uint32_t words. */
#endif
#ifndef SECTION_STL_STACK_WORDS
#define SECTION_STL_STACK_WORDS 512u /* Shared task stack, including guard. */
#endif
#ifndef SECTION_STL_SLICE_TICKS
#define SECTION_STL_SLICE_TICKS 10u /* Maximum per-task quantum in platform ticks. */
#endif
#ifndef SECTION_STL_MAX_REGISTRATIONS
#define SECTION_STL_MAX_REGISTRATIONS 128u /* Bounded section-table input. */
#endif

typedef enum
{
    SECTION_STL_OK = 0, /* Operation completed; revoke alone does not authorize overwrite. */
    SECTION_STL_BAD_ARGUMENT, /* Invalid bounded input. */
    SECTION_STL_BAD_STATE, /* Operation is invalid in the current lifecycle. */
    SECTION_STL_STALE, /* No resident group has the supplied identity. */
    SECTION_STL_BUSY, /* In-flight work, records or references still exist. */
    SECTION_STL_NO_CAPACITY, /* Fixed resident slots are exhausted. */
    SECTION_STL_POOL_FULL, /* Snapshot storage is full; current execution is retained. */
    SECTION_STL_GENERATION_EXHAUSTED, /* Loading counter cannot be incremented. */
    SECTION_STL_BAD_CONTEXT, /* Operation requires native Thread context. */
    SECTION_STL_CORRUPT /* Invalid stack/frame or broken internal invariant; fail closed. */
} SECTION_STL_STATUS_E;

typedef enum
{
    SECTION_STL_DISABLED = 0, /* No new activations; saved continuations may drain. */
    SECTION_STL_ENABLED, /* Periodic activation allowed. */
    SECTION_STL_PAUSED, /* Accepted only without current/saved execution or leases. */
    SECTION_STL_REVOKING, /* Entry closed; current execution or init still in flight. */
    SECTION_STL_REVOKED /* CPU entry retired; physical reclamation may still be pending. */
} SECTION_STL_GROUP_STATE_E;

typedef struct section_stl_handle
{
    uint32_t group_id; /* Stable loader identity, never a business pointer. */
    uint32_t generation; /* Nonwrapping incarnation, valid only until reset. */
} section_stl_handle_t;

typedef struct section_stl_group_info
{
    SECTION_STL_GROUP_STATE_E state; /* Current resident lifecycle state. */
    uint32_t task_count; /* Copied tasks belonging to this incarnation. */
    uint32_t records; /* Allocations not physically reclaimed, including tombstones. */
    uint32_t references; /* Explicit callback/DMA leases. */
    uint32_t executing; /* Current CPU or managed initialization is in flight. */
    uint32_t initialized; /* Managed init completed and returned. */
    uint32_t quiescent; /* Native resource cleanup was explicitly confirmed. */
} section_stl_group_info_t;

typedef struct section_stl_stats
{
    uint32_t group_count; /* Live resident group-list members. */
    uint32_t ready_count; /* Entries in the ready FIFO. */
    uint32_t pool_used; /* Reserved words, including wrap gaps and consumed records. */
    uint32_t record_count; /* Unreclaimed allocation records. */
    uint32_t saves; /* Completed stack copies into the pool. */
    uint32_t restores; /* Validated stack copies out of the pool. */
    uint32_t discarded; /* Invalidated snapshots consumed without CPU restore. */
    uint32_t pool_full; /* Save attempts that kept current execution due to capacity. */
    SECTION_STL_STATUS_E fault; /* Sticky corruption reason; requires reset. */
    section_stl_handle_t current; /* Zero identity when native owns execution. */
} section_stl_stats_t;

/* These descriptor/wrapper shapes retain the existing section collection scheme. */
typedef struct section_item
{
    void *p_obj; /* Registration object, inspected only during collection. */
    struct section_item *p_next; /* Legacy wrapper link; never used by resident core. */
} section_item_t;

typedef struct reg_section
{
    uint32_t section_type; /* Existing SECTION_INIT=0 / SECTION_TASK=1 tags. */
    void *p_str; /* Pointer to a section_item_t wrapper. */
} reg_section_t;

typedef enum
{
    SECTION_INIT = 0, /* Ordered synchronous initialization callback. */
    SECTION_TASK = 1 /* Task descriptor collected into a resident group. */
} SECTION_E;

typedef enum
{
    SECTION_TASK_DONE = 0, /* Activation finished. */
    SECTION_TASK_RUNNING = 1 /* Step remains runnable, without retaining its C stack. */
} SECTION_TASK_STATUS_E;
typedef SECTION_TASK_STATUS_E section_task_status_t; /* Existing step source spelling. */

typedef struct reg_task
{
    uint32_t t_period; /* Release period in platform ticks, 1..INT32_MAX. */
    void (*p_func)(void); /* Run-to-completion callback, or NULL for step. */
    section_task_status_t (*p_step_func)(void *p_ctx); /* Optional continuation callback. */
    void *p_ctx; /* Business argument; lifetime protected by group state. */
    const char *p_name; /* Source diagnostic name; core deliberately does not retain it. */
} reg_task_t;

typedef struct reg_init
{
    int8_t priority; /* Ascending order, matching existing REG_INIT convention. */
    void (*p_func)(void); /* Must return; timeout enforcement belongs to the loader. */
} reg_init_t;

#define REG_SECTION_INIT(kind, item) {.section_type = (uint32_t)(kind), .p_str = (void *)&(item)}
#define REG_SECTION_FUNC(kind, obj) \
    section_item_t section_item_##obj = {.p_obj = &(obj), .p_next = NULL}; \
    SECTION_REG_ATTR_PREFIX const reg_section_t reg_section_##obj SECTION_REG_ATTR_SUFFIX = \
        REG_SECTION_INIT(kind, section_item_##obj);
#define SECTION_STL_TICKS(period) \
    (((uint64_t)(period) < UINT64_C(2147483648)) ? (uint32_t)(period) : 0u)
#define SECTION_STL_MS_TICKS(period) \
    ((((uint64_t)(period) <= UINT64_MAX / UINT64_C(1000)) && \
      (((uint64_t)(period) * UINT64_C(1000)) % SECTION_SYS_TICK_UNIT_US == 0u) && \
      (((uint64_t)(period) * UINT64_C(1000)) / SECTION_SYS_TICK_UNIT_US < UINT64_C(2147483648))) ? \
         (uint32_t)(((uint64_t)(period) * UINT64_C(1000)) / SECTION_SYS_TICK_UNIT_US) : 0u)
#define REG_TASK_RECORD(period, func) \
    {.t_period = SECTION_STL_TICKS(period), .p_func = (func), .p_step_func = NULL, .p_ctx = NULL, .p_name = #func}
#define REG_TASK_STEP_RECORD(period, func, ctx, name) \
    {.t_period = SECTION_STL_TICKS(period), .p_func = NULL, .p_step_func = (func), .p_ctx = (ctx), .p_name = #name}
#define REG_TASK(period, func) \
    reg_task_t reg_task_##func = REG_TASK_RECORD(period, func); \
    REG_SECTION_FUNC(SECTION_TASK, reg_task_##func)
#define REG_TASK_MS(period, func) REG_TASK(SECTION_STL_MS_TICKS(period), func)
#define REG_TASK_STEP_CTX(period, func, ctx) \
    reg_task_t reg_task_##func = REG_TASK_STEP_RECORD(period, func, ctx, func); \
    REG_SECTION_FUNC(SECTION_TASK, reg_task_##func)
#define REG_TASK_STEP(period, func) \
    static section_task_status_t section_task_step_wrap_##func(void *p_ctx) \
    { (void)p_ctx; return func(); } \
    reg_task_t reg_task_##func = REG_TASK_STEP_RECORD(period, section_task_step_wrap_##func, NULL, func); \
    REG_SECTION_FUNC(SECTION_TASK, reg_task_##func)
#define REG_TASK_STEP_MS(period, func) REG_TASK_STEP(SECTION_STL_MS_TICKS(period), func)
#define REG_TASK_STEP_CTX_MS(period, func, ctx) REG_TASK_STEP_CTX(SECTION_STL_MS_TICKS(period), func, ctx)
#define REG_INIT_RECORD(prio, func) {.priority = (int8_t)(prio), .p_func = (func)}
#define REG_INIT(prio, func) \
    reg_init_t reg_init_##func = REG_INIT_RECORD(prio, func); \
    REG_SECTION_FUNC(SECTION_INIT, reg_init_##func)

/** Public APIs require Thread mode or an IRQ admitted by the M priority ceiling.
 * Fast control IRQs and NMI/Fault receive BAD_CONTEXT before shared-state access.
 * Priority configuration must remain fixed while dispatch is active. */
/** @param now Initial monotonic platform tick. @return OK once per cold boot. */
SECTION_STL_STATUS_E section_stl_init(uint32_t now);
/** @param group_id Loader-owned nonzero identity.
 * @param p_entries Readable section table, copied before returning.
 * @param count Bounded number of records; non-task entries are left to other services.
 * @param p_handle Output resident incarnation. @return Registration result, initially disabled. */
SECTION_STL_STATUS_E section_stl_group_register(uint32_t group_id, const reg_section_t *p_entries,
                                               uint32_t count, section_stl_handle_t *p_handle);
/** @param handle Registered disabled group.
 * @param p_entries Readable table containing its INIT records.
 * @param count Number of records. @return OK after synchronous callbacks return; never runs scheduler. */
SECTION_STL_STATUS_E section_stl_group_initialize(section_stl_handle_t handle,
                                                 const reg_section_t *p_entries, uint32_t count);
/** @param handle Initialized incarnation. @return OK when periodic entry is enabled. */
SECTION_STL_STATUS_E section_stl_group_enable(section_stl_handle_t handle);
/** @param handle Incarnation to drain. @return OK after closing new activations; saved work may run. */
SECTION_STL_STATUS_E section_stl_group_disable(section_stl_handle_t handle);
/** @param handle Incarnation to pause. @return BUSY without changing state if CPU/snapshot/lease exists. */
SECTION_STL_STATUS_E section_stl_group_pause(section_stl_handle_t handle);
/** @param handle Incarnation to cancel. @return BUSY while CPU/init in flight; request still closes entry. */
SECTION_STL_STATUS_E section_stl_group_revoke(section_stl_handle_t handle);
/** @param handle Incarnation whose external resource is being acquired.
 * @return OK and one resident lease; callbacks/DMA must acquire before publishing external pointers. */
SECTION_STL_STATUS_E section_stl_group_acquire(section_stl_handle_t handle);
/** @param handle Original lease identity. @return OK after release; stale releases never affect a new group. */
SECTION_STL_STATUS_E section_stl_group_release(section_stl_handle_t handle);
/** Native attestation: caller has removed IRQ/callback ingress, stopped DMA and cleaned abandoned resources.
 * @param handle Revoked incarnation. @return OK only with no core CPU/lease/record references. */
SECTION_STL_STATUS_E section_stl_group_quiescent(section_stl_handle_t handle);
/** @param handle Quiescent incarnation. @return OK authorizes loader reuse of its code/data memory. */
SECTION_STL_STATUS_E section_stl_group_forget(section_stl_handle_t handle);
/** @param handle Exact incarnation. @param p_info Output lifecycle evidence. @return Query result. */
SECTION_STL_STATUS_E section_stl_group_query(section_stl_handle_t handle, section_stl_group_info_t *p_info);
/** @param now Monotonic modulo-32-bit tick; advances less than 2^31. @return Tick validation result. */
SECTION_STL_STATUS_E section_stl_tick(uint32_t now);
/** @return OK after native scheduling returns, or admission/fault error. */
SECTION_STL_STATUS_E section_stl_run(void);
/** @return OK if a current task requested a safe switch, BAD_CONTEXT otherwise. */
SECTION_STL_STATUS_E section_stl_yield(void);
/** Request a safe return to the native MSP caller. Requires an active dispatch.
 * Snapshot exhaustion preserves the current task and this request; response is not unconditionally bounded. */
SECTION_STL_STATUS_E section_stl_request_native(void);
/** Resident host-only weak hook, called under the kernel priority ceiling.
 * @return 0 to preserve the current frame without modifying queues or slice state. */
uint32_t section_stl_host_switch_allowed(void);
/** Resident host-only weak hook for each new/resumed/inline task and native return.
 * @param owner Exact group identity, or all-zero for native. No business callbacks allowed.
 * @return 0 locks a CORRUPT isolation fault; no business frame is restored. */
uint32_t section_stl_host_owner_commit(section_stl_handle_t owner);
/** @param p_stats Output resident diagnostics; does not inspect business memory.
 * In a rejected fast IRQ, output is zero except fault=BAD_CONTEXT; global stats are unchanged. */
void section_stl_stats_get(section_stl_stats_t *p_stats);
/** Port-only, protected MSP transaction. @param p_saved_sp Full task frame or NULL on native start.
 * @return Selected task frame, or NULL to restore the native caller. */
uint32_t *section_stl_switch(uint32_t *p_saved_sp);
/** Port-only task trampoline; never returns through a business return address. */
void section_stl_entry(void);

#endif /* SECTION_STL_SECTION_H */
