// SPDX-License-Identifier: MIT
/**
 * @file sliding_window.c
 * @brief 固定长度浮点滑窗的初始化及逐点均值计算。
 * @details
 *          使用调用方提供的环形缓冲区，每拍移出一个旧点、加入一个新点。
 *          补偿累加保存舍入误差；窗口满后复用倒数系数，不分配内存或操作硬件。
 * @author Max.Li
 * @date 2026-10-02
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#include "sliding_window.h"

#include <stddef.h>

/**
 * @brief 在窗口累计值中加入一个带符号的项，并保留舍入误差。
 * @param[in,out] p_window 本窗口的累计值及补偿历史。
 * @param value 新点取正号，移出点取负号，单位与输入相同。
 * @note 每次执行固定数量的浮点加减；须使用保留浮点运算顺序的编译选项。
 */
static inline void sliding_window_accumulate(sliding_window_t *p_window, float value)
{
    const float corrected = value - p_window->inter.sum_error;
    const float sum_next = p_window->inter.sum + corrected;

    p_window->inter.sum_error = (sum_next - p_window->inter.sum) - corrected;
    p_window->inter.sum = sum_next;
}

/**
 * @brief 绑定固定长度缓冲区并清空窗口历史。
 * @param[in,out] p_window 调用方独占的滑窗模块。
 * @param[in,out] p_buffer 至少 length 个 float 的长寿命存储。
 * @param length 非零窗口点数。
 * @param[in] p_value 每拍输入地址，寿命覆盖所有计算调用。
 * @return 1：初始化成功；0：指针或长度无效，不修改原窗口。
 */
uint8_t sliding_window_init(sliding_window_t *p_window, float *p_buffer,
                            uint32_t length, const float *p_value)
{
    if (    (p_window == NULL)
         || (p_buffer == NULL)
         || (p_value == NULL)
         || (length == 0u))
    {
        return 0u;
    }

    p_window->input.p_value = p_value;
    p_window->input.length = length;
    p_window->inter.p_buffer = p_buffer;
    p_window->inter.index = 0u;
    p_window->inter.count = 0u;
    p_window->inter.sum = 0.0f;
    p_window->inter.sum_error = 0.0f;
    p_window->inter.mean_weight = 1.0f / (float)length;
    p_window->output.mean = 0.0f;

    for (uint32_t point = 0u; point < length; ++point)
    {
        p_buffer[point] = 0.0f;
    }
    return 1u;
}

/**
 * @brief 移出旧点、加入新点，并发布当前有效采样的均值。
 * @param[in,out] p_window 已成功初始化的滑窗模块；本函数每拍只读取输入一次。
 * @details 窗口未满时按实际点数平均；填满后复用窗长倒数，每拍工作量固定。
 */
void sliding_window_cal(sliding_window_t *p_window)
{
    const float value = *p_window->input.p_value;
    const uint32_t index = p_window->inter.index;

    sliding_window_accumulate(p_window, -p_window->inter.p_buffer[index]);
    sliding_window_accumulate(p_window, value);
    p_window->inter.p_buffer[index] = value;

    if (p_window->inter.count < p_window->input.length)
    {
        ++p_window->inter.count;
    }
    p_window->inter.index = index + 1u;

    if (p_window->inter.index == p_window->input.length)
    {
        p_window->inter.index = 0u;
    }
    p_window->output.mean = (p_window->inter.count == p_window->input.length)
                               ? p_window->inter.sum * p_window->inter.mean_weight
                               : p_window->inter.sum / (float)p_window->inter.count;
}
