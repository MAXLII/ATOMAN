// SPDX-License-Identifier: MIT
/**
 * @file    npc_hal.c
 * @brief   Own static hardware bindings and protection latch; supply default run actions.
 * @details Base digital power framework. C11, static storage, no allocation.
 *          Bindings change only while stopped. PLECS serializes the dispatcher;
 *          ISR setpoints use the FSM publication snapshot.
 * @author  Max.Li
 * @date    2026-09-13
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License. See LICENSE in the project root.
 */
#include "npc_hal.h"
#include "npc_ctrl.h"
#include "section.h"
#include <stddef.h>
#include <stdatomic.h>
static npc_ctrl_hal_t ctrl_hal = {0};
static uint8_t binding_locked = 1U;
/* One atomic word publishes fault and phase together. */
static atomic_uint fault_word = ATOMIC_VAR_INIT(0U);
static void npc_hal_enter_run(void) { npc_ctrl_prepare_run(); }
static void npc_hal_exit_run(void) { npc_ctrl_stop(); }
static npc_fsm_hal_t fsm_hal = {npc_hal_enter_run, npc_hal_exit_run};
static void npc_hal_init(void) { atomic_store(&fault_word, 0U); }
REG_INIT(0, npc_hal_init)
const npc_ctrl_hal_t *npc_hal_get_ctrl(void) { return &ctrl_hal; }
const npc_fsm_hal_t *npc_hal_get_fsm(void) { return &fsm_hal; }
void npc_hal_lock_binding(void) { binding_locked = 1U; }
void npc_hal_unlock_binding(void) { binding_locked = 0U; }
uint8_t npc_hal_is_ready(void)
{
    for (uint32_t phase = 0U; phase < 3U; ++phase)
        if ((ctrl_hal.p_v_out[phase] == NULL) || (ctrl_hal.p_i_l[phase] == NULL)) return 0U;
    return ((ctrl_hal.p_v_dc_p != NULL) && (ctrl_hal.p_v_dc_n != NULL) &&
            (ctrl_hal.p_theta != NULL) && (ctrl_hal.p_vd_pos_ref != NULL) &&
            (ctrl_hal.p_set_pwm_func != NULL) &&
            (ctrl_hal.p_pwm_disable != NULL) && (fsm_hal.p_enter_run_func != NULL) &&
            (fsm_hal.p_exit_run_func != NULL)) ? 1U : 0U;
}
void npc_hal_set_v_out_ptr(uint32_t phase, float *p_value)
{ if ((binding_locked == 0U) && (phase < 3U)) ctrl_hal.p_v_out[phase] = p_value; }
void npc_hal_set_i_l_ptr(uint32_t phase, float *p_value)
{ if ((binding_locked == 0U) && (phase < 3U)) ctrl_hal.p_i_l[phase] = p_value; }
void npc_hal_set_v_dc_p_ptr(float *p_value)
{ if (binding_locked == 0U) ctrl_hal.p_v_dc_p = p_value; }
void npc_hal_set_v_dc_n_ptr(float *p_value)
{ if (binding_locked == 0U) ctrl_hal.p_v_dc_n = p_value; }
void npc_hal_set_theta_ptr(float *p_value)
{ if (binding_locked == 0U) ctrl_hal.p_theta = p_value; }
void npc_hal_set_vd_pos_ref_ptr(float *p_value)
{
    if (binding_locked == 0U)
    {
        ctrl_hal.p_vd_pos_ref = p_value;
    }
}
void npc_hal_set_pwm_setter(uint32_t (*p_func)(float, float, float, float))
{ if (binding_locked == 0U) ctrl_hal.p_set_pwm_func = p_func; }
void npc_hal_set_pwm_disable(void (*p_func)(void))
{ if (binding_locked == 0U) ctrl_hal.p_pwm_disable = p_func; }
void npc_hal_set_enter_run_func(void (*p_func)(void))
{ if (binding_locked == 0U) fsm_hal.p_enter_run_func = p_func; }
void npc_hal_set_exit_run_func(void (*p_func)(void))
{ if (binding_locked == 0U) fsm_hal.p_exit_run_func = p_func; }
void npc_hal_hard_protect_trip(uint32_t fault, uint32_t phase)
{
    unsigned int expected = 0U;
    if (ctrl_hal.p_pwm_disable != NULL) ctrl_hal.p_pwm_disable();
    if ((fault != 0U) && (fault <= 255U) && (phase < 3U))
        (void)atomic_compare_exchange_strong(&fault_word, &expected, (unsigned int)((phase << 8U) | fault));
}
void npc_hal_hard_protect_clear(void) { atomic_store(&fault_word, 0U); }
uint8_t npc_hal_hard_protect_is_latched(void) { return (atomic_load(&fault_word) != 0U) ? 1U : 0U; }
uint32_t npc_hal_get_fault(void) { return atomic_load(&fault_word) & 255U; }
uint32_t npc_hal_get_fault_phase(void) { return atomic_load(&fault_word) >> 8U; }
