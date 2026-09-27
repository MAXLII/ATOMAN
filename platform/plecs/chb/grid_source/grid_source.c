// SPDX-License-Identifier: MIT
/**
 * @file grid_source.c
 * @brief FRAME-configurable fundamental and harmonic grid voltage generator.
 * @details Simulation callbacks serialize parameter access and phase integration.
 * @author Max.Li
 * @date 2026-09-27
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. Licensed under the MIT License.
 */
#include "grid_source.h"
#include "plecs.h"
#include "shell.h"
#include <float.h>
#include <math.h>

static float source_v; /* Instantaneous voltage output, V. */
static float rms_v; /* Fundamental RMS voltage command, V. */
static float frequency_ref_hz; /* Target fundamental frequency, Hz. */
static float frequency_actual_hz; /* Ramped fundamental frequency, Hz. */
static float frequency_ramp_hz_ms; /* Maximum frequency slew, Hz/ms. */
static float harmonic_pu[4]; /* 3rd, 5th, 7th, 9th amplitudes relative to fundamental. */
static float harmonic_phase_rad[4]; /* Harmonic phase offsets, rad. */
static long double phase_rad; /* Continuous fundamental phase modulo 2 pi. */
static double last_time_s; /* Latest committed source timestamp, s. */

REG_SHELL_VAR(GRID_SOURCE_V, source_v, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_SOURCE_RMS_V, rms_v, SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_SOURCE_HZ, frequency_ref_hz, SHELL_FP32, 70.0f, 40.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_SOURCE_HZ_ACT, frequency_actual_hz, SHELL_FP32, 70.0f, 40.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_FREQ_RAMP_HZ_MS, frequency_ramp_hz_ms, SHELL_FP32, 10.0f, 0.001f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_H3_PU, harmonic_pu[0], SHELL_FP32, 1.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_H5_PU, harmonic_pu[1], SHELL_FP32, 1.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_H7_PU, harmonic_pu[2], SHELL_FP32, 1.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_H9_PU, harmonic_pu[3], SHELL_FP32, 1.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_H3_PHASE_RAD, harmonic_phase_rad[0], SHELL_FP32, 3.141592654f, -3.141592654f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_H5_PHASE_RAD, harmonic_phase_rad[1], SHELL_FP32, 3.141592654f, -3.141592654f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_H7_PHASE_RAD, harmonic_phase_rad[2], SHELL_FP32, 3.141592654f, -3.141592654f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_H9_PHASE_RAD, harmonic_phase_rad[3], SHELL_FP32, 3.141592654f, -3.141592654f, NULL, SHELL_STA_NULL)

void grid_source_reset(void)
{
    source_v = 0.0f;
    rms_v = 6000.0f;
    frequency_ref_hz = 50.0f;
    frequency_actual_hz = 50.0f;
    frequency_ramp_hz_ms = 10.0f;
    phase_rad = 0.0L;
    last_time_s = 0.0f;
    for (uint32_t index = 0u; index < 4u; ++index)
    {
        harmonic_pu[index] = 0.0f;
        harmonic_phase_rad[index] = 0.0f;
    }
}

float grid_source_step(double time_s)
{
    if (time_s > last_time_s)
    {
        const long double dt_s = (long double)(time_s - last_time_s); /* Elapsed source time, s. */
        const float step_hz = frequency_ramp_hz_ms * 1000.0f * (float)dt_s; /* Allowed frequency increment. */
        const float old_hz = frequency_actual_hz; /* Previous endpoint for trapezoidal integration. */
        frequency_actual_hz += fminf(step_hz, fmaxf(-step_hz, frequency_ref_hz - old_hz));
        phase_rad += 6.2831853071795864769L * dt_s
                     * 0.5L * ((long double)old_hz + (long double)frequency_actual_hz);
        phase_rad = fmodl(phase_rad, 6.2831853071795864769L);
        last_time_s = time_s;
    }
    long double wave = sinl(phase_rad); /* Unit fundamental plus harmonic contributions. */
    for (uint32_t index = 0u; index < 4u; ++index)
    {
        const long double order = (long double)(2u * index + 3u); /* Odd harmonic order. */
        wave += (long double)harmonic_pu[index]
                * sinl(order * phase_rad + (long double)harmonic_phase_rad[index]);
    }
    source_v = (float)(1.4142135623730950488L * (long double)rms_v * wave);
    return source_v;
}

void grid_source_output_step(double time_s)
{
    plecs_set_output(PLECS_OUTPUT_GRID_SOURCE_V, grid_source_step(time_s));
}

void plecs_platform_start(void)
{
    grid_source_reset(); /* Restore parameters and phase for the new simulation. */
    grid_source_output_step(0.0f); /* Initialize the source output before the first callback. */
}
