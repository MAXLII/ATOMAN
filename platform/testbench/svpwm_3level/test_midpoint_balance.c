// SPDX-License-Identifier: MIT
/**
 * @file    test_midpoint_balance.c
 * @brief   Independent midpoint-current and capacitor-balance checks.
 * @details Base digital power framework. Host-only C11 test using the production
 *          SVPWM library and double-precision capacitor charge conservation.
 *          Checks voltage preservation, limited control authority and both power directions.
 * @author  Max.Li
 * @date    2026-09-13
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * This file is licensed under the MIT License. See LICENSE for details.
 */
#include "svpwm_3level.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) check((condition), #condition, __LINE__) /* Always evaluate physical invariants. */

static uint32_t checks = 0u; /* Completed assertions. */

/** @param passed Assertion result. @param p_text Failed expression. @param line Source line. */
static void check(bool passed, const char *p_text, int line)
{
    ++checks;
    if (passed == false)
    {
        (void)fprintf(stderr, "FAIL line %d: %s\n", line, p_text);
        exit(EXIT_FAILURE);
    }
}

/** @param p_mod Valid production modulation result.
 *  @return Independently reconstructed midpoint current flowing toward the AC side, A. */
static double verify_output(const svpwm_3level_t *p_mod)
{
    const svpwm_3level_phase_output_t phases[3] = { /* Copy the 3 completed phase results. */
        p_mod->output.phase_a, p_mod->output.phase_b, p_mod->output.phase_c
    };
    const double currents[3] = {p_mod->input.i_a, p_mod->input.i_b, p_mod->input.i_c}; /* Signed phase currents. */
    double poles[3] = {0.0}; /* Mean pole voltages reconstructed from dwell fractions. */
    double neutral = 0.0;    /* Sum of currents during midpoint connection. */

    for (uint32_t phase = 0u; phase < 3u; ++phase) /* Independent voltage and charge reconstruction. */
    {
        CHECK((phases[phase].duty_p >= 0.0f) && /* Positive dwell cannot be negative. */
              (phases[phase].duty_p <= 1.0f)); /* Positive dwell fits one period. */
        CHECK((phases[phase].duty_o >= 0.0f) && /* Midpoint dwell cannot be negative. */
              (phases[phase].duty_o <= 1.0f)); /* Midpoint dwell fits one period. */
        CHECK((phases[phase].duty_n >= 0.0f) && /* Negative-level dwell remains nonnegative. */
              (phases[phase].duty_n <= 1.0f)); /* Negative-level dwell fits one period. */
        CHECK((phases[phase].duty_p == 0.0f) || /* Each phase uses one adjacent pair. */
              (phases[phase].duty_n == 0.0f)); /* P and N cannot both have positive dwell. */
        CHECK(fabs((double)phases[phase].duty_p + phases[phase].duty_o + phases[phase].duty_n - 1.0) < 2.0e-7);
        poles[phase] = (double)phases[phase].duty_p * p_mod->input.v_dc_p -
                       (double)phases[phase].duty_n * p_mod->input.v_dc_n;
        neutral += (double)phases[phase].duty_o * currents[phase];
    }
    CHECK(fabs((2.0 * poles[0] - poles[1] - poles[2]) / 3.0 - p_mod->input.v_alpha) < 0.002);
    CHECK(fabs((poles[1] - poles[2]) / sqrt(3.0) - p_mod->input.v_beta) < 0.002);
    CHECK(fabs((poles[0] + poles[1] + poles[2]) / 3.0 - p_mod->output.common_mode_v) < 0.002);
    CHECK(fabs(neutral - p_mod->output.midpoint_current) < 0.002);
    return neutral;
}

/** @brief Compare selected current against a dense independent feasible-offset sweep. */
static void test_offset_authority(void)
{
    svpwm_3level_t mod = {0}; /* Production instance with balancing enabled. */
    const svpwm_3level_cfg_t cfg = {.v_dc_half_min = 10.0f, .midpoint_kp = 1.8849556f}; /* A/V gain. */
    const double pi = acos(-1.0); /* Host reference constant. */

    svpwm_3level_init(&mod, &cfg);
    for (uint32_t step = 0u; step < 720u; ++step) /* Sweep all voltage angles and both current directions. */
    {
        const double theta = 2.0 * pi * (double)step / 720.0; /* Voltage angle. */
        double phase_v[3] = {0.0}; /* Zero-sequence-free voltage references, V. */
        double phase_i[3] = {0.0}; /* Three-wire current snapshot, A. */
        double lower = -10000.0;  /* Physical feasible mixed-polarity lower offset bound. */
        double upper = 10000.0;   /* Physical feasible mixed-polarity upper offset bound. */
        double best_grid = 1.0e30; /* Best current error observed on the independent grid. */
        double actual = 0.0;       /* Reconstructed production midpoint current. */
        double target = 0.0;       /* Balance current target, A. */
        const double sign = (step < 360u) ? 1.0 : -1.0; /* Inverting and rectifying operation. */

        mod.input.v_dc_p = (step % 2u == 0u) ? 765.0f : 565.0f;
        mod.input.v_dc_n = 1330.0f - mod.input.v_dc_p;
        mod.input.v_alpha = (float)(560.0 * cos(theta));
        mod.input.v_beta = (float)(560.0 * sin(theta));
        phase_v[0] = mod.input.v_alpha;
        phase_v[1] = -0.5 * mod.input.v_alpha + sqrt(3.0) * 0.5 * mod.input.v_beta;
        phase_v[2] = -0.5 * mod.input.v_alpha - sqrt(3.0) * 0.5 * mod.input.v_beta;
        phase_i[0] = sign * 2000.0 * cos(theta - 0.7);
        phase_i[1] = sign * 2000.0 * cos(theta - 0.7 - 2.0 * pi / 3.0);
        phase_i[2] = -phase_i[0] - phase_i[1];
        mod.input.i_a = (float)phase_i[0];
        mod.input.i_b = (float)phase_i[1];
        mod.input.i_c = (float)phase_i[2];
        phase_i[0] = mod.input.i_a;
        phase_i[1] = mod.input.i_b;
        phase_i[2] = mod.input.i_c;
        svpwm_3level_cal(&mod);
        actual = verify_output(&mod);
        target = -(double)cfg.midpoint_kp * ((double)mod.input.v_dc_p - mod.input.v_dc_n);
        CHECK(fabs(target - mod.output.midpoint_current_ref) < 0.001);
        for (uint32_t phase = 0u; phase < 3u; ++phase) /* Bounds from real voltage levels, not mask formulas. */
        {
            lower = fmax(lower, -mod.input.v_dc_n - phase_v[phase]);
            upper = fmin(upper, mod.input.v_dc_p - phase_v[phase]);
        }
        lower = fmax(lower, -fmax(phase_v[0], fmax(phase_v[1], phase_v[2])));
        upper = fmin(upper, -fmin(phase_v[0], fmin(phase_v[1], phase_v[2])));
        for (uint32_t point = 0u; point <= 512u; ++point) /* Dense voltage-offset search independent of production solver. */
        {
            const double offset = lower + (upper - lower) * (double)point / 512.0; /* Candidate common mode, V. */
            double neutral = 0.0; /* Candidate average midpoint current. */
            for (uint32_t phase = 0u; phase < 3u; ++phase) /* Derive O occupancy directly from physical pole voltage. */
            {
                const double pole = phase_v[phase] + offset; /* Candidate pole voltage, V. */
                const double rail = (pole >= 0.0) ? mod.input.v_dc_p : mod.input.v_dc_n; /* Applicable voltage level. */
                neutral += (1.0 - fabs(pole) / rail) * phase_i[phase];
            }
            best_grid = fmin(best_grid, fabs(neutral - target));
        }
        CHECK(fabs(actual - target) <= best_grid + 0.05);
    }
}

/** @param initial_delta Initial upper-minus-lower bus voltage, V.
 *  @param amplitude Signed phase-current amplitude, A; negative denotes regeneration. */
static void test_capacitor_loop(double initial_delta, double amplitude)
{
    svpwm_3level_t mod = {0}; /* Real modulation instance. */
    const svpwm_3level_cfg_t cfg = {.v_dc_half_min = 20.0f, .midpoint_kp = 1.8849556f}; /* 60 mF, ideal 5 Hz. */
    const double pi = acos(-1.0); /* Host reference constant. */
    const double ts = 0.0002;     /* 5 kHz control period, s. */
    const double capacitance = 0.06; /* Each capacitor, F. */
    double upper_charge = capacitance * (1330.0 + initial_delta) / 2.0; /* Upper capacitor positive-plate charge, C. */
    double lower_charge = capacitance * (1330.0 - initial_delta) / 2.0; /* Lower capacitor positive-plate charge, C. */
    double final_peak = 0.0; /* Maximum imbalance over the final electrical period, V. */

    svpwm_3level_init(&mod, &cfg);
    for (uint32_t tick = 0u; tick < 15000u; ++tick) /* Three seconds of ideal capacitor charge dynamics. */
    {
        const double theta = 2.0 * pi * 50.0 * ts * (double)tick; /* Balanced voltage angle. */
        double midpoint = 0.0; /* Mean current leaving the midpoint, A. */

        mod.input.v_alpha = (float)(563.0 * cos(theta));
        mod.input.v_beta = (float)(563.0 * sin(theta));
        mod.input.v_dc_p = (float)(upper_charge / capacitance);
        mod.input.v_dc_n = (float)(lower_charge / capacitance);
        mod.input.i_a = (float)(amplitude * cos(theta));
        mod.input.i_b = (float)(amplitude * cos(theta - 2.0 * pi / 3.0));
        mod.input.i_c = -mod.input.i_a - mod.input.i_b;
        svpwm_3level_cal(&mod);
        midpoint = verify_output(&mod);
        /* Fixed total bus and equal capacitors: Q_lower - Q_upper loses i_mid * Ts. */
        upper_charge += 0.5 * midpoint * ts;
        lower_charge -= 0.5 * midpoint * ts;
        CHECK((upper_charge > 0.0) && /* Upper capacitor retains a positive voltage. */
              (lower_charge > 0.0)); /* Lower capacitor retains a positive voltage. */
        if (tick >= 14900u)
        {
            final_peak = fmax(final_peak, fabs((upper_charge - lower_charge) / capacitance));
        }
    }
    (void)printf("MIDPOINT delta_start=%.1f A_peak=%.1f final_peak_v=%.6f\n", initial_delta, amplitude, final_peak);
    CHECK(final_peak < 1.0);
}

/** @brief No current or zero reference cannot be treated as guaranteed balancing authority. */
static void test_no_authority(void)
{
    svpwm_3level_t mod = {0}; /* Instance with no phase current. */
    const svpwm_3level_cfg_t cfg = {.v_dc_half_min = 20.0f, .midpoint_kp = 2.0f}; /* Nonzero balance demand. */

    svpwm_3level_init(&mod, &cfg);
    mod.input = (svpwm_3level_input_t){.v_alpha = 300.0f, .v_beta = 100.0f, .v_dc_p = 700.0f, .v_dc_n = 630.0f};
    svpwm_3level_cal(&mod);
    CHECK(verify_output(&mod) == 0.0);
    CHECK(mod.output.midpoint_current_ref == -140.0f);
    mod.input.v_alpha = 0.0f;
    mod.input.v_beta = 0.0f;
    svpwm_3level_cal(&mod);
    CHECK(mod.output.phase_a.duty_o == 1.0f);
    CHECK(mod.output.phase_b.duty_o == 1.0f);
    CHECK(mod.output.phase_c.duty_o == 1.0f);
    svpwm_3level_reset(&mod);
    CHECK(mod.output.midpoint_current == 0.0f);
    CHECK(mod.output.midpoint_current_ref == 0.0f);
    CHECK(mod.output.common_mode_v == 0.0f);
}

/** @return EXIT_SUCCESS after all production-library checks pass. */
int main(void)
{
    test_offset_authority(); /* Check feasible control authority and preserved voltage. */
    test_capacitor_loop(100.0, 100.0); /* Positive imbalance, inverter power flow. */
    test_capacitor_loop(-100.0, 100.0); /* Negative imbalance, inverter power flow. */
    test_capacitor_loop(100.0, -100.0); /* Positive imbalance, regenerative power flow. */
    test_capacitor_loop(-100.0, -100.0); /* Negative imbalance, regenerative power flow. */
    test_capacitor_loop(100.0, 3000.0); /* High-current operating point. */
    test_no_authority(); /* Verify zero-current and zero-reference behavior. */
    (void)printf("PASS: midpoint balance, %lu checks\n", (unsigned long)checks);
    return EXIT_SUCCESS;
}
