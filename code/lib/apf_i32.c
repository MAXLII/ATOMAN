// SPDX-License-Identifier: MIT
/**
 * @file    apf_i32.c
 * @brief   Integer first-order all-pass filter module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Convert floating design parameters into Q30 coefficients outside the ISR
 *          - Execute the all-pass recurrence with integer state
 *          - Saturate 64-bit multiply-accumulate results before returning int32_t data
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Runtime path uses no division
 *          - No hardware access
 *
 * @author  Max.Li
 * @date    2026-08-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "apf_i32.h"

#include <limits.h>
#include <stddef.h>

#define APF_I32_COEFF_SCALE (1073741824.0f)

static int32_t float_to_i32(float value)
{
    if (value >= (float)INT32_MAX)
    {
        return INT32_MAX;
    }
    if (value <= (float)INT32_MIN)
    {
        return INT32_MIN;
    }
    return (value >= 0.0f) ? (int32_t)(value + 0.5f) : (int32_t)(value - 0.5f);
}

static int32_t sat_i64_to_i32(int64_t value)
{
    if (value > (int64_t)INT32_MAX)
    {
        return INT32_MAX;
    }
    if (value < (int64_t)INT32_MIN)
    {
        return INT32_MIN;
    }
    return (int32_t)value;
}

bool apf_i32_design_coeff(apf_i32_coeff_t *p_coeff, float omega_radps, float ts)
{
    float denominator = 0.0f; /**< Bilinear-transform denominator. */
    float b0 = 0.0f;          /**< Designed all-pass coefficient. */

    if ((p_coeff == NULL) ||
        (omega_radps <= 0.0f) ||
        (ts <= 0.0f))
    {
        return false;
    }

    denominator = (omega_radps * ts) + 2.0f;
    b0 = ((omega_radps * ts) - 2.0f) / denominator;
    p_coeff->b0 = float_to_i32(b0 * APF_I32_COEFF_SCALE);
    p_coeff->b1 = (int32_t)(1UL << APF_I32_COEFF_Q_SHIFT);
    p_coeff->a1 = p_coeff->b0;
    return true;
}

bool apf_i32_init(apf_i32_t *p_apf, const apf_i32_coeff_t *p_coeff)
{
    if ((p_apf == NULL) ||
        (p_coeff == NULL))
    {
        return false;
    }
    p_apf->coeff = *p_coeff;
    apf_i32_reset(p_apf);
    return true;
}

bool apf_i32_set_coeff(apf_i32_t *p_apf, const apf_i32_coeff_t *p_coeff)
{
    if ((p_apf == NULL) ||
        (p_coeff == NULL))
    {
        return false;
    }
    p_apf->coeff = *p_coeff;
    return true;
}

void apf_i32_reset(apf_i32_t *p_apf)
{
    if (p_apf == NULL)
    {
        return;
    }
    p_apf->x1 = 0;
    p_apf->y1 = 0;
    p_apf->residual_q30 = 0;
}

int32_t apf_i32_cal(apf_i32_t *p_apf, int32_t input)
{
    int64_t accumulator = 0; /**< Q30 multiply-accumulate result. */
    int64_t scaled = 0;      /**< Output-domain value before saturation. */
    int32_t output = 0;      /**< Current output in the input code domain. */

    if (p_apf == NULL)
    {
        return 0;
    }

    accumulator = ((int64_t)p_apf->coeff.b0 * (int64_t)input) +
                  ((int64_t)p_apf->coeff.b1 * (int64_t)p_apf->x1) -
                  ((int64_t)p_apf->coeff.a1 * (int64_t)p_apf->y1) +
                  p_apf->residual_q30;
    scaled = accumulator >> APF_I32_COEFF_Q_SHIFT;
    output = sat_i64_to_i32(scaled);
    if (scaled == (int64_t)output)
    {
        p_apf->residual_q30 = accumulator -
                              ((int64_t)output *
                               (int64_t)(1UL << APF_I32_COEFF_Q_SHIFT));
    }
    else
    {
        p_apf->residual_q30 = 0;
    }
    p_apf->x1 = input;
    p_apf->y1 = output;
    return output;
}
