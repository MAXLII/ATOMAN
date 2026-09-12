// SPDX-License-Identifier: MIT
/**
 * @file    pwm.c
 * @brief   NPC SVPWM and 2 state-based duties per phase.
 * @details
 *          This file is part of the base digital power framework project.
 *          Use Q1..Q4 from top to bottom: P=1100, O=0110, N=0011.
 *          C11 compatible; no dynamic allocation; single simulation instance.
 * @author  Max.Li
 * @date    2026-09-12
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "pwm.h"
#include <stddef.h>
#include "bsp_pwm.h"
#include "platform.h"

static svpwm_3level_t modulator; /* One NPC bridge controlled by this interface. */

/** @param p_phase Valid SVPWM phase output. @param p_duty Destination for P and N fractions. */
static inline void map_phase(const svpwm_3level_phase_output_t *p_phase, bsp_pwm_phase_duty_t *p_duty)
{
    p_duty->positive_duty = p_phase->duty_p;
    p_duty->negative_duty = p_phase->duty_n;
}

bool pwm_init(float v_dc_half_min)
{
    const svpwm_3level_cfg_t cfg = {.v_dc_half_min = v_dc_half_min}; /* Per-half-bus voltage floor. */
    bsp_pwm_disable();
    return svpwm_3level_init(&modulator, &cfg);
}

void pwm_disable(void)
{
    bsp_pwm_disable();
    svpwm_3level_reset(&modulator);
}

SVPWM_3LEVEL_STATUS_E FUNC_RAM pwm_update(const svpwm_3level_input_t *p_input)
{
    bsp_pwm_phase_duty_t duty[BSP_PWM_PHASE_COUNT] = {0}; /* Complete A/B/C phase-pair frame. */
    SVPWM_3LEVEL_STATUS_E status = SVPWM_3LEVEL_OK; /* Result of the actual library calculation. */
    if (p_input == NULL)
    {
        pwm_disable();
        return SVPWM_3LEVEL_INVALID_ARGUMENT;
    }
    modulator.input = *p_input;
    status = svpwm_3level_cal(&modulator);
    if (status != SVPWM_3LEVEL_OK)
    {
        bsp_pwm_disable();
        return status;
    }
    map_phase(&modulator.output.phase_a, &duty[0]);
    map_phase(&modulator.output.phase_b, &duty[1]);
    map_phase(&modulator.output.phase_c, &duty[2]);
    if (bsp_pwm_set_duty(duty) == false)
    {
        pwm_disable();
        return SVPWM_3LEVEL_INVALID_INPUT;
    }
    return SVPWM_3LEVEL_OK;
}
