// SPDX-License-Identifier: MIT
/**
 * @file chb_protect.h
 * @brief Application-owned sampled protection for the CHB rectifier.
 * @details Limits must be supplied for the actual model or hardware before FSM INIT completes.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef CHB_PROTECT_H
#define CHB_PROTECT_H

#include <stdint.h>

/**
 * @brief 配置同拍采样保护门限，仅允许在 FSM INIT 阶段调用。
 * @param current_trip_a 电网电流绝对值门限，A。
 * @param bus_min_v 每级母线最低有效电压，V。
 * @param bus_max_v 每级母线最高允许电压，V。
 * @return 1：配置有效；0：拒绝。
 */
uint8_t chb_protect_configure(float current_trip_a, float bus_min_v, float bus_max_v);

/** @return 1：保护配置齐全，FSM 可离开 INIT。 */
uint8_t chb_protect_is_ready(void);

/** @return 1：当前采样数值有效且位于应用保护范围内。 */
uint8_t chb_protect_sample_healthy(void);

/**
 * @brief 仅在 IDLE 或软起超时 FAULT 且明确停机后核实采样并清除应用保护闭锁。
 * @return 1：故障已清除；0：请求运行或采样仍越限。
 */
uint8_t chb_protect_clear_latch(void);

#endif /* CHB_PROTECT_H */
