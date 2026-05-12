// SPDX-License-Identifier: MIT
/**
 * @file    sfra.h
 * @brief   Software frequency response analyzer module.
 * @details
 *          This file is part of the fft project.
 *
 *          Module responsibilities:
 *          - Generate a swept sine injection signal for frequency response analysis
 *          - Collect one response signal and calculate its response against injection
 *          - Publish the latest frequency-magnitude and frequency-phase result
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

#ifndef __SFRA_H
#define __SFRA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dft.h"
#include "section.h"

#include <stdint.h>

#define SFRA_FREQ_TABLE_SIZE          (64U)
#define SFRA_DEFAULT_SETTLE_CYCLES    (2.0f)
#define SFRA_DEFAULT_COLLECT_CYCLES   (5.0f)
#define SFRA_MAX_INJECT_DELAY_TICK    (2U)
#define SFRA_SAMPLE_BUFFER_SIZE       (32U)

#define SFRA_REG(name, delay_tick, ts, inject_amp, freq_start, freq_end)           \
    sfra_t name = {                                                               \
        .p_name = #name,                                                          \
    };                                                                            \
    float name##_inject;                                                          \
    float name##_collect;                                                         \
    static inline sfra_status_t name##_reg_init(void)                             \
    {                                                                             \
        sfra_status_t status;                                                     \
                                                                                  \
        name##_inject = 0.0f;                                                     \
        name##_collect = 0.0f;                                                    \
        status = sfra_init(&(name),                                               \
                           &(name##_inject),                                      \
                           &(name##_collect),                                     \
                           1.0f / (ts),                                           \
                           (inject_amp),                                          \
                           (freq_start),                                          \
                           1.1f);                                                 \
        if (status != SFRA_STATUS_OK)                                             \
        {                                                                         \
            return status;                                                        \
        }                                                                         \
                                                                                  \
        status = sfra_set_inject_delay(&(name), (delay_tick));                    \
        if (status != SFRA_STATUS_OK)                                             \
        {                                                                         \
            return status;                                                        \
        }                                                                         \
                                                                                  \
        return sfra_set_sweep_range(&(name), (freq_start), (freq_end));           \
    }

#define REG_SFRA(name, delay_tick, ts, inject_amp, freq_start, freq_end)           \
    SFRA_REG(name, delay_tick, ts, inject_amp, freq_start, freq_end)               \
    static void name##_section_init(void)                                          \
    {                                                                             \
        (void)name##_reg_init();                                                   \
    }                                                                             \
    REG_INIT(0, name##_section_init)                                               \
    REG_SECTION_FUNC(SECTION_SFRA, name)

typedef enum
{
    SFRA_STATUS_OK = 0,
    SFRA_STATUS_NULL = 1,
    SFRA_STATUS_INVALID_PARAM = 2,
    SFRA_STATUS_BUSY = 3,
    SFRA_STATUS_DONE = 4
} sfra_status_t;

typedef enum
{
    SFRA_STATE_IDLE = 0,
    SFRA_STATE_PREPARE_FREQ = 1,
    SFRA_STATE_SETTLE = 2,
    SFRA_STATE_COLLECT = 3,
    SFRA_STATE_CALC = 4,
    SFRA_STATE_DONE = 5
} sfra_state_t;

typedef void (*sfra_freq_prepare_cb_t)(void *p_ctx);

typedef struct
{
    /** SFRA writes the current sine injection value to this pointer. */
    float *p_inject;
    /** User writes the measured response value to this pointer. */
    float *p_collect;
} sfra_port_t;

typedef struct
{
    /** Optional user context passed to frequency-prepare callback. */
    void *p_ctx;
    /** Called by sfra_task each time a new frequency point is prepared. */
    sfra_freq_prepare_cb_t freq_prepare;
} sfra_cb_t;

typedef struct
{
    /** Frequency of sfra_isr_pre_sample/post_sample calls, in Hz. */
    float isr_freq_hz;
    /** ISR sampling period, in seconds. */
    float sample_period_s;
    /** Sine injection amplitude. */
    float inject_amplitude;
    /** First sweep frequency, in Hz. */
    float freq_start_hz;
    /** Last sweep frequency, in Hz. */
    float freq_end_hz;
    /** Frequency multiplier between adjacent sweep points. */
    float freq_step_mul;
    /** Target-frequency cycles used for plant settling before DFT collection. */
    float settle_cycle_count;
    /** Target-frequency cycles used by internal DFT collection. */
    float collect_cycle_count;
    /** Injection delay ticks used to align excitation with collected response. */
    uint16_t inject_delay_tick;
    /** Number of sweep points, limited by SFRA_FREQ_TABLE_SIZE. */
    uint16_t freq_length;
} sfra_cfg_t;

typedef struct
{
    /** Current excitation sample copied from *p_inject. */
    float inject_sample;
    /** Current response sample copied from *p_collect. */
    float collect_sample;
    /** Internal DFT start flag shared by the injection and response analyzers. */
    uint8_t dft_start;
    /** Internal DFT analyzer for injection, used as transfer-function denominator. */
    dft_t inject_dft;
    /** Internal DFT analyzer for response, used as transfer-function numerator. */
    dft_t collect_dft;
} sfra_dft_t;

typedef struct
{
    /** Current injection and analysis frequency, in Hz. */
    float current_freq_hz;
    /** Current sine injection phase, in radians. */
    float phase_rad;
    /** Sine phase increment per ISR call, in radians. */
    float phase_step_rad;
    /** Current sine injection value before delay alignment. */
    float injection_now;
    /** Injection delay line, index 0 is one ISR tick old. */
    float injection_delay[SFRA_MAX_INJECT_DELAY_TICK];
    /** Required ISR samples for the settling stage. */
    uint32_t settle_sample_size;
    /** Current settling sample counter. */
    uint32_t settle_sample_count;
    /** Set by post-sample ISR when both internal DFT results are valid. */
    uint8_t point_ready;
    /** Internal DFT state for injection and response signals. */
    sfra_dft_t dft;
    /** Buffered injection samples collected by ISR and consumed by sfra_task. */
    float inject_sample_buf[SFRA_SAMPLE_BUFFER_SIZE];
    /** Buffered response samples collected by ISR and consumed by sfra_task. */
    float collect_sample_buf[SFRA_SAMPLE_BUFFER_SIZE];
    /** Ring-buffer write index owned by the ISR path. */
    uint8_t sample_write_index;
    /** Ring-buffer read index owned by sfra_task. */
    uint8_t sample_read_index;
    /** Number of buffered samples waiting for task-side DFT calculation. */
    uint8_t sample_count;
    /** Non-zero if the ISR sample buffer overflows. */
    uint8_t sample_overflow;
} sfra_isr_t;

typedef struct
{
    /** Sweep state owned by sfra_task. */
    sfra_state_t state;
    /** Current sweep point index. */
    uint16_t freq_index;
    /** Non-zero while a sweep is active. */
    uint8_t active;
    /** Non-zero when the sweep has completed all frequency points. */
    uint8_t done;
    /** Latest task-level status. */
    sfra_status_t status;
} sfra_task_t;

typedef struct
{
    /** Current sweep frequency, in Hz. */
    float current_freq_hz;
    /** Current sweep point index. */
    uint16_t freq_index;
    /** Latest completed frequency point index. */
    uint16_t point_index;
    /** Number of completed frequency points in the current sweep. */
    uint16_t point_count;
    /** Magnitude of the latest completed frequency point. */
    float mag;
    /** Magnitude of the latest completed frequency point, in dB. */
    float mag_db;
    /** Phase of the latest completed frequency point, in degrees. */
    float phase;
    /** Non-zero while sweeping. */
    uint8_t busy;
    /** Non-zero when all frequency points are complete. */
    uint8_t done;
    /** Non-zero when a new point result is available. */
    uint8_t point_done;
    /** Latest module status. */
    sfra_status_t status;
} sfra_output_t;

typedef struct
{
    /** Sweep tag associated with this cached result. */
    uint32_t sweep_tag;
    /** Frequency point index inside the current sweep. */
    uint16_t point_index;
    /** Frequency of this completed point, in Hz. */
    float freq_hz;
    /** True magnitude ratio of this completed point. */
    float magnitude;
    /** Magnitude of this completed point, in dB. */
    float magnitude_db;
    /** Phase of this completed point, in degrees. */
    float phase_deg;
} sfra_result_t;

typedef struct sfra_t
{
    /** External injection and response ports. */
    sfra_port_t port;
    /** Sweep and injection configuration. */
    sfra_cfg_t cfg;
    /** Fast path state used by ISR pre/post sample calls. */
    sfra_isr_t isr;
    /** Slow sweep state machine used by sfra_task. */
    sfra_task_t task;
    /** Optional application callbacks. */
    sfra_cb_t cb;
    /** User-readable sweep status. */
    sfra_output_t output;
    /** SFRA service id assigned from SECTION_SFRA entries. */
    uint8_t sfra_id;
    /** Non-zero when at least one sweep point is available for query. */
    uint8_t data_ready;
    /** Non-zero after sfra_service has reported the current sweep completion. */
    uint8_t done_reported;
    /** Number of completed points cached for the current sweep. */
    uint16_t result_count;
    /** Completed point cache owned by sfra_service. */
    sfra_result_t result_cache[SFRA_FREQ_TABLE_SIZE];
    /** Monotonic tag used to identify the active sweep result set. */
    uint32_t sweep_tag;
    /** User-readable SFRA instance name. */
    const char *p_name;
    /** Linked-list pointer owned by sfra_service. */
    struct sfra_t *p_next;
} sfra_t;

sfra_status_t sfra_init(sfra_t *sfra,
                        float *p_inject,
                        float *p_collect,
                        float isr_freq_hz,
                        float inject_amplitude,
                        float freq_start_hz,
                        float freq_step_mul);
sfra_status_t sfra_start(sfra_t *sfra);
sfra_status_t sfra_stop(sfra_t *sfra);
sfra_status_t sfra_reset(sfra_t *sfra);
sfra_status_t sfra_set_sweep_range(sfra_t *sfra,
                                   float freq_start_hz,
                                   float freq_end_hz);
sfra_status_t sfra_set_inject_delay(sfra_t *sfra, uint16_t inject_delay_tick);
sfra_status_t sfra_set_freq_prepare_cb(sfra_t *sfra,
                                       sfra_freq_prepare_cb_t freq_prepare,
                                       void *p_ctx);
void sfra_isr_pre_sample(sfra_t *sfra);
void sfra_isr_post_sample(sfra_t *sfra);
sfra_status_t sfra_task(sfra_t *sfra);

#ifdef __cplusplus
}
#endif

#endif /* __SFRA_H */
