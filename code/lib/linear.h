// SPDX-License-Identifier: MIT
/**
 * @file    linear.h
 * @brief   linear library public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Provide piecewise-linear lookup and interpolation support
 *          - Map an input value through caller-provided x-y point tables
 *          - Offer sequential and binary-search based interpolation paths
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
#ifndef __LINEAR_H
#define __LINEAR_H

#include "stdint.h"
#include "stdbool.h"

typedef struct
{
    float *p_in;         // 输入变量指针
    float (*p_x_y)[2];   // 二维数组 [][2]，每行是 {x,y}
    uint32_t size;       // 数据点数量
    float out;           // 插值输出
    uint32_t last_index; // 上次区间索引
    uint8_t err;         // 错误标志
} linear_t;

bool linear_init(linear_t *p_str, float *p_in, float (*p_x_y)[2], uint32_t size);

void linear_func(linear_t *p_str);

void linear_func_bin(linear_t *p_str);

#endif
