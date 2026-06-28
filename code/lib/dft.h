// SPDX-License-Identifier: MIT
/**
 * @file    dft.h
 * @brief   Single-bin DFT module.
 * @details
 *          This file is part of the fft project.
 *
 *          Module responsibilities:
 *          - Bind caller-owned real-time sample and start inputs
 *          - Define the public data structures for a streaming single-bin DFT
 *          - Expose reset and per-sample calculation interfaces for embedded C callers
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented by the caller
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-12
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef __DFT_H
#define __DFT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define DFT_DEFAULT_VALID_CYCLES (5.0f)

typedef enum
{
    /** Calculation completed successfully. */
    DFT_STATUS_OK = 0,
    /** A required object or sample pointer is NULL. */
    DFT_STATUS_NULL = 1,
    /** The sample count, sample period, or target frequency is invalid. */
    DFT_STATUS_INVALID_PARAM = 2
} dft_status_t;

typedef struct
{
    /** Pointer to the current real-time sample value. */
    const float *sample;
    /** Pointer to the start command. Non-zero starts or keeps the DFT window active. */
    const uint8_t *start;
} dft_input_t;

typedef struct
{
    /** Frequency bin to calculate, in Hz. */
    float target_freq_hz;
    /** Sampling period of the discrete samples, in seconds. */
    float sample_period_s;
    /** Number of target-frequency cycles required before output becomes valid. */
    float valid_cycle_count;
} dft_cfg_t;

typedef struct
{
    /** Phase increment between adjacent samples, in radians. */
    float phase_step_rad;
    /** Cosine of phase_step_rad, calculated once when a DFT window starts. */
    float phase_step_cos;
    /** Sine of phase_step_rad, calculated once when a DFT window starts. */
    float phase_step_sin;
    /** Current phase of the active DFT window, in radians. */
    float phase_rad;
    /** Cosine of current phase for recurrence-based accumulation. */
    float phase_cos;
    /** Sine of current phase for recurrence-based accumulation. */
    float phase_sin;
    /** Accumulated real component for the active DFT window. */
    float real_sum;
    /** Accumulated imaginary component for the active DFT window. */
    float imag_sum;
    /** Number of samples required for the configured valid-cycle window. */
    uint32_t required_sample_size;
    /** Number of samples accumulated in the active DFT window. */
    uint32_t sample_count;
    /** Non-zero while a DFT window is being accumulated. */
    uint8_t running;
} dft_inter_t;

typedef struct
{
    /** Normalized real component of the requested frequency bin. */
    float real;
    /** Normalized imaginary component of the requested frequency bin. */
    float imag;
    /** Non-zero when real and imaginary outputs are valid. */
    uint8_t valid;
    /** Status of the most recent module operation. */
    dft_status_t status;
} dft_output_t;

typedef struct
{
    /** Input sample binding. */
    dft_input_t input;
    /** DFT calculation configuration. */
    dft_cfg_t cfg;
    /** Runtime intermediates owned by the module instance. */
    dft_inter_t inter;
    /** Calculation result and status. */
    dft_output_t output;
} dft_t;

/**
 * @brief Initialize a single-bin DFT instance.
 *
 * @param dft             DFT instance supplied by the caller.
 * @param sample          Pointer to the current real-time sample.
 * @param start           Pointer to the start command.
 * @param target_freq_hz  Frequency bin to calculate, in Hz.
 * @param sample_period_s Sampling period, in seconds.
 * @return Operation status.
 */
dft_status_t dft_init(dft_t *dft,
                      const float *sample,
                      const uint8_t *start,
                      float target_freq_hz,
                      float sample_period_s);

/**
 * @brief Clear runtime intermediates and output values.
 *
 * @param dft DFT instance supplied by the caller.
 * @return Operation status.
 */
dft_status_t dft_reset(dft_t *dft);

/**
 * @brief Run one real-time DFT calculation step using the current sample.
 *
 * @param dft DFT instance supplied by the caller.
 * @return Operation status.
 */
dft_status_t dft_cal(dft_t *dft);

#ifdef __cplusplus
}
#endif

#endif /* __DFT_H */
