// SPDX-License-Identifier: MIT
/**
 * @file npc_hal.h
 * @brief Analog and PWM bindings, lifecycle hooks and immediate protection inhibition.
 * @details Base digital power framework. C11, static storage, no allocation.
 *          Bindings change only while stopped. PLECS serializes the dispatcher;
 *          ISR setpoints use the FSM publication snapshot.
 * @author Max.Li
 * @date 2026-09-13
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef NPC_HAL_H
#define NPC_HAL_H

#include <stdint.h>

/** 采样阶段写入、应用保护与控制共用的本拍快照。 */
typedef struct npc_hal_sample
{
    float v_out[3]; /* 输出相电压 A/B/C，V。 */
    float i_l[3];   /* 电感电流 A/B/C，A，桥臂流向输出为正。 */
    float v_dc_p;   /* 正侧母线电压，V。 */
    float v_dc_n;   /* 负侧母线电压幅值，V。 */
    float theta;    /* 本拍外部电角度，rad。 */
} npc_hal_sample_t;

/** @brief Capture bound inputs once after FSM INIT; dispatch serially before protection and control. */
void npc_hal_sample(void);

/** @return Read-only current-period sample; valid after npc_hal_sample completes. */
const npc_hal_sample_t *npc_hal_get_sample(void);

/** Analog sources and immediate PWM actions; all source objects outlive the binding. */
typedef struct npc_ctrl_hal
{
    float *p_v_out[3];   /* Present A/B/C phase voltages, V. */
    float *p_i_l[3];     /* Present A/B/C currents, positive from bridge to load, A. */
    float *p_v_dc_p;     /* Positive half-bus voltage, V. */
    float *p_v_dc_n;     /* Negative half-bus magnitude, V. */
    float *p_theta;      /* Present electrical angle, 0..M_2PI rad (my_math.h). */
    float *p_vd_pos_ref; /* Positive d-axis phase-voltage peak target, 0..1e6 V. */
    /**
     * @brief Apply this sample's stationary voltage command.
     * @param alpha Alpha-axis voltage command, V.
     * @param beta Beta-axis voltage command, V.
     * @param v_dc_p Positive half-bus voltage, V.
     * @param v_dc_n Negative half-bus magnitude, V.
     * @param p_current Same-update A/B/C fundamental currents for midpoint balancing, A.
     *        The callback consumes all three values synchronously; it must not retain this pointer.
     */
    void (*p_set_pwm_func)(float alpha, float beta, float v_dc_p, float v_dc_n, const float *p_current);
    void (*p_pwm_disable)(void); /* Disable all phases immediately. */
} npc_ctrl_hal_t;

/** Run entry/exit hooks; parameter initialization remains in the owning modules. */
typedef struct npc_fsm_hal
{
    void (*p_enter_run_func)(void); /* Prepare control state on entry to run. */
    void (*p_exit_run_func)(void);  /* Stop PWM and clear dynamics on exit from run. */
} npc_fsm_hal_t;

/** @return Static control bindings; callers must not modify the returned object. */
const npc_ctrl_hal_t *npc_hal_get_ctrl(void);

/** @return Static lifecycle hooks; callers must not modify the returned object. */
const npc_fsm_hal_t *npc_hal_get_fsm(void);

/**
 * @brief Check bindings only from the FSM INIT stage, before locking them for IDLE/RUN.
 * @return 1: every required binding is present; 0: a required binding is missing.
 */
uint8_t npc_hal_is_ready(void);

/** @brief Lock validated bindings before leaving INIT; retain the lock in IDLE/RUN. */
void npc_hal_lock_binding(void);

/** @brief Allow binding changes only during initialization with control dispatch stopped. */
void npc_hal_unlock_binding(void);

/**
 * @brief Bind one voltage source while unlocked; otherwise preserve the existing binding.
 * @param phase Phase index: 0=A, 1=B, 2=C; other indexes are ignored.
 * @param p_value Source voltage in V; NULL disconnects this source.
 */
void npc_hal_set_v_out_ptr(uint32_t phase, float *p_value);

/**
 * @brief Bind one current source while unlocked; otherwise preserve the existing binding.
 * @param phase Phase index: 0=A, 1=B, 2=C; other indexes are ignored.
 * @param p_value Source current in A; NULL disconnects this source.
 */
void npc_hal_set_i_l_ptr(uint32_t phase, float *p_value);

/**
 * @brief Bind the positive half-bus source while unlocked.
 * @param p_value Source voltage in V; NULL disconnects this source.
 */
void npc_hal_set_v_dc_p_ptr(float *p_value);

/**
 * @brief Bind the negative half-bus magnitude source while unlocked.
 * @param p_value Source magnitude in V; NULL disconnects this source.
 */
void npc_hal_set_v_dc_n_ptr(float *p_value);

/**
 * @brief Bind the present-sample phase source while unlocked.
 * @param p_value Electrical angle in rad, within 0..M_2PI (my_math.h); NULL disconnects the source.
 */
void npc_hal_set_theta_ptr(float *p_value);

/**
 * @brief Bind the external amplitude source while unlocked.
 * @param p_value Positive d-axis voltage peak in V, within 0..1e6; NULL disconnects the source.
 */
void npc_hal_set_vd_pos_ref_ptr(float *p_value);

/**
 * @brief Bind the voltage-command output callback while unlocked.
 * @param p_func Callback with the same units and input contract as p_set_pwm_func; NULL disconnects it.
 */
void npc_hal_set_pwm_setter(void (*p_func)(float alpha, float beta, float v_dc_p, float v_dc_n,
                                           const float *p_current));

/**
 * @brief Bind immediate PWM shutdown while unlocked.
 * @param p_func Shutdown callback; NULL disconnects it.
 */
void npc_hal_set_pwm_disable(void (*p_func)(void));

/**
 * @brief Bind the run-entry action while unlocked.
 * @param p_func Lifecycle callback; NULL disconnects it.
 */
void npc_hal_set_enter_run_func(void (*p_func)(void));

/**
 * @brief Bind the run-exit action while unlocked.
 * @param p_func Lifecycle callback; NULL disconnects it.
 */
void npc_hal_set_exit_run_func(void (*p_func)(void));

/**
 * @brief After FSM INIT validates bindings, disable PWM and atomically latch the first valid fault.
 * @param fault Fault code in 1..255; invalid codes still disable PWM but do not update the latch.
 * @param phase Phase index: 0=A, 1=B, 2=C; other indexes do not update the latch.
 */
void npc_hal_hard_protect_trip(uint32_t fault, uint32_t phase);

/** @brief Clear the fault latch only after the sampled-protection owner accepts recovery. */
void npc_hal_hard_protect_clear(void);

/** @return 1: fault latched; 0: no fault latched. */
uint8_t npc_hal_hard_protect_is_latched(void);

/** @return Latched fault code, or 0 when no fault is latched. */
uint32_t npc_hal_get_fault(void);

/** @return Latched phase index 0..2; meaningful only while a fault is latched. */
uint32_t npc_hal_get_fault_phase(void);

#endif /* NPC_HAL_H */
