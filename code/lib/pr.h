// SPDX-License-Identifier: MIT
/**
 * @file    pr.h
 * @brief   pr library public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement a proportional-resonant controller for AC signal regulation
 *          - Calculate resonant-controller output from reference and feedback pointers
 *          - Support resonant-frequency updates and state reset for grid-synchronous control
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
#ifndef __PR_H
#define __PR_H

#include <stdbool.h>

typedef struct
{
    float *p_act;
    float *p_ref;
} pr_input_t;

typedef struct
{
    float kp;
    float kr;
    float w0;
    float wc;
    float ts;
    float a1;
    float a2;
    float b0;
    float b1;
    float b2;
    float up_lmt;
    float dn_lmt;

    float e[3];
    float u[3];
} pr_inter_t;

typedef struct
{
    float raw;
    float sat;
    float val;
    bool is_saturated;
} pr_output_t;

typedef struct
{
    pr_input_t input;
    pr_inter_t inter;
    pr_output_t output;
} pr_t;

bool pr_init(pr_t *p_str,
             float kp,
             float kr,
             float w0,
             float wc,
             float ts,
             float up_lmt,
             float dn_lmt,
             float *p_ref,
             float *p_act);

bool pr_cal(pr_t *p_str);

bool pr_update_freq(pr_t *p_str, float omega);

void pr_reset(pr_t *p_str);

#endif
