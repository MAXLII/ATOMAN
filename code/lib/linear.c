// SPDX-License-Identifier: MIT
/**
 * @file    linear.c
 * @brief   linear library module.
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
#include "linear.h"
#include "stdbool.h"

bool linear_init(linear_t *p_str,
                 float *p_in,
                 float (*p_x_y)[2],
                 uint32_t size)
{
    // 拦截：至少要有两个点
    if (size < 2)
    {
        p_str->err = 1;
        return false;
    }

    // 拦截：检查 x 是否严格递增
    for (uint32_t i = 0; i < size - 1; i++)
    {
        if (p_x_y[i + 1][0] <= p_x_y[i][0])
        {
            p_str->err = 2;
            return false;
        }
    }

    // 通过检查，赋值
    p_str->p_in = p_in;
    p_str->p_x_y = p_x_y;
    p_str->size = size;
    p_str->out = 0.0f;
    p_str->last_index = 0;
    p_str->err = 0;

    return true;
}

void linear_func(linear_t *p_str)
{
    if (p_str->err)
        return;

    float x = *p_str->p_in;
    uint32_t i = p_str->last_index;
    uint32_t size = p_str->size;
    float (*table)[2] = p_str->p_x_y;

    // 边界处理
    if (x <= table[0][0])
    {
        p_str->out = table[0][1];
        p_str->last_index = 0;
        return;
    }
    if (x >= table[size - 1][0])
    {
        p_str->out = table[size - 1][1];
        p_str->last_index = size - 2;
        return;
    }

    // 向上搜索
    if (x >= table[i][0])
    {
        while ((i < size - 1) && (x >= table[i + 1][0]))
            i++;
    }
    // 向下搜索
    else
    {
        while ((i > 0) && (x < table[i][0]))
            i--;
    }

    // 插值
    float xi = table[i][0];
    float yi = table[i][1];
    float xi1 = table[i + 1][0];
    float yi1 = table[i + 1][1];

    float dx = xi1 - xi;
    float dy = yi1 - yi;
    p_str->out = yi + (dy / dx) * (x - xi);

    p_str->last_index = i;
}

void linear_func_bin(linear_t *p_str)
{
    if (p_str->err)
        return;

    float x = *p_str->p_in;
    uint32_t size = p_str->size;
    float (*table)[2] = p_str->p_x_y;

    // 边界处理
    if (x <= table[0][0])
    {
        p_str->out = table[0][1];
        p_str->last_index = 0;
        return;
    }
    if (x >= table[size - 1][0])
    {
        p_str->out = table[size - 1][1];
        p_str->last_index = size - 2;
        return;
    }

    // 二分查找
    uint32_t low = 0, high = size - 1, mid;
    while (high - low > 1)
    {
        mid = (low + high) >> 1;
        if (x < table[mid][0])
            high = mid;
        else
            low = mid;
    }

    // 插值
    float xi = table[low][0];
    float yi = table[low][1];
    float xi1 = table[low + 1][0];
    float yi1 = table[low + 1][1];

    float dx = xi1 - xi;
    float dy = yi1 - yi;
    p_str->out = yi + (dy / dx) * (x - xi);

    p_str->last_index = low;
}
