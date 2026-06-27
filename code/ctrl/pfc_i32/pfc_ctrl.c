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
#include "pfc_cfg.h"
#include "pi_tustin_i32.h"
#include "section.h"
#include <math.h>
#include <stddef.h>

#define p_hal (p_ctrl_hal)

static pi_tustin_i32_t vbus_volt_loop = {0};
static pi_tustin_i32_t ind_curr_loop = {0};

typedef struct
{
    int32_t b0;
    int32_t b2;
    int32_t a1;
    int32_t a2;
    int32_t qb0;
    int32_t qb1;
    int32_t qb2;
    int32_t u[3];
    int32_t osg_u[3];
    int32_t osg_qu[3];
    int32_t err;
    int32_t omega_mradps;
} pfc_i32_sogi_t;

typedef struct
{
    int32_t omega_mradps;
    int32_t i_err_mradps;
    int32_t gain_q;
} pfc_i32_fll_t;

static pfc_i32_ctrl_hal_t *p_ctrl_hal = NULL;
static pfc_i32_ctrl_setpoint_t pfc_i32_ctrl_safe_setpoint = {0};
static pfc_i32_ctrl_setpoint_t *p_ctrl_active_setpoint = &pfc_i32_ctrl_safe_setpoint;
static pfc_i32_sogi_t grid_sogi = {0};
static pfc_i32_fll_t grid_fll = {0};
static int32_t vbus_ref_ramped = 0;
static int32_t ind_curr_ref = 0;
static int32_t ind_curr_loop_out = 0;
static int32_t pfc_pwm_cmp = 0;
static uint8_t pfc_i32_ctrl_run_active = 0U;

static inline int32_t pfc_i32_ctrl_limit_i32(int32_t val, int32_t up_lmt, int32_t dn_lmt)
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

static inline int32_t pfc_i32_ctrl_sat_i64_to_i32(int64_t val)
{
    return pi_tustin_i32_sat_i64_to_i32(val);
}

static inline int32_t pfc_i32_ctrl_abs_i32(int32_t val)
{
    if (val < 0)
    {
        return (val == INT32_MIN) ? INT32_MAX : -val;
    }

    return val;
}

static inline int32_t pfc_i32_ctrl_float_to_i32(float val)
{
    if (val >= 0.0f)
    {
        return (int32_t)(val + 0.5f);
    }

    return (int32_t)(val - 0.5f);
}

static inline int32_t pfc_i32_ctrl_mul_q_i32(int32_t coeff_q, int32_t val)
{
    int64_t prod = 0;

    prod = (int64_t)coeff_q * (int64_t)val;

    return pfc_i32_ctrl_sat_i64_to_i32(prod >> PFC_I32_CTRL_SOGI_COEFF_Q_SHIFT);
}

static void pfc_i32_sogi_update_coeff(pfc_i32_sogi_t *p_sogi, int32_t omega_mradps)
{
    float ts = PFC_I32_CTRL_TS;
    float w = (float)omega_mradps * 0.001f;
    float k = PFC_I32_CTRL_SOGI_GAIN;
    float n0 = 0.0f;
    float n1 = 0.0f;
    float n2 = 0.0f;
    float d0 = 0.0f;
    float d1 = 0.0f;
    float d2 = 0.0f;

    if ((p_sogi == NULL) ||
        (ts <= 0.0f) ||
        (w <= 0.0f))
    {
        return;
    }

    n0 = 2.0f * ts * k * w;
    n2 = -2.0f * ts * k * w;
    d0 = ts * ts * w * w + 2.0f * ts * k * w + 4.0f;
    d1 = 2.0f * ts * ts * w * w - 8.0f;
    d2 = ts * ts * w * w - 2.0f * ts * k * w + 4.0f;

    p_sogi->b0 = pfc_i32_ctrl_float_to_i32((n0 / d0) * (float)PFC_I32_CTRL_SOGI_COEFF_Q);
    p_sogi->b2 = pfc_i32_ctrl_float_to_i32((n2 / d0) * (float)PFC_I32_CTRL_SOGI_COEFF_Q);
    p_sogi->a1 = pfc_i32_ctrl_float_to_i32((d1 / d0) * (float)PFC_I32_CTRL_SOGI_COEFF_Q);
    p_sogi->a2 = pfc_i32_ctrl_float_to_i32((d2 / d0) * (float)PFC_I32_CTRL_SOGI_COEFF_Q);

    n0 = ts * ts * k * w * w;
    n1 = 2.0f * ts * ts * k * w * w;
    n2 = ts * ts * k * w * w;

    p_sogi->qb0 = pfc_i32_ctrl_float_to_i32((n0 / d0) * (float)PFC_I32_CTRL_SOGI_COEFF_Q);
    p_sogi->qb1 = pfc_i32_ctrl_float_to_i32((n1 / d0) * (float)PFC_I32_CTRL_SOGI_COEFF_Q);
    p_sogi->qb2 = pfc_i32_ctrl_float_to_i32((n2 / d0) * (float)PFC_I32_CTRL_SOGI_COEFF_Q);
    p_sogi->omega_mradps = omega_mradps;
}

static void pfc_i32_sogi_reset(pfc_i32_sogi_t *p_sogi)
{
    if (p_sogi == NULL)
    {
        return;
    }

    p_sogi->u[0] = 0;
    p_sogi->u[1] = 0;
    p_sogi->u[2] = 0;
    p_sogi->osg_u[0] = 0;
    p_sogi->osg_u[1] = 0;
    p_sogi->osg_u[2] = 0;
    p_sogi->osg_qu[0] = 0;
    p_sogi->osg_qu[1] = 0;
    p_sogi->osg_qu[2] = 0;
    p_sogi->err = 0;
}

static void pfc_i32_sogi_init(pfc_i32_sogi_t *p_sogi, int32_t omega_mradps)
{
    pfc_i32_sogi_reset(p_sogi);
    pfc_i32_sogi_update_coeff(p_sogi, omega_mradps);
}

static inline void pfc_i32_sogi_cal(pfc_i32_sogi_t *p_sogi, int32_t input)
{
    int64_t osg_u = 0;
    int64_t osg_qu = 0;

    p_sogi->u[2] = p_sogi->u[1];
    p_sogi->u[1] = p_sogi->u[0];
    p_sogi->u[0] = input;

    osg_u = (int64_t)pfc_i32_ctrl_mul_q_i32(p_sogi->b0, p_sogi->u[0]) +
            (int64_t)pfc_i32_ctrl_mul_q_i32(p_sogi->b2, p_sogi->u[2]) -
            (int64_t)pfc_i32_ctrl_mul_q_i32(p_sogi->a1, p_sogi->osg_u[1]) -
            (int64_t)pfc_i32_ctrl_mul_q_i32(p_sogi->a2, p_sogi->osg_u[2]);

    osg_qu = (int64_t)pfc_i32_ctrl_mul_q_i32(p_sogi->qb0, p_sogi->u[0]) +
             (int64_t)pfc_i32_ctrl_mul_q_i32(p_sogi->qb1, p_sogi->u[1]) +
             (int64_t)pfc_i32_ctrl_mul_q_i32(p_sogi->qb2, p_sogi->u[2]) -
             (int64_t)pfc_i32_ctrl_mul_q_i32(p_sogi->a1, p_sogi->osg_qu[1]) -
             (int64_t)pfc_i32_ctrl_mul_q_i32(p_sogi->a2, p_sogi->osg_qu[2]);

    p_sogi->osg_u[0] = pfc_i32_ctrl_sat_i64_to_i32(osg_u);
    p_sogi->osg_qu[0] = pfc_i32_ctrl_sat_i64_to_i32(osg_qu);
    p_sogi->osg_u[2] = p_sogi->osg_u[1];
    p_sogi->osg_u[1] = p_sogi->osg_u[0];
    p_sogi->osg_qu[2] = p_sogi->osg_qu[1];
    p_sogi->osg_qu[1] = p_sogi->osg_qu[0];
    p_sogi->err = pfc_i32_ctrl_sat_i64_to_i32((int64_t)p_sogi->u[0] - (int64_t)p_sogi->osg_u[0]);
}

static void pfc_i32_fll_init(pfc_i32_fll_t *p_fll)
{
    if (p_fll == NULL)
    {
        return;
    }

    p_fll->omega_mradps = PFC_I32_CTRL_GRID_OMEGA_INIT_MRADPS;
    p_fll->i_err_mradps = 0;
    p_fll->gain_q = pfc_i32_ctrl_float_to_i32(PFC_I32_CTRL_FLL_GAIN *
                                              1.414f *
                                              PFC_I32_CTRL_TS *
                                              (float)PFC_I32_CTRL_FLL_GAIN_Q);
}

static inline void pfc_i32_fll_cal(pfc_i32_fll_t *p_fll, const pfc_i32_sogi_t *p_sogi)
{
    int64_t qvv = 0;
    int64_t err_qv_q = 0;
    int64_t omega_gain = 0;
    int64_t delta = 0;

    qvv = (int64_t)p_sogi->osg_u[0] * (int64_t)p_sogi->osg_u[0] +
          (int64_t)p_sogi->osg_qu[0] * (int64_t)p_sogi->osg_qu[0];
    if (qvv <= 0)
    {
        return;
    }

    err_qv_q = (((int64_t)p_sogi->err * (int64_t)p_sogi->osg_qu[0]) <<
                PFC_I32_CTRL_FLL_GAIN_Q_SHIFT) /
               qvv;
    omega_gain = ((int64_t)p_fll->omega_mradps * (int64_t)p_fll->gain_q) >>
                 PFC_I32_CTRL_FLL_GAIN_Q_SHIFT;
    delta = -((omega_gain * err_qv_q) >> PFC_I32_CTRL_FLL_GAIN_Q_SHIFT);

    p_fll->i_err_mradps = pfc_i32_ctrl_limit_i32(
        pfc_i32_ctrl_sat_i64_to_i32((int64_t)p_fll->i_err_mradps + delta),
        PFC_I32_CTRL_FLL_I_ERR_MAX_MRADPS,
        PFC_I32_CTRL_FLL_I_ERR_MIN_MRADPS);
    p_fll->omega_mradps = pfc_i32_ctrl_limit_i32(
        PFC_I32_CTRL_GRID_OMEGA_INIT_MRADPS + p_fll->i_err_mradps,
        PFC_I32_CTRL_GRID_OMEGA_MAX_MRADPS,
        PFC_I32_CTRL_GRID_OMEGA_MIN_MRADPS);
}

static inline void pfc_i32_ctrl_ramp_i32(int32_t *p_val, int32_t target, int32_t step)
{
    int32_t val = *p_val;

    if (step < 0)
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

static inline int32_t pfc_i32_ctrl_calc_slew_step(const pfc_i32_ctrl_setpoint_t *p_setpoint)
{
    uint32_t ctrl_freq_hz = pfc_i32_cfg_get_ctrl_freq_hz();
    int32_t step = 1;

    if (ctrl_freq_hz > 0U)
    {
        step = p_setpoint->vbus_slew_code_per_s / (int32_t)ctrl_freq_hz;
        if (step <= 0)
        {
            step = 1;
        }
    }

    return step;
}

static inline int32_t pfc_i32_ctrl_calc_ind_curr_ref(int32_t i_amp_k2, int32_t v_g, int32_t v_rms)
{
    int64_t numerator = 0;
    int64_t denominator = 0;

    if (v_rms <= 0)
    {
        return 0;
    }

    numerator = (int64_t)i_amp_k2 * (int64_t)v_g;
    denominator = (int64_t)v_rms * (int64_t)PFC_I32_CTRL_K2_CURR_REF_K;

    if (denominator == 0)
    {
        return 0;
    }

    return pfc_i32_ctrl_sat_i64_to_i32(numerator / denominator);
}

static inline int32_t pfc_i32_ctrl_calc_pwm_cmp(int32_t v_cap, int32_t v_bus, int32_t v_l_cmd)
{
    int64_t numerator = 0;
    int64_t cmp_k4 = 0;
    int64_t cmp = 0;

    if (v_bus <= 0)
    {
        return 0;
    }

    /*
     * v_l_cmd is in the K4 AC-voltage-code domain. v_cap uses signed
     * +/-400 V / 12-bit code, while v_bus uses 0..500 V / 12-bit code.
     */
    numerator = ((int64_t)v_cap * (int64_t)PFC_I32_CTRL_K4_AC_VOLT_CMD_K) -
                (int64_t)v_l_cmd;
    cmp_k4 = (numerator * (int64_t)PFC_I32_CTRL_BUS_VOLT_CODE_MAX) /
             ((int64_t)v_bus * (int64_t)PFC_I32_CTRL_AC_VOLT_CODE_MAX);
    cmp = (cmp_k4 * (int64_t)PFC_I32_CTRL_PWM_CMP_MAX) /
          (int64_t)PFC_I32_CTRL_K4_AC_VOLT_CMD_K;

    return pfc_i32_ctrl_limit_i32(pfc_i32_ctrl_sat_i64_to_i32(cmp),
                                  PFC_I32_CTRL_PWM_CMP_MAX,
                                  PFC_I32_CTRL_PWM_CMP_MIN);
}

static inline void pfc_i32_ctrl_reset_loops(void)
{
    pi_tustin_i32_reset_inline(&vbus_volt_loop);
    pi_tustin_i32_reset_inline(&ind_curr_loop);
    pfc_i32_sogi_reset(&grid_sogi);
    pfc_i32_fll_init(&grid_fll);
    ind_curr_ref = 0;
    ind_curr_loop_out = 0;
    pfc_pwm_cmp = 0;
}

static inline void pfc_i32_ctrl_force_safe_output(void)
{
    ind_curr_ref = 0;
    ind_curr_loop_out = 0;
    pfc_pwm_cmp = 0;

    if ((p_hal != NULL) &&
        (p_hal->p_pwm_disable != NULL))
    {
        p_hal->p_pwm_disable();
    }
}

static void pfc_i32_ctrl_reinit_states(void)
{
    pfc_i32_ctrl_setpoint_t *p_active_setpoint = NULL;

    p_ctrl_hal = pfc_i32_hal_get_ctrl();
    pfc_i32_cfg_sync_building_to_active();
    p_active_setpoint = pfc_i32_cfg_get_p_active();
    pfc_i32_ctrl_run_active = 0U;

    if ((p_hal == NULL) ||
        (pfc_i32_cfg_is_ready() == 0U) ||
        (pfc_i32_hal_is_ready() == 0U) ||
        (p_active_setpoint == NULL))
    {
        return;
    }

    p_ctrl_active_setpoint = p_active_setpoint;
    vbus_ref_ramped = *p_hal->p_v_bus;

    (void)pi_tustin_i32_init(&vbus_volt_loop,
                             PFC_I32_CTRL_VOLT_LOOP_KP,
                             PFC_I32_CTRL_VOLT_LOOP_KI,
                             PFC_I32_CTRL_TS,
                             PFC_I32_CTRL_VBUS_LOOP_OUT_MAX,
                             PFC_I32_CTRL_VBUS_LOOP_OUT_MIN,
                             &vbus_ref_ramped,
                             p_hal->p_v_bus);

    (void)pi_tustin_i32_init(&ind_curr_loop,
                             PFC_I32_CTRL_CURR_LOOP_KP,
                             PFC_I32_CTRL_CURR_LOOP_KI,
                             PFC_I32_CTRL_TS,
                             PFC_I32_CTRL_CURR_LOOP_OUT_MAX,
                             PFC_I32_CTRL_CURR_LOOP_OUT_MIN,
                             &ind_curr_ref,
                             p_hal->p_i_l);

    pfc_i32_sogi_init(&grid_sogi, PFC_I32_CTRL_GRID_OMEGA_INIT_MRADPS);
    pfc_i32_fll_init(&grid_fll);
    pfc_i32_ctrl_reset_loops();
}

static void pfc_i32_ctrl_init(void)
{
    pfc_i32_ctrl_reinit_states();
}

REG_INIT(0, pfc_i32_ctrl_init)

static void FUNC_RAM pfc_i32_ctrl_isr(void)
{
    pfc_i32_ctrl_hal_t *p_hal_isr = p_hal;
    pfc_i32_ctrl_setpoint_t *p_setpoint = p_ctrl_active_setpoint;
    int32_t i_amp_k2 = 0;
    int32_t v_g_sogi = 0;
    int32_t v_rms = 0;
    int32_t v_cap = 0;
    int32_t v_bus = 0;
    int32_t slew_step = 0;

    if ((p_hal_isr == NULL) ||
        (pfc_i32_cfg_is_ready() == 0U) ||
        (pfc_i32_hal_is_ready() == 0U) ||
        (p_setpoint == NULL))
    {
        return;
    }

    pfc_i32_cfg_sync_building_to_active();

    if ((p_setpoint->run_allowed == 0U) ||
        (*p_hal_isr->p_main_rly_is_closed == 0U))
    {
        if (pfc_i32_ctrl_run_active != 0U)
        {
            vbus_ref_ramped = *p_hal_isr->p_v_bus;
            pfc_i32_ctrl_reset_loops();
            pfc_i32_ctrl_force_safe_output();
            pfc_i32_ctrl_run_active = 0U;
        }
        return;
    }

    pfc_i32_ctrl_run_active = 1U;

    pfc_i32_sogi_cal(&grid_sogi, *p_hal_isr->p_v_g);
    pfc_i32_fll_cal(&grid_fll, &grid_sogi);
    v_g_sogi = grid_sogi.osg_u[0];
    v_rms = pfc_i32_ctrl_abs_i32(*p_hal_isr->p_v_rms);
    v_cap = *p_hal_isr->p_v_cap;
    v_bus = *p_hal_isr->p_v_bus;
    slew_step = pfc_i32_ctrl_calc_slew_step(p_setpoint);
    pfc_i32_ctrl_ramp_i32(&vbus_ref_ramped, p_setpoint->vbus_ref, slew_step);

    pi_tustin_i32_cal_a1_neg1_inline(&vbus_volt_loop);
    i_amp_k2 = vbus_volt_loop.output.val;
    if (i_amp_k2 < 0)
    {
        pi_tustin_i32_reset_inline(&vbus_volt_loop);
        i_amp_k2 = 0;
    }

    ind_curr_ref = pfc_i32_ctrl_calc_ind_curr_ref(i_amp_k2, v_g_sogi, v_rms);
    ind_curr_ref = pfc_i32_ctrl_limit_i32(ind_curr_ref,
                                            PFC_I32_CTRL_IND_CURR_CODE_MAX,
                                            PFC_I32_CTRL_IND_CURR_CODE_MIN);

    pi_tustin_i32_cal_a1_neg1_inline(&ind_curr_loop);
    ind_curr_loop_out = ind_curr_loop.output.val;
    pfc_pwm_cmp = pfc_i32_ctrl_calc_pwm_cmp(v_cap, v_bus, ind_curr_loop_out);

    p_hal_isr->p_set_pwm_func(pfc_pwm_cmp, 1U, 1U, 1U, 1U);
}

REG_INTERRUPT(3, pfc_i32_ctrl_isr)

static void pfc_i32_ctrl_task(void)
{
    static int32_t grid_omega_last_mradps = PFC_I32_CTRL_GRID_OMEGA_INIT_MRADPS;
    int32_t grid_omega_now_mradps = grid_fll.omega_mradps;
    int32_t omega_delta_mradps = grid_omega_now_mradps - grid_omega_last_mradps;

    if (omega_delta_mradps < 0)
    {
        omega_delta_mradps = -omega_delta_mradps;
    }

    if (omega_delta_mradps >= 1000)
    {
        pfc_i32_sogi_update_coeff(&grid_sogi, grid_omega_now_mradps);
        grid_omega_last_mradps = grid_omega_now_mradps;
    }
}

REG_TASK(1, pfc_i32_ctrl_task)

void pfc_i32_ctrl_set_p_hal(pfc_i32_ctrl_hal_t *p)
{
    (void)p;
}

void pfc_i32_ctrl_prepare_run(void)
{
    pfc_i32_ctrl_reinit_states();
}
