// SPDX-License-Identifier: MIT
/**
 * @file    inv_hal.c
 * @brief   Inverter int32 HAL binding module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Store integer inverter feedback and PWM callback bindings
 *          - Coordinate control preparation with FSM run entry and exit
 *          - Reject binding changes after platform initialization is complete
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Binding validation occurs before run entry
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
#include "inv_hal.h"

#include <stddef.h>

#include "inv_cfg.h"
#include "inv_ctrl.h"
#include "inv_fsm.h"
#include "my_math.h"

static void enter_run(void);
static void exit_run(void);
static void relay_on_unbound(void);
static void relay_off_unbound(void);

static uint8_t binding_locked = 1U; /**< Prevents runtime changes to ISR bindings. */
static inv_ctrl_hal_t ctrl_hal = {0}; /**< Integer control HAL bindings. */
static inv_fsm_hal_t fsm_hal = {
    .p_enter_run_func = enter_run,
    .p_exit_run_func = exit_run,
    .p_inv_rly_on_func = relay_on_unbound,
    .p_inv_rly_off_func = relay_off_unbound,
}; /**< FSM callback bindings. */

static void enter_run(void)
{
    inv_ctrl_prepare_run();
    if (ctrl_hal.p_pwm_enable != NULL)
    {
        ctrl_hal.p_pwm_enable();
    }
    inv_cfg_set_run_allowed(1U);
    inv_cfg_publish_building();
}

static void exit_run(void)
{
    if (ctrl_hal.p_pwm_disable != NULL)
    {
        ctrl_hal.p_pwm_disable();
    }
    inv_cfg_set_run_allowed(0U);
    inv_cfg_publish_building();
}

static void relay_on_unbound(void)
{
    PLECS_LOG("inv_i32 relay on hook is not bound\n");
}

static void relay_off_unbound(void)
{
    PLECS_LOG("inv_i32 relay off hook is not bound\n");
}

inv_ctrl_hal_t *inv_hal_get_ctrl(void)
{
    return &ctrl_hal;
}

inv_fsm_hal_t *inv_hal_get_fsm(void)
{
    return &fsm_hal;
}

void inv_hal_hard_protect_trip(void)
{
    if (ctrl_hal.p_pwm_disable != NULL)
    {
        ctrl_hal.p_pwm_disable();
    }
    inv_cfg_set_run_allowed(0U);
    inv_cfg_publish_building();
}

uint8_t inv_hal_is_ready(void)
{
    return (uint8_t)((STRUCT_ALL_PTR_VALID(ctrl_hal) != 0) &&
                     (STRUCT_ALL_PTR_VALID(fsm_hal) != 0));
}

void inv_hal_lock_binding(void)
{
    binding_locked = 1U;
}

void inv_hal_unlock_binding(void)
{
    binding_locked = 0U;
}

void inv_hal_set_v_cap_ptr(int32_t *p_value)
{
    if (binding_locked == 0U)
    {
        ctrl_hal.p_v_cap = p_value;
    }
}

void inv_hal_set_i_l_ptr(int32_t *p_value)
{
    if (binding_locked == 0U)
    {
        ctrl_hal.p_i_l = p_value;
    }
}

void inv_hal_set_v_bus_ptr(int32_t *p_value)
{
    if (binding_locked == 0U)
    {
        ctrl_hal.p_v_bus = p_value;
    }
}

void inv_hal_set_pwm_setter(void (*p_func)(int32_t v_pwm, int32_t v_bus))
{
    if (binding_locked == 0U)
    {
        ctrl_hal.p_set_pwm_func = p_func;
    }
}

void inv_hal_set_pwm_enable(void (*p_func)(void))
{
    if (binding_locked == 0U)
    {
        ctrl_hal.p_pwm_enable = p_func;
    }
}

void inv_hal_set_pwm_disable(void (*p_func)(void))
{
    if (binding_locked == 0U)
    {
        ctrl_hal.p_pwm_disable = p_func;
    }
}

void inv_hal_set_enter_run_func(void (*p_func)(void))
{
    if (binding_locked == 0U)
    {
        fsm_hal.p_enter_run_func = p_func;
    }
}

void inv_hal_set_exit_run_func(void (*p_func)(void))
{
    if (binding_locked == 0U)
    {
        fsm_hal.p_exit_run_func = p_func;
    }
}

void inv_hal_set_inv_rly_on_func(void (*p_func)(void))
{
    if (binding_locked == 0U)
    {
        fsm_hal.p_inv_rly_on_func = p_func;
    }
}

void inv_hal_set_inv_rly_off_func(void (*p_func)(void))
{
    if (binding_locked == 0U)
    {
        fsm_hal.p_inv_rly_off_func = p_func;
    }
}
