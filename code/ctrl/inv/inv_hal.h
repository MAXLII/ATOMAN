// SPDX-License-Identifier: MIT
/**
 * @file    inv_hal.h
 * @brief   inv_hal control public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Declare inverter HAL binding accessors and hard-protection APIs
 *          - Expose binding lock, unlock, and readiness checks for platform integration
 *          - Provide the hardware abstraction bridge for inverter control and FSM modules
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __INV_HAL_H
#define __INV_HAL_H

#include <stdint.h>

typedef struct
{
    float *p_v_cap;
    float *p_i_l;
    float *p_v_bus;
    void (*p_set_pwm_func)(float v_pwm, float v_bus);
    void (*p_pwm_enable)(void);
    void (*p_pwm_disable)(void);
} inv_ctrl_hal_t;

typedef struct
{
    void (*p_enter_run_func)(void);
    void (*p_exit_run_func)(void);
    void (*p_inv_rly_on_func)(void);
    void (*p_inv_rly_off_func)(void);
    uint8_t *p_latched;
} inv_fsm_hal_t;

inv_ctrl_hal_t *inv_hal_get_ctrl(void);
inv_fsm_hal_t *inv_hal_get_fsm(void);
void inv_hal_hard_protect_trip(void);
void inv_hal_hard_protect_clear(void);
uint8_t inv_hal_is_ready(void);
void inv_hal_lock_binding(void);
void inv_hal_unlock_binding(void);
void inv_hal_set_v_cap_ptr(float *p);
void inv_hal_set_i_l_ptr(float *p);
void inv_hal_set_v_bus_ptr(float *p);
void inv_hal_set_pwm_setter(void (*p)(float v_pwm, float v_bus));
void inv_hal_set_pwm_enable(void (*p)(void));
void inv_hal_set_pwm_disable(void (*p)(void));
void inv_hal_set_enter_run_func(void (*p)(void));
void inv_hal_set_exit_run_func(void (*p)(void));
void inv_hal_set_inv_rly_on_func(void (*p)(void));
void inv_hal_set_inv_rly_off_func(void (*p)(void));
void inv_hal_set_latched_ptr(uint8_t *p);

#endif
