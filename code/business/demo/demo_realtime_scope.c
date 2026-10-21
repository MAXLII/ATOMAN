// SPDX-License-Identifier: MIT
/**
 * @file demo_realtime_scope.c
 * @brief Real RAM waveforms for the real-time oscilloscope demo.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Publish nested scalar members for ELF/DWARF channel selection
 *          - Generate sine, cosine, triangle and square waves in a registered task
 *          - Register the demo sampling callback on the existing SECTION interrupt chain
 *
 *          Design notes:
 *          - Entity: g_scope_demo is device RAM, sampled by the normal scope core.
 *          - Prior: this demo never operates power outputs or physical peripherals.
 *          - Time: phase follows elapsed platform ticks, not task invocation count.
 *          - Wave generation runs in task context; sampling runs in the existing control ISR.
 *          - Individual 32-bit fields are observations, not an atomic group snapshot.
 *
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "section.h"
#include <stdint.h>

#ifndef DEMO_REALTIME_SCOPE_ENABLE
#define DEMO_REALTIME_SCOPE_ENABLE 0
#endif

#if DEMO_REALTIME_SCOPE_ENABLE
#include "realtime_scope_service.h"

typedef struct demo_realtime_scope_wave
{
    float sine;
    float cosine;
    float triangle;
    uint32_t square;
} demo_realtime_scope_wave_t;

typedef struct demo_realtime_scope
{
    demo_realtime_scope_wave_t wave;
    uint32_t updates;
} demo_realtime_scope_t;

volatile demo_realtime_scope_t g_scope_demo;

static const float quarter_sine[26] = {0.000000f, 0.062791f, 0.125333f, 0.187381f, 0.248690f, 0.309017f, 0.368125f,
                                       0.425779f, 0.481754f, 0.535827f, 0.587785f, 0.637424f, 0.684547f, 0.728969f,
                                       0.770513f, 0.809017f, 0.844328f, 0.876307f, 0.904827f, 0.929776f, 0.951057f,
                                       0.968583f, 0.982287f, 0.992115f, 0.998027f, 1.000000f};

static float demo_realtime_scope_sine(uint32_t index)
{
    index %= 100u;
    uint32_t quadrant = index / 25u;
    uint32_t offset   = index % 25u;
    float value = quarter_sine[(quadrant & 1u) == 0u ? offset : 25u - offset];
    return quadrant < 2u ? value : -value;
}

static void demo_realtime_scope_tick(void)
{
    static uint32_t previous_tick;
    static uint32_t phase_us;
    uint32_t now           = SECTION_SYS_TICK;
    uint32_t elapsed_ticks = now - previous_tick;
    previous_tick          = now;
    uint64_t elapsed_us    = (uint64_t)elapsed_ticks * SECTION_SYS_TICK_UNIT_US;
    phase_us = (phase_us + (uint32_t)(elapsed_us % 100000u)) % 100000u;
    uint32_t index           = phase_us / 1000u;
    float position           = (float)phase_us / 100000.0f;
    g_scope_demo.wave.sine   = 2.0f * demo_realtime_scope_sine(index);
    g_scope_demo.wave.cosine = demo_realtime_scope_sine(index + 25u);
    g_scope_demo.wave.triangle = position < 0.5f ? -1.0f + 4.0f * position : 3.0f - 4.0f * position;
    g_scope_demo.wave.square = phase_us < 50000u ? 1u : 0u;
    g_scope_demo.updates++;
}

REG_TASK_MS(1, demo_realtime_scope_tick)

#if REALTIME_SCOPE_ENABLE
static void sample_interrupt(void)
{
    realtime_scope_service_sample();
}
REG_INTERRUPT(255, sample_interrupt)
#endif
#endif
