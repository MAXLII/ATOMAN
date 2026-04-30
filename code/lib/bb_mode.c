// SPDX-License-Identifier: MIT
/**
 * @file    bb_mode.c
 * @brief   bb_mode library module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Select Buck, Boost, Buck-Boost, and soft-switching operating modes
 *          - Calculate coordinated duty references from input, output, and inductor voltage feedback
 *          - Apply mode-transition thresholds and duty limits for bidirectional converter control
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "bb_mode.h"
#include <stddef.h>
#include "my_math.h"

static inline float bb_mode_lmt_min(float value, float min)
{
    return (value < min) ? min : value;
}

static inline float bb_mode_calc_buck_duty(float v_l, float v_in, float v_out)
{
    return (v_l + v_out) / v_in;
}

static inline float bb_mode_calc_boost_duty(float v_l, float v_in, float v_out)
{
    return (v_in - v_l) / v_out;
}

static inline float bb_mode_calc_boost_duty_with_buck(float v_l,
                                                      float v_in,
                                                      float v_out,
                                                      float buck_duty)
{
    return (v_in * buck_duty - v_l) / v_out;
}

static inline float bb_mode_calc_buck_duty_with_boost(float v_l,
                                                      float v_in,
                                                      float v_out,
                                                      float boost_duty)
{
    return (v_l + v_out * boost_duty) / v_in;
}

void bb_mode_init(bb_mode_t *p_mode,
                  float *p_v_l,
                  float *p_v_in,
                  float *p_v_out,
                  bb_mode_e mode)
{
    if (p_mode == NULL)
    {
        return;
    }

    p_mode->input.p_v_l = p_v_l;
    p_mode->input.p_v_in = p_v_in;
    p_mode->input.p_v_out = p_v_out;
    p_mode->inter.mode = mode;
    p_mode->output.buck_duty = 0.0f;
    p_mode->output.boost_duty = 0.0f;
    p_mode->output.is_half_freq = 0U;
}

void bb_mode_func(bb_mode_t *p_mode)
{
    if ((p_mode == NULL) ||
        (p_mode->input.p_v_l == NULL) ||
        (p_mode->input.p_v_in == NULL) ||
        (p_mode->input.p_v_out == NULL))
    {
        return;
    }

    const float v_l = *p_mode->input.p_v_l;
    const float v_in = bb_mode_lmt_min(*p_mode->input.p_v_in, 0.001f);
    const float v_out = bb_mode_lmt_min(*p_mode->input.p_v_out, 0.001f);

    switch (p_mode->inter.mode)
    {
    case BB_MODE_BUCK:
        p_mode->output.buck_duty = bb_mode_calc_buck_duty(v_l, v_in, v_out);
        if (p_mode->output.buck_duty > BB_MODE_SSW_TO_BB_MODE_THR)
        {
            p_mode->output.boost_duty = BB_MODE_DUTY_MAX;
            p_mode->output.buck_duty = bb_mode_calc_buck_duty_with_boost(v_l,
                                                                         v_in,
                                                                         v_out,
                                                                         p_mode->output.boost_duty);
            p_mode->inter.mode = BB_MODE_BUCK_BOOST;
        }
        else
        {
            p_mode->output.boost_duty = 1.0f;
        }
        break;

    case BB_MODE_BOOST:
        p_mode->output.boost_duty = bb_mode_calc_boost_duty(v_l, v_in, v_out);
        if (p_mode->output.boost_duty > BB_MODE_SSW_TO_BB_MODE_THR)
        {
            p_mode->output.buck_duty = BB_MODE_DUTY_MAX;
            p_mode->output.boost_duty = bb_mode_calc_boost_duty_with_buck(v_l,
                                                                          v_in,
                                                                          v_out,
                                                                          p_mode->output.buck_duty);
            p_mode->inter.mode = BB_MODE_BUCK_BOOST;
        }
        else
        {
            p_mode->output.buck_duty = 1.0f;
        }
        break;

    case BB_MODE_BUCK_BOOST:
        p_mode->output.buck_duty = bb_mode_calc_buck_duty(v_l, v_in, v_out);
        if (p_mode->output.buck_duty > BB_MODE_DUTY_MAX)
        {
            p_mode->output.buck_duty = BB_MODE_DUTY_MAX;
            p_mode->output.boost_duty = bb_mode_calc_boost_duty_with_buck(v_l,
                                                                          v_in,
                                                                          v_out,
                                                                          p_mode->output.buck_duty);
            if ((p_mode->output.boost_duty + p_mode->output.buck_duty) > BB_MODE_DUTY_SUM)
            {
                p_mode->output.buck_duty = BB_MODE_DUTY_SUM - p_mode->output.boost_duty;
                p_mode->output.boost_duty = bb_mode_calc_boost_duty_with_buck(v_l,
                                                                              v_in,
                                                                              v_out,
                                                                              p_mode->output.buck_duty);
            }

            if (p_mode->output.boost_duty < BB_MODE_BB_MODE_TO_SSW_THR)
            {
                p_mode->inter.mode = BB_MODE_BOOST;
                p_mode->output.buck_duty = 1.0f;
                p_mode->output.boost_duty = bb_mode_calc_boost_duty(v_l, v_in, v_out);
            }
        }
        else
        {
            p_mode->output.boost_duty = BB_MODE_BUCK_BOOST_FIXED_BOOST_DUTY;
            p_mode->output.buck_duty = bb_mode_calc_buck_duty_with_boost(v_l,
                                                                         v_in,
                                                                         v_out,
                                                                         p_mode->output.boost_duty);
            if ((p_mode->output.boost_duty + p_mode->output.buck_duty) > BB_MODE_DUTY_SUM)
            {
                p_mode->output.boost_duty = BB_MODE_DUTY_SUM - p_mode->output.buck_duty;
                p_mode->output.buck_duty = bb_mode_calc_buck_duty_with_boost(v_l,
                                                                             v_in,
                                                                             v_out,
                                                                             p_mode->output.boost_duty);
            }

            if (p_mode->output.buck_duty < BB_MODE_BB_MODE_TO_SSW_THR)
            {
                p_mode->inter.mode = BB_MODE_BUCK;
                p_mode->output.boost_duty = 1.0f;
                p_mode->output.buck_duty = bb_mode_calc_buck_duty(v_l, v_in, v_out);
            }
        }
        break;

    default:
        bb_mode_init(p_mode,
                     p_mode->input.p_v_l,
                     p_mode->input.p_v_in,
                     p_mode->input.p_v_out,
                     BB_MODE_BUCK);
        break;
    }

    p_mode->output.is_half_freq = (p_mode->inter.mode == BB_MODE_BUCK_BOOST) ? 1U : 0U;

    UP_DN_LMT(p_mode->output.buck_duty, 1.0f, 0.0f);
    UP_DN_LMT(p_mode->output.boost_duty, 1.0f, 0.0f);
}
