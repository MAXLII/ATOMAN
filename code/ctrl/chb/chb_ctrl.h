// SPDX-License-Identifier: MIT
/**
 * @file chb_ctrl.h
 * @brief CHB control lifecycle and same-period protection inhibition.
 * @details Algorithm state and PWM diagnostics remain private to the controller.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef CHB_CTRL_H
#define CHB_CTRL_H

#include "chb_cfg.h"

#include <stdint.h>

/** @brief chb_ctrl.c 内部观测环节生成的同拍快照。 */
typedef struct chb_observer_sample
{
    float grid_v;     /* 电网侧本拍电压，V。 */
    float grid_rms_v; /* 电网基波有效值，V。 */
    float grid_hz;    /* 锁相频率，Hz。 */
    float theta_rad;  /* 本拍锁相角，rad。 */
    float i_alpha_a;  /* 电感电流物理采样，A。 */
    float i_beta_a;   /* 正交电流观测，A。 */
    float bus_v[CHB_CELL_COUNT];
    float load_i_a[CHB_CELL_COUNT];
    float grid_harmonic_alpha_v[CHB_HARMONIC_COUNT];
    float grid_harmonic_beta_v[CHB_HARMONIC_COUNT];
    float current_harmonic_alpha_a[CHB_HARMONIC_COUNT];
    float current_harmonic_beta_a[CHB_HARMONIC_COUNT];
    float grid_harmonic_peak_v[CHB_HARMONIC_COUNT];    /* 仅供诊断。 */
    float current_harmonic_peak_a[CHB_HARMONIC_COUNT]; /* 仅供诊断。 */
    float harmonic_feedback_weight; /* 稳频观测可信度，0..1。 */
    float grid_fundamental_beta_v;  /* 电网基波正交分量，V。 */
    float grid_frequency_rate_hz_s; /* 实测频率变化率，Hz/s。 */
    float grid_phase_error_rad;     /* PLL 相位检测误差，rad。 */
} chb_observer_sample_t;

/** @return 1：chb_ctrl 内部观测环节已初始化；0：不能运行。 */
uint8_t chb_observer_is_ready(void);

/** @return chb_ctrl 内部保存的本拍完整观测快照；调用方只读。 */
const chb_observer_sample_t *chb_observer_get_sample(void);

/** @brief 从已锁定的 HAL 输入指针取得同拍原始量并更新控制观测结果。 */
void chb_ctrl_update_sample(void);

/** @brief 在串行调度边界复位控制动态并准备新一轮运行。 */
void chb_ctrl_prepare_run(void);

/** @brief 停止发波并清除本拍控制许可。 */
void chb_ctrl_stop(void);

/** @brief 应用保护在采样之后、控制之前禁止本拍发波。 */
void chb_ctrl_inhibit(void);

#endif /* CHB_CTRL_H */
