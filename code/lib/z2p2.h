// SPDX-License-Identifier: MIT
/**
 * @file    z2p2.h
 * @brief   z2p2 library public interface.
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
#ifndef __Z2P2_H
#define __Z2P2_H

typedef struct
{
    float *p_ref;
    float *p_act;
} z2p2_input_t;

typedef struct
{
    float a1;
    float a2;
    float a3;
    float b0;
    float b1;
    float b2;
    float b3;
    float e[4];
    float u[4];
    float up_lmt;
    float dn_lmt;
} z2p2_inter_t;

typedef struct
{
    float val;
} z2p2_output_t;

typedef struct
{
    z2p2_input_t input;
    z2p2_inter_t inter;
    z2p2_output_t output;
} z2p2_t;

void z2p2_init(z2p2_t *p_str, float k, float fz, float fp, float ts, float up_lmt, float dn_lmt, float *p_ref, float *p_act);

void z2p2_cal(z2p2_t *p_str);

#endif
