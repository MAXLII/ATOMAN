// SPDX-License-Identifier: MIT
/**
 * @file    boost_hal.h
 * @brief   boost_hal control public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Declare BOOST HAL binding accessors and run-control APIs
 *          - Expose binding lock, unlock, and readiness checks for platform integration
 *          - Provide the bridge between hardware callbacks and BOOST control/FSM modules
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-23
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __BOOST_HAL_H
#define __BOOST_HAL_H

#include <stdint.h>

#ifndef BOOST_CTRL_IND_CURR_CH_NUM
#define BOOST_CTRL_IND_CURR_CH_NUM (2U)
#endif

#if (BOOST_CTRL_IND_CURR_CH_NUM == 0U)
#error "BOOST_CTRL_IND_CURR_CH_NUM must be greater than 0"
#endif

typedef void (*boost_pwm_setter_t)(uint32_t cmp, uint8_t up_en, uint8_t dn_en);

typedef struct
{
    volatile uint32_t *p_v_in;
    volatile uint32_t *p_v_out;
    volatile uint32_t *p_i_l[BOOST_CTRL_IND_CURR_CH_NUM];
    boost_pwm_setter_t p_set_pwm_func[BOOST_CTRL_IND_CURR_CH_NUM];
    void (*p_pwm_enable)(void);
    void (*p_pwm_disable)(void);
} boost_ctrl_hal_t;

typedef struct
{
    void (*p_enter_run_func)(void);
    void (*p_exit_run_func)(void);
} boost_fsm_hal_t;

boost_ctrl_hal_t *boost_hal_get_ctrl(void);
boost_fsm_hal_t *boost_hal_get_fsm(void);
void boost_hal_pwm_disable(void);
uint8_t boost_hal_is_ready(void);
void boost_hal_lock_binding(void);
void boost_hal_unlock_binding(void);
void boost_hal_set_v_in_ptr(volatile uint32_t *p);
void boost_hal_set_v_out_ptr(volatile uint32_t *p);
void boost_hal_set_i_l_ptr(uint32_t ch, volatile uint32_t *p);
void boost_hal_set_pwm_setter(uint32_t ch, boost_pwm_setter_t p);
void boost_hal_set_pwm_enable(void (*p)(void));
void boost_hal_set_pwm_disable(void (*p)(void));
void boost_hal_set_enter_run_func(void (*p)(void));
void boost_hal_set_exit_run_func(void (*p)(void));

#endif
