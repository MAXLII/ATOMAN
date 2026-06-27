// SPDX-License-Identifier: MIT
/**
 * @file    pfc_hal.c
 * @brief   PFC int32 HAL binding module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Hold PFC int32 controller HAL binding pointers
 *          - Validate integer feedback and PWM callback bindings before run
 *          - Provide lockable binding setters for platform startup code
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-27
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "pfc_hal.h"
#include "pfc_ctrl.h"
#include "pfc_cfg.h"
#include "section.h"
#include <stddef.h>

static void pfc_hal_enter_run(void);
static void pfc_hal_exit_run(void);
static uint8_t pfc_hal_binding_locked = 1U;
static pfc_ctrl_hal_t pfc_ctrl_hal = {0};
static pfc_fsm_hal_t pfc_fsm_hal = {
    .p_enter_run_func = pfc_hal_enter_run,
    .p_exit_run_func = pfc_hal_exit_run,
};

static void pfc_hal_enter_run(void)
{
    PLECS_LOG("PFC_hal enter run\n");
    pfc_ctrl_prepare_run();
    pfc_cfg_set_run_allowed(1U);
    pfc_cfg_publish_building();

    if (pfc_ctrl_hal.p_pwm_enable != NULL)
    {
        pfc_ctrl_hal.p_pwm_enable();
    }
}

static void pfc_hal_exit_run(void)
{
    PLECS_LOG("PFC_hal exit run\n");
    pfc_cfg_set_run_allowed(0U);
    pfc_cfg_publish_building();

    if (pfc_ctrl_hal.p_pwm_disable != NULL)
    {
        pfc_ctrl_hal.p_pwm_disable();
    }
}

pfc_ctrl_hal_t *pfc_hal_get_ctrl(void)
{
    return &pfc_ctrl_hal;
}

pfc_fsm_hal_t *pfc_hal_get_fsm(void)
{
    return &pfc_fsm_hal;
}

void pfc_hal_hard_protect_trip(void)
{
    if (pfc_ctrl_hal.p_pwm_disable != NULL)
    {
        pfc_ctrl_hal.p_pwm_disable();
    }

    pfc_cfg_set_run_allowed(0U);
    pfc_cfg_publish_building();
}

uint8_t pfc_hal_is_ready(void)
{
    return (uint8_t)((pfc_ctrl_hal.p_v_g != NULL) &&
                     (pfc_ctrl_hal.p_v_cap != NULL) &&
                     (pfc_ctrl_hal.p_i_l != NULL) &&
                     (pfc_ctrl_hal.p_v_bus != NULL) &&
                     (pfc_ctrl_hal.p_v_rms != NULL) &&
                     (pfc_ctrl_hal.p_main_rly_is_closed != NULL) &&
                     (pfc_ctrl_hal.p_set_pwm_func != NULL) &&
                     (pfc_ctrl_hal.p_pwm_enable != NULL) &&
                     (pfc_ctrl_hal.p_pwm_disable != NULL) &&
                     (pfc_fsm_hal.p_vbus_sta != NULL) &&
                     (pfc_fsm_hal.p_main_rly_is_closed != NULL) &&
                     (pfc_fsm_hal.p_enter_run_func != NULL) &&
                     (pfc_fsm_hal.p_exit_run_func != NULL) &&
                     (pfc_fsm_hal.p_main_rly_on_func != NULL) &&
                     (pfc_fsm_hal.p_main_rly_off_func != NULL));
}

void pfc_hal_lock_binding(void)
{
    pfc_hal_binding_locked = 1U;
}

void pfc_hal_unlock_binding(void)
{
    pfc_hal_binding_locked = 0U;
}

void pfc_hal_set_v_g_ptr(int32_t *p)
{
    if (pfc_hal_binding_locked == 0U)
    {
        pfc_ctrl_hal.p_v_g = p;
    }
}

void pfc_hal_set_v_cap_ptr(int32_t *p)
{
    if (pfc_hal_binding_locked == 0U)
    {
        pfc_ctrl_hal.p_v_cap = p;
    }
}

void pfc_hal_set_i_l_ptr(int32_t *p)
{
    if (pfc_hal_binding_locked == 0U)
    {
        pfc_ctrl_hal.p_i_l = p;
    }
}

void pfc_hal_set_v_bus_ptr(int32_t *p)
{
    if (pfc_hal_binding_locked == 0U)
    {
        pfc_ctrl_hal.p_v_bus = p;
    }
}

void pfc_hal_set_v_rms_ptr(int32_t *p)
{
    if (pfc_hal_binding_locked == 0U)
    {
        pfc_ctrl_hal.p_v_rms = p;
    }
}

void pfc_hal_set_main_rly_is_closed_ptr(uint8_t *p)
{
    if (pfc_hal_binding_locked == 0U)
    {
        pfc_ctrl_hal.p_main_rly_is_closed = p;
        pfc_fsm_hal.p_main_rly_is_closed = p;
    }
}

void pfc_hal_set_pwm_setter(void (*p)(int32_t v_pwm, int32_t v_bus))
{
    if (pfc_hal_binding_locked == 0U)
    {
        pfc_ctrl_hal.p_set_pwm_func = p;
    }
}

void pfc_hal_set_pwm_enable(void (*p)(void))
{
    if (pfc_hal_binding_locked == 0U)
    {
        pfc_ctrl_hal.p_pwm_enable = p;
    }
}

void pfc_hal_set_pwm_disable(void (*p)(void))
{
    if (pfc_hal_binding_locked == 0U)
    {
        pfc_ctrl_hal.p_pwm_disable = p;
    }
}

void pfc_hal_set_vbus_sta_ptr(pfc_vbus_sta_e *p)
{
    if (pfc_hal_binding_locked == 0U)
    {
        pfc_fsm_hal.p_vbus_sta = p;
    }
}

void pfc_hal_set_enter_run_func(void (*p)(void))
{
    if (pfc_hal_binding_locked == 0U)
    {
        pfc_fsm_hal.p_enter_run_func = p;
    }
}

void pfc_hal_set_exit_run_func(void (*p)(void))
{
    if (pfc_hal_binding_locked == 0U)
    {
        pfc_fsm_hal.p_exit_run_func = p;
    }
}

void pfc_hal_set_main_rly_on_func(void (*p)(void))
{
    if (pfc_hal_binding_locked == 0U)
    {
        pfc_fsm_hal.p_main_rly_on_func = p;
    }
}

void pfc_hal_set_main_rly_off_func(void (*p)(void))
{
    if (pfc_hal_binding_locked == 0U)
    {
        pfc_fsm_hal.p_main_rly_off_func = p;
    }
}
