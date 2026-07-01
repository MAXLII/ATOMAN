// SPDX-License-Identifier: MIT
/**
 * @file    pll.h
 * @brief   pll library public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement an alpha-beta input synchronous reference frame PLL
 *          - Calculate alpha-beta, dq, angular frequency, and phase angle outputs
 *          - Support runtime PI coefficient updates for grid synchronization
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe when caller owns the instance
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-07-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __PLL_H
#define __PLL_H

#include "pi_tustin.h"
#include <stdbool.h>

typedef struct
{
    float *p_alpha;
    float *p_beta;
} pll_input_t;

typedef struct
{
    float ts;
    float omega_center;
    float omega_up_lmt;
    float omega_dn_lmt;
    float vm;
    float zeta;
    float omega_n;
    float pi_kp;
    float pi_ki;
} pll_cfg_t;

typedef struct
{
    float pi_ref;
    float pi_act;
    float alpha;
    float beta;
    float vd;
    float vq;
    float vf;
    float theta;
} pll_inter_t;

typedef struct
{
    float omega;
    float theta;
} pll_output_t;

typedef struct
{
    pll_input_t input;
    pll_cfg_t cfg;
    pll_inter_t inter;
    pll_output_t output;
    pi_tustin_t pi;
} pll_t;

bool pll_init(pll_t *p_pll,
              float ts,
              float omega_center,
              float omega_up_lmt,
              float omega_dn_lmt,
              float vm,
              float zeta,
              float omega_n,
              float pi_up_lmt,
              float pi_dn_lmt,
              float theta_init,
              float *p_alpha,
              float *p_beta);

void pll_reset(pll_t *p_pll, float theta_init);

bool pll_update_pi(pll_t *p_pll, float pi_kp, float pi_ki);

bool pll_update_tuning(pll_t *p_pll, float vm, float zeta, float omega_n);

bool pll_cal(pll_t *p_pll);

#endif
