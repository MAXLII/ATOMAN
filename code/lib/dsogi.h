// SPDX-License-Identifier: MIT
/**
 * @file    dsogi.h
 * @brief   Dual SOGI positive/negative sequence extraction, without a PLL.
 * @details
 *          This file is part of the base digital power framework project.
 *          Caller-owned state; C11; no dynamic allocation or hardware access.
 * @author  Max.Li
 * @date    2026-09-12
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef DSOGI_H
#define DSOGI_H
#include <stdbool.h>
#include "sogi.h"

typedef struct dsogi_input
{
    float *p_alpha; /* Stationary alpha voltage, V; caller-owned binding. */
    float *p_beta;  /* Stationary beta voltage, V; positive sequence is cos/sin. */
    float *p_omega; /* External center frequency, rad/s; fixed source or previous PLL result. */
} dsogi_input_t;

typedef struct dsogi_cfg
{
    float ts;        /* Sample period, s; [1e-6,0.01]. */
    float k;         /* SOGI damping coefficient, [0.1,4]; typical sqrt(2). */
    float omega_min; /* Minimum accepted frequency, rad/s; >= 1. */
    float omega_max; /* Maximum accepted frequency, rad/s; <= 100000. */
} dsogi_cfg_t;

typedef struct dsogi_inter
{
    sogi_t alpha;     /* Alpha-axis Tustin SOGI, including sample history. */
    sogi_t beta;      /* Beta-axis Tustin SOGI with the same coefficients. */
    bool initialized; /* Configuration and bindings were accepted. */
} dsogi_inter_t;

typedef struct dsogi_output
{
    float alpha_pos; /* Positive-sequence alpha component, V. */
    float beta_pos;  /* Positive-sequence beta component, V. */
    float alpha_neg; /* Negative-sequence alpha component, V. */
    float beta_neg;  /* Negative-sequence beta component, V. */
} dsogi_output_t;

typedef struct dsogi
{
    dsogi_input_t input;   /* Bound input sources; must outlive this instance. */
    dsogi_cfg_t cfg;       /* Fixed configuration; change through init. */
    dsogi_inter_t inter;   /* Private runtime history; caller must not modify. */
    dsogi_output_t output; /* Latest separated outputs; zeroed on rejection/reset. */
} dsogi_t;

/** @param p_dsogi Instance to initialize.
 *  @param p_cfg Configuration; require 0.001 <= omega_min*ts <= omega_max*ts <= 1.
 *  @param p_alpha Live alpha source.
 *  @param p_beta Live beta source.
 *  @param p_omega Live center frequency within configured limits.
 *  @return true if initialized; failed init invalidates the instance. Inputs may not alias the instance. */
bool dsogi_init(dsogi_t *p_dsogi, const dsogi_cfg_t *p_cfg,
                float *p_alpha, float *p_beta, float *p_omega);
/** @param p_dsogi Instance; clears histories/outputs while retaining configuration and bindings. */
void dsogi_reset(dsogi_t *p_dsogi);
/** @param p_dsogi Initialized instance; call once per configured sample period.
 *  @return true on a finite update; false clears histories/outputs, allowing recovery on valid input.
 *  No PLL, Clarke transform, FLL, DC rejection or lock detector is included. */
bool dsogi_cal(dsogi_t *p_dsogi);
#endif
