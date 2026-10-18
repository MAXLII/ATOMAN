// SPDX-License-Identifier: MIT
/**
 * @file pi_tustin.c
 * @brief pi_tustin library module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement float and int32_t PI controllers using Tustin discretization
 *          - Calculate controller output from reference and feedback pointers
 *          - Support coefficient update and state reset for runtime tuning
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author Max.Li
 * @date 2026-05-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "pi_tustin.h"

#include <stddef.h>

bool pi_tustin_init(pi_tustin_t *p_str,
                    float kp,
                    float ki,
                    float ts,
                    float up_lmt,
                    float dn_lmt,
                    float *p_ref,
                    float *p_act)
{
    if (    (p_ref == NULL)
         || (p_act == NULL)
         || (p_str == NULL))
    {
        return false;
    }
    p_str->input.p_ref = p_ref;
    p_str->input.p_act = p_act;

    p_str->inter.a1     = 0.0f;
    p_str->inter.b0     = 0.0f;
    p_str->inter.b1     = 0.0f;
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

PI_TUSTIN_STATUS_E pi_tustin_cal(pi_tustin_t *p_str)
{
    PI_TUSTIN_STATUS_E status = PI_TUSTIN_STATUS_NORMAL; /* Saturation state for this calculation. */

    if (    (p_str == NULL)
         || (p_str->input.p_ref == NULL)
         || (p_str->input.p_act == NULL))
    {
        return PI_TUSTIN_STATUS_INVALID;
    }

    p_str->inter.e[0] = *p_str->input.p_ref - *p_str->input.p_act;
    p_str->inter.u[1] = p_str->inter.u[0];

    p_str->inter.u[0] = p_str->inter.b0 * p_str->inter.e[0] + p_str->inter.b1 * p_str->inter.e[1]
                      - p_str->inter.a1 * p_str->inter.u[1];

    if (p_str->inter.u[0] > p_str->inter.up_lmt)
    {
        status = PI_TUSTIN_STATUS_POS_SATURATION;
        p_str->inter.u[0] = p_str->inter.up_lmt;
        p_str->inter.e[1] = (p_str->inter.u[0] - p_str->inter.b0 * p_str->inter.e[0]
                             + p_str->inter.a1 * p_str->inter.u[1])
                          * p_str->inter.b1_inv;
    }
    else if (p_str->inter.u[0] < p_str->inter.dn_lmt)
    {
        status = PI_TUSTIN_STATUS_NEG_SATURATION;
        p_str->inter.u[0] = p_str->inter.dn_lmt;
        p_str->inter.e[1] = (p_str->inter.u[0] - p_str->inter.b0 * p_str->inter.e[0]
                             + p_str->inter.a1 * p_str->inter.u[1])
                          * p_str->inter.b1_inv;
    }
    else
    {
        p_str->inter.e[1] = p_str->inter.e[0];
    }

    p_str->output.val = p_str->inter.u[0];
    return status;
}

bool pi_tustin_update(pi_tustin_t *p_str,
                      float kp,
                      float ki,
                      float ts)
{
    if (    (kp <= 0.0f)
         || (ki < 0.0f)
         || (ts <= 0.0f))
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

    p_str->inter.a1     = d1 / d0;
    p_str->inter.b0     = n0 / d0;
    p_str->inter.b1     = b1;
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

static int64_t pi_tustin_i32_float_to_i64(float val)
{
    if (val >= (float)INT64_MAX)
    {
        return INT64_MAX;
    }
    else if (val <= (float)INT64_MIN)
    {
        return INT64_MIN;
    }

    if (val >= 0.0f)
    {
        return (int64_t)(val + 0.5f);
    }

    return (int64_t)(val - 0.5f);
}

static int32_t pi_tustin_i32_div2_i64_to_i32(int64_t val)
{
    /* Use a shift-based half calculation so coefficient update avoids 64-bit division. */
    int64_t half = 0;

    if (val == INT64_MIN)
    {
        return INT32_MIN;
    }

    if (val < 0)
    {
        half = -((-val) >> 1);
    }
    else
    {
        half = val >> 1;
    }

    return pi_tustin_i32_sat_i64_to_i32(half);
}

bool pi_tustin_i32_init(pi_tustin_i32_t *p_str,
                        float kp,
                        float ki,
                        float ts,
                        int32_t up_lmt,
                        int32_t dn_lmt,
                        int32_t *p_ref,
                        int32_t *p_act)
{
    if (    (p_ref == NULL)
         || (p_act == NULL)
         || (p_str == NULL)
         || (up_lmt < dn_lmt))
    {
        return false;
    }

    p_str->input.p_ref = p_ref;
    p_str->input.p_act = p_act;

    p_str->inter.a1     = 0;
    p_str->inter.b0     = 0;
    p_str->inter.b1     = 0;
    p_str->inter.up_lmt = up_lmt;
    p_str->inter.dn_lmt = dn_lmt;
    p_str->inter.e[0]   = 0;
    p_str->inter.e[1]   = 0;
    p_str->inter.u[0]   = 0;
    p_str->inter.u[1]   = 0;
    p_str->output.val   = 0;

    if (!pi_tustin_i32_update(p_str, kp, ki, ts))
    {
        return false;
    }

    return true;
}

PI_TUSTIN_STATUS_E pi_tustin_i32_cal(pi_tustin_i32_t *p_str)
{
    if ((p_str == NULL) || /* Controller storage must be valid. */
        (p_str->input.p_ref == NULL) || /* Reference input must be bound. */
        (p_str->input.p_act == NULL)) /* Feedback input must be bound. */
    {
        return PI_TUSTIN_STATUS_INVALID;
    }

    return pi_tustin_i32_cal_inline(p_str);
}

bool pi_tustin_i32_update(pi_tustin_i32_t *p_str,
                          float kp,
                          float ki,
                          float ts)
{
    int64_t n0         = 0;
    int64_t n1         = 0;
    int64_t kp_term    = 0;
    int64_t ki_ts_term = 0;

    if (    (p_str == NULL)
         || (kp <= 0.0f)
         || (ki < 0.0f)
         || (ts <= 0.0f))
    {
        return false;
    }

    kp_term    = pi_tustin_i32_float_to_i64(2.0f * kp);
    ki_ts_term = pi_tustin_i32_float_to_i64(ki * ts);

    if (ki_ts_term >= kp_term)
    {
        return false;
    }

    n0 = ki_ts_term + kp_term;
    n1 = ki_ts_term - kp_term;

    p_str->inter.a1 = -1;
    p_str->inter.b0 = pi_tustin_i32_div2_i64_to_i32(n0);
    p_str->inter.b1 = pi_tustin_i32_div2_i64_to_i32(n1);

    return true;
}

void pi_tustin_i32_reset(pi_tustin_i32_t *p_str)
{
    if (p_str == NULL)
    {
        return;
    }

    p_str->inter.e[0] = 0;
    p_str->inter.e[1] = 0;
    p_str->inter.u[0] = 0;
    p_str->inter.u[1] = 0;
    p_str->output.val = 0;
}
