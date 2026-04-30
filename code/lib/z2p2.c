// SPDX-License-Identifier: MIT
/**
 * @file    z2p2.c
 * @brief   z2p2 library module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement a two-zero two-pole digital compensator
 *          - Calculate compensator output from reference and feedback pointers
 *          - Apply configured limits for reusable closed-loop control blocks
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
#include "z2p2.h"
#include "my_math.h"

void z2p2_init(z2p2_t *p_str,
               float k,
               float fz,
               float fp,
               float ts,
               float up_lmt,
               float dn_lmt,
               float *p_ref,
               float *p_act)
{
    p_str->input.p_act = p_act;
    p_str->input.p_ref = p_ref;
    p_str->inter.up_lmt = up_lmt;
    p_str->inter.dn_lmt = dn_lmt;
    float wz1 = 2.0f * M_PI * fz;
    float wz2 = 2.0f * M_PI * fz;
    float wp1 = 2.0f * M_PI * fp;
    float wp2 = 2.0f * M_PI * fp;

    float n3 = k * ts * wp1 * wp2 * (4 - 2 * ts * wz1 - ts * wz2 + ts * ts * wz1 * wz2);
    float n2 = k * ts * wp1 * wp2 * (-4 - 2 * ts * wz1 - ts * wz2 + 3 * ts * ts * wz1 * wz2);
    float n1 = k * ts * wp1 * wp2 * (-4 + 2 * ts * wz1 + 2 * ts * wz2 + 3 * ts * ts * wz1 * wz2);
    float n0 = k * ts * wp1 * wp2 * (4 + 2 * ts * wz1 + 2 * ts * wz2 + ts * ts * wz1 * wz2);

    float d3 = 2 * (-4 + 2 * ts * wp1 + 2 * ts * wp2 - ts * ts * wp1 * wp2) * wz1 * wz2;
    float d2 = 2 * (12 - 2 * ts * wp1 - 2 * ts * wp2 - ts * ts * wp1 * wp2) * wz1 * wz2;
    float d1 = 2 * (-12 - 2 * ts * wp1 - 2 * ts * wp2 + ts * ts * wp1 * wp2) * wz1 * wz2;
    float d0 = 2 * (4 + 2 * ts * wp1 + 2 * ts * wp2 + ts * ts * wp1 * wp2) * wz1 * wz2;

    p_str->inter.a1 = d1 / d0;
    p_str->inter.a2 = d2 / d0;
    p_str->inter.a3 = d3 / d0;
    p_str->inter.b0 = n0 / d0;
    p_str->inter.b1 = n1 / d0;
    p_str->inter.b2 = n2 / d0;
    p_str->inter.b3 = n3 / d0;

    p_str->inter.e[3] = 0.0f;
    p_str->inter.e[2] = 0.0f;
    p_str->inter.e[1] = 0.0f;
    p_str->inter.e[0] = 0.0f;
    p_str->inter.u[3] = 0.0f;
    p_str->inter.u[2] = 0.0f;
    p_str->inter.u[1] = 0.0f;
    p_str->inter.u[0] = 0.0f;
}

void z2p2_cal(z2p2_t *p_str)
{
    p_str->inter.e[3] = p_str->inter.e[2];
    p_str->inter.e[2] = p_str->inter.e[2];
    p_str->inter.e[1] = p_str->inter.e[2];
    p_str->inter.e[0] = *p_str->input.p_ref - *p_str->input.p_act;

    p_str->inter.u[3] = p_str->inter.u[2];
    p_str->inter.u[2] = p_str->inter.u[1];
    p_str->inter.u[1] = p_str->inter.u[0];
    p_str->inter.u[0] = p_str->inter.b0 * p_str->inter.e[0] +
                        p_str->inter.b1 * p_str->inter.e[1] +
                        p_str->inter.b2 * p_str->inter.e[2] +
                        p_str->inter.b3 * p_str->inter.e[3] -
                        p_str->inter.a1 * p_str->inter.u[1] -
                        p_str->inter.a2 * p_str->inter.u[2] -
                        p_str->inter.a3 * p_str->inter.u[3];

    UP_DN_LMT(p_str->inter.u[0], p_str->inter.up_lmt, p_str->inter.dn_lmt);
    p_str->output.val = p_str->inter.u[0];
}
