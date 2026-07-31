// SPDX-License-Identifier: MIT
/**
 * @file    inv_hal.h
 * @brief   Inverter int32 HAL public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Declare integer feedback and PWM-command bindings for inverter control
 *          - Expose binding lifecycle and hard-protection APIs
 *          - Keep ADC-code and PWM-code domains independent from platform hardware
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Bindings must be complete before ISR execution
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-08-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef INV_I32_HAL_H
#define INV_I32_HAL_H

#include <stdint.h>

typedef struct
{
    int32_t *p_v_cap; /**< Capacitor voltage in signed AC-voltage codes. */
    int32_t *p_i_l;   /**< Inductor current in signed current codes. */
    int32_t *p_v_bus; /**< DC-bus voltage in unsigned bus-voltage codes. */
    void (*p_set_pwm_func)(int32_t v_pwm, int32_t v_bus); /**< Publish AC-code-times-reload command. */
    void (*p_pwm_enable)(void);  /**< Enable bridge PWM outputs. */
    void (*p_pwm_disable)(void); /**< Disable bridge PWM outputs. */
} inv_ctrl_hal_t;

typedef struct
{
    void (*p_enter_run_func)(void);   /**< Prepare and enable control execution. */
    void (*p_exit_run_func)(void);    /**< Stop control execution. */
    void (*p_inv_rly_on_func)(void);  /**< Close inverter output relay. */
    void (*p_inv_rly_off_func)(void); /**< Open inverter output relay. */
} inv_fsm_hal_t;

inv_ctrl_hal_t *inv_hal_get_ctrl(void);
inv_fsm_hal_t *inv_hal_get_fsm(void);
void inv_hal_hard_protect_trip(void);
uint8_t inv_hal_is_ready(void);
void inv_hal_lock_binding(void);
void inv_hal_unlock_binding(void);
void inv_hal_set_v_cap_ptr(int32_t *p_value);
void inv_hal_set_i_l_ptr(int32_t *p_value);
void inv_hal_set_v_bus_ptr(int32_t *p_value);
void inv_hal_set_pwm_setter(void (*p_func)(int32_t v_pwm, int32_t v_bus));
void inv_hal_set_pwm_enable(void (*p_func)(void));
void inv_hal_set_pwm_disable(void (*p_func)(void));
void inv_hal_set_enter_run_func(void (*p_func)(void));
void inv_hal_set_exit_run_func(void (*p_func)(void));
void inv_hal_set_inv_rly_on_func(void (*p_func)(void));
void inv_hal_set_inv_rly_off_func(void (*p_func)(void));

#endif
