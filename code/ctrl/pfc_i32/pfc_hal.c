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
#include <stddef.h>

static uint8_t pfc_i32_hal_binding_locked = 1U;
static pfc_i32_ctrl_hal_t pfc_i32_ctrl_hal = {0};

pfc_i32_ctrl_hal_t *pfc_i32_hal_get_ctrl(void)
{
    return &pfc_i32_ctrl_hal;
}

uint8_t pfc_i32_hal_is_ready(void)
{
    return (uint8_t)((pfc_i32_ctrl_hal.p_v_g != NULL) &&
                     (pfc_i32_ctrl_hal.p_v_cap != NULL) &&
                     (pfc_i32_ctrl_hal.p_i_l != NULL) &&
                     (pfc_i32_ctrl_hal.p_v_bus != NULL) &&
                     (pfc_i32_ctrl_hal.p_v_rms != NULL) &&
                     (pfc_i32_ctrl_hal.p_main_rly_is_closed != NULL) &&
                     (pfc_i32_ctrl_hal.p_set_pwm_func != NULL) &&
                     (pfc_i32_ctrl_hal.p_pwm_enable != NULL) &&
                     (pfc_i32_ctrl_hal.p_pwm_disable != NULL));
}

void pfc_i32_hal_lock_binding(void)
{
    pfc_i32_hal_binding_locked = 1U;
}

void pfc_i32_hal_unlock_binding(void)
{
    pfc_i32_hal_binding_locked = 0U;
}

void pfc_i32_hal_set_v_g_ptr(int32_t *p)
{
    if (pfc_i32_hal_binding_locked == 0U)
    {
        pfc_i32_ctrl_hal.p_v_g = p;
    }
}

void pfc_i32_hal_set_v_cap_ptr(int32_t *p)
{
    if (pfc_i32_hal_binding_locked == 0U)
    {
        pfc_i32_ctrl_hal.p_v_cap = p;
    }
}

void pfc_i32_hal_set_i_l_ptr(int32_t *p)
{
    if (pfc_i32_hal_binding_locked == 0U)
    {
        pfc_i32_ctrl_hal.p_i_l = p;
    }
}

void pfc_i32_hal_set_v_bus_ptr(int32_t *p)
{
    if (pfc_i32_hal_binding_locked == 0U)
    {
        pfc_i32_ctrl_hal.p_v_bus = p;
    }
}

void pfc_i32_hal_set_v_rms_ptr(int32_t *p)
{
    if (pfc_i32_hal_binding_locked == 0U)
    {
        pfc_i32_ctrl_hal.p_v_rms = p;
    }
}

void pfc_i32_hal_set_main_rly_is_closed_ptr(uint8_t *p)
{
    if (pfc_i32_hal_binding_locked == 0U)
    {
        pfc_i32_ctrl_hal.p_main_rly_is_closed = p;
    }
}

void pfc_i32_hal_set_pwm_setter(pfc_i32_pwm_setter_t p)
{
    if (pfc_i32_hal_binding_locked == 0U)
    {
        pfc_i32_ctrl_hal.p_set_pwm_func = p;
    }
}

void pfc_i32_hal_set_pwm_enable(void (*p)(void))
{
    if (pfc_i32_hal_binding_locked == 0U)
    {
        pfc_i32_ctrl_hal.p_pwm_enable = p;
    }
}

void pfc_i32_hal_set_pwm_disable(void (*p)(void))
{
    if (pfc_i32_hal_binding_locked == 0U)
    {
        pfc_i32_ctrl_hal.p_pwm_disable = p;
    }
}
