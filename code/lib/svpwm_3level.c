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
#include "my_math.h"

#define SVPWM_3LEVEL_ROUNDOFF (8.0f * FLT_EPSILON) /* Tolerance in max-half-bus units. */
#define SVPWM_3LEVEL_CURRENT_ROUNDOFF (32.0f * FLT_EPSILON) /* Relative current comparison tolerance. */

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
 * @param p_current Three bridge-to-AC phase currents, A.
 * @param midpoint_ref Desired mean current out of the midpoint, A.
 * @param balance_enabled Enable current-based offset selection.
 * @param p_output Destination updated only if this mapping is feasible.
 * @return true if one common-mode offset satisfies all 3 phase intervals.
 */
static bool FUNC_RAM calculate_sector(const float *p_phase,
                                      float positive_bus,
                                      float negative_bus,
                                      uint8_t mask,
                                      const float *p_current,
                                      float midpoint_ref,
                                      bool balance_enabled,
                                      svpwm_3level_output_t *p_output)
{
    float lower = -FLT_MAX;                      /* Greatest required common-mode lower bound. */
    float upper = FLT_MAX;                       /* Least permitted common-mode upper bound. */
    float offset = 0.0f;                         /* Shared offset leaves alpha/beta unchanged. */
    float midpoint_center = 0.0f;                /* Current at the centered offset, A. */
    float midpoint_slope = 0.0f;                 /* Current change per normalized offset, A. */
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
    if (balance_enabled == true)
    {
        for (uint32_t index = 0u; index < 3u; ++index) /* Build the affine midpoint-current relation. */
        {
            float pole = p_phase[index] + offset; /* Centered normalized pole voltage. */

            if (((uint32_t)mask & (1u << index)) != 0u)
            {
                midpoint_center += (1.0f - pole / positive_bus) * p_current[index];
                midpoint_slope -= p_current[index] / positive_bus;
            }
            else
            {
                midpoint_center += (1.0f + pole / negative_bus) * p_current[index];
                midpoint_slope += p_current[index] / negative_bus;
            }
        }
        if ((midpoint_slope != 0.0f) && /* A zero slope means no current authority within this mapping. */
            (upper > lower))           /* A collapsed interval cannot move the common-mode offset. */
        {
            offset += (midpoint_ref - midpoint_center) / midpoint_slope;
            offset = fmaxf(lower, fminf(upper, offset));
        }
    }
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
    p_output->midpoint_current_ref = midpoint_ref;
    p_output->midpoint_current = result[0].duty_o * p_current[0] +
                                 result[1].duty_o * p_current[1] +
                                 result[2].duty_o * p_current[2];
    p_output->common_mode_v = offset; /* Caller converts the normalized value to volts. */
    return true;
}

/**
 * @brief Use slow bus-difference feedback to shift a continuous centered common mode.
 * @param p_svpwm Initialized modulator with coherent phase currents and bus voltages.
 * @param p_phase Normalized inverse-Clarke phase commands.
 * @param p_current Fundamental currents, positive from bridge to AC side.
 * @param scale Voltage normalization base, V.
 * @return Modulation status; an infeasible voltage span clears all output duties.
 */
static SVPWM_3LEVEL_STATUS_E FUNC_RAM calculate_average_balance(
    svpwm_3level_t *p_svpwm, const float *p_phase, const float *p_current, float scale)
{
    const svpwm_3level_cfg_t *p_cfg = &p_svpwm->cfg;
    svpwm_3level_inter_t *p_inter = &p_svpwm->inter;
    svpwm_3level_phase_output_t *phase_output[3] = {
        &p_svpwm->output.phase_a, &p_svpwm->output.phase_b, &p_svpwm->output.phase_c};
    float vp = p_svpwm->input.v_dc_p;
    float vn = p_svpwm->input.v_dc_n;
    float minimum = fminf(p_phase[0], fminf(p_phase[1], p_phase[2])) * scale;
    float maximum = fmaxf(p_phase[0], fmaxf(p_phase[1], p_phase[2])) * scale;
    float offset_min = -vn - minimum;
    float offset_max = vp - maximum;
    float centered = 0.5f * (vp - vn - minimum - maximum);
    float current_sum = fabsf(p_current[0]) + fabsf(p_current[1]) + fabsf(p_current[2]);
    float authority = 0.0f; /* Inverse approximate neutral-current slope, V/A. */
    float sensitivity = 0.0f; /* Negative derivative of neutral current with respect to common-mode shift, A/V. */
    float slope_floor = 0.0f; /* Regularization near a phase combination with no midpoint authority, A/V. */
    float correction_current = 0.0f;
    float correction = 0.0f;
    float offset = 0.0f;

    if (offset_min > offset_max)
    {
        return invalidate(p_svpwm, SVPWM_3LEVEL_OUT_OF_RANGE);
    }
    p_inter->delta_filtered += p_inter->filter_coeff * (vp - vn - p_inter->delta_filtered);
    p_inter->current_filtered += p_inter->filter_coeff * (current_sum - p_inter->current_filtered);
    for (uint32_t index = 0u; index < 3u; ++index)
    {
        float pole = p_phase[index] * scale + centered;
        /* A capacitive or regenerative phase can reverse the balancing direction. */
        sensitivity += (pole >= 0.0f) ? p_current[index] / vp : -p_current[index] / vn;
    }
    slope_floor = fmaxf(p_cfg->midpoint_current_min,
                        p_cfg->midpoint_slope_floor_ratio * p_inter->current_filtered) /
                  (0.5f * (vp + vn));
    authority = sensitivity / (sensitivity * sensitivity + slope_floor * slope_floor);
    if (p_cfg->midpoint_kp > 0.0f)
    {
        correction_current = p_cfg->midpoint_kp * p_inter->delta_filtered + p_inter->balance_integral;
    }
    else
    {
        p_inter->balance_integral = 0.0f; /* Gain zero disables P and I together. */
    }
    correction = authority * correction_current;
    offset = centered + fmaxf(-p_cfg->midpoint_offset_max, fminf(p_cfg->midpoint_offset_max, correction));
    offset = fmaxf(offset_min, fminf(offset_max, offset));
    if ((p_cfg->midpoint_kp > 0.0f) && (fabsf(sensitivity) > slope_floor))
    {
        /* Integrate only with useful midpoint authority; track either offset bound. */
        p_inter->balance_integral += p_cfg->ts *
            (p_cfg->midpoint_ki * p_inter->delta_filtered +
             p_cfg->midpoint_kaw * (offset - centered - correction) / authority);
    }
    p_svpwm->output.common_mode_v = offset;
    p_svpwm->output.midpoint_delta_filtered = p_inter->delta_filtered;
    p_svpwm->output.midpoint_correction_v = offset - centered;
    p_svpwm->output.midpoint_current_magnitude = p_inter->current_filtered;
    p_svpwm->output.midpoint_current_ref = -correction_current;
    p_svpwm->output.midpoint_current = 0.0f;
    for (uint32_t index = 0u; index < 3u; ++index)
    {
        float pole = p_phase[index] * scale + offset; /* Desired pole average relative to the DC midpoint. */
        phase_output[index]->duty_p = bound_duty(fmaxf(0.0f, pole / vp));
        phase_output[index]->duty_n = bound_duty(fmaxf(0.0f, -pole / vn));
        phase_output[index]->duty_o = 1.0f - phase_output[index]->duty_p - phase_output[index]->duty_n;
        p_svpwm->output.midpoint_current += phase_output[index]->duty_o * p_current[index];
    }
    p_svpwm->output.status = SVPWM_3LEVEL_OK;
    return SVPWM_3LEVEL_OK;
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
    if (cfg.average_balance == true)
    {
        if ((cfg.ts <= 0.0f) || (cfg.midpoint_filter_hz <= 0.0f) ||
            (cfg.midpoint_current_min <= 0.0f) || (cfg.midpoint_slope_floor_ratio < 0.0f) ||
            (cfg.midpoint_offset_max < 0.0f) ||
            (cfg.midpoint_ki < 0.0f) || (cfg.midpoint_kaw < 0.0f))
        {
            p_svpwm->output.status = SVPWM_3LEVEL_INVALID_CONFIG;
            return false;
        }
        p_svpwm->inter.filter_coeff = -expm1f(-M_2PI * cfg.midpoint_filter_hz * cfg.ts);
        p_svpwm->inter.current_filtered = cfg.midpoint_current_min;
    }
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
    float phase_current[3] = {0}; /* Same-period bridge currents, A. */
    float midpoint_ref = 0.0f;    /* Corrective current out of the midpoint, A. */
    float best_error = FLT_MAX;  /* Best achievable current tracking error, A. */
    float current_roundoff = 0.0f; /* Tie tolerance retaining the preferred mapping. */
    bool balance_enabled = false; /* Zero gain preserves the legacy modulation. */
    bool found = false;           /* At least one mapping can synthesize the voltage. */

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

    phase_current[0] = p_svpwm->input.i_a;
    phase_current[1] = p_svpwm->input.i_b;
    phase_current[2] = p_svpwm->input.i_c;
    balance_enabled = p_svpwm->cfg.midpoint_kp > 0.0f;
    if (balance_enabled == true)
    {
        midpoint_ref = -p_svpwm->cfg.midpoint_kp * (p_svpwm->input.v_dc_p - p_svpwm->input.v_dc_n);
    }
    current_roundoff = SVPWM_3LEVEL_CURRENT_ROUNDOFF *
                       (fabsf(phase_current[0]) + fabsf(phase_current[1]) +
                        fabsf(phase_current[2]) + fabsf(midpoint_ref));

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
    phase[1] = (-0.5f * alpha) + (M_SQRT3_2 * beta);
    phase[2] = (-0.5f * alpha) - (M_SQRT3_2 * beta);
    if (p_svpwm->cfg.average_balance == true)
    {
        return calculate_average_balance(p_svpwm, phase, phase_current, scale);
    }
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
        p_svpwm->output.midpoint_current_ref = midpoint_ref;
        p_svpwm->output.midpoint_current = phase_current[0] + phase_current[1] + phase_current[2];
        return SVPWM_3LEVEL_OK;
    }

    if (calculate_sector(phase, positive_bus, negative_bus, preferred_mask,
                         phase_current, midpoint_ref, balance_enabled, &p_svpwm->output) == true)
    {
        found = true;
        best_error = fabsf(p_svpwm->output.midpoint_current - midpoint_ref);
        if (balance_enabled == false)
        {
            p_svpwm->output.common_mode_v *= scale;
            return SVPWM_3LEVEL_OK;
        }
    }

    /* An unequal split may move the feasible mapping away from the sign sector.
     * Trying the other 5 mappings keeps the full physical hexagon available. */
    for (uint32_t index = 0u; index < 6u; ++index) /* Fixed upper bound on alternative sector checks. */
    {
        svpwm_3level_output_t candidate = {0}; /* Complete result for this alternative mapping. */
        float candidate_error = 0.0f;         /* Current tracking error for comparison, A. */

        if (sector_masks[index] == preferred_mask)
        {
            continue;
        }
        if (calculate_sector(phase, positive_bus, negative_bus, sector_masks[index],
                             phase_current, midpoint_ref, balance_enabled, &candidate) == true)
        {
            candidate_error = fabsf(candidate.midpoint_current - midpoint_ref);
            if ((found == false) || /* Accept the first feasible mapping. */
                (candidate_error + current_roundoff < best_error)) /* Prefer materially better balance authority. */
            {
                p_svpwm->output = candidate;
                best_error = candidate_error;
                found = true;
            }
            if (balance_enabled == false)
            {
                p_svpwm->output.common_mode_v *= scale;
                return SVPWM_3LEVEL_OK;
            }
        }
    }

    if (found == true)
    {
        p_svpwm->output.common_mode_v *= scale;
        return SVPWM_3LEVEL_OK;
    }

    return invalidate(p_svpwm, SVPWM_3LEVEL_OUT_OF_RANGE);
}

void svpwm_3level_reset(svpwm_3level_t *p_svpwm)
{
    if (p_svpwm != NULL)
    {
        p_svpwm->inter.delta_filtered = 0.0f;
        p_svpwm->inter.current_filtered = p_svpwm->cfg.midpoint_current_min;
        p_svpwm->inter.balance_integral = 0.0f;
        (void)invalidate(p_svpwm, SVPWM_3LEVEL_NOT_READY);
    }
}
