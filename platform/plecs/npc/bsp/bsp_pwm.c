// SPDX-License-Identifier: MIT
/**
 * @file    bsp_pwm.c
 * @brief   NPC PLECS 6-channel state-based duty output.
 * @details
 *          This file is part of the base digital power framework project.
 *          Publish duty references and a bridge enable; downstream blocks generate switching waveforms.
 *          C11 compatible; no dynamic allocation; single simulation instance.
 * @author  Max.Li
 * @date    2026-09-12
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "bsp_pwm.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include "plecs.h"

_Static_assert(PLECS_OUTPUT_PWM_ENABLE == BSP_PWM_CHANNEL_COUNT, "Duty ports must precede enable");

void bsp_pwm_disable(void)
{
    plecs_set_output(PLECS_OUTPUT_PWM_ENABLE, 0.0f);
    for (uint32_t i = 0u; i < BSP_PWM_CHANNEL_COUNT; ++i) /* Duty channel index. */
    {
        plecs_set_output((PLECS_OUTPUT_E)i, 0.0f);
    }
}

bool bsp_pwm_set_duty(const bsp_pwm_phase_duty_t *p_duty)
{
    if (p_duty == NULL)
    {
        bsp_pwm_disable();
        return false;
    }
    for (uint32_t i = 0u; i < BSP_PWM_PHASE_COUNT; ++i) /* Validate all phase pairs before publishing. */
    {
        if ((isfinite(p_duty[i].positive_duty) == 0) || /* Finite P fraction. */
            (isfinite(p_duty[i].negative_duty) == 0) || /* Finite N fraction. */
            (p_duty[i].positive_duty < 0.0f) || /* P must be non-negative. */
            (p_duty[i].positive_duty > 1.0f) || /* P upper bound. */
            (p_duty[i].negative_duty < 0.0f) || /* N must be non-negative. */
            (p_duty[i].negative_duty > 1.0f) || /* N upper bound. */
            ((p_duty[i].positive_duty + p_duty[i].negative_duty) > 1.0f)) /* O must be non-negative. */
        {
            bsp_pwm_disable();
            return false;
        }
    }
    for (uint32_t i = 0u; i < BSP_PWM_PHASE_COUNT; ++i) /* Publish the coherent 6-channel duty frame. */
    {
        plecs_set_output((PLECS_OUTPUT_E)(2u * i), p_duty[i].positive_duty);
        plecs_set_output((PLECS_OUTPUT_E)(2u * i + 1u), p_duty[i].negative_duty);
    }
    plecs_set_output(PLECS_OUTPUT_PWM_ENABLE, 1.0f);
    return true;
}
