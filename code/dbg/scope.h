// SPDX-License-Identifier: MIT
/**
 * @file    scope.h
 * @brief   Scope core public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define the scope capture state and circular-buffer object
 *          - Provide SCOPE_DEFINE / SCOPE_* macros for scope instance creation
 *          - Expose core capture APIs (scope_run / start / stop / trigger / reset)
 *
 *          Design notes:
 *          - C11 compatible, no external framework dependencies
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
#ifndef __SCOPE_H
#define __SCOPE_H

#include "section.h"

#include <stdint.h>

typedef enum
{
    SCOPE_STATE_IDLE,
    SCOPE_STATE_RUNNING,
    SCOPE_STATE_TRIGGERED,
} scope_state_e;

typedef struct scope_t
{
    /* Capture state — used by scope.c */
    uint32_t write_index;
    uint32_t trigger_index;
    uint8_t is_triggered;
    uint8_t is_running;
    uint32_t buffer_size;
    uint32_t trigger_post_cnt;
    uint8_t var_count;
    uint32_t trigger_counter;
    uint8_t in_trigger;
    float *buffer;
    float **var_ptrs;
    const char **var_names;
    scope_state_e state;
    /* Service state — used by scope_service.c */
    uint8_t scope_id;
    uint8_t data_ready;
    scope_state_e last_state;
    uint32_t sample_period_us;
    uint32_t capture_tag;
    const char *p_name;
    struct scope_t *p_next;
} scope_t;

#define SCOPE_ADDR(x) (&x)
#define SCOPE_EXPAND(...) __VA_ARGS__

#define SCOPE_FOR_EACH_1(m, a) m(a)
#define SCOPE_FOR_EACH_2(m, a, ...) m(a), SCOPE_FOR_EACH_1(m, __VA_ARGS__)
#define SCOPE_FOR_EACH_3(m, a, ...) m(a), SCOPE_FOR_EACH_2(m, __VA_ARGS__)
#define SCOPE_FOR_EACH_4(m, a, ...) m(a), SCOPE_FOR_EACH_3(m, __VA_ARGS__)
#define SCOPE_FOR_EACH_5(m, a, ...) m(a), SCOPE_FOR_EACH_4(m, __VA_ARGS__)
#define SCOPE_FOR_EACH_6(m, a, ...) m(a), SCOPE_FOR_EACH_5(m, __VA_ARGS__)
#define SCOPE_FOR_EACH_7(m, a, ...) m(a), SCOPE_FOR_EACH_6(m, __VA_ARGS__)
#define SCOPE_FOR_EACH_8(m, a, ...) m(a), SCOPE_FOR_EACH_7(m, __VA_ARGS__)
#define SCOPE_FOR_EACH_9(m, a, ...) m(a), SCOPE_FOR_EACH_8(m, __VA_ARGS__)
#define SCOPE_FOR_EACH_10(m, a, ...) m(a), SCOPE_FOR_EACH_9(m, __VA_ARGS__)
#define SCOPE_FOR_EACH_N( \
    _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) SCOPE_FOR_EACH_##N
#define SCOPE_FOR_EACH(m, ...) \
    SCOPE_EXPAND(SCOPE_FOR_EACH_N(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)(m, __VA_ARGS__))

#define SCOPE_COUNT_ARGS_N(_, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N
#define SCOPE_COUNT_ARGS(...) \
    SCOPE_EXPAND(SCOPE_COUNT_ARGS_N(_, __VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))

#define SCOPE_STR(x) #x

#define SCOPE_DEFINE(name, buf_size, trig_post_cnt, sample_us, ...)                                                 \
    float scope_##name##_buffer[SCOPE_COUNT_ARGS(__VA_ARGS__)][buf_size];                                           \
    float __VA_ARGS__;                                                                                              \
    float *scope_##name##_var_ptrs[SCOPE_COUNT_ARGS(__VA_ARGS__)] = {SCOPE_FOR_EACH(SCOPE_ADDR, __VA_ARGS__)};      \
    const char *scope_##name##_var_names[SCOPE_COUNT_ARGS(__VA_ARGS__)] = {SCOPE_FOR_EACH(SCOPE_STR, __VA_ARGS__)}; \
    scope_t scope_##name = {                                                                                        \
        .write_index = 0u,                                                                                          \
        .trigger_index = 0u,                                                                                        \
        .is_triggered = 0u,                                                                                         \
        .is_running = 0u,                                                                                           \
        .buffer_size = buf_size,                                                                                    \
        .var_count = SCOPE_COUNT_ARGS(__VA_ARGS__),                                                                 \
        .trigger_post_cnt = trig_post_cnt,                                                                          \
        .var_ptrs = &scope_##name##_var_ptrs[0],                                                                    \
        .buffer = &scope_##name##_buffer[0][0],                                                                     \
        .var_names = &scope_##name##_var_names[0],                                                                  \
        .trigger_counter = 0u,                                                                                      \
        .in_trigger = 0u,                                                                                           \
        .state = SCOPE_STATE_IDLE,                                                                                  \
        .scope_id = 0u,                                                                                             \
        .data_ready = 0u,                                                                                           \
        .last_state = SCOPE_STATE_IDLE,                                                                             \
        .sample_period_us = (sample_us),                                                                            \
        .capture_tag = 0u,                                                                                          \
        .p_name = #name,                                                                                            \
        .p_next = NULL,                                                                                             \
    };

void scope_run(scope_t *scope);
void scope_start(scope_t *scope);
void scope_stop(scope_t *scope);
void scope_trigger(scope_t *scope);
void scope_reset(scope_t *scope);

#define SCOPE_RUN(name) scope_run(&scope_##name)
#define SCOPE_TRIGGER(name) scope_trigger(&scope_##name)
#define SCOPE_GET_BUFFER(name) (scope_##name.buffer)
#define SCOPE_GET_BUFFER_SIZE(name) (scope_##name.buffer_size)
#define SCOPE_GET_VAR_NUM(name) (scope_##name.var_count)
#define SCOPE_GET_VAR_PTRS(name) (scope_##name.var_ptrs)

/* Scope linked list — built at init from SECTION_SCOPE entries */
extern scope_t *g_scope_first;
void scope_service_init(void);
#define REG_SCOPE(name, buf_size, trig_post_cnt, ...)         \
    SCOPE_DEFINE(name, buf_size, trig_post_cnt, 1000u, __VA_ARGS__); \
    const reg_section_t reg_scope_##name AUTO_REG_SECTION = { \
        .section_type = (uint32_t)SECTION_SCOPE,              \
        .p_str = (void *)&scope_##name,                       \
    };

#define REG_SCOPE_EX(name, buf_size, trig_post_cnt, _sample_period_us, ...) \
    SCOPE_DEFINE(name, buf_size, trig_post_cnt, (_sample_period_us), __VA_ARGS__); \
    const reg_section_t reg_scope_##name AUTO_REG_SECTION = {               \
        .section_type = (uint32_t)SECTION_SCOPE,                            \
        .p_str = (void *)&scope_##name,                                     \
    };

#endif
