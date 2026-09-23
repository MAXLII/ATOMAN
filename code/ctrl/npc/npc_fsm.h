// SPDX-License-Identifier: MIT
/**
 * @file npc_fsm.h
 * @brief Lifecycle states and bounded access to the FSM-owned control snapshot.
 * @details Base digital power framework. C11, static storage, no allocation.
 *          Bindings change only while stopped. PLECS serializes the dispatcher;
 *          ISR setpoints use the FSM publication snapshot.
 * @author Max.Li
 * @date 2026-09-13
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef NPC_FSM_H
#define NPC_FSM_H

#include "npc_cfg.h"

/** 状态机内部状态编号，与对外运行状态及故障码独立。 */
typedef enum
{
    NPC_FSM_STA_INIT = 1, /* 检查配置及硬件绑定。 */
    NPC_FSM_STA_IDLE,     /* 等待开机请求。 */
    NPC_FSM_STA_RUN       /* 维持运行或处理停机请求。 */
} NPC_FSM_STA_E;

/** 状态切换事件，由状态执行函数产生。 */
typedef enum
{
    NPC_FSM_EV_NULL = 0, /* 无状态切换请求。 */
    NPC_FSM_EV_TO_IDLE,  /* 请求进入待机。 */
    NPC_FSM_EV_TO_RUN    /* 请求进入运行。 */
} NPC_FSM_EV_E;

/** @return 当前对外运行状态，不包含故障诊断码。 */
NPC_RUN_STA_E npc_fsm_get_run_sta(void);

/**
 * @brief 有界读取已发布快照，不在中断中等待发布完成。
 * @param p_setpoint 调用方保证有效的快照接收地址；失败时目标内容保持不变。
 * @return 1：读到完整快照；0：发布正在进行或读取期间发生更新。
 */
uint8_t npc_fsm_read_published(npc_ctrl_setpoint_t *p_setpoint);

#endif /* NPC_FSM_H */
