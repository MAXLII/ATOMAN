// SPDX-License-Identifier: MIT
/**
 * @file    bsp_pwm.c
 * @brief   PLECS LLC PWM adapter module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Clamp LLC controller output into a normalized PLECS duty command
 *          - Publish PWM enable state to the simulation model
 *          - Provide the same PWM control surface used by the LLC HAL
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-10
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "bsp_pwm.h"
#include "plecs.h"

#define PLECS_LLC_PWM_MIN_FREQ_HZ (70.0e3f)
#define PLECS_LLC_PWM_MAX_FREQ_HZ (150.0e3f)
#define PLECS_LLC_PWM_DEADTIME_S (200.0e-9f)
#define PLECS_LLC_PWM_DUTY_MODE_END (0.2f)

static float bsp_pwm_limit_mod(float mod)
{
    if (mod > 1.0f)
    {
        return 1.0f;
    }

    if (mod < 0.0f)
    {
        return 0.0f;
    }

    return mod;
}

static float bsp_pwm_calc_duty_max(float freq_hz)
{
    float duty_max = 0.5f - (PLECS_LLC_PWM_DEADTIME_S * freq_hz);

    if (duty_max < 0.0f)
    {
        return 0.0f;
    }

    if (duty_max > 0.5f)
    {
        return 0.5f;
    }

    return duty_max;
}

static void bsp_pwm_calc_modulation(float mod, float *p_duty, float *p_freq_hz)
{
    float freq_hz = PLECS_LLC_PWM_MAX_FREQ_HZ;
    float duty = 0.0f;
    float duty_max = 0.0f;

    mod = bsp_pwm_limit_mod(mod);

    if (mod <= PLECS_LLC_PWM_DUTY_MODE_END)
    {
        freq_hz = PLECS_LLC_PWM_MAX_FREQ_HZ;
        duty_max = bsp_pwm_calc_duty_max(freq_hz);
        duty = (mod / PLECS_LLC_PWM_DUTY_MODE_END) * duty_max;
    }
    else
    {
        float freq_ratio = (mod - PLECS_LLC_PWM_DUTY_MODE_END) /
                           (1.0f - PLECS_LLC_PWM_DUTY_MODE_END);

        freq_hz = PLECS_LLC_PWM_MAX_FREQ_HZ -
                  ((PLECS_LLC_PWM_MAX_FREQ_HZ - PLECS_LLC_PWM_MIN_FREQ_HZ) * freq_ratio);
        duty = bsp_pwm_calc_duty_max(freq_hz);
    }

    *p_duty = duty;
    *p_freq_hz = freq_hz;
}

void bsp_pwm_enable(void)
{
    plecs_set_output(PLECS_OUTPUT_PWM_EN, 1.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_FREQ, PLECS_LLC_PWM_MAX_FREQ_HZ);
}

void bsp_pwm_disable(void)
{
    plecs_set_output(PLECS_OUTPUT_PWM_EN, 0.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_DUTY, 0.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_FREQ, 0.0f);
}

void bsp_pwm_set_duty(float duty)
{
    float pwm_duty = 0.0f;
    float pwm_freq_hz = 0.0f;

    bsp_pwm_calc_modulation(duty, &pwm_duty, &pwm_freq_hz);

    plecs_set_output(PLECS_OUTPUT_PWM_EN, 1.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_DUTY, pwm_duty);
    plecs_set_output(PLECS_OUTPUT_PWM_FREQ, pwm_freq_hz);
}
