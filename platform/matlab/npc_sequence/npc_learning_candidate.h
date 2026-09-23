// SPDX-License-Identifier: MIT
/**
 * @file npc_learning_candidate.h
 * @brief Offline frequency-shaped repetitive voltage correction experiment.
 * @details Learns periodic nonfundamental output error using measured voltage.
 *          The inverse LC model includes PWM and computation delay. No true
 *          electrical state or load parameter enters the online update.
 * @author Max.Li
 * @date 2026-09-14
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef NPC_LEARNING_CANDIDATE_H
#define NPC_LEARNING_CANDIDATE_H
static double learning_u[2][36][2] = {
    {{0}}
};
static double learning_v[2][36][2] = {
    {{0}}
};
static void candidate_learning(npc_ctrl_t *p_control,
                               const npc_ctrl_sample_t *p_sample,
                               int delay,
                               int tick)
{
    double correction[2] = {0};

    if (tick < 15000)
    {
        return;
    }

    for (int harmonic = 2; harmonic <= 35; ++harmonic)
    {
        double angle  = harmonic * (double)p_sample->theta;
        double cosine = cos(angle);
        double sine   = sin(angle);

        for (int axis = 0; axis < 2; ++axis)
        {
            correction[axis] += learning_u[axis][harmonic][0] * cosine - learning_u[axis][harmonic][1] * sine;
            learning_v[axis][harmonic][0] += .02 * p_control->inter.v_ab[axis] * cosine;
            learning_v[axis][harmonic][1] -= .02 * p_control->inter.v_ab[axis] * sine;
        }

        if ((tick + 1) % 100 == 0)
        {
            double omega = harmonic * M_2PI * 50.0;
            double phase = omega * .0002 * (delay + .5);
            double sinc_gain = sin(omega * .0001) / (omega * .0001);
            double real_inverse = 1.0 - omega * omega * 80e-6 * 200e-6;
            double imaginary_inverse = omega * .002 * 200e-6;
            double real_comp = (real_inverse * cos(phase) - imaginary_inverse * sin(phase)) / sinc_gain;
            double imaginary_comp = (real_inverse * sin(phase) + imaginary_inverse * cos(phase)) / sinc_gain;

            for (int axis = 0; axis < 2; ++axis)
            {
                double re = learning_v[axis][harmonic][0];
                double im = learning_v[axis][harmonic][1];
                learning_u[axis][harmonic][0] =
                    .999 * learning_u[axis][harmonic][0] - .1 * (real_comp * re - imaginary_comp * im);
                learning_u[axis][harmonic][1] =
                    .999 * learning_u[axis][harmonic][1] - .1 * (real_comp * im + imaginary_comp * re);
                double scale =
                    fmin(1.0, 50.0 / fmax(hypot(learning_u[axis][harmonic][0], learning_u[axis][harmonic][1]), 1e-6));
                learning_u[axis][harmonic][0] *= scale;
                learning_u[axis][harmonic][1] *= scale;
                learning_v[axis][harmonic][0] = 0.0;
                learning_v[axis][harmonic][1] = 0.0;
            }
        }
    }
    double command[2] = {p_control->output.v_alpha + correction[0], p_control->output.v_beta + correction[1]};
    double phases[3] = {command[0],
                        -.5 * command[0] + M_SQRT3_2 * command[1],
                        -.5 * command[0] - M_SQRT3_2 * command[1]};
    double span = fmax(phases[0], fmax(phases[1], phases[2])) - fmin(phases[0], fmin(phases[1], phases[2]));
    double scale = fmin(1.0, .95 * (p_sample->v_dc_p + p_sample->v_dc_n) / fmax(span, 1e-6));
    p_control->output.v_alpha         = (float)(command[0] * scale);
    p_control->output.v_beta          = (float)(command[1] * scale);
    p_control->output.voltage_limited = scale < 1.0;
}
#endif
