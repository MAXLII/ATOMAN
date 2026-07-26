// SPDX-License-Identifier: MIT
/**
 * @file    bsp_pwm.c
 * @brief   PLECS CLLC PWM adapter implementation.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Write independent primary and secondary PWM commands into the PLECS output vector
 *          - Bound duty and switching frequency at the final BSP boundary
 *          - Keep the inactive bridge disabled and clear both bridges during shutdown
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Final clamps protect the model from invalid interface callers
 *          - No MCU timer or register access is present
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
#include "bsp_pwm.h"

#include "cllc_hw_param.h"
#include "plecs.h"

/** Clamp a floating-point value to an inclusive range. */
static float clamp_float(float value, float lower, float upper)
{
    if (value > upper)
    {
        return upper;
    }
    if (value < lower)
    {
        return lower;
    }
    return value;
}

/** Return the lowest legal frequency for the selected direction. */
static float minimum_frequency(CLLC_DIRECTION_E direction)
{
    if (direction == CLLC_DIRECTION_FORWARD)
    {
        return CLLC_HW_PRIMARY_RESONANT_FREQ_HZ;
    }
    return CLLC_HW_REVERSE_MIN_FREQ_HZ;
}

/** Return the highest legal frequency for the selected direction. */
static float maximum_frequency(CLLC_DIRECTION_E direction)
{
    if (direction == CLLC_DIRECTION_FORWARD)
    {
        return CLLC_HW_FORWARD_MAX_FREQ_HZ;
    }
    return CLLC_HW_PRIMARY_RESONANT_FREQ_HZ;
}

/** Publish one bounded primary-side bridge command. */
static void set_primary_bridge(float enable, float duty, float frequency_hz)
{
    plecs_set_output(PLECS_OUTPUT_PRI_PWM_ENABLE, enable);
    plecs_set_output(PLECS_OUTPUT_PRI_PWM_DUTY, duty);
    plecs_set_output(PLECS_OUTPUT_PRI_PWM_FREQUENCY_HZ, frequency_hz);
}

/** Publish one bounded secondary-side bridge command. */
static void set_secondary_bridge(float enable, float duty, float frequency_hz)
{
    plecs_set_output(PLECS_OUTPUT_SEC_PWM_ENABLE, enable);
    plecs_set_output(PLECS_OUTPUT_SEC_PWM_DUTY, duty);
    plecs_set_output(PLECS_OUTPUT_SEC_PWM_FREQUENCY_HZ, frequency_hz);
}

/** Route one modulation tuple to the power-input bridge for the selected direction. */
static void set_active_bridge(CLLC_DIRECTION_E direction, float duty, float frequency_hz)
{
    if (direction == CLLC_DIRECTION_FORWARD)
    {
        set_primary_bridge(1.0f, duty, frequency_hz);
        set_secondary_bridge(0.0f, 0.0f, 0.0f);
    }
    else
    {
        set_primary_bridge(0.0f, 0.0f, 0.0f);
        set_secondary_bridge(1.0f, duty, frequency_hz);
    }
}

void bsp_pwm_enable(CLLC_DIRECTION_E direction)
{
    float initial_frequency_hz = 0.0f; /* Zero-power PSM frequency. */

    if ((direction < CLLC_DIRECTION_FORWARD) ||
        (direction >= CLLC_DIRECTION_MAX)) /* Never publish an undefined bridge direction. */
    {
        bsp_pwm_disable();
        return;
    }
    initial_frequency_hz = maximum_frequency(direction);
    set_active_bridge(direction, 0.0f, initial_frequency_hz);
}

void bsp_pwm_set_modulation(CLLC_DIRECTION_E direction,
                            float duty,
                            float frequency_hz)
{
    float bounded_duty = clamp_float(duty, 0.0f, CLLC_HW_MAX_PHASE_SHIFT_DUTY); /* Safe duty command. */
    float bounded_frequency_hz = 0.0f; /* Direction-specific safe switching frequency. */

    if ((direction < CLLC_DIRECTION_FORWARD) ||
        (direction >= CLLC_DIRECTION_MAX)) /* Reject calls that bypass interface validation. */
    {
        bsp_pwm_disable();
        return;
    }
    bounded_frequency_hz = clamp_float(
        frequency_hz,
        minimum_frequency(direction),
        maximum_frequency(direction));
    set_active_bridge(direction, bounded_duty, bounded_frequency_hz);
}

void bsp_pwm_disable(void)
{
    set_primary_bridge(0.0f, 0.0f, 0.0f);
    set_secondary_bridge(0.0f, 0.0f, 0.0f);
}
