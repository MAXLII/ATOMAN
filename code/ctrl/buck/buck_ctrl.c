// SPDX-License-Identifier: MIT
/**
 * @file    buck_ctrl.c
 * @brief   buck_ctrl control module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement buck voltage, current, and power-limit control paths
 *          - Prepare and reset PI controllers before entering run state
 *          - Consume HAL measurements and setpoints to generate one buck PWM command
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
#include "buck_ctrl.h"
#include "buck_cfg.h"
#include "pi_tustin_i32.h"
#include "section.h"
#include <stddef.h>

#define p_hal (p_ctrl_hal)

static pi_tustin_i32_t out_volt_loop = {0};
static pi_tustin_i32_t in_volt_lmt_loop = {0};
static pi_tustin_i32_t ind_curr_loop[BUCK_CTRL_IND_CURR_CH_NUM] = {0};

typedef struct
{
    /* Voltage-loop current limit kept in the K2 current domain for diagnostics and limiting. */
    int32_t i_l_lmt;
    /* Upper switch enable command latched into the ISR update point. */
    uint8_t up_en;
    /* Lower switch enable command latched into the ISR update point. */
    uint8_t dn_en;
} buck_ctrl_isr_param_t;

static buck_ctrl_hal_t *p_ctrl_hal = NULL;
static buck_ctrl_setpoint_t buck_ctrl_safe_setpoint = {0};
static buck_ctrl_setpoint_t *p_ctrl_active_setpoint = &buck_ctrl_safe_setpoint;
static volatile buck_ctrl_isr_param_t buck_ctrl_isr_param = {
    .i_l_lmt = 0,
    .up_en = 1U,
    .dn_en = 1U,
};
static volatile buck_ctrl_isr_param_t buck_ctrl_isr_param_pending[2] = {
    {
        .i_l_lmt = 0,
        .up_en = 1U,
        .dn_en = 1U,
    },
    {
        .i_l_lmt = 0,
        .up_en = 1U,
        .dn_en = 1U,
    },
};
static volatile uint8_t buck_ctrl_isr_param_pending_idx = 0U;
static volatile uint32_t buck_ctrl_isr_param_publish_seq = 0U;
static uint32_t buck_ctrl_isr_param_applied_seq = 0U;
static int32_t ind_curr_ref = 0;
static uint8_t buck_ctrl_run_active = 0U;

#if (BUCK_CTRL_IND_CURR_CH_NUM == 2U)
/* Shift used to convert the K2-domain total current command into a per-channel current code. */
#define BUCK_CTRL_IND_CURR_REF_SHIFT (BUCK_CTRL_K2_IND_CURR_FB_SHIFT + 1U)
#endif

static inline void buck_ctrl_isr_param_request_update(buck_ctrl_isr_param_t param)
{
    uint8_t pending_idx = 0U;

    pending_idx = (uint8_t)(buck_ctrl_isr_param_pending_idx ^ 1U);
    buck_ctrl_isr_param_pending[pending_idx].i_l_lmt = param.i_l_lmt;
    buck_ctrl_isr_param_pending[pending_idx].up_en = param.up_en;
    buck_ctrl_isr_param_pending[pending_idx].dn_en = param.dn_en;
    buck_ctrl_isr_param_pending_idx = pending_idx;
    buck_ctrl_isr_param_publish_seq++;
}

static uint8_t buck_ctrl_channel_ready(void)
{
    uint32_t ch = 0U;

    if (p_hal == NULL)
    {
        return 0U;
    }

    for (ch = 0U; ch < BUCK_CTRL_IND_CURR_CH_NUM; ch++)
    {
        if ((p_hal->p_i_l[ch] == NULL) ||
            (p_hal->p_set_pwm_func[ch] == NULL))
        {
            return 0U;
        }
    }

    return 1U;
}

static inline int32_t buck_ctrl_limit_cmp(int32_t cmp)
{
    if (cmp > BUCK_CTRL_CMP_MAX)
    {
        cmp = BUCK_CTRL_CMP_MAX;
    }
    else if (cmp < BUCK_CTRL_CMP_MIN)
    {
        cmp = BUCK_CTRL_CMP_MIN;
    }

    return (int32_t)cmp;
}

static inline int32_t buck_ctrl_calc_cmp(int32_t v_l_cmd, int32_t v_in, int32_t v_out)
{
    /* Compare numerator after applying output-voltage feedforward compensation. */
    int64_t numerator_i64 = 0;

    /* Compare numerator saturated before division to keep the divider 32-bit. */
    int32_t numerator = 0;

    /* Compare command before final output limiting. */
    int32_t cmp = 0;

    if (v_in <= 0)
    {
        return BUCK_CTRL_CMP_MIN;
    }

    numerator_i64 = (int64_t)v_l_cmd +
                    ((int64_t)v_out * (int64_t)BUCK_CTRL_OUT_VOLT_LOOP_V_OUT_FF_K);
    numerator = pi_tustin_i32_sat_i64_to_i32(numerator_i64);
    cmp = numerator / v_in;

    return buck_ctrl_limit_cmp(cmp);
}

static inline int32_t buck_ctrl_scale_ind_curr_to_k2(int32_t i_l)
{
    /* Current command converted into the K2 current domain. */
    int64_t scaled = 0;

    scaled = (int64_t)i_l * (int64_t)BUCK_CTRL_IND_CURR_LOOP_FB_K;

    return pi_tustin_i32_sat_i64_to_i32(scaled);
}

static inline int32_t buck_ctrl_min_i32(int32_t a, int32_t b)
{
    if (a < b)
    {
        return a;
    }

    return b;
}

static inline int32_t buck_ctrl_div_pos_i32(int32_t numerator, int32_t denominator)
{
    if (denominator <= 0)
    {
        return numerator;
    }

    return numerator / denominator;
}

static inline int32_t buck_ctrl_div_by_cmp_i32(int32_t numerator, int32_t cmp)
{
    /* Scaled numerator saturated before division so the runtime divider stays 32-bit. */
    int64_t scaled = 0;
    int32_t scaled_i32 = 0;

    if (cmp <= 0)
    {
        return numerator;
    }

    scaled = (int64_t)numerator * (int64_t)BUCK_CTRL_OUT_VOLT_LOOP_V_OUT_FF_K;
    scaled_i32 = pi_tustin_i32_sat_i64_to_i32(scaled);

    return scaled_i32 / cmp;
}

static inline int32_t buck_ctrl_limit_pos_i32(int32_t val)
{
    if (val < 0)
    {
        return 0;
    }

    return val;
}

static void buck_ctrl_reinit_states(void)
{
    buck_ctrl_setpoint_t *p_active_setpoint = NULL;
    uint32_t ch = 0U;

    p_ctrl_hal = buck_hal_get_ctrl();
    buck_cfg_sync_building_to_active();
    p_active_setpoint = buck_cfg_get_p_active();

    if ((p_hal == NULL) ||
        (p_active_setpoint == NULL) ||
        (p_hal->p_v_in == NULL) ||
        (p_hal->p_v_out == NULL) ||
        (buck_ctrl_channel_ready() == 0U))
    {
        return;
    }

    buck_ctrl_run_active = 0U;
    p_ctrl_active_setpoint = p_active_setpoint;
    ind_curr_ref = 0;

    (void)pi_tustin_i32_init(&out_volt_loop,
                             BUCK_CTRL_OUT_VOLT_LOOP_KP,
                             BUCK_CTRL_OUT_VOLT_LOOP_KI,
                             BUCK_CTRL_TS,
                             BUCK_CTRL_OUT_VOLT_LOOP_UP_LMT,
                             BUCK_CTRL_OUT_VOLT_LOOP_DN_LMT,
                             &p_active_setpoint->out_volt_ref,
                             p_hal->p_v_out);

    (void)pi_tustin_i32_init(&in_volt_lmt_loop,
                             BUCK_CTRL_IN_VOLT_LMT_LOOP_KP,
                             BUCK_CTRL_IN_VOLT_LMT_LOOP_KI,
                             BUCK_CTRL_TASK_TS,
                             BUCK_CTRL_IN_VOLT_LMT_LOOP_UP_LMT,
                             BUCK_CTRL_IN_VOLT_LMT_LOOP_DN_LMT,
                             p_hal->p_v_in,
                             &p_active_setpoint->in_volt_lmt);

    for (ch = 0U; ch < BUCK_CTRL_IND_CURR_CH_NUM; ch++)
    {
        (void)pi_tustin_i32_init(&ind_curr_loop[ch],
                                 BUCK_CTRL_IND_CURR_LOOP_KP,
                                 BUCK_CTRL_IND_CURR_LOOP_KI,
                                 BUCK_CTRL_TS,
                                 BUCK_CTRL_IND_CURR_LOOP_UP_LMT,
                                 BUCK_CTRL_IND_CURR_LOOP_DN_LMT,
                                 &ind_curr_ref,
                                 p_hal->p_i_l[ch]);
    }
}

static void buck_ctrl_init(void)
{
    buck_ctrl_reinit_states();
}

REG_INIT(0, buck_ctrl_init)

static void FUNC_RAM buck_ctrl_isr(void)
{
    buck_ctrl_isr_param_t isr_param = {0};
    uint8_t pending_idx = 0U;
    uint32_t publish_seq = 0U;
    int32_t v_in = 0;
    int32_t v_out = 0;
    int32_t cmp = 0;
    int32_t cmp_calc = 0;
    int32_t cmp_numerator = 0;
    /* Voltage-loop total current reference before channel split. */
    int32_t i_l_ref_total = 0;
    int32_t v_out_ff = 0;

    buck_cfg_sync_building_to_active_fast();

    /* Commit the task-built ISR parameter snapshot at the PWM update point. */
    publish_seq = buck_ctrl_isr_param_publish_seq;
    if (buck_ctrl_isr_param_applied_seq != publish_seq)
    {
        pending_idx = buck_ctrl_isr_param_pending_idx;
        buck_ctrl_isr_param.i_l_lmt = buck_ctrl_isr_param_pending[pending_idx].i_l_lmt;
        buck_ctrl_isr_param.up_en = buck_ctrl_isr_param_pending[pending_idx].up_en;
        buck_ctrl_isr_param.dn_en = buck_ctrl_isr_param_pending[pending_idx].dn_en;
        buck_ctrl_isr_param_applied_seq = publish_seq;
    }

    if (p_ctrl_active_setpoint->run_allowed == 0U)
    {
        if (buck_ctrl_run_active != 0U)
        {
            p_hal->p_pwm_disable();
            buck_ctrl_run_active = 0U;
        }
        return;
    }

    buck_ctrl_run_active = 1U;

    isr_param.i_l_lmt = buck_ctrl_isr_param.i_l_lmt;
    isr_param.up_en = buck_ctrl_isr_param.up_en;
    isr_param.dn_en = buck_ctrl_isr_param.dn_en;

    v_in = *p_hal->p_v_in;
    v_out = *p_hal->p_v_out;

    /* Voltage loop now runs at the PWM interrupt rate and publishes the current-loop reference directly. */
    out_volt_loop.inter.up_lmt = isr_param.i_l_lmt;
    pi_tustin_i32_cal_a1_neg1_inline(&out_volt_loop);
    i_l_ref_total = out_volt_loop.output.val;

    if (out_volt_loop.output.val < 0)
    {
        pi_tustin_i32_reset_inline(&out_volt_loop);
        i_l_ref_total = 0;
    }

#if defined(BUCK_CTRL_IND_CURR_REF_SHIFT)
    if (i_l_ref_total <= 0)
    {
        ind_curr_ref = 0;
    }
    else
    {
        ind_curr_ref = (int32_t)((uint32_t)i_l_ref_total >> BUCK_CTRL_IND_CURR_REF_SHIFT);
    }
#else
    {
        /* Raw denominator for converting total current reference to each channel. */
        int64_t denominator = 0;
        /* Saturated denominator used by the 32-bit divider. */
        int32_t denominator_i32 = 0;

        denominator = (int64_t)BUCK_CTRL_IND_CURR_LOOP_FB_K * (int64_t)BUCK_CTRL_IND_CURR_CH_NUM;
        denominator_i32 = pi_tustin_i32_sat_i64_to_i32(denominator);
        if ((i_l_ref_total <= 0) ||
            (denominator_i32 <= 0))
        {
            ind_curr_ref = 0;
        }
        else
        {
            ind_curr_ref = i_l_ref_total / denominator_i32;
        }
    }
#endif

    if (v_in <= 0)
    {
#if (BUCK_CTRL_IND_CURR_CH_NUM == 2U)
        p_hal->p_set_pwm_func[0](BUCK_CTRL_CMP_MIN, isr_param.up_en, isr_param.dn_en);
        p_hal->p_set_pwm_func[1](BUCK_CTRL_CMP_MIN, isr_param.up_en, isr_param.dn_en);
#else
        uint32_t ch = 0U;

        for (ch = 0U; ch < BUCK_CTRL_IND_CURR_CH_NUM; ch++)
        {
            p_hal->p_set_pwm_func[ch](BUCK_CTRL_CMP_MIN, isr_param.up_en, isr_param.dn_en);
        }
#endif
        return;
    }

    v_out_ff = pi_tustin_i32_sat_i64_to_i32((int64_t)v_out *
                                            (int64_t)BUCK_CTRL_OUT_VOLT_LOOP_V_OUT_FF_K);

#if (BUCK_CTRL_IND_CURR_CH_NUM == 2U)
    pi_tustin_i32_cal_a1_neg1_inline(&ind_curr_loop[0]);
    cmp_numerator = pi_tustin_i32_sat_i64_to_i32((int64_t)ind_curr_loop[0].output.val +
                                                 (int64_t)v_out_ff);
    cmp_calc = cmp_numerator / v_in;
    if (cmp_calc > BUCK_CTRL_CMP_MAX)
    {
        cmp = BUCK_CTRL_CMP_MAX;
    }
    else if (cmp_calc < BUCK_CTRL_CMP_MIN)
    {
        cmp = BUCK_CTRL_CMP_MIN;
    }
    else
    {
        cmp = (int32_t)cmp_calc;
    }
    p_hal->p_set_pwm_func[0](cmp, isr_param.up_en, isr_param.dn_en);

    pi_tustin_i32_cal_a1_neg1_inline(&ind_curr_loop[1]);
    cmp_numerator = pi_tustin_i32_sat_i64_to_i32((int64_t)ind_curr_loop[1].output.val +
                                                 (int64_t)v_out_ff);
    cmp_calc = cmp_numerator / v_in;
    if (cmp_calc > BUCK_CTRL_CMP_MAX)
    {
        cmp = BUCK_CTRL_CMP_MAX;
    }
    else if (cmp_calc < BUCK_CTRL_CMP_MIN)
    {
        cmp = BUCK_CTRL_CMP_MIN;
    }
    else
    {
        cmp = (int32_t)cmp_calc;
    }
    p_hal->p_set_pwm_func[1](cmp, isr_param.up_en, isr_param.dn_en);
#else
    uint32_t ch = 0U;
    for (ch = 0U; ch < BUCK_CTRL_IND_CURR_CH_NUM; ch++)
    {
        pi_tustin_i32_cal_a1_neg1_inline(&ind_curr_loop[ch]);
        cmp_numerator = pi_tustin_i32_sat_i64_to_i32((int64_t)ind_curr_loop[ch].output.val +
                                                     (int64_t)v_out_ff);
        cmp_calc = cmp_numerator / v_in;
        if (cmp_calc > BUCK_CTRL_CMP_MAX)
        {
            cmp = BUCK_CTRL_CMP_MAX;
        }
        else if (cmp_calc < BUCK_CTRL_CMP_MIN)
        {
            cmp = BUCK_CTRL_CMP_MIN;
        }
        else
        {
            cmp = (int32_t)cmp_calc;
        }
        p_hal->p_set_pwm_func[ch](cmp, isr_param.up_en, isr_param.dn_en);
    }
#endif
}

REG_INTERRUPT(3, buck_ctrl_isr)

static void buck_ctrl_task(void)
{
    buck_ctrl_isr_param_t param = {0};
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
        return;
    }

    v_in = *p_hal->p_v_in;
    v_out = *p_hal->p_v_out;

    cmp = buck_ctrl_calc_cmp(0, v_in, v_out);
    pwr_i_in_lmt = buck_ctrl_div_pos_i32(p_ctrl_active_setpoint->pwr_lmt, v_in);
    in_curr_i_in_lmt = buck_ctrl_min_i32(p_ctrl_active_setpoint->in_curr_lmt, pwr_i_in_lmt);

    pi_tustin_i32_cal_a1_neg1_inline(&in_volt_lmt_loop);
    in_volt_i_l_lmt = buck_ctrl_limit_pos_i32(in_volt_lmt_loop.output.val);

    /* Candidate current limits are compared in the K2 current domain. */
    in_curr_i_l_lmt = buck_ctrl_scale_ind_curr_to_k2(
        buck_ctrl_div_by_cmp_i32(in_curr_i_in_lmt, cmp));
    out_curr_i_l_lmt = buck_ctrl_scale_ind_curr_to_k2(
        buck_ctrl_limit_pos_i32(p_ctrl_active_setpoint->out_curr_lmt));

    i_l_lmt = buck_ctrl_min_i32(in_volt_i_l_lmt, in_curr_i_l_lmt);
    i_l_lmt = buck_ctrl_min_i32(i_l_lmt, out_curr_i_l_lmt);

    param.i_l_lmt = buck_ctrl_limit_pos_i32(i_l_lmt);
    param.up_en = 1U;
    param.dn_en = 1U;
    buck_ctrl_isr_param_request_update(param);
}

REG_TASK(1, buck_ctrl_task)

void buck_ctrl_set_p_hal(buck_ctrl_hal_t *p)
{
    (void)p;
}

void buck_ctrl_prepare_run(void)
{
    buck_ctrl_reinit_states();
}
