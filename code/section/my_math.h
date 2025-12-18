#ifndef __MY_MATH_H
#define __MY_MATH_H

#ifdef IS_PLECS
#include "math.h"
#else
#include "math.h"
#endif

#define BUCK_PWM_FREQ 60.0e3f
#define PFC_PWM_FREQ 30.0e3f
#define CTRL_FREQ 30.0e3f
#define BUCK_PWM_TS (1.0f / BUCK_PWM_FREQ)
#define CTRL_TS (1.0f / CTRL_FREQ)

#define UP_LMT(in, lmt) (in = ((in > (lmt)) ? (lmt) : in))
#define DN_LMT(in, lmt) (in = ((in < (lmt)) ? (lmt) : in))
#define UP_DN_LMT(in, up_lmt, dn_lmt) (in = ((in > (up_lmt)) ? (up_lmt) : ((in < (dn_lmt)) ? (dn_lmt) : in)))

#define MIN(val, a, b) (val) = ((a) < (b)) ? (a) : (b)

#define HPF(in, in_last, out, Ts, wc) out = 2.0f / (Ts * wc + 2.0f) * in -      \
                                            2.0f / (Ts * wc + 2.0f) * in_last - \
                                            (Ts * wc - 2) / (Ts * wc + 2) * out

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

#define DQ_CAL(a, b, sintheta, costheta, d, q) \
    d = costheta * a + sintheta * b;           \
    q = sintheta * a - costheta * b;

#define INC_AND_WRAP(count, max_value) \
    do                                 \
    {                                  \
        (count)++;                     \
        if ((count) >= (max_value))    \
        {                              \
            (count) -= (max_value);    \
        }                              \
    } while (0)

#define DN_CNT(cnt) \
    do              \
    {               \
        if (cnt)    \
        {           \
            cnt--;  \
        }           \
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

#ifdef M_2PI
#undef M_2PI
#endif
#define M_2PI (2.0f * M_PI)

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

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

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

#define TIME_CNT_1MS_IN_CTRL ((uint32_t)(0.001f * CTRL_FREQ))
#define TIME_CNT_10MS_IN_CTRL (10 * TIME_CNT_1MS_IN_CTRL)
#define TIME_CNT_50MS_IN_CTRL (50 * TIME_CNT_1MS_IN_CTRL)
#define TIME_CNT_100MS_IN_CTRL (100 * TIME_CNT_1MS_IN_CTRL)

#endif
