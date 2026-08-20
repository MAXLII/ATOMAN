// SPDX-License-Identifier: MIT
/**
 * @file    buck_hal.c
 * @brief   buck_hal control module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Hold buck controller and FSM HAL binding objects for platform callbacks
 *          - Manage run-entry/run-exit actions, PWM disable, and hard-protection latch handling
 *          - Allow the Buck FSM init state to validate the complete binding objects
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
#include "buck_hal.h"
#include "buck_ctrl.h"
#include "section.h"

static void buck_hal_enter_run(void);
static void buck_hal_exit_run(void);
static volatile uint8_t hard_protect_latched;
static uint8_t buck_hal_binding_locked = 0U;

buck_ctrl_hal_t buck_ctrl_hal = {0};

static void buck_hal_enter_run(void)
{
    PLECS_LOG("buck_hal enter run\n");
    buck_ctrl_prepare_run();
}

static void buck_hal_exit_run(void)
{
    PLECS_LOG("buck_hal exit run\n");
    buck_ctrl_hal.p_pwm_disable();
}

buck_fsm_hal_t buck_fsm_hal = {
    .p_enter_run_func = buck_hal_enter_run,
    .p_exit_run_func = buck_hal_exit_run,
};

buck_ctrl_hal_t *buck_hal_get_ctrl(void)
{
    return &buck_ctrl_hal;
}

buck_fsm_hal_t *buck_hal_get_fsm(void)
{
    return &buck_fsm_hal;
}

void buck_hal_hard_protect_trip(void)
{
    buck_ctrl_hal.p_pwm_disable();
    hard_protect_latched = 1U;
}

void buck_hal_hard_protect_clear(void)
{
    hard_protect_latched = 0U;
}

uint8_t buck_hal_hard_protect_is_latched(void)
{
    return hard_protect_latched;
}

void buck_hal_lock_binding(void)
{
    buck_hal_binding_locked = 1U;
}

void buck_hal_unlock_binding(void)
{
    buck_hal_binding_locked = 0U;
}

void buck_hal_set_v_in_ptr(int32_t *p)
{
    if (buck_hal_binding_locked != 0U)
    {
        return;
    }
    buck_ctrl_hal.p_v_in = p;
}

void buck_hal_set_v_out_ptr(int32_t *p)
{
    if (buck_hal_binding_locked != 0U)
    {
        return;
    }
    buck_ctrl_hal.p_v_out = p;
}

void buck_hal_set_i_l_ptr(uint32_t ch, int32_t *p)
{
    if ((buck_hal_binding_locked != 0U) ||
        (ch >= BUCK_CTRL_IND_CURR_CH_NUM))
    {
        return;
    }
    buck_ctrl_hal.p_i_l[ch] = p;
}

void buck_hal_set_pwm_setter(uint32_t ch, buck_pwm_setter_t p)
{
    if ((buck_hal_binding_locked != 0U) ||
        (ch >= BUCK_CTRL_IND_CURR_CH_NUM))
    {
        return;
    }
    buck_ctrl_hal.p_set_pwm_func[ch] = p;
}

void buck_hal_set_pwm_disable(void (*p)(void))
{
    if (buck_hal_binding_locked != 0U)
    {
        return;
    }
    buck_ctrl_hal.p_pwm_disable = p;
}
