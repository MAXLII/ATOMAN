// SPDX-License-Identifier: MIT
/**
 * @file    sfra.c
 * @brief   Software frequency response analyzer module.
 * @details
 *          This file is part of the fft project.
 *
 *          Module responsibilities:
 *          - Generate a swept sine injection signal in the pre-sample ISR path
 *          - Feed current excitation and response samples to internal DFT analyzers
 *          - Calculate true magnitude ratio and phase in the background task
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

#include "sfra.h"
#include "my_math.h"

#include <stddef.h>

#define SFRA_TWO_PI       (6.28318530717958647692f)
#define SFRA_RAD_TO_DEG   (57.2957795130823208768f)
#define SFRA_DIV_MIN_ABS  (1.0e-12f)

typedef struct
{
    float real;
    float imag;
} sfra_complex_t;

static sfra_status_t sfra_validate(const sfra_t *sfra)
{
    if (sfra == NULL)
    {
        return SFRA_STATUS_NULL;
    }

    if ((sfra->port.p_inject == NULL) || (sfra->port.p_collect == NULL))
    {
        return SFRA_STATUS_NULL;
    }

    if ((sfra->cfg.isr_freq_hz <= 0.0f) ||
        (sfra->cfg.sample_period_s <= 0.0f) ||
        (sfra->cfg.inject_amplitude <= 0.0f) ||
        (sfra->cfg.freq_start_hz <= 0.0f) ||
        (sfra->cfg.freq_end_hz < sfra->cfg.freq_start_hz) ||
        (sfra->cfg.settle_cycle_count <= 0.0f) ||
        (sfra->cfg.collect_cycle_count <= 0.0f) ||
        (sfra->cfg.freq_length == 0U) ||
        (sfra->cfg.freq_length > SFRA_FREQ_TABLE_SIZE))
    {
        return SFRA_STATUS_INVALID_PARAM;
    }

    return SFRA_STATUS_OK;
}

static uint32_t sfra_calc_sample_size(float cycle_count,
                                      float freq_hz,
                                      float sample_period_s)
{
    const float sample_count = cycle_count / (freq_hz * sample_period_s);

    if (sample_count < 1.0f)
    {
        return 1U;
    }

    return (uint32_t)(sample_count + 0.5f);
}

static void sfra_update_freq_step(sfra_t *sfra)
{
    if (sfra->cfg.freq_length <= 1U)
    {
        sfra->cfg.freq_step_mul = 1.0f;
    }
    else
    {
        sfra->cfg.freq_step_mul =
            powf(sfra->cfg.freq_end_hz / sfra->cfg.freq_start_hz,
                 1.0f / (float)(sfra->cfg.freq_length - 1U));
    }
}

static void sfra_update_output(sfra_t *sfra, sfra_status_t status)
{
    sfra->output.current_freq_hz = sfra->isr.current_freq_hz;
    sfra->output.freq_index = sfra->task.freq_index;
    sfra->output.busy = sfra->task.active;
    sfra->output.done = sfra->task.done;
    sfra->output.status = status;
    sfra->task.status = status;
}

static uint16_t sfra_sample_buffer_count(uint16_t write_index, uint16_t read_index)
{
    if (write_index >= read_index)
    {
        return (uint16_t)(write_index - read_index);
    }

    return (uint16_t)(SFRA_SAMPLE_BUFFER_SIZE - read_index + write_index);
}

static void sfra_sample_buffer_clear(sfra_t *sfra)
{
    if (sfra == NULL)
    {
        return;
    }

    sfra->isr.sample_write_index = 0U;
    sfra->isr.sample_read_index = 0U;
    sfra->isr.sample_count = 0U;
    sfra->isr.sample_overflow = 0U;
}

static inline void sfra_sample_buffer_push(sfra_t *sfra,
                                    float inject_sample,
                                    float collect_sample)
{
    uint16_t write_index;
    uint16_t next_index;
    uint16_t read_index;

    if (sfra == NULL)
    {
        return;
    }

    if (sfra->isr.sample_overflow != 0U)
    {
        return;
    }

    write_index = sfra->isr.sample_write_index;
    next_index = (uint16_t)((write_index + 1U) % SFRA_SAMPLE_BUFFER_SIZE);
    read_index = sfra->isr.sample_read_index;
    if (next_index == read_index)
    {
        sfra->isr.sample_overflow = 1U;
        return;
    }

    sfra->isr.inject_sample_buf[write_index] = inject_sample;
    sfra->isr.collect_sample_buf[write_index] = collect_sample;
    sfra->isr.sample_write_index = next_index;
    sfra->isr.sample_count = sfra_sample_buffer_count(next_index, read_index);
}

static uint8_t sfra_sample_buffer_pop(sfra_t *sfra,
                                      float *p_inject_sample,
                                      float *p_collect_sample)
{
    uint16_t read_index;
    uint16_t write_index;
    uint16_t next_index;

    if ((sfra == NULL) ||
        (p_inject_sample == NULL) ||
        (p_collect_sample == NULL))
    {
        return 0U;
    }

    read_index = sfra->isr.sample_read_index;
    write_index = sfra->isr.sample_write_index;
    if (read_index == write_index)
    {
        sfra->isr.sample_count = 0U;
        return 0U;
    }

    *p_inject_sample = sfra->isr.inject_sample_buf[read_index];
    *p_collect_sample = sfra->isr.collect_sample_buf[read_index];
    next_index = (uint16_t)((read_index + 1U) % SFRA_SAMPLE_BUFFER_SIZE);
    sfra->isr.sample_read_index = next_index;
    sfra->isr.sample_count = sfra_sample_buffer_count(write_index, next_index);

    return 1U;
}

static void sfra_prepare_current_freq(sfra_t *sfra)
{
    uint16_t i;
    float freq_hz;

    sfra_update_freq_step(sfra);

    freq_hz = sfra->cfg.freq_start_hz;
    for (i = 0U; i < sfra->task.freq_index; i++)
    {
        freq_hz *= sfra->cfg.freq_step_mul;
    }

    sfra->isr.current_freq_hz = freq_hz;
    sfra->isr.phase_rad = 0.0f;
    sfra->isr.phase_step_rad = SFRA_TWO_PI * freq_hz * sfra->cfg.sample_period_s;
    sfra->isr.injection_now = 0.0f;
    sfra->isr.injection_delay[0] = 0.0f;
    sfra->isr.injection_delay[1] = 0.0f;
    sfra->isr.settle_sample_size =
        sfra_calc_sample_size(sfra->cfg.settle_cycle_count,
                              freq_hz,
                              sfra->cfg.sample_period_s);
    sfra->isr.settle_sample_count = 0U;
    sfra->isr.point_ready = 0U;
    sfra->isr.dft.dft_start = 0U;
    sfra->isr.dft.inject_sample = 0.0f;
    sfra->isr.dft.collect_sample = 0.0f;
    sfra_sample_buffer_clear(sfra);
    *(sfra->port.p_inject) = 0.0f;

    dft_init(&sfra->isr.dft.inject_dft,
             &sfra->isr.dft.inject_sample,
             &sfra->isr.dft.dft_start,
             freq_hz,
             sfra->cfg.sample_period_s);
    sfra->isr.dft.inject_dft.cfg.valid_cycle_count = sfra->cfg.collect_cycle_count;

    dft_init(&sfra->isr.dft.collect_dft,
             &sfra->isr.dft.collect_sample,
             &sfra->isr.dft.dft_start,
             freq_hz,
             sfra->cfg.sample_period_s);
    sfra->isr.dft.collect_dft.cfg.valid_cycle_count = sfra->cfg.collect_cycle_count;
}

static sfra_complex_t sfra_complex_div(sfra_complex_t numerator,
                                       sfra_complex_t denominator)
{
    sfra_complex_t result;
    float den;

    den = (denominator.real * denominator.real) +
          (denominator.imag * denominator.imag);
    if (den < SFRA_DIV_MIN_ABS)
    {
        den = SFRA_DIV_MIN_ABS;
    }

    result.real = ((numerator.real * denominator.real) +
                   (numerator.imag * denominator.imag)) /
                  den;
    result.imag = ((numerator.imag * denominator.real) -
                   (numerator.real * denominator.imag)) /
                  den;

    return result;
}

static void sfra_calc_current_point(sfra_t *sfra)
{
    sfra_complex_t inject;
    sfra_complex_t collect;
    sfra_complex_t response;
    const uint16_t index = sfra->task.freq_index;

    inject.real = sfra->isr.dft.inject_dft.output.real;
    inject.imag = sfra->isr.dft.inject_dft.output.imag;
    collect.real = sfra->isr.dft.collect_dft.output.real;
    collect.imag = sfra->isr.dft.collect_dft.output.imag;
    response = sfra_complex_div(collect, inject);

    sfra->output.current_freq_hz = sfra->isr.current_freq_hz;
    sfra->output.point_index = index;
    sfra->output.point_count = index + 1U;
    sfra->output.mag =
        sqrtf((response.real * response.real) + (response.imag * response.imag));
    sfra->output.phase = atan2f(response.imag, response.real) * SFRA_RAD_TO_DEG;
    sfra->output.point_done = 1U;
}

sfra_status_t sfra_init(sfra_t *sfra,
                        float *p_inject,
                        float *p_collect,
                        float isr_freq_hz,
                        float inject_amplitude,
                        float freq_start_hz,
                        float freq_step_mul)
{
    sfra_status_t status;

    if (sfra == NULL)
    {
        return SFRA_STATUS_NULL;
    }

    sfra->port.p_inject = p_inject;
    sfra->port.p_collect = p_collect;
    sfra->cfg.isr_freq_hz = isr_freq_hz;
    sfra->cfg.sample_period_s = 1.0f / isr_freq_hz;
    sfra->cfg.inject_amplitude = inject_amplitude;
    sfra->cfg.freq_start_hz = freq_start_hz;
    sfra->cfg.freq_end_hz = freq_start_hz;
    sfra->cfg.freq_step_mul = freq_step_mul;
    sfra->cfg.settle_cycle_count = SFRA_DEFAULT_SETTLE_CYCLES;
    sfra->cfg.collect_cycle_count = SFRA_DEFAULT_COLLECT_CYCLES;
    sfra->cfg.inject_delay_tick = 0U;
    sfra->cfg.freq_length = SFRA_FREQ_TABLE_SIZE;
    sfra->cb.freq_prepare = NULL;
    sfra->cb.p_ctx = NULL;

    status = sfra_reset(sfra);
    if (status != SFRA_STATUS_OK)
    {
        return status;
    }

    status = sfra_validate(sfra);
    sfra_update_output(sfra, status);

    return status;
}

sfra_status_t sfra_reset(sfra_t *sfra)
{
    if (sfra == NULL)
    {
        return SFRA_STATUS_NULL;
    }

    sfra->isr.current_freq_hz = 0.0f;
    sfra->isr.phase_rad = 0.0f;
    sfra->isr.phase_step_rad = 0.0f;
    sfra->isr.injection_now = 0.0f;
    sfra->isr.injection_delay[0] = 0.0f;
    sfra->isr.injection_delay[1] = 0.0f;
    sfra->isr.settle_sample_size = 0U;
    sfra->isr.settle_sample_count = 0U;
    sfra->isr.point_ready = 0U;
    sfra->isr.dft.inject_sample = 0.0f;
    sfra->isr.dft.collect_sample = 0.0f;
    sfra->isr.dft.dft_start = 0U;
    sfra_sample_buffer_clear(sfra);

    if (sfra->port.p_inject != NULL)
    {
        *(sfra->port.p_inject) = 0.0f;
    }

    sfra->task.state = SFRA_STATE_IDLE;
    sfra->task.freq_index = 0U;
    sfra->task.active = 0U;
    sfra->task.done = 0U;
    sfra->task.status = SFRA_STATUS_OK;
    sfra->output.point_index = 0U;
    sfra->output.point_count = 0U;
    sfra->output.mag = 0.0f;
    sfra->output.phase = 0.0f;
    sfra->output.point_done = 0U;

    sfra_update_output(sfra, SFRA_STATUS_OK);

    return SFRA_STATUS_OK;
}

sfra_status_t sfra_start(sfra_t *sfra)
{
    sfra_status_t status;

    status = sfra_validate(sfra);
    if (status != SFRA_STATUS_OK)
    {
        if (sfra != NULL)
        {
            sfra_update_output(sfra, status);
        }
        return status;
    }

    sfra->task.state = SFRA_STATE_PREPARE_FREQ;
    sfra->task.freq_index = 0U;
    sfra->task.active = 1U;
    sfra->task.done = 0U;
    sfra->isr.point_ready = 0U;
    sfra_sample_buffer_clear(sfra);
    sfra->output.point_index = 0U;
    sfra->output.point_count = 0U;
    sfra->output.mag = 0.0f;
    sfra->output.phase = 0.0f;
    sfra->output.point_done = 0U;

    sfra_update_output(sfra, SFRA_STATUS_BUSY);

    return SFRA_STATUS_BUSY;
}

sfra_status_t sfra_stop(sfra_t *sfra)
{
    if (sfra == NULL)
    {
        return SFRA_STATUS_NULL;
    }

    sfra->task.state = SFRA_STATE_IDLE;
    sfra->task.active = 0U;
    sfra->task.done = 0U;
    sfra->isr.point_ready = 0U;
    sfra->isr.dft.dft_start = 0U;
    sfra_sample_buffer_clear(sfra);
    sfra->isr.injection_now = 0.0f;
    sfra->isr.injection_delay[0] = 0.0f;
    sfra->isr.injection_delay[1] = 0.0f;

    if (sfra->port.p_inject != NULL)
    {
        *(sfra->port.p_inject) = 0.0f;
    }

    sfra_update_output(sfra, SFRA_STATUS_OK);

    return SFRA_STATUS_OK;
}

sfra_status_t sfra_set_sweep_range(sfra_t *sfra,
                                   float freq_start_hz,
                                   float freq_end_hz)
{
    if (sfra == NULL)
    {
        return SFRA_STATUS_NULL;
    }

    if ((freq_start_hz <= 0.0f) ||
        (freq_end_hz < freq_start_hz))
    {
        sfra_update_output(sfra, SFRA_STATUS_INVALID_PARAM);
        return SFRA_STATUS_INVALID_PARAM;
    }

    sfra->cfg.freq_start_hz = freq_start_hz;
    sfra->cfg.freq_end_hz = freq_end_hz;
    sfra->cfg.freq_length = SFRA_FREQ_TABLE_SIZE;

    sfra_update_freq_step(sfra);

    sfra_update_output(sfra, SFRA_STATUS_OK);

    return SFRA_STATUS_OK;
}

sfra_status_t sfra_set_inject_delay(sfra_t *sfra, uint16_t inject_delay_tick)
{
    if (sfra == NULL)
    {
        return SFRA_STATUS_NULL;
    }

    if (inject_delay_tick > SFRA_MAX_INJECT_DELAY_TICK)
    {
        sfra_update_output(sfra, SFRA_STATUS_INVALID_PARAM);
        return SFRA_STATUS_INVALID_PARAM;
    }

    sfra->cfg.inject_delay_tick = inject_delay_tick;
    sfra->isr.injection_delay[0] = 0.0f;
    sfra->isr.injection_delay[1] = 0.0f;
    sfra_update_output(sfra, SFRA_STATUS_OK);

    return SFRA_STATUS_OK;
}

void sfra_isr_pre_sample(sfra_t *sfra)
{
    if ((sfra == NULL) || (sfra->port.p_inject == NULL))
    {
        return;
    }

    if ((sfra->task.state != SFRA_STATE_SETTLE) &&
        (sfra->task.state != SFRA_STATE_COLLECT))
    {
        *(sfra->port.p_inject) = 0.0f;
        sfra->isr.injection_now = 0.0f;
        return;
    }

    sfra->isr.injection_now =
        sfra->cfg.inject_amplitude * sinf(sfra->isr.phase_rad);
    *(sfra->port.p_inject) = sfra->isr.injection_now;
    sfra->isr.phase_rad += sfra->isr.phase_step_rad;
}

void sfra_isr_post_sample(sfra_t *sfra)
{
    float inject_sample;

    if ((sfra == NULL) || (sfra->port.p_collect == NULL))
    {
        return;
    }

    if (sfra->task.state == SFRA_STATE_SETTLE)
    {
        if (sfra->isr.settle_sample_count < sfra->isr.settle_sample_size)
        {
            sfra->isr.settle_sample_count++;
        }
    }
    else if ((sfra->task.state == SFRA_STATE_COLLECT) &&
             (sfra->isr.point_ready == 0U))
    {
        if (sfra->cfg.inject_delay_tick == 0U)
        {
            inject_sample = *(sfra->port.p_inject);
        }
        else if (sfra->cfg.inject_delay_tick == 1U)
        {
            inject_sample = sfra->isr.injection_delay[0];
        }
        else
        {
            inject_sample = sfra->isr.injection_delay[1];
        }

        sfra_sample_buffer_push(sfra, inject_sample, *(sfra->port.p_collect));
    }

    sfra->isr.injection_delay[1] = sfra->isr.injection_delay[0];
    sfra->isr.injection_delay[0] = *(sfra->port.p_inject);
}

static void sfra_task_collect_samples(sfra_t *sfra)
{
    float inject_sample;
    float collect_sample;
    uint16_t handled_count;

    if (sfra == NULL)
    {
        return;
    }

    if (sfra->isr.sample_overflow != 0U)
    {
        sfra->task.state = SFRA_STATE_PREPARE_FREQ;
        return;
    }

    sfra->isr.dft.dft_start = 1U;
    handled_count = 0U;
    while ((sfra->isr.point_ready == 0U) &&
           (sfra->isr.sample_overflow == 0U) &&
           (handled_count < SFRA_TASK_SAMPLE_BUDGET) &&
           (sfra_sample_buffer_pop(sfra, &inject_sample, &collect_sample) != 0U))
    {
        sfra->isr.dft.inject_sample = inject_sample;
        sfra->isr.dft.collect_sample = collect_sample;
        dft_cal(&sfra->isr.dft.inject_dft);
        dft_cal(&sfra->isr.dft.collect_dft);

        if ((sfra->isr.dft.inject_dft.output.valid != 0U) &&
            (sfra->isr.dft.collect_dft.output.valid != 0U))
        {
            sfra->isr.point_ready = 1U;
        }

        handled_count++;
    }

    if (sfra->isr.sample_overflow != 0U)
    {
        sfra->task.state = SFRA_STATE_PREPARE_FREQ;
    }
}

sfra_status_t sfra_task(sfra_t *sfra)
{
    sfra_status_t status;

    status = sfra_validate(sfra);
    if (status != SFRA_STATUS_OK)
    {
        if (sfra != NULL)
        {
            sfra_update_output(sfra, status);
        }
        return status;
    }

    switch (sfra->task.state)
    {
    case SFRA_STATE_IDLE:
        status = SFRA_STATUS_OK;
        break;

    case SFRA_STATE_PREPARE_FREQ:
        sfra_prepare_current_freq(sfra);
        if (sfra->cb.freq_prepare != NULL)
        {
            sfra->cb.freq_prepare(sfra->cb.p_ctx);
        }
        sfra->task.state = SFRA_STATE_SETTLE;
        status = SFRA_STATUS_BUSY;
        break;

    case SFRA_STATE_SETTLE:
        if (sfra->isr.settle_sample_count >= sfra->isr.settle_sample_size)
        {
            sfra->isr.dft.dft_start = 1U;
            sfra->task.state = SFRA_STATE_COLLECT;
        }
        status = SFRA_STATUS_BUSY;
        break;

    case SFRA_STATE_COLLECT:
        sfra_task_collect_samples(sfra);
        if (sfra->isr.point_ready != 0U)
        {
            sfra->task.state = SFRA_STATE_CALC;
        }
        status = SFRA_STATUS_BUSY;
        break;

    case SFRA_STATE_CALC:
        sfra_calc_current_point(sfra);
        sfra->task.freq_index++;
        if (sfra->task.freq_index >= sfra->cfg.freq_length)
        {
            sfra->task.state = SFRA_STATE_DONE;
        }
        else
        {
            sfra->task.state = SFRA_STATE_PREPARE_FREQ;
        }
        status = SFRA_STATUS_BUSY;
        break;

    case SFRA_STATE_DONE:
        sfra->task.active = 0U;
        sfra->task.done = 1U;
        *(sfra->port.p_inject) = 0.0f;
        status = SFRA_STATUS_DONE;
        break;

    default:
        sfra_reset(sfra);
        status = SFRA_STATUS_INVALID_PARAM;
        break;
    }

    sfra_update_output(sfra, status);

    return status;
}
