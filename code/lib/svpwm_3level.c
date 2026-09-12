// SPDX-License-Identifier: MIT
/**
 * @file    svpwm_3level.c
 * @brief   Center-aligned 3-level space-vector modulation.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Select adjacent phase levels using the TI SPRABS6 sector mapping
 *          - Solve the common-mode interval using measured split-bus voltages
 *          - Produce bounded phase dwell ratios or invalidate the whole result
 *
 *          Design notes:
 *          - C11 compatible; no dynamic allocation or hardware access
 *          - Caller owns the instance and provides exclusive access during calls
 *          - Dead time and neutral-point regulation are outside this module
 *
 * @author  Max.Li
 * @date    2026-09-12
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "svpwm_3level.h"

#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include "platform.h"

#define SVPWM_3LEVEL_SQRT3_HALF (0.8660254037844386f) /* Inverse Clarke beta coefficient. */
#define SVPWM_3LEVEL_ROUNDOFF (8.0f * FLT_EPSILON)    /* Tolerance in max-half-bus units. */

/* Set bits select O/P; clear bits select N/O. Bit order is A, B, C. */
static const uint8_t sector_masks[6] = {1u, 3u, 2u, 6u, 4u, 5u};

/**
 * @param p_svpwm Non-NULL instance whose previous result must be discarded.
 * @param status Reason why the output is not usable.
 * @return The supplied status.
 */
static inline SVPWM_3LEVEL_STATUS_E invalidate(svpwm_3level_t *p_svpwm, SVPWM_3LEVEL_STATUS_E status)
{
    p_svpwm->output = (svpwm_3level_output_t){0};
    p_svpwm->output.status = status;
    return status;
}

/**
 * @param value Fraction already constrained by a feasible common-mode interval.
 * @return Fraction clipped to [0, 1] to remove boundary roundoff only.
 */
static inline float bound_duty(float value)
{
    return fminf(1.0f, fmaxf(0.0f, value));
}

/**
 * @param p_phase Three zero-sequence-free phase references in max-half-bus units.
 * @param positive_bus Positive half-bus voltage in the same units; > 0.
 * @param negative_bus Negative half-bus magnitude in the same units; > 0.
 * @param mask Phase-level mapping: set bit means O/P, clear bit means N/O.
 * @param p_output Destination updated only if this mapping is feasible.
 * @return true if one common-mode offset satisfies all 3 phase intervals.
 */
static bool FUNC_RAM calculate_sector(const float *p_phase,
                                      float positive_bus,
                                      float negative_bus,
                                      uint8_t mask,
                                      svpwm_3level_output_t *p_output)
{
    float lower = -FLT_MAX;                      /* Greatest required common-mode lower bound. */
    float upper = FLT_MAX;                       /* Least permitted common-mode upper bound. */
    float offset;                                /* Shared offset leaves alpha/beta unchanged. */
    svpwm_3level_phase_output_t result[3] = {0}; /* Candidate phase ratios, committed together. */

    for (uint32_t index = 0u; index < 3u; ++index) /* Phase index in A, B, C order. */
    {
        if (((uint32_t)mask & (1u << index)) != 0u)
        {
            lower = fmaxf(lower, -p_phase[index]);
            upper = fminf(upper, positive_bus - p_phase[index]);
        }
        else
        {
            lower = fmaxf(lower, -negative_bus - p_phase[index]);
            upper = fminf(upper, -p_phase[index]);
        }
    }

    if ((lower - upper) > SVPWM_3LEVEL_ROUNDOFF)
    {
        return false;
    }

    /* Equal buses: this is the mapped 2-level min/max zero-sequence injection,
     * splitting the virtual zero dwell equally. Unequal buses: it centers the
     * available voltage margin, without assuming redundant vectors are equal. */
    offset = 0.5f * (lower + upper);
    for (uint32_t index = 0u; index < 3u; ++index) /* Phase index in A, B, C order. */
    {
        float pole = p_phase[index] + offset; /* Required average pole voltage in normalized units. */

        if (((uint32_t)mask & (1u << index)) != 0u)
        {
            result[index].duty_p = bound_duty(pole / positive_bus);
            result[index].duty_o = 1.0f - result[index].duty_p;
        }
        else
        {
            result[index].duty_n = bound_duty(-pole / negative_bus);
            result[index].duty_o = 1.0f - result[index].duty_n;
        }
    }

    p_output->phase_a = result[0];
    p_output->phase_b = result[1];
    p_output->phase_c = result[2];
    p_output->status = SVPWM_3LEVEL_OK;
    return true;
}

bool svpwm_3level_init(svpwm_3level_t *p_svpwm, const svpwm_3level_cfg_t *p_cfg)
{
    if (p_svpwm == NULL)
    {
        return false;
    }
    if (p_cfg == NULL)
    {
        *p_svpwm = (svpwm_3level_t){0};
        p_svpwm->output.status = SVPWM_3LEVEL_INVALID_ARGUMENT;
        return false;
    }

    /* Copy before clearing also permits reinitialization from the instance cfg. */
    svpwm_3level_cfg_t cfg = *p_cfg; /* Configuration snapshot owned by this call. */
    *p_svpwm = (svpwm_3level_t){0};
    if ((isfinite(cfg.v_dc_half_min) == 0) || /* Threshold must be a real voltage. */
        (cfg.v_dc_half_min <= 0.0f))          /* Prevent division by a zero half bus. */
    {
        p_svpwm->output.status = SVPWM_3LEVEL_INVALID_CONFIG;
        return false;
    }

    p_svpwm->cfg = cfg;
    return true;
}

SVPWM_3LEVEL_STATUS_E FUNC_RAM svpwm_3level_cal(svpwm_3level_t *p_svpwm)
{
    float scale;                 /* Maximum half bus, used without adding potentially large voltages. */
    float positive_bus;          /* Normalized positive half bus. */
    float negative_bus;          /* Normalized negative half-bus magnitude. */
    float alpha;                 /* Normalized alpha-axis reference. */
    float beta;                  /* Normalized beta-axis reference. */
    float phase[3];              /* Inverse Clarke result in A, B, C order. */
    uint8_t preferred_mask = 0u; /* Sign-based mapping from SPRABS6 Table 3. */

    if (p_svpwm == NULL)
    {
        return SVPWM_3LEVEL_INVALID_ARGUMENT;
    }
    if ((isfinite(p_svpwm->cfg.v_dc_half_min) == 0) || /* Detect corrupt or uninitialized configuration. */
        (p_svpwm->cfg.v_dc_half_min <= 0.0f))          /* Require a usable lower bound. */
    {
        return invalidate(p_svpwm, SVPWM_3LEVEL_INVALID_CONFIG);
    }
    if ((isfinite(p_svpwm->input.v_alpha) == 0) ||              /* Reject non-finite controller output. */
        (isfinite(p_svpwm->input.v_beta) == 0) ||               /* Both reference axes must be valid. */
        (isfinite(p_svpwm->input.v_dc_p) == 0) ||               /* Positive rail measurement must be finite. */
        (isfinite(p_svpwm->input.v_dc_n) == 0) ||               /* Negative rail measurement must be finite. */
        (p_svpwm->input.v_dc_p < p_svpwm->cfg.v_dc_half_min) || /* Enforce the positive half-bus floor. */
        (p_svpwm->input.v_dc_n < p_svpwm->cfg.v_dc_half_min))   /* Enforce the negative half-bus floor. */
    {
        return invalidate(p_svpwm, SVPWM_3LEVEL_INVALID_INPUT);
    }

    scale = fmaxf(p_svpwm->input.v_dc_p, p_svpwm->input.v_dc_n);
    positive_bus = p_svpwm->input.v_dc_p / scale;
    negative_bus = p_svpwm->input.v_dc_n / scale;
    if ((positive_bus <= 0.0f) || /* Reject a voltage ratio that underflows float. */
        (negative_bus <= 0.0f))   /* Keep both duty denominators representable. */
    {
        return invalidate(p_svpwm, SVPWM_3LEVEL_INVALID_INPUT);
    }

    alpha = p_svpwm->input.v_alpha / scale;
    beta = p_svpwm->input.v_beta / scale;
    if ((isfinite(alpha) == 0) || /* Division overflow denotes an infeasible reference. */
        (isfinite(beta) == 0) ||  /* Keep inverse Clarke arithmetic bounded. */
        (fabsf(alpha) > 2.0f) ||  /* Entire attainable hexagon lies inside this bound. */
        (fabsf(beta) > 2.0f))     /* Loose precheck avoids overflow, not modulation limiting. */
    {
        return invalidate(p_svpwm, SVPWM_3LEVEL_OUT_OF_RANGE);
    }

    phase[0] = alpha;
    phase[1] = (-0.5f * alpha) + (SVPWM_3LEVEL_SQRT3_HALF * beta);
    phase[2] = (-0.5f * alpha) - (SVPWM_3LEVEL_SQRT3_HALF * beta);
    for (uint32_t index = 0u; index < 3u; ++index) /* Phase index used to encode the preferred mapping. */
    {
        if (phase[index] > 0.0f)
        {
            preferred_mask = (uint8_t)((uint32_t)preferred_mask | (1u << index));
        }
    }

    if (preferred_mask == 0u)
    {
        /* Exactly zero reference uses OOO, avoiding unnecessary switching. */
        (void)invalidate(p_svpwm, SVPWM_3LEVEL_OK);
        p_svpwm->output.phase_a.duty_o = 1.0f;
        p_svpwm->output.phase_b.duty_o = 1.0f;
        p_svpwm->output.phase_c.duty_o = 1.0f;
        return SVPWM_3LEVEL_OK;
    }

    if (calculate_sector(phase, positive_bus, negative_bus, preferred_mask, &p_svpwm->output) == true)
    {
        return SVPWM_3LEVEL_OK;
    }

    /* An unequal split may move the feasible mapping away from the sign sector.
     * Trying the other 5 mappings keeps the full physical hexagon available. */
    for (uint32_t index = 0u; index < 6u; ++index) /* Fixed upper bound on alternative sector checks. */
    {
        if (sector_masks[index] == preferred_mask)
        {
            continue;
        }
        if (calculate_sector(phase, positive_bus, negative_bus, sector_masks[index], &p_svpwm->output) == true)
        {
            return SVPWM_3LEVEL_OK;
        }
    }

    return invalidate(p_svpwm, SVPWM_3LEVEL_OUT_OF_RANGE);
}

void svpwm_3level_reset(svpwm_3level_t *p_svpwm)
{
    if (p_svpwm != NULL)
    {
        (void)invalidate(p_svpwm, SVPWM_3LEVEL_NOT_READY);
    }
}
