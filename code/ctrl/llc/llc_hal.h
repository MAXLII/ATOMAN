// SPDX-License-Identifier: MIT
/**
 * @file    llc_hal.h
 * @brief   LLC HAL binding public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Declare LLC control measurement and actuator bindings
 *          - Expose binding lock, unlock, and readiness checks for platform integration
 *          - Provide the bridge between hardware callbacks and LLC control/FSM modules
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-10
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __LLC_HAL_H
#define __LLC_HAL_H

#include <stdint.h>

typedef void (*llc_pwm_setter_t)(float duty);
typedef struct
{
    float *p_v_out;
    float *p_i_out;
    float *p_v_bus;
    llc_pwm_setter_t p_set_pwm_func;
    void (*p_pwm_enable)(void);
    void (*p_pwm_disable)(void);
} llc_ctrl_hal_t;

typedef struct
{
    void (*p_enter_run_func)(void);
    void (*p_exit_run_func)(void);
    uint8_t *p_latched;
} llc_fsm_hal_t;

llc_ctrl_hal_t *llc_hal_get_ctrl(void);
llc_fsm_hal_t *llc_hal_get_fsm(void);
uint8_t llc_hal_is_ready(void);
void llc_hal_lock_binding(void);
void llc_hal_unlock_binding(void);
void llc_hal_pwm_disable(void);
void llc_hal_hard_protect_trip(void);
void llc_hal_hard_protect_clear(void);
void llc_hal_set_v_out_ptr(float *p);
void llc_hal_set_i_out_ptr(float *p);
void llc_hal_set_v_bus_ptr(float *p);
void llc_hal_set_pwm_setter(llc_pwm_setter_t p);
void llc_hal_set_pwm_enable(void (*p)(void));
void llc_hal_set_pwm_disable(void (*p)(void));
void llc_hal_set_enter_run_func(void (*p)(void));
void llc_hal_set_exit_run_func(void (*p)(void));
void llc_hal_set_latched_ptr(uint8_t *p);

#endif
