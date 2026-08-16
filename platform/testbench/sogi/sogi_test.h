// SPDX-License-Identifier: MIT
/**
 * @file    sogi_test.h
 * @brief   SOGI quadrature signal generator testbench fixture definitions.
 * @details
 *          This file is part of the base digital power framework project.
 *
 *          Module responsibilities:
 *          - Define the host test fixture for the production float SOGI
 *          - Share scenario state between the steady, frequency-update, and harmonic test cases
 *          - Carry deterministic correlation accumulators used by final assertions
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Uses the production sogi.c implementation directly
 *          - Runs only on the host testbench and is never called from an ISR
 *
 * @author  Max.Li
 * @date    2026-08-16
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef SOGI_TEST_H
#define SOGI_TEST_H

#include <stdint.h>

#include "sogi.h"

typedef enum
{
    SOGI_TEST_STEADY_E = 0,   /**< Pure 50 Hz input at the SOGI center frequency. */
    SOGI_TEST_FREQ_UPDATE_E,  /**< Input and center frequency switch from 50 Hz to 60 Hz. */
    SOGI_TEST_HARMONIC_E      /**< 50 Hz input distorted by 3rd and 5th harmonics. */
} SOGI_TEST_SCENARIO_E;

typedef struct sogi_test_fixture
{
    sogi_t dut;
    SOGI_TEST_SCENARIO_E scenario;
    float input;   /**< Current generated input sample. */
    float out_u;   /**< In-phase band-pass output of the current beat. */
    float out_qu;  /**< Quadrature output of the current beat. */
    float err;     /**< Input minus in-phase output of the current beat. */
    float phase_rad; /**< Continuous input phase, wrapped to [0, 2*pi). */
    uint8_t dut_init_ok;
    uint8_t frequency_updated;
    uint32_t measure_sample_count;
    double sd_u;   /**< Sum of out_u * sin(w_ref * t). */
    double sq_u;   /**< Sum of out_u * cos(w_ref * t). */
    double sd_qu;  /**< Sum of out_qu * sin(w_ref * t). */
    double sq_qu;  /**< Sum of out_qu * cos(w_ref * t). */
    double err_sq_sum; /**< Sum of err^2 over the measurement window. */
    double u_sq_sum;   /**< Sum of out_u^2 over the measurement window. */
    float amp_u;      /**< Fundamental amplitude of out_u. */
    float amp_qu;     /**< Fundamental amplitude of out_qu. */
    float phase_u_rad; /**< Fundamental phase of out_u relative to the reference. */
    float phase_qu_rad; /**< Fundamental phase of out_qu relative to the reference. */
    float err_rms;    /**< RMS of err over the measurement window. */
    float thd_u;      /**< Harmonic distortion of out_u excluding its fundamental. */
} sogi_test_fixture_t;

#endif /* SOGI_TEST_H */
