// SPDX-License-Identifier: MIT
/**
 * @file npc_modulation_candidate.h
 * @brief Offline three-state duty allocation with continuous common-mode voltage.
 * @details Preserves each pole average while allocating midpoint dwell to meet
 *          the requested neutral current. Analytical prototype, not a copy of
 *          the published virtual-vector switching sequence.
 * @author Max.Li
 * @date 2026-09-14
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef NPC_MODULATION_CANDIDATE_H
#define NPC_MODULATION_CANDIDATE_H

static void candidate_modulate(svpwm_3level_t *p_mod)
{
    double phase[3] = {p_mod->input.v_alpha,
        -0.5 * p_mod->input.v_alpha + M_SQRT3_2 * p_mod->input.v_beta,
        -0.5 * p_mod->input.v_alpha - M_SQRT3_2 * p_mod->input.v_beta};
    double current[3] = {p_mod->input.i_a, p_mod->input.i_b, p_mod->input.i_c};
    double vp = p_mod->input.v_dc_p;
    double vn = p_mod->input.v_dc_n;
    double minimum = fmin(phase[0], fmin(phase[1], phase[2]));
    double maximum = fmax(phase[0], fmax(phase[1], phase[2]));
    double offset = 0.5 * (vp - vn - minimum - maximum);
    double zero[3] = {0};
    double floor = 0.1;
    double initial = 0.0;
    double target = -p_mod->cfg.midpoint_kp * (vp - vn);
    bool active[3] = {false};
    for (int phase_index = 0; phase_index < 3; ++phase_index)
    {
        phase[phase_index] += offset;
        zero[phase_index] = 1.0 - fmax(phase[phase_index] / vp, -phase[phase_index] / vn);
        zero[phase_index] = fmax(0.0, zero[phase_index]);
        floor = fmin(floor, zero[phase_index]);
        initial += zero[phase_index] * current[phase_index];
    }
    double direction = initial >= target ? 1.0 : -1.0;
    double remaining = fabs(initial - target);
    for (int phase_index = 0; phase_index < 3; ++phase_index)
    {
        active[phase_index] = direction * current[phase_index] > 0.0;
    }
    for (int iteration = 0; iteration < 3; ++iteration)
    {
        double denominator = 0.0;
        bool clipped = false;
        for (int phase_index = 0; phase_index < 3; ++phase_index)
        {
            if (active[phase_index])
            {
                denominator += current[phase_index] * current[phase_index];
            }
        }
        if (denominator == 0.0)
        {
            break;
        }
        double multiplier = remaining / denominator;
        for (int phase_index = 0; phase_index < 3; ++phase_index)
        {
            double sensitivity = direction * current[phase_index];
            if (active[phase_index] && multiplier * sensitivity >= zero[phase_index] - floor)
            {
                remaining -= sensitivity * (zero[phase_index] - floor);
                zero[phase_index] = floor;
                active[phase_index] = false;
                clipped = true;
            }
        }
        if (!clipped)
        {
            for (int phase_index = 0; phase_index < 3; ++phase_index)
            {
                if (active[phase_index])
                {
                    zero[phase_index] -= multiplier * direction * current[phase_index];
                }
            }
            break;
        }
    }
    svpwm_3level_phase_output_t *p_phase_output[3] = {
        &p_mod->output.phase_a, &p_mod->output.phase_b, &p_mod->output.phase_c};
    p_mod->output.midpoint_current_ref = (float)target;
    p_mod->output.midpoint_current = 0.0f;
    p_mod->output.common_mode_v = (float)offset;
    for (int phase_index = 0; phase_index < 3; ++phase_index)
    {
        double positive = (phase[phase_index] + vn * (1.0 - zero[phase_index])) / (vp + vn);
        double negative = (vp * (1.0 - zero[phase_index]) - phase[phase_index]) / (vp + vn);
        p_phase_output[phase_index]->duty_p = (float)fmax(0.0, positive);
        p_phase_output[phase_index]->duty_n = (float)fmax(0.0, negative);
        p_phase_output[phase_index]->duty_o = (float)zero[phase_index];
        p_mod->output.midpoint_current += (float)(zero[phase_index] * current[phase_index]);
    }
}
#endif
