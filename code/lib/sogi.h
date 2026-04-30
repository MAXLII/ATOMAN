// SPDX-License-Identifier: MIT
/**
 * @file    sogi.h
 * @brief   sogi library public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement a second-order generalized integrator signal observer
 *          - Generate orthogonal signal components from sampled input feedback
 *          - Support frequency updates for grid-synchronous signal processing
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
#ifndef SOGI_H
#define SOGI_H

typedef struct
{
    float *p_val;

    // 滤波器系数
    float b0;
    float b2;
    float a1;
    float a2;
    float qb0;
    float qb1;
    float qb2;

    // 输入序列
    float u[3]; // u[0]=ui(k), u[1]=ui(k-1), u[2]=ui(k-2)

    // 正交输出序列
    float osg_u[3]; // [0]=uo(k), [1]=uo(k-1), [2]=uo(k-2)

    // 正交信号输出序列
    float osg_qu[3]; // [0]=quo(k), [1]=quo(k-1), [2]=quo(k-2)

    // 配置参数
    float Ts;
    float w; // 中心频率 (rad/s)
    float k; // 阻尼系数

    float err;
} sogi_t;

// 初始化正交信号发生器
void sogi_init(sogi_t *sogi, float Ts, float w, float k, float *p_val);


// 更新正交信号发生器状态并计算输出
void sogi_cal(sogi_t *sogi);


// 更新中心频率并重新计算系数
void sogi_update_frequency(sogi_t *sogi, float new_w);

#endif // SOGI_H
