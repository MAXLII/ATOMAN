// SPDX-License-Identifier: MIT
/**
 * @file    scope.c
 * @brief   Scope core module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Sample registered floating-point variables into a column-major circular buffer
 *          - Run the idle, running, and triggered capture state machine used by waveform capture
 *          - Provide core start, stop, trigger, and reset controls without shell or communication dependencies
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
#include "scope.h"

#include <stddef.h>

__attribute__((always_inline, hot)) inline void scope_run(scope_t *scope)
{
    float *buffer;
    float **var_ptrs;
    float *buf_base;
    uint32_t write_idx;
    const uint32_t buf_size = scope->buffer_size;
    const uint32_t var_count = scope->var_count;

    if (__builtin_expect(scope->state == SCOPE_STATE_IDLE, 0))
    {
        if (scope->is_running)
        {
            scope->state = SCOPE_STATE_RUNNING;
            scope->write_index = 0u;
            scope->trigger_counter = 0u;
        }
        else
        {
            return;
        }
    }

    buffer = scope->buffer;
    write_idx = scope->write_index;
    var_ptrs = scope->var_ptrs;
    buf_base = buffer + write_idx;
    for (uint32_t i = 0u; i < var_count; ++i)
    {
        buf_base[i * buf_size] = *(var_ptrs[i]);
    }

    if (__builtin_expect(scope->state == SCOPE_STATE_RUNNING, 1) && (scope->is_triggered != 0u))
    {
        scope->trigger_index = write_idx;
        scope->is_triggered = 0u;
        scope->in_trigger = 1u;
        scope->state = SCOPE_STATE_TRIGGERED;
    }
    else if (scope->state == SCOPE_STATE_TRIGGERED)
    {
        if (++scope->trigger_counter >= scope->trigger_post_cnt)
        {
            scope->trigger_counter = 0u;
            scope->is_running = 0u;
            scope->in_trigger = 0u;
            scope->state = SCOPE_STATE_IDLE;
        }
    }

    ++write_idx;
    if (write_idx >= buf_size)
    {
        write_idx = 0u;
    }
    scope->write_index = write_idx;
}

void scope_start(scope_t *scope)
{
    if ((scope != NULL) && (scope->state == SCOPE_STATE_IDLE))
    {
        scope->is_running = 1u;
        scope->write_index = 0u;
        scope->trigger_counter = 0u;
        scope->in_trigger = 0u;
        scope->is_triggered = 0u;
    }
}

void scope_stop(scope_t *scope)
{
    if (scope != NULL)
    {
        scope->is_running = 0u;
        scope->state = SCOPE_STATE_IDLE;
        scope->in_trigger = 0u;
    }
}

void scope_trigger(scope_t *scope)
{
    if ((scope != NULL) && (scope->state == SCOPE_STATE_RUNNING))
    {
        scope->is_triggered = 1u;
    }
}

void scope_reset(scope_t *scope)
{
    if (scope != NULL)
    {
        scope->write_index = 0u;
        scope->trigger_counter = 0u;
        scope->is_triggered = 0u;
        scope->is_running = 0u;
        scope->in_trigger = 0u;
        scope->state = SCOPE_STATE_IDLE;
    }
}
