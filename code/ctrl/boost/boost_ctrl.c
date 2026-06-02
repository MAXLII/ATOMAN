// SPDX-License-Identifier: MIT
/**
 * @file    boost_ctrl.c
 * @brief   boost_ctrl control module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement BOOST voltage, current, and power-limit control paths
 *          - Prepare and reset PI controllers before entering run state
 *          - Consume HAL measurements and setpoints to generate one BOOST PWM command
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-23
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "boost_ctrl.h"
#include "boost_cfg.h"
#include "pi_tustin_i32.h"
#include "section.h"
#include <stddef.h>

#define p_hal (p_ctrl_hal)

#define BOOST_CTRL_DCM_DUTY_CONST_SHIFT (14U)
#define BOOST_CTRL_DCM_DUTY_CONST_SCALE ((int32_t)(1L << BOOST_CTRL_DCM_DUTY_CONST_SHIFT))
#define BOOST_CTRL_DCM_CURR_REF_FRAC_SHIFT (14U)
#define BOOST_CTRL_DCM_CURR_REF_FRAC_SCALE ((int32_t)(1L << BOOST_CTRL_DCM_CURR_REF_FRAC_SHIFT))
#define BOOST_CTRL_DCM_SQRT_INPUT_FRAC_SHIFT (14U)
#define BOOST_CTRL_DCM_SQRT_RESULT_FRAC_SHIFT (BOOST_CTRL_DCM_SQRT_INPUT_FRAC_SHIFT / 2U)
#define BOOST_CTRL_IND_CURR_REF_SHIFT (BOOST_CTRL_K2_IND_CURR_FB_SHIFT + 1U)
#define BOOST_CTRL_DCM_CURR_REF_DIV_SHIFT \
    (BOOST_CTRL_K2_IND_CURR_FB_SHIFT - BOOST_CTRL_DCM_CURR_REF_FRAC_SHIFT)
#define BOOST_CTRL_IND_CURR_FB_CODE_CENTER ((int32_t)0x4000)
#define BOOST_CTRL_DCM_ENTER_DELAY_TICKS (TIME_CNT_100MS_IN_CTRL)
#define BOOST_CTRL_DCM_ENTER_CMP_DELTA (8000)
#define BOOST_CTRL_DCM_EXIT_CMP_DELTA (5000)
#define BOOST_CTRL_DCM_CODE_NORM                      \
    ((float)BOOST_CTRL_CMP_MAX /                      \
     (((float)BOOST_CTRL_IND_CURR_LOOP_REF_CODE_MAX / \
       BOOST_CTRL_IND_CURR_LOOP_REF_MAX_A) *          \
      (BOOST_CTRL_OUT_VOLT_LOOP_REF_MAX_V /           \
       (float)BOOST_CTRL_OUT_VOLT_LOOP_REF_CODE_MAX)))
#define BOOST_CTRL_DCM_CONST_SCALE (10000)

static pi_tustin_i32_t out_volt_loop = {0};
static pi_tustin_i32_t in_volt_lmt_loop = {0};
static pi_tustin_i32_t ind_curr_loop[BOOST_CTRL_IND_CURR_CH_NUM] = {0};

typedef struct
{
    /* Voltage-loop current limit kept in the K2 current domain for diagnostics and limiting. */
    int32_t i_l_lmt;
} isr_param_t;

static boost_ctrl_hal_t *p_ctrl_hal = NULL;
static boost_ctrl_setpoint_t safe_setpoint = {0};
static boost_ctrl_setpoint_t *p_ctrl_active_setpoint = &safe_setpoint;
static volatile isr_param_t isr_param = {
    .i_l_lmt = 0,
};
static volatile isr_param_t isr_param_pending[2] = {
    {
        .i_l_lmt = 0,
    },
    {
        .i_l_lmt = 0,
    },
};
static volatile uint8_t isr_param_pending_idx = 0U;
static volatile uint32_t isr_param_publish_seq = 0U;
static uint32_t isr_param_applied_seq = 0U;
static int32_t v_in_fb = 0;
static int32_t v_out_fb = 0;
static int32_t i_l_fb[BOOST_CTRL_IND_CURR_CH_NUM] = {0};
static int32_t ind_curr_ref = 0;
static int32_t dcm_duty_const = 0;
static uint8_t run_active = 0U;
static uint8_t up_en_dcm[BOOST_CTRL_IND_CURR_CH_NUM] = {1U};
static int32_t dcm_curr_ref = 0;
static uint32_t dcm_enter_delay_cnt[BOOST_CTRL_IND_CURR_CH_NUM] = {0U};

static inline int32_t div_pow2_i32(int32_t val, uint32_t shift)
{
    if (val >= 0)
    {
        return val >> shift;
    }

    return -((-val) >> shift);
}

static inline int32_t sub_floor_i32(int32_t a, int32_t b)
{
    if (a <= b)
    {
        return 0;
    }

    return a - b;
}

static inline int32_t scale_ref_by_volt_ratio(int32_t curr_ref, int32_t v_out, int32_t v_in)
{
    int32_t quotient = 0;
    int32_t remainder = 0;
    int32_t rem_scaled = 0;
    int64_t scaled = 0;

    if (v_in <= 0)
    {
        return curr_ref;
    }

    quotient = curr_ref / v_in;
    remainder = curr_ref - (quotient * v_in);
    scaled = (int64_t)quotient * (int64_t)v_out;
    rem_scaled = pi_tustin_i32_sat_i64_to_i32((int64_t)remainder * (int64_t)v_out) / v_in;

    return pi_tustin_i32_sat_i64_to_i32(scaled + (int64_t)rem_scaled);
}

static inline void update_adc_feedback(boost_ctrl_hal_t *p)
{
    uint32_t ch = 0U;

    v_in_fb = (int32_t)*p->p_v_in;
    v_out_fb = (int32_t)*p->p_v_out;

    for (ch = 0U; ch < BOOST_CTRL_IND_CURR_CH_NUM; ch++)
    {
        i_l_fb[ch] = BOOST_CTRL_IND_CURR_FB_CODE_CENTER - (int32_t)*p->p_i_l[ch];
    }
}

static void reset_dcm_state(void)
{
    uint32_t ch = 0U;

    dcm_curr_ref = 0;

    for (ch = 0U; ch < BOOST_CTRL_IND_CURR_CH_NUM; ch++)
    {
        up_en_dcm[ch] = 1U;
        dcm_enter_delay_cnt[ch] = 0U;
    }
}

static inline void isr_param_request_update(isr_param_t param)
{
    uint8_t pending_idx = 0U;

    pending_idx = (uint8_t)(isr_param_pending_idx ^ 1U);
    isr_param_pending[pending_idx].i_l_lmt = param.i_l_lmt;
    isr_param_pending_idx = pending_idx;
    isr_param_publish_seq++;
}

static uint8_t channel_ready(void)
{
    uint32_t ch = 0U;

    if (p_hal == NULL)
    {
        return 0U;
    }

    for (ch = 0U; ch < BOOST_CTRL_IND_CURR_CH_NUM; ch++)
    {
        if ((p_hal->p_i_l[ch] == NULL) ||
            (p_hal->p_set_pwm_func[ch] == NULL))
        {
            return 0U;
        }
    }

    return 1U;
}

static inline int32_t limit_cmp(int32_t cmp)
{
    if (cmp > BOOST_CTRL_CMP_MAX)
    {
        cmp = BOOST_CTRL_CMP_MAX;
    }
    else if (cmp < BOOST_CTRL_CMP_MIN)
    {
        cmp = BOOST_CTRL_CMP_MIN;
    }

    return (int32_t)cmp;
}

static inline int32_t calc_cmp(int32_t v_l_cmd, int32_t v_in, int32_t v_out)
{
    /* Boost compare uses vpwm = vin - vl, then duty = vpwm / vout. */
    int64_t numerator_i64 = 0;

    /* Compare numerator saturated before division to keep the divider 32-bit. */
    int32_t numerator = 0;

    /* Compare command before final output limiting. */
    int32_t cmp = 0;

    if (v_out <= 0)
    {
        return BOOST_CTRL_CMP_MIN;
    }

    numerator_i64 = ((int64_t)v_in * (int64_t)BOOST_CTRL_OUT_VOLT_LOOP_V_OUT_FF_K) -
                    (int64_t)v_l_cmd;
    numerator = pi_tustin_i32_sat_i64_to_i32(numerator_i64);
    cmp = numerator / v_out;

    return limit_cmp(cmp);
}

static inline int32_t scale_ind_curr_to_k2(int32_t i_l)
{
    /* Current command converted into the K2 current domain. */
    int64_t scaled = 0;

    scaled = (int64_t)i_l * (int64_t)BOOST_CTRL_IND_CURR_LOOP_FB_K;

    return pi_tustin_i32_sat_i64_to_i32(scaled);
}

static void calc_dcm_duty_const(void)
{
    float const_val = 0.0f;
    float pwm_ts = boost_cfg_get_pwm_ts();

    if ((BOOST_HW_IND <= 0.0f) ||
        (pwm_ts <= 0.0f))
    {
        dcm_duty_const = 0;
        return;
    }

    const_val = (float)BOOST_CTRL_DCM_CONST_SCALE *
                sqrtf(((2.0f * BOOST_HW_IND) / pwm_ts) *
                      BOOST_CTRL_DCM_CODE_NORM);
    dcm_duty_const = (int32_t)const_val;
}

static inline int32_t mul_div_u32_sat_i32(uint32_t a, uint32_t b, uint32_t divisor)
{
    uint32_t quotient = 0U;
    uint32_t remainder = 0U;
    uint32_t remainder_term = 0U;
    int64_t result = 0;

    if (divisor == 0U)
    {
        return INT32_MAX;
    }

    quotient = a / divisor;
    remainder = a - (quotient * divisor);
    if ((b != 0U) &&
        (remainder > (UINT32_MAX / b)))
    {
        remainder_term = UINT32_MAX / divisor;
    }
    else
    {
        remainder_term = (remainder * b) / divisor;
    }

    result = ((int64_t)quotient * (int64_t)b) + (int64_t)remainder_term;

    return pi_tustin_i32_sat_i64_to_i32(result);
}

static inline int32_t calc_dcm_duty_code(int32_t const_k,
                                         int32_t i_ref,
                                         int32_t v_in,
                                         int32_t duty_ccm)
{
    int32_t sqrt_input = 0;
    int32_t sqrt_result = 0;

    if ((const_k <= 0) ||
        (i_ref <= 0) ||
        (v_in <= 0) ||
        (duty_ccm <= 0))
    {
        return duty_ccm;
    }

    sqrt_input = mul_div_u32_sat_i32((uint32_t)i_ref,
                                     (uint32_t)duty_ccm,
                                     (uint32_t)v_in);
    sqrt_result = (int32_t)sqrtf((float)sqrt_input);

    return mul_div_u32_sat_i32((uint32_t)const_k,
                               (uint32_t)sqrt_result,
                               (uint32_t)BOOST_CTRL_DCM_CONST_SCALE);
}

static inline int32_t min_i32(int32_t a, int32_t b)
{
    if (a < b)
    {
        return a;
    }

    return b;
}

static inline int32_t div_pos_i32(int32_t numerator, int32_t denominator)
{
    if (denominator <= 0)
    {
        return numerator;
    }

    return numerator / denominator;
}

static inline int32_t div_by_cmp_i32(int32_t numerator, int32_t cmp)
{
    /* Scaled numerator saturated before division so the runtime divider stays 32-bit. */
    int64_t scaled = 0;
    int32_t scaled_i32 = 0;

    if (cmp <= 0)
    {
        return numerator;
    }

    scaled = (int64_t)numerator * (int64_t)BOOST_CTRL_OUT_VOLT_LOOP_V_OUT_FF_K;
    scaled_i32 = pi_tustin_i32_sat_i64_to_i32(scaled);

    return scaled_i32 / cmp;
}

static void reinit_states(void)
{
    boost_ctrl_setpoint_t *p_active_setpoint = NULL;
    uint32_t ch = 0U;

    p_ctrl_hal = boost_hal_get_ctrl();
    run_active = 0U;
    reset_dcm_state();
    boost_cfg_sync_building_to_active();
    p_active_setpoint = boost_cfg_get_p_active();

    if ((p_hal == NULL) ||
        (boost_cfg_is_ready() == 0U) ||
        (p_active_setpoint == NULL) ||
        (p_hal->p_v_in == NULL) ||
        (p_hal->p_v_out == NULL) ||
        (channel_ready() == 0U))
    {
        return;
    }

    calc_dcm_duty_const();
    p_ctrl_active_setpoint = p_active_setpoint;
    ind_curr_ref = 0;

    (void)pi_tustin_i32_init(&out_volt_loop,
                             BOOST_CTRL_OUT_VOLT_LOOP_KP,
                             BOOST_CTRL_OUT_VOLT_LOOP_KI,
                             BOOST_CTRL_TS,
                             BOOST_CTRL_OUT_VOLT_LOOP_UP_LMT,
                             BOOST_CTRL_OUT_VOLT_LOOP_DN_LMT,
                             &p_active_setpoint->out_volt_ref,
                             &v_out_fb);

    (void)pi_tustin_i32_init(&in_volt_lmt_loop,
                             BOOST_CTRL_IN_VOLT_LMT_LOOP_KP,
                             BOOST_CTRL_IN_VOLT_LMT_LOOP_KI,
                             BOOST_CTRL_TASK_TS,
                             BOOST_CTRL_IN_VOLT_LMT_LOOP_UP_LMT,
                             BOOST_CTRL_IN_VOLT_LMT_LOOP_DN_LMT,
                             &v_in_fb,
                             &p_active_setpoint->in_volt_lmt);

    for (ch = 0U; ch < BOOST_CTRL_IND_CURR_CH_NUM; ch++)
    {
        (void)pi_tustin_i32_init(&ind_curr_loop[ch],
                                 BOOST_CTRL_IND_CURR_LOOP_KP,
                                 BOOST_CTRL_IND_CURR_LOOP_KI,
                                 BOOST_CTRL_TS,
                                 BOOST_CTRL_IND_CURR_LOOP_UP_LMT,
                                 BOOST_CTRL_IND_CURR_LOOP_DN_LMT,
                                 &ind_curr_ref,
                                 &i_l_fb[ch]);
    }
}

static void boost_init(void)
{
    reinit_states();
}

REG_INIT(0, boost_init)

static void FUNC_RAM boost_isr(void)
{
    boost_ctrl_hal_t *p_hal_isr = p_hal;
    boost_ctrl_setpoint_t *p_setpoint = p_ctrl_active_setpoint;
    uint8_t pending_idx = 0U;
    uint8_t freewheel_en = 1U;
    uint32_t publish_seq = 0U;
    uint32_t ch = 0U;
    int32_t i_l_lmt = 0;
    int32_t v_in = 0;
    int32_t v_out = 0;
    int32_t cmp = 0;
    int32_t cmp_calc = 0;
    int32_t cmp_numerator = 0;
    int32_t ccm_cmp = 0;
    int32_t dcm_cmp = 0;
    /* Voltage-loop total current reference before channel split. */
    int32_t i_l_ref_total = 0;
    int32_t i_l_ref_input_total = 0;
    int32_t v_in_ff = 0;
    int32_t ind_curr_pi_out = 0;
    uint8_t main_en = 1U;

    boost_cfg_sync_building_to_active_fast();

    /* Commit the task-built ISR parameter snapshot at the PWM update point. */
    publish_seq = isr_param_publish_seq;
    if (isr_param_applied_seq != publish_seq)
    {
        pending_idx = isr_param_pending_idx;
        isr_param.i_l_lmt = isr_param_pending[pending_idx].i_l_lmt;
        isr_param_applied_seq = publish_seq;
    }

    if (p_setpoint->run_allowed == 0U)
    {
        if (run_active != 0U)
        {
            p_hal_isr->p_pwm_disable();
            run_active = 0U;
        }
        return;
    }

    if (run_active == 0U)
    {
        p_hal_isr->p_pwm_enable();
        run_active = 1U;
    }

    i_l_lmt = isr_param.i_l_lmt;
    freewheel_en = 1U;
    main_en = 1U;

    update_adc_feedback(p_hal_isr);
    v_in = v_in_fb;
    v_out = v_out_fb;

    if ((v_in <= 0) ||
        (v_out <= 0))
    {
        for (ch = 0U; ch < BOOST_CTRL_IND_CURR_CH_NUM; ch++)
        {
            p_hal_isr->p_set_pwm_func[ch]((uint32_t)BOOST_CTRL_CMP_MIN,
                                          freewheel_en,
                                          main_en);
        }

        return;
    }

    /* Voltage loop now runs at the PWM interrupt rate and publishes the current-loop reference directly. */
    out_volt_loop.inter.up_lmt = i_l_lmt;
    pi_tustin_i32_cal_a1_neg1_inline(&out_volt_loop);
    i_l_ref_total = out_volt_loop.output.val;
    i_l_ref_input_total = i_l_ref_total;

    ind_curr_ref = div_pow2_i32(i_l_ref_input_total, BOOST_CTRL_IND_CURR_REF_SHIFT);
    dcm_curr_ref = ind_curr_ref;

    v_in_ff = v_in * BOOST_CTRL_K4_V_OUT_FF_K;

    for (ch = 0U; ch < BOOST_CTRL_IND_CURR_CH_NUM; ch++)
    {
        pi_tustin_i32_cal_a1_neg1_inline(&ind_curr_loop[ch]);
        ind_curr_pi_out = ind_curr_loop[ch].output.val;
        cmp_numerator = v_in_ff - ind_curr_pi_out;
        cmp_calc = cmp_numerator / v_out;
        if (cmp_calc > BOOST_CTRL_CMP_MAX)
        {
            cmp = BOOST_CTRL_CMP_MAX;
        }
        else if (cmp_calc < BOOST_CTRL_CMP_MIN)
        {
            cmp = BOOST_CTRL_CMP_MIN;
        }
        else
        {
            cmp = (int32_t)cmp_calc;
        }

        ccm_cmp = cmp;
        dcm_cmp = calc_dcm_duty_code(dcm_duty_const,
                                     dcm_curr_ref,
                                     v_in,
                                     BOOST_CTRL_CMP_MAX - ccm_cmp);
        dcm_cmp = limit_cmp(dcm_cmp);

        if (dcm_cmp < sub_floor_i32(ccm_cmp, BOOST_CTRL_DCM_ENTER_CMP_DELTA))
        {
            if (dcm_enter_delay_cnt[ch] < BOOST_CTRL_DCM_ENTER_DELAY_TICKS)
            {
                dcm_enter_delay_cnt[ch]++;
            }

            if (dcm_enter_delay_cnt[ch] >= BOOST_CTRL_DCM_ENTER_DELAY_TICKS)
            {
                up_en_dcm[ch] = 0U;
            }
        }
        else if (dcm_cmp > sub_floor_i32(ccm_cmp, BOOST_CTRL_DCM_EXIT_CMP_DELTA))
        {
            up_en_dcm[ch] = 1U;
            dcm_enter_delay_cnt[ch] = 0U;
        }
        else if (up_en_dcm[ch] != 0U)
        {
            dcm_enter_delay_cnt[ch] = 0U;
        }

        if (up_en_dcm[ch] == 0U)
        {
            cmp = BOOST_CTRL_CMP_MAX - dcm_cmp;
        }
        else
        {
            cmp = ccm_cmp;
        }

        freewheel_en = up_en_dcm[ch];
        p_hal_isr->p_set_pwm_func[ch]((uint32_t)cmp,
                                      freewheel_en,
                                      main_en);
    }
}

REG_INTERRUPT(3, boost_isr)

static void boost_task(void)
{
    isr_param_t param = {0};
    int32_t v_in = 0;
    int32_t v_out = 0;
    int32_t cmp = 0;
    int32_t pwr_i_in_lmt = 0;
    int32_t in_curr_i_in_lmt = 0;
    int32_t in_curr_i_l_lmt = 0;
    int32_t out_curr_i_l_lmt = 0;
    int32_t in_volt_i_l_lmt = 0;
    int32_t i_l_lmt = 0;

    if (p_ctrl_active_setpoint->run_allowed == 0U)
    {
        reset_dcm_state();
        return;
    }

    update_adc_feedback(p_hal);
    v_in = v_in_fb;
    v_out = v_out_fb;

    cmp = calc_cmp(0, v_in, v_out);

    pwr_i_in_lmt = div_pos_i32(p_ctrl_active_setpoint->pwr_lmt, v_in);
    in_curr_i_in_lmt = min_i32(p_ctrl_active_setpoint->in_curr_lmt, pwr_i_in_lmt);

    pi_tustin_i32_cal_a1_neg1_inline(&in_volt_lmt_loop);
    in_volt_i_l_lmt = in_volt_lmt_loop.output.val;

    /* Boost input current is the inductor current; output current is carried during upper-switch on-time. */
    in_curr_i_l_lmt = scale_ind_curr_to_k2(in_curr_i_in_lmt);
    out_curr_i_l_lmt = scale_ind_curr_to_k2(
        div_by_cmp_i32(p_ctrl_active_setpoint->out_curr_lmt, cmp));

    i_l_lmt = min_i32(in_volt_i_l_lmt, in_curr_i_l_lmt);
    i_l_lmt = min_i32(i_l_lmt, out_curr_i_l_lmt);

    param.i_l_lmt = i_l_lmt;
    isr_param_request_update(param);
}

REG_TASK(1, boost_task)

void boost_ctrl_set_p_hal(boost_ctrl_hal_t *p)
{
    (void)p;
}

void boost_ctrl_prepare_run(void)
{
    reinit_states();
}
