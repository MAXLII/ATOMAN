// SPDX-License-Identifier: MIT
/**
 * @file    my_math.h
 * @brief   Control math helper macros and inline coordinate transforms.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Require shared control-loop timing constants
 *          - Provide clamp, counter, filter, transform, and angle helper macros
 *          - Keep lightweight math utilities available to ISR and control paths without allocation
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
#ifndef __MY_MATH_H
#define __MY_MATH_H

#include "math.h"
#include <stdint.h>
#include <stddef.h>
#include "timing.h"

#ifndef CTRL_TS
#error "CTRL_TS is not defined. Please define the control timebase in timing.h."
#endif

/* In-place clamp helpers. */
#define UP_LMT(in, lmt) (in = ((in > (lmt)) ? (lmt) : in))
#define DN_LMT(in, lmt) (in = ((in < (lmt)) ? (lmt) : in))
#define UP_DN_LMT(in, up_lmt, dn_lmt) (in = ((in > (up_lmt)) ? (up_lmt) : ((in < (dn_lmt)) ? (dn_lmt) : in)))

/* Assign the smaller value of a and b to val. */
#define MIN(val, a, b) (val) = ((a) < (b)) ? (a) : (b)

/* First-order high-pass filter in discrete form. */
#define HPF(in, in_last, out, Ts, wc) out = 2.0f / (Ts * wc + 2.0f) * in -      \
                                            2.0f / (Ts * wc + 2.0f) * in_last - \
                                            (Ts * wc - 2) / (Ts * wc + 2) * out

/* First-order low-pass filter in discrete form; updates in_last internally. */
#define LPF(in, in_last, out, Ts, wc)                   \
    do                                                  \
    {                                                   \
        const float b0 = Ts * wc / (Ts * wc + 2.0f);    \
        const float b1 = b0;                            \
        const float a1 = (Ts * wc - 2) / (Ts * wc + 2); \
        out = b0 * in +                                 \
              b1 * in_last -                            \
              a1 * out;                                 \
        in_last = in;                                   \
    } while (0)

/* Alpha-beta to dq transform. */
#define DQ_CAL(a, b, sintheta, costheta, d, q) \
    d = costheta * a + sintheta * b;           \
    q = -sintheta * a + costheta * b;

/* Increment a counter and wrap it into [0, max_value). */
#define INC_AND_WRAP(count, max_value) \
    do                                 \
    {                                  \
        (count)++;                     \
        if ((count) >= (max_value))    \
        {                              \
            (count) -= (max_value);    \
        }                              \
    } while (0)

/* Safe down-counter: decrement only when cnt is non-zero. */
#define DN_CNT(cnt)  \
    do               \
    {                \
        if (cnt)     \
        {            \
            (cnt)--; \
        }            \
    } while (0)

#ifdef M_E
#undef M_E
#endif
#define M_E 2.7182818284590452354f

#ifdef M_LOG2E
#undef M_LOG2E
#endif
#define M_LOG2E 1.4426950408889634074f

#ifdef M_LOG10E
#undef M_LOG10E
#endif
#define M_LOG10E 0.43429448190325182765f

#ifdef M_LN2
#undef M_LN2
#endif
#define M_LN2 0.69314718055994530942f

#ifdef M_LN10
#undef M_LN10
#endif
#define M_LN10 2.30258509299404568402f

#ifdef M_PI
#undef M_PI
#endif
#define M_PI 3.14159265358979323846f

#ifdef M_PI_2
#undef M_PI_2
#endif
#define M_PI_2 1.57079632679489661923f

#ifdef M_PI_4
#undef M_PI_4
#endif
#define M_PI_4 0.78539816339744830962f

#ifdef M_1_PI
#undef M_1_PI
#endif
#define M_1_PI 0.31830988618379067154f

#ifdef M_2_PI
#undef M_2_PI
#endif
#define M_2_PI 0.63661977236758134308f

#ifdef M_2_SQRTPI
#undef M_2_SQRTPI
#endif
#define M_2_SQRTPI 1.12837916709551257390f

#ifdef M_SQRT2
#undef M_SQRT2
#endif
#define M_SQRT2 1.41421356237309504880f

#ifdef M_SQRT1_2
#undef M_SQRT1_2
#endif
#define M_SQRT1_2 0.70710678118654752440f


#ifdef M_SQRT3_2
#undef M_SQRT3_2
#endif
#define M_SQRT3_2 0.8660254037844386f /* sqrt(3) / 2 */

#ifdef M_1_SQRT3
#undef M_1_SQRT3
#endif
#define M_1_SQRT3 0.5773502691896258f /* 1 / sqrt(3) */

#ifdef M_2PI
#undef M_2PI
#endif
#define M_2PI (2.0f * M_PI)

/**
 * @brief Amplitude-invariant Clarke transform; zero sequence is discarded.
 * @param a Phase A value.
 * @param b Phase B value, lagging A by 120 degrees for positive sequence.
 * @param c Phase C value, leading A by 120 degrees for positive sequence.
 * @param p_alpha Alpha output; valid and distinct from p_beta.
 * @param p_beta Beta output; valid and distinct from p_alpha.
 */
static inline void clarke(float a, float b, float c, float *p_alpha, float *p_beta)
{
    *p_alpha = (2.0f * a - b - c) / 3.0f;
    *p_beta = (b - c) * M_1_SQRT3;
}

/**
 * @brief Inverse amplitude-invariant Clarke transform with zero sequence set to zero.
 * @param alpha Alpha-axis value.
 * @param beta Beta-axis value.
 * @param p_a Phase A output; all output pointers must be valid and distinct.
 * @param p_b Phase B output.
 * @param p_c Phase C output.
 */
static inline void inv_clarke(float alpha, float beta, float *p_a, float *p_b, float *p_c)
{
    *p_a = alpha;
    *p_b = -0.5f * alpha + M_SQRT3_2 * beta;
    *p_c = -0.5f * alpha - M_SQRT3_2 * beta;
}

/**
 * @brief Park transform; the d axis follows theta and q is positive at theta + pi/2.
 * @param alpha Alpha-axis value.
 * @param beta Beta-axis value.
 * @param sine Sine of theta; negate for a negative-sequence rotating frame.
 * @param cosine Cosine of theta.
 * @param p_d Direct-axis output; valid and distinct from p_q.
 * @param p_q Quadrature-axis output; valid and distinct from p_d.
 */
static inline void park(float alpha, float beta, float sine, float cosine, float *p_d, float *p_q)
{
    *p_d = cosine * alpha + sine * beta;
    *p_q = -sine * alpha + cosine * beta;
}

/**
 * @brief Inverse Park transform using the same angle convention as park.
 * @param d Direct-axis value.
 * @param q Quadrature-axis value.
 * @param sine Sine of theta; negate for a negative-sequence rotating frame.
 * @param cosine Cosine of theta.
 * @param p_alpha Alpha output; valid and distinct from p_beta.
 * @param p_beta Beta output; valid and distinct from p_alpha.
 */
static inline void inv_park(float d, float q, float sine, float cosine, float *p_alpha, float *p_beta)
{
    *p_alpha = cosine * d - sine * q;
    *p_beta = sine * d + cosine * q;
}

/* Symmetric ramp: move act toward tag with the same step in both directions. */
#define RAMP(act, tag, step)            \
    do                                  \
    {                                   \
        const float step_temp = step;   \
        (act > tag + step_temp)         \
            ? (act -= step_temp)        \
            : ((act < tag - step_temp)  \
                   ? (act += step_temp) \
                   : (act = tag));      \
    } while (0)

/* Asymmetric ramp: independent rising and falling slew limits. */
#define RAMP_UP_DN(act, tag, up_step, dn_step) \
    do                                         \
    {                                          \
        const float up_step_temp = up_step;    \
        const float dn_step_temp = dn_step;    \
        (act > tag + dn_step_temp)             \
            ? (act -= dn_step_temp)            \
            : ((act < tag - up_step_temp)      \
                   ? (act += up_step_temp)     \
                   : (act = tag));             \
    } while (0)

/* Number of elements in a static array. */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static inline uint8_t struct_all_ptr_valid(const void *obj, uint16_t ptr_count)
{
    const uintptr_t *p = (const uintptr_t *)obj;

    if (p == NULL)
    {
        return 0U;
    }

    for (uint16_t i = 0U; i < ptr_count; ++i)
    {
        if (p[i] == (uintptr_t)0U)
        {
            return 0U;
        }
    }

    return 1U;
}

/* Check whether every member in a pointer-only struct is non-NULL. */
#define STRUCT_ALL_PTR_VALID(obj) \
    struct_all_ptr_valid(&(obj), (uint16_t)(sizeof(obj) / sizeof(uintptr_t)))

/* Time constants expressed in 1 ms task ticks. */
#define TIME_CNT_1MS_IN_1MS (1)
#define TIME_CNT_5MS_IN_1MS (5 * TIME_CNT_1MS_IN_1MS)
#define TIME_CNT_10MS_IN_1MS (10 * TIME_CNT_1MS_IN_1MS)
#define TIME_CNT_20MS_IN_1MS (20 * TIME_CNT_1MS_IN_1MS)
#define TIME_CNT_50MS_IN_1MS (50 * TIME_CNT_1MS_IN_1MS)
#define TIME_CNT_100MS_IN_1MS (100 * TIME_CNT_1MS_IN_1MS)
#define TIME_CNT_200MS_IN_1MS (200 * TIME_CNT_1MS_IN_1MS)
#define TIME_CNT_300MS_IN_1MS (300 * TIME_CNT_1MS_IN_1MS)
#define TIME_CNT_400MS_IN_1MS (400 * TIME_CNT_1MS_IN_1MS)
#define TIME_CNT_500MS_IN_1MS (500 * TIME_CNT_1MS_IN_1MS)
#define TIME_CNT_1S_IN_1MS (1000 * TIME_CNT_1MS_IN_1MS)
#define TIME_CNT_2S_IN_1MS (2 * TIME_CNT_1S_IN_1MS)
#define TIME_CNT_3S_IN_1MS (3 * TIME_CNT_1S_IN_1MS)
#define TIME_CNT_5S_IN_1MS (5 * TIME_CNT_1S_IN_1MS)
#define TIME_CNT_10S_IN_1MS (10 * TIME_CNT_1S_IN_1MS)

/* Time constants expressed in control ISR ticks. */
#define TIME_CNT_1MS_IN_CTRL ((uint32_t)(0.001f / CTRL_TS))
#define TIME_CNT_10MS_IN_CTRL (10 * TIME_CNT_1MS_IN_CTRL)
#define TIME_CNT_50MS_IN_CTRL (50 * TIME_CNT_1MS_IN_CTRL)
#define TIME_CNT_100MS_IN_CTRL (100 * TIME_CNT_1MS_IN_CTRL)

/* Time constants expressed in 100 us base ticks. */
#define TIME_CNT_100US_IN_100US (1)
#define TIME_CNT_1MS_IN_100US (10 * TIME_CNT_100US_IN_100US)
#define TIME_CNT_10MS_IN_100US (10 * TIME_CNT_1MS_IN_100US)
#define TIME_CNT_500MS_IN_100US (500 * TIME_CNT_1MS_IN_100US)

#endif
