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

#include <stdint.h>

/**
 * @brief 读取采样观测器初始化状态，供 FSM INIT 使用。
 * @return 1：观测器已初始化；0：未就绪或初始化失败。
 * @note 初始化就绪不等同于本拍采样已经通过应用保护。
 */
uint8_t chb_ctrl_is_ready(void);

/**
 * @brief 从已锁定 HAL 取得原始采样及 app 的 RMS、角频率、相位，再更新控制观测。
 * @details 所有输入各读取一次；后续保护、控制和发波共用该拍副本，PLL 在 app 计算。
 * @note 由第一阶段采样或 FSM INIT 就绪检查调用，调用方保证绑定已就绪。
 */
void chb_ctrl_update_sample(void);

/**
 * @brief 复制 PWM 日志所需的同拍相角和正交电流，不暴露内部采样对象。
 * @param[out] p_theta_grid 电网相角，rad；调用方提供有效地址。
 * @param[out] p_i_grid_beta 正交电流，A；调用方提供有效地址。
 * @note 与采样串行调用；只读当前值，不参与控制反馈。
 */
void chb_ctrl_read_phase(float *p_theta_grid, float *p_i_grid_beta);

/**
 * @brief 在进入 RUN 前复位控制历史并准备母线参考斜坡。
 * @details 先关闭 PWM，再清积分；滤波和斜坡从最新预充母线状态开始。
 * @note 由 FSM 在尚未授予运行许可的串行边界调用，不与控制计算并发。
 */
void chb_ctrl_prepare_run(void);

/**
 * @brief 同步停止发波，并清除动态控制状态和本拍执行资格。
 * @details 不修改应用运行请求或 FSM 运行许可；重新运行须经过 prepare_run。
 */
void chb_ctrl_stop(void);

/**
 * @brief 应用保护在采样之后、控制之前禁止本拍控制及发波。
 * @details 撤销本拍资格；PLECS 构建保留首次故障快照。
 * @note 保护判据与紧急停波动作属于应用保护，本函数不重复执行阈值判断。
 */
void chb_ctrl_inhibit(void);

#endif /* CHB_CTRL_H */
