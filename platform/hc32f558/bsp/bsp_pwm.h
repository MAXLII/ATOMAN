// SPDX-License-Identifier: MIT
/**
 * @file    bsp_pwm.h
 * @brief   HC32F558 HRPWM BSP interface.
 * @details
 *          This file is part of the HC32F558 AC project.
 *
 *          Module responsibilities:
 *          - Provide HRPWM output enable and disable controls
 *          - Provide fast and slow bridge duty update API
 *          - Provide a PWM interrupt hook for section interrupt scheduling
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Duty update path is suitable for interrupt context
 *          - Hardware access is abstracted through HC32 LL HRPWM/GPIO APIs
 *
 * @author  Max.Li
 * @date    2026-06-06
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef BSP_PWM_H
#define BSP_PWM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void bsp_pwm_enable(void);
void bsp_pwm_disable(void);
void bsp_pwm_set_duty(float duty_fast,
                      float duty_slow,
                      uint8_t up_en_fast,
                      uint8_t dn_en_fast,
                      uint8_t up_en_slow,
                      uint8_t dn_en_slow);

#ifdef __cplusplus
}
#endif

#endif /* BSP_PWM_H */
