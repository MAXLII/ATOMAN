// SPDX-License-Identifier: MIT
/**
 * @file    notch_i32.h
 * @brief   Integer notch filter public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define int32_t notch filter state and fixed-point coefficients
 *          - Convert floating design parameters into integer IIR coefficients
 *          - Process integer feedback samples through a second-order notch filter
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe when caller owns the instance and input pointer
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
#ifndef __NOTCH_I32_H
#define __NOTCH_I32_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    int32_t *p_input;
} notch_i32_input_t;

typedef struct
{
    int32_t b[3];
    int32_t a[2];
    int32_t x[3];
    int32_t y[3];
    uint8_t coeff_q_shift;
} notch_i32_inter_t;

typedef struct
{
    int32_t val;
} notch_i32_output_t;

typedef struct
{
    notch_i32_input_t input;
    notch_i32_inter_t inter;
    notch_i32_output_t output;
} notch_i32_t;

bool notch_i32_init(notch_i32_t *p_notch,
                    float w0,
                    float wb,
                    float ts,
                    uint8_t coeff_q_shift,
                    int32_t *p_input);
void notch_i32_reset(notch_i32_t *p_notch);
bool notch_i32_update(notch_i32_t *p_notch,
                      float w0,
                      float wb,
                      float ts,
                      uint8_t coeff_q_shift);
bool notch_i32_cal(notch_i32_t *p_notch);

#endif
