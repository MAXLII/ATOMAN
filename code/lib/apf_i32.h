// SPDX-License-Identifier: MIT
/**
 * @file    apf_i32.h
 * @brief   Integer first-order all-pass filter public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define Q30 all-pass filter coefficients and integer runtime state
 *          - Separate non-real-time coefficient design from ISR calculation
 *          - Provide reset, coefficient update, and saturating sample calculation APIs
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
#ifndef APF_I32_H
#define APF_I32_H

#include <stdbool.h>
#include <stdint.h>

#define APF_I32_COEFF_Q_SHIFT (30U)

typedef struct
{
    int32_t b0; /**< Current-input Q30 numerator coefficient. */
    int32_t b1; /**< Previous-input Q30 numerator coefficient. */
    int32_t a1; /**< Previous-output Q30 denominator coefficient. */
} apf_i32_coeff_t;

typedef struct
{
    apf_i32_coeff_t coeff; /**< Coefficients used by the ISR calculation. */
    int32_t x1;            /**< Previous input sample in the caller's code domain. */
    int32_t y1;            /**< Previous output sample in the caller's code domain. */
    int64_t residual_q30;  /**< Bounded fractional recurrence residue retained across samples. */
} apf_i32_t;

bool apf_i32_design_coeff(apf_i32_coeff_t *p_coeff, float omega_radps, float ts);
bool apf_i32_init(apf_i32_t *p_apf, const apf_i32_coeff_t *p_coeff);
bool apf_i32_set_coeff(apf_i32_t *p_apf, const apf_i32_coeff_t *p_coeff);
void apf_i32_reset(apf_i32_t *p_apf);
int32_t apf_i32_cal(apf_i32_t *p_apf, int32_t input);

#endif
