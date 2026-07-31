// SPDX-License-Identifier: MIT
/**
 * @file    test_inv_i32.c
 * @brief   Host tests for inverter integer support libraries.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Verify Q32-to-Q15 trigonometric lookup accuracy and quadrant boundaries
 *          - Verify the integer APF amplitude and quadrature phase at nominal frequency
 *          - Verify resonant coefficient invariants, runtime activity, and reset behavior
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Runs on the MinGW host toolchain
 *          - Does not access hardware
 *
 * @author  Max.Li
 * @date    2026-08-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "apf_i32.h"
#include "resonant_i32.h"
#include "trig_i32.h"

#define TEST_PI (3.14159265358979323846)
#define TEST_SAMPLE_FREQ_HZ (30000.0)
#define TEST_FUNDAMENTAL_HZ (50.0)

static void require_true(int condition, const char *p_message)
{
    if (condition == 0)
    {
        (void)fprintf(stderr, "FAIL: %s\n", p_message);
        exit(EXIT_FAILURE);
    }
}

static int32_t round_i32(double value)
{
    return (value >= 0.0) ? (int32_t)(value + 0.5) : (int32_t)(value - 0.5);
}

static void test_trig(void)
{
    uint32_t sample = 0U; /**< Phase sample index. */
    int32_t sin_q15 = 0;  /**< Integer sine output. */
    int32_t cos_q15 = 0;  /**< Integer cosine output. */
    int32_t max_error = 0;/**< Maximum Q15 lookup error. */

    trig_i32_sin_cos_q15(0U, &sin_q15, &cos_q15);
    require_true((sin_q15 == 0) && (cos_q15 == TRIG_I32_Q15_ONE), "trig zero axis");
    trig_i32_sin_cos_q15(0x40000000UL, &sin_q15, &cos_q15);
    require_true((sin_q15 == TRIG_I32_Q15_ONE) && (abs(cos_q15) <= 1), "trig quarter axis");
    trig_i32_sin_cos_q15(0x80000000UL, &sin_q15, &cos_q15);
    require_true((abs(sin_q15) <= 1) && (cos_q15 == -TRIG_I32_Q15_ONE), "trig half axis");

    for (sample = 0U; sample < 4096U; sample++)
    {
        uint32_t phase = sample << 20U; /**< Uniform Q32 test phase. */
        double angle = (2.0 * TEST_PI * (double)sample) / 4096.0; /**< Reference angle. */
        int32_t sin_ref = round_i32(sin(angle) * (double)TRIG_I32_Q15_ONE); /**< Reference sine. */
        int32_t cos_ref = round_i32(cos(angle) * (double)TRIG_I32_Q15_ONE); /**< Reference cosine. */
        int32_t sin_error = 0; /**< Absolute sine error. */
        int32_t cos_error = 0; /**< Absolute cosine error. */

        trig_i32_sin_cos_q15(phase, &sin_q15, &cos_q15);
        sin_error = abs(sin_q15 - sin_ref);
        cos_error = abs(cos_q15 - cos_ref);
        if (sin_error > max_error)
        {
            max_error = sin_error;
        }
        if (cos_error > max_error)
        {
            max_error = cos_error;
        }
    }
    require_true(max_error <= 2, "trig interpolation error");
}

static void test_apf(void)
{
    apf_i32_coeff_t coeff = {0}; /**< Nominal 50 Hz APF coefficients. */
    apf_i32_t apf = {0};         /**< APF runtime state. */
    uint32_t sample = 0U;        /**< Simulation sample index. */
    int32_t max_error = 0;       /**< Last-cycle error from the ideal quadrature output. */

    require_true(apf_i32_design_coeff(&coeff,
                                      (float)(2.0 * TEST_PI * TEST_FUNDAMENTAL_HZ),
                                      (float)(1.0 / TEST_SAMPLE_FREQ_HZ)),
                 "APF coefficient design");
    require_true(apf_i32_init(&apf, &coeff), "APF initialization");

    for (sample = 0U; sample < 60000U; sample++)
    {
        double angle = 2.0 * TEST_PI * TEST_FUNDAMENTAL_HZ *
                       (double)sample / TEST_SAMPLE_FREQ_HZ; /**< Input angle. */
        int32_t input = round_i32(1000.0 * sin(angle));      /**< Integer sinusoidal input. */
        int32_t expected = round_i32(-1000.0 * cos(angle));  /**< Ideal lagging quadrature output. */
        int32_t output = apf_i32_cal(&apf, input);           /**< Integer APF output. */

        if (sample >= 59400U)
        {
            int32_t error = abs(output - expected); /**< Settled last-cycle sample error. */
            if (error > max_error)
            {
                max_error = error;
            }
        }
    }
    require_true(max_error <= 15, "APF nominal quadrature accuracy");
    apf_i32_reset(&apf);
    require_true((apf.x1 == 0) && (apf.y1 == 0), "APF reset");
}

static void test_resonant(void)
{
    resonant_i32_coeff_t coeff = {0}; /**< 3rd-harmonic coefficient set. */
    resonant_i32_t resonant = {0};    /**< Resonant runtime state. */
    uint32_t sample = 0U;             /**< Simulation sample index. */
    int32_t peak_output = 0;          /**< Observed absolute output peak. */

    require_true(resonant_i32_design_coeff(&coeff,
                                           0.30f,
                                           3U,
                                           (float)(2.0 * TEST_PI * TEST_FUNDAMENTAL_HZ),
                                           (float)(1.0 / TEST_SAMPLE_FREQ_HZ),
                                           81.91f,
                                           5.1175f),
                 "resonant coefficient design");
    require_true((coeff.b0 > 0) &&
                 (coeff.b2 == -coeff.b0) &&
                 (coeff.a2 == (int32_t)(1UL << RESONANT_I32_COEFF_Q_SHIFT)),
                 "resonant coefficient invariants");
    require_true(resonant_i32_init(&resonant, &coeff), "resonant initialization");

    for (sample = 0U; sample < 30000U; sample++)
    {
        double angle = 2.0 * TEST_PI * 100.0 *
                       (double)sample / TEST_SAMPLE_FREQ_HZ; /**< Off-resonance input angle. */
        int32_t input = round_i32(1000.0 * sin(angle));      /**< Integer voltage-code input. */
        int32_t output = resonant_i32_cal(&resonant, input); /**< Integer current-code output. */
        int32_t magnitude = abs(output);                     /**< Absolute output magnitude. */

        if (magnitude > peak_output)
        {
            peak_output = magnitude;
        }
    }
    require_true((peak_output > 0) && (peak_output < 10000), "resonant bounded off-resonance output");
    resonant_i32_reset(&resonant);
    require_true((resonant.x1 == 0) &&
                 (resonant.x2 == 0) &&
                 (resonant.y1 == 0) &&
                 (resonant.y2 == 0) &&
                 (resonant.residual_q29 == 0),
                 "resonant reset");
}

int main(void)
{
    test_trig();
    test_apf();
    test_resonant();
    (void)puts("PASS: inverter int32 library tests");
    return EXIT_SUCCESS;
}
