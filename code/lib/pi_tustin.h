// SPDX-License-Identifier: MIT
/**
 * @file    pi_tustin.h
 * @brief   pi_tustin library public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement a PI controller using Tustin discretization
 *          - Calculate controller output from reference and feedback pointers
 *          - Support coefficient update and state reset for runtime tuning
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
#ifndef __PI_TUSTIN_H
#define __PI_TUSTIN_H

#include <stdbool.h>

typedef struct
{
    float *p_act;
    float *p_ref;
} pi_tustin_input_t;

typedef struct
{
    float a1;
    float b0;
    float b1;
    float b1_inv;
    float up_lmt;
    float dn_lmt;
    float e[2];
    float u[2];
} pi_tustin_inter_t;

typedef struct
{
    float val;
} pi_tustin_output_t;

typedef struct
{
    pi_tustin_input_t input;
    pi_tustin_inter_t inter;
    pi_tustin_output_t output;
} pi_tustin_t;

bool pi_tustin_init(pi_tustin_t *p_str,
                    float kp,
                    float ki,
                    float ts,
                    float up_lmt,
                    float dn_lmt,
                    float *p_ref,
                    float *p_act);

bool pi_tustin_cal(pi_tustin_t *p_str);

bool pi_tustin_update(pi_tustin_t *p_str, float kp, float ki, float ts);

static inline void pi_tustin_update_b0(pi_tustin_t *p_str, float b0)
{
    p_str->inter.b0 = b0;
}

static inline void pi_tustin_update_b1(pi_tustin_t *p_str, float b1)
{
    p_str->inter.b1 = b1;
}

void pi_tustin_reset(pi_tustin_t *p_str);

#endif
