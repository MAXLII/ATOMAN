// SPDX-License-Identifier: MIT
/**
 * @file    dft.c
 * @brief   Single-bin DFT module.
 * @details
 *          This file is part of the fft project.
 *
 *          Module responsibilities:
 *          - Validate real-time sample input and single-bin DFT configuration
 *          - Accumulate one requested frequency bin one sample at a time
 *          - Mark normalized real and imaginary outputs valid after the configured cycle window
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

#include "dft.h"

#include <math.h>

#define DFT_TWO_PI (6.28318530717958647692f)

static uint32_t dft_calc_required_sample_size(const dft_t *dft)
{
    const float sample_count =
        dft->cfg.valid_cycle_count / (dft->cfg.target_freq_hz * dft->cfg.sample_period_s);

    if (sample_count < 1.0f)
    {
        return 1U;
    }

    return (uint32_t)(sample_count + 0.5f);
}

static dft_status_t dft_validate(const dft_t *dft)
{
    /* The module stores caller-owned pointers, so every calculation rechecks them. */
    if (dft == 0)
    {
        return DFT_STATUS_NULL;
    }

    if ((dft->input.sample == 0) || (dft->input.start == 0))
    {
        return DFT_STATUS_NULL;
    }

    if ((dft->cfg.sample_period_s <= 0.0f) ||
        (dft->cfg.target_freq_hz <= 0.0f) ||
        (dft->cfg.valid_cycle_count <= 0.0f))
    {
        return DFT_STATUS_INVALID_PARAM;
    }

    return DFT_STATUS_OK;
}

dft_status_t dft_init(dft_t *dft,
                      const float *sample,
                      const uint8_t *start,
                      float target_freq_hz,
                      float sample_period_s)
{
    dft_status_t status;

    if (dft == 0)
    {
        return DFT_STATUS_NULL;
    }

    dft->input.sample = sample;
    dft->input.start = start;
    dft->cfg.target_freq_hz = target_freq_hz;
    dft->cfg.sample_period_s = sample_period_s;
    dft->cfg.valid_cycle_count = DFT_DEFAULT_VALID_CYCLES;

    /* Reset clears stale results before the new configuration is validated. */
    status = dft_reset(dft);
    if (status != DFT_STATUS_OK)
    {
        return status;
    }

    status = dft_validate(dft);
    dft->output.status = status;
    if (status != DFT_STATUS_OK)
    {
        /* Invalid bindings are recorded in output.status for the caller to inspect. */
        return status;
    }

    dft->inter.phase_step_rad = DFT_TWO_PI * target_freq_hz * sample_period_s;
    dft->inter.required_sample_size = dft_calc_required_sample_size(dft);

    return DFT_STATUS_OK;
}

dft_status_t dft_reset(dft_t *dft)
{
    if (dft == 0)
    {
        return DFT_STATUS_NULL;
    }

    dft->inter.phase_step_rad = 0.0f;
    dft->inter.phase_rad = 0.0f;
    dft->inter.real_sum = 0.0f;
    dft->inter.imag_sum = 0.0f;
    dft->inter.required_sample_size = 0U;
    dft->inter.sample_count = 0U;
    dft->inter.running = 0U;
    dft->output.real = 0.0f;
    dft->output.imag = 0.0f;
    dft->output.valid = 0U;
    dft->output.status = DFT_STATUS_OK;

    return DFT_STATUS_OK;
}

dft_status_t dft_cal(dft_t *dft)
{
    dft_status_t status;

    status = dft_validate(dft);
    if (dft != 0)
    {
        dft->output.status = status;
    }
    if (status != DFT_STATUS_OK)
    {
        return status;
    }

    dft->inter.phase_step_rad =
        DFT_TWO_PI * dft->cfg.target_freq_hz * dft->cfg.sample_period_s;
    dft->inter.required_sample_size = dft_calc_required_sample_size(dft);

    if (*(dft->input.start) == 0U)
    {
        dft->inter.phase_rad = 0.0f;
        dft->inter.real_sum = 0.0f;
        dft->inter.imag_sum = 0.0f;
        dft->inter.sample_count = 0U;
        dft->inter.running = 0U;
        dft->output.valid = 0U;
        dft->output.status = DFT_STATUS_OK;
        return DFT_STATUS_OK;
    }

    if (dft->output.valid != 0U)
    {
        dft->output.status = DFT_STATUS_OK;
        return DFT_STATUS_OK;
    }

    if (dft->inter.running == 0U)
    {
        dft->inter.phase_rad = 0.0f;
        dft->inter.real_sum = 0.0f;
        dft->inter.imag_sum = 0.0f;
        dft->inter.sample_count = 0U;
        dft->inter.running = 1U;
    }

    /* X(f) = sum x[n] * exp(-j * 2*pi*f*n*Ts), one sample per call. */
    dft->inter.real_sum += *(dft->input.sample) * cosf(dft->inter.phase_rad);
    dft->inter.imag_sum -= *(dft->input.sample) * sinf(dft->inter.phase_rad);
    dft->inter.phase_rad += dft->inter.phase_step_rad;
    dft->inter.sample_count++;

    if (dft->inter.sample_count >= dft->inter.required_sample_size)
    {
        const float gain = 2.0f / (float)dft->inter.sample_count;

        dft->output.real = dft->inter.real_sum * gain;
        dft->output.imag = dft->inter.imag_sum * gain;
        dft->output.valid = 1U;
        dft->inter.running = 0U;
    }

    dft->output.status = DFT_STATUS_OK;

    return DFT_STATUS_OK;
}
