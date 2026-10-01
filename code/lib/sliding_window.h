// SPDX-License-Identifier: MIT
/**
 * @file sliding_window.h
 * @brief 调用方提供存储的浮点滑窗均值接口。
 * @details
 *          固定长度环形窗口；启动阶段按已填入点数求均值，满窗后每拍移出最旧点。
 *          每个实例独立保存输入、环形历史及输出，不分配内存，不包含平方或开方。
 *          Entity: 调用方持有实例、输入和缓冲区，窗口只维护自己的历史。
 *          Prior: INIT 验证地址和非零长度；调用方保证缓冲区容量及数值范围。
 *          Time: 单一执行上下文更新实例，每拍计算量固定，初始化清理长度有界。
 * @author Max.Li
 * @date 2026-10-02
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef SLIDING_WINDOW_H
#define SLIDING_WINDOW_H

#include <stdint.h>

typedef struct sliding_window
{
    struct
    {
        const float *p_value; /* 本拍输入地址，在整个运行期有效。 */
        uint32_t length;      /* 固定窗口点数，INIT 后不修改。 */
    } input;
    struct
    {
        float *p_buffer;      /* 调用方提供的 length 点环形历史。 */
        uint32_t index;       /* 下一写入位置，同时也是满窗后最旧点的位置。 */
        uint32_t count;       /* 已填入点数，最大为 length。 */
        float sum;            /* 当前有效窗口的累计值。 */
        float sum_error;      /* 补偿滚动累加的浮点舍入误差。 */
        float mean_weight;    /* 满窗后的 1/length，INIT 计算一次。 */
    } inter;
    struct
    {
        float mean;           /* 当前有效点的均值；INIT 后为 0。 */
    } output;
} sliding_window_t;

/**
 * @brief 初始化输入连接和固定长度窗口，清除上一轮历史。
 * @param p_window 调用方持有的窗口实例。
 * @param p_buffer 至少 length 个 float 的独立缓冲区，不与实例或输入地址重叠。
 * @param length 非零窗口点数；调用方保证不超过缓冲区容量。
 * @param p_value 长期有效的输入地址。
 * @return 1：初始化成功；0：地址为空或长度为零，实例及缓冲区不被修改。
 * @note 初始化与周期计算串行执行；输入及累计值须在 float 可表示范围内。
 */
uint8_t sliding_window_init(sliding_window_t *p_window, float *p_buffer,
                            uint32_t length, const float *p_value);

/**
 * @brief 加入一个新点，移出最旧点，并发布当前滑窗均值。
 * @param[in,out] p_window 已成功初始化的独占实例，只读取一次 input.p_value。
 * @details 启动阶段按 inter.count 求均值；count 达到 length 后为完整窗口。
 *          使用补偿累加减少长期滚动的舍入漂移；不遍历窗口，不重复检查绑定。
 * @note 本函数不滤除无效数值，计算异常会传播到输出，由调用方保护机制处理。
 */
void sliding_window_cal(sliding_window_t *p_window);

#endif /* SLIDING_WINDOW_H */
