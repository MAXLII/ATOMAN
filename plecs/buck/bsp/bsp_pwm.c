// SPDX-License-Identifier: MIT
/**
 * @file    bsp_pwm.c
 * @brief   PLECS buck PWM adapter module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Convert buck compare commands into normalized PLECS duty outputs
 *          - Publish A/B complementary enable states to the simulation model
 *          - Provide the same PWM control surface used by the buck HAL
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-24
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "bsp_pwm.h"
#include "buck_cfg.h"
#include "plecs.h"

static int32_t bsp_pwm_limit_cmp(int32_t cmp)
{
    if (cmp > BUCK_CTRL_CMP_MAX)
    {
        return BUCK_CTRL_CMP_MAX;
    }

    if (cmp < BUCK_CTRL_CMP_MIN)
    {
        return BUCK_CTRL_CMP_MIN;
    }

    return cmp;
}

static void bsp_pwm_set_channel(PLECS_OUTPUT_E duty_ch,
                                PLECS_OUTPUT_E cmp_ch,
                                PLECS_OUTPUT_E up_en_ch,
                                PLECS_OUTPUT_E dn_en_ch,
                                int32_t cmp,
                                uint8_t up_en,
                                uint8_t dn_en)
{
    int32_t cmp_limited = 0;
    float duty = 0.0f;

    cmp_limited = bsp_pwm_limit_cmp(cmp);
    duty = (float)cmp_limited / (float)BUCK_CTRL_CMP_MAX;

    plecs_set_output(duty_ch, duty);
    plecs_set_output(cmp_ch, (float)cmp_limited);
    plecs_set_output(up_en_ch, (up_en != 0U) ? 1.0f : 0.0f);
    plecs_set_output(dn_en_ch, (dn_en != 0U) ? 1.0f : 0.0f);
}

void bsp_pwm_enable(void)
{
    bsp_pwm_set_a_cmp(0, 1U, 1U);
    bsp_pwm_set_b_cmp(0, 1U, 1U);
}

void bsp_pwm_disable(void)
{
    bsp_pwm_set_a_cmp(0, 0U, 0U);
    bsp_pwm_set_b_cmp(0, 0U, 0U);
}

void bsp_pwm_set_a_cmp(int32_t cmp, uint8_t up_en, uint8_t dn_en)
{
    bsp_pwm_set_channel(PLECS_OUTPUT_PWM_A_DUTY,
                        PLECS_OUTPUT_PWM_A_CMP,
                        PLECS_OUTPUT_PWM_A_UP_EN,
                        PLECS_OUTPUT_PWM_A_DN_EN,
                        cmp,
                        up_en,
                        dn_en);
}

void bsp_pwm_set_b_cmp(int32_t cmp, uint8_t up_en, uint8_t dn_en)
{
    bsp_pwm_set_channel(PLECS_OUTPUT_PWM_B_DUTY,
                        PLECS_OUTPUT_PWM_B_CMP,
                        PLECS_OUTPUT_PWM_B_UP_EN,
                        PLECS_OUTPUT_PWM_B_DN_EN,
                        cmp,
                        up_en,
                        dn_en);
}
