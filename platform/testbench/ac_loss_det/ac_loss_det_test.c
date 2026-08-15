// SPDX-License-Identifier: MIT
/**
 * @file    ac_loss_det_test.c
 * @brief   Host test cases for the production AC loss detector.
 * @details
 *          This file is part of the base digital power framework project.
 *
 *          Module responsibilities:
 *          - Register ac_loss_det and its input-generation process in the common testbench
 *          - Exercise startup qualification, healthy operation, loss detection, and reset
 *          - Record deterministic state transitions and check final DUT state
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Compiles code/lib/ac_loss_det.c directly as the production DUT
 *          - Uses a deterministic 50 Hz-equivalent square wave at the DUT sample rate
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

#include "ac_loss_det_test.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "testbench.h"

#define AC_LOSS_DET_TEST_HALF_PERIOD_SAMPLES UINT32_C(100)
#define AC_LOSS_DET_TEST_PERIOD_SAMPLES UINT32_C(200)
#define AC_LOSS_DET_TEST_PEAK_VOLTAGE_V (325.0f)
#define AC_LOSS_DET_TEST_RUN_LIMIT_S (500.0 * (double)AC_LOSS_DET_TS)

static ac_loss_det_test_fixture_t fixture; /**< Fixture reset before every registered case. */
static FILE *p_waveform_file;              /**< CSV waveform file owned by the active test case. */
static uint8_t waveform_write_ok;          /**< 1 while the active CSV waveform file remains valid. */

static void process_record(double time_s);

/**
 * @brief Open one case-specific CSV waveform file and write its column names.
 * @param p_path Relative output path below the AC loss detector test directory.
 */
static void waveform_open(const char *p_path)
{
    int write_result = 0; /**< Result returned while writing the CSV header. */

    if (p_waveform_file != NULL)
    {
        (void)fclose(p_waveform_file);
        p_waveform_file = NULL;
    }

    waveform_write_ok = 0u;
    p_waveform_file = fopen(p_path, "w");
    if (p_waveform_file == NULL)
    {
        (void)printf("    CSV OPEN FAIL | path=%s\n", p_path);
        return;
    }

    write_result = fprintf(p_waveform_file,
                           "time_s,voltage_v,ac_is_ok,state,buffer_index,"
                           "ovf_diff_count,is_loss,healthy_seen\n");
    if (write_result >= 0)
    {
        waveform_write_ok = 1u;
    }
}

/**
 * @brief Close the active CSV waveform file and report its generation result.
 * @return
 *         0: the file could not be opened, written, or closed.
 *         1: the complete waveform file was generated successfully.
 */
static uint8_t waveform_close(void)
{
    int close_result = EOF; /**< Result returned while flushing and closing the CSV file. */

    if (p_waveform_file != NULL)
    {
        close_result = fclose(p_waveform_file);
        p_waveform_file = NULL;
        if (close_result != 0)
        {
            waveform_write_ok = 0u;
        }
    }

    (void)printf("    CHECK CSV waveform | expected=1 actual=%" PRIu8 " %s\n",
                 waveform_write_ok,
                 (waveform_write_ok == 1u) ? "PASS" : "FAIL");
    return waveform_write_ok;
}

/**
 * @brief Return the deterministic periodic AC voltage for one sample.
 * @param sample_index Zero-based sample index at the DUT rate.
 * @return Positive or negative square-wave voltage, in V.
 */
static float periodic_voltage_get(uint32_t sample_index)
{
    uint32_t phase = 0u; /**< Sample position within the periodic waveform. */

    phase = sample_index % AC_LOSS_DET_TEST_PERIOD_SAMPLES;
    if (phase < AC_LOSS_DET_TEST_HALF_PERIOD_SAMPLES)
    {
        return AC_LOSS_DET_TEST_PEAK_VOLTAGE_V;
    }
    return -AC_LOSS_DET_TEST_PEAK_VOLTAGE_V;
}

/**
 * @brief Reset all fixture-owned state and select a scenario.
 * @param scenario Input behavior used by the active case.
 * @param ac_is_ok Initial normalized AC-valid input.
 * @param p_waveform_path Relative path of the case-specific CSV waveform file.
 */
static void fixture_prepare(AC_LOSS_DET_TEST_SCENARIO_E scenario,
                            uint8_t ac_is_ok,
                            const char *p_waveform_path)
{
    (void)memset(&fixture, 0, sizeof(fixture));
    fixture.scenario = scenario;
    fixture.ac_is_ok = ac_is_ok;
    fixture.last_recorded_state = UINT32_MAX;
    fixture.last_recorded_ovf_diff_count = UINT32_MAX;
    fixture.last_recorded_loss = UINT8_MAX;
    waveform_open(p_waveform_path);
}

/**
 * @brief Initialize the production DUT while retaining the selected environment.
 */
static void dut_init(void)
{
    (void)memset(&fixture.dut, 0, sizeof(fixture.dut));
    ac_loss_det_init(&fixture.dut, &fixture.voltage_v, &fixture.ac_is_ok);
}

/**
 * @brief Generate one scenario input before the production DUT run.
 */
static void environment_before_dut(double time_s)
{
    (void)time_s;
    if (fixture.sample_index == 0u)
    {
        process_record(0.0); /* Preserve the initialized environment and DUT state as beat 0. */
    }

    if (fixture.scenario == AC_LOSS_DET_TEST_RESET_E)
    {
        fixture.dut.inter.sta = AC_LOSS_DET_STA_DET_NEG;
        fixture.dut.inter.buffer_index = 42u;
        fixture.dut.inter.ovf_diff_cnt = 12u;
        fixture.dut.output.is_loss = 0u;
        return;
    }

    if (fixture.scenario == AC_LOSS_DET_TEST_AC_UNAVAILABLE_E)
    {
        fixture.voltage_v = AC_LOSS_DET_TEST_PEAK_VOLTAGE_V;
    }
    else if ((fixture.scenario == AC_LOSS_DET_TEST_FROZEN_WAVE_E) &&
             (fixture.healthy_seen == 1u))
    {
        fixture.voltage_v = 0.0f;
    }
    else
    {
        fixture.voltage_v = periodic_voltage_get(fixture.sample_index);
    }

}

/**
 * @brief Execute one production AC loss detector operation.
 */
static void dut_run(void)
{
    if (fixture.scenario == AC_LOSS_DET_TEST_RESET_E)
    {
        ac_loss_det_reset(&fixture.dut);
    }
    else
    {
        ac_loss_det_func(&fixture.dut);
    }
    fixture.sample_index++;
}

/**
 * @brief Record DUT state changes and difference-counter activity.
 * @param time_s Simulated time elapsed since the case started, in seconds.
 */
static void process_record(double time_s)
{
    uint32_t state = 0u; /**< Current detector state converted for stable logging. */
    uint8_t should_record = 0u; /**< 1 when an observable value changed. */
    int csv_write_result = 0; /**< Result returned while appending one CSV waveform sample. */

    state = (uint32_t)fixture.dut.inter.sta;
    if (state != fixture.last_recorded_state)
    {
        should_record = 1u;
    }
    if (fixture.dut.inter.ovf_diff_cnt != fixture.last_recorded_ovf_diff_count)
    {
        should_record = 1u;
    }
    if (fixture.dut.output.is_loss != fixture.last_recorded_loss)
    {
        should_record = 1u;
    }

    if (fixture.dut.output.is_loss == 0u)
    {
        fixture.healthy_seen = 1u;
    }

    if (p_waveform_file != NULL)
    {
        csv_write_result = fprintf(p_waveform_file,
                                   "%.9f,%.6f,%" PRIu8 ",%" PRIu32
                                   ",%" PRIu32 ",%" PRIu32 ",%" PRIu8 ",%" PRIu8 "\n",
                                   time_s,
                                   (double)fixture.voltage_v,
                                   fixture.ac_is_ok,
                                   state,
                                   fixture.dut.inter.buffer_index,
                                   fixture.dut.inter.ovf_diff_cnt,
                                   fixture.dut.output.is_loss,
                                   fixture.healthy_seen);
        if (csv_write_result < 0)
        {
            waveform_write_ok = 0u;
        }
    }

    if (should_record == 1u)
    {
        (void)printf("    RECORD time_s=%.9f"
                     " voltage=%.1fV state=%" PRIu32
                     " index=%" PRIu32
                     " diff_count=%" PRIu32
                     " loss=%" PRIu8 "\n",
                     time_s,
                     (double)fixture.voltage_v,
                     state,
                     fixture.dut.inter.buffer_index,
                     fixture.dut.inter.ovf_diff_cnt,
                     fixture.dut.output.is_loss);
    }

    fixture.last_recorded_state = state;
    fixture.last_recorded_ovf_diff_count = fixture.dut.inter.ovf_diff_cnt;
    fixture.last_recorded_loss = fixture.dut.output.is_loss;
}

/**
 * @brief Check one unsigned value and print expected and actual results.
 * @param p_name Meaning of the value being checked.
 * @param expected Expected unsigned value.
 * @param actual Actual unsigned value produced by the DUT.
 * @return
 *         0: the values differ.
 *         1: the values match.
 */
static uint8_t expect_u32(const char *p_name, uint32_t expected, uint32_t actual)
{
    uint8_t passed = 0u; /**< Normalized comparison result. */

    if (actual == expected)
    {
        passed = 1u;
    }
    (void)printf("    CHECK %s | expected=%" PRIu32 " actual=%" PRIu32 " %s\n",
                 p_name,
                 expected,
                 actual,
                 (passed == 1u) ? "PASS" : "FAIL");
    return passed;
}

/**
 * @brief Prepare a case where the upstream AC-valid input is inactive.
 */
static void unavailable_init(void)
{
    fixture_prepare(AC_LOSS_DET_TEST_AC_UNAVAILABLE_E,
                    0u,
                    "build/ac_unavailable_blocks_startup.csv");
}

/**
 * @brief Prepare a case with a stable periodic AC waveform.
 */
static void healthy_init(void)
{
    fixture_prepare(AC_LOSS_DET_TEST_HEALTHY_WAVE_E,
                    1u,
                    "build/periodic_ac_becomes_healthy.csv");
}

/**
 * @brief Prepare a periodic waveform that freezes after the DUT becomes healthy.
 */
static void frozen_init(void)
{
    fixture_prepare(AC_LOSS_DET_TEST_FROZEN_WAVE_E,
                    1u,
                    "build/frozen_voltage_triggers_loss.csv");
}

/**
 * @brief Prepare direct verification of the public reset operation.
 */
static void reset_init(void)
{
    fixture_prepare(AC_LOSS_DET_TEST_RESET_E,
                    1u,
                    "build/reset_restores_runtime_state.csv");
}

/**
 * @brief Stop the unavailable-input case after a short deterministic interval.
 * @param time_s Simulated time elapsed since the case started, in seconds.
 * @return 1 after 5 DUT calls, otherwise 0.
 */
static uint8_t unavailable_finished(double time_s)
{
    (void)time_s;
    return (fixture.sample_index >= 5u) ? 1u : 0u;
}

/**
 * @brief Stop when periodic AC is accepted or the local diagnostic limit is reached.
 * @param time_s Simulated time elapsed since the case started, in seconds.
 * @return 1 when the case should stop, otherwise 0.
 */
static uint8_t healthy_finished(double time_s)
{
    if (fixture.dut.output.is_loss == 0u)
    {
        return 1u;
    }
    return (time_s >= AC_LOSS_DET_TEST_RUN_LIMIT_S) ? 1u : 0u;
}

/**
 * @brief Stop after healthy AC has been observed and the frozen input is declared lost.
 * @param time_s Simulated time elapsed since the case started, in seconds.
 * @return 1 when loss is detected or the local diagnostic limit is reached.
 */
static uint8_t frozen_finished(double time_s)
{
    if (fixture.healthy_seen == 1u)
    {
        if (fixture.dut.output.is_loss == 1u)
        {
            if (fixture.dut.inter.sta == AC_LOSS_DET_STA_IDLE)
            {
                return 1u;
            }
        }
    }
    return (time_s >= AC_LOSS_DET_TEST_RUN_LIMIT_S) ? 1u : 0u;
}

/**
 * @brief Stop after one invocation of the public reset operation.
 * @param time_s Simulated time elapsed since the case started, in seconds.
 * @return 1 after one DUT call, otherwise 0.
 */
static uint8_t reset_finished(double time_s)
{
    (void)time_s;
    return (fixture.sample_index >= 1u) ? 1u : 0u;
}

/**
 * @brief Verify that inactive AC validity holds the detector in its loss state.
 * @return 1 when all final-state checks pass, otherwise 0.
 */
static uint8_t unavailable_assert(void)
{
    uint8_t passed = 1u; /**< Aggregate assertion result. */

    if (expect_u32("state", (uint32_t)AC_LOSS_DET_STA_IDLE, (uint32_t)fixture.dut.inter.sta) == 0u)
    {
        passed = 0u;
    }
    if (expect_u32("loss", 1u, fixture.dut.output.is_loss) == 0u)
    {
        passed = 0u;
    }
    if (expect_u32("buffer index", 0u, fixture.dut.inter.buffer_index) == 0u)
    {
        passed = 0u;
    }
    if (waveform_close() == 0u)
    {
        passed = 0u;
    }
    return passed;
}

/**
 * @brief Verify that a complete periodic waveform establishes healthy AC.
 * @return 1 when all final-state checks pass, otherwise 0.
 */
static uint8_t healthy_assert(void)
{
    uint8_t passed = 1u; /**< Aggregate assertion result. */

    if (expect_u32("healthy observed", 1u, fixture.healthy_seen) == 0u)
    {
        passed = 0u;
    }
    if (expect_u32("loss", 0u, fixture.dut.output.is_loss) == 0u)
    {
        passed = 0u;
    }
    if (expect_u32("state", (uint32_t)AC_LOSS_DET_STA_DET_POS, (uint32_t)fixture.dut.inter.sta) == 0u)
    {
        passed = 0u;
    }
    if (expect_u32("buffer index", 0u, fixture.dut.inter.buffer_index) == 0u)
    {
        passed = 0u;
    }
    if (waveform_close() == 0u)
    {
        passed = 0u;
    }
    return passed;
}

/**
 * @brief Verify that a frozen input is detected and resets runtime detection state.
 * @return 1 when all final-state checks pass, otherwise 0.
 */
static uint8_t frozen_assert(void)
{
    uint8_t passed = 1u; /**< Aggregate assertion result. */

    if (expect_u32("healthy observed", 1u, fixture.healthy_seen) == 0u)
    {
        passed = 0u;
    }
    if (expect_u32("loss", 1u, fixture.dut.output.is_loss) == 0u)
    {
        passed = 0u;
    }
    if (expect_u32("state", (uint32_t)AC_LOSS_DET_STA_IDLE, (uint32_t)fixture.dut.inter.sta) == 0u)
    {
        passed = 0u;
    }
    if (expect_u32("difference counter", 0u, fixture.dut.inter.ovf_diff_cnt) == 0u)
    {
        passed = 0u;
    }
    if (waveform_close() == 0u)
    {
        passed = 0u;
    }
    return passed;
}

/**
 * @brief Verify reset state while preserving production input bindings and buffer sizing.
 * @return 1 when all reset checks pass, otherwise 0.
 */
static uint8_t reset_assert(void)
{
    uint8_t passed = 1u; /**< Aggregate assertion result. */

    if (expect_u32("state", (uint32_t)AC_LOSS_DET_STA_IDLE, (uint32_t)fixture.dut.inter.sta) == 0u)
    {
        passed = 0u;
    }
    if (expect_u32("buffer index", 0u, fixture.dut.inter.buffer_index) == 0u)
    {
        passed = 0u;
    }
    if (expect_u32("difference counter", 0u, fixture.dut.inter.ovf_diff_cnt) == 0u)
    {
        passed = 0u;
    }
    if (expect_u32("loss", 1u, fixture.dut.output.is_loss) == 0u)
    {
        passed = 0u;
    }
    if (expect_u32("buffer size", AC_LOSS_DET_BUFF_SIZE, fixture.dut.inter.buffer_size) == 0u)
    {
        passed = 0u;
    }
    if (fixture.dut.input.p_v != &fixture.voltage_v)
    {
        (void)puts("    CHECK voltage binding | FAIL");
        passed = 0u;
    }
    else
    {
        (void)puts("    CHECK voltage binding | PASS");
    }
    if (fixture.dut.input.p_ac_is_ok != &fixture.ac_is_ok)
    {
        (void)puts("    CHECK AC-valid binding | FAIL");
        passed = 0u;
    }
    else
    {
        (void)puts("    CHECK AC-valid binding | PASS");
    }
    if (waveform_close() == 0u)
    {
        passed = 0u;
    }
    return passed;
}

static TESTBENCH_CASE_STATE_E assertion_state_get(uint8_t assertion_passed)
{
    if (assertion_passed == 1u)
    {
        return TESTBENCH_CASE_PASS_E;
    }
    return TESTBENCH_CASE_FAIL_E;
}

static TESTBENCH_CASE_STATE_E unavailable_after_dut(double time_s)
{
    process_record(time_s);
    if (unavailable_finished(time_s) == 0u)
    {
        return TESTBENCH_CASE_RUNNING_E;
    }
    return assertion_state_get(unavailable_assert());
}

static TESTBENCH_CASE_STATE_E healthy_after_dut(double time_s)
{
    process_record(time_s);
    if (healthy_finished(time_s) == 0u)
    {
        return TESTBENCH_CASE_RUNNING_E;
    }
    return assertion_state_get(healthy_assert());
}

static TESTBENCH_CASE_STATE_E frozen_after_dut(double time_s)
{
    process_record(time_s);
    if (frozen_finished(time_s) == 0u)
    {
        return TESTBENCH_CASE_RUNNING_E;
    }
    return assertion_state_get(frozen_assert());
}

static TESTBENCH_CASE_STATE_E reset_after_dut(double time_s)
{
    process_record(time_s);
    if (reset_finished(time_s) == 0u)
    {
        return TESTBENCH_CASE_RUNNING_E;
    }
    return assertion_state_get(reset_assert());
}

TESTBENCH_REGISTER(ac_loss_det, AC_LOSS_DET_TS, dut_init, dut_run)

TESTBENCH_CASE(ac_loss_det,
               ac_unavailable_blocks_startup,
               unavailable_init,
               environment_before_dut,
               unavailable_after_dut)

TESTBENCH_CASE(ac_loss_det,
               periodic_ac_becomes_healthy,
               healthy_init,
               environment_before_dut,
               healthy_after_dut)

TESTBENCH_CASE(ac_loss_det,
               frozen_voltage_triggers_loss,
               frozen_init,
               environment_before_dut,
               frozen_after_dut)

TESTBENCH_CASE(ac_loss_det,
               reset_restores_runtime_state,
               reset_init,
               environment_before_dut,
               reset_after_dut)
