// SPDX-License-Identifier: MIT
/**
 * @file    scope.h
 * @brief   Scope Section adapter public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Preserve existing Scope APIs and instance macros
 *          - Register Scope instances in SECTION_SCOPE
 *          - Own the Section list consumed by communication services
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Capture algorithms are implemented by scope_core.c
 *          - Protocol handling belongs to scope_service.c
 *
 * @author  Max.Li
 * @date    2026-08-02
 * @version 2.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __SCOPE_H__
#define __SCOPE_H__

#include "scope_core.h"
#include "section.h"

extern section_item_t *p_scope_first;

typedef struct
{
    scope_t *p_scope;
    const char *p_name;
    uint32_t sample_period_us;
    uint32_t capture_tag;
    scope_state_e last_state;
    uint8_t scope_id;
    uint8_t data_ready;
} scope_registration_t;

void scope_init(void);
void scope_run(scope_t *p_scope);
void scope_start(scope_t *p_scope);
void scope_stop(scope_t *p_scope);
void scope_trigger(scope_t *p_scope);
void scope_reset(scope_t *p_scope);

#if (SCOPE_ENABLE == 1u)
#define SCOPE_RUN(name) scope_run(&scope_##name)
#define SCOPE_TRIGGER(name) scope_trigger(&scope_##name)
#define SCOPE_GET_BUFFER(name) (scope_##name.buffer)
#define SCOPE_GET_BUFFER_SIZE(name) (scope_##name.buffer_size)
#define SCOPE_GET_VAR_NUM(name) (scope_##name.var_count)
#define SCOPE_GET_VAR_PTRS(name) (scope_##name.var_ptrs)

#define SCOPE_DEFINE(name, buf_size, trig_post_cnt, sample_us, ...)        \
    SCOPE_CORE_DEFINE(name, buf_size, trig_post_cnt, __VA_ARGS__);         \
    scope_registration_t scope_registration_##name = {                    \
        .p_scope = &scope_##name,                                          \
        .p_name = #name,                                                   \
        .sample_period_us = (sample_us),                                   \
        .last_state = SCOPE_STATE_IDLE,                                    \
    }

#define REG_SCOPE(name, buf_size, trig_post_cnt, ...)                \
    SCOPE_DEFINE(name, buf_size, trig_post_cnt, 1000u, __VA_ARGS__); \
    REG_SECTION_FUNC(SECTION_SCOPE, scope_registration_##name)

#define REG_SCOPE_EX(name, buf_size, trig_post_cnt, _sample_period_us, ...)        \
    SCOPE_DEFINE(name, buf_size, trig_post_cnt, (_sample_period_us), __VA_ARGS__); \
    REG_SECTION_FUNC(SECTION_SCOPE, scope_registration_##name)
#else
#define SCOPE_RUN(name) ((void)0)
#define SCOPE_TRIGGER(name) ((void)0)
#define SCOPE_GET_BUFFER(name) NULL
#define SCOPE_GET_BUFFER_SIZE(name) 0u
#define SCOPE_GET_VAR_NUM(name) 0u
#define SCOPE_GET_VAR_PTRS(name) NULL
#define REG_SCOPE(name, buf_size, trig_post_cnt, ...) float __VA_ARGS__;
#define REG_SCOPE_EX(name, buf_size, trig_post_cnt, _sample_period_us, ...) float __VA_ARGS__;
#endif

#endif /* __SCOPE_H__ */
