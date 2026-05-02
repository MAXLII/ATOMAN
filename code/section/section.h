// SPDX-License-Identifier: MIT
/**
 * @file    section.h
 * @brief   Section framework public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define the shared descriptors stored in the linker-managed section table
 *          - Provide REG_INIT, REG_TASK, REG_INTERRUPT, REG_FSM, link, perf, and comm registration macros
 *          - Expose runtime dispatch APIs used by the main loop, interrupt path, and service modules
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
#include "perf.h"

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
} SECTION_E;

typedef struct
{
    uint32_t section_type;
    void *p_str;
} reg_section_t;

#ifdef IS_PLECS
#define AUTO_REG_SECTION __attribute__((__section__("section")))
#define FUNC_RAM
#else
#define AUTO_REG_SECTION __attribute__((used, __section__("section")))
#define FUNC_RAM __attribute__((section(".func_ram")))
#endif

#define REG_SECTION_FUNC(_section_type, _p_str)                   \
    const reg_section_t reg_section_##_p_str AUTO_REG_SECTION = { \
        .section_type = (uint32_t)(_section_type),                \
        .p_str = (void *)&(_p_str),                               \
    };

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

#define REG_INIT(prio, func)       \
    reg_init_t reg_init_##func = { \
        .priority = (prio),        \
        .p_func = (func),          \
        .p_next = NULL,            \
    };                             \
    REG_SECTION_FUNC(SECTION_INIT, reg_init_##func)

void section_init(void);

typedef struct reg_task_t
{
    uint32_t t_period;
    uint32_t time_last;
    void (*p_func)(void);
    section_perf_record_t *p_perf_record;
    struct reg_task_t *p_next;
} reg_task_t;

#define TASK_RECORD_PERF_ENABLE 1

#if (TASK_RECORD_PERF_ENABLE == 1)
#define TASK_RECORD_PERF(name) P_RECORD_PERF(name)
#else
#define TASK_RECORD_PERF(name) NULL
#undef REG_TASK_PERF_RECORD
#define REG_TASK_PERF_RECORD(name)
#endif

#define REG_TASK(period, func)                   \
    REG_TASK_PERF_RECORD(func)                   \
    reg_task_t reg_task_##func = {               \
        .t_period = (period),                    \
        .p_func = (func),                        \
        .time_last = 0,                          \
        .p_perf_record = TASK_RECORD_PERF(func), \
        .p_next = NULL,                          \
    };                                           \
    REG_SECTION_FUNC(SECTION_TASK, reg_task_##func)

#define REG_TASK_MS(period, func) REG_TASK(((period) * 10u), func)

void run_task(void);

#define PRIORITY_NUM_MAX 16

#define INTERRUPT_RECORD_PERF_ENABLE 1

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
    section_perf_record_t *p_perf_record;
    struct reg_interrupt *p_next;
} reg_interrupt_t;

#define REG_INTERRUPT(priority_num, func)             \
    REG_INTERRUPT_PERF_RECORD(func)                   \
    reg_interrupt_t reg_interrupt_##func = {          \
        .priority = (priority_num),                   \
        .p_func = (func),                             \
        .p_perf_record = INTERRUPT_RECORD_PERF(func), \
        .p_next = NULL,                               \
    };                                                \
    REG_SECTION_FUNC(SECTION_INTERRUPT, reg_interrupt_##func)

void section_interrupt(void);

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

#define REG_FSM(name, init_sta, fsm_ev, ...)                                            \
    static reg_fsm_func_t reg_fsm_func_##name##_table[] = {__VA_ARGS__};                \
    static reg_fsm_t reg_fsm_##name = {                                                 \
        .fsm_sta = (init_sta),                                                          \
        .p_fsm_func_table = reg_fsm_func_##name##_table,                                \
        .fsm_table_size = sizeof(reg_fsm_func_##name##_table) / sizeof(reg_fsm_func_t), \
        .fsm_sta_is_change = 1,                                                         \
        .p_fsm_ev = (uint32_t *)&(fsm_ev),                                              \
    };                                                                                  \
    static void fsm_##name##_run(void)                                                  \
    {                                                                                   \
        section_fsm_func(&reg_fsm_##name);                                              \
    }                                                                                   \
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

typedef struct section_link_t
{
    uint8_t (*rx_get_byte)(uint8_t *p_data);
    DEC_MY_PRINTF;
    struct section_link_t *p_next;
    const section_link_handler_item_t *handler_arr;
    uint32_t handler_num;
    uint8_t link_id;
} section_link_t;

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
