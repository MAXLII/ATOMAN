// SPDX-License-Identifier: MIT
/**
 * @file    apf.h
 * @brief   First-order all-pass filter public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define a discrete first-order all-pass filter state
 *          - Configure the filter from angular frequency and sample time
 *          - Generate a phase-shifted signal for orthogonal signal construction
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Calculation is suitable for ISR use after initialization
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
#ifndef APF_H
#define APF_H

#include <stdbool.h>

typedef struct
{
    float ts; /**< Sample time in seconds. */
    float b0; /**< Current-input numerator coefficient. */
    float b1; /**< Previous-input numerator coefficient. */
    float a1; /**< Previous-output denominator coefficient. */
    float x1; /**< Previous input sample. */
    float y1; /**< Previous output sample. */
} apf_t;

/**
 * @brief Initialize a first-order all-pass filter.
 * @param p_apf Filter instance.
 * @param omega_radps Angular frequency in radians per second.
 * @param ts Sample time in seconds.
 * @return true when the parameters are valid; otherwise false.
 */
bool apf_init(apf_t *p_apf, float omega_radps, float ts);

/**
 * @brief Update the filter angular frequency without clearing its state.
 * @param p_apf Filter instance.
 * @param omega_radps Angular frequency in radians per second.
 * @return true when the parameters are valid; otherwise false.
 */
bool apf_update_frequency(apf_t *p_apf, float omega_radps);

/**
 * @brief Clear the filter input and output history.
 * @param p_apf Filter instance.
 */
void apf_reset(apf_t *p_apf);

/**
 * @brief Calculate one all-pass filter sample.
 * @param p_apf Filter instance.
 * @param input Current input sample.
 * @return Current output sample, or zero for a null instance.
 */
float apf_cal(apf_t *p_apf, float input);

#endif // APF_H
