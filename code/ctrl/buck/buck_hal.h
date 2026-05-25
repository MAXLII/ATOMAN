// SPDX-License-Identifier: MIT
/**
 * @file    buck_hal.h
 * @brief   buck_hal control public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Declare buck HAL binding accessors and protection-control APIs
 *          - Expose binding lock, unlock, and readiness checks for platform integration
 *          - Provide the bridge between hardware callbacks and buck control/FSM modules
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
#ifndef __BUCK_HAL_H
#define __BUCK_HAL_H

#include <stdint.h>

#ifndef BUCK_CTRL_IND_CURR_CH_NUM
#define BUCK_CTRL_IND_CURR_CH_NUM (2U)
#endif

#if (BUCK_CTRL_IND_CURR_CH_NUM == 0U)
#error "BUCK_CTRL_IND_CURR_CH_NUM must be greater than 0"
#endif

typedef void (*buck_pwm_setter_t)(int32_t cmp, uint8_t up_en, uint8_t dn_en);

typedef struct
{
    int32_t *p_v_in;
    int32_t *p_i_in;
    int32_t *p_v_out;
    int32_t *p_i_out;
    int32_t *p_i_l[BUCK_CTRL_IND_CURR_CH_NUM];
    buck_pwm_setter_t p_set_pwm_func[BUCK_CTRL_IND_CURR_CH_NUM];
    void (*p_pwm_disable)(void);
} buck_ctrl_hal_t;

typedef struct
{
    void (*p_enter_run_func)(void);
    void (*p_exit_run_func)(void);
    uint8_t *p_latched;
} buck_fsm_hal_t;

buck_ctrl_hal_t *buck_hal_get_ctrl(void);
buck_fsm_hal_t *buck_hal_get_fsm(void);
void buck_hal_hard_protect_trip(void);
void buck_hal_hard_protect_clear(void);
uint8_t buck_hal_is_ready(void);
void buck_hal_lock_binding(void);
void buck_hal_unlock_binding(void);
void buck_hal_set_v_in_ptr(int32_t *p);
void buck_hal_set_i_in_ptr(int32_t *p);
void buck_hal_set_v_out_ptr(int32_t *p);
void buck_hal_set_i_out_ptr(int32_t *p);
void buck_hal_set_i_l_ptr(uint32_t ch, int32_t *p);
void buck_hal_set_pwm_setter(uint32_t ch, buck_pwm_setter_t p);
void buck_hal_set_pwm_disable(void (*p)(void));
void buck_hal_set_enter_run_func(void (*p)(void));
void buck_hal_set_exit_run_func(void (*p)(void));
void buck_hal_set_latched_ptr(uint8_t *p);

#endif
