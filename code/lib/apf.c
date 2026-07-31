// SPDX-License-Identifier: MIT
/**
 * @file    apf.c
 * @brief   First-order all-pass filter module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Calculate Tustin-discretized all-pass filter coefficients
 *          - Maintain the filter sample history
 *          - Produce a phase-shifted output for each input sample
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
#include "apf.h"

#include <stddef.h>

bool apf_init(apf_t *p_apf, float omega_radps, float ts)
{
    if ((p_apf == NULL) || /* Filter instance must be valid. */
        (ts <= 0.0f))      /* Sample time must be positive. */
    {
        return false;
    }

    p_apf->ts = ts;
    apf_reset(p_apf);
    return apf_update_frequency(p_apf, omega_radps);
}

bool apf_update_frequency(apf_t *p_apf, float omega_radps)
{
    float denominator = 0.0f; /**< Bilinear-transform denominator. */

    if ((p_apf == NULL) ||      /* Filter instance must be valid. */
        (p_apf->ts <= 0.0f) ||  /* Sample time must be configured. */
        (omega_radps <= 0.0f))  /* Angular frequency must be positive. */
    {
        return false;
    }

    denominator = (omega_radps * p_apf->ts) + 2.0f;
    if (denominator == 0.0f)
    {
        return false;
    }

    p_apf->b0 = ((omega_radps * p_apf->ts) - 2.0f) / denominator;
    p_apf->b1 = 1.0f;
    p_apf->a1 = p_apf->b0;
    return true;
}

void apf_reset(apf_t *p_apf)
{
    if (p_apf == NULL)
    {
        return;
    }

    p_apf->x1 = 0.0f;
    p_apf->y1 = 0.0f;
}

float apf_cal(apf_t *p_apf, float input)
{
    float output = 0.0f; /**< Current filter output. */

    if (p_apf == NULL)
    {
        return 0.0f;
    }

    output = (p_apf->b0 * input) +
             (p_apf->b1 * p_apf->x1) -
             (p_apf->a1 * p_apf->y1);
    p_apf->x1 = input;
    p_apf->y1 = output;
    return output;
}
