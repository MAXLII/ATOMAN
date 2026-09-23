// SPDX-License-Identifier: MIT
/**
 * @file npc_average_balance_candidate.h
 * @brief Offline averaged midpoint-voltage feedback experiment.
 * @details Reject the three-times-line-frequency capacitor ripple from the
 *          balancing loop, instead of cancelling instantaneous neutral current.
 *          The ac voltage command and measured bus voltages determine all duties.
 */
#ifndef NPC_AVERAGE_BALANCE_CANDIDATE_H
#define NPC_AVERAGE_BALANCE_CANDIDATE_H
static double balance_delta_filtered    = 0.0;
static double balance_current_filtered  = 10.0;
static double balance_integral          = 0.0;
static double resonance_v_history[2][2] = {
    {0}
};
static double resonance_y_history[2][2] = {
    {0}
};
static void candidate_resonance_damping(npc_ctrl_t *p_ctrl, int mode)
{
    double gain = mode == 76 ? .15 : (mode == 77 ? .3 : .6);
    double correction[2] = {0};

    for (int axis = 0; axis < 2; ++axis)
    {
        double value = p_ctrl->inter.v_ab[axis];
        double filtered = (value - resonance_v_history[axis][1] - resonance_y_history[axis][1]) / 3.0;
        resonance_v_history[axis][1] = resonance_v_history[axis][0];
        resonance_v_history[axis][0] = value;
        resonance_y_history[axis][1] = resonance_y_history[axis][0];
        resonance_y_history[axis][0] = filtered;
        correction[axis]             = gain * filtered;
    }
    p_ctrl->output.v_alpha += (float)correction[0];
    p_ctrl->output.v_beta += (float)correction[1];
}
static void candidate_average_balance(svpwm_3level_t *p_mod, int mode)
{
    double vp = p_mod->input.v_dc_p;
    double vn = p_mod->input.v_dc_n;
    double v[3] = {p_mod->input.v_alpha,
                   -.5 * p_mod->input.v_alpha + M_SQRT3_2 * p_mod->input.v_beta,
                   -.5 * p_mod->input.v_alpha - M_SQRT3_2 * p_mod->input.v_beta};
    double max_v       = fmax(v[0], fmax(v[1], v[2]));
    double min_v       = fmin(v[0], fmin(v[1], v[2]));
    double current     = 0.0;
    double currents[3] = {p_mod->input.i_a, p_mod->input.i_b, p_mod->input.i_c};
    svpwm_3level_phase_output_t *phases[3] = {&p_mod->output.phase_a, &p_mod->output.phase_b, &p_mod->output.phase_c};

    for (int phase = 0; phase < 3; ++phase)
    {
        current += fabs(currents[phase]);
    }
    balance_delta_filtered += (1.0 - exp(-M_2PI * 10.0 * .0002)) * (vp - vn - balance_delta_filtered);
    balance_current_filtered += (1.0 - exp(-M_2PI * 10.0 * .0002)) * (current - balance_current_filtered);
    double balance_gain = .06 * .5 * (vp + vn) / fmax(balance_current_filtered, 10.0);

    if (mode == 81)
    {
        double sensitivity = 0.0;
        double center = .5 * (vp - vn - max_v - min_v);

        for (int phase = 0; phase < 3; ++phase)
        {
            sensitivity += (v[phase] + center >= 0.0 ? currents[phase] / vp : -currents[phase] / vn);
        }
        double regularization = fmax(10.0, .1 * balance_current_filtered) / (.5 * (vp + vn));
        balance_gain = .06 * sensitivity / (sensitivity * sensitivity + regularization * regularization);
    }
    double correction = balance_gain * (M_2PI * 2.0 * balance_delta_filtered + M_2PI * M_2PI * balance_integral);
    double base_offset = .5 * (vp - vn - max_v - min_v);
    double offset = base_offset + fmax(-150.0, fmin(150.0, correction));
    offset = fmax(-vn - min_v, fmin(vp - max_v, offset));

    if (    (mode >= 72)
         && (fabs(balance_gain) > 1e-6))
    {
        balance_integral +=
            .0002
            * (balance_delta_filtered + 10.0 * (offset - base_offset - correction) / (balance_gain * M_2PI * M_2PI));
    }
    p_mod->output.common_mode_v    = (float)offset;
    p_mod->output.midpoint_current = 0.0f;

    for (int phase = 0; phase < 3; ++phase)
    {
        double pole           = v[phase] + offset;
        phases[phase]->duty_p = (float)fmax(0.0, pole / vp);
        phases[phase]->duty_n = (float)fmax(0.0, -pole / vn);
        phases[phase]->duty_o = 1.0f - phases[phase]->duty_p - phases[phase]->duty_n;
        p_mod->output.midpoint_current += phases[phase]->duty_o * (float)currents[phase];
    }
}
#endif
