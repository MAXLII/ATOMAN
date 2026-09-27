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

/**
 * @brief Reconstruct and jointly constrain all bridge voltages at one action time.
 * @param p_command Synchronous controller voltage and bus snapshot.
 * @param phase_advance Electrical angle from sampling to the requested action time.
 * @param p_voltage Output bridge voltages, V; sum follows the attainable total request.
 */
static void pwm_project_voltage(const chb_pwm_command_t *p_command, float phase_advance,
                                float p_voltage[CHB_CELL_COUNT])
{
    const float theta = p_command->theta_rad + phase_advance; /* Shared action angle, rad. */
    const float modulation = chb_cfg_get_ctrl_cfg()->modulation_limit; /* Available linear duty range. */
    float harmonic = 0.0f; /* Total harmonic voltage at the action time, V. */
    float requested = 0.0f; /* Unconstrained sum voltage, V. */
    float applied = 0.0f; /* Sum after individual clipping, V. */
    float capacity = 0.0f; /* Sum of bridge voltage bounds, V. */
    float headroom = 0.0f; /* Available voltage in the correction direction, V. */
    for (uint32_t h = 0u; h < CHB_HARMONIC_COUNT; ++h)
    {
        const float angle = (float)(2u * h + 3u) * phase_advance; /* Harmonic phase advance. */
        harmonic += p_command->harmonic_alpha_v[h] * cosf(angle)
                    - p_command->harmonic_beta_v[h] * sinf(angle);
    }
    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        const float bound = modulation * p_command->bus_v[cell]; /* Bridge voltage bound, V. */
        const float voltage = p_command->cell_d_v[cell] * cosf(theta)
                              - p_command->cell_q_v[cell] * sinf(theta)
                              + p_command->harmonic_share[cell] * harmonic; /* Requested voltage, V. */
        requested += voltage;
        capacity += bound;
        p_voltage[cell] = fminf(bound, fmaxf(-bound, voltage));
        applied += p_voltage[cell];
    }
    const float residual = fminf(capacity, fmaxf(-capacity, requested)) - applied; /* Sum error, V. */
    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        headroom += modulation * p_command->bus_v[cell]
                    - ((residual >= 0.0f) ? p_voltage[cell] : -p_voltage[cell]);
    }
    if (headroom > 0.0f)
    {
        for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
        {
            const float room = modulation * p_command->bus_v[cell]
                               - ((residual >= 0.0f) ? p_voltage[cell] : -p_voltage[cell]); /* Directional room, V. */
            p_voltage[cell] += residual * room / headroom;
        }
    }
}

void chb_pwm_update(const chb_pwm_command_t *p_command, float p_v_pwm_v[CHB_CELL_COUNT])
{
    float duty[CHB_CELL_COUNT] = {0.0f}; /* Complete three-cell duty frame. */
    float dead_time_ratio = 2.0f * CHB_PWM_DEAD_TIME_S / chb_cfg_get_ctrl_cfg()->ts;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        /* 各级载波及 ZOH 相移为 0、1/6、1/3 周期；BSP 延迟仍实际存在。 */
        float delay_periods = CHB_PWM_BASE_DELAY_PERIODS
                              + (float)cell / (2.0f * (float)CHB_CELL_COUNT);
        float phase_advance = M_2PI * p_command->grid_hz
                              * chb_cfg_get_ctrl_cfg()->ts * delay_periods;
        float projected_voltage[CHB_CELL_COUNT] = {0.0f}; /* Three-bridge allocation at this carrier's action time. */
        float predicted_current = p_command->i_comp_ref_a * cosf(phase_advance)
                                  - p_command->i_comp_beta_ref_a * sinf(phase_advance);
        float current_direction = predicted_current
                                  / hypotf(predicted_current, CHB_PWM_CURRENT_SMOOTH_A);
        float modulation; /* 时序预测后的归一化电压及死区补偿。 */

        pwm_project_voltage(p_command, phase_advance, projected_voltage);
        p_v_pwm_v[cell] = projected_voltage[cell];
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
