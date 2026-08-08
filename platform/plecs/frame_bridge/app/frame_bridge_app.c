// SPDX-License-Identifier: MIT
/**
 * @file    frame_bridge_app.c
 * @brief   PLECS FRAME bridge simulation application.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Sample the PLECS input and simulation time at each DLL interrupt callback
 *          - Calculate output as input multiplied by gain plus offset
 *          - Register simulation values in the common Shell parameter service
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The platform dispatch lock serializes simulation callbacks and FRAME requests
 *          - Hardware access is not used by this simulation project
 *
 * @author  Max.Li
 * @date    2026-08-08
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "frame_bridge_app.h"

#include "plecs.h"
#include "scope.h"
#include "section.h"
#include "shell.h"
#include "sfra.h"
#include "trace.h"

#include <math.h>
#include <windows.h>

#define FRAME_BRIDGE_TCP_PORT (5000u)

typedef struct
{
    float sim_time_s;
    float input;
    float gain;
    float offset;
    float output;
    uint32_t step_count;
} frame_bridge_state_t;

static volatile uint32_t s_trace_time = 0u;
static volatile uint32_t s_perf_counter = 0u;
static uint32_t frame_tcp_port = FRAME_BRIDGE_TCP_PORT; /**< Read-only TCP port exposed through the Shell registry. */
static frame_bridge_state_t s_state = {
    .sim_time_s = 0.0f,
    .input = 0.0f,
    .gain = 1.0f,
    .offset = 0.0f,
    .output = 0.0f,
    .step_count = 0u,
};

/**
 * @brief Validate writable PLECS parameters after a Shell update.
 * @param[in] p_io Shell output interface supplied by the parameter service; output is not required here.
 */
static void frame_bridge_parameter_changed(shell_core_io_t *p_io)
{
    (void)p_io;

    if (!isfinite(s_state.gain))
    {
        s_state.gain = 1.0f;
    }
    if (!isfinite(s_state.offset))
    {
        s_state.offset = 0.0f;
    }
    DBG_TRACE_MARK();
}

REG_SHELL_VAR(SIM_TIME_S,
              s_state.sim_time_s,
              SHELL_FP32,
              0.0f,
              0.0f,
              NULL,
              SHELL_STA_READ_ONLY)
REG_SHELL_VAR(SIM_INPUT, s_state.input, SHELL_FP32, 0.0f, 0.0f, NULL, SHELL_STA_READ_ONLY)
REG_SHELL_VAR(SIM_GAIN, s_state.gain, SHELL_FP32, 10.0f, -10.0f, frame_bridge_parameter_changed, SHELL_STA_NULL)
REG_SHELL_VAR(SIM_OFFSET,
              s_state.offset,
              SHELL_FP32,
              100.0f,
              -100.0f,
              frame_bridge_parameter_changed,
              SHELL_STA_NULL)
REG_SHELL_VAR(SIM_OUTPUT, s_state.output, SHELL_FP32, 0.0f, 0.0f, NULL, SHELL_STA_READ_ONLY)
REG_SHELL_VAR(SIM_STEP_COUNT, s_state.step_count, SHELL_UINT32, 0u, 0u, NULL, SHELL_STA_READ_ONLY)
REG_SHELL_VAR(FRAME_TCP_PORT, frame_tcp_port, SHELL_UINT32, 0u, 0u, NULL, SHELL_STA_READ_ONLY)

REG_SCOPE_EX(frame_simulation,
             512u,
             128u,
             100u,
             scope_sim_input,
             scope_sim_output,
             scope_sim_gain,
             scope_sim_offset)

REG_PERF_BASE_CNT(&s_perf_counter, 1.0e-7f)

REG_SFRA(frame_gain,
         0u,
         100.0e-6f,
         0.05f,
         10.0f,
         1000.0f,
         NULL,
         NULL)

void plecs_perf_counter_refresh(void)
{
    static LARGE_INTEGER frequency = {0};
    LARGE_INTEGER counter = {0};
    uint64_t counter_100ns = 0u;
    uint64_t whole_seconds = 0u;
    uint64_t remainder = 0u;

    if (frequency.QuadPart == 0)
    {
        (void)QueryPerformanceFrequency(&frequency);
    }
    (void)QueryPerformanceCounter(&counter);
    if (frequency.QuadPart > 0)
    {
        whole_seconds = (uint64_t)counter.QuadPart / (uint64_t)frequency.QuadPart;
        remainder = (uint64_t)counter.QuadPart % (uint64_t)frequency.QuadPart;
        counter_100ns = (whole_seconds * 10000000u) +
                        ((remainder * 10000000u) / (uint64_t)frequency.QuadPart);
        s_perf_counter = (uint32_t)counter_100ns;
    }
}

static void frame_bridge_sample(void)
{
    float input = 0.0f;
    float output = 0.0f;
    float gain = 0.0f;
    float offset = 0.0f;
    const uint32_t simulation_tick = SECTION_SYS_TICK;

    sfra_isr_pre_sample(&frame_gain);
    input = plecs_get_input(PLECS_INPUT_SIGNAL) + frame_gain_inject;
    output = (input * s_state.gain) + s_state.offset;
    gain = s_state.gain;
    offset = s_state.offset;
    s_state.sim_time_s = (float)simulation_tick * 0.0001f;
    s_state.input = input;
    s_state.output = output;
    s_state.step_count++;

    plecs_set_output(PLECS_OUTPUT_SIGNAL, output);
    s_trace_time = simulation_tick;
    scope_sim_input = input;
    scope_sim_output = output;
    scope_sim_gain = gain;
    scope_sim_offset = offset;
    SCOPE_RUN(frame_simulation);
    frame_gain_collect = output;
    sfra_isr_post_sample(&frame_gain);
}

REG_INTERRUPT(0, frame_bridge_sample)

static void frame_bridge_sfra_task(void)
{
    (void)sfra_task(&frame_gain);
}

REG_TASK_MS(1, frame_bridge_sfra_task)

void frame_bridge_state_reset(void)
{
    s_state.sim_time_s = 0.0f;
    s_state.input = 0.0f;
    s_state.gain = 1.0f;
    s_state.offset = 0.0f;
    s_state.output = 0.0f;
    s_state.step_count = 0u;

    s_trace_time = 0u;
    DBG_TRACE_BIND_TIME(&s_trace_time);
    dbg_trace_clear();
    scope_reset(&scope_frame_simulation);
    (void)sfra_reset(&frame_gain);
    DBG_TRACE_MARK();
}
