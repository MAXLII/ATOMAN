// SPDX-License-Identifier: MIT
/**
 * @file pwm.h
 * @brief Convert CHB bridge voltage commands into three unipolar PWM duties.
 * @details Each duty controls one H-bridge leg; the other leg uses 1 - duty
 *          in the platform modulator. The BSP owns the common bridge enable.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef CHB_PWM_H
#define CHB_PWM_H

#include "chb_hal.h"

#define CHB_PWM_DEAD_TIME_S 2.0e-6f /* 两桥臂实际互补开关的死区时间，s。 */
#define CHB_PWM_CURRENT_SMOOTH_A 1.0f /* 电流过零处的死区补偿平滑尺度，A。 */
#define CHB_PWM_BASE_DELAY_PERIODS 1.5f /* BSP 一拍加单次更新三角 PWM 的半周期。 */
#define CHB_PWM_COMP_GAIN 1.0f /* 死区平均压降补偿占理想值的比例。 */

/** @brief Predict each bridge voltage at its PWM action time, normalize and enable.
 * @param p_command 本拍物理 dq 电压、电流参考及电网相角。
 * @param p_v_pwm_v 输出各桥时序补偿后的调制电压，V，尚未叠加死区补偿。
 */
void chb_pwm_update(const chb_pwm_command_t *p_command, float p_v_pwm_v[CHB_CELL_COUNT]);

/** @brief Disable all three H-bridge PWM channels. */
void chb_pwm_disable(void);

#endif /* CHB_PWM_H */
