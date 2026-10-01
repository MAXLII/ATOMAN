// SPDX-License-Identifier: MIT
/**
 * @file msogi.c
 * @brief Simultaneous discrete MSOGI update for harmonics 1, 3, 5, 7, 9.
 * @details All resonators share input minus the sum of their in-phase outputs
 *          and the DC estimate. Solving the trapezoidal residual explicitly
 *          avoids delaying cross-feedback. No allocation or hardware access.
 * @author Max.Li
 * @date 2026-09-27
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License. See LICENSE in the project root.
 */
#include "msogi.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

uint8_t msogi_init(msogi_t *p_observer, float ts, float omega, float k, const float *p_input)
{
    if ((p_observer == NULL) || /* State must exist before binding. */
        (p_input == NULL) ||    /* Input must outlive the observer. */
        (!isfinite(ts)) ||      /* Reject invalid timing at initialization. */
        (!isfinite(omega)) ||   /* Reject invalid frequency at initialization. */
        (!isfinite(k)) ||       /* Reject invalid damping at initialization. */
        (ts <= 0.0f) ||         /* Sampling must advance time. */
        (omega <= 0.0f) ||      /* Fundamental must be positive. */
        (k <= 0.0f) ||          /* Residual injection must be dissipative. */
        (9.0f * omega * ts >= 3.141592654f)) /* All channels must be below Nyquist. */
    {
        return 0u;
    }
    (void)memset(p_observer, 0, sizeof(*p_observer));
    p_observer->p_input = p_input;
    p_observer->ts = ts;
    p_observer->k = k;
    msogi_update_frequency(p_observer, omega);
    return 1u;
}

void msogi_update_frequency(msogi_t *p_observer, float omega)
{
    if (p_observer == NULL)
    {
        return;
    }
    for (uint32_t channel = 0u; channel < MSOGI_CHANNEL_COUNT; ++channel)
    {
        const float order = (float)(2u * channel + 1u);
        p_observer->tangent[channel] = tanf(0.5f * order * omega * p_observer->ts);
    }
    msogi_update_frequency_with_tangent(p_observer, omega, p_observer->tangent);
}

/** @brief Apply one set of frequency tangents without sharing residuals or k-dependent gains. */
void msogi_update_frequency_with_tangent(msogi_t *p_observer, float omega, const float *p_tangent)
{
    if (p_observer == NULL)
    {
        return;
    }
    const float ts = p_observer->ts; /* Bound sample interval. */
    const float k = p_observer->k; /* Bound fundamental damping gain. */
    p_observer->dc_gain = 0.05f * omega * ts; /* DC pole at 0.1 times fundamental frequency. */
    p_observer->divisor = 1.0f + p_observer->dc_gain;
    for (uint32_t channel = 0u; channel < MSOGI_CHANNEL_COUNT; ++channel)
    {
        const float order = (float)(2u * channel + 1u); /* Odd harmonic order. */
        const float tangent = p_tangent[channel]; /* Reuse the caller's matching prewarp. */
        const float denominator = 1.0f + tangent * tangent; /* Trapezoidal oscillator divisor. */
        p_observer->tangent[channel] = tangent;
        p_observer->decay[channel] = (1.0f - tangent * tangent) / denominator;
        p_observer->cross[channel] = 2.0f * tangent / denominator;
        p_observer->gain[channel] = (k / order) * tangent / denominator;
        p_observer->divisor += p_observer->gain[channel];
    }
}

void msogi_cal(msogi_t *p_observer)
{
    float base[MSOGI_CHANNEL_COUNT] = {0.0f}; /* State evolution before this sample's residual. */
    float sum = 0.0f; /* Sum of autonomous channel and DC predictions. */
    float residual = 0.0f; /* Simultaneously solved current-sample residual. */
    if (p_observer == NULL)
    {
        return;
    }
    sum = p_observer->dc + p_observer->dc_gain * p_observer->residual;
    for (uint32_t channel = 0u; channel < MSOGI_CHANNEL_COUNT; ++channel)
    {
        base[channel] = p_observer->decay[channel] * p_observer->alpha[channel]
                        - p_observer->cross[channel] * p_observer->beta[channel]
                        + p_observer->gain[channel] * p_observer->residual;
        sum += base[channel];
    }
    residual = (*p_observer->p_input - sum) / p_observer->divisor;
    for (uint32_t channel = 0u; channel < MSOGI_CHANNEL_COUNT; ++channel)
    {
        const float alpha = base[channel] + p_observer->gain[channel] * residual; /* New in-phase state. */
        p_observer->beta[channel] += p_observer->tangent[channel]
                                    * (alpha + p_observer->alpha[channel]);
        p_observer->alpha[channel] = alpha;
    }
    p_observer->dc += p_observer->dc_gain * (residual + p_observer->residual);
    p_observer->residual = residual;
}
