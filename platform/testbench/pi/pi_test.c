// SPDX-License-Identifier: MIT
/**
 * @file    pi_test.c
 * @brief   Host tests for a PI-controlled ideal inductor.
 * @details
 *          This file is part of the base digital power framework project.
 *
 *          Module responsibilities:
 *          - Calculate PI gains for 45-degree phase margin at 10 kHz bandwidth
 *          - Verify a current-reference step against an ideal 5 uH inductor
 *          - Run the production SFRA core and export the closed-loop Bode response
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The ideal plant is integrated once per 100 kHz control period
 *          - CSV output is written below the local build directory
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

#include "pi_test.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "my_math.h"
#include "testbench.h"

#define PI_TEST_INDUCTANCE_H (5.0e-6f)
#define PI_TEST_CONTROL_FREQ_HZ (100000.0f)
#define PI_TEST_CONTROL_PERIOD_S (1.0f / PI_TEST_CONTROL_FREQ_HZ)
#define PI_TEST_BANDWIDTH_HZ (10000.0f)
#define PI_TEST_PHASE_MARGIN_DEG (45.0f)
#define PI_TEST_STEP_TIME_S (200.0e-6f)
#define PI_TEST_STEP_CURRENT_A (10.0f)
#define PI_TEST_STABLE_ERROR_A (0.01f)
#define PI_TEST_STABLE_SAMPLE_COUNT UINT32_C(100)
#define PI_TEST_STEP_TIMEOUT_S (0.05)
#define PI_TEST_OUTPUT_LIMIT_V (100.0f)
#define PI_TEST_SFRA_START_HZ (10.0f)
#define PI_TEST_SFRA_END_HZ (20000.0f)
#define PI_TEST_SFRA_INJECT_A (0.1f)
#define PI_TEST_SFRA_POINT_COUNT UINT16_C(299)

static pi_test_fixture_t fixture;
static FILE *p_csv_file;
static uint8_t csv_write_ok;

static void step_record(double time_s);
static void plant_update(void);
static void sfra_closed_loop_plant_update(void);
static void sfra_open_loop_plant_update(void);

static float pi_kp_get(void)
{
    const float crossover_rad_s = M_2PI * PI_TEST_BANDWIDTH_HZ;

    return crossover_rad_s * PI_TEST_INDUCTANCE_H / M_SQRT2;
}

static float pi_ki_get(void)
{
    const float crossover_rad_s = M_2PI * PI_TEST_BANDWIDTH_HZ;

    return crossover_rad_s * crossover_rad_s * PI_TEST_INDUCTANCE_H / M_SQRT2;
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

    (void)printf("    CHECK CSV | expected=1 actual=%" PRIu8 " %s\n",
                 csv_write_ok,
                 (csv_write_ok == 1u) ? "PASS" : "FAIL");
    return csv_write_ok;
}

static uint8_t expect_true(const char *p_name, uint8_t condition)
{
    const uint8_t passed = (condition != 0u) ? 1u : 0u;

    (void)printf("    CHECK %s | %s\n", p_name, (passed == 1u) ? "PASS" : "FAIL");
    return passed;
}

static void fixture_reset(PI_TEST_SCENARIO_E scenario)
{
    (void)memset(&fixture, 0, sizeof(fixture));
    fixture.scenario = scenario;
}

static void sfra_frequency_prepare(void *p_context)
{
    pi_test_fixture_t *p_fixture = (pi_test_fixture_t *)p_context;

    p_fixture->current_ref_a = 0.0f;
    p_fixture->current_a = 0.0f;
    p_fixture->pi_feedback_a = 0.0f;
    p_fixture->voltage_v = 0.0f;
    p_fixture->plant_input_valid = 0u;
    pi_tustin_reset(&p_fixture->dut);
}

static void dut_init(void)
{
    fixture.dut_init_ok = pi_tustin_init(&fixture.dut,
                                         pi_kp_get(),
                                         pi_ki_get(),
                                         PI_TEST_CONTROL_PERIOD_S,
                                         PI_TEST_OUTPUT_LIMIT_V,
                                         -PI_TEST_OUTPUT_LIMIT_V,
                                         &fixture.current_ref_a,
                                         &fixture.pi_feedback_a)
                               ? 1u
                               : 0u;
    if (fixture.scenario == PI_TEST_STEP_RESPONSE_E)
    {
        step_record(0.0); /* Record the initialized step-response state as beat 0. */
    }
}

static void dut_run(void)
{
    if (pi_tustin_cal(&fixture.dut))
    {
        fixture.voltage_v = fixture.dut.output.val;
        fixture.plant_input_valid = 1u;
    }
    else
    {
        fixture.dut_init_ok = 0u;
        fixture.voltage_v = 0.0f;
        fixture.plant_input_valid = 0u;
    }
}

static void step_init(void)
{
    fixture_reset(PI_TEST_STEP_RESPONSE_E);
    csv_open("build/pi_current_step_response.csv",
             "time_s,current_ref_a,current_a,error_a,voltage_v,kp,ki,stable_count");
}

static void sfra_prepare(PI_TEST_SCENARIO_E scenario,
                         const char *p_csv_path,
                         uint8_t include_dc_point)
{
    sfra_status_t status = SFRA_STATUS_OK;

    fixture_reset(scenario);
    csv_open(p_csv_path,
             "point_index,frequency_hz,magnitude,magnitude_db,phase_deg");
    if ((p_csv_file != NULL) &&       /* The waveform output is available. */
        (include_dc_point == 1u))     /* A finite DC result exists for this transfer function. */
    {
        if (fprintf(p_csv_file, "0,0.000000,1.000000,0.000000,0.000000\n") < 0)
        {
            csv_write_ok = 0u;
        }
    }

    status = sfra_core_init(&fixture.sfra,
                            &fixture.sfra_inject_a,
                            &fixture.sfra_collect_a,
                            PI_TEST_CONTROL_FREQ_HZ,
                            PI_TEST_SFRA_INJECT_A,
                            PI_TEST_SFRA_START_HZ,
                            1.0f);
    if (status == SFRA_STATUS_OK)
    {
        status = sfra_core_set_sweep_range(&fixture.sfra,
                                           PI_TEST_SFRA_START_HZ,
                                           PI_TEST_SFRA_END_HZ);
    }
    fixture.sfra.cfg.freq_length = PI_TEST_SFRA_POINT_COUNT;
    fixture.sfra.cb.freq_prepare = sfra_frequency_prepare;
    fixture.sfra.cb.p_ctx = &fixture;
    if (status == SFRA_STATUS_OK)
    {
        status = sfra_core_start(&fixture.sfra);
    }
    fixture.sfra_init_ok = (status == SFRA_STATUS_BUSY) ? 1u : 0u;
}

static void sfra_closed_loop_init(void)
{
    sfra_prepare(PI_TEST_SFRA_CLOSED_LOOP_E,
                 "build/pi_sfra_closed_loop_bode.csv",
                 1u);
}

static void sfra_open_loop_init(void)
{
    sfra_prepare(PI_TEST_SFRA_OPEN_LOOP_E,
                 "build/pi_sfra_open_loop_bode.csv",
                 0u);
}

static void step_before_dut(double time_s)
{
    plant_update(); /* Apply the preceding beat's PI voltage before sampling the current beat. */
    fixture.pi_feedback_a = fixture.current_a;
    if (time_s >= (double)PI_TEST_STEP_TIME_S)
    {
        fixture.current_ref_a = PI_TEST_STEP_CURRENT_A;
    }
    else
    {
        fixture.current_ref_a = 0.0f;
    }
}

static void sfra_feedback_loop_before_dut(double time_s)
{
    (void)time_s;
    if (fixture.plant_input_valid == 1u)
    {
        sfra_closed_loop_plant_update();
        fixture.plant_input_valid = 0u;
    }
    (void)sfra_core_task(&fixture.sfra);
    sfra_core_isr_pre_sample(&fixture.sfra);
    fixture.pi_feedback_a = fixture.current_a;
    fixture.current_ref_a = fixture.sfra_inject_a;
}

static void sfra_open_loop_before_dut(double time_s)
{
    (void)time_s;
    if (fixture.plant_input_valid == 1u)
    {
        sfra_open_loop_plant_update();
        fixture.plant_input_valid = 0u;
    }
    (void)sfra_core_task(&fixture.sfra);
    sfra_core_isr_pre_sample(&fixture.sfra);
    fixture.pi_feedback_a = fixture.current_a;
    fixture.current_ref_a = 0.0f;
}

static void plant_update(void)
{
    fixture.current_a +=
        (fixture.voltage_v / PI_TEST_INDUCTANCE_H) * PI_TEST_CONTROL_PERIOD_S;
    if (fixture.current_a > fixture.max_current_a)
    {
        fixture.max_current_a = fixture.current_a;
    }
}

static void step_response_evaluate(double time_s)
{
    const float error_a = fabsf(fixture.current_ref_a - fixture.current_a);

    if ((time_s >= (double)PI_TEST_STEP_TIME_S) &&
        (error_a <= PI_TEST_STABLE_ERROR_A))
    {
        fixture.stable_sample_count++;
        if (fixture.stable_sample_count >= PI_TEST_STABLE_SAMPLE_COUNT)
        {
            fixture.stable_reached = 1u;
        }
    }
    else
    {
        fixture.stable_sample_count = 0u;
    }
}

static void sfra_closed_loop_plant_update(void)
{
    plant_update();
    fixture.sfra_collect_a = fixture.current_a;
    sfra_core_isr_post_sample(&fixture.sfra);
}

static void sfra_open_loop_plant_update(void)
{
    const float controller_voltage_v = fixture.voltage_v;

    fixture.voltage_v += fixture.sfra_inject_a;
    plant_update();
    fixture.sfra_collect_a = -controller_voltage_v;
    fixture.sfra_inject_a = fixture.voltage_v;
    sfra_core_isr_post_sample(&fixture.sfra);
}

static void step_record(double time_s)
{
    int write_result = 0;

    if (p_csv_file != NULL)
    {
        write_result = fprintf(p_csv_file,
                               "%.9f,%.6f,%.6f,%.6f,%.6f,%.9f,%.9f,%" PRIu32 "\n",
                               time_s,
                               (double)fixture.current_ref_a,
                               (double)fixture.current_a,
                               (double)(fixture.current_ref_a - fixture.current_a),
                               (double)fixture.voltage_v,
                               (double)pi_kp_get(),
                               (double)pi_ki_get(),
                               fixture.stable_sample_count);
        if (write_result < 0)
        {
            csv_write_ok = 0u;
        }
    }
}

static void sfra_record(double time_s)
{
    uint16_t csv_point_index = 0u;
    float magnitude_db = 0.0f;
    int write_result = 0;

    (void)time_s;
    if (fixture.sfra.output.point_done == 0u)
    {
        return;
    }

    magnitude_db = -400.0f;
    if (fixture.sfra.output.mag > 1.0e-20f)
    {
        magnitude_db = 20.0f * log10f(fixture.sfra.output.mag);
    }
    csv_point_index = fixture.sfra.output.point_index;
    if (fixture.scenario == PI_TEST_SFRA_CLOSED_LOOP_E)
    {
        csv_point_index++;
    }
    else if (fabsf(fixture.sfra.output.current_freq_hz - PI_TEST_BANDWIDTH_HZ) <
             fabsf(fixture.crossover_freq_hz - PI_TEST_BANDWIDTH_HZ))
    {
        fixture.crossover_freq_hz = fixture.sfra.output.current_freq_hz;
        fixture.crossover_magnitude = fixture.sfra.output.mag;
        fixture.crossover_phase_deg = fixture.sfra.output.phase;
    }
    if (p_csv_file != NULL)
    {
        write_result = fprintf(p_csv_file,
                               "%u,%.6f,%.9f,%.6f,%.6f\n",
                               (unsigned int)csv_point_index,
                               (double)fixture.sfra.output.current_freq_hz,
                               (double)fixture.sfra.output.mag,
                               (double)magnitude_db,
                               (double)fixture.sfra.output.phase);
        if (write_result < 0)
        {
            csv_write_ok = 0u;
        }
    }
    fixture.sfra_recorded_point_count++;
    fixture.sfra.output.point_done = 0u;
}

static uint8_t step_finished(double time_s)
{
    if (fixture.stable_reached == 1u)
    {
        return 1u;
    }
    return (time_s >= PI_TEST_STEP_TIMEOUT_S) ? 1u : 0u;
}

static uint8_t sfra_finished(double time_s)
{
    (void)time_s;
    return (fixture.sfra.output.done != 0u) ? 1u : 0u;
}

static uint8_t step_assert(void)
{
    uint8_t passed = 1u;
    const float final_error_a = fabsf(fixture.current_ref_a - fixture.current_a);

    (void)printf("    PI GAINS | phase_margin=%.1fdeg bandwidth=%.1fHz kp=%.9f ki=%.9f\n",
                 (double)PI_TEST_PHASE_MARGIN_DEG,
                 (double)PI_TEST_BANDWIDTH_HZ,
                 (double)pi_kp_get(),
                 (double)pi_ki_get());
    (void)printf("    STEP RESULT | final_current=%.6fA final_error=%.6fA peak=%.6fA\n",
                 (double)fixture.current_a,
                 (double)final_error_a,
                 (double)fixture.max_current_a);
    if (expect_true("PI initialization", fixture.dut_init_ok) == 0u)
    {
        passed = 0u;
    }
    if (expect_true("stable response reached", fixture.stable_reached) == 0u)
    {
        passed = 0u;
    }
    if (expect_true("final error within tolerance",
                    (final_error_a <= PI_TEST_STABLE_ERROR_A) ? 1u : 0u) == 0u)
    {
        passed = 0u;
    }
    if (csv_close() == 0u)
    {
        passed = 0u;
    }
    return passed;
}

static uint8_t sfra_assert_common(const char *p_loop_name)
{
    uint8_t passed = 1u;
    const float final_freq_error_hz =
        fabsf(fixture.sfra.output.current_freq_hz - PI_TEST_SFRA_END_HZ);

    (void)printf("    SFRA %s RESULT | range=10..%.1fHz logarithmic_points=%u recorded=%u\n",
                 p_loop_name,
                 (double)PI_TEST_SFRA_END_HZ,
                 (unsigned int)PI_TEST_SFRA_POINT_COUNT,
                 (unsigned int)fixture.sfra_recorded_point_count);
    if (expect_true("PI initialization", fixture.dut_init_ok) == 0u)
    {
        passed = 0u;
    }
    if (expect_true("SFRA initialization", fixture.sfra_init_ok) == 0u)
    {
        passed = 0u;
    }
    if (expect_true("all SFRA points recorded",
                    (fixture.sfra_recorded_point_count == PI_TEST_SFRA_POINT_COUNT) ? 1u : 0u) == 0u)
    {
        passed = 0u;
    }
    if (expect_true("20 kHz endpoint reached",
                    (final_freq_error_hz <= 1.0f) ? 1u : 0u) == 0u)
    {
        passed = 0u;
    }
    if (fixture.scenario == PI_TEST_SFRA_OPEN_LOOP_E)
    {
        (void)printf("    OPEN LOOP CROSSOVER | frequency=%.3fHz magnitude=%.6f phase=%.3fdeg\n",
                     (double)fixture.crossover_freq_hz,
                     (double)fixture.crossover_magnitude,
                     (double)fixture.crossover_phase_deg);
        if (expect_true("10 kHz crossover magnitude",
                        ((fixture.crossover_magnitude >= 0.9f) &&
                         (fixture.crossover_magnitude <= 1.1f))
                            ? 1u
                            : 0u) == 0u)
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

static uint8_t sfra_closed_loop_assert(void)
{
    return sfra_assert_common("CLOSED LOOP");
}

static uint8_t sfra_open_loop_assert(void)
{
    return sfra_assert_common("OPEN LOOP");
}

static TESTBENCH_CASE_STATE_E assertion_state_get(uint8_t assertion_passed)
{
    if (assertion_passed == 1u)
    {
        return TESTBENCH_CASE_PASS;
    }
    return TESTBENCH_CASE_FAIL;
}

static TESTBENCH_CASE_STATE_E step_after_dut(double time_s)
{
    step_response_evaluate(time_s);
    step_record(time_s);

    if ((fixture.dut_init_ok == 1u) &&     /* The production PI initialized successfully. */
        (step_finished(time_s) == 0u))     /* The response still requires additional DUT runs. */
    {
        return TESTBENCH_CASE_RUNNING;
    }
    return assertion_state_get(step_assert());
}

static TESTBENCH_CASE_STATE_E sfra_closed_loop_after_dut(double time_s)
{
    if ((fixture.dut_init_ok == 0u) ||     /* The production PI cannot execute valid samples. */
        (fixture.sfra_init_ok == 0u))      /* The requested SFRA sweep could not start. */
    {
        return assertion_state_get(sfra_closed_loop_assert());
    }

    sfra_record(time_s);
    if (sfra_finished(time_s) == 0u)
    {
        return TESTBENCH_CASE_RUNNING;
    }
    return assertion_state_get(sfra_closed_loop_assert());
}

static TESTBENCH_CASE_STATE_E sfra_open_loop_after_dut(double time_s)
{
    if ((fixture.dut_init_ok == 0u) ||     /* The production PI cannot execute valid samples. */
        (fixture.sfra_init_ok == 0u))      /* The requested SFRA sweep could not start. */
    {
        return assertion_state_get(sfra_open_loop_assert());
    }

    sfra_record(time_s);
    if (sfra_finished(time_s) == 0u)
    {
        return TESTBENCH_CASE_RUNNING;
    }
    return assertion_state_get(sfra_open_loop_assert());
}

TESTBENCH_REGISTER(pi, PI_TEST_CONTROL_PERIOD_S, dut_init, dut_run)

TESTBENCH_CASE(pi,
               current_reference_step,
               step_init,
               step_before_dut,
               step_after_dut)

TESTBENCH_CASE(pi,
               sfra_closed_loop_sweep,
               sfra_closed_loop_init,
               sfra_feedback_loop_before_dut,
               sfra_closed_loop_after_dut)

TESTBENCH_CASE(pi,
               sfra_open_loop_sweep,
               sfra_open_loop_init,
               sfra_open_loop_before_dut,
               sfra_open_loop_after_dut)
