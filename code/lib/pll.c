// SPDX-License-Identifier: MIT
/**
 * @file    pll.c
 * @brief   pll library module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement an alpha-beta input synchronous reference frame PLL
 *          - Calculate alpha-beta, dq, angular frequency, and phase angle outputs
 *          - Support runtime PI coefficient updates for grid synchronization
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe when caller owns the instance
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-07-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "pll.h"
#include "my_math.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

static float pll_limit(float val, float up_lmt, float dn_lmt)
{
    float result = val;

    if (result > up_lmt)
    {
        result = up_lmt;
    }
    else if (result < dn_lmt)
    {
        result = dn_lmt;
    }
    else
    {
        /* Keep result unchanged. */
    }

    return result;
}

static float pll_wrap_theta(float theta)
{
    float wrapped = theta;

    while (wrapped >= M_2PI)
    {
        wrapped -= M_2PI;
    }

    while (wrapped < 0.0f)
    {
        wrapped += M_2PI;
    }

    return wrapped;
}

static bool pll_cfg_update_pi_gain(pll_cfg_t *p_cfg)
{
    if ((p_cfg == NULL) ||
        (p_cfg->vm <= 0.0f) ||
        (p_cfg->zeta <= 0.0f) ||
        (p_cfg->omega_n <= 0.0f))
    {
        return false;
    }

    p_cfg->pi_kp = (2.0f * p_cfg->zeta * p_cfg->omega_n) / p_cfg->vm;
    p_cfg->pi_ki = (p_cfg->omega_n * p_cfg->omega_n) / p_cfg->vm;

    return true;
}

bool pll_init(pll_t *p_pll,
              float ts,
              float omega_center,
              float omega_up_lmt,
              float omega_dn_lmt,
              float vm,
              float zeta,
              float omega_n,
              float pi_up_lmt,
              float pi_dn_lmt,
              float theta_init,
              float *p_alpha,
              float *p_beta)
{
    if ((p_pll == NULL) ||
        (p_alpha == NULL) ||
        (p_beta == NULL) ||
        (ts <= 0.0f) ||
        (omega_center <= 0.0f) ||
        (omega_up_lmt < omega_dn_lmt))
    {
        return false;
    }

    (void)memset(p_pll, 0, sizeof(pll_t));

    p_pll->input.p_alpha = p_alpha;
    p_pll->input.p_beta = p_beta;
    p_pll->cfg.ts = ts;
    p_pll->cfg.omega_center = omega_center;
    p_pll->cfg.omega_up_lmt = omega_up_lmt;
    p_pll->cfg.omega_dn_lmt = omega_dn_lmt;
    p_pll->cfg.vm = vm;
    p_pll->cfg.zeta = zeta;
    p_pll->cfg.omega_n = omega_n;
    if (!pll_cfg_update_pi_gain(&p_pll->cfg))
    {
        return false;
    }

    p_pll->inter.theta = pll_wrap_theta(theta_init);
    p_pll->inter.pi_ref = 0.0f;
    p_pll->inter.pi_act = 0.0f;
    p_pll->output.omega = omega_center;
    p_pll->output.theta = p_pll->inter.theta;

    if (!pi_tustin_init(&p_pll->pi,
                        p_pll->cfg.pi_kp,
                        p_pll->cfg.pi_ki,
                        ts,
                        pi_up_lmt,
                        pi_dn_lmt,
                        &p_pll->inter.pi_ref,
                        &p_pll->inter.pi_act))
    {
        return false;
    }

    return true;
}

void pll_reset(pll_t *p_pll, float theta_init)
{
    if (p_pll == NULL)
    {
        return;
    }

    pi_tustin_reset(&p_pll->pi);

    p_pll->inter.pi_ref = 0.0f;
    p_pll->inter.pi_act = 0.0f;
    p_pll->inter.theta = pll_wrap_theta(theta_init);

    p_pll->inter.alpha = 0.0f;
    p_pll->inter.beta = 0.0f;
    p_pll->inter.vd = 0.0f;
    p_pll->inter.vq = 0.0f;
    p_pll->inter.vf = 0.0f;
    p_pll->output.omega = p_pll->cfg.omega_center;
    p_pll->output.theta = p_pll->inter.theta;
}

bool pll_update_pi(pll_t *p_pll, float pi_kp, float pi_ki)
{
    if (p_pll == NULL)
    {
        return false;
    }

    if (!pi_tustin_update(&p_pll->pi, pi_kp, pi_ki, p_pll->cfg.ts))
    {
        return false;
    }

    p_pll->cfg.pi_kp = pi_kp;
    p_pll->cfg.pi_ki = pi_ki;

    return true;
}

bool pll_update_tuning(pll_t *p_pll, float vm, float zeta, float omega_n)
{
    pll_cfg_t next_cfg = {0};

    if (p_pll == NULL)
    {
        return false;
    }

    next_cfg = p_pll->cfg;
    next_cfg.vm = vm;
    next_cfg.zeta = zeta;
    next_cfg.omega_n = omega_n;

    if (!pll_cfg_update_pi_gain(&next_cfg))
    {
        return false;
    }

    if (!pi_tustin_update(&p_pll->pi,
                          next_cfg.pi_kp,
                          next_cfg.pi_ki,
                          p_pll->cfg.ts))
    {
        return false;
    }

    p_pll->cfg = next_cfg;

    return true;
}

bool pll_cal(pll_t *p_pll)
{
    float sin_theta = 0.0f;
    float cos_theta = 0.0f;
    float omega = 0.0f;

    if ((p_pll == NULL) ||
        (p_pll->input.p_alpha == NULL) ||
        (p_pll->input.p_beta == NULL))
    {
        return false;
    }

    p_pll->inter.alpha = *p_pll->input.p_alpha;
    p_pll->inter.beta = *p_pll->input.p_beta;

    sin_theta = sinf(p_pll->inter.theta);
    cos_theta = cosf(p_pll->inter.theta);

    DQ_CAL(p_pll->inter.alpha,
           p_pll->inter.beta,
           sin_theta,
           cos_theta,
           p_pll->inter.vd,
           p_pll->inter.vq);

    p_pll->inter.pi_ref = 0.0f;
    p_pll->inter.pi_act = p_pll->inter.vq;

    if (!pi_tustin_cal(&p_pll->pi))
    {
        return false;
    }

    p_pll->inter.vf = p_pll->pi.output.val;
    omega = p_pll->cfg.omega_center + p_pll->inter.vf;
    p_pll->output.omega = pll_limit(omega,
                                    p_pll->cfg.omega_up_lmt,
                                    p_pll->cfg.omega_dn_lmt);

    p_pll->inter.theta += p_pll->output.omega * p_pll->cfg.ts;
    p_pll->inter.theta = pll_wrap_theta(p_pll->inter.theta);
    p_pll->output.theta = p_pll->inter.theta;

    return true;
}
