// SPDX-License-Identifier: MIT
/**
 * @file demo_realtime_scope.c
 * @brief Real RAM waveforms for the real-time oscilloscope demo.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Publish nested scalar members for ELF/DWARF channel selection
 *          - Generate sine, cosine, triangle and square waves before registered sampling
 *          - Register the demo sampling callback on the existing SECTION interrupt chain
 *
 *          Design notes:
 *          - Entity: g_scope_demo is device RAM, sampled by the normal scope core.
 *          - Prior: this demo never operates power outputs or physical peripherals.
 *          - Time: phase advances from the declared hardware sampling frequency.
 *          - Wave generation precedes sampling in the same existing control ISR.
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

#define DEMO_REALTIME_SCOPE_FREQUENCY_HZ 50u /**< Shared frequency of all demo waveforms. */
#define DEMO_REALTIME_SCOPE_PERIOD_US    (1000000u / DEMO_REALTIME_SCOPE_FREQUENCY_HZ) /**< 20 ms cycle. */

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
    uint32_t sample_count; /**< Monotonic ISR counter used to verify adjacent uploaded samples. */
} demo_realtime_scope_t;

volatile demo_realtime_scope_t g_scope_demo;

#include "demo_realtime_scope_wave.inc"

static void demo_realtime_scope_tick(void)
{
    static uint32_t phase; /**< Rational phase numerator, modulo the declared sampling frequency. */
    uint32_t frequency = realtime_scope_service_source_frequency_get(); /**< Actual source Hz supplied by Interface. */

    if (    (frequency < DEMO_REALTIME_SCOPE_FREQUENCY_HZ) /* A complete waveform needs at least 1 sample per cycle. */
         || (frequency > UINT32_MAX / 625u)) /* Keep lookup-position multiplication within 32 bits. */
        return;
    phase += DEMO_REALTIME_SCOPE_FREQUENCY_HZ;

    if (phase >= frequency)
        phase %= frequency;
    uint32_t index = phase * 625u / frequency; /**< Read-only table position; exactly 625 samples per 50 Hz period at 31250 Hz. */
    g_scope_demo.wave = waveform_table[index];
    g_scope_demo.updates++;
}

REG_INTERRUPT(254, demo_realtime_scope_tick)

#if REALTIME_SCOPE_ENABLE
static void sample_interrupt(void)
{
    ++g_scope_demo.sample_count;
    realtime_scope_service_sample();
}
REG_INTERRUPT(255, sample_interrupt)
#endif
#endif
