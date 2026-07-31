// SPDX-License-Identifier: MIT
/**
 * @file    inv_ctrl.c
 * @brief   Inverter int32 SRFPI and capacitor-current damping controller.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Run integer phase generation, DQ voltage PI, and inverse-DQ current reference control
 *          - Apply integer APF, capacitor-current damping, and 3/5/7 harmonic compensation
 *          - Publish a bus-normalized PWM numerator without any 64-bit division
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR uses integer arithmetic and bounded int64_t multiply-accumulates
 *          - Floating coefficient design runs only in the 1 ms task or initialization path
 *
 * @author  Max.Li
 * @date    2026-08-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "inv_ctrl.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

#include "apf_i32.h"
#include "pi_tustin_i32.h"
#include "resonant_i32.h"
#include "section.h"
#include "trig_i32.h"

#define INV_CTRL_COEFF_UPDATE_FREQ_Q16 ((uint32_t)(0.01f * (float)INV_CTRL_FREQ_Q_ONE + 0.5f))

typedef struct
{
    apf_i32_coeff_t apf;             /**< APF Q30 coefficient set. */
    resonant_i32_coeff_t harmonic_3; /**< 3rd-harmonic Q29 coefficient set. */
    resonant_i32_coeff_t harmonic_5; /**< 5th-harmonic Q29 coefficient set. */
    resonant_i32_coeff_t harmonic_7; /**< 7th-harmonic Q29 coefficient set. */
    uint32_t freq_hz_q16;            /**< Frequency represented by this coefficient bundle. */
} inv_ctrl_coeff_bundle_t;

static pi_tustin_i32_t volt_loop_d = {0}; /**< D-axis voltage PI in K2 current-code domain. */
static pi_tustin_i32_t volt_loop_q = {0}; /**< Q-axis voltage PI in K2 current-code domain. */
static apf_i32_t volt_apf = {0};          /**< Capacitor-voltage quadrature generator. */
static resonant_i32_t harmonic_3 = {0};   /**< 3rd-harmonic current compensator. */
static resonant_i32_t harmonic_5 = {0};   /**< 5th-harmonic current compensator. */
static resonant_i32_t harmonic_7 = {0};   /**< 7th-harmonic current compensator. */

static inv_ctrl_hal_t *p_ctrl_hal = NULL;                       /**< Bound integer inverter HAL. */
static inv_ctrl_setpoint_t safe_setpoint = {0};                 /**< Disabled setpoint used before configuration. */
static inv_ctrl_setpoint_t *p_active_setpoint = &safe_setpoint; /**< Published setpoint snapshot. */

static int32_t v_cap_fb = 0;                      /**< Capacitor-voltage feedback in AC-voltage codes. */
static int32_t i_l_fb = 0;                        /**< Inductor-current feedback in current codes. */
static int32_t v_bus_fb = 0;                      /**< Bus-voltage feedback in bus-voltage codes. */
static int32_t v_d_ref = 0;                       /**< D-axis voltage reference in AC-voltage codes. */
static int32_t v_d_act = 0;                       /**< D-axis voltage feedback in AC-voltage codes. */
static int32_t v_q_ref = 0;                       /**< Q-axis voltage reference in AC-voltage codes. */
static int32_t v_q_act = 0;                       /**< Q-axis voltage feedback in AC-voltage codes. */
static int32_t sin_theta_q15 = 0;                 /**< Q15 phase sine. */
static int32_t cos_theta_q15 = TRIG_I32_Q15_ONE;  /**< Q15 phase cosine. */
static uint32_t phase_q32 = 0U;                   /**< Per-unit phase accumulator. */
static volatile uint32_t freq_hz_ramped_q16 = 0U; /**< ISR frequency ramp observed by the task. */
static int32_t v_ref_peak_ramped_q16 = 0;         /**< Q16 peak voltage-reference ramp. */
static int32_t v_cap_last = 0;                    /**< Previous capacitor-voltage code for differentiation. */
static int32_t cap_diff_gain_q16 = 0;             /**< Voltage-difference to current-code Q16 gain. */

static inv_ctrl_coeff_bundle_t pending_coeff = {0}; /**< Coefficients prepared outside the ISR. */
static volatile uint8_t pending_coeff_valid = 0U;   /**< 1 after the complete bundle is published. */
static uint32_t prepared_freq_hz_q16 = 0U;          /**< Last prepared coefficient frequency. */
static uint8_t loops_ready = 0U;                    /**< 1 after all integer loops initialize. */
static uint8_t run_active = 0U;                     /**< 1 while PWM control is active. */
static uint8_t first_run_cycle = 1U;                /**< Forces deterministic first-cycle state. */

static int32_t sat_i64_to_i32(int64_t value)
{
    if (value > (int64_t)INT32_MAX)
    {
        return INT32_MAX;
    }
    if (value < (int64_t)INT32_MIN)
    {
        return INT32_MIN;
    }
    return (int32_t)value;
}

static int32_t shift_i64_to_i32(int64_t value, uint8_t shift)
{
    int64_t shifted = 0; /**< Sign-preserving shifted value. */

    if (value < 0)
    {
        shifted = -((-value) >> shift);
    }
    else
    {
        shifted = value >> shift;
    }
    return sat_i64_to_i32(shifted);
}

static uint32_t abs_diff_u32(uint32_t first, uint32_t second)
{
    return (first >= second) ? (first - second) : (second - first);
}

static uint32_t limit_frequency_q16(uint32_t frequency_q16)
{
    uint32_t ctrl_freq_hz = inv_cfg_get_ctrl_freq_hz();                     /**< Integer sample frequency. */
    uint32_t max_freq_hz = ctrl_freq_hz / (2U * INV_CTRL_HARMONIC_7_ORDER); /**< Nyquist limit. */
    uint32_t max_frequency_q16 = max_freq_hz << INV_CTRL_FREQ_Q_SHIFT;      /**< Q16 upper bound. */
    uint32_t min_frequency_q16 = 1UL << INV_CTRL_FREQ_Q_SHIFT;              /**< 1 Hz lower bound. */

    if (frequency_q16 < min_frequency_q16)
    {
        return min_frequency_q16;
    }
    if (frequency_q16 > max_frequency_q16)
    {
        return max_frequency_q16;
    }
    return frequency_q16;
}

static uint32_t ramp_u32(uint32_t value, uint32_t target, uint32_t step)
{
    if ((step == 0U) ||
        (value == target))
    {
        return target;
    }
    if (value < target)
    {
        return ((target - value) <= step) ? target : (value + step);
    }
    return ((value - target) <= step) ? target : (value - step);
}

static int32_t ramp_i32(int32_t value, int32_t target, uint32_t step)
{
    int64_t delta = (int64_t)target - (int64_t)value; /**< Signed distance to the target. */

    if ((step == 0U) ||
        (delta == 0))
    {
        return target;
    }
    if (delta > 0)
    {
        return ((uint64_t)delta <= (uint64_t)step) ? target : sat_i64_to_i32((int64_t)value + step);
    }
    return ((uint64_t)(-delta) <= (uint64_t)step) ? target : sat_i64_to_i32((int64_t)value - step);
}

static int32_t dq_shift(int64_t accumulator)
{
    return shift_i64_to_i32(accumulator, 15U);
}

static bool design_coeff_bundle(inv_ctrl_coeff_bundle_t *p_bundle, uint32_t frequency_q16)
{
    float frequency_hz = 0.0f;             /**< Fundamental frequency used for coefficient design. */
    float omega_radps = 0.0f;              /**< Fundamental angular frequency. */
    float ctrl_ts = inv_cfg_get_ctrl_ts(); /**< Configured sample period. */
    bool valid = true;                     /**< Combined coefficient-design result. */

    if (p_bundle == NULL)
    {
        return false;
    }

    frequency_hz = (float)frequency_q16 / (float)INV_CTRL_FREQ_Q_ONE;
    omega_radps = M_2PI * frequency_hz;
    valid = apf_i32_design_coeff(&p_bundle->apf, omega_radps, ctrl_ts);
    valid = resonant_i32_design_coeff(&p_bundle->harmonic_3,
                                      INV_CTRL_HARMONIC_3_GAIN,
                                      INV_CTRL_HARMONIC_3_ORDER,
                                      omega_radps,
                                      ctrl_ts,
                                      INV_CTRL_IND_CURR_CODE_PER_A,
                                      INV_CTRL_AC_VOLT_CODE_PER_V) &&
            valid;
    valid = resonant_i32_design_coeff(&p_bundle->harmonic_5,
                                      INV_CTRL_HARMONIC_5_GAIN,
                                      INV_CTRL_HARMONIC_5_ORDER,
                                      omega_radps,
                                      ctrl_ts,
                                      INV_CTRL_IND_CURR_CODE_PER_A,
                                      INV_CTRL_AC_VOLT_CODE_PER_V) &&
            valid;
    valid = resonant_i32_design_coeff(&p_bundle->harmonic_7,
                                      INV_CTRL_HARMONIC_7_GAIN,
                                      INV_CTRL_HARMONIC_7_ORDER,
                                      omega_radps,
                                      ctrl_ts,
                                      INV_CTRL_IND_CURR_CODE_PER_A,
                                      INV_CTRL_AC_VOLT_CODE_PER_V) &&
            valid;
    p_bundle->freq_hz_q16 = frequency_q16;
    return valid;
}

static bool apply_coeff_bundle(const inv_ctrl_coeff_bundle_t *p_bundle)
{
    bool valid = true; /**< Combined coefficient-application result. */

    if (p_bundle == NULL)
    {
        return false;
    }
    valid = apf_i32_set_coeff(&volt_apf, &p_bundle->apf);
    valid = resonant_i32_set_coeff(&harmonic_3, &p_bundle->harmonic_3) && valid;
    valid = resonant_i32_set_coeff(&harmonic_5, &p_bundle->harmonic_5) && valid;
    valid = resonant_i32_set_coeff(&harmonic_7, &p_bundle->harmonic_7) && valid;
    return valid;
}

static void update_feedback(inv_ctrl_hal_t *p_hal)
{
    v_cap_fb = *p_hal->p_v_cap;
    i_l_fb = *p_hal->p_i_l;
    v_bus_fb = *p_hal->p_v_bus;
}

static void reset_loops(void)
{
    pi_tustin_i32_reset_inline(&volt_loop_d);
    pi_tustin_i32_reset_inline(&volt_loop_q);
    apf_i32_reset(&volt_apf);
    resonant_i32_reset(&harmonic_3);
    resonant_i32_reset(&harmonic_5);
    resonant_i32_reset(&harmonic_7);
    v_cap_last = v_cap_fb;
}

static void reinit_states(void)
{
    inv_ctrl_coeff_bundle_t initial_coeff = {0}; /**< Initial frequency coefficient bundle. */
    inv_ctrl_setpoint_t *p_setpoint = NULL;      /**< Active integer setpoint. */
    float cap_diff_gain = 0.0f;                  /**< Q16 capacitor-current differentiator gain. */
    bool valid = true;                           /**< Combined controller initialization result. */

    p_ctrl_hal = inv_hal_get_ctrl();
    loops_ready = 0U;
    run_active = 0U;
    first_run_cycle = 1U;
    pending_coeff_valid = 0U;
    inv_cfg_sync_building_to_active();
    p_setpoint = inv_cfg_get_p_active();

    if ((p_ctrl_hal == NULL) ||
        (p_setpoint == NULL) ||
        (inv_cfg_is_ready() == 0U) ||
        (p_ctrl_hal->p_v_cap == NULL) ||
        (p_ctrl_hal->p_i_l == NULL) ||
        (p_ctrl_hal->p_v_bus == NULL))
    {
        return;
    }

    p_active_setpoint = p_setpoint;
    update_feedback(p_ctrl_hal);
    freq_hz_ramped_q16 = limit_frequency_q16(p_setpoint->freq_hz_q16);
    prepared_freq_hz_q16 = freq_hz_ramped_q16;
    valid = design_coeff_bundle(&initial_coeff, freq_hz_ramped_q16);
    valid = apf_i32_init(&volt_apf, &initial_coeff.apf) && valid;
    valid = resonant_i32_init(&harmonic_3, &initial_coeff.harmonic_3) && valid;
    valid = resonant_i32_init(&harmonic_5, &initial_coeff.harmonic_5) && valid;
    valid = resonant_i32_init(&harmonic_7, &initial_coeff.harmonic_7) && valid;
    valid = pi_tustin_i32_init(&volt_loop_d,
                               INV_CTRL_VOLT_LOOP_KP_I32,
                               INV_CTRL_VOLT_LOOP_KI_I32,
                               inv_cfg_get_ctrl_ts(),
                               INV_CTRL_VOLT_LOOP_OUT_MAX_I32,
                               INV_CTRL_VOLT_LOOP_OUT_MIN_I32,
                               &v_d_ref,
                               &v_d_act) &&
            valid;
    valid = pi_tustin_i32_init(&volt_loop_q,
                               INV_CTRL_VOLT_LOOP_KP_I32,
                               INV_CTRL_VOLT_LOOP_KI_I32,
                               inv_cfg_get_ctrl_ts(),
                               INV_CTRL_VOLT_LOOP_OUT_MAX_I32,
                               INV_CTRL_VOLT_LOOP_OUT_MIN_I32,
                               &v_q_ref,
                               &v_q_act) &&
            valid;

    cap_diff_gain = HW_AC_SIDE_CAP_VALUE * (float)inv_cfg_get_ctrl_freq_hz() *
                    INV_CTRL_IND_CURR_CODE_PER_A / INV_CTRL_AC_VOLT_CODE_PER_V *
                    (float)(1UL << INV_CTRL_CAP_DIFF_GAIN_Q_SHIFT);
    cap_diff_gain_q16 = (int32_t)(cap_diff_gain + 0.5f);
    phase_q32 = 0U;
    v_ref_peak_ramped_q16 = 0;
    sin_theta_q15 = 0;
    cos_theta_q15 = TRIG_I32_Q15_ONE;
    reset_loops();
    loops_ready = valid ? 1U : 0U;
}

static void inv_ctrl_init(void)
{
    reinit_states();
}

REG_INIT(0, inv_ctrl_init)

static void prepare_frequency_coeff_task(void)
{
    uint32_t frequency_snapshot = freq_hz_ramped_q16; /**< Atomic 32-bit ISR frequency snapshot. */

    if ((loops_ready == 0U) ||
        (pending_coeff_valid != 0U) ||
        (abs_diff_u32(frequency_snapshot, prepared_freq_hz_q16) < INV_CTRL_COEFF_UPDATE_FREQ_Q16))
    {
        return;
    }

    if (design_coeff_bundle(&pending_coeff, frequency_snapshot))
    {
        prepared_freq_hz_q16 = frequency_snapshot;
        pending_coeff_valid = 1U;
    }
}

REG_TASK_MS(1, prepare_frequency_coeff_task)

static void inv_ctrl_cal_theta(void)
{
    trig_i32_sin_cos_q15(phase_q32, &sin_theta_q15, &cos_theta_q15);
}

REG_INTERRUPT(0, inv_ctrl_cal_theta)

static void inv_ctrl_isr(void)
{
    inv_ctrl_hal_t *p_hal = p_ctrl_hal;                  /**< Stable HAL snapshot for this ISR. */
    inv_ctrl_setpoint_t *p_setpoint = p_active_setpoint; /**< Stable setpoint pointer. */
    uint32_t ctrl_freq_hz = inv_cfg_get_ctrl_freq_hz();  /**< Integer sample frequency. */
    uint32_t freq_step_q16 = 0U;                         /**< Per-cycle Q16 frequency increment. */
    uint32_t ref_step_q16 = 0U;                          /**< Per-cycle Q16 voltage-code increment. */
    uint32_t phase_step_q32 = 0U;                        /**< Current Q32 phase increment. */
    int32_t v_cap_beta = 0;                              /**< APF quadrature voltage code. */
    int32_t i_cap_ref_k2 = 0;                            /**< Inverse-DQ current reference in K2 domain. */
    int32_t i_cap_ref = 0;                               /**< Current reference in raw current codes. */
    int32_t i_cap_act = 0;                               /**< Differentiated capacitor current code. */
    int32_t harmonic_comp = 0;                           /**< Limited harmonic current compensation. */
    int32_t current_error = 0;                           /**< Capacitor-current damping error code. */
    int32_t inner_output = 0;                            /**< AC-code-times-reload damping output. */
    int32_t v_ref_code = 0;                              /**< Instantaneous voltage reference code. */
    int32_t pwm_command = 0;                             /**< AC-code-times-reload PWM numerator. */
    int64_t accumulator = 0;                             /**< Bounded multiply-accumulate scratch. */

    if ((p_hal == NULL) ||
        (p_setpoint == NULL) ||
        (loops_ready == 0U) ||
        (ctrl_freq_hz == 0U) ||
        (p_hal->p_v_cap == NULL) ||
        (p_hal->p_i_l == NULL) ||
        (p_hal->p_v_bus == NULL) ||
        (p_hal->p_set_pwm_func == NULL) ||
        (p_hal->p_pwm_disable == NULL))
    {
        return;
    }

    inv_cfg_sync_building_to_active();
    update_feedback(p_hal);

    if (p_setpoint->run_allowed == 0U)
    {
        if (run_active != 0U)
        {
            p_hal->p_pwm_disable();
            reset_loops();
            run_active = 0U;
            first_run_cycle = 1U;
        }
        return;
    }

    if (pending_coeff_valid != 0U)
    {
        if (apply_coeff_bundle(&pending_coeff))
        {
            pending_coeff_valid = 0U;
        }
    }

    run_active = 1U;
    if (first_run_cycle != 0U)
    {
        freq_hz_ramped_q16 = limit_frequency_q16(p_setpoint->freq_hz_q16);
        first_run_cycle = 0U;
    }
    else
    {
        freq_step_q16 = p_setpoint->freq_slew_hzps_q16 / ctrl_freq_hz;
        if ((p_setpoint->freq_slew_hzps_q16 != 0U) &&
            (freq_step_q16 == 0U))
        {
            freq_step_q16 = 1U;
        }
        freq_hz_ramped_q16 = ramp_u32(freq_hz_ramped_q16,
                                      limit_frequency_q16(p_setpoint->freq_hz_q16),
                                      freq_step_q16);
    }

    ref_step_q16 = p_setpoint->v_ref_slew_code_q16_per_s / ctrl_freq_hz;
    if ((p_setpoint->v_ref_slew_code_q16_per_s != 0U) &&
        (ref_step_q16 == 0U))
    {
        ref_step_q16 = 1U;
    }
    v_ref_peak_ramped_q16 = ramp_i32(v_ref_peak_ramped_q16,
                                     p_setpoint->v_ref_peak_code_q16,
                                     ref_step_q16);
    v_d_ref = v_ref_peak_ramped_q16 >> INV_CTRL_REF_Q_SHIFT;
    v_q_ref = 0;

    v_cap_beta = apf_i32_cal(&volt_apf, v_cap_fb);
    accumulator = ((int64_t)v_cap_fb * (int64_t)cos_theta_q15) +
                  ((int64_t)v_cap_beta * (int64_t)sin_theta_q15);
    v_d_act = dq_shift(accumulator);
    accumulator = -((int64_t)v_cap_fb * (int64_t)sin_theta_q15) +
                  ((int64_t)v_cap_beta * (int64_t)cos_theta_q15);
    v_q_act = dq_shift(accumulator);

    pi_tustin_i32_cal_a1_neg1_inline(&volt_loop_d);
    pi_tustin_i32_cal_a1_neg1_inline(&volt_loop_q);
    accumulator = ((int64_t)cos_theta_q15 * (int64_t)volt_loop_d.output.val) -
                  ((int64_t)sin_theta_q15 * (int64_t)volt_loop_q.output.val);
    i_cap_ref_k2 = dq_shift(accumulator);
    i_cap_ref = shift_i64_to_i32(i_cap_ref_k2, INV_CTRL_K2_CURR_SHIFT);

    accumulator = (int64_t)cap_diff_gain_q16 * ((int64_t)v_cap_fb - (int64_t)v_cap_last);
    i_cap_act = shift_i64_to_i32(accumulator, INV_CTRL_CAP_DIFF_GAIN_Q_SHIFT);
    v_cap_last = v_cap_fb;

    accumulator = (int64_t)resonant_i32_cal(&harmonic_3, v_cap_fb) +
                  (int64_t)resonant_i32_cal(&harmonic_5, v_cap_fb) +
                  (int64_t)resonant_i32_cal(&harmonic_7, v_cap_fb);
    harmonic_comp = sat_i64_to_i32(accumulator);
    if (harmonic_comp > INV_CTRL_HARMONIC_OUT_MAX_I32)
    {
        harmonic_comp = INV_CTRL_HARMONIC_OUT_MAX_I32;
    }
    else if (harmonic_comp < INV_CTRL_HARMONIC_OUT_MIN_I32)
    {
        harmonic_comp = INV_CTRL_HARMONIC_OUT_MIN_I32;
    }

    accumulator = (int64_t)i_cap_ref - (int64_t)i_cap_act - (int64_t)harmonic_comp;
    current_error = sat_i64_to_i32(accumulator);
    accumulator = (int64_t)INV_CTRL_INNER_GAIN_I32 * (int64_t)current_error;
    inner_output = sat_i64_to_i32(accumulator);

    accumulator = (int64_t)v_ref_peak_ramped_q16 * (int64_t)cos_theta_q15;
    v_ref_code = shift_i64_to_i32(accumulator, INV_CTRL_REF_Q_SHIFT + 15U);
    accumulator = ((int64_t)v_ref_code * (int64_t)INV_CTRL_PWM_RELOAD) + (int64_t)inner_output;
    pwm_command = sat_i64_to_i32(accumulator);
    p_hal->p_set_pwm_func(pwm_command, v_bus_fb);

    accumulator = (int64_t)freq_hz_ramped_q16 * (int64_t)inv_cfg_get_phase_step_gain_q24();
    phase_step_q32 = (uint32_t)(accumulator >> INV_CTRL_PHASE_GAIN_Q_SHIFT);
    phase_q32 += phase_step_q32;
    (void)i_l_fb;
}

REG_INTERRUPT(3, inv_ctrl_isr)

void inv_ctrl_set_p_hal(inv_ctrl_hal_t *p_hal)
{
    (void)p_hal;
}

void inv_ctrl_prepare_run(void)
{
    reinit_states();
}
