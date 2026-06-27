// SPDX-License-Identifier: MIT
/**
 * @file    sogi_i32.h
 * @brief   Integer SOGI library public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define int32_t second-order generalized integrator state and coefficients
 *          - Calculate in-phase and quadrature outputs from integer sampled input
 *          - Support runtime frequency updates with fixed-point coefficients
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
#ifndef __SOGI_I32_H
#define __SOGI_I32_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    int32_t *p_input;
} sogi_i32_input_t;

typedef struct
{
    int32_t b[3];
    int32_t a[2];
    int32_t qb[3];
    int32_t x[3];
    int32_t y[3];
    int32_t qy[3];
    int32_t err;
    int32_t omega_mradps;
    uint8_t coeff_q_shift;
} sogi_i32_inter_t;

typedef struct
{
    int32_t u;
    int32_t qu;
    int32_t err;
} sogi_i32_output_t;

typedef struct
{
    sogi_i32_input_t input;
    sogi_i32_inter_t inter;
    sogi_i32_output_t output;
} sogi_i32_t;

bool sogi_i32_init(sogi_i32_t *p_sogi,
                   float ts,
                   float omega_radps,
                   float gain,
                   uint8_t coeff_q_shift,
                   int32_t *p_input);
void sogi_i32_reset(sogi_i32_t *p_sogi);
bool sogi_i32_update(sogi_i32_t *p_sogi,
                     float ts,
                     float omega_radps,
                     float gain,
                     uint8_t coeff_q_shift);
bool sogi_i32_cal(sogi_i32_t *p_sogi);

#endif
