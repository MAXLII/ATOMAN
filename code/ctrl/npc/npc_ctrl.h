// SPDX-License-Identifier: MIT
/**
 * @file npc_ctrl.h
 * @brief Control lifecycle; periodic computation and runtime data remain private.
 * @details Base digital power framework. C11, static storage, no allocation.
 *          Bindings change only while stopped. PLECS serializes the dispatcher;
 *          ISR setpoints use the FSM publication snapshot.
 * @author Max.Li
 * @date 2026-09-13
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef NPC_CTRL_H
#define NPC_CTRL_H

/** @brief 准备运行所需的配置和动态；调用方须与控制中断串行执行。 */
void npc_ctrl_prepare_run(void);

/** @brief 直接关闭 PWM 并清除控制动态；调用方须已完成 HAL 绑定；不清除保护闭锁。 */
void npc_ctrl_stop(void);

#endif /* NPC_CTRL_H */
