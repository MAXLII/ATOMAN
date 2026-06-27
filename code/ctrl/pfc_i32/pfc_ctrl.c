// SPDX-License-Identifier: MIT
/**
 * @file    pfc_ctrl.c
 * @brief   PFC int32 controller module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement PFC DC-bus voltage and inductor-current control in int32_t domains
 *          - Convert the voltage-loop output to a grid-shaped current reference without floats in ISR
 *          - Generate signed PWM compare commands from integer voltage and current feedback codes
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
#include "pfc_ctrl.h"
#include "fll_i32.h"
#include "pfc_cfg.h"
#include "pi_tustin.h"
#include "pi_tustin_i32.h"
#include "section.h"
#include "sogi_i32.h"
#include <limits.h>
#include <stddef.h>

#define p_hal (p_ctrl_hal)

static pi_tustin_t vbus_volt_loop = {0};
static pi_tustin_i32_t ind_curr_loop = {0};

static pfc_ctrl_hal_t *p_ctrl_hal = NULL;
static pfc_ctrl_setpoint_t pfc_ctrl_safe_setpoint = {0};
static pfc_ctrl_setpoint_t *p_ctrl_active_setpoint = &pfc_ctrl_safe_setpoint;
static sogi_i32_t grid_sogi = {0};
static fll_i32_t grid_fll = {0};
static int32_t grid_sogi_input = 0;
static int32_t grid_fll_input_u = 0;
static int32_t grid_fll_input_qu = 0;
static int32_t grid_fll_input_err = 0;
static int32_t grid_omega_last_mradps = PFC_CTRL_GRID_OMEGA_INIT_MRADPS;
static volatile uint32_t grid_sogi_snapshot_seq = 0U;
static volatile int32_t grid_sogi_snapshot_u = 0;
static volatile int32_t grid_sogi_snapshot_qu = 0;
static volatile int32_t grid_sogi_snapshot_err = 0;
static volatile int32_t grid_omega_pending_mradps = PFC_CTRL_GRID_OMEGA_INIT_MRADPS;
static int32_t vbus_loop_i_amp = 0;
static float vbus_ref_ramped = 0.0f;
static float vbus_feedback = 0.0f;
static int8_t grid_zero_last_sign = 0;
static int32_t ind_curr_ref = 0;
static int32_t ind_curr_loop_out = 0;
static int32_t pfc_pwm_cmd = 0;
static uint8_t pfc_ctrl_run_active = 0U;

static inline int32_t pfc_ctrl_limit_i32(int32_t val, int32_t up_lmt, int32_t dn_lmt)
{
    if (val > up_lmt)
    {
        val = up_lmt;
    }
    else if (val < dn_lmt)
    {
        val = dn_lmt;
    }

    return val;
}

static inline int32_t pfc_ctrl_sat_i64_to_i32(int64_t val)
{
    return pi_tustin_i32_sat_i64_to_i32(val);
}

static inline int32_t pfc_ctrl_float_to_i32(float val)
{
    if (val >= (float)INT32_MAX)
    {
        return INT32_MAX;
    }
    else if (val <= (float)INT32_MIN)
    {
        return INT32_MIN;
    }

    if (val >= 0.0f)
    {
        return (int32_t)(val + 0.5f);
    }

    return (int32_t)(val - 0.5f);
}

static inline void pfc_ctrl_ramp_float(float *p_val, float target, float step)
{
    float val = *p_val;

    if (step < 0.0f)
    {
        step = -step;
    }

    if (val < target)
    {
        val += step;
        if (val > target)
        {
            val = target;
        }
    }
    else if (val > target)
    {
        val -= step;
        if (val < target)
        {
            val = target;
        }
    }

    *p_val = val;
}

static inline float pfc_ctrl_calc_vbus_slew_step(const pfc_ctrl_setpoint_t *p_setpoint)
{
    float step = (float)p_setpoint->vbus_slew_code_per_s * PFC_CTRL_VOLT_LOOP_TS;

    if (step < 1.0f)
    {
        step = 1.0f;
    }

    return step;
}

static inline uint8_t pfc_ctrl_grid_zero_cross_update(int32_t v_g)
{
    int8_t sign = 0;
    uint8_t is_cross = 0U;

    if (v_g > 0)
    {
        sign = 1;
    }
    else if (v_g < 0)
    {
        sign = -1;
    }
    else
    {
        return 0U;
    }

    if ((grid_zero_last_sign != 0) &&
        (sign != grid_zero_last_sign))
    {
        is_cross = 1U;
    }

    grid_zero_last_sign = sign;

    return is_cross;
}

static inline int32_t pfc_ctrl_calc_ind_curr_ref(int32_t i_amp, int32_t v_g, int32_t v_rms)
{
    int64_t numerator = 0;

    if (v_rms <= 0)
    {
        return 0;
    }

    numerator = (int64_t)i_amp *
                (int64_t)v_g *
                (int64_t)PFC_CTRL_GRID_RMS_NOMINAL_CODE;

    return pfc_ctrl_sat_i64_to_i32(numerator / ((int64_t)v_rms * (int64_t)v_rms));
}

static inline int32_t pfc_ctrl_calc_pwm_cmd(int32_t v_cap, int32_t v_l_cmd)
{
    int64_t numerator = 0;

    /*
     * Match ctrl/pfc modulation: v_pwm = v_cap - v_l_cmd. In the int32 path
     * this node is kept in AC-voltage-code * PWM-reload domain. The app layer
     * divides by the bus-voltage feedback and converts the result to duty.
     */
    numerator = ((int64_t)v_cap * (int64_t)PFC_CTRL_PWM_CMP_MAX) -
                (int64_t)v_l_cmd;

    return pfc_ctrl_sat_i64_to_i32(numerator);
}

static inline void pfc_ctrl_publish_sogi_snapshot(void)
{
    grid_sogi_snapshot_seq++;
    grid_sogi_snapshot_u = grid_sogi.output.u;
    grid_sogi_snapshot_qu = grid_sogi.output.qu;
    grid_sogi_snapshot_err = grid_sogi.output.err;
    grid_sogi_snapshot_seq++;
}

static inline uint8_t pfc_ctrl_read_sogi_snapshot(void)
{
    uint32_t seq_before = 0U;
    uint32_t seq_after = 0U;

    do
    {
        seq_before = grid_sogi_snapshot_seq;
        grid_fll_input_u = grid_sogi_snapshot_u;
        grid_fll_input_qu = grid_sogi_snapshot_qu;
        grid_fll_input_err = grid_sogi_snapshot_err;
        seq_after = grid_sogi_snapshot_seq;
    } while ((seq_before != seq_after) || ((seq_after & 1U) != 0U));

    return (uint8_t)(seq_after != 0U);
}

static inline void pfc_ctrl_update_sogi_omega(int32_t omega_mradps)
{
    (void)sogi_i32_update(&grid_sogi,
                          PFC_CTRL_TS,
                          (float)omega_mradps / 1000.0f,
                          PFC_CTRL_SOGI_GAIN,
                          PFC_CTRL_SOGI_COEFF_Q_SHIFT);
    grid_omega_last_mradps = omega_mradps;
}

static inline void pfc_ctrl_apply_pending_sogi_omega(void)
{
    int32_t omega_mradps = grid_omega_pending_mradps;

    if (omega_mradps != grid_omega_last_mradps)
    {
        pfc_ctrl_update_sogi_omega(omega_mradps);
    }
}

static void pfc_ctrl_init_observer(void)
{
    (void)sogi_i32_init(&grid_sogi,
                        PFC_CTRL_TS,
                        (float)PFC_CTRL_GRID_OMEGA_INIT_MRADPS / 1000.0f,
                        PFC_CTRL_SOGI_GAIN,
                        PFC_CTRL_SOGI_COEFF_Q_SHIFT,
                        &grid_sogi_input);
    (void)fll_i32_init(&grid_fll,
                       PFC_CTRL_FLL_GAIN,
                       PFC_CTRL_TS,
                       PFC_CTRL_GRID_OMEGA_INIT_MRADPS,
                       PFC_CTRL_GRID_OMEGA_MAX_MRADPS,
                       PFC_CTRL_GRID_OMEGA_MIN_MRADPS,
                       PFC_CTRL_FLL_I_ERR_MAX_MRADPS,
                       PFC_CTRL_FLL_I_ERR_MIN_MRADPS,
                       PFC_CTRL_FLL_GAIN_Q_SHIFT,
                       &grid_fll_input_u,
                       &grid_fll_input_qu,
                       &grid_fll_input_err);
    grid_omega_last_mradps = PFC_CTRL_GRID_OMEGA_INIT_MRADPS;
    grid_omega_pending_mradps = PFC_CTRL_GRID_OMEGA_INIT_MRADPS;
}

static inline void pfc_ctrl_reset_loops(void)
{
    pi_tustin_reset(&vbus_volt_loop);
    pi_tustin_i32_reset_inline(&ind_curr_loop);
    sogi_i32_reset(&grid_sogi);
    fll_i32_reset(&grid_fll);
    pfc_ctrl_update_sogi_omega(PFC_CTRL_GRID_OMEGA_INIT_MRADPS);
    grid_omega_pending_mradps = PFC_CTRL_GRID_OMEGA_INIT_MRADPS;
    pfc_ctrl_publish_sogi_snapshot();
    grid_zero_last_sign = 0;
    vbus_loop_i_amp = 0;
    ind_curr_ref = 0;
    ind_curr_loop_out = 0;
    pfc_pwm_cmd = 0;
}

static inline void pfc_ctrl_force_safe_output(void)
{
    if ((p_hal != NULL) &&
        (p_hal->p_pwm_disable != NULL))
    {
        p_hal->p_pwm_disable();
    }
}

static inline int32_t pfc_ctrl_run_vbus_loop(pfc_ctrl_hal_t *p_hal_task,
                                             pfc_ctrl_setpoint_t *p_setpoint);

static void pfc_ctrl_reinit_states(void)
{
    pfc_ctrl_setpoint_t *p_active_setpoint = NULL;

    p_ctrl_hal = pfc_hal_get_ctrl();
    pfc_cfg_sync_building_to_active();
    p_active_setpoint = pfc_cfg_get_p_active();
    pfc_ctrl_run_active = 0U;

    if ((p_hal == NULL) ||
        (pfc_cfg_is_ready() == 0U) ||
        (pfc_hal_is_ready() == 0U) ||
        (p_active_setpoint == NULL))
    {
        return;
    }

    p_ctrl_active_setpoint = p_active_setpoint;
    vbus_feedback = (float)*p_hal->p_v_bus;
    vbus_ref_ramped = vbus_feedback;

    (void)pi_tustin_init(&vbus_volt_loop,
                         PFC_CTRL_VOLT_LOOP_KP,
                         PFC_CTRL_VOLT_LOOP_KI,
                         PFC_CTRL_VOLT_LOOP_TS,
                         (float)PFC_CTRL_VBUS_LOOP_OUT_MAX,
                         (float)PFC_CTRL_VBUS_LOOP_OUT_MIN,
                         &vbus_ref_ramped,
                         &vbus_feedback);

    (void)pi_tustin_i32_init(&ind_curr_loop,
                             PFC_CTRL_CURR_LOOP_KP,
                             PFC_CTRL_CURR_LOOP_KI,
                             PFC_CTRL_TS,
                             PFC_CTRL_CURR_LOOP_OUT_MAX,
                             PFC_CTRL_CURR_LOOP_OUT_MIN,
                             &ind_curr_ref,
                             p_hal->p_i_l);

    pfc_ctrl_init_observer();
    pfc_ctrl_reset_loops();
}

static void pfc_ctrl_init(void)
{
    pfc_ctrl_reinit_states();
}

REG_INIT(0, pfc_ctrl_init)

static void FUNC_RAM pfc_ctrl_isr(void)
{
    pfc_ctrl_hal_t *p_hal_isr = p_hal;
    pfc_ctrl_setpoint_t *p_setpoint = p_ctrl_active_setpoint;

    if ((p_hal_isr == NULL) ||
        (p_setpoint == NULL))
    {
        return;
    }

    if ((p_setpoint->run_allowed == 0U) ||
        (*p_hal_isr->p_main_rly_is_closed == 0U))
    {
        if (pfc_ctrl_run_active != 0U)
        {
            vbus_ref_ramped = (float)*p_hal_isr->p_v_bus;
            pfc_ctrl_reset_loops();
            pfc_ctrl_force_safe_output();
            pfc_ctrl_run_active = 0U;
        }
        return;
    }

    pfc_ctrl_run_active = 1U;

    pfc_ctrl_apply_pending_sogi_omega();
    grid_sogi_input = *p_hal_isr->p_v_g;
    (void)sogi_i32_cal(&grid_sogi);
    pfc_ctrl_publish_sogi_snapshot();

    if (pfc_ctrl_grid_zero_cross_update(grid_sogi_input) != 0U)
    {
        vbus_loop_i_amp = pfc_ctrl_run_vbus_loop(p_hal_isr, p_setpoint);
    }

    ind_curr_ref = pfc_ctrl_calc_ind_curr_ref(vbus_loop_i_amp,
                                              grid_sogi.output.u,
                                              *p_hal_isr->p_v_rms);
    ind_curr_ref = pfc_ctrl_limit_i32(ind_curr_ref,
                                      PFC_CTRL_IND_CURR_CODE_MAX,
                                      PFC_CTRL_IND_CURR_CODE_MIN);

    pi_tustin_i32_cal_a1_neg1_inline(&ind_curr_loop);
    ind_curr_loop_out = ind_curr_loop.output.val;
    pfc_pwm_cmd = pfc_ctrl_calc_pwm_cmd(*p_hal_isr->p_v_cap,
                                        ind_curr_loop_out);

    p_hal_isr->p_set_pwm_func(pfc_pwm_cmd, *p_hal_isr->p_v_bus);
}

REG_INTERRUPT(3, pfc_ctrl_isr)

static inline int32_t pfc_ctrl_run_vbus_loop(pfc_ctrl_hal_t *p_hal_task,
                                             pfc_ctrl_setpoint_t *p_setpoint)
{
    vbus_feedback = (float)*p_hal_task->p_v_bus;
    pfc_ctrl_ramp_float(&vbus_ref_ramped,
                        (float)p_setpoint->vbus_ref,
                        pfc_ctrl_calc_vbus_slew_step(p_setpoint));

    (void)pi_tustin_cal(&vbus_volt_loop);
    if (vbus_volt_loop.output.val < 0.0f)
    {
        pi_tustin_reset(&vbus_volt_loop);
        vbus_volt_loop.output.val = 0.0f;
    }

    return pfc_ctrl_float_to_i32(vbus_volt_loop.output.val);
}

static void pfc_ctrl_task(void)
{
    int32_t grid_omega_now_mradps = 0;
    int32_t omega_delta_mradps = 0;

    if (pfc_ctrl_read_sogi_snapshot() == 0U)
    {
        return;
    }

    (void)fll_i32_cal(&grid_fll);
    grid_omega_now_mradps = grid_fll.output.omega_mradps;
    omega_delta_mradps = grid_omega_now_mradps - grid_omega_pending_mradps;

    if (omega_delta_mradps < 0)
    {
        omega_delta_mradps = -omega_delta_mradps;
    }

    if (omega_delta_mradps >= 1000)
    {
        grid_omega_pending_mradps = grid_omega_now_mradps;
    }
}

REG_TASK(1, pfc_ctrl_task)

void pfc_ctrl_set_p_hal(pfc_ctrl_hal_t *p)
{
    (void)p;
}

void pfc_ctrl_prepare_run(void)
{
    pfc_ctrl_reinit_states();
}
