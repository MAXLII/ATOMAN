// SPDX-License-Identifier: MIT
/**
 * @file    npc_hal.h
 * @brief   Analog and PWM bindings, lifecycle hooks and immediate protection inhibition.
 * @details Base digital power framework. C11, static storage, no allocation.
 *          Bindings change only while stopped. PLECS serializes the dispatcher;
 *          ISR setpoints use the FSM publication snapshot.
 * @author  Max.Li
 * @date    2026-09-13
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef NPC_HAL_H
#define NPC_HAL_H
#include <stdint.h>
/* Analog bindings carry physical values at the current sample instant. */
typedef struct {
    float *p_v_out[3]; /* A/B/C phase voltages, V. */
    float *p_i_l[3]; /* A/B/C currents, bridge-to-load positive, A. */
    float *p_v_dc_p; /* Positive half bus, V. */
    float *p_v_dc_n; /* Negative half-bus magnitude, V. */
    float *p_theta; /* Present electrical angle, rad, 0..2*pi (Shell upper bound 6.283186). */
    float *p_vd_pos_ref; /* External positive d-axis phase-voltage peak target, V, 0..1e6. */
    /* Current voltage command -> modulation; zero succeeds, nonzero is platform status. */
    uint32_t (*p_set_pwm_func)(float alpha, float beta, float v_dc_p, float v_dc_n);
    void (*p_pwm_disable)(void); /* Disable all phases immediately. */
} npc_ctrl_hal_t;
/* Lifecycle actions, not sampling or parameter initialization. */
typedef struct {
    void (*p_enter_run_func)(void);
    void (*p_exit_run_func)(void);
} npc_fsm_hal_t;
const npc_ctrl_hal_t *npc_hal_get_ctrl(void);
const npc_fsm_hal_t *npc_hal_get_fsm(void);
uint8_t npc_hal_is_ready(void);
void npc_hal_lock_binding(void);
void npc_hal_unlock_binding(void);
void npc_hal_set_v_out_ptr(uint32_t phase, float *p_value);
void npc_hal_set_i_l_ptr(uint32_t phase, float *p_value);
void npc_hal_set_v_dc_p_ptr(float *p_value);
void npc_hal_set_v_dc_n_ptr(float *p_value);
void npc_hal_set_theta_ptr(float *p_value);
void npc_hal_set_vd_pos_ref_ptr(float *p_value);
void npc_hal_set_pwm_setter(uint32_t (*p_func)(float, float, float, float));
void npc_hal_set_pwm_disable(void (*p_func)(void));
void npc_hal_set_enter_run_func(void (*p_func)(void));
void npc_hal_set_exit_run_func(void (*p_func)(void));
/* Trip disables immediately. Clearing is the sampled protection owner's decision. */
void npc_hal_hard_protect_trip(uint32_t fault, uint32_t phase);
void npc_hal_hard_protect_clear(void);
uint8_t npc_hal_hard_protect_is_latched(void);
uint32_t npc_hal_get_fault(void);
uint32_t npc_hal_get_fault_phase(void);
#endif
