// SPDX-License-Identifier: MIT
/**
 * @file    notch.h
 * @brief   notch library public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement a configurable digital notch filter
 *          - Update filter coefficients from target angular frequency and sampling time
 *          - Process input feedback through the notch transfer function with output limiting
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
#ifndef __NOTCH_H
#define __NOTCH_H

typedef struct
{
    float *p_val;
} notch_input_t;

typedef struct
{
    float w0;
    float wb;
    float ts;
} notch_cfg_t;

typedef struct
{
    float c0;
    float c1;
    float c2;
    float d1;
    float d2;
    float x[3];
    float y[3];
} notch_inter_t;

typedef struct
{
    float val;
} notch_output_t;

typedef struct
{
    notch_input_t input;
    notch_cfg_t cfg;
    notch_inter_t inter;
    notch_output_t output;
} notch_t;

void notch_init(notch_t *p_str,
                float w0,
                float wb,
                float ts,
                float *p_val);
void notch_update_freq(notch_t *p_str, float omega);

void notch_cal(notch_t *p_str);

#endif
