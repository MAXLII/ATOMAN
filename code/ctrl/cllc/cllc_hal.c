// SPDX-License-Identifier: MIT
/**
 * @file    cllc_hal.c
 * @brief   Bidirectional CLLC HAL binding module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Own CLLC measurement, modulation, and FSM binding objects
 *          - Coordinate direction-aware run entry and safe run exit
 *          - Latch hard protection independently of the control ISR
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Hard-protection trip is safe to call from interrupt context
 *          - Platform-specific PWM details remain behind callbacks
 *
 * @author  Max.Li
 * @date    2026-07-26
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "cllc_hal.h"

#include "cllc_ctrl.h"

#include <stddef.h>

static void enter_run(CLLC_DIRECTION_E direction);
static void exit_run(void);

static uint8_t hard_protect_latched = 0u; /* Shared hardware-protection latch. */
static uint8_t binding_locked = 1u;       /* Nonzero rejects platform rebinding. */
static cllc_ctrl_hal_t ctrl_hal = {0};    /* Fast-loop hardware bindings. */
static cllc_fsm_hal_t fsm_hal = {         /* FSM lifecycle bindings. */
    .p_enter_run = enter_run,
    .p_exit_run = exit_run,
    .p_latched = &hard_protect_latched,
};

/** Prepare controller state, enable the selected bridge, and publish run permission. */
static void enter_run(CLLC_DIRECTION_E direction)
{
    cllc_ctrl_prepare_run(direction);
    if (ctrl_hal.p_pwm_enable != NULL)
    {
        ctrl_hal.p_pwm_enable(direction);
    }
    cllc_cfg_set_run_allowed(1u);
    cllc_cfg_publish_building();
}

/** Disable both bridges and revoke control-loop run permission. */
static void exit_run(void)
{
    cllc_hal_pwm_disable();
    cllc_cfg_set_run_allowed(0u);
    cllc_cfg_publish_building();
}

cllc_ctrl_hal_t *cllc_hal_get_ctrl(void)
{
    return &ctrl_hal;
}

cllc_fsm_hal_t *cllc_hal_get_fsm(void)
{
    return &fsm_hal;
}

uint8_t cllc_hal_is_ready(void)
{
    if (ctrl_hal.p_v_battery == NULL)
    {
        return 0u;
    }
    if (ctrl_hal.p_i_battery == NULL)
    {
        return 0u;
    }
    if (ctrl_hal.p_v_bus == NULL)
    {
        return 0u;
    }
    if (ctrl_hal.p_set_modulation == NULL)
    {
        return 0u;
    }
    if (ctrl_hal.p_pwm_enable == NULL)
    {
        return 0u;
    }
    if (ctrl_hal.p_pwm_disable == NULL)
    {
        return 0u;
    }
    if (fsm_hal.p_enter_run == NULL)
    {
        return 0u;
    }
    if (fsm_hal.p_exit_run == NULL)
    {
        return 0u;
    }
    return (fsm_hal.p_latched != NULL) ? 1u : 0u;
}

void cllc_hal_lock_binding(void)
{
    binding_locked = 1u;
}

void cllc_hal_unlock_binding(void)
{
    binding_locked = 0u;
}

void cllc_hal_pwm_disable(void)
{
    if (ctrl_hal.p_pwm_disable != NULL)
    {
        ctrl_hal.p_pwm_disable();
    }
}

void cllc_hal_hard_protect_trip(void)
{
    cllc_hal_pwm_disable();
    if (fsm_hal.p_latched != NULL)
    {
        *fsm_hal.p_latched = 1u;
    }
    cllc_cfg_set_run_allowed(0u);
    cllc_cfg_publish_building();
}

void cllc_hal_hard_protect_clear(void)
{
    if (fsm_hal.p_latched != NULL)
    {
        *fsm_hal.p_latched = 0u;
    }
}

void cllc_hal_set_v_battery_ptr(float *p_value)
{
    if (binding_locked == 0u)
    {
        ctrl_hal.p_v_battery = p_value;
    }
}

void cllc_hal_set_i_battery_ptr(float *p_value)
{
    if (binding_locked == 0u)
    {
        ctrl_hal.p_i_battery = p_value;
    }
}

void cllc_hal_set_v_bus_ptr(float *p_value)
{
    if (binding_locked == 0u)
    {
        ctrl_hal.p_v_bus = p_value;
    }
}

void cllc_hal_set_modulation_setter(cllc_modulation_setter_t p_setter)
{
    if (binding_locked == 0u)
    {
        ctrl_hal.p_set_modulation = p_setter;
    }
}

void cllc_hal_set_pwm_enable(cllc_pwm_enable_t p_enable)
{
    if (binding_locked == 0u)
    {
        ctrl_hal.p_pwm_enable = p_enable;
    }
}

void cllc_hal_set_pwm_disable(void (*p_disable)(void))
{
    if (binding_locked == 0u)
    {
        ctrl_hal.p_pwm_disable = p_disable;
    }
}

void cllc_hal_set_latched_ptr(uint8_t *p_latched)
{
    if (binding_locked == 0u)
    {
        fsm_hal.p_latched = p_latched;
    }
}
