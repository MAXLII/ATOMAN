// SPDX-License-Identifier: MIT
/**
 * @file pwm.c
 * @brief NPC SVPWM and 2 state-based duties per phase.
 * @details
 *          This file is part of the base digital power framework project.
 *          Use Q1..Q4 from top to bottom: P=1100, O=0110, N=0011.
 *          C11 compatible; no dynamic allocation; single simulation instance.
 * @author Max.Li
 * @date 2026-09-12
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li.
 *          All rights reserved.
 *          This file is licensed under the MIT License.
 *          See the LICENSE file in the project root for full license text.
 */

#include "pwm.h"
#include <stddef.h>
#include "bsp_pwm.h"
#include "platform.h"

static svpwm_3level_t modulator; /* One NPC bridge controlled by this interface. */

/** @param p_phase Valid SVPWM phase output. @param p_duty Destination for P and N fractions. */
static inline void map_phase(const svpwm_3level_phase_output_t *p_phase,
                             float current, bsp_pwm_phase_duty_t *p_duty)
{
    pwm_correct_phase_duty(p_phase,
                           current,
                           modulator.inter.current_filtered,
                           &p_duty->positive_duty,
                           &p_duty->negative_duty);
}

void pwm_init(float v_dc_half_min, float midpoint_kp)
{
    const svpwm_3level_cfg_t cfg = pwm_make_modulator_cfg(v_dc_half_min, midpoint_kp);
    bsp_pwm_disable();
    svpwm_3level_init(&modulator, &cfg);
}

void pwm_disable(void)
{
    bsp_pwm_disable();
    svpwm_3level_reset(&modulator);
}

const svpwm_3level_output_t *pwm_get_output(void)
{
    return &modulator.output;
}

void FUNC_RAM pwm_update(const svpwm_3level_input_t *p_input)
{
    bsp_pwm_phase_duty_t duty[BSP_PWM_PHASE_COUNT] = {0}; /* Complete A/B/C phase-pair frame. */
    modulator.input                                = *p_input;
    svpwm_3level_cal(&modulator);
    map_phase(&modulator.output.phase_a, p_input->i_a, &duty[0]);
    map_phase(&modulator.output.phase_b, p_input->i_b, &duty[1]);
    map_phase(&modulator.output.phase_c, p_input->i_c, &duty[2]);
    (void)bsp_pwm_set_duty(duty); /* BSP owns gate-output validation and immediate hardware disable. */
}
