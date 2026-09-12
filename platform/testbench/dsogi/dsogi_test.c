// SPDX-License-Identifier: MIT
/**
 * @file    dsogi_test.c
 * @brief   Registered production DSOGI and PLL scenarios with analytical references.
 * @details
 *          This file is part of the base digital power framework project.
 *          Caller-owned state; C11; no dynamic allocation or hardware access.
 * @author  Max.Li
 * @date    2026-09-12
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "dsogi.h"
#include "pll.h"
#include "testbench.h"
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define TEST_TS (0.0002) /* 5 kHz sampling. */
#define TEST_PI (3.14159265358979323846) /* Double precision reference constant. */
static dsogi_t filter; /* Production DSOGI instance. */
static pll_t loop; /* Independent production PLL instance. */
static float alpha; /* Test source alpha, V. */
static float beta; /* Test source beta, V. */
static float omega; /* External SOGI center frequency, rad/s. */
static uint32_t scenario; /* Selected registered case. */
static uint32_t tick; /* Current sample number. */
static bool passed; /* Sticky case verdict. */
static bool result; /* Current DUT validity. */
static double theta; /* Analytical present input phase. */
static double frequency; /* Present source frequency, Hz. */
static double pos_amp; /* Present positive sequence amplitude, V. */
static double neg_amp; /* Present negative sequence amplitude, V. */
static double sequence_error; /* Maximum normalized sequence error after settling. */
static double phase_error; /* Maximum next-sample PLL phase error, rad. */
static double frequency_error; /* Maximum PLL frequency error, Hz. */
static FILE *p_csv; /* Per-case sampled trace. */

/** @param condition Required invariant. */
static void expect(bool condition)
{
    if (condition == false)
    {
        passed = false;
    }
}

/** @param selected Case number. */
static void select_case(uint32_t selected)
{
    char path[80] = {0}; /* Case-specific CSV output path. */
    scenario = selected;
    tick = 0u;
    passed = true;
    sequence_error = 0.0;
    phase_error = 0.0;
    frequency_error = 0.0;
    (void)snprintf(path, sizeof(path), "build/case_%lu.csv", (unsigned long)selected);
    p_csv = fopen(path, "w");
    expect(p_csv != NULL);
    if (p_csv != NULL)
    {
        (void)fprintf(p_csv, "time_s,alpha,beta,alpha_pos,beta_pos,alpha_neg,beta_neg,pll_hz,pll_theta,valid\n");
    }
}
/** @brief Pure positive sequence. */
static void positive_init(void) { select_case(0u); }
/** @brief Pure negative sequence. */
static void negative_init(void) { select_case(1u); }
/** @brief Mixed positive and negative sequences. */
static void mixed_init(void) { select_case(2u); }
/** @brief Balanced input to the DSOGI/PLL chain. */
static void pll_init_case(void) { select_case(3u); }
/** @brief Unbalanced input to the DSOGI/PLL chain. */
static void unbalanced_init(void) { select_case(4u); }
/** @brief 50 to 55 Hz frequency step. */
static void frequency_init(void) { select_case(5u); }
/** @brief 0.4 rad input phase jump. */
static void phase_init(void) { select_case(6u); }
/** @brief Voltage sag to 40 percent. */
static void sag_init(void) { select_case(7u); }
/** @brief Fifth/seventh harmonic disturbance. */
static void harmonic_init(void) { select_case(8u); }
/** @brief Invalid configuration/input, reset and instance isolation. */
static void fault_init(void) { select_case(9u); }

/** @brief Configure actual DSOGI and existing PLL independently. */
static void dut_init(void)
{
    const dsogi_cfg_t cfg = {.ts = (float)TEST_TS, .k = 1.41421356237f,
                             .omega_min = 250.0f, .omega_max = 400.0f}; /* 40..63 Hz tuning envelope. */
    alpha = 0.0f;
    beta = 0.0f;
    omega = (float)(2.0 * TEST_PI * 50.0);
    expect(dsogi_init(&filter, &cfg, &alpha, &beta, &omega));
    expect(pll_init(&loop, (float)TEST_TS, omega, 400.0f, 250.0f, 300.0f,
                    0.70710678f, 125.663706f, 85.0f, -64.0f, 0.0f,
                    &filter.output.alpha_pos, &filter.output.beta_pos));
    if (scenario == 9u)
    {
        dsogi_t other = {0}; /* Second instance for reset/isolation checks. */
        dsogi_cfg_t bad = cfg; /* Candidate invalid configuration. */
        expect(dsogi_cal(NULL) == false);
        dsogi_reset(NULL);
        expect(dsogi_init(NULL, &cfg, &alpha, &beta, &omega) == false);
        expect(dsogi_init(&other, NULL, &alpha, &beta, &omega) == false);
        expect(dsogi_cal(&other) == false);
        bad.ts = NAN;
        expect(dsogi_init(&other, &bad, &alpha, &beta, &omega) == false);
        bad = cfg;
        bad.k = 0.0f;
        expect(dsogi_init(&other, &bad, &alpha, &beta, &omega) == false);
        expect(dsogi_init(&other, &cfg, NULL, &beta, &omega) == false);
        expect(dsogi_init(&other, &cfg, &alpha, &beta, NULL) == false);
        expect(dsogi_init(&other, &cfg, &alpha, &beta, &omega));
        alpha = 50.0f;
        expect(dsogi_cal(&other));
        dsogi_reset(&filter);
        expect(other.output.alpha_pos != filter.output.alpha_pos);
        for (uint32_t fault = 0u; fault < 4u; ++fault) /* Validate recovery from non-finite, overflow and frequency faults. */
        {
            alpha = 50.0f;
            beta = 0.0f;
            omega = 314.159265f;
            if (fault == 0u) { alpha = NAN; }
            else if (fault == 1u) { beta = INFINITY; }
            else if (fault == 2u) { alpha = FLT_MAX; }
            else { omega = 0.0f; }
            expect(dsogi_cal(&other) == false);
            expect(other.output.alpha_pos == 0.0f);
            alpha = 0.0f;
            beta = 0.0f;
            omega = 314.159265f;
            expect(dsogi_cal(&other));
        }
    }
}

/** @param time_s Current testbench timestamp. */
static void before_dut(double time_s)
{
    frequency = ((scenario == 5u) && (time_s >= 0.5)) ? 55.0 : 50.0;
    theta = 2.0 * TEST_PI * 50.0 * time_s + 0.3;
    if ((scenario == 5u) && (time_s >= 0.5))
    {
        theta += 2.0 * TEST_PI * 5.0 * (time_s - 0.5);
    }
    if ((scenario == 6u) && (time_s >= 0.5))
    {
        theta += 0.4;
    }
    pos_amp = (scenario == 1u) ? 0.0 : 300.0;
    if ((scenario == 7u) && (time_s >= 0.5))
    {
        pos_amp = 120.0;
    }
    neg_amp = ((scenario == 1u) || (scenario == 2u) || (scenario == 4u)) ? 90.0 : 0.0;
    alpha = (float)(pos_amp * cos(theta) + neg_amp * cos(theta + 0.7));
    beta = (float)(pos_amp * sin(theta) - neg_amp * sin(theta + 0.7));
    if (scenario == 8u)
    {
        alpha += (float)(15.0 * cos(5.0 * theta) + 9.0 * cos(7.0 * theta));
        beta += (float)(-15.0 * sin(5.0 * theta) + 9.0 * sin(7.0 * theta));
    }
    if ((scenario >= 3u) && (scenario <= 8u))
    {
        omega = loop.output.omega; /* Previous-step PLL estimate; no algebraic feedback loop. */
    }
    else
    {
        omega = (float)(2.0 * TEST_PI * frequency);
    }
    if ((scenario == 9u) && (tick == 2500u))
    {
        alpha = NAN;
    }
}

/** @brief Run the real production chain once. */
static void dut_run(void)
{
    result = dsogi_cal(&filter);
    if ((result == true) && (scenario >= 3u) && (scenario <= 8u))
    {
        result = pll_cal(&loop);
    }
}

/** @param time_s Current testbench timestamp. @return Case verdict after 2 s. */
static TESTBENCH_CASE_STATE_E after_dut(double time_s)
{
    expect(result == !((scenario == 9u) && (tick == 2500u)));
    if (time_s > 1.7)
    {
        const double ea = fabs((double)filter.output.alpha_pos - pos_amp * cos(theta)); /* Positive alpha error. */
        const double eb = fabs((double)filter.output.beta_pos - pos_amp * sin(theta)); /* Positive beta error. */
        const double en_a = fabs((double)filter.output.alpha_neg - neg_amp * cos(theta + 0.7)); /* Negative alpha error. */
        const double en_b = fabs((double)filter.output.beta_neg + neg_amp * sin(theta + 0.7)); /* Negative beta error. */
        sequence_error = fmax(sequence_error, fmax(fmax(ea, eb), fmax(en_a, en_b)) / 300.0);
        if ((scenario >= 3u) && (scenario <= 8u))
        {
            const double delta = (double)loop.output.theta - theta - 2.0 * TEST_PI * frequency * TEST_TS; /* PLL reports next-sample phase. */
            phase_error = fmax(phase_error, fabs(atan2(sin(delta), cos(delta))));
            frequency_error = fmax(frequency_error, fabs((double)loop.output.omega / (2.0 * TEST_PI) - frequency));
            expect(loop.output.theta >= 0.0f);
            expect(loop.output.theta < 6.283186f);
        }
    }
    if ((p_csv != NULL) && ((tick % 5u) == 0u))
    {
        (void)fprintf(p_csv, "%.6f,%.6g,%.6g,%.6g,%.6g,%.6g,%.6g,%.6g,%.6g,%d\n",
                      time_s, (double)alpha, (double)beta, (double)filter.output.alpha_pos,
                      (double)filter.output.beta_pos, (double)filter.output.alpha_neg,
                      (double)filter.output.beta_neg, (double)loop.output.omega / (2.0 * TEST_PI),
                      (double)loop.output.theta, (int)result);
    }
    ++tick;
    if (tick < 10000u)
    {
        return TESTBENCH_CASE_RUNNING;
    }
    expect(sequence_error < ((scenario == 8u) ? 0.04 : 0.004));
    expect(phase_error < ((scenario == 8u) ? 0.04 : 0.01));
    expect(frequency_error < ((scenario == 8u) ? 1.0 : 0.1));
    if (p_csv != NULL)
    {
        expect(fclose(p_csv) == 0);
        p_csv = NULL;
    }
    (void)printf("    seq_error=%.6g phase_rad=%.6g freq_hz=%.6g\n",
                 sequence_error, phase_error, frequency_error);
    return (passed == true) ? TESTBENCH_CASE_PASS : TESTBENCH_CASE_FAIL;
}

TESTBENCH_REGISTER(dsogi, TEST_TS, dut_init, dut_run)
TESTBENCH_CASE(dsogi, positive, positive_init, before_dut, after_dut)
TESTBENCH_CASE(dsogi, negative, negative_init, before_dut, after_dut)
TESTBENCH_CASE(dsogi, mixed, mixed_init, before_dut, after_dut)
TESTBENCH_CASE(dsogi, pll_balanced, pll_init_case, before_dut, after_dut)
TESTBENCH_CASE(dsogi, pll_unbalanced, unbalanced_init, before_dut, after_dut)
TESTBENCH_CASE(dsogi, frequency_step, frequency_init, before_dut, after_dut)
TESTBENCH_CASE(dsogi, phase_jump, phase_init, before_dut, after_dut)
TESTBENCH_CASE(dsogi, sag, sag_init, before_dut, after_dut)
TESTBENCH_CASE(dsogi, harmonics, harmonic_init, before_dut, after_dut)
TESTBENCH_CASE(dsogi, recovery, fault_init, before_dut, after_dut)

