// SPDX-License-Identifier: MIT
/**
 * @file    npc_edge_compensation_candidate.h
 * @brief   Diagnostic NPC switching-edge current prediction.
 * @details Reconstruct ideal ripple for a floating-neutral three-phase inductor.
 *          This experiment is compiled only into a separate validation DLL.
 *          It does not generate gates or alter the downstream dead time.
 * @author  Max.Li
 * @date    2026-09-14
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef NPC_EDGE_COMPENSATION_CANDIDATE_H
#define NPC_EDGE_COMPENSATION_CANDIDATE_H

#define NPC_EDGE_INDUCTANCE (0.00008f)
#define NPC_EDGE_SIGN_WIDTH (5.0f)

static inline float edge_positive_fraction(float value)
{
    return 0.5f + 0.5f * fmaxf(-1.0f, fminf(1.0f, value / NPC_EDGE_SIGN_WIDTH));
}

static inline void candidate_edge_compensation(const svpwm_3level_t *p_mod,
                                              bsp_pwm_phase_duty_t *p_duty)
{
    const svpwm_3level_phase_output_t *p_phase[3] = {
        &p_mod->output.phase_a, &p_mod->output.phase_b, &p_mod->output.phase_c};
    float current[3] = {p_mod->input.i_a, p_mod->input.i_b, p_mod->input.i_c};
    float pole[3] = {0};
    float average[3] = {0};
    float event[3] = {0};
    float ripple[3] = {0};
    float edge_ripple[3] = {0};
    uint32_t order[3] = {0u, 1u, 2u};
    float previous = 0.0f;
    float average_common = 0.0f;

    for (uint32_t phase = 0u; phase < 3u; ++phase)
    {
        average[phase] = p_phase[phase]->duty_p * p_mod->input.v_dc_p -
                         p_phase[phase]->duty_n * p_mod->input.v_dc_n;
        average_common += average[phase] / 3.0f;
        if (p_phase[phase]->duty_p > 0.0f)
        {
            pole[phase] = p_mod->input.v_dc_p;
            event[phase] = 0.5f * p_phase[phase]->duty_p;
        }
        else
        {
            event[phase] = 0.5f * (1.0f - p_phase[phase]->duty_n);
        }
    }
    for (uint32_t index = 1u; index < 3u; ++index)
    {
        uint32_t item = order[index];
        uint32_t position = index;
        while ((position > 0u) && (event[order[position - 1u]] > event[item]))
        {
            order[position] = order[position - 1u];
            --position;
        }
        order[position] = item;
    }
    for (uint32_t index = 0u; index < 3u; ++index)
    {
        uint32_t phase = order[index];
        float common = (pole[0] + pole[1] + pole[2]) / 3.0f;
        float step = (event[phase] - previous) * NPC_PWM_CONTROL_TS / NPC_EDGE_INDUCTANCE;
        for (uint32_t axis = 0u; axis < 3u; ++axis)
        {
            ripple[axis] += step * (pole[axis] - common - average[axis] + average_common);
        }
        edge_ripple[phase] = ripple[phase];
        previous = event[phase];
        pole[phase] = p_phase[phase]->duty_n > 0.0f ? -p_mod->input.v_dc_n : 0.0f;
    }
    for (uint32_t phase = 0u; phase < 3u; ++phase)
    {
        float first = current[phase] + edge_ripple[phase];
        float second = current[phase] - edge_ripple[phase];
        float correction = NPC_PWM_DEAD_TIME_S / NPC_PWM_CONTROL_TS *
                           (edge_positive_fraction(first) + edge_positive_fraction(second) - 1.0f);
        p_duty[phase].positive_duty = p_phase[phase]->duty_p;
        p_duty[phase].negative_duty = p_phase[phase]->duty_n;
        if (p_phase[phase]->duty_p > 0.0f)
        {
            p_duty[phase].positive_duty = fmaxf(0.0f, fminf(1.0f, p_phase[phase]->duty_p + correction));
        }
        else if (p_phase[phase]->duty_n > 0.0f)
        {
            p_duty[phase].negative_duty = fmaxf(0.0f, fminf(1.0f, p_phase[phase]->duty_n - correction));
        }
    }
}
#endif
