// SPDX-License-Identifier: MIT
/**
 * @file    inv_ctrl.c
 * @brief   inv_ctrl control module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement the inverter SRFPI voltage loop and capacitor-current active-damping loop
 *          - Prepare controller state and references for closed-loop or test-mode operation
 *          - Consume HAL measurements and setpoints to produce inverter PWM modulation commands
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "inv_ctrl.h"
#include "inv_cfg.h"
#include "apf.h"
#include "pi_tustin.h"
#include "resonant.h"
#include "section.h"
#include <stddef.h>

#define p_hal (p_ctrl_hal)

static pi_tustin_t volt_loop_d = {0};
static pi_tustin_t volt_loop_q = {0};
static apf_t volt_apf = {0};
static resonant_t harmonic_3 = {0};
static resonant_t harmonic_5 = {0};
static resonant_t harmonic_7 = {0};

static float v_d_ref = 0.0f;
static float v_d_act = 0.0f;
static float v_q_ref = 0.0f;
static float v_q_act = 0.0f;

static float i_cap_d_ref = 0.0f;
static float i_cap_q_ref = 0.0f;
static float i_cap_ref = 0.0f;
static float i_cap_act = 0.0f;
static float i_cap_inner_out = 0.0f;
static float harmonic_comp = 0.0f;
static float v_cap_beta = 0.0f;
static float v_cap_last = 0.0f;
static float v_ref = 0.0f;
static float omega = 0.0f;

static inv_ctrl_hal_t *p_ctrl_hal = NULL;
static inv_ctrl_setpoint_t inv_ctrl_safe_setpoint = {0};
static inv_ctrl_setpoint_t *p_ctrl_active_setpoint = &inv_ctrl_safe_setpoint;
static float v_cap_fb = 0.0f;
static float i_l_fb = 0.0f;
static float v_bus_fb = 0.0f;

static float sintheta = 0.0f;
static float costheta = 1.0f;
static float phase_pu = 0.0f;
static float freq_hz_ramped = 0.0f;
static float v_ref_pk = 0.0f;
static float v_ref_pk_tag = 0.0f;
static float vpwm = 0.0f;
static uint8_t inv_ctrl_loops_ready = 0U;
static uint8_t inv_ctrl_run_active = 0U;
static uint8_t inv_ctrl_first_run_cycle = 0U;

static inline void inv_ctrl_update_feedback(inv_ctrl_hal_t *p)
{
    v_cap_fb = *p->p_v_cap;
    i_l_fb = *p->p_i_l;
    v_bus_fb = *p->p_v_bus;
}

static float inv_ctrl_limit_freq_hz(float freq_hz)
{
    float ctrl_freq = inv_cfg_get_ctrl_freq();

    if (freq_hz < 1.0f)
    {
        return 1.0f;
    }

    if (freq_hz > (ctrl_freq / 4.0f))
    {
        return ctrl_freq / 4.0f;
    }

    return freq_hz;
}

static bool inv_ctrl_update_filter_frequency(float freq_hz)
{
    float freq_hz_limited = inv_ctrl_limit_freq_hz(freq_hz);
    float omega_new = M_2PI * freq_hz_limited;
    bool update_ok = true;

    if (omega_new != omega)
    {
        omega = omega_new;
        update_ok = apf_update_frequency(&volt_apf, omega);
        update_ok = resonant_update_frequency(&harmonic_3, omega) && update_ok;
        update_ok = resonant_update_frequency(&harmonic_5, omega) && update_ok;
        update_ok = resonant_update_frequency(&harmonic_7, omega) && update_ok;
    }

    return update_ok;
}

static void inv_ctrl_reset_loops(void)
{
    pi_tustin_reset(&volt_loop_d);
    pi_tustin_reset(&volt_loop_q);
    apf_reset(&volt_apf);
    resonant_reset(&harmonic_3);
    resonant_reset(&harmonic_5);
    resonant_reset(&harmonic_7);
    i_cap_d_ref = 0.0f;
    i_cap_q_ref = 0.0f;
    i_cap_ref = 0.0f;
    i_cap_act = 0.0f;
    i_cap_inner_out = 0.0f;
    harmonic_comp = 0.0f;
    v_cap_beta = 0.0f;
    v_cap_last = v_cap_fb;
    v_ref = 0.0f;
    vpwm = 0.0f;
}

static void inv_ctrl_reinit_states(void)
{
    inv_ctrl_setpoint_t *p_active_setpoint = NULL;
    float ctrl_ts = inv_cfg_get_ctrl_ts();
    bool init_ok = true;

    p_ctrl_hal = inv_hal_get_ctrl();
    inv_ctrl_loops_ready = 0U;
    inv_ctrl_run_active = 0U;
    inv_ctrl_first_run_cycle = 1U;
    inv_cfg_sync_building_to_active();
    p_active_setpoint = inv_cfg_get_p_active();

    if ((p_hal == NULL) ||
        (inv_cfg_is_ready() == 0U) ||
        (p_active_setpoint == NULL) ||
        (p_hal->p_v_cap == NULL) ||
        (p_hal->p_i_l == NULL) ||
        (p_hal->p_v_bus == NULL))
    {
        PLECS_LOG("inv_ctrl reinit skipped: hal invalid\n");
        return;
    }

    p_ctrl_active_setpoint = p_active_setpoint;
    inv_ctrl_update_feedback(p_hal);

    if (pi_tustin_init(&volt_loop_d,
                       INV_CTRL_VOLT_LOOP_KP,
                       INV_CTRL_VOLT_LOOP_KI,
                       ctrl_ts,
                       INV_CTRL_VOLT_LOOP_OUT_MAX_A,
                       INV_CTRL_VOLT_LOOP_OUT_MIN_A,
                       &v_d_ref,
                       &v_d_act) == false)
    {
        init_ok = false;
    }
    if (pi_tustin_init(&volt_loop_q,
                       INV_CTRL_VOLT_LOOP_KP,
                       INV_CTRL_VOLT_LOOP_KI,
                       ctrl_ts,
                       INV_CTRL_VOLT_LOOP_OUT_MAX_A,
                       INV_CTRL_VOLT_LOOP_OUT_MIN_A,
                       &v_q_ref,
                       &v_q_act) == false)
    {
        init_ok = false;
    }
    phase_pu = 0.0f;
    freq_hz_ramped = inv_ctrl_limit_freq_hz(p_active_setpoint->freq_hz);
    omega = M_2PI * freq_hz_ramped;

    if (apf_init(&volt_apf, omega, ctrl_ts) == false)
    {
        init_ok = false;
    }
    if (resonant_init(&harmonic_3,
                      INV_CTRL_HARMONIC_3_GAIN,
                      INV_CTRL_HARMONIC_3_ORDER,
                      omega,
                      ctrl_ts) == false)
    {
        init_ok = false;
    }
    if (resonant_init(&harmonic_5,
                      INV_CTRL_HARMONIC_5_GAIN,
                      INV_CTRL_HARMONIC_5_ORDER,
                      omega,
                      ctrl_ts) == false)
    {
        init_ok = false;
    }
    if (resonant_init(&harmonic_7,
                      INV_CTRL_HARMONIC_7_GAIN,
                      INV_CTRL_HARMONIC_7_ORDER,
                      omega,
                      ctrl_ts) == false)
    {
        init_ok = false;
    }

    if (init_ok == false)
    {
        PLECS_LOG("inv_ctrl reinit skipped: PI init failed\n");
        return;
    }

    sintheta = 0.0f;
    costheta = 1.0f;
    v_ref_pk = 0.0f;
    v_ref_pk_tag = p_active_setpoint->rms_ref_v * M_SQRT2;
    vpwm = 0.0f;

    inv_ctrl_reset_loops();
    inv_ctrl_loops_ready = 1U;
    PLECS_LOG("inv_ctrl reinit done: freq=%.3f freq_slew=%.3f rms=%.3f rms_slew=%.3f\n",
              p_active_setpoint->freq_hz,
              p_active_setpoint->freq_slew_hzps,
              p_active_setpoint->rms_ref_v,
              p_active_setpoint->rms_slew_vps);
}

static void inv_ctrl_init(void)
{
    PLECS_LOG("inv_ctrl init\n");
    inv_ctrl_reinit_states();
}

REG_INIT(0, inv_ctrl_init)

static void inv_ctrl_cal_theta(void)
{
    if (freq_hz_ramped <= 0.0f)
    {
        return;
    }

    sintheta = sinf(phase_pu * 2.0f * M_PI);
    costheta = cosf(phase_pu * 2.0f * M_PI);
}

REG_INTERRUPT(0, inv_ctrl_cal_theta)

static void inv_ctrl_isr(void)
{
    inv_ctrl_hal_t *p_hal_isr = p_hal;
    inv_ctrl_setpoint_t *p_setpoint = p_ctrl_active_setpoint;

    if ((p_hal_isr == NULL) ||
        (inv_cfg_is_ready() == 0U) ||
        (inv_ctrl_loops_ready == 0U) ||
        (p_setpoint == NULL) ||
        (p_hal_isr->p_v_cap == NULL) ||
        (p_hal_isr->p_i_l == NULL) ||
        (p_hal_isr->p_v_bus == NULL) ||
        (p_hal_isr->p_set_pwm_func == NULL) ||
        (p_hal_isr->p_pwm_disable == NULL))
    {
        return;
    }

    inv_cfg_sync_building_to_active();
    inv_ctrl_update_feedback(p_hal_isr);

    if (p_setpoint->run_allowed == 0U)
    {
        if (inv_ctrl_run_active != 0U)
        {
            p_hal_isr->p_pwm_disable();
            inv_ctrl_reset_loops();
            inv_ctrl_run_active = 0U;
            inv_ctrl_first_run_cycle = 1U;
        }
        return;
    }

    if (inv_ctrl_run_active == 0U)
    {
        inv_ctrl_first_run_cycle = 1U;
    }

    inv_ctrl_run_active = 1U;

    if (inv_ctrl_first_run_cycle != 0U)
    {
        freq_hz_ramped = inv_ctrl_limit_freq_hz(p_setpoint->freq_hz);
        inv_ctrl_first_run_cycle = 0U;
    }
    else if (p_setpoint->freq_slew_hzps > 0.0f)
    {
        RAMP(freq_hz_ramped,
             p_setpoint->freq_hz,
             p_setpoint->freq_slew_hzps * inv_cfg_get_ctrl_ts());
    }
    else
    {
        freq_hz_ramped = p_setpoint->freq_hz;
    }
    freq_hz_ramped = inv_ctrl_limit_freq_hz(freq_hz_ramped);
    if (inv_ctrl_update_filter_frequency(freq_hz_ramped) == false)
    {
        p_hal_isr->p_pwm_disable();
        inv_ctrl_loops_ready = 0U;
        return;
    }

    v_ref_pk_tag = p_setpoint->rms_ref_v * M_SQRT2;
    RAMP(v_ref_pk, v_ref_pk_tag, p_setpoint->rms_slew_vps * M_SQRT2 * inv_cfg_get_ctrl_ts());

    v_d_ref = v_ref_pk;
    v_q_ref = 0.0f;
    v_ref = v_ref_pk * costheta;

    v_cap_beta = apf_cal(&volt_apf, v_cap_fb);
    DQ_CAL(v_cap_fb,
           v_cap_beta,
           sintheta,
           costheta,
           v_d_act,
           v_q_act);

    (void)pi_tustin_cal(&volt_loop_d);
    (void)pi_tustin_cal(&volt_loop_q);
    i_cap_d_ref = volt_loop_d.output.val;
    i_cap_q_ref = volt_loop_q.output.val;
    i_cap_ref = (costheta * i_cap_d_ref) - (sintheta * i_cap_q_ref);

    i_cap_act = HW_AC_SIDE_CAP_VALUE * (v_cap_fb - v_cap_last) / inv_cfg_get_ctrl_ts();
    v_cap_last = v_cap_fb;

    harmonic_comp = resonant_cal(&harmonic_3, v_cap_fb) +
                    resonant_cal(&harmonic_5, v_cap_fb) +
                    resonant_cal(&harmonic_7, v_cap_fb);
    UP_DN_LMT(harmonic_comp, INV_CTRL_HARMONIC_OUT_MAX_A, INV_CTRL_HARMONIC_OUT_MIN_A);

    i_cap_inner_out = INV_CTRL_INNER_LOOP_K * (i_cap_ref - i_cap_act - harmonic_comp);
    vpwm = v_ref + i_cap_inner_out;
    if (v_bus_fb > 0.0f)
    {
        UP_DN_LMT(vpwm, v_bus_fb, -v_bus_fb);
    }

#ifdef IS_PLECS
    plecs_set_output((PLECS_OUTPUT_E)(PLECS_OUTPUT_DBG + 0), v_d_ref);                /* D 轴电压给定，DLL:9。 */
    plecs_set_output((PLECS_OUTPUT_E)(PLECS_OUTPUT_DBG + 1), v_d_act);                /* D 轴电压反馈，DLL:10。 */
    plecs_set_output((PLECS_OUTPUT_E)(PLECS_OUTPUT_DBG + 2), volt_loop_d.output.val); /* D 轴电压环输出，DLL:11。 */
    plecs_set_output((PLECS_OUTPUT_E)(PLECS_OUTPUT_DBG + 3), v_q_ref);                /* Q 轴电压给定，DLL:12。 */
    plecs_set_output((PLECS_OUTPUT_E)(PLECS_OUTPUT_DBG + 4), v_q_act);                /* Q 轴电压反馈，DLL:13。 */
    plecs_set_output((PLECS_OUTPUT_E)(PLECS_OUTPUT_DBG + 5), volt_loop_q.output.val); /* Q 轴电压环输出，DLL:14。 */
    plecs_set_output((PLECS_OUTPUT_E)(PLECS_OUTPUT_DBG + 6), i_cap_ref);              /* 电容电流给定，DLL:15。 */
    plecs_set_output((PLECS_OUTPUT_E)(PLECS_OUTPUT_DBG + 7), i_cap_act);              /* 电容电流反馈，DLL:16。 */
    plecs_set_output((PLECS_OUTPUT_E)(PLECS_OUTPUT_DBG + 8), i_cap_inner_out);        /* 电流内环输出，DLL:17。 */
    plecs_set_output((PLECS_OUTPUT_E)(PLECS_OUTPUT_DBG + 9), harmonic_comp);          /* 谐波补偿输出，DLL:18。 */
    plecs_set_output((PLECS_OUTPUT_E)(PLECS_OUTPUT_DBG + 10), v_cap_beta);            /* APF 正交电压，DLL:19。 */
    plecs_set_output((PLECS_OUTPUT_E)(PLECS_OUTPUT_DBG + 11), vpwm);                  /* PWM 电压命令，DLL:20。 */
#endif                                                                                /* IS_PLECS */

    p_hal_isr->p_set_pwm_func(vpwm, v_bus_fb);

    phase_pu += freq_hz_ramped * inv_cfg_get_ctrl_ts();
    while (phase_pu >= 1.0f)
    {
        phase_pu -= 1.0f;
    }
}

REG_INTERRUPT(3, inv_ctrl_isr)

void inv_ctrl_set_p_hal(inv_ctrl_hal_t *p)
{
    (void)p;
}

void inv_ctrl_prepare_run(void)
{
    PLECS_LOG("inv_ctrl prepare run\n");
    inv_ctrl_run_active = 0U;
    inv_ctrl_first_run_cycle = 1U;
    inv_ctrl_reinit_states();
}
