#ifndef __MY_MATH_H
#define __MY_MATH_H

#ifdef IS_PLECS
#include "math.h"
#else
#include "math.h"
#endif

#define PWM_FREQ 22.2e3f
#define CTRL_FREQ 22.2e3f
#define PWM_TS (1.0f / PWM_FREQ)
#define CTRL_TS (1.0f / CTRL_FREQ)

#define UP_LMT(in, lmt) (in = ((in > (lmt)) ? (lmt) : in))
#define DN_LMT(in, lmt) (in = ((in < (lmt)) ? (lmt) : in))
#define UP_DN_LMT(in, up_lmt, dn_lmt) (in = ((in > (up_lmt)) ? (up_lmt) : ((in < (dn_lmt)) ? (dn_lmt) : in)))

#define MIN(val, a, b) (val) = ((a) < (b)) ? (a) : (b)

#define HPF(in, in_last, out, Ts, wc) out = 2.0f / (Ts * wc + 2.0f) * in -      \
                                            2.0f / (Ts * wc + 2.0f) * in_last - \
                                            (Ts * wc - 2) / (Ts * wc + 2) * out

#define LPF(in, in_last, out, Ts, wc) out = Ts * wc / (Ts * wc + 2.0f) * in +      \
                                            Ts * wc / (Ts * wc + 2.0f) * in_last - \
                                            (Ts * wc - 2) / (Ts * wc + 2) * out

#ifndef M_E
#define M_E 2.7182818284590452354
#endif
#ifndef M_LOG2E
#define M_LOG2E 1.4426950408889634074
#endif
#ifndef M_LOG10E
#define M_LOG10E 0.43429448190325182765
#endif
#ifndef M_LN2
#define M_LN2 0.69314718055994530942
#endif
#ifndef M_LN10
#define M_LN10 2.30258509299404568402
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif
#ifndef M_PI_4
#define M_PI_4 0.78539816339744830962
#endif
#ifndef M_1_PI
#define M_1_PI 0.31830988618379067154
#endif
#ifndef M_2_PI
#define M_2_PI 0.63661977236758134308
#endif
#ifndef M_2_SQRTPI
#define M_2_SQRTPI 1.12837916709551257390
#endif
#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2 0.70710678118654752440
#endif
#ifndef M_2PI
#define M_2PI (2.0f * M_PI)
#endif

#define RAMP(act, tag, step) (act > tag + step)         \
                                 ? (act -= step)        \
                                 : ((act < tag - step)  \
                                        ? (act += step) \
                                        : (act = tag))

#define TIME_CNT_1MS_IN_1MS (1)
#define TIME_CNT_5MS_IN_1MS (5 * TIME_CNT_1MS_IN_1MS)
#define TIME_CNT_10MS_IN_1MS (10 * TIME_CNT_1MS_IN_1MS)
#define TIME_CNT_20MS_IN_1MS (20 * TIME_CNT_1MS_IN_1MS)
#define TIME_CNT_200MS_IN_1MS (200 * TIME_CNT_1MS_IN_1MS)
#define TIME_CNT_1S_IN_1MS (1000 * TIME_CNT_1MS_IN_1MS)
#define TIME_CNT_2S_IN_1MS (2 * TIME_CNT_1S_IN_1MS)
#define TIME_CNT_3S_IN_1MS (3 * TIME_CNT_1S_IN_1MS)
#define TIME_CNT_10S_IN_1MS (10 * TIME_CNT_1S_IN_1MS)

#endif
