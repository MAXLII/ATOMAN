// SPDX-License-Identifier: MIT
/**
 * @file    bsp_pwm.h
 * @brief   PLECS buck PWM adapter public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Declare the simulated A/B PWM output control functions
 *          - Match the buck HAL PWM setter callback contract
 *          - Keep PLECS output signal writes behind the BSP PWM interface
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-24
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __BSP_PWM_H
#define __BSP_PWM_H

#include <stdint.h>

void bsp_pwm_enable(void);
void bsp_pwm_disable(void);
void bsp_pwm_set_a_cmp(int32_t cmp, uint8_t up_en, uint8_t dn_en);
void bsp_pwm_set_b_cmp(int32_t cmp, uint8_t up_en, uint8_t dn_en);

#endif
