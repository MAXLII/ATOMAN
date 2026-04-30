// SPDX-License-Identifier: MIT
/**
 * @file    sogi.c
 * @brief   sogi library module.
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
#include "sogi.h"
#include <string.h>

// 内部函数：重新计算滤波器系数
static void calculate_coefficients(sogi_t *sogi)
{
    float k = sogi->k;
    float w = sogi->w;
    float Ts = sogi->Ts;

    float n0 = 2 * Ts * k * w;
    float n1 = 0;
    float n2 = -2 * Ts * k * w;
    float d0 = Ts * Ts * w * w + 2 * Ts * k * w + 4;
    float d1 = 2 * Ts * Ts * w * w - 8;
    float d2 = Ts * Ts * w * w - 2 * Ts * k * w + 4;

    sogi->a1 = d1 / d0;
    sogi->a2 = d2 / d0;
    sogi->b0 = n0 / d0;
    sogi->b2 = n2 / d0;

    n0 = Ts * Ts * k * w * w;
    n1 = 2 * Ts * Ts * k * w * w;
    n2 = Ts * Ts * k * w * w;
    d0 = Ts * Ts * w * w + 2 * Ts * k * w + 4;
    d1 = 2 * Ts * Ts * w * w - 8;
    d2 = Ts * Ts * w * w - 2 * Ts * k * w + 4;

    sogi->qb0 = n0 / d0;
    sogi->qb1 = n1 / d0;
    sogi->qb2 = n2 / d0;
}

void sogi_init(sogi_t *sogi,
               float Ts,
               float w,
               float k,
               float *p_val)
{
    // 清零所有结构体成员
    memset(sogi, 0, sizeof(sogi_t));

    sogi->p_val = p_val;

    // 设置固定参数
    sogi->Ts = Ts;
    sogi->w = w;
    sogi->k = k;

    // 计算初始系数
    calculate_coefficients(sogi);
}

void sogi_cal(sogi_t *sogi)
{
    if (sogi->p_val == NULL)
    {
        return;
    }
    // 更新输入序列
    sogi->u[2] = sogi->u[1];
    sogi->u[1] = sogi->u[0];
    sogi->u[0] = *sogi->p_val;

    // 计算正交输出
    sogi->osg_u[0] = (sogi->b0 * (sogi->u[0] - sogi->u[2])) -
                     (sogi->a1 * sogi->osg_u[1]) -
                     (sogi->a2 * sogi->osg_u[2]);

    // 计算正交信号输出
    sogi->osg_qu[0] = (sogi->qb0 * sogi->u[0]) +
                      (sogi->qb1 * sogi->u[1]) +
                      (sogi->qb2 * sogi->u[2]) -
                      (sogi->a1 * sogi->osg_qu[1]) -
                      (sogi->a2 * sogi->osg_qu[2]);

    // 更新历史状态
    sogi->osg_u[2] = sogi->osg_u[1];
    sogi->osg_u[1] = sogi->osg_u[0];

    sogi->osg_qu[2] = sogi->osg_qu[1];
    sogi->osg_qu[1] = sogi->osg_qu[0];
    sogi->err = sogi->u[0] - sogi->osg_u[0];
}

void sogi_update_frequency(sogi_t *sogi, float new_w)
{
    // 更新频率
    sogi->w = new_w;

    // 重新计算系数
    calculate_coefficients(sogi);
}
