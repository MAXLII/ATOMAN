// SPDX-License-Identifier: MIT
/**
 * @file    llc_hal.c
 * @brief   LLC HAL binding module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Hold LLC controller and FSM HAL binding objects for platform callbacks
 *          - Manage run-entry/run-exit actions, PWM disable, and hard-protection latch handling
 *          - Lock and validate HAL binding state before the LLC FSM allows operation
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
#include "llc_hal.h"
#include "llc_cfg.h"
#include "llc_ctrl.h"
#include <stddef.h>

static void llc_hal_enter_run(void);
static void llc_hal_exit_run(void);

static uint8_t hard_protect_latched = 0U;
static uint8_t llc_hal_binding_locked = 1U;
static llc_ctrl_hal_t llc_ctrl_hal = {0};
static llc_fsm_hal_t llc_fsm_hal = {
    .p_enter_run_func = llc_hal_enter_run,
    .p_exit_run_func = llc_hal_exit_run,
    .p_latched = &hard_protect_latched,
};

static void llc_hal_enter_run(void)
{
    llc_ctrl_prepare_run();

    if (llc_ctrl_hal.p_pwm_enable != NULL)
    {
        llc_ctrl_hal.p_pwm_enable();
    }

    llc_cfg_set_run_allowed(1U);
    llc_cfg_publish_building();
}

static void llc_hal_exit_run(void)
{
    llc_hal_pwm_disable();
    llc_cfg_set_run_allowed(0U);
    llc_cfg_publish_building();
}

llc_ctrl_hal_t *llc_hal_get_ctrl(void)
{
    return &llc_ctrl_hal;
}

llc_fsm_hal_t *llc_hal_get_fsm(void)
{
    return &llc_fsm_hal;
}

uint8_t llc_hal_is_ready(void)
{
    return (uint8_t)((llc_ctrl_hal.p_v_out != NULL) &&
                     (llc_ctrl_hal.p_i_out != NULL) &&
                     (llc_ctrl_hal.p_v_bus != NULL) &&
                     (llc_ctrl_hal.p_set_pwm_func != NULL) &&
                     (llc_ctrl_hal.p_pwm_disable != NULL) &&
                     (llc_fsm_hal.p_enter_run_func != NULL) &&
                     (llc_fsm_hal.p_exit_run_func != NULL) &&
                     (llc_fsm_hal.p_latched != NULL));
}

void llc_hal_lock_binding(void)
{
    llc_hal_binding_locked = 1U;
}

void llc_hal_unlock_binding(void)
{
    llc_hal_binding_locked = 0U;
}

void llc_hal_pwm_disable(void)
{
    if (llc_ctrl_hal.p_pwm_disable != NULL)
    {
        llc_ctrl_hal.p_pwm_disable();
    }
}

void llc_hal_hard_protect_trip(void)
{
    llc_hal_pwm_disable();

    if (llc_fsm_hal.p_latched != NULL)
    {
        *llc_fsm_hal.p_latched = 1U;
    }

    llc_cfg_set_run_allowed(0U);
    llc_cfg_publish_building();
}

void llc_hal_hard_protect_clear(void)
{
    if (llc_fsm_hal.p_latched != NULL)
    {
        *llc_fsm_hal.p_latched = 0U;
    }
}

void llc_hal_set_v_out_ptr(float *p)
{
    if (llc_hal_binding_locked == 0U)
    {
        llc_ctrl_hal.p_v_out = p;
    }
}

void llc_hal_set_i_out_ptr(float *p)
{
    if (llc_hal_binding_locked == 0U)
    {
        llc_ctrl_hal.p_i_out = p;
    }
}

void llc_hal_set_v_bus_ptr(float *p)
{
    if (llc_hal_binding_locked == 0U)
    {
        llc_ctrl_hal.p_v_bus = p;
    }
}

void llc_hal_set_pwm_setter(llc_pwm_setter_t p)
{
    if (llc_hal_binding_locked == 0U)
    {
        llc_ctrl_hal.p_set_pwm_func = p;
    }
}

void llc_hal_set_pwm_enable(void (*p)(void))
{
    if (llc_hal_binding_locked == 0U)
    {
        llc_ctrl_hal.p_pwm_enable = p;
    }
}

void llc_hal_set_pwm_disable(void (*p)(void))
{
    if (llc_hal_binding_locked == 0U)
    {
        llc_ctrl_hal.p_pwm_disable = p;
    }
}

void llc_hal_set_enter_run_func(void (*p)(void))
{
    if (llc_hal_binding_locked == 0U)
    {
        llc_fsm_hal.p_enter_run_func = p;
    }
}

void llc_hal_set_exit_run_func(void (*p)(void))
{
    if (llc_hal_binding_locked == 0U)
    {
        llc_fsm_hal.p_exit_run_func = p;
    }
}

void llc_hal_set_latched_ptr(uint8_t *p)
{
    if (llc_hal_binding_locked == 0U)
    {
        llc_fsm_hal.p_latched = p;
    }
}
