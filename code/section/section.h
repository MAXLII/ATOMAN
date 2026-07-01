// SPDX-License-Identifier: MIT
/**
 * @file    section.h
 * @brief   Section framework public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define the shared descriptors stored in the linker-managed section table
 *          - Provide REG_INIT, REG_TASK, REG_INTERRUPT, REG_FSM, and link registration macros
 *          - Expose runtime dispatch APIs used by the main loop, interrupt path, and service modules
 *          - Offer optional instrumentation hook points without depending on the dbg/perf module
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

#ifndef __SECTION_H__
#define __SECTION_H__

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "platform.h"

#ifndef SECTION_PERF_ENABLE
#define SECTION_PERF_ENABLE 0u
#endif

#if (SECTION_PERF_ENABLE != 0u) && (SECTION_PERF_ENABLE != 1u)
#error "SECTION_PERF_ENABLE must be 0 or 1."
#endif

#if (SECTION_PERF_ENABLE == 1u)
typedef struct section_perf_record section_perf_record_t;
#define SECTION_PERF_RECORD_T_DECLARED 1u
#define SECTION_TASK_PERF_FIELD section_perf_record_t *p_perf_record;
#define SECTION_TASK_PERF_INIT(name) , .p_perf_record = TASK_RECORD_PERF(name)
#define SECTION_INTERRUPT_PERF_FIELD section_perf_record_t *p_perf_record;
#define SECTION_INTERRUPT_PERF_INIT(name) , .p_perf_record = INTERRUPT_RECORD_PERF(name)
#ifndef TASK_RECORD_PERF_ENABLE
#define TASK_RECORD_PERF_ENABLE 1
#endif
#ifndef INTERRUPT_RECORD_PERF_ENABLE
#define INTERRUPT_RECORD_PERF_ENABLE 1
#endif
#else
#define SECTION_TASK_PERF_FIELD
#define SECTION_TASK_PERF_INIT(name)
#define SECTION_INTERRUPT_PERF_FIELD
#define SECTION_INTERRUPT_PERF_INIT(name)
#undef TASK_RECORD_PERF_ENABLE
#define TASK_RECORD_PERF_ENABLE 0
#undef INTERRUPT_RECORD_PERF_ENABLE
#define INTERRUPT_RECORD_PERF_ENABLE 0
#endif

typedef struct
{
    uint32_t cfsr;
    uint32_t hfsr;
    uint32_t bfar;
    uint32_t mmfar;
    uint32_t exc_return;
    uint32_t msp;
    uint32_t psp;
    uint32_t stacked_lr;
    uint32_t stacked_pc;
    uint32_t stacked_xpsr;
    uint32_t task_sp;
    uint32_t task_pc;
    uint32_t task_xpsr;
    uint32_t task_stack_base;
    uint32_t task_stack_words;
    uint32_t task_frame_valid;
    uint32_t task_name;
    uint32_t task_stack_free_words;
    uint32_t task_context_pool_words;
    uint32_t task_context_pool_used;
    uint32_t task_context_pool_head;
    uint32_t task_context_pool_tail;
    uint32_t task_context_save_fail_count;
    uint32_t task_context_release_fail_count;
    uint32_t task_fault_reason;
    uint32_t task_fault_policy;
    uint32_t task_context_required_words;
    uint32_t task_runtime_stack_used_words;
} section_fault_debug_t;

extern volatile section_fault_debug_t g_section_fault_debug;

typedef struct
{
    uint32_t probe_enter_count;
    uint32_t probe_reentry_count;
    uint32_t probe_invariant_fail_count;
    uint32_t probe_max_depth;
    uint32_t probe_last_tag;
    uint32_t critical_enter_count;
    uint32_t critical_exit_count;
} section_critical_race_debug_t;

extern volatile section_critical_race_debug_t g_section_critical_race_debug;

#ifndef SECTION_TASK_RUNTIME_STACK_WORDS
#define SECTION_TASK_RUNTIME_STACK_WORDS 512u
#endif

#ifndef SECTION_TASK_CONTEXT_POOL_WORDS
#define SECTION_TASK_CONTEXT_POOL_WORDS 1024u
#endif

#ifndef SECTION_TASK_SLICE_TICKS
#define SECTION_TASK_SLICE_TICKS 10u
#endif

#ifndef SECTION_TASK_READY_BURST_MAX
#define SECTION_TASK_READY_BURST_MAX 4u
#endif

#ifndef SECTION_CRITICAL_USE_PRIMASK
#define SECTION_CRITICAL_USE_PRIMASK 0u
#endif

#ifndef SECTION_CRITICAL_RACE_PROBE_ENABLE
#define SECTION_CRITICAL_RACE_PROBE_ENABLE 0u
#endif

#ifndef SECTION_CRITICAL_RACE_PROBE_SPIN
#define SECTION_CRITICAL_RACE_PROBE_SPIN 64u
#endif

#ifndef SECTION_SRTOS_QUEUE_INTERNAL_CRITICAL
#define SECTION_SRTOS_QUEUE_INTERNAL_CRITICAL 0u
#endif

#define SECTION_TASK_CONTEXT_POOL_FAULT 0u
#define SECTION_TASK_CONTEXT_POOL_KEEP_RUNNING 1u

#define SECTION_TASK_FAULT_NONE 0u
#define SECTION_TASK_FAULT_CONTEXT_POOL_FULL 1u
#define SECTION_TASK_FAULT_PSP_OVERFLOW 2u
#define SECTION_TASK_FAULT_CONTEXT_RESTORE_OVERFLOW 3u
#define SECTION_TASK_FAULT_CONTEXT_RELEASE_ORDER 4u
#define SECTION_TASK_FAULT_RUNTIME_STACK_TOO_SMALL 5u

#ifndef SECTION_TASK_CONTEXT_POOL_FULL_POLICY
#define SECTION_TASK_CONTEXT_POOL_FULL_POLICY SECTION_TASK_CONTEXT_POOL_FAULT
#endif

#if defined(__GNUC__) || defined(__ARMCC_VERSION)
#define SECTION_TASK_STACK_ATTR __attribute__((aligned(8)))
#else
#define SECTION_TASK_STACK_ATTR
#endif

#if (SRTOS == 1)
#define SECTION_SRTOS_TASK_FIELDS     \
    uint32_t *p_sp;                   \
    uint32_t *p_stack;                \
    uint32_t *p_snapshot;             \
    uint32_t snapshot_words;          \
    uint32_t snapshot_capacity_words; \
    uint8_t state;

#define SECTION_SRTOS_TASK_INIT       \
    , .p_sp = NULL, .p_stack = NULL,  \
      .p_snapshot = NULL,             \
      .snapshot_words = 0u,           \
      .snapshot_capacity_words = 0u,  \
      .state = 0u
#else
#define SECTION_SRTOS_TASK_FIELDS
#define SECTION_SRTOS_TASK_INIT
#endif

#ifndef PERF_START
#define PERF_START(name)
#endif

#ifndef PERF_END
#define PERF_END(name)
#endif

#ifndef P_RECORD_PERF
#define P_RECORD_PERF(name) NULL
#endif

#ifndef REG_PERF_RECORD
#define REG_PERF_RECORD(name)
#endif

#ifndef REG_TASK_PERF_RECORD
#define REG_TASK_PERF_RECORD(name)
#endif

#ifndef REG_INTERRUPT_PERF_RECORD
#define REG_INTERRUPT_PERF_RECORD(name)
#endif

typedef struct
{
    void (*my_printf)(const char *__format, ...);
    void (*tx_by_dma)(char *ptr, int len);
} section_link_tx_func_t;

#define DEC_MY_PRINTF section_link_tx_func_t *my_printf

typedef enum
{
    SECTION_INIT = 0,
    SECTION_TASK,
    SECTION_INTERRUPT,
    SECTION_SHELL,
    SECTION_LINK,
    SECTION_PERF,
    SECTION_COMM,
    SECTION_COMM_ROUTE,
    SECTION_SCOPE,
    SECTION_SFRA,
} SECTION_E;

typedef struct
{
    uint32_t section_type;
    void *p_str;
} reg_section_t;

typedef struct section_link_t section_link_t;

#if defined(_MSC_VER) && !defined(__clang__)
#define SECTION_STATIC_ASSERT_JOIN_(a, b) a##b
#define SECTION_STATIC_ASSERT_JOIN(a, b) SECTION_STATIC_ASSERT_JOIN_(a, b)
#define SECTION_STATIC_ASSERT(cond, msg) \
    typedef char SECTION_STATIC_ASSERT_JOIN(section_static_assert_, __LINE__)[(cond) ? 1 : -1]
#else
#define SECTION_STATIC_ASSERT(cond, msg) _Static_assert((cond), msg)
#endif

#define REG_SECTION_INIT(_section_type, _p_str) \
    {.section_type = (uint32_t)(_section_type), .p_str = (void *)&(_p_str)}

#define REG_SECTION_FUNC(_section_type, _p_str)                      \
    SECTION_REG_ATTR_PREFIX const reg_section_t reg_section_##_p_str \
        SECTION_REG_ATTR_SUFFIX = REG_SECTION_INIT(_section_type, _p_str);

#ifdef __GNUC__
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

typedef struct reg_init
{
    int8_t priority;
    void (*p_func)(void);
    struct reg_init *p_next;
} reg_init_t;

#define REG_INIT_RECORD(prio, func) \
    {.priority = (int8_t)(prio), .p_func = (func), .p_next = NULL}

#define REG_INIT(prio, func)                                  \
    reg_init_t reg_init_##func = REG_INIT_RECORD(prio, func); \
    REG_SECTION_FUNC(SECTION_INIT, reg_init_##func)

void section_init(void);
void section_runtime_reset(void);

#if (SECTION_PERF_ENABLE == 1u)
uint32_t section_perf_task_begin(section_perf_record_t *record);
void section_perf_task_end(section_perf_record_t *record, uint32_t start_cnt);
void section_perf_task_period_set(section_perf_record_t *record, uint32_t period_us);
uint32_t FUNC_RAM section_perf_interrupt_begin(section_perf_record_t *record);
void FUNC_RAM section_perf_interrupt_end(section_perf_record_t *record, uint32_t start_cnt);
#endif

typedef enum
{
    SECTION_TASK_DONE = 0,
    SECTION_TASK_RUNNING = 1,
} section_task_status_t;

typedef section_task_status_t (*section_task_step_f)(void *ctx);

typedef struct reg_task_t
{
    uint32_t t_period;
    uint32_t time_last;
    void (*p_func)(void);
    section_task_step_f p_step_func;
    void *p_ctx;
    const char *p_name;
    SECTION_TASK_PERF_FIELD
    struct reg_task_t *p_next;
    struct reg_task_t *p_ready_next;
    uint8_t is_ready;
    uint8_t is_running;
    SECTION_SRTOS_TASK_FIELDS
} reg_task_t;

#if (TASK_RECORD_PERF_ENABLE == 1)
#define TASK_RECORD_PERF(name) P_RECORD_PERF(name)
#else
#define TASK_RECORD_PERF(name) NULL
#undef REG_TASK_PERF_RECORD
#define REG_TASK_PERF_RECORD(name)
#endif

#define REG_TASK_RECORD(period, func)         \
    {                                         \
        .t_period = (uint32_t)(period),       \
        .time_last = 0u,                      \
        .p_func = (func),                     \
        .p_step_func = NULL,                  \
        .p_ctx = NULL,                        \
        .p_name = #func SECTION_TASK_PERF_INIT(func), \
        .p_next = NULL,                       \
        .p_ready_next = NULL,                 \
        .is_ready = 0u,                       \
        .is_running = 0u SECTION_SRTOS_TASK_INIT \
    }

#define REG_TASK_STEP_RECORD(period, func, ctx, perf_name) \
    {                                                      \
        .t_period = (uint32_t)(period),                    \
        .time_last = 0u,                                   \
        .p_func = NULL,                                    \
        .p_step_func = (func),                             \
        .p_ctx = (ctx),                                    \
        .p_name = #perf_name SECTION_TASK_PERF_INIT(perf_name), \
        .p_next = NULL,                                    \
        .p_ready_next = NULL,                              \
        .is_ready = 0u,                                    \
        .is_running = 0u SECTION_SRTOS_TASK_INIT           \
    }

#define REG_TASK(period, func)                                  \
    REG_TASK_PERF_RECORD(func)                                  \
    reg_task_t reg_task_##func = REG_TASK_RECORD(period, func); \
    REG_SECTION_FUNC(SECTION_TASK, reg_task_##func)

#define REG_TASK_STEP_CTX(period, func, ctx)                                    \
    REG_TASK_PERF_RECORD(func)                                                  \
    reg_task_t reg_task_##func = REG_TASK_STEP_RECORD(period, func, ctx, func); \
    REG_SECTION_FUNC(SECTION_TASK, reg_task_##func)

#define REG_TASK_STEP(period, func)                                                                       \
    static section_task_status_t section_task_step_wrap_##func(void *ctx)                                 \
    {                                                                                                     \
        (void)ctx;                                                                                        \
        return func();                                                                                    \
    }                                                                                                     \
    REG_TASK_PERF_RECORD(func)                                                                            \
    reg_task_t reg_task_##func = REG_TASK_STEP_RECORD(period, section_task_step_wrap_##func, NULL, func); \
    REG_SECTION_FUNC(SECTION_TASK, reg_task_##func)

#define REG_TASK_MS(period, func) REG_TASK(((uint32_t)(period) * 10u), func)
#define REG_TASK_STEP_MS(period, func) REG_TASK_STEP(((uint32_t)(period) * 10u), func)
#define REG_TASK_STEP_CTX_MS(period, func, ctx) REG_TASK_STEP_CTX(((uint32_t)(period) * 10u), func, ctx)

void run_task(void);
void section_task_tick(void);
#if (SRTOS == 1)
void section_task_start(void);
void section_task_yield(void);
void section_task_complete_current(void);
void section_task_start_request(void);
uint32_t section_task_scheduler_started(void);
uint32_t section_task_switch_pending(void);
uint32_t section_task_slice_elapsed(void);
uint32_t *section_task_start_sp_get(void);
uint32_t *section_task_switch_sp(uint32_t *sp);
#endif

#define PRIORITY_NUM_MAX 16

#if (INTERRUPT_RECORD_PERF_ENABLE == 1)
#define INTERRUPT_RECORD_PERF(name) P_RECORD_PERF(name)
#else
#define INTERRUPT_RECORD_PERF(name) NULL
#undef REG_INTERRUPT_PERF_RECORD
#define REG_INTERRUPT_PERF_RECORD(name)
#endif

typedef struct reg_interrupt
{
    uint8_t priority;
    void (*p_func)(void);
    SECTION_INTERRUPT_PERF_FIELD
    struct reg_interrupt *p_next;
} reg_interrupt_t;

#define REG_INTERRUPT_RECORD(priority_num, func) \
    {.priority = (uint8_t)(priority_num), .p_func = (func) SECTION_INTERRUPT_PERF_INIT(func), .p_next = NULL}

#define REG_INTERRUPT(priority_num, func)                                            \
    REG_INTERRUPT_PERF_RECORD(func)                                                  \
    reg_interrupt_t reg_interrupt_##func = REG_INTERRUPT_RECORD(priority_num, func); \
    REG_SECTION_FUNC(SECTION_INTERRUPT, reg_interrupt_##func)

void FUNC_RAM section_interrupt(void);

typedef struct
{
    const char *p_name;
    uint32_t fsm_sta;
    void (*func_in)(void);
    void (*func_exe)(void);
    uint32_t (*func_chk)(uint32_t);
    void (*func_out)(void);
} reg_fsm_func_t;

typedef struct
{
    uint32_t fsm_sta;
    reg_fsm_func_t *p_fsm_func_table;
    uint32_t fsm_table_size;
    uint8_t fsm_sta_is_change;
    uint32_t *p_fsm_ev;
} reg_fsm_t;

#define FSM_ENTRY(sta, in, exe, chk, out) \
    {                                     \
        .p_name = #sta,                   \
        .fsm_sta = (sta),                 \
        .func_in = (in),                  \
        .func_exe = (exe),                \
        .func_chk = (chk),                \
        .func_out = (out),                \
    }

#define REG_FSM(name, init_sta, fsm_ev, ...)                                                       \
    SECTION_STATIC_ASSERT(sizeof(fsm_ev) == sizeof(uint32_t), "FSM event must be uint32_t-sized"); \
    static reg_fsm_func_t reg_fsm_func_##name##_table[] = {__VA_ARGS__};                           \
    static reg_fsm_t reg_fsm_##name = {                                                            \
        .fsm_sta = (init_sta),                                                                     \
        .p_fsm_func_table = reg_fsm_func_##name##_table,                                           \
        .fsm_table_size = sizeof(reg_fsm_func_##name##_table) / sizeof(reg_fsm_func_t),            \
        .fsm_sta_is_change = 1u,                                                                   \
        .p_fsm_ev = (uint32_t *)&(fsm_ev),                                                         \
    };                                                                                             \
    static void fsm_##name##_run(void)                                                             \
    {                                                                                              \
        section_fsm_func(&reg_fsm_##name);                                                         \
    }                                                                                              \
    REG_TASK_MS(1, fsm_##name##_run)

#define FSM_GET_STATE(name) (reg_fsm_##name.fsm_sta)
#define FSM_EXTERN_VAR(name) extern reg_fsm_t reg_fsm_##name;

void section_fsm_func(reg_fsm_t *str);

typedef void (*section_link_handler_f)(uint8_t data, DEC_MY_PRINTF, void *ctx);

typedef struct
{
    section_link_handler_f func;
    void *ctx;
} section_link_handler_item_t;

struct section_link_t
{
    uint8_t (*rx_get_byte)(uint8_t *p_data);
    DEC_MY_PRINTF;
    struct section_link_t *p_next;
    const section_link_handler_item_t *handler_arr;
    uint32_t handler_num;
    uint8_t link_id;
};

const section_link_t *section_link_first_get(void);

#define REG_LINK(link, print, _rx_get_byte, _handler_arr, _handler_num) \
    section_link_t section_link_##link = {                              \
        .rx_get_byte = (_rx_get_byte),                                  \
        .my_printf = &(print),                                          \
        .p_next = NULL,                                                 \
        .handler_arr = (_handler_arr),                                  \
        .handler_num = (uint32_t)(_handler_num),                        \
        .link_id = (uint8_t)(link),                                     \
    };                                                                  \
    REG_SECTION_FUNC(SECTION_LINK, section_link_##link)

#define EXT_LINK(link) extern section_link_t section_link_##link
#define LINK_PRINTF(link) section_link_##link.my_printf

#endif /* __SECTION_H__ */
