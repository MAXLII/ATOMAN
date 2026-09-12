// SPDX-License-Identifier: MIT
/**
 * @file    svpwm_3level_testbench.c
 * @brief   Registered periodic scenarios for the production 3-level SVPWM library.
 * @details
 *          This file is part of the base digital power framework project.
 *          - Exercise the real DUT through the common runner at 10 kHz
 *          - Check voltage reconstruction and recovery after rejected inputs
 *          - Export per-period CSV records for 4 independent scenarios
 *          C11 compatible; host-only fixture; no MCU or gate-driver simulation.
 * @author  Max.Li
 * @date    2026-09-12
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "svpwm_3level.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "testbench.h"

typedef enum
{
    SVPWM_TEST_BALANCED = 0, /* Constant 350/350 V split. */
    SVPWM_TEST_UNBALANCED,  /* Split changes from 250/450 V to 450/250 V. */
    SVPWM_TEST_RANGE,       /* Infeasible command followed by automatic recovery. */
    SVPWM_TEST_UNDERVOLTAGE /* Half-bus undervoltage followed by automatic recovery. */
} SVPWM_TEST_SCENARIO_E;

static svpwm_3level_t dut;                   /* Production modulation instance, reset per case. */
static SVPWM_TEST_SCENARIO_E scenario;       /* Scenario selected before DUT initialization. */
static SVPWM_3LEVEL_STATUS_E actual_status;  /* Status from this period's only DUT call. */
static SVPWM_3LEVEL_STATUS_E expected_status; /* Test environment's expected result. */
static FILE *p_csv;                         /* Case-owned recording stream. */
static bool passed;                         /* Sticky assertion and recording result. */
static uint32_t step;                       /* Completed input-generation periods. */
static uint32_t rejected;                   /* Number of correctly rejected periods. */
static double max_error;                    /* Largest alpha/beta reconstruction error, V. */

/** @param condition Required invariant. */
static void expect(bool condition)
{
    if (condition == false)
    {
        if (passed == true)
        {
            (void)fprintf(stderr, "SVPWM assertion failed: scenario=%d step=%lu\n",
                          (int)scenario, (unsigned long)step);
        }
        passed = false;
    }
}

/** @param selected Scenario to initialize. @param p_path Output CSV path below build/. */
static void case_init(SVPWM_TEST_SCENARIO_E selected, const char *p_path)
{
    scenario = selected;
    step = 0u;
    rejected = 0u;
    max_error = 0.0;
    passed = true;
    p_csv = fopen(p_path, "w");
    expect(p_csv != NULL);
    if (p_csv != NULL)
    {
        expect(fprintf(p_csv, "time_s,alpha_ref_v,beta_ref_v,v_dc_p,v_dc_n,status,"
                       "a_p,a_o,a_n,b_p,b_o,b_n,c_p,c_o,c_n,alpha_out_v,beta_out_v,error_v\n") > 0);
    }
}

/** @brief Configure the equal-bus scenario. */
static void balanced_init(void) { case_init(SVPWM_TEST_BALANCED, "build/balanced.csv"); }
/** @brief Configure a split-bus step scenario. */
static void unbalanced_init(void) { case_init(SVPWM_TEST_UNBALANCED, "build/unbalanced.csv"); }
/** @brief Configure command rejection and recovery. */
static void range_init(void) { case_init(SVPWM_TEST_RANGE, "build/range_recovery.csv"); }
/** @brief Configure undervoltage rejection and recovery. */
static void undervoltage_init(void) { case_init(SVPWM_TEST_UNDERVOLTAGE, "build/undervoltage_recovery.csv"); }

/** @brief Initialize a fresh production instance for every registered case. */
static void dut_init(void)
{
    const svpwm_3level_cfg_t cfg = {20.0f}; /* Each half bus must be at least 20 V. */
    expect(svpwm_3level_init(&dut, &cfg) == true);
    expect(dut.output.status == SVPWM_3LEVEL_NOT_READY);
}

/** @param time_s Simulated time supplied by the common testbench runner. */
static void before_dut(double time_s)
{
    double amplitude = 300.0;                  /* Normal rotating reference amplitude, V. */
    const double theta = 2.0 * acos(-1.0) * 50.0 * time_s; /* 50 Hz electrical angle. */
    ++step;
    dut.input.v_dc_p = 350.0f;
    dut.input.v_dc_n = 350.0f;
    expected_status = SVPWM_3LEVEL_OK;
    if (scenario == SVPWM_TEST_UNBALANCED)
    {
        dut.input.v_dc_p = (step < 500u) ? 250.0f : 450.0f;
        dut.input.v_dc_n = 700.0f - dut.input.v_dc_p;
    }
    if ((step >= 200u) && /* Fault starts after valid output has been produced. */
        (step < 400u))   /* Leave enough subsequent periods to verify recovery. */
    {
        if (scenario == SVPWM_TEST_RANGE)
        {
            amplitude = 600.0;
            expected_status = SVPWM_3LEVEL_OUT_OF_RANGE;
        }
        if (scenario == SVPWM_TEST_UNDERVOLTAGE)
        {
            dut.input.v_dc_p = 10.0f;
            expected_status = SVPWM_3LEVEL_INVALID_INPUT;
        }
    }
    dut.input.v_alpha = (float)(amplitude * cos(theta));
    dut.input.v_beta = (float)(amplitude * sin(theta));
}

/** @brief Run the real modulator exactly once for this period. */
static void dut_run(void)
{
    actual_status = svpwm_3level_cal(&dut);
}

/** @param time_s Current simulated time. @return Case progress or final assertion result. */
static TESTBENCH_CASE_STATE_E after_dut(double time_s)
{
    const svpwm_3level_phase_output_t *p_phases[3] = { /* Phase outputs from this DUT invocation. */
        &dut.output.phase_a, &dut.output.phase_b, &dut.output.phase_c
    };
    double pole[3] = {0.0}; /* Actual average pole voltages, V. */
    double alpha;          /* Reconstructed alpha voltage, V. */
    double beta;           /* Reconstructed beta voltage, V. */
    double error;          /* Maximum component error, V; NaN for invalid output. */
    expect(actual_status == expected_status);
    expect(dut.output.status == actual_status);
    for (uint32_t i = 0u; i < 3u; ++i) /* Phase index. */
    {
        if (actual_status == SVPWM_3LEVEL_OK)
        {
            expect(isfinite(p_phases[i]->duty_p) != 0);
            expect(isfinite(p_phases[i]->duty_o) != 0);
            expect(isfinite(p_phases[i]->duty_n) != 0);
            expect(p_phases[i]->duty_p >= 0.0f);
            expect(p_phases[i]->duty_p <= 1.0f);
            expect(p_phases[i]->duty_o >= 0.0f);
            expect(p_phases[i]->duty_o <= 1.0f);
            expect(p_phases[i]->duty_n >= 0.0f);
            expect(p_phases[i]->duty_n <= 1.0f);
            expect(fabs((double)p_phases[i]->duty_p + p_phases[i]->duty_o + p_phases[i]->duty_n - 1.0) < 2.0e-7);
            expect((p_phases[i]->duty_p == 0.0f) || /* N/O pair is permitted. */
                   (p_phases[i]->duty_n == 0.0f)); /* O/P pair is permitted. */
        }
        else
        {
            expect(p_phases[i]->duty_p == 0.0f);
            expect(p_phases[i]->duty_o == 0.0f);
            expect(p_phases[i]->duty_n == 0.0f);
        }
        pole[i] = ((double)p_phases[i]->duty_p * dut.input.v_dc_p) -
                  ((double)p_phases[i]->duty_n * dut.input.v_dc_n);
    }
    alpha = (2.0 * pole[0] - pole[1] - pole[2]) / 3.0;
    beta = (pole[1] - pole[2]) / sqrt(3.0);
    error = fmax(fabs(alpha - dut.input.v_alpha), fabs(beta - dut.input.v_beta));
    if (actual_status == SVPWM_3LEVEL_OK)
    {
        expect(error < 0.001);
        max_error = fmax(max_error, error);
    }
    else
    {
        ++rejected;
        alpha = nan(""); /* Invalid duties do not represent an applied zero-voltage command. */
        beta = nan("");
        error = nan("");
    }
    if (p_csv != NULL)
    {
        expect(fprintf(p_csv, "%.7f,%.9g,%.9g,%.9g,%.9g,%d,"
                       "%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g\n",
                       time_s, (double)dut.input.v_alpha, (double)dut.input.v_beta,
                       (double)dut.input.v_dc_p, (double)dut.input.v_dc_n, (int)actual_status,
                       (double)p_phases[0]->duty_p, (double)p_phases[0]->duty_o, (double)p_phases[0]->duty_n,
                       (double)p_phases[1]->duty_p, (double)p_phases[1]->duty_o, (double)p_phases[1]->duty_n,
                       (double)p_phases[2]->duty_p, (double)p_phases[2]->duty_o, (double)p_phases[2]->duty_n,
                       alpha, beta, error) > 0);
    }
    if (step < 1000u)
    {
        return TESTBENCH_CASE_RUNNING;
    }
    if ((scenario == SVPWM_TEST_RANGE) || /* Command fault must have exactly 200 rejected periods. */
        (scenario == SVPWM_TEST_UNDERVOLTAGE)) /* Same length for the undervoltage fault. */
    {
        expect(rejected == 200u);
    }
    else
    {
        expect(rejected == 0u);
    }
    if (p_csv != NULL)
    {
        expect(fclose(p_csv) == 0);
        p_csv = NULL;
    }
    (void)printf("    periods=%lu rejected=%lu max_error_v=%.9g\n",
                 (unsigned long)step, (unsigned long)rejected, max_error);
    return (passed == true) ? TESTBENCH_CASE_PASS : TESTBENCH_CASE_FAIL;
}

TESTBENCH_REGISTER(svpwm_3level, 0.0001, dut_init, dut_run)
TESTBENCH_CASE(svpwm_3level, balanced, balanced_init, before_dut, after_dut)
TESTBENCH_CASE(svpwm_3level, unbalanced_step, unbalanced_init, before_dut, after_dut)
TESTBENCH_CASE(svpwm_3level, range_recovery, range_init, before_dut, after_dut)
TESTBENCH_CASE(svpwm_3level, undervoltage_recovery, undervoltage_init, before_dut, after_dut)
