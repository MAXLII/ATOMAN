// SPDX-License-Identifier: MIT
/**
 * @file    scope.h
 * @brief   Scope core public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define the scope capture state and circular-buffer object
 *          - Provide SCOPE_DEFINE / REG_SCOPE macros for scope instance creation
 *          - Define scope_service_obj_t and scope_service_register() for the service layer
 *          - Expose core capture APIs (scope_run / start / stop / trigger / reset)
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - scope.h does NOT depend on scope_service.h (no circular include)
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

#include "my_math.h"
#include "section.h"

#include <stdint.h>

typedef enum
{
    SCOPE_STATE_IDLE,
    SCOPE_STATE_RUNNING,
    SCOPE_STATE_TRIGGERED,
} scope_state_e;

typedef struct
{
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

#define SCOPE_DEFINE(name, buf_size, trig_post_cnt, ...)                                                            \
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
    }

void scope_run(scope_t *scope);
void scope_start(scope_t *scope);
void scope_stop(scope_t *scope);
void scope_trigger(scope_t *scope);
void scope_reset(scope_t *scope);

#define SCOPE_RUN(name) scope_run(&scope_##name)
#define SCOPE(name) scope_run(&scope_##name)
#define SCOPE_TRIGGER(name) scope_trigger(&scope_##name)
#define SCOPE_GET_BUFFER(name) (scope_##name.buffer)
#define SCOPE_GET_BUFFER_SIZE(name) (scope_##name.buffer_size)
#define SCOPE_GET_VAR_NUM(name) (scope_##name.var_count)
#define SCOPE_GET_VAR_PTRS(name) (scope_##name.var_ptrs)

/* =========================================================================
 * Service object — binds a scope_t instance into the service registry
 * ========================================================================= */
typedef struct scope_service_obj_t
{
    uint8_t scope_id;
    const char *p_name;
    scope_t *p_scope;
    uint32_t sample_period_us;
    uint32_t capture_tag;
    uint8_t data_ready;
    scope_state_e last_state;
    struct scope_service_obj_t *p_next;
} scope_service_obj_t;

void scope_service_register(scope_service_obj_t *p_obj);

/* =========================================================================
 * REG_SCOPE — combines SCOPE_DEFINE with service registration
 * ========================================================================= */
#define REG_SCOPE_EX(name, buf_size, trig_post_cnt, _sample_period_us, ...) \
    SCOPE_DEFINE(name, buf_size, trig_post_cnt, __VA_ARGS__);               \
    scope_service_obj_t scope_service_obj_##name = {                        \
        .scope_id = 0u,                                                      \
        .p_name = #name,                                                     \
        .p_scope = &scope_##name,                                            \
        .sample_period_us = (_sample_period_us),                             \
        .capture_tag = 0u,                                                   \
        .data_ready = 0u,                                                    \
        .last_state = SCOPE_STATE_IDLE,                                      \
        .p_next = NULL,                                                      \
    };                                                                       \
    static void scope_service_auto_reg_##name(void)                          \
    {                                                                        \
        scope_service_register(&scope_service_obj_##name);                   \
    }                                                                        \
    REG_INIT(1, scope_service_auto_reg_##name)

#define REG_SCOPE(name, buf_size, trig_post_cnt, ...) \
    REG_SCOPE_EX(name, buf_size, trig_post_cnt, (uint32_t)(CTRL_TS * 1000000.0f + 0.5f), __VA_ARGS__)

#endif
