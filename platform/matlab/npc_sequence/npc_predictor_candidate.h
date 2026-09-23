// SPDX-License-Identifier: MIT
/**
 * @file npc_predictor_candidate.h
 * @brief Offline delayed state-feedback voltage controller candidate.
 * @details Uses a voltage-only disturbance observer and exact discrete LC
 *          matrices. No true load current or simulator state enters control.
 * @author Max.Li
 * @date 2026-09-14
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef NPC_PREDICTOR_CANDIDATE_H
#define NPC_PREDICTOR_CANDIDATE_H
#include "predictor.h"
static double predictor_state[2][4] = {
    {0}
};
static double predictor_queued[2] = {0};

static void candidate_control(npc_ctrl_t *p_control, const npc_ctrl_sample_t *p_sample, int delay)
{
    double corrected[2][4] = {
        {0}
    };
    double next[2][4] = {
        {0}
    };
    double command[2] = {0};
    double angle = p_sample->theta + delay * .0002 * M_2PI * 50.0;
    double amplitude = p_sample->vd_pos_ref;

    for (int axis = 0; axis < 2; ++axis)
    {
        double error = p_control->inter.v_ab[axis] - predictor_state[axis][0];

        for (int row = 0; row < 4; ++row)
        {
            corrected[axis][row] = predictor_state[axis][row] + predictor_l[row] * error;
        }

        for (int row = 0; row < 4; ++row)
        {
            next[axis][row] = predictor_b[row] * predictor_queued[axis];

            for (int col = 0; col < 4; ++col)
            {
                next[axis][row] += predictor_a[row][col] * corrected[axis][col];
            }
        }
        const double *p_feedback = delay == 0 ? corrected[axis] : next[axis];
        double cosine = axis == 0 ? cos(angle) : sin(angle);
        double sine = axis == 0 ? sin(angle) : -cos(angle);
        command[axis] = amplitude * (predictor_reference[0] * cosine - predictor_reference[1] * sine)
                      - predictor_k[0] * p_feedback[0] - predictor_k[1] * p_feedback[1]
                      - predictor_disturbance[0] * p_feedback[2] - predictor_disturbance[1] * p_feedback[3]
                      - .1 * p_control->output.i_bias_ab[axis];
    }
    double phases[3] = {command[0],
                        -.5 * command[0] + M_SQRT3_2 * command[1],
                        -.5 * command[0] - M_SQRT3_2 * command[1]};
    double span = fmax(phases[0], fmax(phases[1], phases[2])) - fmin(phases[0], fmin(phases[1], phases[2]));
    double scale = fmin(1.0, .95 * (p_sample->v_dc_p + p_sample->v_dc_n) / fmax(span, 1e-6));

    for (int axis = 0; axis < 2; ++axis)
    {
        command[axis] *= scale;

        for (int row = 0; row < 4; ++row)
        {
            if (delay == 0)
            {
                next[axis][row] = predictor_b[row] * command[axis];

                for (int col = 0; col < 4; ++col)
                {
                    next[axis][row] += predictor_a[row][col] * corrected[axis][col];
                }
            }
            predictor_state[axis][row] = next[axis][row];
        }
        predictor_queued[axis] = command[axis];
    }
    p_control->output.v_alpha         = (float)command[0];
    p_control->output.v_beta          = (float)command[1];
    p_control->output.voltage_limited = scale < 1.0;
}
#endif
