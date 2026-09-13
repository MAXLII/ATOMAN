// SPDX-License-Identifier: MIT
/**
 * @file    pwm.h
 * @brief   NPC modulation-to-gate interface.
 * @details
 *          This file is part of the base digital power framework project.
 *          Translate SVPWM dwell ratios into P and N duty pairs for downstream modulation.
 *          C11 compatible; no dynamic allocation; single simulation instance.
 * @author  Max.Li
 * @date    2026-09-12
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef NPC_PWM_H
#define NPC_PWM_H
#include <stdbool.h>
#include <math.h>
#include "svpwm_3level.h"
#include "my_math.h"

#define NPC_PWM_CONTROL_TS (0.0002f) /* 5 kHz duty update period, s. */
#define NPC_PWM_DEAD_TIME_S (0.000002f) /* Must match the downstream PLECS gate dead time. */
#define NPC_PWM_BALANCE_FILTER_HZ (10.0f)
#define NPC_PWM_BALANCE_PI_ZERO_HZ (0.5f)
#define NPC_PWM_BALANCE_KAW (10.0f)
#define NPC_PWM_BALANCE_CURRENT_MIN (10.0f) /* Current-authority denominator floor, A. */
#define NPC_PWM_BALANCE_SLOPE_FLOOR_RATIO (0.1f) /* Normalize inverse sensitivity near a zero crossing. */
#define NPC_PWM_BALANCE_OFFSET_MAX (150.0f) /* Extra common-mode correction limit, V. */
#define NPC_PWM_COMPENSATION_CURRENT_MIN (200.0f) /* Sum of fundamental current magnitudes, A. */
#define NPC_PWM_COMPENSATION_TRANSITION_A (100.0f) /* Continuous polarity transition around current zero. */

/** @return NPC modulation configuration shared by the platform and offline validation. */
static inline svpwm_3level_cfg_t pwm_make_modulator_cfg(float half_min, float midpoint_kp)
{
    return (svpwm_3level_cfg_t){
        .v_dc_half_min = half_min,
        .midpoint_kp = midpoint_kp,
        .average_balance = true,
        .ts = NPC_PWM_CONTROL_TS,
        .midpoint_filter_hz = NPC_PWM_BALANCE_FILTER_HZ,
        .midpoint_ki = midpoint_kp * M_2PI * NPC_PWM_BALANCE_PI_ZERO_HZ,
        .midpoint_kaw = NPC_PWM_BALANCE_KAW,
        .midpoint_current_min = NPC_PWM_BALANCE_CURRENT_MIN,
        .midpoint_slope_floor_ratio = NPC_PWM_BALANCE_SLOPE_FLOOR_RATIO,
        .midpoint_offset_max = NPC_PWM_BALANCE_OFFSET_MAX};
}

/**
 * @brief Convert ideal P/N dwell to dead-time-corrected gate duty commands.
 * @param p_phase Ideal phase dwell; remains unchanged for voltage reconstruction.
 * @param current Fundamental phase current, positive from bridge to AC side.
 * @param current_magnitude Filtered sum of absolute phase currents, A.
 * @param p_positive Destination for the corrected positive gate duty.
 * @param p_negative Destination for the corrected negative gate duty.
 * @details Light-load compensation is disabled because switching ripple can
 *          reverse the actual commutation current relative to the fundamental.
 *          No complementary gate generation or dead-time insertion occurs here.
 */
static inline void pwm_correct_phase_duty(const svpwm_3level_phase_output_t *p_phase,
    float current, float current_magnitude, float *p_positive, float *p_negative)
{
    float correction = 0.0f;
    *p_positive = p_phase->duty_p;
    *p_negative = p_phase->duty_n;
    if (current_magnitude > NPC_PWM_COMPENSATION_CURRENT_MIN)
    {
        correction = NPC_PWM_DEAD_TIME_S / NPC_PWM_CONTROL_TS *
            fmaxf(-1.0f, fminf(1.0f, current / NPC_PWM_COMPENSATION_TRANSITION_A));
        if (*p_positive > 0.0f)
        {
            *p_positive = fmaxf(0.0f, fminf(1.0f, *p_positive + correction));
        }
        else if (*p_negative > 0.0f)
        {
            *p_negative = fmaxf(0.0f, fminf(1.0f, *p_negative - correction));
        }
    }
}

/** @param v_dc_half_min Minimum valid voltage of each half bus in V.
 *  @param midpoint_kp Midpoint balance gain, A/V; 0 disables balancing.
 *  @return true when the modulator is initialized; outputs remain disabled. */
bool pwm_init(float v_dc_half_min, float midpoint_kp);
/** @param p_input Coherent voltage snapshot captured at the 5 kHz control update.
 *  @return Modulation status; any failure immediately disables all gates. */
SVPWM_3LEVEL_STATUS_E pwm_update(const svpwm_3level_input_t *p_input);
/** @brief Disable all gates and invalidate modulation output; update is needed to restart. */
void pwm_disable(void);
/** @return Latest modulation result; read serially after pwm_update(). */
const svpwm_3level_output_t *pwm_get_output(void);
#endif /* NPC_PWM_H */
