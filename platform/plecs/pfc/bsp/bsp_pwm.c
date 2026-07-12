// SPDX-License-Identifier: MIT
/**
 * @file    bsp_pwm.c
 * @brief   PLECS PFC PWM adapter module.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Drive simulated PFC PWM enable outputs
 *          - Limit and publish bridge PWM duty values to PLECS outputs
 *          - Provide the BSP PWM callbacks bound into the PFC HAL
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-19
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "bsp_pwm.h"

void bsp_pwm_enable(void)
{
    plecs_set_output(PLECS_OUTPUT_PWM_FAST_DUTY, 0.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_SLOW_DUTY, 0.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_FAST_UP_EN, 1.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_FAST_DN_EN, 1.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_SLOW_UP_EN, 1.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_SLOW_DN_EN, 1.0f);
}

void bsp_pwm_disable(void)
{
    plecs_set_output(PLECS_OUTPUT_PWM_FAST_DUTY, 0.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_SLOW_DUTY, 0.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_FAST_UP_EN, 0.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_FAST_DN_EN, 0.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_SLOW_UP_EN, 0.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_SLOW_DN_EN, 0.0f);
}

void bsp_pwm_set_duty(float duty_fast,
                      float duty_slow,
                      uint8_t up_en_fast,
                      uint8_t dn_en_fast,
                      uint8_t up_en_slow,
                      uint8_t dn_en_slow)
{
    UP_DN_LMT(duty_fast, 1.0f, 0.0f);
    UP_DN_LMT(duty_slow, 1.0f, 0.0f);

    plecs_set_output(PLECS_OUTPUT_PWM_FAST_DUTY, duty_fast);
    plecs_set_output(PLECS_OUTPUT_PWM_SLOW_DUTY, duty_slow);
    plecs_set_output(PLECS_OUTPUT_PWM_FAST_UP_EN, up_en_fast ? 1.0f : 0.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_FAST_DN_EN, dn_en_fast ? 1.0f : 0.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_SLOW_UP_EN, up_en_slow ? 1.0f : 0.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_SLOW_DN_EN, dn_en_slow ? 1.0f : 0.0f);
}
