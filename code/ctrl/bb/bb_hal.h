// SPDX-License-Identifier: MIT
/**
 * @file    bb_hal.h
 * @brief   bb_hal control public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Declare buck-boost HAL binding accessors and protection-control APIs
 *          - Expose binding lock, unlock, and readiness checks for platform integration
 *          - Provide the bridge between hardware callbacks and buck-boost control/FSM modules
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
#ifndef __BB_HAL_H
#define __BB_HAL_H

#include <stdint.h>

typedef struct
{
    float *p_v_in;  /* p_v_in: input-voltage sample */
    float *p_i_in;  /* p_i_in: input-current sample */
    float *p_v_out; /* p_v_out: output-voltage sample */
    float *p_i_out; /* p_i_out: output-current sample */
    float *p_i_l;   /* p_i_l: inductor-current sample */
    void (*p_set_pwm_func)(float buck_duty,
                           uint8_t buck_up_en,
                           uint8_t buck_dn_en,
                           float boost_duty,
                           uint8_t boost_up_en,
                           uint8_t boost_dn_en); /* p_set_pwm_func: buck-boost PWM update hook */
    void (*p_pwm_disable)(void); /* p_pwm_disable: fast PWM shutdown hook */
} bb_ctrl_hal_t;

typedef struct
{
    void (*p_enter_run_func)(void); /* p_enter_run_func: enter-run side effects */
    void (*p_exit_run_func)(void);  /* p_exit_run_func: exit-run side effects */
    uint8_t *p_latched;             /* p_latched: hard-protect latch shared with FSM */
} bb_fsm_hal_t;

bb_ctrl_hal_t *bb_hal_get_ctrl(void);
bb_fsm_hal_t *bb_hal_get_fsm(void);
void bb_hal_hard_protect_trip(void);
void bb_hal_hard_protect_clear(void);
uint8_t bb_hal_is_ready(void);
void bb_hal_lock_binding(void);
void bb_hal_unlock_binding(void);
void bb_hal_set_v_in_ptr(float *p);
void bb_hal_set_i_in_ptr(float *p);
void bb_hal_set_v_out_ptr(float *p);
void bb_hal_set_i_out_ptr(float *p);
void bb_hal_set_i_l_ptr(float *p);
void bb_hal_set_pwm_setter(void (*p)(float buck_duty,
                                     uint8_t buck_up_en,
                                     uint8_t buck_dn_en,
                                     float boost_duty,
                                     uint8_t boost_up_en,
                                     uint8_t boost_dn_en));
void bb_hal_set_pwm_disable(void (*p)(void));
void bb_hal_set_enter_run_func(void (*p)(void));
void bb_hal_set_exit_run_func(void (*p)(void));
void bb_hal_set_latched_ptr(uint8_t *p);

#endif
