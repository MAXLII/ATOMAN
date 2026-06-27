// SPDX-License-Identifier: MIT
/**
 * @file    pfc_hal.h
 * @brief   PFC int32 HAL public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Declare PFC int32 HAL feedback and PWM callback bindings
 *          - Expose binding lock, unlock, and readiness checks for platform integration
 *          - Keep ISR control inputs and PWM outputs in integer ADC / compare domains
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-27
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __PFC_I32_HAL_H
#define __PFC_I32_HAL_H

#include <stdint.h>

typedef void (*pfc_i32_pwm_setter_t)(int32_t cmp,
                                     uint8_t up_en_fast,
                                     uint8_t dn_en_fast,
                                     uint8_t up_en_slow,
                                     uint8_t dn_en_slow);

typedef struct
{
    int32_t *p_v_g;
    int32_t *p_v_cap;
    int32_t *p_i_l;
    int32_t *p_v_bus;
    int32_t *p_v_rms;
    uint8_t *p_main_rly_is_closed;
    pfc_i32_pwm_setter_t p_set_pwm_func;
    void (*p_pwm_enable)(void);
    void (*p_pwm_disable)(void);
} pfc_i32_ctrl_hal_t;

pfc_i32_ctrl_hal_t *pfc_i32_hal_get_ctrl(void);
uint8_t pfc_i32_hal_is_ready(void);
void pfc_i32_hal_lock_binding(void);
void pfc_i32_hal_unlock_binding(void);
void pfc_i32_hal_set_v_g_ptr(int32_t *p);
void pfc_i32_hal_set_v_cap_ptr(int32_t *p);
void pfc_i32_hal_set_i_l_ptr(int32_t *p);
void pfc_i32_hal_set_v_bus_ptr(int32_t *p);
void pfc_i32_hal_set_v_rms_ptr(int32_t *p);
void pfc_i32_hal_set_main_rly_is_closed_ptr(uint8_t *p);
void pfc_i32_hal_set_pwm_setter(pfc_i32_pwm_setter_t p);
void pfc_i32_hal_set_pwm_enable(void (*p)(void));
void pfc_i32_hal_set_pwm_disable(void (*p)(void));

#endif
