// SPDX-License-Identifier: MIT
/**
 * @file    sogi_i32.c
 * @brief   Integer SOGI library module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Convert SOGI floating design parameters into fixed-point coefficients
 *          - Run the int32_t SOGI recurrence in caller-owned state
 *          - Keep integer histories and error output for FLL or control-loop users
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
#include "sogi_i32.h"
#ifdef IS_PLECS
#include "plecs.h"
#endif
#include <limits.h>
#include <stddef.h>

#define SOGI_I32_NUM_GAIN (128LL)

#define SOGI_I32_SAT_LOG_PERIOD_TICKS (1000U)

static uint32_t sogi_i32_sat_log_cnt = 0U;

static int32_t sogi_i32_sat_i64_to_i32(int64_t val)
{
    if (val > (int64_t)INT32_MAX)
    {
#ifdef IS_PLECS
        sogi_i32_sat_log_cnt++;
        if (sogi_i32_sat_log_cnt >= SOGI_I32_SAT_LOG_PERIOD_TICKS)
        {
            sogi_i32_sat_log_cnt = 0U;
            PLECS_LOG("sogi_i32 sat high val=%lld\n", (long long)val);
        }
#endif
        return INT32_MAX;
    }
    else if (val < (int64_t)INT32_MIN)
    {
#ifdef IS_PLECS
        sogi_i32_sat_log_cnt++;
        if (sogi_i32_sat_log_cnt >= SOGI_I32_SAT_LOG_PERIOD_TICKS)
        {
            sogi_i32_sat_log_cnt = 0U;
            PLECS_LOG("sogi_i32 sat low val=%lld\n", (long long)val);
        }
#endif
        return INT32_MIN;
    }

    return (int32_t)val;
}

static int32_t sogi_i32_float_to_i32(float val)
{
    if (val >= (float)INT32_MAX)
    {
        return INT32_MAX;
    }
    else if (val <= (float)INT32_MIN)
    {
        return INT32_MIN;
    }

    if (val >= 0.0f)
    {
        return (int32_t)(val + 0.5f);
    }

    return (int32_t)(val - 0.5f);
}

void sogi_i32_reset(sogi_i32_t *p_sogi)
{
    if (p_sogi == NULL)
    {
        return;
    }

    p_sogi->inter.x[0] = 0;
    p_sogi->inter.x[1] = 0;
    p_sogi->inter.x[2] = 0;
    p_sogi->inter.y[0] = 0;
    p_sogi->inter.y[1] = 0;
    p_sogi->inter.y[2] = 0;
    p_sogi->inter.qy[0] = 0;
    p_sogi->inter.qy[1] = 0;
    p_sogi->inter.qy[2] = 0;
    p_sogi->inter.err = 0;
    p_sogi->output.u = 0;
    p_sogi->output.qu = 0;
    p_sogi->output.err = 0;
}

bool sogi_i32_update(sogi_i32_t *p_sogi,
                     float ts,
                     float omega_radps,
                     float gain,
                     uint8_t coeff_q_shift)
{
    float coeff_q = 0.0f;
    float n0 = 0.0f;
    float n1 = 0.0f;
    float n2 = 0.0f;
    float d0 = 0.0f;
    float d1 = 0.0f;
    float d2 = 0.0f;

    if ((p_sogi == NULL) ||
        (ts <= 0.0f) ||
        (gain <= 0.0f) ||
        (coeff_q_shift >= 30U) ||
        (omega_radps <= 0.0f))
    {
        return false;
    }

    coeff_q = (float)(1L << coeff_q_shift);

    n0 = 2.0f * ts * gain * omega_radps;
    n2 = -2.0f * ts * gain * omega_radps;
    d0 = ts * ts * omega_radps * omega_radps + 2.0f * ts * gain * omega_radps + 4.0f;
    d1 = 2.0f * ts * ts * omega_radps * omega_radps - 8.0f;
    d2 = ts * ts * omega_radps * omega_radps - 2.0f * ts * gain * omega_radps + 4.0f;

    p_sogi->inter.b[0] = sogi_i32_float_to_i32((n0 / d0) *
                                               coeff_q *
                                               (float)SOGI_I32_NUM_GAIN);
    p_sogi->inter.b[1] = 0;
    p_sogi->inter.b[2] = sogi_i32_float_to_i32((n2 / d0) *
                                               coeff_q *
                                               (float)SOGI_I32_NUM_GAIN);
    p_sogi->inter.a[0] = sogi_i32_float_to_i32((d1 / d0) * coeff_q);
    p_sogi->inter.a[1] = sogi_i32_float_to_i32((d2 / d0) * coeff_q);

    n0 = ts * ts * gain * omega_radps * omega_radps;
    n1 = 2.0f * ts * ts * gain * omega_radps * omega_radps;
    n2 = ts * ts * gain * omega_radps * omega_radps;

    p_sogi->inter.qb[0] = sogi_i32_float_to_i32((n0 / d0) *
                                                coeff_q *
                                                (float)SOGI_I32_NUM_GAIN);
    p_sogi->inter.qb[1] = sogi_i32_float_to_i32((n1 / d0) *
                                                coeff_q *
                                                (float)SOGI_I32_NUM_GAIN);
    p_sogi->inter.qb[2] = sogi_i32_float_to_i32((n2 / d0) *
                                                coeff_q *
                                                (float)SOGI_I32_NUM_GAIN);
    p_sogi->inter.omega_mradps = sogi_i32_float_to_i32(omega_radps * 1000.0f);
    p_sogi->inter.coeff_q_shift = coeff_q_shift;

    return true;
}

bool sogi_i32_init(sogi_i32_t *p_sogi,
                   float ts,
                   float omega_radps,
                   float gain,
                   uint8_t coeff_q_shift,
                   int32_t *p_input)
{
    if ((p_sogi == NULL) ||
        (p_input == NULL))
    {
        return false;
    }

    p_sogi->input.p_input = p_input;
    sogi_i32_reset(p_sogi);

    return sogi_i32_update(p_sogi, ts, omega_radps, gain, coeff_q_shift);
}

bool sogi_i32_cal(sogi_i32_t *p_sogi)
{
    int64_t y = 0;
    int64_t qy = 0;

    if ((p_sogi == NULL) ||
        (p_sogi->input.p_input == NULL))
    {
        return false;
    }

    p_sogi->inter.x[2] = p_sogi->inter.x[1];
    p_sogi->inter.x[1] = p_sogi->inter.x[0];
    p_sogi->inter.x[0] = *p_sogi->input.p_input;

    p_sogi->inter.y[2] = p_sogi->inter.y[1];
    p_sogi->inter.y[1] = p_sogi->inter.y[0];
    p_sogi->inter.qy[2] = p_sogi->inter.qy[1];
    p_sogi->inter.qy[1] = p_sogi->inter.qy[0];

    y = ((int64_t)p_sogi->inter.b[0] * (int64_t)p_sogi->inter.x[0]) +
        ((int64_t)p_sogi->inter.b[1] * (int64_t)p_sogi->inter.x[1]) +
        ((int64_t)p_sogi->inter.b[2] * (int64_t)p_sogi->inter.x[2]) -
        ((int64_t)p_sogi->inter.a[0] * (int64_t)p_sogi->inter.y[1]) -
        ((int64_t)p_sogi->inter.a[1] * (int64_t)p_sogi->inter.y[2]);

    qy = ((int64_t)p_sogi->inter.qb[0] * (int64_t)p_sogi->inter.x[0]) +
         ((int64_t)p_sogi->inter.qb[1] * (int64_t)p_sogi->inter.x[1]) +
         ((int64_t)p_sogi->inter.qb[2] * (int64_t)p_sogi->inter.x[2]) -
         ((int64_t)p_sogi->inter.a[0] * (int64_t)p_sogi->inter.qy[1]) -
         ((int64_t)p_sogi->inter.a[1] * (int64_t)p_sogi->inter.qy[2]);

    y >>= p_sogi->inter.coeff_q_shift;
    qy >>= p_sogi->inter.coeff_q_shift;

    p_sogi->inter.y[0] = sogi_i32_sat_i64_to_i32(y);
    p_sogi->inter.qy[0] = sogi_i32_sat_i64_to_i32(qy);
    p_sogi->output.u = p_sogi->inter.y[0] / (int32_t)SOGI_I32_NUM_GAIN;
    p_sogi->output.qu = p_sogi->inter.qy[0] / (int32_t)SOGI_I32_NUM_GAIN;
    p_sogi->inter.err = sogi_i32_sat_i64_to_i32((int64_t)p_sogi->inter.x[0] -
                                                (int64_t)p_sogi->output.u);
    p_sogi->output.err = p_sogi->inter.err;

    return true;
}
