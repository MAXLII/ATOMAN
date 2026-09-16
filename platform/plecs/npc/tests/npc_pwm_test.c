// SPDX-License-Identifier: MIT
/**
 * @file    npc_pwm_test.c
 * @brief   5 kHz NPC duty-output DLL integration tests.
 * @details
 *          This file is part of the base digital power framework project.
 *          Verify 6 state-based duties, bridge enable, sample-and-hold timing and fault recovery.
 *          C11 compatible; no dynamic allocation; single simulation instance.
 * @author  Max.Li
 * @date    2026-09-12
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "DllHeader.h"
#include <math.h>
#include <float.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plecs_port.h"
#include "bsp_pwm.h"
#include "shell.h"
#include "section.h"
#include "npc_hal.h"
#include "npc_cfg.h"
#include "my_math.h"
#include "npc_protect.h"

/* PLECS exposes double-valued ports and timestamps; retain ABI precision in this host test. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wunsuffixed-float-constants"

static uint32_t task_calls; /* Calls to the registered 1 ms scheduler regression task. */

/** @brief Observe execution through the same Section dispatch used by waveform reporting. */
static void scheduler_probe(void)
{
    ++task_calls;
}
REG_TASK_MS(1, scheduler_probe)

#define CHECK(value) check((value), __LINE__) /* Assertions remain active in release builds. */
static double inputs[PLECS_INPUT_MAX]; /* Half-bus voltage input fixture. */
static double outputs[PLECS_OUTPUT_MAX]; /* Actual DLL duty and enable outputs. */
static double ref_alpha; /* Independently expected generated alpha voltage. */
static double ref_beta;  /* Independently expected generated beta voltage. */
static uint32_t checks; /* Assertion count. */
static uint32_t calls; /* Actual DLL output callback count. */
static double max_error; /* Largest reconstructed alpha/beta error in V. */

/** @param value Assertion result. @param line Test source line. */
static void check(bool value, int line)
{
    ++checks;
    if (value == false)
    {
        (void)fprintf(stderr, "FAIL line %d call %lu\n", line, (unsigned long)calls);
        exit(EXIT_FAILURE);
    }
}

/** @param p_format Shell output format; text is irrelevant to numeric readback assertions. */
static void discard_output(const char *p_format, ...)
{
    (void)p_format;
}

/** @param p_name Registered variable name. @param value Desired value sent through the real Shell parser. */
static void shell_set(const char *p_name, double value)
{
    char command[80]; /* Bounded Shell assignment string. */
    shell_ctx_t context = {0}; /* Independent text-parser input context. */
    section_link_tx_func_t io = {.my_printf = discard_output}; /* Real Shell text-output facade. */
    section_shell_t *p_item; /* Descriptor discovered through Section registration. */
    const int length = snprintf(command, sizeof(command), "%s:%.9f\r\n", p_name, value); /* Decimal-only Shell grammar. */
    CHECK(length > 0);
    CHECK((size_t)length < sizeof(command));
    p_item = shell_find(p_name, (uint8_t)strlen(p_name));
    CHECK(p_item != NULL);
    for (int i = 0; i < length; ++i) /* Feed the same bytes accepted by the text Shell service. */
    {
        shell_run((uint8_t)command[i], &io, &context);
    }
    if (p_item->type == (uint32_t)SHELL_FP32)
    {
        CHECK(fabs((double)*(float *)p_item->p_var - value) < 0.0001);
    }
    else
    {
        CHECK(p_item->type == (uint32_t)SHELL_UINT8);
        CHECK((double)*(uint8_t *)p_item->p_var == value);
    }
}

/** @param p_name Registered monitor name. @param expected Expected sampled value. */
static void check_monitor(const char *p_name, double expected)
{
    section_shell_t *p_item; /* Descriptor inspected under the same lock as protocol dispatch. */
    p_item = shell_find(p_name, (uint8_t)strlen(p_name));
    CHECK(p_item != NULL);
    CHECK(p_item->type == (uint32_t)SHELL_FP32);
    if (expected != expected) /* NaN comparison avoids MinGW's narrowing double classification macro. */
    {
        CHECK(isnan(*(float *)p_item->p_var) != 0);
    }
    else
    {
        CHECK(fabs((double)*(float *)p_item->p_var - expected) < 0.0001);
    }
}

/** @param time_s Simulation timestamp. @return true if the DLL reports no simulation error. */
static bool output_at(double time_s)
{
    struct SimulationState state = {inputs, outputs, NULL, NULL, time_s, NULL, NULL}; /* ABI fixture. */
    plecsOutput(&state);
    ++calls;
    return state.errorMessage == NULL;
}

/** @param time_s Simulation start time. */
static void start_at(double time_s)
{
    struct SimulationState state = {inputs, outputs, NULL, NULL, time_s, NULL, NULL}; /* Startup fixture. */
    plecsStart(&state);
    CHECK(state.errorMessage == NULL);
}

/** @brief Invalid/disabled frames must clear every duty and enable. */
static void check_off(void)
{
    for (uint32_t i = 0u; i < (uint32_t)PLECS_OUTPUT_MAX; ++i) /* Output index. */
    {
        CHECK(outputs[i] == 0.0);
    }
}

/** @brief Check duty mapping and reconstruct reference voltage independently. */
static void check_duty(void)
{
    double pole[3] = {0}; /* Average phase-to-midpoint voltages derived from P and N. */
    double error = 0.0; /* Maximum alpha/beta component error in V. */
    CHECK(outputs[PLECS_OUTPUT_PWM_ENABLE] == 1.0);
    for (uint32_t phase = 0u; phase < 3u; ++phase) /* Phase index. */
    {
        const double *p_duty = &outputs[2u * phase]; /* Positive and negative fractions. */
        for (uint32_t q = 0u; q < 2u; ++q) /* State-based duty index. */
        {
            CHECK(p_duty[q] >= 0.0);
            CHECK(p_duty[q] <= 1.0);
        }
        CHECK(p_duty[0] + p_duty[1] <= 1.0 + 1.0e-7);
        pole[phase] = p_duty[0] * inputs[PLECS_INPUT_V_DC_P] - p_duty[1] * inputs[PLECS_INPUT_V_DC_N];
    }
    error = fmax(fabs((2.0 * pole[0] - pole[1] - pole[2]) / 3.0 - ref_alpha),
                 fabs((pole[1] - pole[2]) / sqrt(3.0) - ref_beta));
    CHECK(error < 0.0002);
    max_error = fmax(max_error, error);
}

/** @brief Check transform normalization, rotation signs and inverse consistency independently of NPC. */
static void check_coordinate_transforms(void)
{
    float alpha = 0.0f; /* Stationary alpha result. */
    float beta = 0.0f; /* Stationary beta result. */
    float d = 0.0f; /* Rotating direct-axis result. */
    float q = 0.0f; /* Rotating quadrature-axis result. */
    float abc[3] = {0}; /* Reconstructed phase values. */
    clarke(1.0f, 0.0f, 0.0f, &alpha, &beta);
    CHECK(fabsf(alpha - 2.0f / 3.0f) < 1.0e-6f);
    CHECK(beta == 0.0f);
    inv_clarke(alpha, beta, &abc[0], &abc[1], &abc[2]);
    CHECK(fabsf(abc[0] - 2.0f / 3.0f) < 1.0e-6f);
    CHECK(fabsf(abc[1] + 1.0f / 3.0f) < 1.0e-6f);
    CHECK(fabsf(abc[2] + 1.0f / 3.0f) < 1.0e-6f);
    clarke(5.0f, 5.0f, 5.0f, &alpha, &beta);
    CHECK(alpha == 0.0f);
    CHECK(beta == 0.0f);
    inv_clarke(0.0f, 1.0f, &abc[0], &abc[1], &abc[2]);
    CHECK(abc[0] == 0.0f);
    CHECK(fabsf(abc[1] - 0.8660254f) < 1.0e-6f);
    CHECK(fabsf(abc[2] + 0.8660254f) < 1.0e-6f);
    park(3.0f, 4.0f, 1.0f, 0.0f, &d, &q);
    CHECK(d == 4.0f);
    CHECK(q == -3.0f);
    park(3.0f, 4.0f, -1.0f, 0.0f, &d, &q);
    CHECK(d == -4.0f);
    CHECK(q == 3.0f);
    inv_park(4.0f, -3.0f, 1.0f, 0.0f, &alpha, &beta);
    CHECK(alpha == 3.0f);
    CHECK(beta == 4.0f);
    for (int32_t index = -32; index <= 32; ++index) /* Both rotation directions and every quadrant. */
    {
        float angle = (float)index * M_PI / 16.0f; /* Test electrical angle, rad. */
        float sine = sinf(angle); /* Unit rotation sine. */
        float cosine = cosf(angle); /* Unit rotation cosine. */
        park(3.0f, 4.0f, sine, cosine, &d, &q);
        CHECK(fabsf(d * d + q * q - 25.0f) < 1.0e-5f);
        inv_park(d, q, sine, cosine, &alpha, &beta);
        CHECK(fabsf(alpha - 3.0f) < 2.0e-6f);
        CHECK(fabsf(beta - 4.0f) < 2.0e-6f);
        inv_clarke(cosine, sine, &abc[0], &abc[1], &abc[2]);
        clarke(abc[0], abc[1], abc[2], &alpha, &beta);
        CHECK(fabsf(alpha - cosine) < 1.0e-6f);
        CHECK(fabsf(beta - sine) < 1.0e-6f);
    }
}

/** @return EXIT_SUCCESS if the actual DLL meets the duty and 5 kHz timing contract. */
int main(void)
{
    struct SimulationSizes sizes = {0}; /* Actual DLL port dimensions. */
    double time_s = 0.0; /* Current 200 us control grid time. */
    double saved[PLECS_OUTPUT_MAX] = {0}; /* Held output frame between controller ticks. */
    bool saw_fractional_duty = false; /* Confirm state duties rather than binary gates. */
    npc_ctrl_cfg_t cfg = npc_cfg_default(); /* Coefficients for an independent zero-feedback PI recurrence. */
    float expected_ramp = 0.0f; /* Expected amplitude held across five 200 us controller ticks. */
    float expected_integral_v = 0.0f; /* Independent outer-loop integral in the unsaturated fixture. */
    float expected_integral_i = 0.0f; /* Independent inner-loop integral in the unsaturated fixture. */
    uint32_t ramp_checks = 0u; /* Samples whose voltage command proves the ramp cadence. */
    check_coordinate_transforms();
    plecsSetSizes(&sizes);
    CHECK(sizes.numInputs == 8);
    CHECK(sizes.numOutputs == 7);
    CHECK(sizes.numParameters == 0);
    CHECK(sizes.numStates == 0);
    start_at(time_s);
    shell_set("TRACE_ENABLE", 0.0); /* Keep this deterministic fixture free of automatic CSV captures. */
    shell_set("NP_BAL_KP", 0.0); /* Isolate duty reconstruction from midpoint balancing. */
    shell_set("VD_POS_REF", 100.0); /* Exercise the closed-loop soft-start path. */
    shell_set("RUN_ENABLE", 1.0); /* Request run through the real application/FSM boundary. */
    inputs[0] = 350.0;
    inputs[1] = 350.0;
    task_calls = 0u;
    for (uint32_t tick = 0u; tick < 2000u; ++tick) /* Exercise the actual closed-loop dispatcher for 0.4 s. */
    {
        time_s = (double)tick * PLECS_NPC_CONTROL_PERIOD_S;
        if (tick == 100u)
        {
            shell_set("VD_POS_REF", 0.0); /* Exercise downward slew using the same 1 ms task. */
        }
        if (tick == 1000u)
        {
            inputs[0] = 250.0;
            inputs[1] = 450.0; /* Unequal half buses must retain the same voltage command mapping. */
            shell_set("FREQ_HZ", 60.0); /* Rebind observers at the lifecycle update boundary. */
        }
        CHECK(output_at(time_s) == true);
        CHECK(task_calls == (tick + 1u) / 5u);
        if ((tick < 200u) && (outputs[PLECS_OUTPUT_PWM_ENABLE] == 1.0))
        {
            section_shell_t *p_alpha = shell_find("V_ALPHA_PWM", 11u); /* Actual modulation command. */
            section_shell_t *p_beta = shell_find("V_BETA_PWM", 10u); /* Actual modulation command. */
            float expected_current = cfg.kp_v * expected_ramp + expected_integral_v; /* Zero-feedback outer PI. */
            float expected_voltage = cfg.kp_i * expected_current + expected_integral_i; /* Zero-feedback inner PI. */
            float target = (tick < 100u) ? 100.0f : 0.0f; /* Independent rise/fall fixture target. */
            float step = NPC_CFG_DEFAULT_VD_POS_SLEW_VPS / 1000.0f; /* Requested V/s over a 1 ms interval. */
            CHECK(p_alpha != NULL);
            CHECK(p_beta != NULL);
            CHECK(fabsf(hypotf(*(float *)p_alpha->p_var, *(float *)p_beta->p_var) - expected_voltage) < 0.001f);
            CHECK(expected_current < cfg.current_peak); /* PI recurrence assumes no current saturation. */
            expected_integral_v += cfg.ts * cfg.ki_v * expected_ramp;
            expected_integral_i += cfg.ts * cfg.ki_i * expected_current;
            if (((tick + 1u) % 5u) == 0u) /* The task runs after this controller sample. */
            {
                expected_ramp = (expected_ramp < target) ? fminf(expected_ramp + step, target)
                                                       : fmaxf(expected_ramp - step, target);
            }
            ++ramp_checks;
        }
        if (tick >= 50u)
        {
            section_shell_t *p_alpha = shell_find("V_ALPHA_PWM", 11u); /* Actual HAL alpha command. */
            section_shell_t *p_beta = shell_find("V_BETA_PWM", 10u); /* Actual HAL beta command. */
            CHECK(p_alpha != NULL);
            CHECK(p_beta != NULL);
            ref_alpha = (double)*(float *)p_alpha->p_var;
            ref_beta = (double)*(float *)p_beta->p_var;
            check_duty();
        }
        for (uint32_t channel = 0u; channel < PLECS_OUTPUT_MAX; ++channel)
        {
            saved[channel] = outputs[channel];
            if ((channel < BSP_PWM_CHANNEL_COUNT) && (outputs[channel] > 0.0) && (outputs[channel] < 1.0))
            {
                saw_fractional_duty = true;
            }
        }
        CHECK(output_at(time_s) == true);
        CHECK(output_at(time_s + 0.0001) == true);
        CHECK(task_calls == (tick + 1u) / 5u);
        for (uint32_t channel = 0u; channel < PLECS_OUTPUT_MAX; ++channel)
        {
            CHECK(outputs[channel] == saved[channel]); /* Repeated and faster callbacks hold all ports. */
        }
    }
    CHECK(saw_fractional_duty == true);
    CHECK(ramp_checks > 150u); /* Both ramp directions were checked across multiple task periods. */
    CHECK(shell_find("VD_POS", 6u) == NULL);
    CHECK(shell_find("CTRL_STATUS", 11u) == NULL);
    CHECK(shell_find("VD_POS_REF_ACT", 14u) == NULL);
    inputs[PLECS_INPUT_I_L_B] = 7000.0; /* Exceeds the configured sampled-current trip. */
    time_s += PLECS_NPC_CONTROL_PERIOD_S;
    CHECK(output_at(time_s) == true);
    check_off();
    CHECK(npc_hal_get_fault() == NPC_PROTECT_OVERCURRENT);
    CHECK(npc_hal_get_fault_phase() == 1u);
    inputs[PLECS_INPUT_I_L_B] = 0.0;
    time_s += PLECS_NPC_CONTROL_PERIOD_S;
    CHECK(output_at(time_s) == true);
    check_off(); /* Removing current alone must not release the protection latch. */
    shell_set("RUN_ENABLE", 0.0);
    time_s += PLECS_NPC_CONTROL_PERIOD_S;
    CHECK(output_at(time_s) == true);
    check_off();
    CHECK(npc_hal_hard_protect_is_latched() == 0u);
    shell_set("RUN_ENABLE", 1.0);
    for (uint32_t tick = 0u; tick < 50u; ++tick) /* Allow the real 1 ms FSM to complete restart. */
    {
        time_s += PLECS_NPC_CONTROL_PERIOD_S;
        CHECK(output_at(time_s) == true);
    }
    CHECK(outputs[PLECS_OUTPUT_PWM_ENABLE] == 1.0);
    inputs[0] = 10.0; /* Same-period undervoltage must prevent the priority-3 PWM update. */
    time_s += PLECS_NPC_CONTROL_PERIOD_S;
    CHECK(output_at(time_s) == true);
    check_off();
    inputs[0] = 350.0;
    time_s += PLECS_NPC_CONTROL_PERIOD_S;
    CHECK(output_at(time_s) == true);
    CHECK(outputs[PLECS_OUTPUT_PWM_ENABLE] == 1.0);
    check_monitor("V_DC_P", 350.0);
    shell_set("RUN_ENABLE", 0.0);
    CHECK(output_at(time_s + 0.0001) == true);
    CHECK(outputs[PLECS_OUTPUT_PWM_ENABLE] == 1.0);
    time_s += PLECS_NPC_CONTROL_PERIOD_S;
    CHECK(output_at(time_s) == true);
    check_off();
    CHECK(output_at(time_s + 0.001) == false); /* Missing a controller tick remains an ABI timing error. */
    start_at(3.0);
    CHECK(output_at(3.0) == true);
    {
        bsp_pwm_phase_duty_t duty[BSP_PWM_PHASE_COUNT] = { /* Distinct phase pairs exercise publication order. */
            {.positive_duty = 0.1f, .negative_duty = 0.4f},
            {.positive_duty = 0.2f, .negative_duty = 0.5f},
            {.positive_duty = 0.3f, .negative_duty = 0.6f}
        };
        CHECK(bsp_pwm_set_duty(duty) == true);
        CHECK(output_at(3.0) == true); /* Publish BSP cache without running another control tick. */
        CHECK(outputs[PLECS_OUTPUT_PWM_ENABLE] == 1.0);
        for (uint32_t phase = 0u; phase < BSP_PWM_PHASE_COUNT; ++phase) /* Check every named field reaches its port. */
        {
            CHECK(outputs[2u * phase] == (double)duty[phase].positive_duty);
            CHECK(outputs[2u * phase + 1u] == (double)duty[phase].negative_duty);
        }
        for (uint32_t fault = 0u; fault < 6u; ++fault) /* Reject P+N overflow, non-finite values and bounds. */
        {
            duty[2].positive_duty = 0.3f;
            duty[2].negative_duty = 0.6f;
            CHECK(bsp_pwm_set_duty(duty) == true);
            if (fault == 0u)
            {
                duty[2].negative_duty = 0.8f;
            }
            else if (fault == 1u)
            {
                duty[2].positive_duty = nanf("");
            }
            else if (fault == 2u)
            {
                duty[2].positive_duty = -0.1f;
            }
            else if (fault == 3u)
            {
                duty[2].negative_duty = 1.1f;
            }
            else if (fault == 4u)
            {
                duty[2].negative_duty = -0.1f;
            }
            else
            {
                duty[2].negative_duty = nanf("");
            }
            CHECK(bsp_pwm_set_duty(duty) == false);
            CHECK(output_at(3.0) == true);
            check_off();
        }
        CHECK(bsp_pwm_set_duty(NULL) == false);
        CHECK(output_at(3.0) == true);
        check_off();
    }
    {
        struct SimulationState state = {inputs, outputs, NULL, NULL, 3.0, NULL, NULL}; /* Termination fixture. */
        plecsTerminate(&state);
    }
    check_off();
    (void)printf("PASS NPC 5 kHz duty DLL: %lu calls, %lu checks, max_error_v=%.9g\n",
                 (unsigned long)calls, (unsigned long)checks, max_error);
    return EXIT_SUCCESS;
}

#pragma GCC diagnostic pop
