// SPDX-License-Identifier: MIT
/**
 * @file    trig_i32.c
 * @brief   Integer phase-to-sine/cosine lookup module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Store a Q15 quarter-wave sine table
 *          - Reconstruct signed sine values for every Q32 phase quadrant
 *          - Interpolate adjacent samples with integer multiply and shift operations
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Runtime path uses no division
 *          - No hardware access
 *
 * @author  Max.Li
 * @date    2026-08-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "trig_i32.h"

#include <stddef.h>

#define TRIG_I32_QUARTER_BITS (30U)
#define TRIG_I32_TABLE_INDEX_BITS (8U)
#define TRIG_I32_TABLE_FRAC_BITS (TRIG_I32_QUARTER_BITS - TRIG_I32_TABLE_INDEX_BITS)
#define TRIG_I32_QUARTER_PHASE (1UL << TRIG_I32_QUARTER_BITS)
#define TRIG_I32_QUARTER_MASK (TRIG_I32_QUARTER_PHASE - 1UL)

static const int16_t sin_quarter_q15[257] = {
    0, 201, 402, 603, 804, 1005, 1206, 1407,
    1608, 1809, 2009, 2210, 2410, 2611, 2811, 3012,
    3212, 3412, 3612, 3811, 4011, 4210, 4410, 4609,
    4808, 5007, 5205, 5404, 5602, 5800, 5998, 6195,
    6393, 6590, 6786, 6983, 7179, 7375, 7571, 7767,
    7962, 8157, 8351, 8545, 8739, 8933, 9126, 9319,
    9512, 9704, 9896, 10087, 10278, 10469, 10659, 10849,
    11039, 11228, 11417, 11605, 11793, 11980, 12167, 12353,
    12539, 12725, 12910, 13094, 13279, 13462, 13645, 13828,
    14010, 14191, 14372, 14553, 14732, 14912, 15090, 15269,
    15446, 15623, 15800, 15976, 16151, 16325, 16499, 16673,
    16846, 17018, 17189, 17360, 17530, 17700, 17869, 18037,
    18204, 18371, 18537, 18703, 18868, 19032, 19195, 19357,
    19519, 19680, 19841, 20000, 20159, 20317, 20475, 20631,
    20787, 20942, 21096, 21250, 21403, 21554, 21705, 21856,
    22005, 22154, 22301, 22448, 22594, 22739, 22884, 23027,
    23170, 23311, 23452, 23592, 23731, 23870, 24007, 24143,
    24279, 24413, 24547, 24680, 24811, 24942, 25072, 25201,
    25329, 25456, 25582, 25708, 25832, 25955, 26077, 26198,
    26319, 26438, 26556, 26674, 26790, 26905, 27019, 27133,
    27245, 27356, 27466, 27575, 27683, 27790, 27896, 28001,
    28105, 28208, 28310, 28411, 28510, 28609, 28706, 28803,
    28898, 28992, 29085, 29177, 29268, 29358, 29447, 29534,
    29621, 29706, 29791, 29874, 29956, 30037, 30117, 30195,
    30273, 30349, 30424, 30498, 30571, 30643, 30714, 30783,
    30852, 30919, 30985, 31050, 31113, 31176, 31237, 31297,
    31356, 31414, 31470, 31526, 31580, 31633, 31685, 31736,
    31785, 31833, 31880, 31926, 31971, 32014, 32057, 32098,
    32137, 32176, 32213, 32250, 32285, 32318, 32351, 32382,
    32412, 32441, 32469, 32495, 32521, 32545, 32567, 32589,
    32609, 32628, 32646, 32663, 32678, 32692, 32705, 32717,
    32728, 32737, 32745, 32752, 32757, 32761, 32765, 32766,
    32767};

static int32_t quarter_sin_q15(uint32_t quarter_phase)
{
    uint32_t index = 0U;    /**< Quarter-wave table index. */
    uint32_t fraction = 0U; /**< Q22 interpolation fraction. */
    int32_t base = 0;       /**< Lower table sample. */
    int32_t delta = 0;      /**< Difference to the next table sample. */

    if (quarter_phase >= TRIG_I32_QUARTER_PHASE)
    {
        return TRIG_I32_Q15_ONE;
    }

    index = quarter_phase >> TRIG_I32_TABLE_FRAC_BITS;
    fraction = quarter_phase & ((1UL << TRIG_I32_TABLE_FRAC_BITS) - 1UL);
    base = sin_quarter_q15[index];
    delta = (int32_t)sin_quarter_q15[index + 1U] - base;
    return base + (int32_t)(((int64_t)delta * (int64_t)fraction) >>
                            TRIG_I32_TABLE_FRAC_BITS);
}

static int32_t sin_q15(uint32_t phase_q32)
{
    uint32_t quadrant = phase_q32 >> TRIG_I32_QUARTER_BITS; /**< Phase quadrant. */
    uint32_t offset = phase_q32 & TRIG_I32_QUARTER_MASK;    /**< Position inside the quadrant. */

    switch (quadrant)
    {
    case 0U:
        return quarter_sin_q15(offset);
    case 1U:
        return quarter_sin_q15(TRIG_I32_QUARTER_PHASE - offset);
    case 2U:
        return -quarter_sin_q15(offset);
    default:
        return -quarter_sin_q15(TRIG_I32_QUARTER_PHASE - offset);
    }
}

void trig_i32_sin_cos_q15(uint32_t phase_q32, int32_t *p_sin_q15, int32_t *p_cos_q15)
{
    if ((p_sin_q15 == NULL) ||
        (p_cos_q15 == NULL))
    {
        return;
    }

    *p_sin_q15 = sin_q15(phase_q32);
    *p_cos_q15 = sin_q15(phase_q32 + TRIG_I32_QUARTER_PHASE);
}
