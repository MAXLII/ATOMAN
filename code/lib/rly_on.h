// SPDX-License-Identifier: MIT
/**
 * @file    rly_on.h
 * @brief   rly_on library public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Manage relay on/off sequencing from trigger and synchronization inputs
 *          - Delay relay actions until configured timing and equality conditions are satisfied
 *          - Call caller-provided relay control callbacks when state transitions are ready
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __RLY_ON_H
#define __RLY_ON_H

#include "stdint.h"

typedef enum
{
    RLY_ON_STA_INIT,
    RLY_ON_STA_IDLE,
    RLY_ON_STA_WAIT,
    RLY_ON_STA_DLY,
    RLY_ON_STA_RUN,
    RLY_ON_STA_ERR,
} RLY_ON_STA_E;

typedef struct
{
    uint8_t *p_rly_on_trig;
    uint8_t *p_rly_off_trig;
    uint8_t *p_is_equal;
    float *p_freq;
} rly_on_input_t;

typedef struct
{
    float ctrl_freq;
    float rly_on_time_def; // unit:s
} rly_on_cfg_t;

typedef struct
{
    uint32_t dly_cnt;
    uint32_t dly;
    uint32_t on_cnt;
    uint32_t on_confirm;
    uint8_t sta;
} rly_on_inter_t;

typedef struct
{
    uint8_t is_closed;
} rly_on_output_t;

typedef struct
{
    void (*rly_on)(void);
    void (*rly_off)(void);
} rly_on_func_t;

typedef struct
{
    rly_on_input_t input;
    rly_on_cfg_t cfg;
    rly_on_inter_t inter;
    rly_on_func_t func;
    rly_on_output_t output;
} rly_on_t;

void rly_on_init(rly_on_t *p_str, uint8_t *p_rly_on_trig, uint8_t *p_rly_off_trig, uint8_t *p_is_equal, float *p_freq, float ctrl_freq, float rly_on_time_def, void (*rly_on)(void), void (*rly_off)(void));

void rly_on_func(rly_on_t *p_str);

#endif

