// SPDX-License-Identifier: MIT
/**
 * @file chk_grid.h
 * @brief chk_grid library public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Check grid voltage and frequency validity against configured thresholds
 *          - Track grid qualification state using timing and counter based filters
 *          - Provide reset and periodic validation logic for grid-connected control modules
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author Max.Li
 * @date 2026-05-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __CHK_GRID_H
#define __CHK_GRID_H
#include "stdint.h"

typedef struct
{
    float *p_rms;
    float *p_freq;
} chk_grid_input_t;

typedef struct
{
    float max;
    float min;
} chk_grid_lmt_t;

typedef struct
{
    chk_grid_lmt_t normal;
    chk_grid_lmt_t abnormal;
} chk_grid_normal_t;

typedef struct
{
    uint32_t judge_time;
    uint32_t abnormale_time;
    chk_grid_normal_t rms;
    chk_grid_normal_t freq;
} chk_grid_cfg_t;

typedef struct
{
    uint32_t is_ok_cnt;
    uint32_t abnormal_cnt;
} chk_grid_inter_t;

typedef struct
{
    uint8_t is_ok;
} chk_grid_output_t;

typedef struct
{
    chk_grid_input_t input;
    chk_grid_cfg_t cfg;
    chk_grid_inter_t inter;
    chk_grid_output_t output;
} chk_grid_t;

/**
 * @brief 绑定电网幅值、频率并清空资格历史。
 * @param p_str 调用方独占的电网检测模块。
 * @param p_rms 长期有效的电压有效值地址。
 * @param p_freq 长期有效的频率地址，可使用 Hz 或 rad/s；须与阈值单位一致。
 * @param judge_time 合格计数阈值；连续 judge_time + 1 次正常输入后置位。
 * @param abnormale_time 异常计数阈值；超过此次数后撤销资格，正常拍会逐次衰减。
 * @param rms_normal_max 启动电压上限，包含边界。
 * @param rms_normal_min 启动电压下限，包含边界。
 * @param rms_abnormal_max 已合格时的电压保持上限，包含边界。
 * @param rms_abnormal_min 已合格时的电压保持下限，包含边界。
 * @param freq_normal_max 启动频率上限，包含边界。
 * @param freq_normal_min 启动频率下限，包含边界。
 * @param freq_abnormal_max 已合格时的频率保持上限，包含边界。
 * @param freq_abnormal_min 已合格时的频率保持下限，包含边界。
 * @note 调用方在 INIT 保证地址、有限阈值、上下限顺序及有界计数有效。
 */
void chk_grid_init(chk_grid_t *p_str,
                   float *p_rms,
                   float *p_freq,
                   uint32_t judge_time,
                   uint32_t abnormale_time,
                   float rms_normal_max,
                   float rms_normal_min,
                   float rms_abnormal_max,
                   float rms_abnormal_min,
                   float freq_normal_max,
                   float freq_normal_min,
                   float freq_abnormal_max,
                   float freq_abnormal_min);

/**
 * @brief 根据本次输入推进资格或异常计数，更新 output.is_ok。
 * @param p_str 已初始化的独占实例；调用周期与配置计数相对应。
 * @note NaN 及无穷不满足有限阈值区间，按异常输入处理。
 */
void chk_grid_func(chk_grid_t *p_str);

void chk_grid_reset(chk_grid_t *p_str);

#endif
