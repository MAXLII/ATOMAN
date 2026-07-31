// SPDX-License-Identifier: MIT
/**
 * @file    resonant.c
 * @brief   Ideal resonant controller module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Calculate Tustin-discretized ideal resonant coefficients
 *          - Maintain controller input and output history
 *          - Produce harmonic compensation for each input sample
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
#include "resonant.h"

#include <stddef.h>

bool resonant_init(resonant_t *p_resonant,
                   float gain,
                   float order,
                   float omega_radps,
                   float ts)
{
    if ((p_resonant == NULL) || /* Controller instance must be valid. */
        (gain < 0.0f) ||        /* Gain cannot be negative. */
        (order <= 0.0f) ||      /* Harmonic order must be positive. */
        (ts <= 0.0f))           /* Sample time must be positive. */
    {
        return false;
    }

    p_resonant->gain = gain;
    p_resonant->order = order;
    p_resonant->ts = ts;
    resonant_reset(p_resonant);
    return resonant_update_frequency(p_resonant, omega_radps);
}

bool resonant_update_frequency(resonant_t *p_resonant, float omega_radps)
{
    float omega_n = 0.0f; /**< Target resonant angular frequency. */
    float d0 = 0.0f;      /**< Bilinear-transform denominator. */

    if ((p_resonant == NULL) ||       /* Controller instance must be valid. */
        (p_resonant->gain < 0.0f) ||  /* Gain cannot be negative. */
        (p_resonant->order <= 0.0f) ||/* Harmonic order must be positive. */
        (p_resonant->ts <= 0.0f) ||   /* Sample time must be configured. */
        (omega_radps <= 0.0f))        /* Fundamental frequency must be positive. */
    {
        return false;
    }

    omega_n = p_resonant->order * omega_radps;
    d0 = 4.0f + (p_resonant->ts * p_resonant->ts * omega_n * omega_n);
    if (d0 == 0.0f)
    {
        return false;
    }

    p_resonant->b0 = 2.0f * p_resonant->gain * p_resonant->ts / d0;
    p_resonant->b2 = -p_resonant->b0;
    p_resonant->a1 = ((2.0f * p_resonant->ts * p_resonant->ts * omega_n * omega_n) - 8.0f) / d0;
    p_resonant->a2 = 1.0f;
    return true;
}

void resonant_reset(resonant_t *p_resonant)
{
    if (p_resonant == NULL)
    {
        return;
    }

    p_resonant->x1 = 0.0f;
    p_resonant->x2 = 0.0f;
    p_resonant->y1 = 0.0f;
    p_resonant->y2 = 0.0f;
}

float resonant_cal(resonant_t *p_resonant, float input)
{
    float output = 0.0f; /**< Current controller output. */

    if (p_resonant == NULL)
    {
        return 0.0f;
    }

    output = (p_resonant->b0 * input) +
             (p_resonant->b2 * p_resonant->x2) -
             (p_resonant->a1 * p_resonant->y1) -
             (p_resonant->a2 * p_resonant->y2);
    p_resonant->x2 = p_resonant->x1;
    p_resonant->x1 = input;
    p_resonant->y2 = p_resonant->y1;
    p_resonant->y1 = output;
    return output;
}
