// SPDX-License-Identifier: MIT
/**
 * @file pwm.h
 * @brief Convert CHB bridge voltage commands into unipolar PWM duties.
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

#include <stdint.h>

/**
 * @brief Cache one bridge duty from its final modulation voltage.
 * @param cell Bridge index in the complete CHB frame.
 * @param v_pwm_ref Final voltage after timing prediction and joint limiting, V.
 * @param v_bus_raw Same-period DC bus voltage used for normalization, V.
 * @param deadtime_flag Predicted current direction; BSP owns compensation magnitude.
 */
void chb_pwm_set_cell(uint32_t cell, float v_pwm_ref, float v_bus_raw,
                      chb_pwm_deadtime_flag_t deadtime_flag);

/** @brief Disable all H-bridge PWM channels. */
void chb_pwm_disable(void);

#endif /* CHB_PWM_H */
