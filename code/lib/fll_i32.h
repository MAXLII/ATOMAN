// SPDX-License-Identifier: MIT
/**
 * @file    fll_i32.h
 * @brief   Integer FLL library public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define int32_t frequency-locked loop state and configuration
 *          - Track signal angular frequency from integer SOGI outputs
 *          - Clamp frequency and integral-error state for embedded control loops
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe when caller owns the instance
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-27
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __FLL_I32_H
#define __FLL_I32_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    int32_t *p_v;
    int32_t *p_qv;
    int32_t *p_epsilon;
} fll_i32_input_t;

typedef struct
{
    int32_t omega_init_mradps;
    int32_t omega_max_mradps;
    int32_t omega_min_mradps;
    int32_t i_err_max_mradps;
    int32_t i_err_min_mradps;
    int32_t gain_q;
    uint8_t gain_q_shift;
} fll_i32_inter_t;

typedef struct
{
    int32_t omega_mradps;
    int32_t i_err_mradps;
} fll_i32_output_t;

typedef struct
{
    fll_i32_input_t input;
    fll_i32_inter_t inter;
    fll_i32_output_t output;
} fll_i32_t;

bool fll_i32_init(fll_i32_t *p_fll,
                  float gamma,
                  float ts,
                  int32_t omega_init_mradps,
                  int32_t omega_max_mradps,
                  int32_t omega_min_mradps,
                  int32_t i_err_max_mradps,
                  int32_t i_err_min_mradps,
                  uint8_t gain_q_shift,
                  int32_t *p_v,
                  int32_t *p_qv,
                  int32_t *p_epsilon);
void fll_i32_reset(fll_i32_t *p_fll);
bool fll_i32_update(fll_i32_t *p_fll,
                    float gamma,
                    float ts,
                    uint8_t gain_q_shift);
bool fll_i32_cal(fll_i32_t *p_fll);

#endif
