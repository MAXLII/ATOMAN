// SPDX-License-Identifier: MIT
/**
 * @file    bsp_iir.c
 * @brief   Zynq-7020 AXI 3P3Z IIR peripheral driver.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Perform ordered 32-bit MMIO access to the PL IIR peripheral
 *          - Program all feedforward and feedback coefficients
 *          - Submit samples and poll deterministic PL completion
 *          - Execute a destructive closed-loop hardware self-test
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Runs in both no-RTOS and section SRTOS builds
 *          - The self-test restores coefficients but intentionally clears state
 *
 * @author  Max.Li
 * @date    2026-07-17
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "bsp_iir.h"

#include "xil_io.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#define BSP_IIR_SELF_TEST_TIMEOUT 100000U /* 单样本最大 AXI 状态轮询次数。 */
#define BSP_IIR_TEST_IMPULSE 1048576L     /* 精确二进制定标的脉冲输入。 */

static const bsp_iir_coefficients_t s_impulse_test_coefficients = {
    .b0 = 0x20000000L, /* +0.5。 */
    .b1 = 0x10000000L, /* +0.25。 */
    .b2 = 0x08000000L, /* +0.125。 */
    .b3 = 0x04000000L, /* +0.0625。 */
    .a1 = 0x10000000L, /* +0.25，公式中执行减法。 */
    .a2 = 0x08000000L, /* +0.125，公式中执行减法。 */
    .a3 = 0x04000000L, /* +0.0625，公式中执行减法。 */
};

static const int32_t s_impulse_expected[BSP_IIR_SELF_TEST_SAMPLE_COUNT] = {
    524288L,
    131072L,
    32768L,
    8192L,
    -14336L,
    512L,
    1152L,
    544L,
};

static uint8_t offset_is_valid(uint32_t offset)
{
    if (offset > (uint32_t)BSP_IIR_MAX_OFFSET)
    {
        return 0U;
    }

    if ((offset & 0x03U) != 0U)
    {
        return 0U;
    }

    return 1U;
}

static bsp_iir_coefficients_t coefficients_read(void)
{
    bsp_iir_coefficients_t coefficients = {0}; /* 当前 PL 系数快照。 */

    coefficients.b0 = (int32_t)bsp_iir_read_register((uint32_t)BSP_IIR_B0_OFFSET);
    coefficients.b1 = (int32_t)bsp_iir_read_register((uint32_t)BSP_IIR_B1_OFFSET);
    coefficients.b2 = (int32_t)bsp_iir_read_register((uint32_t)BSP_IIR_B2_OFFSET);
    coefficients.b3 = (int32_t)bsp_iir_read_register((uint32_t)BSP_IIR_B3_OFFSET);
    coefficients.a1 = (int32_t)bsp_iir_read_register((uint32_t)BSP_IIR_A1_OFFSET);
    coefficients.a2 = (int32_t)bsp_iir_read_register((uint32_t)BSP_IIR_A2_OFFSET);
    coefficients.a3 = (int32_t)bsp_iir_read_register((uint32_t)BSP_IIR_A3_OFFSET);

    return coefficients;
}

void bsp_iir_write_register(uint32_t offset, uint32_t value)
{
    if (offset_is_valid(offset) == 0U)
    {
        return;
    }

    Xil_Out32((UINTPTR)(BSP_IIR_BASE_ADDR + offset), value); /* 提交有序 AXI4-Lite 写事务。 */
}

uint32_t bsp_iir_read_register(uint32_t offset)
{
    if (offset_is_valid(offset) == 0U)
    {
        return 0U;
    }

    return Xil_In32((UINTPTR)(BSP_IIR_BASE_ADDR + offset)); /* 提交有序 AXI4-Lite 读事务。 */
}

uint8_t bsp_iir_configure(const bsp_iir_coefficients_t *coefficients)
{
    if (coefficients == NULL)
    {
        return 0U;
    }

    bsp_iir_write_register((uint32_t)BSP_IIR_B0_OFFSET, (uint32_t)coefficients->b0);
    bsp_iir_write_register((uint32_t)BSP_IIR_B1_OFFSET, (uint32_t)coefficients->b1);
    bsp_iir_write_register((uint32_t)BSP_IIR_B2_OFFSET, (uint32_t)coefficients->b2);
    bsp_iir_write_register((uint32_t)BSP_IIR_B3_OFFSET, (uint32_t)coefficients->b3);
    bsp_iir_write_register((uint32_t)BSP_IIR_A1_OFFSET, (uint32_t)coefficients->a1);
    bsp_iir_write_register((uint32_t)BSP_IIR_A2_OFFSET, (uint32_t)coefficients->a2);
    bsp_iir_write_register((uint32_t)BSP_IIR_A3_OFFSET, (uint32_t)coefficients->a3);

    return 1U;
}

void bsp_iir_reset_state(void)
{
    bsp_iir_write_register((uint32_t)BSP_IIR_CONTROL_OFFSET,
                           (uint32_t)BSP_IIR_CONTROL_RESET_STATE); /* 产生一个同步状态复位脉冲。 */
}

uint8_t bsp_iir_process_sample(int32_t input_sample,
                               int32_t *output_sample,
                               uint32_t *status,
                               uint32_t timeout_poll_count)
{
    uint32_t current_status = 0U; /* 当前 AXI STATUS 回读值。 */
    uint32_t poll_index = 0U;     /* 当前轮询次数。 */

    if ((output_sample == NULL) || (timeout_poll_count == 0U))
    {
        return 0U;
    }

    for (poll_index = 0U; poll_index < timeout_poll_count; ++poll_index)
    {
        current_status = bsp_iir_read_register((uint32_t)BSP_IIR_STATUS_OFFSET);
        if ((current_status & (uint32_t)BSP_IIR_STATUS_BUSY) == 0U)
        {
            break;
        }
    }
    if ((current_status & (uint32_t)BSP_IIR_STATUS_BUSY) != 0U)
    {
        return 0U;
    }

    bsp_iir_write_register((uint32_t)BSP_IIR_INPUT_OFFSET, (uint32_t)input_sample);
    bsp_iir_write_register((uint32_t)BSP_IIR_CONTROL_OFFSET,
                           (uint32_t)BSP_IIR_CONTROL_START);

    current_status = 0U;
    for (poll_index = 0U; poll_index < timeout_poll_count; ++poll_index)
    {
        current_status = bsp_iir_read_register((uint32_t)BSP_IIR_STATUS_OFFSET);
        if ((current_status & (uint32_t)BSP_IIR_STATUS_DONE) != 0U)
        {
            break;
        }
    }
    if ((current_status & (uint32_t)BSP_IIR_STATUS_DONE) == 0U)
    {
        return 0U;
    }

    *output_sample = (int32_t)bsp_iir_read_register((uint32_t)BSP_IIR_OUTPUT_OFFSET);
    if (status != NULL)
    {
        *status = current_status;
    }
    bsp_iir_write_register((uint32_t)BSP_IIR_CONTROL_OFFSET,
                           (uint32_t)BSP_IIR_CONTROL_CLEAR_DONE);

    return 1U;
}

uint8_t bsp_iir_self_test(bsp_iir_self_test_result_t *result)
{
    bsp_iir_coefficients_t saved_coefficients = coefficients_read(); /* 自测前系数。 */
    bsp_iir_coefficients_t saturation_coefficients = {0}; /* 双向饱和测试系数。 */
    int32_t saved_input = (int32_t)bsp_iir_read_register((uint32_t)BSP_IIR_INPUT_OFFSET); /* 自测前输入。 */
    int32_t output_sample = 0L; /* 当前 PL 输出。 */
    uint32_t status = 0U;       /* 当前样本完成状态。 */
    uint32_t version = 0U;      /* RTL 版本回读值。 */
    uint32_t format = 0U;       /* 定点格式回读值。 */
    uint32_t sample_index = 0U; /* 脉冲响应索引。 */
    uint8_t passed = 1U;        /* 全部 IIR 硬件检查结果。 */

    if (result != NULL)
    {
        for (sample_index = 0U; sample_index < BSP_IIR_SELF_TEST_SAMPLE_COUNT; ++sample_index)
        {
            result->impulse_output[sample_index] = 0L;
        }
        result->positive_saturation_output = 0L;
        result->negative_saturation_output = 0L;
        result->sample_count = 0U;
        result->final_status = 0U;
        result->version = 0U;
        result->format = 0U;
    }

    version = bsp_iir_read_register((uint32_t)BSP_IIR_VERSION_OFFSET);
    format = bsp_iir_read_register((uint32_t)BSP_IIR_FORMAT_OFFSET);
    if ((version != (uint32_t)BSP_IIR_VERSION_VALUE) ||
        (format != (uint32_t)BSP_IIR_FORMAT_VALUE))
    {
        passed = 0U;
    }

    bsp_iir_reset_state();
    if (bsp_iir_configure(&s_impulse_test_coefficients) == 0U)
    {
        passed = 0U;
    }

    for (sample_index = 0U; sample_index < BSP_IIR_SELF_TEST_SAMPLE_COUNT; ++sample_index)
    {
        const int32_t input_sample =
            (sample_index == 0U) ? (int32_t)BSP_IIR_TEST_IMPULSE : 0L;

        if (bsp_iir_process_sample(input_sample,
                                   &output_sample,
                                   &status,
                                   BSP_IIR_SELF_TEST_TIMEOUT) == 0U)
        {
            passed = 0U;
            break;
        }
        if (output_sample != s_impulse_expected[sample_index])
        {
            passed = 0U;
        }
        if ((status & (uint32_t)BSP_IIR_STATUS_SATURATED) != 0U)
        {
            passed = 0U;
        }
        if (result != NULL)
        {
            result->impulse_output[sample_index] = output_sample;
        }
    }

    if (bsp_iir_read_register((uint32_t)BSP_IIR_SAMPLE_COUNT_OFFSET) !=
        (uint32_t)BSP_IIR_SELF_TEST_SAMPLE_COUNT)
    {
        passed = 0U;
    }
    if ((bsp_iir_read_register((uint32_t)BSP_IIR_X1_OFFSET) != 0U) ||
        (bsp_iir_read_register((uint32_t)BSP_IIR_X2_OFFSET) != 0U) ||
        (bsp_iir_read_register((uint32_t)BSP_IIR_X3_OFFSET) != 0U) ||
        (bsp_iir_read_register((uint32_t)BSP_IIR_Y1_OFFSET) !=
         (uint32_t)s_impulse_expected[7]) ||
        (bsp_iir_read_register((uint32_t)BSP_IIR_Y2_OFFSET) !=
         (uint32_t)s_impulse_expected[6]) ||
        (bsp_iir_read_register((uint32_t)BSP_IIR_Y3_OFFSET) !=
         (uint32_t)s_impulse_expected[5]))
    {
        passed = 0U;
    }

    saturation_coefficients.b0 = INT32_MAX;
    bsp_iir_reset_state();
    (void)bsp_iir_configure(&saturation_coefficients);
    if ((bsp_iir_process_sample(INT32_MAX,
                                &output_sample,
                                &status,
                                BSP_IIR_SELF_TEST_TIMEOUT) == 0U) ||
        (output_sample != INT32_MAX) ||
        ((status & (uint32_t)BSP_IIR_STATUS_SATURATED) == 0U))
    {
        passed = 0U;
    }
    if (result != NULL)
    {
        result->positive_saturation_output = output_sample;
    }

    bsp_iir_reset_state();
    if ((bsp_iir_process_sample(INT32_MIN,
                                &output_sample,
                                &status,
                                BSP_IIR_SELF_TEST_TIMEOUT) == 0U) ||
        (output_sample != INT32_MIN) ||
        ((status & (uint32_t)BSP_IIR_STATUS_SATURATED) == 0U))
    {
        passed = 0U;
    }
    if (result != NULL)
    {
        result->negative_saturation_output = output_sample;
        result->sample_count = bsp_iir_read_register((uint32_t)BSP_IIR_SAMPLE_COUNT_OFFSET);
        result->final_status = status;
        result->version = version;
        result->format = format;
    }

    bsp_iir_reset_state();
    (void)bsp_iir_configure(&saved_coefficients);
    bsp_iir_write_register((uint32_t)BSP_IIR_INPUT_OFFSET, (uint32_t)saved_input);

    return passed;
}
