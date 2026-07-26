// SPDX-License-Identifier: MIT
/**
 * @file    pwm.h
 * @brief   Bidirectional CLLC normalized-modulation interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Convert normalized controller output into continuous PSM/PFM commands
 *          - Apply direction-specific forward and reverse frequency trajectories
 *          - Forward bridge enable, modulation, and shutdown to the active BSP
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Functions are allocation-free and suitable for control-interrupt use
 *          - Platform-specific timer updates remain in bsp_pwm
 *
 * @author  Max.Li
 * @date    2026-07-26
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __CLLC_PWM_H
#define __CLLC_PWM_H

#include "cllc_cfg.h"

/** @brief Enable the bridge set for one direction. @param direction Forward or reverse flow. */
void cllc_pwm_enable_direction(CLLC_DIRECTION_E direction);

/**
 * @brief Apply one normalized hybrid-modulation command.
 * @param direction Direction latched by the CLLC FSM.
 * @param normalized_command Controller output in the inclusive 0...1 range.
 */
void cllc_pwm_set_normalized(CLLC_DIRECTION_E direction, float normalized_command);

/** @brief Disable both bridge sets immediately. */
void cllc_pwm_disable(void);

#endif /* __CLLC_PWM_H */
