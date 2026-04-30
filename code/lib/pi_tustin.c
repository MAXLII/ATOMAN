// SPDX-License-Identifier: MIT
/**
 * @file    pi_tustin.c
 * @brief   pi_tustin library module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement a PI controller using Tustin discretization
 *          - Calculate controller output from reference and feedback pointers
 *          - Support coefficient update and state reset for runtime tuning
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
#include "pi_tustin.h"
#include "my_math.h"
#include "section.h"
#include <stdbool.h>
#include <math.h>

bool pi_tustin_init(pi_tustin_t *p_str,
                    float kp,
                    float ki,
                    float ts,
                    float up_lmt,
                    float dn_lmt,
                    float *p_ref,
                    float *p_act)
{
    if ((p_ref == NULL) ||
        (p_act == NULL) ||
        (p_str == NULL))
    {
        return false;
    }
    p_str->input.p_ref = p_ref;
    p_str->input.p_act = p_act;

    p_str->inter.a1 = 0.0f;
    p_str->inter.b0 = 0.0f;
    p_str->inter.b1 = 0.0f;
    p_str->inter.b1_inv = 0.0f;

    p_str->inter.up_lmt = up_lmt;
    p_str->inter.dn_lmt = dn_lmt;

    p_str->inter.e[0] = 0.0f;
    p_str->inter.e[1] = 0.0f;
    p_str->inter.u[0] = 0.0f;
    p_str->inter.u[1] = 0.0f;
    p_str->output.val = 0.0f;

    if (!pi_tustin_update(p_str, kp, ki, ts))
    {
        return false;
    }
    return true;
}

bool pi_tustin_cal(pi_tustin_t *p_str)
{
    if ((p_str == NULL) ||
        (p_str->input.p_ref == NULL) ||
        (p_str->input.p_act == NULL))
    {
        return false;
    }

    p_str->inter.e[0] = *p_str->input.p_ref - *p_str->input.p_act;
    p_str->inter.u[1] = p_str->inter.u[0];

    p_str->inter.u[0] = p_str->inter.b0 * p_str->inter.e[0] +
                        p_str->inter.b1 * p_str->inter.e[1] -
                        p_str->inter.a1 * p_str->inter.u[1];

    if (p_str->inter.u[0] > p_str->inter.up_lmt)
    {
        p_str->inter.u[0] = p_str->inter.up_lmt;
        p_str->inter.e[1] = (p_str->inter.u[0] -
                             p_str->inter.b0 * p_str->inter.e[0] +
                             p_str->inter.a1 * p_str->inter.u[1]) *
                            p_str->inter.b1_inv;
    }
    else if (p_str->inter.u[0] < p_str->inter.dn_lmt)
    {
        p_str->inter.u[0] = p_str->inter.dn_lmt;
        p_str->inter.e[1] = (p_str->inter.u[0] -
                             p_str->inter.b0 * p_str->inter.e[0] +
                             p_str->inter.a1 * p_str->inter.u[1]) *
                            p_str->inter.b1_inv;
    }
    else
    {
        p_str->inter.e[1] = p_str->inter.e[0];
    }

    p_str->output.val = p_str->inter.u[0];
    return true;
}

bool pi_tustin_update(pi_tustin_t *p_str,
                      float kp,
                      float ki,
                      float ts)
{
    if ((kp <= 0.0f) ||
        (ki < 0.0f) ||
        (ts <= 0.0f))
    {
        return false;
    }

    if (ts * ki >= 2.0f * kp)
    {
        return false;
    }

    const float n0 = ts * ki + 2.0f * kp;
    const float n1 = ts * ki - 2.0f * kp;
    const float d0 = 2.0f;
    const float d1 = -2.0f;

    float b1 = n1 / d0;

    p_str->inter.a1 = d1 / d0;
    p_str->inter.b0 = n0 / d0;
    p_str->inter.b1 = b1;
    p_str->inter.b1_inv = 1.0f / b1;

    return true;
}

void pi_tustin_reset(pi_tustin_t *p_str)
{
    p_str->inter.e[0] = 0.0f;
    p_str->inter.e[1] = 0.0f;
    p_str->inter.u[0] = 0.0f;
    p_str->inter.u[1] = 0.0f;
    p_str->output.val = 0.0f;
}
