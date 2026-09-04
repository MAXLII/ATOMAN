// SPDX-License-Identifier: MIT
/**
 * @file    pwm.c
 * @brief   Bidirectional CLLC normalized-modulation implementation.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Map forward u through resonant-to-2fr PSM/PFM operation
 *          - Map reverse u through fr-to-32kHz PSM/PFM operation
 *          - Keep both modulation segments continuous at their FHA-derived boundary
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Controller output is clamped before modulation calculation
 *          - Direction validation prevents an undefined bridge selection
 *
 * @author  Max.Li
 * @date    2026-07-26
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "pwm.h"

#include "bsp_pwm.h"
#include "cllc_hw_param.h"

/** Clamp one normalized command before using it in a frequency expression. */
static float clamp_normalized(float value)
{
    if (value > 1.0f)
    {
        return 1.0f;
    }
    if (value < 0.0f)
    {
        return 0.0f;
    }
    return value;
}

/** Calculate the continuous forward PSM/PFM command. */
static void calculate_forward(float command, float *p_duty, float *p_frequency_hz)
{
    const float transition = CLLC_HW_FORWARD_PSM_PFM_TRANSITION_U; /* FHA continuity boundary. */

    if (command <= transition)
    {
        *p_frequency_hz = CLLC_HW_FORWARD_MAX_FREQ_HZ;
        *p_duty = CLLC_HW_MAX_PHASE_SHIFT_DUTY * command / transition;
    }
    else
    {
        float ratio = (command - transition) / (1.0f - transition); /* PFM progression to resonance. */

        *p_frequency_hz = CLLC_HW_FORWARD_MAX_FREQ_HZ -
                          ((CLLC_HW_FORWARD_MAX_FREQ_HZ - CLLC_HW_PRIMARY_RESONANT_FREQ_HZ) * ratio);
        *p_duty = CLLC_HW_MAX_PHASE_SHIFT_DUTY;
    }
}

/** Calculate the continuous reverse PSM/PFM command. */
static void calculate_reverse(float command, float *p_duty, float *p_frequency_hz)
{
    const float transition = CLLC_HW_REVERSE_PSM_PFM_TRANSITION_U; /* FHA continuity boundary. */

    if (command <= transition)
    {
        *p_frequency_hz = CLLC_HW_PRIMARY_RESONANT_FREQ_HZ;
        *p_duty = CLLC_HW_MAX_PHASE_SHIFT_DUTY * command / transition;
    }
    else
    {
        float ratio = (command - transition) / (1.0f - transition); /* PFM progression below resonance. */

        *p_frequency_hz = CLLC_HW_PRIMARY_RESONANT_FREQ_HZ -
                          ((CLLC_HW_PRIMARY_RESONANT_FREQ_HZ - CLLC_HW_REVERSE_MIN_FREQ_HZ) * ratio);
        *p_duty = CLLC_HW_MAX_PHASE_SHIFT_DUTY;
    }
}

void cllc_pwm_enable_direction(CLLC_DIRECTION_E direction)
{
    if ((direction < CLLC_DIRECTION_FORWARD) ||
        (direction >= CLLC_DIRECTION_MAX)) /* Undefined direction must never select a bridge set. */
    {
        bsp_pwm_disable();
        return;
    }
    bsp_pwm_enable(direction);
}

void cllc_pwm_set_normalized(CLLC_DIRECTION_E direction, float normalized_command)
{
    float command = clamp_normalized(normalized_command); /* Safe normalized command. */
    float duty = 0.0f;                                   /* Equivalent phase-shift duty, 0...0.5. */
    float frequency_hz = 0.0f;                           /* Direction-specific switching frequency. */

    if (direction == CLLC_DIRECTION_FORWARD)
    {
        calculate_forward(command, &duty, &frequency_hz);
    }
    else if (direction == CLLC_DIRECTION_REVERSE)
    {
        calculate_reverse(command, &duty, &frequency_hz);
    }
    else
    {
        bsp_pwm_disable();
        return;
    }

    bsp_pwm_set_modulation(direction, duty, frequency_hz);
}

void cllc_pwm_disable(void)
{
    bsp_pwm_disable();
}
