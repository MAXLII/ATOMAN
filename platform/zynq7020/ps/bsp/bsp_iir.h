// SPDX-License-Identifier: MIT
/**
 * @file    bsp_iir.h
 * @brief   Zynq-7020 AXI 3P3Z IIR peripheral interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Define the PS-visible AXI 3P3Z register map
 *          - Configure signed Q2.30 filter coefficients
 *          - Configure signed lower and upper output limits
 *          - Execute blocking single-cycle sample transactions
 *          - Verify feedforward, feedback, limited history, and saturation
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Input and output share a caller-selected signed 32-bit scale
 *          - The self-test resets the PL filter history
 *
 * @author  Max.Li
 * @date    2026-07-25
 * @version 2.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef BSP_IIR_H
#define BSP_IIR_H

#include <stdint.h>

#define BSP_IIR_BASE_ADDR 0x43C00000UL /* AXI 3P3Z 外设基地址。 */

#define BSP_IIR_CONTROL_OFFSET 0x00UL      /* 写脉冲控制寄存器。 */
#define BSP_IIR_STATUS_OFFSET 0x04UL       /* 当前执行状态寄存器。 */
#define BSP_IIR_INPUT_OFFSET 0x08UL        /* 当前有符号 32 位输入样本。 */
#define BSP_IIR_OUTPUT_OFFSET 0x0CUL       /* 当前有符号 32 位输出样本。 */
#define BSP_IIR_B0_OFFSET 0x10UL           /* Q2.30 前向系数 b0。 */
#define BSP_IIR_B1_OFFSET 0x14UL           /* Q2.30 前向系数 b1。 */
#define BSP_IIR_B2_OFFSET 0x18UL           /* Q2.30 前向系数 b2。 */
#define BSP_IIR_B3_OFFSET 0x1CUL           /* Q2.30 前向系数 b3。 */
#define BSP_IIR_A1_OFFSET 0x20UL           /* Q2.30 反馈系数 a1。 */
#define BSP_IIR_A2_OFFSET 0x24UL           /* Q2.30 反馈系数 a2。 */
#define BSP_IIR_A3_OFFSET 0x28UL           /* Q2.30 反馈系数 a3。 */
#define BSP_IIR_SAMPLE_COUNT_OFFSET 0x2CUL /* 状态复位后完成的样本数。 */
#define BSP_IIR_VERSION_OFFSET 0x30UL      /* RTL 版本寄存器。 */
#define BSP_IIR_FORMAT_OFFSET 0x34UL       /* 数据宽度和系数小数位。 */
#define BSP_IIR_X1_OFFSET 0x38UL           /* 输入历史 x[n-1]。 */
#define BSP_IIR_X2_OFFSET 0x3CUL           /* 输入历史 x[n-2]。 */
#define BSP_IIR_X3_OFFSET 0x40UL           /* 输入历史 x[n-3]。 */
#define BSP_IIR_Y1_OFFSET 0x44UL           /* 输出历史 y[n-1]。 */
#define BSP_IIR_Y2_OFFSET 0x48UL           /* 输出历史 y[n-2]。 */
#define BSP_IIR_Y3_OFFSET 0x4CUL           /* 输出历史 y[n-3]。 */
#define BSP_IIR_LIMIT_LOWER_OFFSET 0x50UL  /* 有符号输出下限。 */
#define BSP_IIR_LIMIT_UPPER_OFFSET 0x54UL  /* 有符号输出上限。 */
#define BSP_IIR_MAX_OFFSET BSP_IIR_LIMIT_UPPER_OFFSET /* 最后一个有效寄存器偏移。 */

#define BSP_IIR_CONTROL_START 0x00000001UL       /* 启动一个样本计算。 */
#define BSP_IIR_CONTROL_RESET_STATE 0x00000002UL /* 清除历史、输出和样本计数。 */
#define BSP_IIR_CONTROL_CLEAR_DONE 0x00000004UL  /* 清除完成标志。 */

#define BSP_IIR_STATUS_BUSY 0x00000001UL      /* 兼容状态；单周期 core 始终为 0。 */
#define BSP_IIR_STATUS_DONE 0x00000002UL      /* 当前输出已更新。 */
#define BSP_IIR_STATUS_SATURATED 0x00000004UL /* 当前输出被配置上下限限幅。 */
#define BSP_IIR_STATUS_READY 0x00000008UL     /* 外设可以接收 START。 */

#define BSP_IIR_VERSION_VALUE 0x00020000UL /* 当前 RTL 版本。 */
#define BSP_IIR_FORMAT_VALUE 0x0000201EUL  /* 32 位样本、30 位系数小数。 */
#define BSP_IIR_Q30_ONE 0x40000000L        /* Q2.30 数值 +1.0。 */
#define BSP_IIR_SELF_TEST_SAMPLE_COUNT 8U  /* 脉冲响应自测样本数。 */

typedef struct
{
    int32_t b0; /* 当前样本的前向系数。 */
    int32_t b1; /* 一拍输入历史的前向系数。 */
    int32_t b2; /* 两拍输入历史的前向系数。 */
    int32_t b3; /* 三拍输入历史的前向系数。 */
    int32_t a1; /* 一拍输出历史的反馈系数。 */
    int32_t a2; /* 两拍输出历史的反馈系数。 */
    int32_t a3; /* 三拍输出历史的反馈系数。 */
} bsp_iir_coefficients_t;

typedef struct
{
    int32_t impulse_output[BSP_IIR_SELF_TEST_SAMPLE_COUNT]; /* 实测脉冲响应。 */
    int32_t positive_saturation_output; /* 正溢出饱和结果。 */
    int32_t negative_saturation_output; /* 负溢出饱和结果。 */
    int32_t upper_limited_output;        /* 可配置上限测试结果。 */
    int32_t lower_limited_output;        /* 可配置下限测试结果。 */
    int32_t limited_feedback_output;     /* 限幅历史反馈测试结果。 */
    uint32_t sample_count;               /* 自测阶段完成的样本数。 */
    uint32_t final_status;               /* 最后一个样本完成时状态。 */
    uint32_t version;                    /* RTL 版本回读值。 */
    uint32_t format;                     /* 定点格式回读值。 */
} bsp_iir_self_test_result_t;

/**
 * @brief 写入一个 IIR AXI 寄存器。
 * @param[in] offset 寄存器字节偏移，必须 4 字节对齐且不大于 0x54。
 * @param[in] value 要写入的原始 32 位值。
 */
void bsp_iir_write_register(uint32_t offset, uint32_t value);

/**
 * @brief 读取一个 IIR AXI 寄存器。
 * @param[in] offset 寄存器字节偏移，必须 4 字节对齐且不大于 0x54。
 * @return 寄存器原始 32 位值；偏移非法时返回 0。
 */
uint32_t bsp_iir_read_register(uint32_t offset);

/**
 * @brief 写入全部七个 Q2.30 系数。
 * @param[in] coefficients 系数集合。
 * @return 0 表示参数非法，1 表示写入完成。
 */
uint8_t bsp_iir_configure(const bsp_iir_coefficients_t *coefficients);

/**
 * @brief 配置有符号输出上下限。
 * @param[in] lower_limit 输出下限。
 * @param[in] upper_limit 输出上限。
 * @return 0 表示下限大于上限，1 表示写入完成。
 */
uint8_t bsp_iir_limit_configure(int32_t lower_limit, int32_t upper_limit);

/**
 * @brief 清除滤波历史、输出、状态和样本计数。
 */
void bsp_iir_reset_state(void);

/**
 * @brief 阻塞处理一个有符号 32 位样本。
 * @param[in] input_sample 当前输入样本。
 * @param[out] output_sample 当前输出样本。
 * @param[out] status 完成时状态，可传 NULL。
 * @param[in] timeout_poll_count 最大状态轮询次数。
 * @return 0 表示参数非法或超时，1 表示完成。
 */
uint8_t bsp_iir_process_sample(int32_t input_sample,
                               int32_t *output_sample,
                               uint32_t *status,
                               uint32_t timeout_poll_count);

/**
 * @brief 验证 AXI 访问、3P3Z 七项运算、历史状态和双向饱和。
 * @param[out] result 自测明细，可传 NULL。
 * @return 0 表示失败，1 表示全部测试通过。
 */
uint8_t bsp_iir_self_test(bsp_iir_self_test_result_t *result);

#endif /* BSP_IIR_H */
