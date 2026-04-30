// SPDX-License-Identifier: MIT
/**
 * @file    bb_ol.h
 * @brief   bb_ol library public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Calculate open-loop duty commands for Buck, Boost, and Buck-Boost operation
 *          - Support DCM-oriented duty and on-time estimation from electrical parameters
 *          - Limit generated duty values before they are consumed by converter control logic
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
#ifndef __BB_OL_H
#define __BB_OL_H

#include <stdint.h>

#define BB_OL_FIX_DUTY (0.8f)

typedef enum
{
    BB_OL_MODE_BUCK = 0,
    BB_OL_MODE_BOOST,
    BB_OL_MODE_BUCK_BOOST,
} bb_ol_mode_e;

typedef struct
{
    bb_ol_mode_e *p_mode;
    float *p_i_ref;
    float *p_pwm_ts;
    float *p_v_in;
    float *p_v_out;
    float *p_l;
} bb_ol_input_t;

typedef struct
{
    uint8_t reserved;
} bb_ol_inter_t;

typedef struct
{
    float buck_duty;
    uint8_t buck_up_en;
    uint8_t buck_dn_en;
    float boost_duty;
    uint8_t boost_up_en;
    uint8_t boost_dn_en;
} bb_ol_output_t;

typedef struct
{
    bb_ol_input_t input;
    bb_ol_inter_t inter;
    bb_ol_output_t output;
} bb_ol_t;

void bb_ol_init(bb_ol_t *p_ol,
                bb_ol_mode_e *p_mode,
                float *p_i_ref,
                float *p_pwm_ts,
                float *p_v_in,
                float *p_v_out,
                float *p_l);
void bb_ol_func(bb_ol_t *p_ol);

#endif
