// SPDX-License-Identifier: MIT
/**
 * @file npc_hal.c
 * @brief Own static hardware bindings and protection latch; supply default run actions.
 * @details Base digital power framework. C11, static storage, no allocation.
 *          Bindings change only while stopped. PLECS serializes the dispatcher;
 *          ISR setpoints use the FSM publication snapshot.
 * @author Max.Li
 * @date 2026-09-13
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#include "npc_hal.h"
#include "npc_ctrl.h"
#include "section.h"

#include <stddef.h>
#include <stdatomic.h>

static npc_ctrl_hal_t ctrl_hal        = {0}; /* Borrowed source pointers and immediate PWM actions. */
static npc_hal_sample_t sampled_input = {0}; /* Coherent input values shared by protection and control. */
static uint8_t binding_locked         = 1u;  /* Bindings may change only after the FSM unlocks them. */
static atomic_uint fault_word         = ATOMIC_VAR_INIT(0u); /* Atomic phase/code pair; zero means no fault. */

/** @brief Prepare fresh controller state on entry to run. */
static void npc_hal_enter_run(void)
{
    npc_ctrl_prepare_run();
}

/** @brief Stop PWM and clear dynamics on exit from run. */
static void npc_hal_exit_run(void)
{
    npc_ctrl_stop();
}

/* Default lifecycle actions, replaceable only while bindings are unlocked. */
static npc_fsm_hal_t fsm_hal = {.p_enter_run_func = npc_hal_enter_run, .p_exit_run_func = npc_hal_exit_run};

/** @brief Clear the fault latch without altering existing bindings. */
static void npc_hal_init(void)
{
    atomic_store(&fault_word, 0u);
}
REG_INIT(0, npc_hal_init)

const npc_ctrl_hal_t *npc_hal_get_ctrl(void)
{
    return &ctrl_hal;
}

const npc_fsm_hal_t *npc_hal_get_fsm(void)
{
    return &fsm_hal;
}

void npc_hal_lock_binding(void)
{
    binding_locked = 1u;
}

void npc_hal_unlock_binding(void)
{
    binding_locked = 0u;
}

uint8_t npc_hal_is_ready(void)
{
    for (uint32_t phase = 0u; phase < 3u; ++phase) /* Required phase source index. */
    {
        if (    (ctrl_hal.p_v_out[phase] == NULL)
             || /* Voltage feedback must be bound. */
                (ctrl_hal.p_i_l[phase] == NULL)) /* Current feedback must be bound. */
        {
            return 0u;
        }
    }

    return (    (ctrl_hal.p_v_dc_p != NULL)
             && /* Positive half-bus source. */
                (ctrl_hal.p_v_dc_n != NULL)
             && /* Negative half-bus source. */
                (ctrl_hal.p_theta != NULL)
             && /* Present-sample electrical angle. */
                (ctrl_hal.p_vd_pos_ref != NULL)
             && /* External amplitude target. */
                (ctrl_hal.p_set_pwm_func != NULL)
             && /* Voltage-command output path. */
                (ctrl_hal.p_pwm_disable != NULL)
             && /* Immediate shutdown path. */
                (fsm_hal.p_enter_run_func != NULL)
             && /* Run-entry action. */
                (fsm_hal.p_exit_run_func != NULL)) /* Run-exit action. */
             ? 1u
             : 0u;
}

void npc_hal_set_v_out_ptr(uint32_t phase, float *p_value)
{
    if (    (binding_locked == 0u)
         && /* Rebinding is allowed only while stopped. */
            (phase < 3u)) /* Keep the phase index within the source array. */
    {
        ctrl_hal.p_v_out[phase] = p_value;
    }
}

void npc_hal_set_i_l_ptr(uint32_t phase, float *p_value)
{
    if (    (binding_locked == 0u)
         && /* Rebinding is allowed only while stopped. */
            (phase < 3u)) /* Keep the phase index within the source array. */
    {
        ctrl_hal.p_i_l[phase] = p_value;
    }
}

void npc_hal_set_v_dc_p_ptr(float *p_value)
{
    if (binding_locked == 0u)
    {
        ctrl_hal.p_v_dc_p = p_value;
    }
}

void npc_hal_set_v_dc_n_ptr(float *p_value)
{
    if (binding_locked == 0u)
    {
        ctrl_hal.p_v_dc_n = p_value;
    }
}

void npc_hal_set_theta_ptr(float *p_value)
{
    if (binding_locked == 0u)
    {
        ctrl_hal.p_theta = p_value;
    }
}

void npc_hal_set_vd_pos_ref_ptr(float *p_value)
{
    if (binding_locked == 0u)
    {
        ctrl_hal.p_vd_pos_ref = p_value;
    }
}

void npc_hal_set_pwm_setter(void (*p_func)(float alpha, float beta, float v_dc_p, float v_dc_n,
                                           const float *p_current))
{
    if (binding_locked == 0u)
    {
        ctrl_hal.p_set_pwm_func = p_func;
    }
}

void npc_hal_set_pwm_disable(void (*p_func)(void))
{
    if (binding_locked == 0u)
    {
        ctrl_hal.p_pwm_disable = p_func;
    }
}

void npc_hal_set_enter_run_func(void (*p_func)(void))
{
    if (binding_locked == 0u)
    {
        fsm_hal.p_enter_run_func = p_func;
    }
}

void npc_hal_set_exit_run_func(void (*p_func)(void))
{
    if (binding_locked == 0u)
    {
        fsm_hal.p_exit_run_func = p_func;
    }
}

void npc_hal_hard_protect_trip(uint32_t fault, uint32_t phase)
{
    unsigned int expected = 0u; /* Required atomic_uint compare-exchange operand; accept only the first fault. */

    ctrl_hal.p_pwm_disable(); /* INIT has validated this callback; inhibit PWM before latching the fault. */

    if (    (fault != 0u)
         && /* Zero denotes an empty latch and cannot identify a fault. */
            (fault <= 255u)
         && /* The low byte carries the diagnostic code. */
            (phase < 3u)) /* The upper byte carries a valid phase index. */
    {
        (void)atomic_compare_exchange_strong(&fault_word, &expected, (unsigned int)((phase << 8u) | fault)); /* Preserve the first latched cause. */
    }
}

void npc_hal_hard_protect_clear(void)
{
    atomic_store(&fault_word, 0u);
}

uint8_t npc_hal_hard_protect_is_latched(void)
{
    return (atomic_load(&fault_word) != 0u) ? 1u : 0u;
}

uint32_t npc_hal_get_fault(void)
{
    return atomic_load(&fault_word) & 255u;
}

uint32_t npc_hal_get_fault_phase(void)
{
    return atomic_load(&fault_word) >> 8u;
}

void FUNC_RAM npc_hal_sample(void)
{
    for (uint32_t phase = 0u; phase < 3u; ++phase) /* Capture each phase once. */
    {
        sampled_input.v_out[phase] = *ctrl_hal.p_v_out[phase];
        sampled_input.i_l[phase]   = *ctrl_hal.p_i_l[phase];
    }
    sampled_input.v_dc_p = *ctrl_hal.p_v_dc_p; /* Positive half bus, V. */
    sampled_input.v_dc_n = *ctrl_hal.p_v_dc_n; /* Negative half-bus magnitude, V. */
    sampled_input.theta  = *ctrl_hal.p_theta;  /* Electrical angle, rad. */
}

const npc_hal_sample_t *npc_hal_get_sample(void)
{
    return &sampled_input;
}
