// SPDX-License-Identifier: MIT
/**
 * @file    resonant_i32.c
 * @brief   Integer ideal resonant controller module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Convert resonant design parameters into Q29 coefficients outside the ISR
 *          - Execute the resonant recurrence across distinct input and output code domains
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
#include "resonant_i32.h"

#include <limits.h>
#include <stddef.h>

#define RESONANT_I32_COEFF_SCALE (536870912.0f)

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

bool resonant_i32_design_coeff(resonant_i32_coeff_t *p_coeff,
                               float gain,
                               uint32_t order,
                               float omega_radps,
                               float ts,
                               float output_code_per_unit,
                               float input_code_per_unit)
{
    float omega_n = 0.0f;     /**< Resonant angular frequency. */
    float denominator = 0.0f; /**< Bilinear-transform denominator. */
    float domain_gain = 0.0f; /**< Input-code to output-code conversion gain. */
    float b0 = 0.0f;          /**< Designed input coefficient. */
    float a1 = 0.0f;          /**< Designed first feedback coefficient. */

    if ((p_coeff == NULL) ||
        (gain < 0.0f) ||
        (order == 0U) ||
        (omega_radps <= 0.0f) ||
        (ts <= 0.0f) ||
        (output_code_per_unit <= 0.0f) ||
        (input_code_per_unit <= 0.0f))
    {
        return false;
    }

    omega_n = (float)order * omega_radps;
    denominator = 4.0f + (ts * ts * omega_n * omega_n);
    domain_gain = output_code_per_unit / input_code_per_unit;
    b0 = (2.0f * gain * ts * domain_gain) / denominator;
    a1 = ((2.0f * ts * ts * omega_n * omega_n) - 8.0f) / denominator;

    p_coeff->b0 = float_to_i32(b0 * RESONANT_I32_COEFF_SCALE);
    p_coeff->b2 = -p_coeff->b0;
    p_coeff->a1 = float_to_i32(a1 * RESONANT_I32_COEFF_SCALE);
    p_coeff->a2 = (int32_t)(1UL << RESONANT_I32_COEFF_Q_SHIFT);
    return true;
}

bool resonant_i32_init(resonant_i32_t *p_resonant, const resonant_i32_coeff_t *p_coeff)
{
    if ((p_resonant == NULL) ||
        (p_coeff == NULL))
    {
        return false;
    }
    p_resonant->coeff = *p_coeff;
    resonant_i32_reset(p_resonant);
    return true;
}

bool resonant_i32_set_coeff(resonant_i32_t *p_resonant, const resonant_i32_coeff_t *p_coeff)
{
    if ((p_resonant == NULL) ||
        (p_coeff == NULL))
    {
        return false;
    }
    p_resonant->coeff = *p_coeff;
    return true;
}

void resonant_i32_reset(resonant_i32_t *p_resonant)
{
    if (p_resonant == NULL)
    {
        return;
    }
    p_resonant->x1 = 0;
    p_resonant->x2 = 0;
    p_resonant->y1 = 0;
    p_resonant->y2 = 0;
    p_resonant->residual_q29 = 0;
}

int32_t resonant_i32_cal(resonant_i32_t *p_resonant, int32_t input)
{
    int64_t accumulator = 0; /**< Q29 multiply-accumulate result. */
    int64_t scaled = 0;      /**< Output-domain value before saturation. */
    int32_t output = 0;      /**< Current output in the configured code domain. */

    if (p_resonant == NULL)
    {
        return 0;
    }

    accumulator = ((int64_t)p_resonant->coeff.b0 * (int64_t)input) +
                  ((int64_t)p_resonant->coeff.b2 * (int64_t)p_resonant->x2) -
                  ((int64_t)p_resonant->coeff.a1 * (int64_t)p_resonant->y1) -
                  ((int64_t)p_resonant->coeff.a2 * (int64_t)p_resonant->y2) +
                  p_resonant->residual_q29;
    scaled = accumulator >> RESONANT_I32_COEFF_Q_SHIFT;
    output = sat_i64_to_i32(scaled);
    if (scaled == (int64_t)output)
    {
        p_resonant->residual_q29 = accumulator -
                                   ((int64_t)output *
                                    (int64_t)(1UL << RESONANT_I32_COEFF_Q_SHIFT));
    }
    else
    {
        p_resonant->residual_q29 = 0;
    }
    p_resonant->x2 = p_resonant->x1;
    p_resonant->x1 = input;
    p_resonant->y2 = p_resonant->y1;
    p_resonant->y1 = output;
    return output;
}
