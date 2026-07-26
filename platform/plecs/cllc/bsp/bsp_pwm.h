// SPDX-License-Identifier: MIT
/**
 * @file    bsp_pwm.h
 * @brief   PLECS CLLC PWM adapter public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Publish independent primary and secondary bridge commands to PLECS
 *          - Route forward modulation to the primary bridge and reverse modulation to the secondary bridge
 *          - Provide an immediate shutdown that clears both bridge outputs
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Functions are safe for the simulation control-interrupt path
 *          - Modulation trajectory calculation remains in code/interface/cllc
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
#ifndef __PLECS_CLLC_BSP_PWM_H
#define __PLECS_CLLC_BSP_PWM_H

#include "cllc_cfg.h"

/** @brief Enable the selected bridge direction. @param direction Forward or reverse. */
void bsp_pwm_enable(CLLC_DIRECTION_E direction);

/**
 * @brief Publish one fully decoded modulation command.
 * @param direction Forward or reverse power flow.
 * @param duty Equivalent phase-shift duty in the 0...0.5 range.
 * @param frequency_hz Switching frequency in hertz.
 */
void bsp_pwm_set_modulation(CLLC_DIRECTION_E direction,
                            float duty,
                            float frequency_hz);

/** @brief Disable all simulated CLLC PWM outputs. */
void bsp_pwm_disable(void);

#endif /* __PLECS_CLLC_BSP_PWM_H */
