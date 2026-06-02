// SPDX-License-Identifier: MIT
/**
 * @file    boost_hal.c
 * @brief   boost_hal control module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Hold BOOST controller and FSM HAL binding objects for platform callbacks
 *          - Manage run-entry/run-exit actions and PWM disable handling
 *          - Lock and validate HAL binding state before the BOOST FSM allows operation
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
#include "boost_hal.h"
#include "boost_cfg.h"
#include "boost_ctrl.h"
#include "boost_fsm.h"
#include "section.h"
#include <stddef.h>

static void enter_run(void);
static void exit_run(void);
static uint8_t ind_curr_ready(void);
static uint8_t pwm_setter_ready(void);
static uint8_t binding_locked = 1U;

static boost_ctrl_hal_t ctrl_hal = {0};

static void enter_run(void)
{
    boost_ctrl_prepare_run();

    boost_cfg_set_run_allowed(1U);
    boost_cfg_publish_building();
}

static void exit_run(void)
{
    if (ctrl_hal.p_pwm_disable != NULL)
    {
        ctrl_hal.p_pwm_disable();
    }

    boost_cfg_set_run_allowed(0U);
    boost_cfg_publish_building();
}

static boost_fsm_hal_t fsm_hal = {
    .p_enter_run_func = enter_run,
    .p_exit_run_func = exit_run,
};

boost_ctrl_hal_t *boost_hal_get_ctrl(void)
{
    return &ctrl_hal;
}

boost_fsm_hal_t *boost_hal_get_fsm(void)
{
    return &fsm_hal;
}

void boost_hal_pwm_disable(void)
{
    if (ctrl_hal.p_pwm_disable != NULL)
    {
        ctrl_hal.p_pwm_disable();
    }
}

static uint8_t ind_curr_ready(void)
{
    uint32_t ch = 0U;

    for (ch = 0U; ch < BOOST_CTRL_IND_CURR_CH_NUM; ch++)
    {
        if (ctrl_hal.p_i_l[ch] == NULL)
        {
            return 0U;
        }
    }

    return 1U;
}

static uint8_t pwm_setter_ready(void)
{
    uint32_t ch = 0U;

    for (ch = 0U; ch < BOOST_CTRL_IND_CURR_CH_NUM; ch++)
    {
        if (ctrl_hal.p_set_pwm_func[ch] == NULL)
        {
            return 0U;
        }
    }

    return 1U;
}

uint8_t boost_hal_is_ready(void)
{
    return (uint8_t)((ctrl_hal.p_v_in != NULL) &&
                     (ctrl_hal.p_v_out != NULL) &&
                     (ind_curr_ready() != 0U) &&
                     (pwm_setter_ready() != 0U) &&
                     (ctrl_hal.p_pwm_enable != NULL) &&
                     (ctrl_hal.p_pwm_disable != NULL) &&
                     (fsm_hal.p_enter_run_func != NULL) &&
                     (fsm_hal.p_exit_run_func != NULL));
}

void boost_hal_lock_binding(void)
{
    binding_locked = 1U;
}

void boost_hal_unlock_binding(void)
{
    binding_locked = 0U;
}

void boost_hal_set_v_in_ptr(volatile uint32_t *p)
{
    if (binding_locked != 0U)
    {
        return;
    }
    ctrl_hal.p_v_in = p;
}

void boost_hal_set_v_out_ptr(volatile uint32_t *p)
{
    if (binding_locked != 0U)
    {
        return;
    }
    ctrl_hal.p_v_out = p;
}

void boost_hal_set_i_l_ptr(uint32_t ch, volatile uint32_t *p)
{
    if ((binding_locked != 0U) ||
        (ch >= BOOST_CTRL_IND_CURR_CH_NUM))
    {
        return;
    }
    ctrl_hal.p_i_l[ch] = p;
}

void boost_hal_set_pwm_setter(uint32_t ch, boost_pwm_setter_t p)
{
    if ((binding_locked != 0U) ||
        (ch >= BOOST_CTRL_IND_CURR_CH_NUM))
    {
        return;
    }
    ctrl_hal.p_set_pwm_func[ch] = p;
}

void boost_hal_set_pwm_enable(void (*p)(void))
{
    if (binding_locked != 0U)
    {
        return;
    }
    ctrl_hal.p_pwm_enable = p;
}

void boost_hal_set_pwm_disable(void (*p)(void))
{
    if (binding_locked != 0U)
    {
        return;
    }
    ctrl_hal.p_pwm_disable = p;
}

void boost_hal_set_enter_run_func(void (*p)(void))
{
    if (binding_locked != 0U)
    {
        return;
    }
    fsm_hal.p_enter_run_func = p;
}

void boost_hal_set_exit_run_func(void (*p)(void))
{
    if (binding_locked != 0U)
    {
        return;
    }
    fsm_hal.p_exit_run_func = p;
}
