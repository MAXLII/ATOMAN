// SPDX-License-Identifier: MIT
/**
 * @file    notch_i32.c
 * @brief   Integer notch filter module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Convert notch filter floating design parameters into fixed-point coefficients
 *          - Run the int32_t second-order IIR recurrence in caller-owned state
 *          - Saturate integer filter output to avoid wraparound
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe when caller owns the instance and input pointer
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-27
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "notch_i32.h"
#include <limits.h>
#include <stddef.h>

static int32_t notch_i32_sat_i64_to_i32(int64_t val)
{
    if (val > (int64_t)INT32_MAX)
    {
        return INT32_MAX;
    }
    else if (val < (int64_t)INT32_MIN)
    {
        return INT32_MIN;
    }

    return (int32_t)val;
}

static int32_t notch_i32_float_to_i32(float val)
{
    if (val >= (float)INT32_MAX)
    {
        return INT32_MAX;
    }
    else if (val <= (float)INT32_MIN)
    {
        return INT32_MIN;
    }

    if (val >= 0.0f)
    {
        return (int32_t)(val + 0.5f);
    }

    return (int32_t)(val - 0.5f);
}

void notch_i32_reset(notch_i32_t *p_notch)
{
    if (p_notch == NULL)
    {
        return;
    }

    p_notch->inter.x[0] = 0;
    p_notch->inter.x[1] = 0;
    p_notch->inter.x[2] = 0;
    p_notch->inter.y[0] = 0;
    p_notch->inter.y[1] = 0;
    p_notch->inter.y[2] = 0;
    p_notch->output.val = 0;
}

bool notch_i32_update(notch_i32_t *p_notch,
                      float w0,
                      float wb,
                      float ts,
                      uint8_t coeff_q_shift)
{
    float coeff_q = 0.0f;
    float n0 = 0.0f;
    float n1 = 0.0f;
    float n2 = 0.0f;
    float d0 = 0.0f;
    float d1 = 0.0f;
    float d2 = 0.0f;

    if ((p_notch == NULL) ||
        (w0 <= 0.0f) ||
        (wb <= 0.0f) ||
        (ts <= 0.0f) ||
        (coeff_q_shift >= 30U))
    {
        return false;
    }

    coeff_q = (float)(1L << coeff_q_shift);

    n0 = ts * ts * w0 * w0 + 4.0f;
    n1 = 2.0f * ts * ts * w0 * w0 - 8.0f;
    n2 = ts * ts * w0 * w0 + 4.0f;
    d0 = ts * ts * w0 * w0 + 2.0f * ts * wb + 4.0f;
    d1 = 2.0f * ts * ts * w0 * w0 - 8.0f;
    d2 = ts * ts * w0 * w0 - 2.0f * ts * wb + 4.0f;

    p_notch->inter.b[0] = notch_i32_float_to_i32((n0 / d0) * coeff_q);
    p_notch->inter.b[1] = notch_i32_float_to_i32((n1 / d0) * coeff_q);
    p_notch->inter.b[2] = notch_i32_float_to_i32((n2 / d0) * coeff_q);
    p_notch->inter.a[0] = notch_i32_float_to_i32((d1 / d0) * coeff_q);
    p_notch->inter.a[1] = notch_i32_float_to_i32((d2 / d0) * coeff_q);
    p_notch->inter.coeff_q_shift = coeff_q_shift;

    return true;
}

bool notch_i32_init(notch_i32_t *p_notch,
                    float w0,
                    float wb,
                    float ts,
                    uint8_t coeff_q_shift,
                    int32_t *p_input)
{
    if ((p_notch == NULL) ||
        (p_input == NULL))
    {
        return false;
    }

    p_notch->input.p_input = p_input;
    notch_i32_reset(p_notch);

    return notch_i32_update(p_notch, w0, wb, ts, coeff_q_shift);
}

bool notch_i32_cal(notch_i32_t *p_notch)
{
    int64_t y = 0;

    if ((p_notch == NULL) ||
        (p_notch->input.p_input == NULL))
    {
        return false;
    }

    p_notch->inter.x[2] = p_notch->inter.x[1];
    p_notch->inter.x[1] = p_notch->inter.x[0];
    p_notch->inter.x[0] = *p_notch->input.p_input;

    p_notch->inter.y[2] = p_notch->inter.y[1];
    p_notch->inter.y[1] = p_notch->inter.y[0];

    y = ((int64_t)p_notch->inter.b[0] * (int64_t)p_notch->inter.x[0]) +
        ((int64_t)p_notch->inter.b[1] * (int64_t)p_notch->inter.x[1]) +
        ((int64_t)p_notch->inter.b[2] * (int64_t)p_notch->inter.x[2]) -
        ((int64_t)p_notch->inter.a[0] * (int64_t)p_notch->inter.y[1]) -
        ((int64_t)p_notch->inter.a[1] * (int64_t)p_notch->inter.y[2]);
    y >>= p_notch->inter.coeff_q_shift;

    p_notch->inter.y[0] = notch_i32_sat_i64_to_i32(y);
    p_notch->output.val = p_notch->inter.y[0];

    return true;
}
