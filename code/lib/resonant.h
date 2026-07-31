// SPDX-License-Identifier: MIT
/**
 * @file    resonant.h
 * @brief   Ideal resonant controller public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define a discrete ideal resonant controller state
 *          - Configure resonant order, gain, frequency, and sample time
 *          - Calculate narrow-band harmonic compensation
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
#ifndef RESONANT_H
#define RESONANT_H

#include <stdbool.h>

typedef struct
{
    float gain;  /**< Resonant gain. */
    float order; /**< Harmonic order relative to the fundamental. */
    float ts;    /**< Sample time in seconds. */
    float b0;    /**< Current-input numerator coefficient. */
    float b2;    /**< Two-sample-delayed numerator coefficient. */
    float a1;    /**< One-sample-delayed denominator coefficient. */
    float a2;    /**< Two-sample-delayed denominator coefficient. */
    float x1;    /**< Previous input sample. */
    float x2;    /**< Input sample delayed by two steps. */
    float y1;    /**< Previous output sample. */
    float y2;    /**< Output sample delayed by two steps. */
} resonant_t;

/**
 * @brief Initialize an ideal resonant controller.
 * @param p_resonant Controller instance.
 * @param gain Resonant gain.
 * @param order Harmonic order relative to the fundamental.
 * @param omega_radps Fundamental angular frequency in radians per second.
 * @param ts Sample time in seconds.
 * @return true when the parameters are valid; otherwise false.
 */
bool resonant_init(resonant_t *p_resonant,
                   float gain,
                   float order,
                   float omega_radps,
                   float ts);

/**
 * @brief Update the fundamental angular frequency without clearing state.
 * @param p_resonant Controller instance.
 * @param omega_radps Fundamental angular frequency in radians per second.
 * @return true when the parameters are valid; otherwise false.
 */
bool resonant_update_frequency(resonant_t *p_resonant, float omega_radps);

/**
 * @brief Clear the controller input and output history.
 * @param p_resonant Controller instance.
 */
void resonant_reset(resonant_t *p_resonant);

/**
 * @brief Calculate one ideal resonant controller sample.
 * @param p_resonant Controller instance.
 * @param input Current input sample.
 * @return Current output sample, or zero for a null instance.
 */
float resonant_cal(resonant_t *p_resonant, float input);

#endif // RESONANT_H
