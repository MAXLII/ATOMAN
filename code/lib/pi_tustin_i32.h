// SPDX-License-Identifier: MIT
/**
 * @file    pi_tustin_i32.h
 * @brief   Integer PI Tustin controller public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement an int32_t runtime PI controller using Tustin discretization
 *          - Calculate controller output from int32_t reference and feedback pointers
 *          - Convert float tuning parameters to int32_t control coefficients
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe when caller owns the instance and input pointers
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-23
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __PI_TUSTIN_I32_H
#define __PI_TUSTIN_I32_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    int32_t *p_act;
    int32_t *p_ref;
} pi_tustin_i32_input_t;

typedef struct
{
    int32_t a1;
    int32_t b0;
    int32_t b1;
    int32_t up_lmt;
    int32_t dn_lmt;
    int32_t e[2];
    int32_t u[2];
} pi_tustin_i32_inter_t;

typedef struct
{
    int32_t val;
} pi_tustin_i32_output_t;

typedef struct
{
    pi_tustin_i32_input_t input;
    pi_tustin_i32_inter_t inter;
    pi_tustin_i32_output_t output;
} pi_tustin_i32_t;

bool pi_tustin_i32_init(pi_tustin_i32_t *p_str,
                        float kp,
                        float ki,
                        float ts,
                        int32_t up_lmt,
                        int32_t dn_lmt,
                        int32_t *p_ref,
                        int32_t *p_act);

bool pi_tustin_i32_cal(pi_tustin_i32_t *p_str);

bool pi_tustin_i32_update(pi_tustin_i32_t *p_str,
                          float kp,
                          float ki,
                          float ts);

static inline void pi_tustin_i32_update_b0(pi_tustin_i32_t *p_str, int32_t b0)
{
    p_str->inter.b0 = b0;
}

static inline void pi_tustin_i32_update_b1(pi_tustin_i32_t *p_str, int32_t b1)
{
    p_str->inter.b1 = b1;
}

static inline int32_t pi_tustin_i32_sat_i64_to_i32(int64_t val)
{
    if (val > (int64_t)INT32_MAX)
    {
        return INT32_MAX;
    }
    else if (val < (int64_t)INT32_MIN)
    {
        return INT32_MIN;
    }

    return (int32_t)val;
}

static inline int32_t pi_tustin_i32_div_i64_sat_i32(int64_t numerator, int32_t denominator)
{
    /* Saturate first so integer division remains in the 32-bit runtime path. */
    int32_t numerator_i32 = 0;

    numerator_i32 = pi_tustin_i32_sat_i64_to_i32(numerator);
    if (denominator == 0)
    {
        return numerator_i32;
    }

    return numerator_i32 / denominator;
}

static inline void pi_tustin_i32_cal_inline(pi_tustin_i32_t *p_str)
{
    int64_t val = 0;
    int64_t anti_windup = 0;

    p_str->inter.e[0] = pi_tustin_i32_sat_i64_to_i32((int64_t)*p_str->input.p_ref -
                                                     (int64_t)*p_str->input.p_act);
    p_str->inter.u[1] = p_str->inter.u[0];

    val = (int64_t)p_str->inter.b0 * (int64_t)p_str->inter.e[0] +
          (int64_t)p_str->inter.b1 * (int64_t)p_str->inter.e[1] -
          (int64_t)p_str->inter.a1 * (int64_t)p_str->inter.u[1];

    if (val > (int64_t)p_str->inter.up_lmt)
    {
        val = p_str->inter.up_lmt;
    }
    else if (val < (int64_t)p_str->inter.dn_lmt)
    {
        val = p_str->inter.dn_lmt;
    }

    if ((val == (int64_t)p_str->inter.up_lmt) ||
        (val == (int64_t)p_str->inter.dn_lmt))
    {
        if (p_str->inter.b1 == 0)
        {
            p_str->inter.e[1] = p_str->inter.e[0];
        }
        else
        {
            anti_windup = val -
                          (int64_t)p_str->inter.b0 * (int64_t)p_str->inter.e[0] +
                          (int64_t)p_str->inter.a1 * (int64_t)p_str->inter.u[1];
            p_str->inter.e[1] = pi_tustin_i32_div_i64_sat_i32(anti_windup,
                                                              p_str->inter.b1);
        }
    }
    else
    {
        p_str->inter.e[1] = p_str->inter.e[0];
    }

    p_str->inter.u[0] = (int32_t)val;
    p_str->output.val = (int32_t)val;
}

static inline void pi_tustin_i32_cal_a1_neg1_inline(pi_tustin_i32_t *p_str)
{
    int64_t val = 0;
    int64_t anti_windup = 0;

    p_str->inter.e[0] = pi_tustin_i32_sat_i64_to_i32((int64_t)*p_str->input.p_ref -
                                                     (int64_t)*p_str->input.p_act);
    p_str->inter.u[1] = p_str->inter.u[0];

    /* Fast Tustin path for controllers whose update equation always has a1 equal to minus one. */
    val = (int64_t)p_str->inter.b0 * (int64_t)p_str->inter.e[0] +
          (int64_t)p_str->inter.b1 * (int64_t)p_str->inter.e[1] +
          (int64_t)p_str->inter.u[1];

    if (val > (int64_t)p_str->inter.up_lmt)
    {
        val = p_str->inter.up_lmt;
    }
    else if (val < (int64_t)p_str->inter.dn_lmt)
    {
        val = p_str->inter.dn_lmt;
    }

    if ((val == (int64_t)p_str->inter.up_lmt) ||
        (val == (int64_t)p_str->inter.dn_lmt))
    {
        if (p_str->inter.b1 == 0)
        {
            p_str->inter.e[1] = p_str->inter.e[0];
        }
        else
        {
            anti_windup = val -
                          (int64_t)p_str->inter.b0 * (int64_t)p_str->inter.e[0] -
                          (int64_t)p_str->inter.u[1];
            p_str->inter.e[1] = pi_tustin_i32_div_i64_sat_i32(anti_windup,
                                                              p_str->inter.b1);
        }
    }
    else
    {
        p_str->inter.e[1] = p_str->inter.e[0];
    }

    p_str->inter.u[0] = (int32_t)val;
    p_str->output.val = (int32_t)val;
}

static inline void pi_tustin_i32_reset_inline(pi_tustin_i32_t *p_str)
{
    p_str->inter.e[0] = 0;
    p_str->inter.e[1] = 0;
    p_str->inter.u[0] = 0;
    p_str->inter.u[1] = 0;
    p_str->output.val = 0;
}

void pi_tustin_i32_reset(pi_tustin_i32_t *p_str);

#endif
