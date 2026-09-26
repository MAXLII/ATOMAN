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

/** @brief 在串行调度边界复位控制动态并准备新一轮运行。 */
void chb_ctrl_prepare_run(void);

/** @brief 停止发波并清除本拍控制许可。 */
void chb_ctrl_stop(void);

/** @brief 应用保护在采样之后、控制之前禁止本拍发波。 */
void chb_ctrl_inhibit(void);

#endif /* CHB_CTRL_H */
