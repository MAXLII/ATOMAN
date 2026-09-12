// SPDX-License-Identifier: MIT
/**
 * @file    dsogi.c
 * @brief   Two existing Tustin SOGIs and stationary-frame sequence calculation.
 * @details
 *          This file is part of the base digital power framework project.
 *          Caller-owned state; C11; no dynamic allocation or hardware access.
 * @author  Max.Li
 * @date    2026-09-12
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "dsogi.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

/** @param p_cfg Candidate configuration. @return true within the supported float coefficient envelope. */
static inline bool config_valid(const dsogi_cfg_t *p_cfg)
{
    if (p_cfg == NULL)
    {
        return false;
    }
    return (p_cfg->ts >= 1.0e-6f) &&                   /* Bound coefficient products. */
           (p_cfg->ts <= 0.01f) &&                     /* Bound coefficient products. */
           (p_cfg->k >= 0.1f) &&                       /* Positive damping. */
           (p_cfg->k <= 4.0f) &&                       /* Supported damping range. */
           (p_cfg->omega_min >= 1.0f) &&               /* Positive center frequency. */
           (p_cfg->omega_max <= 100000.0f) &&          /* Bound coefficient products. */
           (p_cfg->omega_max >= p_cfg->omega_min) &&   /* Ordered interval. */
           (p_cfg->omega_min * p_cfg->ts >= 0.001f) && /* Avoid float pole cancellation. */
           (p_cfg->omega_max * p_cfg->ts <= 1.0f);     /* Sampling margin below Nyquist. */
}

void dsogi_reset(dsogi_t *p_dsogi)
{
    if (p_dsogi == NULL)
    {
        return;
    }
    p_dsogi->output = (dsogi_output_t){0};
    if (p_dsogi->inter.initialized == true)
    {
        sogi_init(&p_dsogi->inter.alpha, p_dsogi->cfg.ts, p_dsogi->cfg.omega_min,
                  p_dsogi->cfg.k, p_dsogi->input.p_alpha);
        sogi_init(&p_dsogi->inter.beta, p_dsogi->cfg.ts, p_dsogi->cfg.omega_min,
                  p_dsogi->cfg.k, p_dsogi->input.p_beta);
    }
}

bool dsogi_init(dsogi_t *p_dsogi, const dsogi_cfg_t *p_cfg,
                float *p_alpha, float *p_beta, float *p_omega)
{
    dsogi_cfg_t cfg = {0}; /* Copy before clearing allows reinit with the instance's own cfg. */
    if (p_dsogi == NULL)
    {
        return false;
    }
    if (p_cfg != NULL)
    {
        cfg = *p_cfg;
    }
    (void)memset(p_dsogi, 0, sizeof(*p_dsogi));
    if ((config_valid(&cfg) == false) || /* Reject invalid parameters, including NaN. */
        (p_alpha == NULL) ||             /* Require all bound sources. */
        (p_beta == NULL) ||              /* Require all bound sources. */
        (p_omega == NULL))               /* Require the external frequency source. */
    {
        return false;
    }
    if (((*p_omega >= cfg.omega_min) == false) || /* Reject NaN and values below range. */
        ((*p_omega <= cfg.omega_max) == false))   /* Reject infinity and values above range. */
    {
        return false;
    }
    p_dsogi->input = (dsogi_input_t){.p_alpha = p_alpha, .p_beta = p_beta, .p_omega = p_omega};
    p_dsogi->cfg = cfg;
    p_dsogi->inter.initialized = true;
    dsogi_reset(p_dsogi);
    return true;
}

bool dsogi_cal(dsogi_t *p_dsogi)
{
    float omega = 0.0f; /* Frequency snapshot shared by both axes. */
    if (p_dsogi == NULL)
    {
        return false;
    }
    if ((p_dsogi->inter.initialized == false) || /* Initialization is mandatory. */
        (p_dsogi->input.p_alpha == NULL) ||      /* Bindings must remain valid. */
        (p_dsogi->input.p_beta == NULL) ||       /* Bindings must remain valid. */
        (p_dsogi->input.p_omega == NULL))        /* Bindings must remain valid. */
    {
        dsogi_reset(p_dsogi);
        return false;
    }
    omega = *p_dsogi->input.p_omega;
    if ((isfinite(*p_dsogi->input.p_alpha) == 0) ||     /* Invalid voltage must not poison history. */
        (isfinite(*p_dsogi->input.p_beta) == 0) ||      /* Invalid voltage must not poison history. */
        ((omega >= p_dsogi->cfg.omega_min) == false) || /* Reject NaN and frequency below range. */
        ((omega <= p_dsogi->cfg.omega_max) == false))   /* Reject infinity and frequency above range. */
    {
        dsogi_reset(p_dsogi);
        return false;
    }
    if (omega != p_dsogi->inter.alpha.w)
    {
        sogi_update_frequency(&p_dsogi->inter.alpha, omega);
        sogi_update_frequency(&p_dsogi->inter.beta, omega);
    }
    sogi_cal(&p_dsogi->inter.alpha);
    sogi_cal(&p_dsogi->inter.beta);
    p_dsogi->output.alpha_pos = 0.5f * p_dsogi->inter.alpha.osg_u[0] - 0.5f * p_dsogi->inter.beta.osg_qu[0];
    p_dsogi->output.beta_pos = 0.5f * p_dsogi->inter.beta.osg_u[0] + 0.5f * p_dsogi->inter.alpha.osg_qu[0];
    p_dsogi->output.alpha_neg = 0.5f * p_dsogi->inter.alpha.osg_u[0] + 0.5f * p_dsogi->inter.beta.osg_qu[0];
    p_dsogi->output.beta_neg = 0.5f * p_dsogi->inter.beta.osg_u[0] - 0.5f * p_dsogi->inter.alpha.osg_qu[0];
    if ((isfinite(p_dsogi->output.alpha_pos) == 0) || /* Catch arithmetic overflow. */
        (isfinite(p_dsogi->output.beta_pos) == 0) ||  /* Catch arithmetic overflow. */
        (isfinite(p_dsogi->output.alpha_neg) == 0) || /* Catch arithmetic overflow. */
        (isfinite(p_dsogi->output.beta_neg) == 0))    /* Catch arithmetic overflow. */
    {
        dsogi_reset(p_dsogi);
        return false;
    }
    return true;
}
