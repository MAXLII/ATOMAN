// SPDX-License-Identifier: MIT
/**
 * @file msogi.h
 * @brief Coupled multiple-SOGI observer with DC rejection.
 * @details Caller-owned, allocation-free C11 observer. Five odd-frequency
 *          channels use a simultaneous prewarped trapezoidal update; outputs
 *          represent the current input sample, not a one-sample-old estimate.
 * @author Max.Li
 * @date 2026-09-27
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef MSOGI_H
#define MSOGI_H

#include <stdint.h>

#define MSOGI_CHANNEL_COUNT 5u /* Fundamental and odd harmonics through order 9. */

typedef struct msogi
{
    float alpha[MSOGI_CHANNEL_COUNT]; /* In-phase components, input units. */
    float beta[MSOGI_CHANNEL_COUNT];  /* Components lagging alpha by 90 degrees. */
    float tangent[MSOGI_CHANNEL_COUNT]; /* Prewarped half-step angular frequencies. */
    float decay[MSOGI_CHANNEL_COUNT]; /* Homogeneous in-phase update coefficients. */
    float cross[MSOGI_CHANNEL_COUNT]; /* Quadrature-to-in-phase coupling. */
    float gain[MSOGI_CHANNEL_COUNT];  /* Shared residual injection coefficients. */
    float dc;       /* Estimated DC input component. */
    float dc_gain;  /* DC observer half-step gain. */
    float residual; /* Previous input minus all estimated components. */
    float divisor;  /* Simultaneous algebraic solve denominator. */
    float ts;       /* Fixed sample interval, seconds. */
    float k;        /* Fundamental residual injection gain. */
    const float *p_input; /* Bound sample; caller owns its lifetime. */
} msogi_t;

/**
 * @brief Bind and initialize the observer. Call before periodic execution.
 * @param p_observer Caller-owned state.
 * @param ts Sample interval, seconds.
 * @param omega Fundamental angular frequency, radians/second.
 * @param k Fundamental SOGI gain; harmonic gains are k/order.
 * @param p_input Bound sampled input.
 * @return 1 on success, 0 for invalid initialization parameters.
 */
uint8_t msogi_init(msogi_t *p_observer, float ts, float omega, float k, const float *p_input);

/** @brief Advance a bound observer by one sample. @param p_observer Initialized state. */
void msogi_cal(msogi_t *p_observer);

/** @brief Retune without clearing states; caller supplies a valid sub-Nyquist frequency.
 * @param p_observer Initialized state. @param omega Fundamental angular frequency, radians/second. */
void msogi_update_frequency(msogi_t *p_observer, float omega);

/**
 * @brief Retune with prewarped tangents already calculated for this sample interval and frequency.
 * @details Reuses the frequency geometry; each observer retains its own k-dependent gains,
 *          input binding and dynamic states. The ordinary retuning API calculates the tangents.
 * @param p_observer Initialized state; its sampling interval must match the tangent source.
 * @param omega Same valid sub-Nyquist fundamental angular frequency as the tangent source, rad/s.
 * @param p_tangent MSOGI_CHANNEL_COUNT values tanf(0.5f * order * omega * ts), in channel order.
 * @note The array is read synchronously and copied; no pointer is retained. It may be the
 *       observer's own tangent array. Caller guarantees the matching interval/frequency.
 */
void msogi_update_frequency_with_tangent(msogi_t *p_observer, float omega, const float *p_tangent);

#endif /* MSOGI_H */
