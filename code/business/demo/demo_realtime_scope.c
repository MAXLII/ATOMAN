// SPDX-License-Identifier: MIT
/**
 * @file demo_realtime_scope.c
 * @brief Real RAM waveforms for the real-time oscilloscope demo.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Publish nested scalar members for ELF/DWARF channel selection
 *          - Register writable demo amplitude and frequency on the normal parameter link
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
#include "shell.h"
#include <string.h>

#define DEMO_REALTIME_SCOPE_DEFAULT_HZ 50u /**< Shared initial waveform frequency; never the sampling rate. */

static float s_demo_scope_amplitude       = 2.0f; /**< Sine peak; cosine and triangle retain half this peak. */
static uint32_t s_demo_scope_frequency    = DEMO_REALTIME_SCOPE_DEFAULT_HZ; /**< Writable waveform Hz, 1..500. */
static volatile uint32_t active_frequency = DEMO_REALTIME_SCOPE_DEFAULT_HZ;
static volatile uint32_t active_gain_bits = UINT32_C(0x3F800000); /**< IEEE754 1.0; default ISR path has no float arithmetic. */

static void demo_realtime_scope_settings_changed(DEC_MY_PRINTF)
{
    (void)my_printf;

    if (    !(    s_demo_scope_amplitude >= 0.0f
               && s_demo_scope_amplitude <= 20.0f)
         || s_demo_scope_frequency < 1u
         || s_demo_scope_frequency > 500u)
        return;
    float gain = s_demo_scope_amplitude * 0.5f;
    uint32_t bits;
    memcpy(&bits, &gain, sizeof(bits));
    /* Task-context callback publishes independent aligned 32-bit settings.
     * The ISR loads each once; parameter changes do not reset waveform phase. */
    active_gain_bits = bits;
    active_frequency = s_demo_scope_frequency;
}
REG_SHELL_VAR(DEMO_SCOPE_AMPLITUDE, s_demo_scope_amplitude, SHELL_FP32, 20.0f, 0.0f,
              demo_realtime_scope_settings_changed, SHELL_STA_NULL)
REG_SHELL_VAR(DEMO_SCOPE_FREQUENCY, s_demo_scope_frequency, SHELL_UINT32, 500u, 1u,
              demo_realtime_scope_settings_changed, SHELL_STA_NULL)

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
    uint32_t frequency          = realtime_scope_service_source_frequency_get(); /**< Actual source Hz supplied by Interface. */
    uint32_t waveform_frequency = active_frequency;

    if (    (waveform_frequency > frequency / 2u) /* Do not publish an aliased waveform if used with a slower sampling source. */
         || (frequency > UINT32_MAX / 625u))      /* Keep lookup-position multiplication within 32 bits. */
        return;
    phase += waveform_frequency;

    if (phase >= frequency)
        phase %= frequency;
    uint32_t index = phase * 625u / frequency; /**< Read-only table position; exactly 625 samples per 50 Hz period at 31250 Hz. */
    demo_realtime_scope_wave_t sample = waveform_table[index];
    uint32_t gain_bits                = active_gain_bits;

    if (gain_bits != UINT32_C(0x3F800000))
    {
        float gain;
        memcpy(&gain, &gain_bits, sizeof(gain));
        sample.sine *= gain;
        sample.cosine *= gain;
        sample.triangle *= gain;
    }
    g_scope_demo.wave = sample;
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
