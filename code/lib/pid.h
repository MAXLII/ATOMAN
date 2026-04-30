// SPDX-License-Identifier: MIT
/**
 * @file    pid.h
 * @brief   pid library public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement positional PID control with output limiting
 *          - Maintain proportional, integral, and derivative terms in caller-owned state
 *          - Provide reset and calculation APIs for closed-loop controllers
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
#ifndef __PID_H
#define __PID_H

#include "stdint.h"

typedef struct
{
    float kp;
    float ki;
    float ki_inv;
    float kd;
    float i_err_lmt_max;
    float i_err_lmt_min;
    float output_lmt_max;
    float output_lmt_min;
} pid_cfg_t;

typedef struct
{
    float ref;
    float act;
} pid_input_t;

typedef struct
{
    float output;
} pid_output_t;

typedef struct
{
    float err;
    float err_last;
    float err_diff;
    float i_err;
} pid_inter_t;

typedef struct
{
    pid_cfg_t cfg;
    pid_inter_t inter;
    pid_input_t input;
    pid_output_t output;
} pid_param_t;

void pid_reset(pid_param_t *pid_param);
float pid_cal(pid_param_t *pid_param, float ref, float act);

#endif
