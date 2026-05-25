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
 *          - Lock and validate HAL binding state before the buck FSM allows operation
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
#include "buck_cfg.h"
#include "buck_ctrl.h"
#include "buck_fsm.h"
#include "section.h"
#include <stddef.h>

static void buck_hal_enter_run(void);
static void buck_hal_exit_run(void);
static uint8_t buck_hal_ind_curr_ready(void);
static uint8_t buck_hal_pwm_setter_ready(void);
static uint8_t hard_protect_latched;
static uint8_t buck_hal_binding_locked = 1U;

static buck_ctrl_hal_t buck_ctrl_hal = {0};

static void buck_hal_enter_run(void)
{
    PLECS_LOG("buck_hal enter run\n");
    buck_ctrl_prepare_run();

    buck_cfg_set_run_allowed(1U);
    buck_cfg_publish_building();
}

static void buck_hal_exit_run(void)
{
    PLECS_LOG("buck_hal exit run\n");

    if (buck_ctrl_hal.p_pwm_disable != NULL)
    {
        buck_ctrl_hal.p_pwm_disable();
    }

    buck_cfg_set_run_allowed(0U);
    buck_cfg_publish_building();
}

static buck_fsm_hal_t buck_fsm_hal = {
    .p_enter_run_func = buck_hal_enter_run,
    .p_exit_run_func = buck_hal_exit_run,
    .p_latched = &hard_protect_latched,
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
    if (buck_ctrl_hal.p_pwm_disable != NULL)
    {
        buck_ctrl_hal.p_pwm_disable();
    }

    if (*buck_fsm_hal.p_latched == 0U)
    {
        *buck_fsm_hal.p_latched = 1U;
    }

    buck_cfg_set_run_allowed(0U);
    buck_cfg_publish_building();
}

void buck_hal_hard_protect_clear(void)
{
    *buck_fsm_hal.p_latched = 0U;
}

static uint8_t buck_hal_ind_curr_ready(void)
{
    uint32_t ch = 0U;

    for (ch = 0U; ch < BUCK_CTRL_IND_CURR_CH_NUM; ch++)
    {
        if (buck_ctrl_hal.p_i_l[ch] == NULL)
        {
            return 0U;
        }
    }

    return 1U;
}

static uint8_t buck_hal_pwm_setter_ready(void)
{
    uint32_t ch = 0U;

    for (ch = 0U; ch < BUCK_CTRL_IND_CURR_CH_NUM; ch++)
    {
        if (buck_ctrl_hal.p_set_pwm_func[ch] == NULL)
        {
            return 0U;
        }
    }

    return 1U;
}

uint8_t buck_hal_is_ready(void)
{
    return (uint8_t)((buck_ctrl_hal.p_v_in != NULL) &&
                     (buck_ctrl_hal.p_v_out != NULL) &&
                     (buck_hal_ind_curr_ready() != 0U) &&
                     (buck_hal_pwm_setter_ready() != 0U) &&
                     (buck_ctrl_hal.p_pwm_disable != NULL) &&
                     (buck_fsm_hal.p_enter_run_func != NULL) &&
                     (buck_fsm_hal.p_exit_run_func != NULL) &&
                     (buck_fsm_hal.p_latched != NULL));
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

void buck_hal_set_i_in_ptr(int32_t *p)
{
    if (buck_hal_binding_locked != 0U)
    {
        return;
    }
    buck_ctrl_hal.p_i_in = p;
}

void buck_hal_set_v_out_ptr(int32_t *p)
{
    if (buck_hal_binding_locked != 0U)
    {
        return;
    }
    buck_ctrl_hal.p_v_out = p;
}

void buck_hal_set_i_out_ptr(int32_t *p)
{
    if (buck_hal_binding_locked != 0U)
    {
        return;
    }
    buck_ctrl_hal.p_i_out = p;
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

void buck_hal_set_enter_run_func(void (*p)(void))
{
    if (buck_hal_binding_locked != 0U)
    {
        return;
    }
    buck_fsm_hal.p_enter_run_func = p;
}

void buck_hal_set_exit_run_func(void (*p)(void))
{
    if (buck_hal_binding_locked != 0U)
    {
        return;
    }
    buck_fsm_hal.p_exit_run_func = p;
}

void buck_hal_set_latched_ptr(uint8_t *p)
{
    if (buck_hal_binding_locked != 0U)
    {
        return;
    }
    buck_fsm_hal.p_latched = p;
}
