// SPDX-License-Identifier: MIT
/**
 * @file    resonant_i32.h
 * @brief   Integer ideal resonant controller public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define Q29 resonant coefficients with separate input and output code domains
 *          - Separate coefficient design from the integer ISR recurrence
 *          - Provide reset, coefficient update, and saturating calculation APIs
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
#ifndef RESONANT_I32_H
#define RESONANT_I32_H

#include <stdbool.h>
#include <stdint.h>

#define RESONANT_I32_COEFF_Q_SHIFT (29U)

typedef struct
{
    int32_t b0; /**< Current-input Q29 coefficient including code-domain conversion. */
    int32_t b2; /**< Two-sample-delayed Q29 input coefficient. */
    int32_t a1; /**< One-sample-delayed Q29 output coefficient. */
    int32_t a2; /**< Two-sample-delayed Q29 output coefficient. */
} resonant_i32_coeff_t;

typedef struct
{
    resonant_i32_coeff_t coeff; /**< Coefficients used by the ISR calculation. */
    int32_t x1;                 /**< Previous input sample. */
    int32_t x2;                 /**< Input delayed by 2 samples. */
    int32_t y1;                 /**< Previous output sample. */
    int32_t y2;                 /**< Output delayed by 2 samples. */
    int64_t residual_q29;       /**< Bounded fractional recurrence residue retained across samples. */
} resonant_i32_t;

bool resonant_i32_design_coeff(resonant_i32_coeff_t *p_coeff,
                               float gain,
                               uint32_t order,
                               float omega_radps,
                               float ts,
                               float output_code_per_unit,
                               float input_code_per_unit);
bool resonant_i32_init(resonant_i32_t *p_resonant, const resonant_i32_coeff_t *p_coeff);
bool resonant_i32_set_coeff(resonant_i32_t *p_resonant, const resonant_i32_coeff_t *p_coeff);
void resonant_i32_reset(resonant_i32_t *p_resonant);
int32_t resonant_i32_cal(resonant_i32_t *p_resonant, int32_t input);

#endif
