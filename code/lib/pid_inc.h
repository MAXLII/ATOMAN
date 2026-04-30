// SPDX-License-Identifier: MIT
/**
 * @file    pid_inc.h
 * @brief   pid_inc library public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement incremental PID control with output limits
 *          - Update controller output from target and feedback deltas
 *          - Provide parameter update and reset APIs for runtime tuning
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
#ifndef __PID_INC_H
#define __PID_INC_H

typedef struct
{
    float Kp; // 比例系数
    float Ki; // 积分系数
    float Kd; // 微分系数

    float error;       // 当前误差
    float error_prev1; // 前一次误差
    float error_prev2; // 前两次误差

    float output;      // 当前输出值
    float output_prev; // 上一次输出值

    float output_max; // 输出上限
    float output_min; // 输出下限

    float delta_max; // 增量输出上限
    float delta_min; // 增量输出下限
} pid_inc_t;

void pid_inc_Init(pid_inc_t *pid,
                  float kp,
                  float ki,
                  float kd,
                  float out_max,
                  float out_min,
                  float delta_max,
                  float delta_min);

float pid_inc_Calculate(pid_inc_t *pid, float target, float feedback);

void pid_inc_SetParameters(pid_inc_t *pid, float kp, float ki, float kd);

void pid_inc_Reset(pid_inc_t *pid);

#endif
