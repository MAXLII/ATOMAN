// SPDX-License-Identifier: MIT
/**
 * @file    sfra.h
 * @brief   SFRA Section adapter public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Preserve existing SFRA APIs and registration macros
 *          - Register SFRA instances in SECTION_SFRA
 *          - Own the Section list consumed by the SFRA protocol service
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Frequency-response algorithms are implemented by sfra_core.c
 *          - Protocol handling belongs to sfra_service.c
 *
 * @author  Max.Li
 * @date    2026-08-02
 * @version 2.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __SFRA_H__
#define __SFRA_H__

#include "section.h"
#include "sfra_core.h"

typedef struct
{
    uint32_t sweep_tag;
    uint16_t point_index;
    float freq_hz;
    float magnitude;
    float phase_deg;
} sfra_result_t;

typedef struct
{
    sfra_t *p_sfra;
    const char *p_name;
    sfra_result_t result_cache[SFRA_FREQ_TABLE_SIZE];
    uint32_t sweep_tag;
    uint16_t result_count;
    uint8_t sfra_id;
    uint8_t data_ready;
    uint8_t done_reported;
} sfra_registration_t;

#define REG_SFRA(name, delay_tick, ts, inject_amp, freq_start, freq_end, \
                 prepare_cb, prepare_ctx)                                \
    float name##_inject;                                                 \
    float name##_collect;                                                \
    sfra_t name = {                                                      \
        .port = {                                                        \
            .p_inject = &(name##_inject),                                \
            .p_collect = &(name##_collect),                              \
        },                                                               \
        .cfg = {                                                         \
            .isr_freq_hz = 1.0f / (ts),                                  \
            .sample_period_s = (ts),                                     \
            .inject_amplitude = (inject_amp),                            \
            .freq_start_hz = (freq_start),                               \
            .freq_end_hz = (freq_end),                                   \
            .freq_step_mul = 1.0f,                                       \
            .settle_cycle_count = SFRA_DEFAULT_SETTLE_CYCLES,            \
            .collect_cycle_count = SFRA_DEFAULT_COLLECT_CYCLES,          \
            .inject_delay_tick = (delay_tick),                           \
            .freq_length = SFRA_FREQ_TABLE_SIZE,                         \
        },                                                               \
        .cb = {                                                          \
            .p_ctx = (prepare_ctx),                                      \
            .freq_prepare = (prepare_cb),                                \
        },                                                               \
    };                                                                   \
    sfra_registration_t sfra_registration_##name = {                     \
        .p_sfra = &(name),                                                \
        .p_name = #name,                                                  \
    };                                                                   \
    REG_SECTION_FUNC(SECTION_SFRA, sfra_registration_##name)

extern section_item_t *p_sfra_first;

void sfra_init_list(void);
sfra_status_t sfra_init(sfra_t *p_sfra,
                        float *p_inject,
                        float *p_collect,
                        float isr_freq_hz,
                        float inject_amplitude,
                        float freq_start_hz,
                        float freq_step_mul);
sfra_status_t sfra_start(sfra_t *p_sfra);
sfra_status_t sfra_stop(sfra_t *p_sfra);
sfra_status_t sfra_reset(sfra_t *p_sfra);
sfra_status_t sfra_set_sweep_range(sfra_t *p_sfra,
                                   float freq_start_hz,
                                   float freq_end_hz);
sfra_status_t sfra_set_inject_delay(sfra_t *p_sfra, uint16_t inject_delay_tick);
void sfra_isr_pre_sample(sfra_t *p_sfra);
void sfra_isr_post_sample(sfra_t *p_sfra);
sfra_status_t sfra_task(sfra_t *p_sfra);

#endif /* __SFRA_H__ */
