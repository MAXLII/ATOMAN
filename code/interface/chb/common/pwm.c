// SPDX-License-Identifier: MIT
/**
 * @file pwm.c
 * @brief CHB voltage normalization and 0..1 duty mapping.
 * @details The RUN-stage protection guarantees a valid positive DC bus before
 *          this callback. The downstream PLECS model applies carrier phase
 *          shifts, complementary gates and dead time.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#include "pwm.h"
#include "bsp_pwm.h"
#include "my_math.h"

#include <math.h>

_Static_assert(CHB_CELL_COUNT == BSP_PWM_CELL_COUNT, "CHB and BSP cell counts must match");

void chb_pwm_update(const chb_pwm_command_t *p_command, float p_v_pwm_v[CHB_CELL_COUNT])
{
    float duty[CHB_CELL_COUNT] = {0.0f}; /* Complete three-cell duty frame. */
    float dead_time_ratio = 2.0f * CHB_PWM_DEAD_TIME_S / chb_cfg_get_ctrl_cfg()->ts;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        /* 各级载波及 ZOH 相移为 0、1/6、1/3 周期；BSP 延迟仍实际存在。 */
        float delay_periods = CHB_PWM_BASE_DELAY_PERIODS
                              + (float)cell / (2.0f * (float)CHB_CELL_COUNT);
        float phase_advance = M_2PI * chb_cfg_get_ctrl_cfg()->grid_hz
                              * chb_cfg_get_ctrl_cfg()->ts * delay_periods;
        float theta_apply = p_command->theta_rad + phase_advance;
        float predicted_current = p_command->i_comp_ref_a * cosf(phase_advance)
                                  - p_command->i_comp_beta_ref_a * sinf(phase_advance);
        float current_direction = predicted_current
                                  / hypotf(predicted_current, CHB_PWM_CURRENT_SMOOTH_A);
        float modulation; /* 时序预测后的归一化电压及死区补偿。 */

        p_v_pwm_v[cell] = p_command->cell_d_v[cell] * cosf(theta_apply)
                         - p_command->cell_q_v[cell] * sinf(theta_apply);
        modulation = p_v_pwm_v[cell] / p_command->bus_v[cell]
                           - CHB_PWM_COMP_GAIN * dead_time_ratio * current_direction;

        if (modulation > 1.0f)
        {
            modulation = 1.0f;
        }
        else if (modulation < -1.0f)
        {
            modulation = -1.0f;
        }
        duty[cell] = 0.5f * (modulation + 1.0f);
    }

    bsp_pwm_set(duty, 1u);
}

void chb_pwm_disable(void)
{
    const float duty[BSP_PWM_CELL_COUNT] = {0.0f};

    bsp_pwm_set(duty, 0u);
}
