// SPDX-License-Identifier: MIT
/**
 * @file    sogi_test.c
 * @brief   Host tests for the production float SOGI quadrature signal generator.
 * @details
 *          This file is part of the base digital power framework project.
 *
 *          Module responsibilities:
 *          - Verify the in-phase and quadrature outputs of a 50 Hz steady input
 *          - Verify runtime center-frequency update while the input switches to 60 Hz
 *          - Verify harmonic rejection of a 3rd/5th harmonic distorted input
 *          - Export time-domain CSV waveforms below the local build directory
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - 12 kHz sample rate keeps 50 Hz, 60 Hz, 150 Hz, and 250 Hz windows
 *            aligned to an integer number of samples
 *          - Amplitude and phase are measured by correlation over an integer
 *            number of cycles after the transient has settled
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

#include "sogi_test.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "my_math.h"
#include "testbench.h"

#define SOGI_TEST_SAMPLE_FREQ_HZ (12000.0f)
#define SOGI_TEST_PERIOD_S (1.0f / SOGI_TEST_SAMPLE_FREQ_HZ)
#define SOGI_TEST_F0_HZ (50.0f)
#define SOGI_TEST_F1_HZ (60.0f)
#define SOGI_TEST_W0_RAD_S (M_2PI * SOGI_TEST_F0_HZ)
#define SOGI_TEST_W1_RAD_S (M_2PI * SOGI_TEST_F1_HZ)
#define SOGI_TEST_K (M_SQRT2)
#define SOGI_TEST_FUNDAMENTAL_AMP (1.0f)
#define SOGI_TEST_H3_AMP (0.2f)
#define SOGI_TEST_H5_AMP (0.1f)
#define SOGI_TEST_SETTLE_TIME_S (0.2)
#define SOGI_TEST_SWITCH_TIME_S (0.2)
#define SOGI_TEST_SWITCH_SETTLE_TIME_S (0.1)
#define SOGI_TEST_F0_MEASURE_SAMPLES UINT32_C(480)  /* 0.04 s at 12 kHz: 2 cycles of 50 Hz. */
#define SOGI_TEST_F1_MEASURE_SAMPLES UINT32_C(1200) /* 0.10 s at 12 kHz: 6 cycles of 60 Hz. */
#define SOGI_TEST_AMP_TOLERANCE (0.02f)
#define SOGI_TEST_PHASE_TOLERANCE_RAD (0.035f) /* About 2 degrees. */
#define SOGI_TEST_STEADY_ERR_RMS_LIMIT (0.01f)
#define SOGI_TEST_HARMONIC_ERR_RMS_LIMIT (0.16f)
#define SOGI_TEST_HARMONIC_THD_U_LIMIT (0.12f)

static sogi_test_fixture_t fixture;
static FILE *p_csv_file;
static uint8_t csv_write_ok;

static void csv_open(const char *p_path, const char *p_header);
static uint8_t csv_close(void);
static uint8_t expect_true(const char *p_name, uint8_t condition);
static void case_init(SOGI_TEST_SCENARIO_E scenario, const char *p_csv_path);
static void measure_accumulate(double time_s);
static void correlation_finalize(void);
static uint8_t sogi_assert(void);
static TESTBENCH_CASE_STATE_E assertion_state_get(uint8_t assertion_passed);
static void sogi_record(double time_s);
static TESTBENCH_CASE_STATE_E sogi_after_dut(double time_s);

static float sogi_input_freq_hz(double time_s)
{
    if ((fixture.scenario == SOGI_TEST_FREQ_UPDATE_E) &&
        (time_s >= (double)SOGI_TEST_SWITCH_TIME_S))
    {
        return SOGI_TEST_F1_HZ;
    }
    return SOGI_TEST_F0_HZ;
}

static float sogi_measure_ref_w(void)
{
    if (fixture.scenario == SOGI_TEST_FREQ_UPDATE_E)
    {
        return SOGI_TEST_W1_RAD_S;
    }
    return SOGI_TEST_W0_RAD_S;
}

static double sogi_measure_start_s(void)
{
    if (fixture.scenario == SOGI_TEST_FREQ_UPDATE_E)
    {
        return (double)SOGI_TEST_SWITCH_TIME_S + (double)SOGI_TEST_SWITCH_SETTLE_TIME_S;
    }
    return (double)SOGI_TEST_SETTLE_TIME_S;
}

static uint32_t sogi_measure_samples(void)
{
    if (fixture.scenario == SOGI_TEST_FREQ_UPDATE_E)
    {
        return SOGI_TEST_F1_MEASURE_SAMPLES;
    }
    return SOGI_TEST_F0_MEASURE_SAMPLES;
}

static void csv_open(const char *p_path, const char *p_header)
{
    int write_result = 0;

    if (p_csv_file != NULL)
    {
        (void)fclose(p_csv_file);
        p_csv_file = NULL;
    }

    csv_write_ok = 0u;
    p_csv_file = fopen(p_path, "w");
    if (p_csv_file == NULL)
    {
        (void)printf("    CSV OPEN FAIL | path=%s\n", p_path);
        return;
    }

    write_result = fprintf(p_csv_file, "%s\n", p_header);
    if (write_result >= 0)
    {
        csv_write_ok = 1u;
    }
}

static uint8_t csv_close(void)
{
    int close_result = EOF;

    if (p_csv_file != NULL)
    {
        close_result = fclose(p_csv_file);
        p_csv_file = NULL;
        if (close_result != 0)
        {
            csv_write_ok = 0u;
        }
    }

    (void)printf("    CHECK CSV | expected=1 actual=%u %s\n",
                 (unsigned int)csv_write_ok,
                 (csv_write_ok == 1u) ? "PASS" : "FAIL");
    return csv_write_ok;
}

static uint8_t expect_true(const char *p_name, uint8_t condition)
{
    const uint8_t passed = (condition != 0u) ? 1u : 0u;

    (void)printf("    CHECK %s | %s\n", p_name, (passed == 1u) ? "PASS" : "FAIL");
    return passed;
}

static void case_init(SOGI_TEST_SCENARIO_E scenario, const char *p_csv_path)
{
    (void)memset(&fixture, 0, sizeof(fixture));
    fixture.scenario = scenario;
    csv_open(p_csv_path, "time_s,input,osg_u,osg_qu,err");
}

static void steady_init(void)
{
    case_init(SOGI_TEST_STEADY_E, "build/sogi_steady_50hz.csv");
}

static void frequency_update_init(void)
{
    case_init(SOGI_TEST_FREQ_UPDATE_E, "build/sogi_frequency_update_60hz.csv");
}

static void harmonic_init(void)
{
    case_init(SOGI_TEST_HARMONIC_E, "build/sogi_harmonic_rejection.csv");
}

static void dut_init(void)
{
    sogi_init(&fixture.dut,
              SOGI_TEST_PERIOD_S,
              SOGI_TEST_W0_RAD_S,
              SOGI_TEST_K,
              &fixture.input);
    fixture.dut_init_ok = ((fixture.dut.p_val == &fixture.input) &&
                           (fixture.dut.Ts > 0.0f))
                              ? 1u
                              : 0u;
    sogi_record(0.0); /* Record the initialized DUT state as beat 0. */
}

static void dut_run(void)
{
    sogi_cal(&fixture.dut);
    fixture.out_u = fixture.dut.osg_u[0];
    fixture.out_qu = fixture.dut.osg_qu[0];
    fixture.err = fixture.dut.err;
}

static void sogi_before_dut(double time_s)
{
    const float freq_hz = sogi_input_freq_hz(time_s);

    /* Advance the continuous input phase, then wrap to keep the argument bounded. */
    fixture.phase_rad += SOGI_TEST_PERIOD_S * (M_2PI * freq_hz);
    if (fixture.phase_rad >= M_2PI)
    {
        fixture.phase_rad -= M_2PI;
    }
    fixture.input = SOGI_TEST_FUNDAMENTAL_AMP * sinf(fixture.phase_rad);
    if (fixture.scenario == SOGI_TEST_HARMONIC_E)
    {
        fixture.input += SOGI_TEST_H3_AMP * sinf(3.0f * fixture.phase_rad);
        fixture.input += SOGI_TEST_H5_AMP * sinf(5.0f * fixture.phase_rad);
    }

    /* Retune the SOGI at the same beat the input frequency switches. */
    if ((fixture.scenario == SOGI_TEST_FREQ_UPDATE_E) &&
        (fixture.frequency_updated == 0u) &&
        (time_s >= (double)SOGI_TEST_SWITCH_TIME_S))
    {
        sogi_update_frequency(&fixture.dut, SOGI_TEST_W1_RAD_S);
        fixture.frequency_updated = 1u;
    }
}

static void measure_accumulate(double time_s)
{
    const float w_ref = sogi_measure_ref_w();
    const float s = sinf((float)((double)w_ref * time_s));
    const float c = cosf((float)((double)w_ref * time_s));

    fixture.sd_u += (double)fixture.out_u * (double)s;
    fixture.sq_u += (double)fixture.out_u * (double)c;
    fixture.sd_qu += (double)fixture.out_qu * (double)s;
    fixture.sq_qu += (double)fixture.out_qu * (double)c;
    fixture.err_sq_sum += (double)fixture.err * (double)fixture.err;
    fixture.u_sq_sum += (double)fixture.out_u * (double)fixture.out_u;
    fixture.measure_sample_count++;
}

static void correlation_finalize(void)
{
    const uint32_t n = fixture.measure_sample_count;
    const float scale = 2.0f / (float)n;
    const double sd_u = fixture.sd_u;
    const double sq_u = fixture.sq_u;
    const double sd_qu = fixture.sd_qu;
    const double sq_qu = fixture.sq_qu;

    fixture.amp_u = scale * sqrtf((float)((sd_u * sd_u) + (sq_u * sq_u)));
    fixture.amp_qu = scale * sqrtf((float)((sd_qu * sd_qu) + (sq_qu * sq_qu)));
    fixture.phase_u_rad = atan2f((float)sq_u, (float)sd_u);
    fixture.phase_qu_rad = atan2f((float)sq_qu, (float)sd_qu);
    fixture.err_rms = sqrtf((float)(fixture.err_sq_sum / (double)n));
    {
        const double u_power = fixture.u_sq_sum / (double)n;
        const double fundamental_power = 0.5 * (double)fixture.amp_u * (double)fixture.amp_u;
        double residual_power = u_power - fundamental_power;

        /* Numeric noise can push the residual slightly negative for a pure sine. */
        if (residual_power < 0.0)
        {
            residual_power = 0.0;
        }
        fixture.thd_u = sqrtf((float)residual_power) / fixture.amp_u;
    }
}

static uint8_t sogi_assert(void)
{
    uint8_t passed = 1u;
    float phase_u_deg = 0.0f;
    float phase_qu_deg = 0.0f;

    correlation_finalize();
    phase_u_deg = fixture.phase_u_rad * (180.0f / M_PI);
    phase_qu_deg = fixture.phase_qu_rad * (180.0f / M_PI);
    (void)printf("    SOGI RESULT | samples=%u amp_u=%.6f phase_u=%.3fdeg amp_qu=%.6f phase_qu=%.3fdeg err_rms=%.6f thd_u=%.6f\n",
                 (unsigned int)fixture.measure_sample_count,
                 (double)fixture.amp_u,
                 (double)phase_u_deg,
                 (double)fixture.amp_qu,
                 (double)phase_qu_deg,
                 (double)fixture.err_rms,
                 (double)fixture.thd_u);

    if (expect_true("SOGI initialization", fixture.dut_init_ok) == 0u)
    {
        passed = 0u;
    }
    if (expect_true("in-phase amplitude",
                    (fabsf(fixture.amp_u - SOGI_TEST_FUNDAMENTAL_AMP) <= SOGI_TEST_AMP_TOLERANCE) ? 1u : 0u) == 0u)
    {
        passed = 0u;
    }
    if (expect_true("in-phase angle near 0 deg",
                    (fabsf(fixture.phase_u_rad) <= SOGI_TEST_PHASE_TOLERANCE_RAD) ? 1u : 0u) == 0u)
    {
        passed = 0u;
    }
    if (expect_true("quadrature amplitude",
                    (fabsf(fixture.amp_qu - SOGI_TEST_FUNDAMENTAL_AMP) <= SOGI_TEST_AMP_TOLERANCE) ? 1u : 0u) == 0u)
    {
        passed = 0u;
    }
    if (expect_true("quadrature angle near -90 deg",
                    (fabsf(fixture.phase_qu_rad + M_PI_2) <= SOGI_TEST_PHASE_TOLERANCE_RAD) ? 1u : 0u) == 0u)
    {
        passed = 0u;
    }
    if (fixture.scenario == SOGI_TEST_HARMONIC_E)
    {
        const float input_thd = sqrtf(0.5f * ((SOGI_TEST_H3_AMP * SOGI_TEST_H3_AMP) +
                                              (SOGI_TEST_H5_AMP * SOGI_TEST_H5_AMP))) /
                                SOGI_TEST_FUNDAMENTAL_AMP;
        if (expect_true("output THD below limit and below input THD",
                        ((fixture.thd_u < SOGI_TEST_HARMONIC_THD_U_LIMIT) &&
                         (fixture.thd_u < input_thd))
                            ? 1u
                            : 0u) == 0u)
        {
            passed = 0u;
        }
        if (expect_true("error carries the rejected harmonics",
                        (fixture.err_rms <= SOGI_TEST_HARMONIC_ERR_RMS_LIMIT) ? 1u : 0u) == 0u)
        {
            passed = 0u;
        }
    }
    else
    {
        if (expect_true("steady error near zero",
                        (fixture.err_rms <= SOGI_TEST_STEADY_ERR_RMS_LIMIT) ? 1u : 0u) == 0u)
        {
            passed = 0u;
        }
    }
    if (fixture.scenario == SOGI_TEST_FREQ_UPDATE_E)
    {
        if (expect_true("frequency update executed", fixture.frequency_updated) == 0u)
        {
            passed = 0u;
        }
        if (expect_true("center frequency set to 60 Hz",
                        (fabsf(fixture.dut.w - SOGI_TEST_W1_RAD_S) <= 1.0e-3f) ? 1u : 0u) == 0u)
        {
            passed = 0u;
        }
    }
    if (csv_close() == 0u)
    {
        passed = 0u;
    }
    return passed;
}

static TESTBENCH_CASE_STATE_E assertion_state_get(uint8_t assertion_passed)
{
    if (assertion_passed == 1u)
    {
        return TESTBENCH_CASE_PASS;
    }
    return TESTBENCH_CASE_FAIL;
}

static void sogi_record(double time_s)
{
    int write_result = 0;

    if (p_csv_file != NULL)
    {
        write_result = fprintf(p_csv_file,
                               "%.9f,%.6f,%.6f,%.6f,%.6f\n",
                               time_s,
                               (double)fixture.input,
                               (double)fixture.out_u,
                               (double)fixture.out_qu,
                               (double)fixture.err);
        if (write_result < 0)
        {
            csv_write_ok = 0u;
        }
    }
}

static TESTBENCH_CASE_STATE_E sogi_after_dut(double time_s)
{
    if ((time_s >= sogi_measure_start_s()) &&
        (fixture.measure_sample_count < sogi_measure_samples()))
    {
        measure_accumulate(time_s);
    }
    sogi_record(time_s);

    if (fixture.measure_sample_count >= sogi_measure_samples())
    {
        return assertion_state_get(sogi_assert());
    }
    return TESTBENCH_CASE_RUNNING;
}

TESTBENCH_REGISTER(sogi, SOGI_TEST_PERIOD_S, dut_init, dut_run)

TESTBENCH_CASE(sogi,
               steady_50hz,
               steady_init,
               sogi_before_dut,
               sogi_after_dut)

TESTBENCH_CASE(sogi,
               frequency_update_60hz,
               frequency_update_init,
               sogi_before_dut,
               sogi_after_dut)

TESTBENCH_CASE(sogi,
               harmonic_rejection,
               harmonic_init,
               sogi_before_dut,
               sogi_after_dut)
