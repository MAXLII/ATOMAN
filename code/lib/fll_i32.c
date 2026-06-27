// SPDX-License-Identifier: MIT
/**
 * @file    fll_i32.c
 * @brief   Integer FLL library module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Convert FLL floating tuning parameters into fixed-point gain
 *          - Run integer frequency adaptation from SOGI in-phase/quadrature outputs
 *          - Saturate FLL integral and angular-frequency outputs
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe when caller owns the instance
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
#include "fll_i32.h"
#include <limits.h>
#include <stddef.h>

static int32_t fll_i32_limit_i32(int32_t val, int32_t up_lmt, int32_t dn_lmt)
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

static int32_t fll_i32_sat_i64_to_i32(int64_t val)
{
    if (val > (int64_t)INT32_MAX)
    {
        return INT32_MAX;
    }
    else if (val < (int64_t)INT32_MIN)
    {
        return INT32_MIN;
    }

    return (int32_t)val;
}

static int32_t fll_i32_float_to_i32(float val)
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

void fll_i32_reset(fll_i32_t *p_fll)
{
    if (p_fll == NULL)
    {
        return;
    }

    p_fll->output.omega_mradps = p_fll->inter.omega_init_mradps;
    p_fll->output.i_err_mradps = 0;
}

bool fll_i32_init(fll_i32_t *p_fll,
                  float gamma,
                  float ts,
                  int32_t omega_init_mradps,
                  int32_t omega_max_mradps,
                  int32_t omega_min_mradps,
                  int32_t i_err_max_mradps,
                  int32_t i_err_min_mradps,
                  uint8_t gain_q_shift,
                  int32_t *p_v,
                  int32_t *p_qv,
                  int32_t *p_epsilon)
{
    if ((p_fll == NULL) ||
        (p_v == NULL) ||
        (p_qv == NULL) ||
        (p_epsilon == NULL) ||
        (omega_max_mradps < omega_min_mradps) ||
        (i_err_max_mradps < i_err_min_mradps))
    {
        return false;
    }

    p_fll->input.p_v = p_v;
    p_fll->input.p_qv = p_qv;
    p_fll->input.p_epsilon = p_epsilon;
    p_fll->inter.omega_init_mradps = omega_init_mradps;
    p_fll->inter.omega_max_mradps = omega_max_mradps;
    p_fll->inter.omega_min_mradps = omega_min_mradps;
    p_fll->inter.i_err_max_mradps = i_err_max_mradps;
    p_fll->inter.i_err_min_mradps = i_err_min_mradps;
    fll_i32_reset(p_fll);

    return fll_i32_update(p_fll, gamma, ts, gain_q_shift);
}

bool fll_i32_update(fll_i32_t *p_fll,
                    float gamma,
                    float ts,
                    uint8_t gain_q_shift)
{
    if ((p_fll == NULL) ||
        (gamma < 0.0f) ||
        (ts <= 0.0f) ||
        (gain_q_shift >= 30U))
    {
        return false;
    }

    p_fll->inter.gain_q_shift = gain_q_shift;
    p_fll->inter.gain_q = fll_i32_float_to_i32(gamma *
                                               1.414f *
                                               ts *
                                               (float)(1L << gain_q_shift));

    return true;
}

bool fll_i32_cal(fll_i32_t *p_fll)
{
    int64_t qvv = 0;
    int64_t err_qv_q = 0;
    int64_t omega_gain = 0;
    int64_t gain_q = 0;
    int64_t delta = 0;
    int32_t v = 0;
    int32_t qv = 0;
    int32_t epsilon = 0;

    if ((p_fll == NULL) ||
        (p_fll->input.p_v == NULL) ||
        (p_fll->input.p_qv == NULL) ||
        (p_fll->input.p_epsilon == NULL))
    {
        return false;
    }

    v = *p_fll->input.p_v;
    qv = *p_fll->input.p_qv;
    epsilon = *p_fll->input.p_epsilon;

    qvv = (int64_t)v * (int64_t)v + (int64_t)qv * (int64_t)qv;
    if (qvv <= 0)
    {
        return false;
    }

    gain_q = (int64_t)(1L << p_fll->inter.gain_q_shift);
    err_qv_q = ((int64_t)epsilon * (int64_t)qv * gain_q) / qvv;
    omega_gain = ((int64_t)p_fll->output.omega_mradps * (int64_t)p_fll->inter.gain_q) >>
                 p_fll->inter.gain_q_shift;
    delta = -((omega_gain * err_qv_q) >> p_fll->inter.gain_q_shift);

    p_fll->output.i_err_mradps = fll_i32_limit_i32(
        fll_i32_sat_i64_to_i32((int64_t)p_fll->output.i_err_mradps + delta),
        p_fll->inter.i_err_max_mradps,
        p_fll->inter.i_err_min_mradps);
    p_fll->output.omega_mradps = fll_i32_limit_i32(
        p_fll->inter.omega_init_mradps + p_fll->output.i_err_mradps,
        p_fll->inter.omega_max_mradps,
        p_fll->inter.omega_min_mradps);

    return true;
}
