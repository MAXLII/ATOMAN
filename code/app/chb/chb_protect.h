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
 * @brief INIT 时连接 ctrl 持有的同拍静态采样与观测信号。
 * @details 所有指针只读，寿命覆盖运行期；保护阶段不再次读取 HAL。
 * @param p_v_grid 电网电压，V。
 * @param p_v_grid_rms 电网有效值，V。
 * @param p_f_grid 观测频率，Hz。
 * @param p_theta_grid 观测相角，rad。
 * @param p_i_grid_alpha 物理电流，A。
 * @param p_i_grid_beta 观测正交电流，A。
 * @param p_v_bus 各桥母线电压数组，V，长度 CHB_CELL_COUNT。
 * @param p_i_load 各桥负载电流数组，A，长度 CHB_CELL_COUNT。
 * @return 1：INIT 连线成功；0：运行阶段或输入地址无效。
 */
uint8_t chb_protect_set_input(const float *p_v_grid, const float *p_v_grid_rms,
                              const float *p_f_grid, const float *p_theta_grid,
                              const float *p_i_grid_alpha, const float *p_i_grid_beta,
                              const float *p_v_bus, const float *p_i_load);

/**
 * @brief 配置同拍采样保护门限，仅允许在 FSM INIT 阶段调用。
 * @param i_grid_trip 运行阶段电网电流绝对值门限，A。
 * @param i_precharge_trip 被动预充阶段电网电流绝对值门限，A。
 * @param v_bus_min 每级母线最低有效电压，V。
 * @param v_bus_max 每级母线最高允许电压，V。
 * @return 1：配置有效；0：拒绝。
 */
uint8_t                             chb_protect_configure(float i_grid_trip, float i_precharge_trip,
                              float v_bus_min, float v_bus_max);

/** @return 1：保护配置齐全，FSM 可离开 INIT。 */
uint8_t chb_protect_is_ready(void);

/** @return 1：当前采样数值有效且位于应用保护范围内。 */
uint8_t chb_protect_sample_healthy(void);

/**
 * @brief 仅在 IDLE 或软起超时 FAULT 且明确停机后核实采样并清除应用保护闭锁。
 * @return 1：故障已清除；0：请求运行或采样仍越限。
 */
uint8_t chb_protect_clear_latch(void);

/** @return 1：应用保护已闭锁；0：未闭锁。 */
uint8_t chb_protect_is_tripped(void);

#endif /* CHB_PROTECT_H */
