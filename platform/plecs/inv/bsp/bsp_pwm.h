// SPDX-License-Identifier: MIT
/**
 * @file    bsp_pwm.h
 * @brief   PLECS inverter PWM adapter interface module.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Declare simulated PWM enable and disable functions
 *          - Declare bridge duty update function used by the inverter HAL
 *          - Keep PLECS output updates behind the BSP PWM interface
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-19
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef PLECS_INV_BSP_PWM_H
#define PLECS_INV_BSP_PWM_H

#include "my_math.h"
#include "plecs.h"

void bsp_pwm_enable(void);
void bsp_pwm_disable(void);
void bsp_pwm_set_duty(float duty_fast,
                      float duty_slow,
                      uint8_t up_en_fast,
                      uint8_t dn_en_fast,
                      uint8_t up_en_slow,
                      uint8_t dn_en_slow);

#endif
