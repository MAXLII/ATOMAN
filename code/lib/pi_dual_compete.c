/*
 * @file    pi_dual_compete.c
 * @brief   双通道 PI 竞争模块.
 * @details
 *          This file is part of the PFC project.
 *
 *          Module responsibilities:
 *          - 两路 PI 控制器竞争输出，取较大/较小值作为实际输出
 *          - 共享同一个积分项，减少积分冲突和震荡
 *
 * @author  Max.Li
 * @date    2026-05-20
 * @version 1.0.0
 */

#include "pi_dual_compete.h"
#include <stddef.h>

static inline float pi_dual_compete_clamp(float x, float min, float max)
{
    if (x > max)
    {
        return max;
    }
    else if (x < min)
    {
        return min;
    }
    else
    {
        return x;
    }
}

bool pi_dual_compete_init(pi_dual_compete_t *p_str,
                          float kp_a,
                          float ki_a,
                          float kp_b,
                          float ki_b,
                          float up_lmt,
                          float dn_lmt,
                          PI_DUAL_COMPETE_MODE_E mode,
                          float *p_ref_a,
                          float *p_act_a,
                          float *p_ref_b,
                          float *p_act_b)
{
    if ((p_str == NULL) ||
        (p_ref_a == NULL) ||
        (p_act_a == NULL) ||
        (p_ref_b == NULL) ||
        (p_act_b == NULL) ||
        (up_lmt < dn_lmt))
    {
        return false;
    }

    p_str->input.p_ref_a = p_ref_a;
    p_str->input.p_act_a = p_act_a;
    p_str->input.p_ref_b = p_ref_b;
    p_str->input.p_act_b = p_act_b;

    p_str->cfg.up_lmt = up_lmt;
    p_str->cfg.dn_lmt = dn_lmt;
    p_str->cfg.mode = mode;

    if (!pi_dual_compete_update_a(p_str, kp_a, ki_a))
    {
        return false;
    }

    if (!pi_dual_compete_update_b(p_str, kp_b, ki_b))
    {
        return false;
    }

    pi_dual_compete_reset(p_str);

    return true;
}

bool pi_dual_compete_update_a(pi_dual_compete_t *p_str,
                              float kp,
                              float ki)
{
    if ((p_str == NULL) ||
        (kp < 0.0f) ||
        (ki < 0.0f))
    {
        return false;
    }

    p_str->cfg.loop_a.kp = kp;
    p_str->cfg.loop_a.ki = ki;

    return true;
}

bool pi_dual_compete_update_b(pi_dual_compete_t *p_str,
                              float kp,
                              float ki)
{
    if ((p_str == NULL) ||
        (kp < 0.0f) ||
        (ki < 0.0f))
    {
        return false;
    }

    p_str->cfg.loop_b.kp = kp;
    p_str->cfg.loop_b.ki = ki;

    return true;
}

bool pi_dual_compete_set_mode(pi_dual_compete_t *p_str,
                              PI_DUAL_COMPETE_MODE_E mode)
{
    if (p_str == NULL)
    {
        return false;
    }

    p_str->cfg.mode = mode;
    return true;
}

bool pi_dual_compete_cal(pi_dual_compete_t *p_str)
{
    if ((p_str == NULL) ||
        (p_str->input.p_ref_a == NULL) ||
        (p_str->input.p_act_a == NULL) ||
        (p_str->input.p_ref_b == NULL) ||
        (p_str->input.p_act_b == NULL))
    {
        return false;
    }

    /* 1. Error */
    const float err_a = (*p_str->input.p_ref_a) - (*p_str->input.p_act_a);
    const float err_b = (*p_str->input.p_ref_b) - (*p_str->input.p_act_b);

    /* 2. P term */
    const float p_a = p_str->cfg.loop_a.kp * err_a;
    const float p_b = p_str->cfg.loop_b.kp * err_b;

    /* 3. Outputs before deciding winner, both use same shared integral */
    float out_a = p_a + p_str->inter.i_share;
    float out_b = p_b + p_str->inter.i_share;

    /* 4. Competition */
    PI_DUAL_COMPETE_CH_E active_ch;
    float active_err;
    float active_ki;
    float active_p;
    float output;

    if (p_str->cfg.mode == PI_DUAL_COMPETE_MIN)
    {
        if (out_a <= out_b)
        {
            active_ch = PI_DUAL_CH_A;
            active_err = err_a;
            active_ki = p_str->cfg.loop_a.ki;
            active_p = p_a;
            output = out_a;
        }
        else
        {
            active_ch = PI_DUAL_CH_B;
            active_err = err_b;
            active_ki = p_str->cfg.loop_b.ki;
            active_p = p_b;
            output = out_b;
        }
    }
    else
    {
        if (out_a >= out_b)
        {
            active_ch = PI_DUAL_CH_A;
            active_err = err_a;
            active_ki = p_str->cfg.loop_a.ki;
            active_p = p_a;
            output = out_a;
        }
        else
        {
            active_ch = PI_DUAL_CH_B;
            active_err = err_b;
            active_ki = p_str->cfg.loop_b.ki;
            active_p = p_b;
            output = out_b;
        }
    }

    /* 5. Only active loop updates shared integrator */
    p_str->inter.i_share += active_ki * active_err;

    /* 6. Rebuild output after integral update */
    output = active_p + p_str->inter.i_share;

    /* 7. Saturation + anti-windup */
    if (output > p_str->cfg.up_lmt)
    {
        output = p_str->cfg.up_lmt;
        p_str->inter.i_share = p_str->cfg.up_lmt - active_p;
    }
    else if (output < p_str->cfg.dn_lmt)
    {
        output = p_str->cfg.dn_lmt;
        p_str->inter.i_share = p_str->cfg.dn_lmt - active_p;
    }

    /* 8. Clamp shared integrator */
    {
        const float i_max = p_str->cfg.up_lmt - active_p;
        const float i_min = p_str->cfg.dn_lmt - active_p;
        p_str->inter.i_share = pi_dual_compete_clamp(p_str->inter.i_share, i_min, i_max);
    }

    /* 9. Update visible outputs */
    out_a = p_a + p_str->inter.i_share;
    out_b = p_b + p_str->inter.i_share;

    /* 10. Save state */
    p_str->inter.err_a = err_a;
    p_str->inter.err_b = err_b;

    p_str->inter.p_a = p_a;
    p_str->inter.p_b = p_b;

    p_str->inter.out_a_raw = out_a;
    p_str->inter.out_b_raw = out_b;
    p_str->inter.active_ch = active_ch;

    p_str->output.val = output;
    p_str->output.val_a = out_a;
    p_str->output.val_b = out_b;

    return true;
}

void pi_dual_compete_reset(pi_dual_compete_t *p_str)
{
    if (p_str == NULL)
    {
        return;
    }

    p_str->inter.i_share = 0.0f;

    p_str->inter.err_a = 0.0f;
    p_str->inter.err_b = 0.0f;

    p_str->inter.p_a = 0.0f;
    p_str->inter.p_b = 0.0f;

    p_str->inter.out_a_raw = 0.0f;
    p_str->inter.out_b_raw = 0.0f;
    p_str->inter.active_ch = PI_DUAL_CH_A;

    p_str->output.val = 0.0f;
    p_str->output.val_a = 0.0f;
    p_str->output.val_b = 0.0f;
}
