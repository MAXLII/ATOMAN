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
#include "frame_tcp_server.h"
#include "section.h"

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
    frame_tcp_server_dispatch_enter();
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
    frame_tcp_server_dispatch_exit();
}

/** @param p_name Registered monitor name. @param expected Expected sampled value. */
static void check_monitor(const char *p_name, double expected)
{
    section_shell_t *p_item; /* Descriptor inspected under the same lock as protocol dispatch. */
    frame_tcp_server_dispatch_enter();
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
    frame_tcp_server_dispatch_exit();
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

/** @return EXIT_SUCCESS if the actual DLL meets the duty and 5 kHz timing contract. */
int main(void)
{
    struct SimulationSizes sizes = {0}; /* Actual DLL port dimensions. */
    double time_s = 0.0; /* Current 200 us control grid time. */
    double expected_cycles = 0.0; /* Unwrapped test-side phase, independent of DLL phase wrapping. */
    double saved[PLECS_OUTPUT_MAX]; /* Held frame before faster or repeated callbacks. */
    bool saw_fractional_duty = false; /* Proves outputs contain duties rather than digital gates. */
    plecsSetSizes(&sizes);
    CHECK(sizes.numInputs == 8);
    CHECK(sizes.numOutputs == 7);
    CHECK(sizes.numParameters == 0);
    CHECK(sizes.numStates == 0);
    start_at(0.0);
    task_calls = 0u;
    for (uint32_t tick = 0u; tick < 20u; ++tick) /* 4 ms of real NPC dispatch, with repeated and intermediate calls. */
    {
        const double instant = (double)tick * PLECS_NPC_CONTROL_PERIOD_S; /* Current control grid time. */
        CHECK(output_at(instant) == true);
        CHECK(task_calls == (tick + 1u) / 5u);
        CHECK(output_at(instant) == true);
        CHECK(output_at(instant + 0.0001) == true);
        CHECK(task_calls == (tick + 1u) / 5u); /* Held callbacks must not execute extra periodic tasks. */
    }
    {
        const char *const p_names[6] = {"V_OUT_A", "V_OUT_B", "V_OUT_C", "I_L_A", "I_L_B", "I_L_C"}; /* Feedback monitor order. */
        double feedback_time = 0.0; /* Independent feedback fixture clock. */
        start_at(feedback_time);
        inputs[0] = 350.0;
        inputs[1] = 350.0;
        for (uint32_t channel = 0u; channel < 6u; ++channel) /* Distinct signed values detect channel swaps. */
        {
            check_monitor(p_names[channel], 0.0);
            inputs[channel + 2u] = (double)channel * 20.0 - 50.0;
        }
        CHECK(output_at(feedback_time) == true);
        check_off(); /* Monitoring remains active while PWM is disabled. */
        for (uint32_t channel = 0u; channel < 6u; ++channel)
        {
            check_monitor(p_names[channel], inputs[channel + 2u]);
            shell_set(p_names[channel], 999.0); /* Simulation monitors accept writes until the next sample. */
        }
        inputs[PLECS_INPUT_V_OUT_A] = 123.0;
        CHECK(output_at(0.0001) == true);
        check_monitor("V_OUT_A", 999.0); /* Intermediate callbacks preserve the Shell write. */
        feedback_time += PLECS_NPC_CONTROL_PERIOD_S;
        CHECK(output_at(feedback_time) == true);
        check_monitor("V_OUT_A", 123.0);
        shell_set("RUN_ENABLE", 1.0);
        for (uint32_t channel = 0u; channel < 6u; ++channel)
        {
            const double invalid[3] = {NAN, INFINITY, DBL_MAX}; /* Non-finite and unrepresentable feedback. */
            for (uint32_t fault = 0u; fault < 3u; ++fault)
            {
                inputs[channel + 2u] = invalid[fault];
                feedback_time += PLECS_NPC_CONTROL_PERIOD_S;
                CHECK(output_at(feedback_time) == true);
                check_monitor(p_names[channel], NAN);
                check_off();
                inputs[channel + 2u] = 0.0;
                feedback_time += PLECS_NPC_CONTROL_PERIOD_S;
                CHECK(output_at(feedback_time) == true);
                check_duty(); /* Valid feedback restores open-loop modulation without altering its reference. */
            }
        }
    }
    start_at(time_s);
    check_off();
    CHECK(shell_count_get() >= 13u);
    shell_set("V_DC_HALF_MIN", 20.0);
    shell_set("RUN_ENABLE", 1.0);
    for (uint32_t tick = 0u; tick < 2000u; ++tick) /* 0.4 s at 5 kHz, including a half-bus split step. */
    {
        const double frequency = (tick < 750u) ? 50.0 : 60.0; /* Change frequency at a nonzero phase. */
        const double beta_amp = (tick < 1000u) ? 300.0 : 200.0; /* Independent axis amplitude step. */
        const double theta = 2.0 * acos(-1.0) * expected_cycles; /* Analytical electrical angle. */
        shell_set("V_ALPHA_AMP", 300.0);
        shell_set("V_BETA_AMP", beta_amp);
        shell_set("FREQ_HZ", frequency);
        ref_alpha = 300.0 * cos(theta);
        ref_beta = beta_amp * sin(theta);
        inputs[0] = (tick < 1000u) ? 350.0 : 250.0;
        inputs[1] = 700.0 - inputs[0];
        CHECK(output_at(time_s) == true);
        check_duty();
        check_monitor("V_ALPHA", ref_alpha);
        check_monitor("V_BETA", ref_beta);
        shell_set("V_ALPHA", 123.0); /* Monitor writes do not recalculate an already latched PWM frame. */
        shell_set("V_BETA", 456.0);
        for (uint32_t i = 0u; i < BSP_PWM_CHANNEL_COUNT; ++i) /* Detect non-binary duty output. */
        {
            if ((outputs[i] > 0.0) && /* Duty is above fully off. */
                (outputs[i] < 1.0))   /* Duty is below fully on. */
            {
                saw_fractional_duty = true;
            }
        }
        for (uint32_t i = 0u; i < (uint32_t)PLECS_OUTPUT_MAX; ++i) /* Snapshot entire output frame. */
        {
            saved[i] = outputs[i];
        }
        shell_set("V_ALPHA_AMP", 2000.0); /* Intermediate callbacks must hold the generated reference. */
        shell_set("V_BETA_AMP", 2000.0);
        CHECK(output_at(time_s) == true);
        CHECK(output_at(time_s + 0.0001) == true);
        check_monitor("V_ALPHA", 123.0);
        check_monitor("V_BETA", 456.0);
        for (uint32_t i = 0u; i < (uint32_t)PLECS_OUTPUT_MAX; ++i) /* Verify exact zero-order hold. */
        {
            CHECK(outputs[i] == saved[i]);
        }
        time_s += PLECS_NPC_CONTROL_PERIOD_S;
        expected_cycles += frequency * PLECS_NPC_CONTROL_PERIOD_S;
    }
    CHECK(saw_fractional_duty == true);
    CHECK(output_at(time_s) == true); /* Commit the infeasible command at the next 200 us boundary. */
    check_off();
    time_s += PLECS_NPC_CONTROL_PERIOD_S;
    ref_alpha = 0.0;
    ref_beta = 0.0;
    shell_set("V_ALPHA_AMP", 0.0);
    shell_set("V_BETA_AMP", 0.0);
    CHECK(output_at(time_s) == true);
    check_duty();
    for (uint32_t phase = 0u; phase < 3u; ++phase) /* Active OOO is distinct from disable. */
    {
        CHECK(outputs[2u * phase] == 0.0);
        CHECK(outputs[2u * phase + 1u] == 0.0);
    }
    shell_set("RUN_ENABLE", 0.0);
    CHECK(output_at(time_s + 0.0001) == true);
    CHECK(outputs[PLECS_OUTPUT_PWM_ENABLE] == 1.0); /* Enable is sampled on the same 5 kHz grid as the duties. */
    time_s += PLECS_NPC_CONTROL_PERIOD_S;
    CHECK(output_at(time_s) == true);
    check_off();
    shell_set("RUN_ENABLE", 1.0);
    inputs[0] = 10.0;
    time_s += PLECS_NPC_CONTROL_PERIOD_S;
    CHECK(output_at(time_s) == true);
    check_off();
    inputs[0] = 350.0;
    inputs[1] = nan("");
    time_s += PLECS_NPC_CONTROL_PERIOD_S;
    CHECK(output_at(time_s) == true);
    check_off();
    inputs[1] = 350.0;
    time_s += PLECS_NPC_CONTROL_PERIOD_S;
    CHECK(output_at(time_s) == true);
    check_duty();
    shell_set("V_DC_HALF_MIN", 400.0);
    time_s += PLECS_NPC_CONTROL_PERIOD_S;
    CHECK(output_at(time_s) == true);
    check_off();
    shell_set("V_DC_HALF_MIN", 20.0);
    time_s += PLECS_NPC_CONTROL_PERIOD_S;
    CHECK(output_at(time_s) == true);
    check_duty();
    CHECK(output_at(time_s + 0.0004) == false); /* A missed control tick must not silently lower the rate. */
    check_off();

    start_at(1.0);
    shell_set("V_ALPHA_AMP", 100.0);
    shell_set("V_BETA_AMP", 200.0);
    CHECK(output_at(1.0) == true);
    check_off(); /* Disabled output still updates the generated monitors. */
    check_monitor("V_ALPHA", 100.0);
    check_monitor("V_BETA", 0.0);
    CHECK(output_at(1.0002) == true);
    check_off();
    check_monitor("V_ALPHA", 100.0 * cos(2.0 * acos(-1.0) * 0.01));
    check_monitor("V_BETA", 200.0 * sin(2.0 * acos(-1.0) * 0.01));
    CHECK(output_at(0.99) == false); /* Unsupported solver rollback fails off. */
    check_off();
    start_at(2.0);
    CHECK(output_at(2.0) == true);
    CHECK(output_at(nan("")) == false);
    check_off();
    for (uint32_t scenario = 0u; scenario < 2u; ++scenario) /* Long run and large nonzero time origin. */
    {
        const double origin = (scenario == 0u) ? 0.0 : 1000000.0; /* Exercise absolute-time rounding. */
        const uint32_t tick_count = (scenario == 0u) ? 300000u : 2000u; /* 60 s and 0.4 s at 5 kHz. */
        start_at(origin);
        shell_set("V_ALPHA_AMP", 100.0);
        shell_set("V_BETA_AMP", 100.0);
        shell_set("RUN_ENABLE", 1.0);
        for (uint32_t tick = 0u; tick <= tick_count; ++tick) /* Host clock computed from integer sample number. */
        {
            const double instant = origin + (double)tick * PLECS_NPC_CONTROL_PERIOD_S; /* Scheduled callback time. */
            CHECK(output_at(instant) == true);
            if ((tick % 1000u) == 0u) /* Verify phase and held outputs during the long run. */
            {
                ref_alpha = 100.0;
                ref_beta = 0.0;
                check_duty();
                CHECK(output_at(instant) == true);
                CHECK(output_at(instant + 0.0001) == true);
                check_monitor("V_ALPHA", 100.0);
                check_monitor("V_BETA", 0.0);
            }
        }
        CHECK(output_at(origin + (double)(tick_count + 2u) * PLECS_NPC_CONTROL_PERIOD_S) == false);
        check_off(); /* A genuinely skipped tick must still fail off after a long run. */
    }
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
