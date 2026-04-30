// SPDX-License-Identifier: MIT
/**
 * @file    ac_loss_det.h
 * @brief   ac_loss_det library public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Detect AC input loss from sampled voltage history
 *          - Track zero-crossing and voltage-difference conditions
 *          - Expose reset and periodic detection routines for protection logic
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
#ifndef __AC_LOSS_DET_H
#define __AC_LOSS_DET_H

#include "stdint.h"

#define AC_LOSS_DET_TS 100e-6f
#define AC_LOSS_DET_BUFF_SIZE (333 + 10) // 100us的周期时间最短频率30hz

#define AC_LOSS_DET_ZERO_VOLT_POS 50.0f
#define AC_LOSS_DET_ZERO_VOLT_NEG -50.0f

#define AC_LOSS_DET_DIFF 20.0f

#define AC_LOSS_DET_DIFF_OVF_TIME 0.001f
#define AC_LOSS_DET_DIFF_OVF_TIME_CNT ((uint32_t)(AC_LOSS_DET_DIFF_OVF_TIME / AC_LOSS_DET_TS))

typedef enum
{
    AC_LOSS_DET_STA_IDLE,
    AC_LOSS_DET_STA_WAIT_NEG,
    AC_LOSS_DET_STA_WAIT_POS,
    AC_LOSS_DET_STA_FIRST_LOOP_POS,
    AC_LOSS_DET_STA_FIRST_LOOP_NEG,
    AC_LOSS_DET_STA_DET_POS,
    AC_LOSS_DET_STA_DET_NEG,
} AC_LOSS_DET_STA_E;

typedef struct
{
    float *p_v;
    uint8_t *p_ac_is_ok;
} ac_loss_det_input_t;

typedef struct
{
    float buffer[AC_LOSS_DET_BUFF_SIZE];
    uint32_t buffer_size;
    AC_LOSS_DET_STA_E sta;
    uint32_t buffer_index;
    uint32_t ovf_diff_cnt;
    float diff_volt;
} ac_loss_det_inter_t;

typedef struct
{
    uint8_t is_loss;
} ac_loss_det_output_t;

typedef struct
{
    ac_loss_det_input_t input;
    ac_loss_det_inter_t inter;
    ac_loss_det_output_t output;
} ac_loss_det_t;

void ac_loss_det_init(ac_loss_det_t *p_str, float *p_v, uint8_t *p_ac_is_ok);

void ac_loss_det_reset(ac_loss_det_t *p_str);

void ac_loss_det_func(ac_loss_det_t *p_str);

#endif
